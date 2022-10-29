#pragma once

#include <istream>
#include <string>

#include "result.h"

namespace bolo_compress {
enum class Scheme {
  DEFLATE,
};

// `in` and `out` should both be binary stream
bolo::Insidious<std::string> Compress(std::istream &in, std::ostream &out, Scheme s);
bolo::Insidious<std::string> Uncompress(std::istream &in, std::ostream &out, Scheme s);
};  // namespace bolo_compress
