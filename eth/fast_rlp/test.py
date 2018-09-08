import rlp
from fast_rlp import *
import timeit


def test_one(obj):
  encoded = rlp.encode(obj)
  assert rlp.decode(encoded) == obj
  print(encoded.hex())
  print(len(encoded))
  print('')

  decoded_fast = rlp_decode_raw(encoded)
  print(obj)
  print(decoded_fast)
  assert obj == decoded_fast
  print('*'*80)

def run_tests():
  test_one(b'a')
  test_one(b'abc')
  test_one([])
  test_one([b'a'])

  test_one([b'aaa'])

  test_one([[[b'abc']]])

  test_one([b'a', b'b', [[]], [[], [b'cdef', b'\x01'*56]]])

  test_one([b'a', b'b', [[]], [[], [b'cdef', b'\x01'*56]]])
  try:
    print(rlp.decode(b''))
    assert False
  except rlp.exceptions.DecodingError:
    pass

  try:
    print(rlp_decode_raw(b''))
    assert False
  except ValueError:
    pass

# run_tests()

# Performance
def run(stmt, setup_code, N=100000):
  M = 10

  timeit.timeit(stmt=stmt, setup=setup_code, number=M)
  result = timeit.timeit(stmt=stmt, setup=setup_code, number=N)
  rate = result / N
  print(rate)
  return rate

def profile_raw():
  setup_code = """
import rlp
from fast_rlp import rlp_decode_raw

with open('big_receipt.txt') as f:
  header = bytes.fromhex(f.read().strip())
rlp_decode = rlp.decode

assert rlp_decode(header) == rlp_decode_raw(header)
  """

  stmt_A = "rlp_decode_raw(header)"
  stmt_B = "rlp_decode(header)"

  print('Running A')
  rateA = run(stmt_A, setup_code, 20000)

  print('Running B')
  rateB = run(stmt_B, setup_code, 20000)

  print('{}x speedup'.format(rateB / rateA))

# profile_raw()


def profile_typed():
  setup_code = """
import rlp
from ethereum.utils import int256, hash32, address
from rlp.sedes import big_endian_int, binary, CountableList
from fast_rlp import rlp_decode_typed, CountableList as MyCountableList

class LogRLP(rlp.Serializable):
  fields = [
    ('sender', address),
    ('topics', CountableList(hash32)),
    ('data', binary),
    ('block_number', big_endian_int),
    ('txn_hash', hash32),
    ('log_index', big_endian_int),
    ('block_hash', hash32),
    ('txn_log_idx', big_endian_int),
  ]


# https://github.com/ethereum/go-ethereum/blob/461291882edce0ac4a28f64c4e8725b7f57cbeae/core/types/receipt.go#L46
class Receipt(rlp.Serializable):
  fields = [
      ('post_state_or_status', binary),
      ('cumulative_gas_used', big_endian_int),
      ('bloom', int256),
      ('hash', hash32),
      ('contract_address', address),
      ('logs', CountableList(LogRLP)),
      ('gas_used', big_endian_int),
  ]
type_spec = CountableList(Receipt)


my_int = lambda x: int.from_bytes(x, 'big')
def my_int_256(x):
  assert len(x) == 256
  return int.from_bytes(x, 'big')

MyLogRLP = (
  ('sender', None),
  ('topics', MyCountableList(None)),
  ('data', None),
  ('block_number', my_int),
  ('txn_hash', None),
  ('log_index', my_int),
  ('block_hash', None),
  ('txn_log_idx', my_int),
)

MyReceipt = (
    ('post_state_or_status', None),
    ('cumulative_gas_used', my_int),
    ('bloom', my_int_256),
    ('hash', None),
    ('contract_address', None),
    ('logs', MyCountableList(MyLogRLP)),
    ('gas_used', my_int),
)
my_type_spec = MyCountableList(MyReceipt)


with open('big_receipt.txt') as f:
  receipt_bytes = bytes.fromhex(f.read().strip())
rlp_decode = rlp.decode

rlp_decode(receipt_bytes, type_spec)
rlp_decode_typed(receipt_bytes, my_type_spec)
  """

  stmt_A = "rlp_decode_typed(receipt_bytes, my_type_spec)"
  stmt_B = "rlp_decode(receipt_bytes, type_spec)"

  print('Running A')
  rateA = run(stmt_A, setup_code, 5000)

  print('Running B')
  rateB = run(stmt_B, setup_code, 5000)

  print('{}x speedup'.format(rateB / rateA))


profile_typed()
