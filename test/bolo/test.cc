#define CATCH_CONFIG_MAIN
#include <catch.h>
#include <test_util.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "bolo.h"

namespace fs = std::filesystem;
using namespace bolo;

const auto config_path = "config.json";
const auto test_dir = "__bolo_test__dir__";
std::vector<std::tuple<std::string, bool, bool, std::string>> cases{
    {"hello/hello.txt", false, true, "hello"},
    {"java.txt", true, true, "world"},
    {"hello/a/python.txt", true, false, ""},
    {"path/ruby.txt", false, false, ""},
    {"best/language/haskell.txt", true, true, "world"},
    {"worst/language/php.txt", true, true, "hh"},
};

std::unordered_map<std::string, std::string> content{
    {"hello/hello.txt", Repeat("你好世界, 真不戳!\n", 200)},
    {"java.txt", Repeat("Java is the best language", 500)},
    {"hello/a/python.txt", ""},
    {"path/ruby.txt", Repeat("ruby ruby?", 100)},
    {"best/language/haskell.txt", Repeat("Haskell is the best language in the world!\n", 1000)},
    {"worst/language/php.txt", ""},
};

bool CreateConfigFile(const std::string &content) {
  std::ofstream f(config_path, std::ofstream::trunc | std::ofstream::out);
  if (!f) return false;

  f << content;
  return !!f;
}

bool RemoveConfig() { return fs::remove(config_path); }

bool CreateFiles() {
  fs::create_directory(test_dir);
  fs::current_path(test_dir);

  for (auto &it : content) {
    fs::path path(it.first);
    if (path.parent_path() != fs::path("")) fs::create_directories(path.parent_path());

    std::ofstream ofs(path, std::ios_base::trunc | std::ofstream::out);
    ofs << it.second;
    if (!ofs.good()) return false;
  }

  return true;
}

void DeleteFiles() {
  fs::current_path("..");
  fs::remove_all(test_dir);
}

TEST_CASE("Bolo-error", "error") {
  // failed to load
  REQUIRE(CreateConfigFile("{ \"backup_list\": [], \"next_\": 0 ,\"backup_path\":\"hello\"}"));
  auto res = Bolo::LoadFromJsonFile(config_path);
  REQUIRE(!res);

  res = Bolo::LoadFromJsonFile("!hello world");
  REQUIRE(!res);

  REQUIRE(RemoveConfig());
}

TEST_CASE("Bolo", "test") {
  BackupList list;

  REQUIRE(CreateFiles());
  REQUIRE(
      CreateConfigFile("{ \"backup_list\": [], \"next_id\": "
                       "0,\"backup_dir\":\"backup_path/\", \"enable_auto_update\": false, "
                       "\"cloud_mount_path\":\"backup_path/\" }"));

  std::unordered_map<BackupFileId, std::string> keys;
  {
    // load
    auto bolo_res = Bolo::LoadFromJsonFile(config_path);
    if (!bolo_res) std::cerr << bolo_res.error() << std::endl;
    REQUIRE(!!bolo_res);
    auto b = std::move(bolo_res.value());

    REQUIRE(b->next_id() == 0);
    REQUIRE(b->backup_files().size() == 0);
    REQUIRE(b->backup_dir() == "backup_path/");

    // backup
    for (auto &[origin, compressed, encrypted, key] : cases) {
      auto res = b->Backup(origin, compressed, encrypted, false, key);
      if (!res) std::cout << res.error() << std::endl;
      REQUIRE(!!(res));
      auto f = res.value();

      list[f.id] = f;
      keys[f.id] = key;

      REQUIRE(f.path == origin);
      REQUIRE(f.is_compressed == compressed);
      REQUIRE(f.is_encrypted == encrypted);

      auto m = b->GetBackupFile(f.id);
      REQUIRE(!!m);
      REQUIRE(m.value() == f);
    }
  }

  fs::path restore_dir = fs::path("restore");
  {
    // restore
    auto bolo_res = Bolo::LoadFromJsonFile(config_path);
    if (!bolo_res) std::cout << bolo_res.error() << std::endl;
    REQUIRE(!!bolo_res);
    auto b = std::move(bolo_res.value());

    fs::create_directory(restore_dir);
    for (auto &it : list) {
      auto file = restore_dir / it.second.filename;
      auto ins = b->Restore(it.first, restore_dir, keys[it.first]);
      if (ins) std::cerr << ins.error() << std::endl;
      REQUIRE(!ins);

      REQUIRE(CompareFiles(file, it.second.path));
    }
  }

  {
    // update
    auto bolo_res = Bolo::LoadFromJsonFile(config_path);
    if (!bolo_res) std::cout << bolo_res.error() << std::endl;
    REQUIRE(!!bolo_res);
    auto b = std::move(bolo_res.value());

    for (auto &it : list) {
      auto ins = b->Update(it.first, keys[it.first]);
      if (ins) std::cerr << ins.error() << std::endl;
      REQUIRE(!ins);

      auto m = b->GetBackupFile(it.first);
      REQUIRE(!!m);

      auto f = m.value();

      REQUIRE(f.timestamp >= it.second.timestamp);

      it.second = f;
    }
  }

  {
    // remove
    auto bolo_res = Bolo::LoadFromJsonFile(config_path);
    REQUIRE(!!bolo_res);
    auto b = std::move(bolo_res.value());

    for (auto &it : list) {
      auto ins = b->Remove(it.first);
      if (ins) std::cerr << ins.error() << std::endl;
      REQUIRE(!ins);
    }
  }

  DeleteFiles();
}
