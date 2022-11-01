#include "crypto.h"

#include <openssl/aes.h>
#include <openssl/evp.h>

#include <memory>

#include "result.h"

namespace bolo_crypto {

// at least 8 bytes
constexpr unsigned char SALT[] = "hellobolo";
using namespace std::string_literals;

template <typename InitFunc>
bolo::Insidious<std::string> InitEVPContext(const std::string &key_data, Scheme s, InitFunc fn,
                                            EVP_CIPHER_CTX *ctx) {
  unsigned char key[32], iv[32];
  /*
   * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
   * nrounds is the number of times the we hash the material. More rounds are more secure but
   * slower.
   */
  int r = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), SALT,
                         reinterpret_cast<const unsigned char *>(key_data.c_str()), key_data.size(),
                         2, key, iv);
  if (r != 32) return bolo::Danger("size of key is not 32 bytes"s);

  EVP_CIPHER_CTX_init(ctx);
  fn(ctx, EVP_aes_256_cbc(), key, iv);
  return bolo::Safe;
}

bolo::Insidious<std::string> InitEVPEncryptCtx(const std::string &key_data, Scheme s,
                                               EVP_CIPHER_CTX *ctx) {
  return InitEVPContext(key_data, s, EVP_EncryptInit, ctx);
}

bolo::Insidious<std::string> InitEVPDecryptCtx(const std::string &key_data, Scheme s,
                                               EVP_CIPHER_CTX *ctx) {
  return InitEVPContext(key_data, s, EVP_DecryptInit, ctx);
}

bolo::Insidious<std::string> Encrypt(std::istream &in, std::ostream &out, const std::string &key,
                                     Scheme s) {
  constexpr int buf_length = 512;
  unsigned char ibuf[buf_length];
  unsigned char obuf[buf_length + AES_BLOCK_SIZE];

  auto ctx = EVP_CIPHER_CTX_new();
  if (auto ins = InitEVPEncryptCtx(key, s, ctx)) {
    EVP_CIPHER_CTX_cleanup(ctx);
    return ins;
  }

  while (in.good() && out.good()) {
    // read
    in.read(reinterpret_cast<char *>(ibuf), buf_length);
    int cnt = in.gcount();
    if (cnt == 0) break;

    // encrypt
    int size;
    EVP_EncryptUpdate(ctx, obuf, &size, ibuf, cnt);

    // write
    out.write(reinterpret_cast<const char *>(obuf), size);
  }

  int size;
  EVP_EncryptFinal(ctx, obuf, &size);
  if (out.good()) out.write(reinterpret_cast<const char*>(obuf), size);

  EVP_CIPHER_CTX_cleanup(ctx);
  if (!out.good()) return bolo::Danger("out stream is not good"s);
  if (!in.eof()) return bolo::Danger("in stream is not eof"s);
  return bolo::Safe;
}

bolo::Insidious<std::string> Decrypt(std::istream &in, std::ostream &out, const std::string &key,
                                     Scheme s) {
  constexpr int buf_length = 512;
  unsigned char ibuf[buf_length];
  unsigned char obuf[buf_length + AES_BLOCK_SIZE];

  auto ctx = EVP_CIPHER_CTX_new();
  if (auto ins = InitEVPDecryptCtx(key, s, ctx)) {
    EVP_CIPHER_CTX_cleanup(ctx);
    return ins;
  }

  while (in.good() && out.good()) {
    // read
    in.read(reinterpret_cast<char *>(ibuf), buf_length);
    int cnt = in.gcount();
    if (cnt == 0) break;

    // encrypt
    int size;
    EVP_DecryptUpdate(ctx, obuf, &size, ibuf, cnt);

    // write
    out.write(reinterpret_cast<const char *>(obuf), size);
  }
  int size;
  EVP_DecryptFinal(ctx, obuf, &size);
  if (out.good()) out.write(reinterpret_cast<const char*>(obuf), size);

  EVP_CIPHER_CTX_cleanup(ctx);
  if (!out.good()) return bolo::Danger("out stream is not good"s);
  if (!in.eof()) return bolo::Danger("in stream is not eof"s);
  return bolo::Safe;
}
};  // namespace bolo_crypto
