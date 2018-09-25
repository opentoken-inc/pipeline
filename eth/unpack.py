import os
import sys
import plyvel
from ethereum.utils import mk_contract_address, sha3
from ethereum.transactions import Transaction as EthTransaction
from fast_rlp.fast_rlp import rlp_decode_typed, CountableList
from tqdm import tqdm

from eth.contract import check_contract_killto, check_contract_kill, get_balance


_TRANSFER_TOPICS = (bytes.fromhex('ddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef'),)

my_int = lambda x: int.from_bytes(x, 'big')
def my_int_256(x):
  assert len(x) == 256
  return int.from_bytes(x, 'big')

BlockHeader = (
  ('prevhash', None),
  ('uncles_hash', None),
  ('coinbase', None),
  ('state_root', None),
  ('tx_list_root', None),
  ('receipts_root', None),
  ('bloom', None),
  ('difficulty', None),
  ('number', my_int),
  ('gas_limit', None),
  ('gas_used', None),
  ('timestamp', my_int),
  ('extra_data', None),
  ('mixhash', None),
  ('nonce', None)
)

Transaction = (
  ('nonce', my_int),
  ('gasprice', my_int),
  ('startgas', my_int),
  ('to', None),
  ('value', my_int),
  ('data', None),
  ('v', my_int),
  ('r', my_int),
  ('s', my_int),
)

UnsignedTransaction = tuple(p for p in Transaction if p[0] not in 'vrs')

BlockBody = (
  ('transaction_list', CountableList(Transaction)),
  ('uncles', CountableList(BlockHeader))
)


LogRLP = (
    ('sender', None),
    ('topics', CountableList(None)),
    ('data', None),
    ('block_number', my_int),
    ('txn_hash', None),
    ('log_index', None),
    ('block_hash', None),
    ('txn_log_idx', None),
)

# https://github.com/ethereum/go-ethereum/blob/461291882edce0ac4a28f64c4e8725b7f57cbeae/core/types/receipt.go#L46
Receipt = (
    ('post_state_or_status', None),
    ('cumulative_gas_used', None),
    ('bloom', None),
    ('hash', None),
    ('contract_address', None),
    ('logs', CountableList(LogRLP)),
    ('gas_used', None),
)

def get_txn_sender(t):
  if hasattr(t, 'sender'):
    return t.sender

  t.sender = EthTransaction(**t.as_dict()).sender
  return t.sender


def iterate_over_block_headers(db, blocks):
  prefix = b'h'

  it = db.prefixed_db(prefix).iterator()
  seek = it.seek
  for block_number in range(*blocks):
    seek(block_number.to_bytes(8, 'big'))
    while True:
      k, v = next(it)
      if len(k) > 9:
        yield rlp_decode_typed(v, BlockHeader)
        break

def iterate_over_block_bodies(db, blocks):
  prefix = b'b'
  rlp_format = BlockBody

  it = db.prefixed_db(prefix).iterator(include_key=False)
  seek = it.seek
  for block_number in range(*blocks):
    seek(block_number.to_bytes(8, 'big'))
    yield rlp_decode_typed(next(it), rlp_format)

def iterate_over_receipts(db, blocks):
  prefix = b'r'
  rlp_format = CountableList(Receipt)

  it = db.prefixed_db(prefix).iterator(include_key=False)
  seek = it.seek
  for block_number in range(*blocks):
    seek(block_number.to_bytes(8, 'big'))
    yield rlp_decode_typed(next(it), rlp_format)

def process_log(t, l):
  if l.topics == _TRANSFER_TOPICS:
    pass


def iterate_over_db(db):
  ZERO_ADDRESS = bytes(20)
  blocks = 0, int(6e6)

  superset_of_contract_addrs = set()

  things = zip(
      iterate_over_block_headers(db, blocks),
      iterate_over_block_bodies(db, blocks), iterate_over_receipts(db, blocks))
  total = 0
  unknown = 0
  i = 0
  for header, body, receipts in tqdm(things, total=blocks[1] - blocks[0], disable=True):
    total += len(receipts)
    for t, r in zip(body.transaction_list, receipts):
      if not t.to:
        contract_addr = mk_contract_address(get_txn_sender(t), t.nonce)
        superset_of_contract_addrs.add(contract_addr)

      # Check for failure.
      if len(r.post_state_or_status) <= 1:
        success = len(r.post_state_or_status) == 1 and r.post_state_or_status[0] == b'\x01'
      elif t.startgas != r.gas_used or r.logs:
        success = True
      elif t.value == 0:
        # no logs, no value
        # Although it may have succeeded, we do not care because neither ETH nor tokens were
        # transferred.
        # TODO(mgraczyk): Not true, contract internal transfers.
        success = False
      elif t.startgas != 21000:
        success = False
        unknown += 1
      elif t.data:
        # No way to pay for attached data.
        success = False
      elif t.to not in superset_of_contract_addrs:
        # Definitely not sending to a contract.
        success = True
      else:
        success = False
        unknown += 1

      for l in r.logs:
        process_log(t, l)

    if i % 1000 == 0 and total:
      sys.stdout.write('{:00.2f}% {} {}   \r'.format(100 * unknown / total, unknown, total))
      sys.stdout.flush()
    i += 1


def main():
  db_path = os.path.expanduser('~/Library/Ethereum/geth/chaindata/')
  db = plyvel.DB(db_path)

  iterate_over_db(db)


if __name__ == '__main__':
  main()
