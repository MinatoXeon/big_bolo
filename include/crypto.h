#pragma once
#include <iostream>
#include <string>

#include "result.h"

namespace bolo_crypto {
enum class Scheme {
  XOR,
  AES,
};

bolo::Insidious<std::string> Encrypt(std::istream &in, std::ostream &out, const std::string &key,
                                     Scheme s = Scheme::AES);
bolo::Insidious<std::string> Decrypt(std::istream &in, std::ostream &out, const std::string &key,
                                      Scheme s = Scheme::AES);
};  // namespace bolo_crypto
