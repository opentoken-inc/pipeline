#ifndef _OPENTOKEN_FAST_RLP_H_
#define _OPENTOKEN_FAST_RLP_H_

#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>


constexpr uint8_t STRING_SB_MAX = 0x7f;
constexpr uint8_t STRING_SHORT_LEN_BASE = 0x80;
constexpr uint8_t STRING_SHORT_LEN_MAX = 0xb7;
constexpr uint8_t LIST_MIN = 0xc0;
constexpr uint8_t LIST_LONG_LEN_BASE = 0xf7;

using Bytes = const uint8_t*;

class FastRlpReader {
 public:
  FastRlpReader(Bytes data, size_t data_size) {
    if (!data or !data_size) {
      item_data_ = nullptr;
      item_length_ = 0;
      data_length_ = 0;
      return;
    }

    Bytes item_data;
    size_t data_length;
    size_t item_length;

    assert(data_size > 0);
    const auto s = data[0];

    if (s <= STRING_SB_MAX) {
      item_length = 1;
      data_length = item_length;
      item_data = &data[0];
    } else if (s <= STRING_SHORT_LEN_MAX) {
      item_length = s - STRING_SHORT_LEN_BASE;
      data_length = 1 + item_length;
      item_data = &data[1];
    } else if (s < LIST_MIN) {
      const auto lenlen = static_cast<uint8_t>(s - STRING_SHORT_LEN_MAX);
      item_length = long_len(data, lenlen);
      data_length = 1 + lenlen + item_length;
      item_data = &data[1 + lenlen];
    } else if (s <= LIST_LONG_LEN_BASE) {
      item_length = s - LIST_MIN;
      data_length = 1 + item_length;
      item_data = &data[1];
    } else {
      const auto lenlen = static_cast<uint8_t>(s - LIST_LONG_LEN_BASE);
      item_length = long_len(data, lenlen);
      data_length = 1 + lenlen + item_length;
      item_data = &data[1 + lenlen];
    }

    assert(data_length <= data_size);

    item_data_ = item_data;
    item_length_ = item_length;
    data_length_ = data_length;
    is_list_ = s >= LIST_MIN;
  }

  FastRlpReader(const char* data)
      : FastRlpReader(reinterpret_cast<const uint8_t*>(data),
                      std::strlen(data)) {}

  FastRlpReader() : FastRlpReader({}, {}) {}

  bool is_null() const { return !item_data_; }
  bool is_list() const { return is_list_; }
  bool is_string() const { return !is_list(); }

  Bytes item_data() const { return item_data_; }
  size_t item_length() const { return item_length_; }
  size_t data_length() const { return data_length_; }

 private:
  Bytes item_data_;
  size_t item_length_;
  size_t data_length_;
  bool is_list_;

  static size_t long_len(Bytes data, uint8_t lenlen) {
    const auto mask = ~(std::numeric_limits<uint64_t>::max() >> (8 * lenlen));
    const size_t len = __builtin_bswap64(
        mask & *reinterpret_cast<const uint64_t*>(&data[lenlen - 7]));

    return len;
  }
};

#endif  // _OPENTOKEN_FAST_RLP_H_
