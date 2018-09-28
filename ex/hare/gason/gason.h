#pragma once

#include "../check.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace gason {

enum class JsonTag {
  JSON_NUMBER = 0,
  JSON_STRING,
  JSON_ARRAY,
  JSON_OBJECT,
  JSON_TRUE,
  JSON_FALSE,
  JSON_NULL = 0xF,
};

struct JsonNode;

#define JSON_VALUE_PAYLOAD_MASK 0x00007FFFFFFFFFFFULL
#define JSON_VALUE_NAN_MASK 0x7FF8000000000000ULL
#define JSON_VALUE_TAG_MASK 0xF
#define JSON_VALUE_TAG_SHIFT 47

union JsonValue {
  uint64_t ival;
  double fval;

  JsonValue(double x) : fval(x) {}
  JsonValue(JsonTag tag = JsonTag::JSON_NULL, void *payload = nullptr) {
    CHECK((uintptr_t)payload <= JSON_VALUE_PAYLOAD_MASK);
    ival = JSON_VALUE_NAN_MASK | ((uint64_t)tag << JSON_VALUE_TAG_SHIFT) |
           (uintptr_t)payload;
  }
  bool isDouble() const {
    return (int64_t)ival <= (int64_t)JSON_VALUE_NAN_MASK;
  }
  JsonTag getTag() const {
    return isDouble()
               ? JsonTag::JSON_NUMBER
               : JsonTag((ival >> JSON_VALUE_TAG_SHIFT) & JSON_VALUE_TAG_MASK);
  }
  uint64_t getPayload() const {
    assert(!isDouble());
    return ival & JSON_VALUE_PAYLOAD_MASK;
  }
  double toNumber() const {
    assert(getTag() == JsonTag::JSON_NUMBER);
    return fval;
  }
  char *toString() const {
    assert(getTag() == JsonTag::JSON_STRING);
    return (char *)getPayload();
  }
  JsonNode *toNode() const {
    assert(getTag() == JsonTag::JSON_ARRAY || getTag() == JsonTag::JSON_OBJECT);
    return (JsonNode *)getPayload();
  }

  double toNumberAlways() const {
    if (getTag() == JsonTag::JSON_NUMBER) {
      return toNumber();
    } else if (getTag() == JsonTag::JSON_STRING) {
      return std::atof(toString());
    } else {
      FAIL("Bad tag %d", getTag());
    }
  }

  void assignTo(uint64_t *dest) const {
    *dest = static_cast<uint64_t>(toNumberAlways());
  }

  void assignTo(double *dest) const { *dest = toNumberAlways(); }
};

struct JsonNode {
  JsonValue value;
  JsonNode *next;
  char *key;
};

struct JsonIterator {
  JsonNode *p;

  void operator++() { p = p->next; }
  bool operator!=(const JsonIterator &x) const { return p != x.p; }
  JsonNode *operator*() const { return p; }
  JsonNode *operator->() const { return p; }
};

inline JsonIterator begin(JsonValue o) { return JsonIterator{o.toNode()}; }
inline JsonIterator end(JsonValue) { return JsonIterator{nullptr}; }

#define JSON_ERRNO_MAP(XX)                         \
  XX(OK, "ok")                                     \
  XX(BAD_NUMBER, "bad number")                     \
  XX(BAD_STRING, "bad string")                     \
  XX(BAD_IDENTIFIER, "bad identifier")             \
  XX(STACK_OVERFLOW, "stack overflow")             \
  XX(STACK_UNDERFLOW, "stack underflow")           \
  XX(MISMATCH_BRACKET, "mismatch bracket")         \
  XX(UNEXPECTED_CHARACTER, "unexpected character") \
  XX(UNQUOTED_KEY, "unquoted key")                 \
  XX(BREAKING_BAD, "breaking bad")                 \
  XX(ALLOCATION_FAILURE, "allocation failure")

enum class JsonErrno {
#define XX(no, str) JSON_##no,
  JSON_ERRNO_MAP(XX)
#undef XX
};

const char *jsonStrError(JsonErrno err);

class JsonAllocator {
  struct Zone {
    Zone *next;
    size_t used;
  } * head;

 public:
  JsonAllocator() : head(nullptr){};
  JsonAllocator(const JsonAllocator &) = delete;
  JsonAllocator &operator=(const JsonAllocator &) = delete;
  JsonAllocator(JsonAllocator &&x) : head(x.head) { x.head = nullptr; }
  JsonAllocator &operator=(JsonAllocator &&x) {
    head = x.head;
    x.head = nullptr;
    return *this;
  }
  ~JsonAllocator() { deallocate(); }
  void *allocate(size_t size);
  void deallocate();
};

JsonErrno jsonParse(char *str, char **endptr, JsonValue *value,
                    JsonAllocator &allocator);

template <typename... Args>
static inline void CHECK_OK_impl(const char *filename, int line,
                                 gason::JsonErrno err, const char *format,
                                 Args... args) {
  return CHECK_impl(filename, line, err == gason::JsonErrno::JSON_OK, format,
                    args...);
}

static inline void CHECK_impl(const char *filename, int line,
                              gason::JsonErrno err) {
  CHECK_OK_impl(filename, line, err, "%s");
}

}  //  namespace gason
