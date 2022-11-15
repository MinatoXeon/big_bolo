#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::string Repeat(const std::string &s, int cnt) {
  std::string res;
  while (cnt-- > 0) {
    res += s;
  }
  return res;
}

bool ReadString(const fs::path &p, std::string &res) {
  std::ifstream ifs(p);

  while (ifs) {
    char buf[1024] = {0};
    ifs.read(buf, 1024);
    res += buf;
  }

  if (!ifs.eof()) return false;
  return true;
}

bool WriteString(const fs::path &p, const std::string &txt) {
  std::ofstream ofs(p);
  ofs << txt;
  return !!ofs;
}

bool CompareFiles(const fs::path &a, const fs::path &b) {
  if (fs::file_size(a) != fs::file_size(b)) {
    return false;
  }

  std::ifstream fa(a);
  std::ifstream fb(b);

  while (fa.good() && fb.good()) {
    constexpr int len = 512;
    char bufa[len] = {0};
    char bufb[len] = {0};

    fa.read(bufa, len);
    fb.read(bufb, len);

    if (std::memcmp(bufb, bufa, len) != 0) {
      /*
      for (int i = 0; i < len; i++) {
        std::cerr << "<" << std::hex << bufa[i] << "," << std::hex << bufb[i] << ">";
      }
      */
      return false;
    }
  }

  return fa.eof() && fb.eof();
}
