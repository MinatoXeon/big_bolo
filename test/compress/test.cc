#define CATCH_CONFIG_MAIN

#include <catch.h>
#include <compress.h>
#include <test_util.h>

#include <fstream>
#include <iostream>
#include <string>

std::string test_dir = "__compress_test_dir__";
using namespace std::string_literals;

void StringTestCase(const std::string &s) {
  auto file = fs::path("test1");
  auto file_z = fs::path("test1.z");
  auto file_out = fs::path("test.oot");

  // test1
  {
    // compress
    {
      WriteString(file, s);

      std::ifstream ifs(file);
      std::ofstream ofs(file_z);

      auto ins = bolo_compress::Compress(ifs, ofs, bolo_compress::Scheme::DEFLATE);
      if (ins) std::cerr << ins.error() << std::endl;
      REQUIRE(!ins);
    }

    // decompress
    {
      std::ifstream ifs(file_z, std::ios_base::binary);
      std::ofstream ofs(file_out, std::ios_base::binary);

      auto ins = bolo_compress::Uncompress(ifs, ofs, bolo_compress::Scheme::DEFLATE);
      if (ins) std::cerr << ins.error() << std::endl;
      REQUIRE(!ins);
    }

    auto s1 = fs::file_size(file_z);
    auto s2 = fs::file_size(file);
    std::cout << "origin/compress: " << s1 << "/" << s2 << "(" << s1 * 0.1 / s2 << ")" << std::endl;

    REQUIRE(CompareFiles(file_out, file));
  }
}

TEST_CASE("compress") {
  fs::create_directories(test_dir);
  fs::current_path(test_dir);

  StringTestCase(Repeat("hello\n", 10000));
  StringTestCase(Repeat("JAVA is the BEST language IN THE world!\n", 1024));
  StringTestCase(Repeat("cpp is the worst programming language in the world!\n", 1024));

  fs::current_path("..");
  fs::remove_all(test_dir);
}
