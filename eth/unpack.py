import os
import plyvel
import rlp
import numba
from tqdm import tqdm
from rlp import sedes
from ethereum.utils import int256, hash32, address, mk_contract_address
from rlp.sedes import big_endian_int, Binary, binary, CountableList
from rlp.exceptions import ListDeserializationError
from ethereum.block import BlockHeader
from ethereum.transactions import Transaction


class BlockBody(rlp.Serializable):
  fields = [
    ('transaction_list', CountableList(Transaction)),
    ('uncles', CountableList(BlockHeader))
  ]

  def __repr__(self):
    return '<{}({} txns, {} uncles)>'.format(self.__class__.__name__,
                                             len(self.transaction_list),
                                             len(self.uncles))



# https://github.com/ethereum/go-ethereum/blob/461291882edce0ac4a28f64c4e8725b7f57cbeae/core/types/receipt.go#L46
class Receipt(rlp.Serializable):
  fields = [
      ('post_state_or_status', binary),
      ('cumulative_gas_used', big_endian_int),
      ('bloom', int256),
      ('hash', hash32),
      ('contract_address', address),
      ('logs', CountableList(sedes.raw)),
      ('gas_used', big_endian_int),
  ]


def iterate_over_block_headers(db, blocks):
  prefix = b'h'
  rlp_decode = rlp.decode

  it = db.prefixed_db(prefix).iterator()
  seek = it.seek
  for block_number in range(*blocks):
    seek(block_number.to_bytes(8, 'big'))
    while True:
      k, v = next(it)
      if len(k) > 9:
        yield rlp_decode(v, BlockHeader)
        break

def iterate_over_block_bodies(db, blocks):
  prefix = b'b'
  rlp_decode = rlp.decode
  rlp_format = BlockBody

  it = db.prefixed_db(prefix).iterator(include_key=False)
  seek = it.seek
  for block_number in range(*blocks):
    seek(block_number.to_bytes(8, 'big'))
    yield rlp_decode(next(it), rlp_format)

def iterate_over_receipts(db, blocks):
  prefix = b'r'
  rlp_decode = rlp.decode
  rlp_format = CountableList(Receipt)

  it = db.prefixed_db(prefix).iterator(include_key=False)
  seek = it.seek
  for block_number in range(*blocks):
    seek(block_number.to_bytes(8, 'big'))
    yield rlp_decode(next(it), rlp_format)


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
        contract_addr = mk_contract_address(t.sender, t.nonce)
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

    if i % 1000 == 0 and total:
      print('{:00.2f}% {} {}'.format(100 * unknown / total, unknown, total))
    i += 1


def main():
  db_path = os.path.expanduser('~/Library/Ethereum/geth/chaindata/')
  db = plyvel.DB(db_path)

  iterate_over_db(db)


if __name__ == '__main__':
  main()
