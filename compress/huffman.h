#pragma once

#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "result.h"
 
namespace bolo_compress {
using namespace bolo;

// TODO(yangtau): split Huffman into compression and decompression
class Huffman {
 public:
  Huffman(std::istream &in, std::ostream &out) : in_{in}, out_{out} {}

  Insidious<std::string> Compress();
  Insidious<std::string> Uncompress();

 private:
  using Weights = std::unordered_map<uint8_t, uint64_t>;
  using Codewords = std::unordered_map<uint8_t, std::vector<bool>>;
  using BitStringUnMap = std::unordered_map<std::string, uint8_t>;

  struct Node {
    uint8_t val;
    uint64_t cnt;
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;

    Node(uint8_t v, uint64_t c) : val{v}, cnt{c} {}
    Node(uint64_t c, std::shared_ptr<Node> l, std::shared_ptr<Node> r)
        : cnt{c}, left{l}, right{r} {}
  };
  struct NodeCmp {
    bool operator()(std::shared_ptr<Node> a, std::shared_ptr<Node> b) { return a->cnt > b->cnt; }
  };

  // get weight of every byte in the input stream
  Result<Weights, std::string> GetWeights();
  // build huffman tree from the weights
  std::shared_ptr<Node> BuildTree(const Weights &w);
  // travel huffman tree, and generate codewords for input bytes
  void GenCodewords(std::shared_ptr<Node> tree, std::vector<bool> bits = {});

  // write tuples (bits in string, uint8)
  void WriteHeader();
  // read tuples (bits in string, uint8)
  Result<BitStringUnMap, std::string> ReadHeader();

  // bit reads and writes
  bool ReadBit();
  void WriteBits(const std::vector<bool> &bits);

 private:
  std::istream &in_;
  std::ostream &out_;
  int64_t byte_number = 0;  // the number of bytes of original file

  std::queue<bool> bit_buffer_;
  Codewords codewords_;
};
};  // namespace bolo_compress
