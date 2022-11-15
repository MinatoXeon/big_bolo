#define CATCH_CONFIG_MAIN

#include <fstream>
#include <iostream>
#include <string>

#include "catch.h"
#include "crypto.h"
#include "test_util.h"

std::string test_dir = "__crypto_test_dir__";
using namespace std::string_literals;

void StringTestCase(const std::string &s, const std::string &key) {
  auto file = fs::path("test1");
  auto file_c = fs::path("test1.cry");
  auto file_out = fs::path("test.out");
  using namespace bolo_crypto;

  {
    // encrypt
    {
      WriteString(file, s);

      std::ifstream ifs(file);
      std::ofstream ofs(file_c);
      auto ins = Encrypt(ifs, ofs, key);

      if (ins) std::cerr << ins.error() << std::endl;
      REQUIRE(!ins);
    }

    // decrypt
    {
      std::ifstream ifs(file_c, std::ios_base::binary);
      std::ofstream ofs(file_out, std::ios_base::binary);

      auto ins = Decrypt(ifs, ofs, key);
      if (ins) std::cerr << ins.error() << std::endl;
      REQUIRE(!ins);
    }

    REQUIRE(CompareFiles(file_out, file));
  }
}

TEST_CASE("crypto") {
  fs::create_directories(test_dir);
  fs::current_path(test_dir);

  StringTestCase(Repeat("hello\n", 100), "");
  StringTestCase(Repeat("hello\n", 10), "a");
  StringTestCase(Repeat("hello\n", 512), "ab");
  StringTestCase(Repeat("hello\n", 6000), "hello world");
  StringTestCase(Repeat("hello world!针不戳\n", 600), "hello world 你好呀");
  StringTestCase(Repeat("JAVA is the BEST language IN THE world!\n", 10), "123456");
  StringTestCase(Repeat("cpp is the worst programming language in the world!\n", 512),
                 "1234567890");

  fs::current_path("..");
  fs::remove_all(test_dir);
}
