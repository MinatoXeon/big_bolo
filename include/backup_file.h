#pragma once

#include <string>

#include "lib/jsonlib.h"
#include "types.h"

namespace bolo {
struct BackupFile {
  BackupFileId id;
  std::string filename;
  std::string path;         // 文件初始目录
  std::string backup_path;  // 本地备份目录
  Timestamp timestamp;      // 备份的时间
  bool is_compressed;       // 是否压缩
  bool is_encrypted;        // 是否加密
  bool is_in_cloud;         // 是否云备份

  bool operator==(const BackupFile &f) const {
    return id == f.id;
  }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BackupFile, id, filename, path, backup_path, timestamp,
                                   is_compressed, is_encrypted, is_in_cloud);

using BackupList = std::unordered_map<std::uint64_t, BackupFile>;

};  // namespace bolo
