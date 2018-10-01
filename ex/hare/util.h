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
    CHECK_ERRNO(fd_ >= 0);
  }

  int fd() const { return fd_; }

  ~PosixFile() { close(fd_); }

 private:
  PosixFile(PosixFile&) = delete;
  PosixFile(PosixFile&&) = delete;

  int fd_;
};

class FileLineReader final {
 public:
  FileLineReader() = default;

  int fd() const { return ::fileno(f_); }
  bool has_next() const { return !done_; }

  char* read_line() {
    size_t len = 0;
    do {
      if (getline(&line_, &len, f_) < 0) {
        if (errno == EAGAIN || errno == EINTR) continue;
        CHECK_ERRNO(false);
      }
    } while (false);
    if (len > 0) {
      return line_;
    } else {
      done_ = true;
      return nullptr;
    }
  }

  ~FileLineReader() {
    if (line_) {
      std::free(line_);
      line_ = nullptr;
    }
  }

 private:
  FileLineReader(FileLineReader&) = delete;
  FileLineReader(FileLineReader&&) = delete;

  FILE* const f_ = stdin;
  char* line_ = nullptr;
  bool done_ = false;
};

}  // namespace opentoken
#endif  // __OPENTOKEN__HARE__UTIL_H_
