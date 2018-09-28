#ifndef _OPENTOKEN__HARE__CHECK_H_
#define _OPENTOKEN__HARE__CHECK_H_

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#ifdef DEBUG
#define dprintf(...) printf(__VA_ARGS__)
#define DCHECK(condition) CHECK(condition)
#else
#define dprintf(...)
#define DCHECK(condition)
#endif

#define CHECK(...) CHECK_impl(__FILE__, __LINE__, __VA_ARGS__)
#define CHECK_OK(...) CHECK_OK_impl(__FILE__, __LINE__, __VA_ARGS__)
#define CHECK_EQ(...) CHECK_EQ_impl(__FILE__, __LINE__, __VA_ARGS__)
#define CHECK_ALL(...) \
  CHECK_ALL_impl(__FILE__, __LINE__, __VA_ARGS__)  // not implemented
#define FAIL(...) FAIL_impl(__FILE__, __LINE__, __VA_ARGS__)
#define NOTNULL(...) NOTNULL_impl(__FILE__, __LINE__, __VA_ARGS__)

namespace {

template <typename... Args>
std::string string_format(const std::string& format, Args... args) {
  size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
  std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1);
}

}  // namespace

static inline bool str_eq(const char* a, const char* b) {
  return std::strcmp(a, b) == 0;
}

template <typename... Args>
__attribute__((__format__(__printf__, 4, 0)))  //
static inline void
CHECK_impl(const char* filename, int line, bool condition, const char* format,
           Args... args) {
  if (!condition) {
    fprintf(stderr, ("ERROR:%s:%d:" + std::string(format) + "\n").c_str(),
            filename, line, args...);
    exit(1);
  }
}

template <typename... Args>
__attribute__((__format__(__printf__, 4, 0)))  //
static inline void
CHECK_impl(const char* filename, int line, bool condition, const char* format,
           const std::string s, Args... args) {
  CHECK_impl(filename, line, condition, format, s.c_str(), args...);
}

template <typename... Args>
__attribute__((__format__(__printf__, 4, 0)))  //
static inline void
CHECK_OK_impl(const char* filename, int line, int status, const char* format,
              Args... args) {
  return CHECK_impl(filename, line, status == 0, format, args...);
}

static inline void CHECK_OK_impl(const char* filename, int line, int status) {
  return CHECK_impl(filename, line, status == 0, "status %d", status);
}

static inline void CHECK_impl(const char* filename, int line, bool condition) {
  CHECK_impl(filename, line, condition, "<no message>");
}

template <typename LHS, typename RHS>
static inline void CHECK_EQ_impl(const char* filename, int line, LHS&& lhs,
                                 RHS&& rhs) {
  return CHECK_impl(filename, line, lhs == rhs, "%s != %s", lhs, rhs);
}

template <typename... Args>
[[noreturn]] __attribute__((__format__(__printf__, 3, 0)))  //
static inline void
FAIL_impl(const char* filename, int line, const char* format, Args... args) {
  CHECK_impl(filename, line, false, format, args...);
  std::abort();
}

template <typename T>
static inline T* NOTNULL_impl(const char* filename, int line, T* ptr) {
  CHECK_impl(filename, line, ptr != nullptr, "ptr is null");
  return ptr;
}

#endif  // _OPENTOKEN__HARE__CHECK_H_
