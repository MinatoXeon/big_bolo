#define CATCH_CONFIG_MAIN
#include <catch.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "copy.h"

namespace fs = std::filesystem;

std::vector<std::string> filenames{
    "foo/",                 //
    "foo/a/",               //
    "foo/a/t.txt",          //
    "foo/a/s.txt",          //
    "foo/b/",               //
    "foo/b/c/",             //
    "foo/b/c/e/",           //
    "foo/b/c/e/f/",         //
    "foo/b/c/e/f/g/",       //
    "foo/b/c/e/f/g/h.txt",  //
    "foo/d.txt",
};

std::string repeat(const std::string &s, int cnt) {
  std::string res;
  while (cnt-- > 0) {
    res += s;
  }
  return res;
}

std::unordered_map<std::string, std::string> contents = {
    {"foo/a/t.txt", "hello world"},
    {"foo/a/s.txt", repeat("Haskell is the best programming language in the world!\n", 10)},
    {"foo/b/c/e/f/g/h.txt", repeat("Hello", 5)},
    {"foo/d.txt", repeat("Java is the best language", 50)},
    {"bar.txt", repeat("python cpp c go fsharp csharp javascript typescript", 4)},
};

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

bool CreateTestFile() {
  // cd to tar_test_dir
  fs::create_directory("tar_test_dir/");
  fs::create_directory("untar_test_dir/");
  fs::current_path("tar_test_dir/");

  for (const auto &f : filenames) {
    if (f.back() == '/') {
      if (!fs::create_directory(f)) return false;
    } else {
      if (!WriteString(f, contents[f])) return false;
    }
  }
  fs::create_symlink("./a/t.txt", "foo/symlink");
  fs::create_hard_link("foo/a/s.txt", "foo/hard");
  filenames.push_back("foo/symlink");
  filenames.push_back("foo/hard");
  return true;
}

bool DeleteTestFile() {
  for (auto it = filenames.rbegin(); it != filenames.rend(); it++) fs::remove(*it);
  fs::remove_all("symlink");
  fs::remove_all("hard");
  fs::current_path("..");
  fs::remove_all("tar_test_dir");
  fs::remove_all("untar_test_dir");

  return true;
}

TEST_CASE("Copy", "test") {
  using namespace bolo_copy;
  REQUIRE(CreateTestFile());

  {
    Copy copy;
    auto res = copy.FullCopy("./", "../untar_test_dir");
    REQUIRE(!res);
  }

  { REQUIRE(std::system("diff -r ../untar_test_dir ./") == 0); }

  REQUIRE(DeleteTestFile());
}
