#ifndef _OPENTOKEN__HARE__HASHER_H_
#define _OPENTOKEN__HARE__HASHER_H_

#include "check.h"

#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <iostream>
#include <string>

namespace opentoken {

constexpr size_t kMaxPrecomputed = 3;
constexpr size_t kHashSizeBytes = 32;

class Hasher final {
 public:
  Hasher(std::string key) : base_ctx_(init_base_context(key)) {}
  Hasher(const char *key) : Hasher(std::string{CHECK_NOTNULL(key)}) {}

  ~Hasher() { HMAC_CTX_cleanup(&base_ctx_); }

  void hash(const uint8_t *hash_input, size_t input_size,
            uint8_t output[kHashSizeBytes]) {
    CHECK(HMAC_Init_ex(&base_ctx_, nullptr, 0, nullptr, nullptr) == 1);
    CHECK(HMAC_Update(&base_ctx_, hash_input, input_size) == 1);

    unsigned len;
    CHECK(HMAC_Final(&base_ctx_, output, &len) == 1);
    CHECK(len == kHashSizeBytes, "len was %u", len);
  }

  void hash(const char *hash_input, size_t input_size,
            uint8_t output[kHashSizeBytes]) {
    hash(reinterpret_cast<const uint8_t *>(hash_input), input_size, output);
  }

  void hash(uint64_t id, uint8_t output[kHashSizeBytes]) {
    if (id_offset_ <= id && id < id_offset_ + num_precomputed_) {
      memcpy(output, precomputed_[id - id_offset_], kHashSizeBytes);
    } else {
      CHECK(HMAC_Init_ex(&base_ctx_, nullptr, 0, nullptr, nullptr) == 1);
      CHECK(HMAC_Update(&base_ctx_, (uint8_t *)&id, sizeof(id)) == 1);

      unsigned len;
      CHECK(HMAC_Final(&base_ctx_, output, &len) == 1);
      CHECK(len == kHashSizeBytes, "len was %u", len);
    }
  }

  bool is_valid_signature(const uint8_t *input_data, size_t input_size,
                          const uint8_t signature[kHashSizeBytes]) {
    uint8_t computed_signature[kHashSizeBytes];
    hash(input_data, input_size, computed_signature);
    return CRYPTO_memcmp(signature, computed_signature, kHashSizeBytes) == 0;
  }

 private:
  HMAC_CTX base_ctx_;
  uint64_t id_offset_ = 0;
  uint8_t precomputed_[kMaxPrecomputed][kHashSizeBytes] = {};
  size_t num_precomputed_ = 0;

  static HMAC_CTX init_base_context(std::string key) {
    CHECK(!key.empty());
    ENGINE_load_builtin_engines();
    ENGINE_register_all_complete();

    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);
    CHECK(HMAC_Init_ex(&ctx, (void *)key.c_str(), (int)key.size(), EVP_sha256(),
                       nullptr) == 1);
    return ctx;
  }
};

}  // namespace opentoken

#endif  // _OPENTOKEN__HARE__HASHER_H_
