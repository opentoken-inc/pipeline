import pyximport
importers = pyximport.install()
try:
  from .fast_rlp_cython import rlp_decode_raw, rlp_decode_typed, CountableList
finally:
  pyximport.uninstall(*importers)
