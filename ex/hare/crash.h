#ifndef _OPENTOKEN__HARE__CRASH_H_
#define _OPENTOKEN__HARE__CRASH_H_

#include <cstdlib>
#include <memory>
#include <string>

namespace {

template <typename... Args>
std::string string_format(const std::string& format, Args... args) {
  size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
  std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1);
}

}  // namespace

template <typename... Args>
static inline void CHECK_impl(const char* filename, int line, bool condition,
                              const char* format, Args... args) {
  if (!condition) {
    fprintf(stderr, ("ERROR:%s:%d:" + std::string(format) + "\n").c_str(),
            filename, line, args...);
    exit(1);
  }
}

template <typename... Args>
static inline void CHECK_OK_impl(const char* filename, int line, int status,
                                 const char* format, Args... args) {
  return CHECK_impl(filename, line, status == 0, format, args...);
}

static inline void CHECK_OK_impl(const char* filename, int line, int status) {
  return CHECK_impl(filename, line, status == 0, "status %d", status);
}

static inline void CHECK_impl(const char* filename, int line, bool condition) {
  CHECK_impl(filename, line, condition, "");
}

template <typename... Args>
[[noreturn]] static inline void FAIL_impl(const char* filename, int line,
                                          const char* format, Args... args) {
  CHECK_impl(filename, line, false, format, args...);
  std::abort();
}

#define CHECK(...) CHECK_impl(__FILE__, __LINE__, __VA_ARGS__)
#define CHECK_OK(...) CHECK_OK_impl(__FILE__, __LINE__, __VA_ARGS__)
#define FAIL(...) FAIL_impl(__FILE__, __LINE__, __VA_ARGS__)

#endif  // _OPENTOKEN__HARE__CRASH_H_
