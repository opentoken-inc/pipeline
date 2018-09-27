#ifndef _OPENTOKEN__HARE__UTIL_H_
#define _OPENTOKEN__HARE__UTIL_H_

#include <string>

namespace opentoken {

template <size_t N>
std::string bin_to_hex(const uint8_t (&s)[N]) {
  constexpr auto hex = "0123456789ABCDEF";
  std::string result(2 * N, '\x00');
  for (size_t i = 0; i < N; ++i) {
    result[2 * i + 0] = hex[s[i] & 0x0F];
    result[2 * i + 1] = hex[s[i] >> 4];
  }
  return result;
}

class File final {
 public:
  File(const std::string& path, const char* mode = "r")
      : fp_(fopen(path.c_str(), mode)) {}

  FILE* fp() const { return fp_; }

  ~File() {
    auto fp = fp_;
    if (fp) {
      fclose(fp);
      fp_ = nullptr;
    }
  }

 private:
  FILE* fp_;
};

}  // namespace opentoken
#endif  // __OPENTOKEN__HARE__UTIL_H_
