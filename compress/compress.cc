#include "compress.h"

#include "huffman.h"

namespace bolo_compress {
bolo::Insidious<std::string> Compress(std::istream &in, std::ostream &out, Scheme s) {
  auto huffman = Huffman(in, out);
  return huffman.Compress();
}

bolo::Insidious<std::string> Uncompress(std::istream &in, std::ostream &out, Scheme s) {
  auto huffman = Huffman(in, out);
  return huffman.Uncompress();
}
};  // namespace bolo_compress
