from libc.stdint cimport uint8_t
from cpython.bytes cimport PyBytes_FromStringAndSize

ctypedef const uint8_t* Bytes;

cdef extern from "fast_rlp.h" nogil:
  cdef cppclass FastRlpReader:
    FastRlpReader()
    FastRlpReader(Bytes data, size_t data_size)

    bint is_list() const
    bint is_string() const

    Bytes item_data() const
    size_t item_length() const
    size_t data_length() const


class CountableList:
  def __init__(self, inner_type):
    self.inner_type = inner_type


class RLPStruct:
  def __init__(self, keys):
    self.__dict__.update({k: None for k in keys})

  def as_dict(self):
    return {k: v.as_dict() if isinstance(v, RLPStruct) else v for k, v in self.__dict__.items() }

cdef object rlp_decode_into_object(Bytes data, Py_ssize_t size):
  if size == 0:
    raise ValueError('size cannot be zero')

  cdef FastRlpReader reader = FastRlpReader(data, <size_t>size)
  cdef Bytes item_data = reader.item_data()
  cdef Bytes sub_data
  cdef Bytes list_end
  cdef Py_ssize_t sub_size
  if reader.is_list():
    sub_data = item_data
    list_end = data + <Py_ssize_t>reader.data_length()
    result = []
    while sub_data < list_end:
      sub_size = size - (sub_data - data)
      result.append(rlp_decode_into_object(sub_data, sub_size))
      sub_data += FastRlpReader(sub_data, <size_t>sub_size).data_length()
    return result
  else:
    return PyBytes_FromStringAndSize(<char*>item_data, <Py_ssize_t>reader.item_length())


cdef object rlp_decode_into_object_with_spec(Bytes data, Py_ssize_t size, object decode_spec):
  if size == 0:
    raise ValueError('size cannot be zero')

  cdef FastRlpReader reader = FastRlpReader(data, <size_t>size)
  cdef Bytes item_data = reader.item_data()
  cdef Bytes sub_data
  cdef Bytes list_end
  cdef object result
  cdef Py_ssize_t sub_size
  if reader.is_list():
    sub_data = item_data
    list_end = data + <Py_ssize_t>reader.data_length()
    i = 0
    if decode_spec is None or isinstance(decode_spec,  CountableList):
      result = []
      while sub_data < list_end:
        sub_size = size - (sub_data - data)
        sub_spec = None if decode_spec is None else decode_spec.inner_type
        result.append(rlp_decode_into_object_with_spec(sub_data, sub_size, sub_spec))
        sub_data += FastRlpReader(sub_data, <size_t>sub_size).data_length()
        i += 1
    else:
      result = RLPStruct(d[0] for d in decode_spec)
      rd = result.__dict__
      while sub_data < list_end:
        sub_size = size - (sub_data - data)
        sub_name, sub_spec = decode_spec[i]
        rd[sub_name] = rlp_decode_into_object_with_spec(sub_data, sub_size, sub_spec)
        sub_data += FastRlpReader(sub_data, <size_t>sub_size).data_length()
        i += 1
    return result
  else:
    result = PyBytes_FromStringAndSize(<char*>item_data, <Py_ssize_t>reader.item_length())
    if decode_spec is None:
      return result
    else:
      return decode_spec(result)


def rlp_decode_raw(bytes data):
  cdef Bytes p = data
  return rlp_decode_into_object(p, len(data))

def rlp_decode_typed(bytes data, object decode_spec):
  cdef Bytes p = data
  return rlp_decode_into_object_with_spec(p, len(data), decode_spec)
