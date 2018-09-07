#ifndef _OPENTOKEN_FAST_RLP_H_
#define _OPENTOKEN_FAST_RLP_H_

#include <iomanip>
#include <iostream>

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

constexpr uint8_t STRING_SB_MAX = 0x7f;
constexpr uint8_t STRING_SHORT_LEN_BASE = 0x80;
constexpr uint8_t STRING_SHORT_LEN_MAX = 0xb7;
constexpr uint8_t LIST_MIN = 0xc0;
constexpr uint8_t LIST_SHORT_LEN_BASE = 0xf7;

class FastRlpReader {
 public:
  FastRlpReader(const uint8_t* data, size_t data_size)
      : data_(data), data_size_(data_size) {}

  FastRlpReader(const char* data)
      : data_(reinterpret_cast<const uint8_t*>(data)),
        data_size_(std::strlen(data)) {}

  const uint8_t* data() const { return data_; }
  size_t data_size() const { return data_size_; }

  bool is_string() const { return s() < LIST_MIN; }
  bool is_list() const { return s() >= LIST_MIN; }

  size_t item_length() const {
    if (is_string()) {
      if (s() <= STRING_SB_MAX) {
        return 1;
      } else if (s() <= STRING_SHORT_LEN_MAX) {
        return s() - STRING_SHORT_LEN_BASE;
      } else {
        return long_len(s() - STRING_SHORT_LEN_MAX);
      }
    } else {
      if (s() <= LIST_SHORT_LEN_BASE) {
        return s() - LIST_SHORT_LEN_BASE;
      } else {
        return long_len(s() - LIST_SHORT_LEN_BASE);
      }
    }
  }

  std::pair<const uint8_t *, size_t> item() const {
    const uint8_t* item_data;
    if (s() <= STRING_SB_MAX) {
      item_data = data_;
    } else if (s() <= STRING_SHORT_LEN_MAX) {
      item_data = &data_[1];
    } else if (s() < LIST_MIN) {
      item_data = &data_[1 + (s() - STRING_SHORT_LEN_MAX)];
    } else if (s() <= LIST_SHORT_LEN_BASE) {
      item_data = &data_[1];
    } else {
      item_data = &data_[1 + (s() - LIST_SHORT_LEN_BASE)];
    }
    return {item_data, item_length()};
  }

 private:
  const uint8_t* const data_;
  const size_t data_size_;

  uint8_t s() const { return data_[0]; }
  size_t long_len(uint8_t lenlen) const {
    const auto mask = ~(std::numeric_limits<uint64_t>::max() >> (8 * lenlen));
    const size_t len = __builtin_bswap64(
        mask & *reinterpret_cast<const uint64_t*>(&data_[lenlen - 7]));

    return len;
  }
};

#endif  // _OPENTOKEN_FAST_RLP_H_
