#ifndef _OPENTOKEN__HARE__UTIL_H_
#define _OPENTOKEN__HARE__UTIL_H_

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <string>

namespace opentoken {

template <size_t N>
std::string bin_to_hex(const uint8_t (&s)[N]) {
  constexpr auto hex = "0123456789ABCDEF";
  std::string result(2 * N, '\x00');
  for (size_t i = 0; i < N; ++i) {
    result[2 * i + 0] = hex[s[i] >> 4];
    result[2 * i + 1] = hex[s[i] & 0x0F];
  }
  return result;
}

class File final {
 public:
  File() = default;
  File(const std::string& path, const char* mode = "r")
      : fp_(fopen(path.c_str(), mode)) {
    CHECK_NOTNULL(fp_);
  }

  FILE* f() const { return fp_; }
  operator FILE*() const { return fp_; }
  int fp() const { return ::fileno(fp_); }

  ~File() {
    auto fp = fp_;
    if (fp && (::fileno(fp) < 0 || ::fileno(fp) > 2)) {
      fclose(fp);
    }
    fp_ = nullptr;
  }

 private:
  File(File&) = delete;
  File(File&&) = delete;

  FILE* fp_;
};

class PosixFile final {
 public:
  PosixFile() = default;
  PosixFile(const std::string& path, int oflag)
      : fd_(open(path.c_str(), oflag)) {
    CHECK(fd_ >= 0);
  }

  int fd() const { return fd_; }

  ~PosixFile() { close(fd_); }

 private:
  PosixFile(PosixFile&) = delete;
  PosixFile(PosixFile&&) = delete;

  int fd_;
};

}  // namespace opentoken
#endif  // __OPENTOKEN__HARE__UTIL_H_
