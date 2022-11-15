#include <backup_file.h>
#include <catch.h>

TEST_CASE("BackupFile", "json") {
  using namespace bolo;

  BackupFile file{0, "hello", "/origin/path", "/backup/path", 10000000, true, true};

  json j = json(file);

  REQUIRE(j.contains("id"));
  REQUIRE(j.contains("path"));
  REQUIRE(j.contains("backup_path"));
  REQUIRE(j.contains("timestamp"));
  REQUIRE(j.contains("is_compressed"));
  REQUIRE(j.contains("is_encrypted"));

  REQUIRE(j.at("id") == file.id);
  REQUIRE(j.at("path") == file.path);
  REQUIRE(j.at("backup_path") == file.backup_path);
  REQUIRE(j.at("timestamp") == file.timestamp);
  REQUIRE(j.at("is_compressed") == file.is_compressed);
  REQUIRE(j.at("is_encrypted") == file.is_encrypted);

  auto f = j.get<BackupFile>();

  REQUIRE(f.id == file.id);
  REQUIRE(f.path == file.path);
  REQUIRE(f.backup_path == file.backup_path);
  REQUIRE(f.timestamp == file.timestamp);
  REQUIRE(f.is_compressed == file.is_compressed);
  REQUIRE(f.is_encrypted == file.is_encrypted);
}

TEST_CASE("BackupList", "json") {
  using namespace bolo;

  BackupFile f1{1, "hello", "/origin/path1", "/backup/path", 10000000, true, true};
  BackupFile f2{2, "hello", "/origin/path2", "/backup/path", 20000000, true, false};
  BackupFile f3{3, "hello", "/origin/path3", "/backup/path", 30000000, false, true};

  BackupList list{
      {f1.id, f1},
      {f2.id, f2},
      {f3.id, f3},
  };

  json j(list);

  auto l = j.get<BackupList>();

  REQUIRE(l.size() == 3);

  REQUIRE(l[f1.id] == f1);
  REQUIRE(l[f2.id] == f2);
  REQUIRE(l[f3.id] == f3);
}
