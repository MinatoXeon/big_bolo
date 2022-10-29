#include "huffman.h"

#include <cassert>
#include <queue>
#include <sstream>

#define log_msg(s) (__FILE__ ":"s + std::to_string(__LINE__) + ":" + __func__ + ":" + s)

#define danger(s) Danger(log_msg(s))
#define err(s) Err(log_msg(s))

namespace bolo_compress {
using namespace std::string_literals;
Insidious<std::string> Huffman::Compress() {
  in_.seekg(0);

  if (auto weights_res = GetWeights())
    GenCodewords(BuildTree(weights_res.value()));
  else
    return Danger(weights_res.error());

  WriteHeader();

  in_.clear();
  in_.seekg(0);

  while (in_.good() && out_.good()) {
    uint8_t v;
    in_.read(reinterpret_cast<char *>(&v), sizeof(v));
    if (sizeof(v) != in_.gcount()) break;

    assert(codewords_.find(v) != codewords_.end());

    WriteBits(codewords_[v]);
  }

  if (bit_buffer_.size() > 0) {
    // write 7 false (让buffer 中剩下的bits组成一个完整的 byte 然后写下文件)
    WriteBits({false, false, false, false, false, false, false});
  }

  if (!out_.good()) return danger("out stream is not good"s);
  if (!in_.eof()) return danger("not end of input stream"s);
  return Safe;
}

Insidious<std::string> Huffman::Uncompress() {
  in_.seekg(0);

  auto unmap_res = ReadHeader();
  if (!unmap_res) return Danger(unmap_res.error());

  auto &unmap = unmap_res.value();

  while (byte_number > 0 && out_.good()) {
    std::string bits;
    while (bit_buffer_.size() > 0 || in_.good()) {
      char c = ReadBit() ? '1' : '0';
      bits.push_back(c);
      if (unmap.find(bits) != unmap.end()) {
        uint8_t u = unmap[bits];
        out_.write(reinterpret_cast<const char *>(&u), sizeof(uint8_t));
        byte_number--;
        break;
      }
    }
    if (!(bit_buffer_.size() > 0 || in_.good())) break;
  }

  if (!out_.good()) return danger("out stream is not good"s);
  if (byte_number != 0) return danger("no more bytes can be extracted"s);
  return Safe;
}

bool Huffman::ReadBit() {
  if (bit_buffer_.size() == 0) {
    uint8_t v{0};
    in_.read(reinterpret_cast<char *>(&v), sizeof v);

    for (int i = 7; i >= 0; i--) bit_buffer_.push(static_cast<bool>((v >> i) & 0x1));
  }

  bool t = bit_buffer_.front();
  bit_buffer_.pop();
  return t;
}

void Huffman::WriteBits(const std::vector<bool> &bits) {
  for (bool b : bits) bit_buffer_.push(b);

  while (bit_buffer_.size() >= 8) {
    uint8_t v{0};
    for (int i = 7; i >= 0; i--) {
      v |= static_cast<uint8_t>(bit_buffer_.front() << i);
      bit_buffer_.pop();
    }

    out_.write(reinterpret_cast<const char *>(&v), sizeof v);
  }
}

Result<Huffman::Weights, std::string> Huffman::GetWeights() {
  auto res = Weights();
  while (in_.good()) {
    uint8_t v;
    in_.read(reinterpret_cast<char *>(&v), sizeof v);
    if (in_.gcount() == 0) break;
    if (res.find(v) == res.end()) res[v] = 0;
    res[v]++;
    byte_number++;
  }

  if (!in_.eof()) return err("not end of input stream"s);

  return Ok(std::move(res));
}

std::shared_ptr<Huffman::Node> Huffman::BuildTree(const Weights &w) {
  if (w.empty()) return nullptr;

  auto pq =
      std::priority_queue<std::shared_ptr<Node>, std::vector<std::shared_ptr<Node>>, NodeCmp>{};
  for (auto const &it : w) pq.push(std::make_shared<Node>(it.first, it.second));

  while (pq.size() > 1) {
    auto l = pq.top();
    pq.pop();
    auto r = pq.top();
    pq.pop();

    pq.push(std::make_shared<Node>(l->cnt + r->cnt, l, r));
  }

  std::shared_ptr<Node> tree = pq.top();
  pq.pop();
  return tree;
}

void Huffman::GenCodewords(std::shared_ptr<Node> tree, std::vector<bool> bits) {
  if (tree->left != nullptr) {
    std::vector<bool> copy = bits;
    copy.push_back(false);
    GenCodewords(tree->left, copy);
  }

  if (tree->right != nullptr) {
    std::vector<bool> copy = bits;
    copy.push_back(true);
    GenCodewords(tree->right, copy);
  }

  if (tree->left == nullptr && tree->right == nullptr) codewords_[tree->val] = bits;
}

void Huffman::WriteHeader() {
  // write the total number of bytes
  out_.write(reinterpret_cast<const char *>(&byte_number), sizeof(byte_number));

  // write length of tuples
  int size = codewords_.size();
  out_.write(reinterpret_cast<const char *>(&size), sizeof(size));

  for (auto const &it : codewords_) {
    // uint8_t(bin): ""(string end with a '$')
    out_.write(reinterpret_cast<const char *>(&it.first), sizeof(uint8_t));

    std::stringstream ss;
    for (bool b : it.second) ss << b;
    ss << '$';
    auto s = ss.str();

    out_.write(s.c_str(), s.size());
  }
}

Result<Huffman::BitStringUnMap, std::string> Huffman::ReadHeader() {
  // read the number of bytes
  in_.read(reinterpret_cast<char *>(&byte_number), sizeof(byte_number));

  // read the unmap size
  int size;
  in_.read(reinterpret_cast<char *>(&size), sizeof(size));
  if (in_.gcount() != sizeof(size)) return err("failed to read the size"s);

  BitStringUnMap unmap;

  for (int i = 0; i < size; i++) {
    uint8_t v;
    in_.read(reinterpret_cast<char *>(&v), sizeof v);

    std::stringstream ss;
    while (true && in_.good()) {
      char c;
      in_.read(&c, sizeof c);
      if (c == '$') break;

      ss << c;
    }
    if (!in_.good()) return err("failed to read unmap"s);

    unmap[ss.str()] = v;
  }

  if (!in_.good()) return err("failed to read compression information"s);

  return Ok(unmap);
}

};  // namespace bolo_compress
