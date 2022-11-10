#include "bolo.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <thread>
#include <unordered_set>

#include "compress.h"
#include "copy.h"
#include "crypto.h"
#include "libfswatch/c++/libfswatch_exception.hpp"
#include "libfswatch/c++/monitor.hpp"
#include "libfswatch/c++/monitor_factory.hpp"
#include "tar.h"
#include "util.h"

namespace bolo {
using namespace std::string_literals;

Result<std::unique_ptr<Bolo>, std::string> Bolo::LoadFromJsonFile(const fs::path &path) try {
  json config;

  // read config
  std::ifstream f(path);
  if (!f.is_open()) {
    return Err("failed to open " + path.string());
  }
  f >> config;

  // parse config
  auto next_id = config.at("next_id").get<BackupFileId>();
  auto list = config.at("backup_list").get<BackupList>();
  auto enable_auto_update = config.at("enable_auto_update").get<bool>();
  fs::path backup_dir = config.at("backup_dir").get<std::string>();
  backup_dir = backup_dir.lexically_normal();

  fs::path cloud_path = config.at("cloud_mount_path").get<std::string>();
  cloud_path = cloud_path.lexically_normal();

  // create and check backup_dir
  fs::create_directories(backup_dir);
  // if backup_path exists
  if (fs::exists(backup_dir)) {
    // check if backup_path is a dir
    if (fs::status(backup_dir).type() != fs::file_type::directory)
      return Err("the backup_path is not a directory: "s + backup_dir.string());
  } else {
    return Err("failed to create backup directory: "s + backup_dir.string());
  }

  return Ok(std::unique_ptr<Bolo>(new Bolo(path, std::move(config), std::move(list), next_id,
                                           backup_dir, cloud_path, enable_auto_update)));
} catch (const fs::filesystem_error &e) {
  return Err("filesystem error: "s + e.what());
} catch (const json::out_of_range &e) {
  return Err("json out_of_range: "s + e.what());
} catch (const json::type_error &e) {
  return Err("json type_error: "s + e.what());
}

Bolo::Bolo(const fs::path &config_path, json &&config, BackupList &&m, BackupFileId next_id,
           const fs::path &backup_dir, const fs::path &cloud_path, bool enable_auto_update)
    : config_file_path_{config_path},
      config_(std::move(config)),
      backup_files_{std::move(m)},
      next_id_{next_id},
      backup_dir_{backup_dir},
      cloud_path_{cloud_path},
      fs_monitor_{nullptr},
      enable_auto_update_{enable_auto_update} {
  if (enable_auto_update_)
    thread_ = std::make_shared<std::thread>([this](auto t) { this->UpdateMonitor(t); }, nullptr);
}

void Bolo::UpdateMonitor(std::shared_ptr<std::thread> join) try {
  // stop monitor
  if (fs_monitor_ != nullptr) {
    fs_monitor_->stop();
    if (join != nullptr && join->joinable()) join->join();
  }

  std::vector<std::string> paths;
  for (auto &it : backup_files_) {
    paths.push_back(it.second.path);
  }

  // create a new monitor
  fs_monitor_ = std::shared_ptr<fsw::monitor>(fsw::monitor_factory::create_monitor(
      ::system_default_monitor_type, paths,
      [](const std::vector<fsw::event> &e, void *_bolo) {
        Bolo *bolo = static_cast<Bolo *>(_bolo);
        bolo->MonitorCallback(e);
      },    // callback
      this  // context
      ));
  fs_monitor_->set_recursive(true);
  fs_monitor_->set_latency(1);
  fs_monitor_->set_allow_overflow(false);
  fs_monitor_->set_event_type_filters({
      {fsw_event_flag::Created},
      {fsw_event_flag::Updated},
      {fsw_event_flag::Renamed},
      {fsw_event_flag::Removed},
      {fsw_event_flag::MovedTo},
  });
  fs_monitor_->start();

} catch (const fsw::libfsw_exception &e) {
  Log(LogLevel::Error, "libfsw error: "s + e.what());
} catch (const std::system_error &e) {
  Log(LogLevel::Error, "system error: failed to create threads, "s + e.what());
}

void Bolo::MonitorCallback(const std::vector<fsw::event> &events) try {
  std::unordered_set<std::string> visited;

  for (auto &it : backup_files_) {
    auto path = fs::path(it.second.path).lexically_normal().relative_path();
    if (visited.count(path.string()) > 0 || it.second.is_encrypted) continue;
    visited.insert(path.string());

    for (const auto &e : events) {
      auto e_path = fs::path(e.get_path()).lexically_normal().relative_path();
      if (e_path.string().find(path.string()) != std::string::npos) {
        if (auto ins = Update(it.second.id)) {
          Log(LogLevel::Error, "monitor update error: " + ins.error());
        }
        Log(LogLevel::Info, "fsw_monitor: "s + it.second.path);
        break;
      }
    }
  }
} catch (const fs::filesystem_error &e) {
  Log(LogLevel::Error, "fs error: "s + e.what());
}

Result<BackupFile, std::string> Bolo::Backup(const fs::path &path, bool is_compressed,
                                             bool is_encrypted, bool enable_cloud,
                                             const std::string &key) try {
  auto id = NextId();
  // remove '/' in directory path
  auto p = path.string().back() == '/' ? path.parent_path() : path;

  std::string filename = p.lexically_relative(p.parent_path());

  // backup filename = filename + id
  auto backup_path = ((enable_cloud ? cloud_path_ : backup_dir_) / (std::to_string(id) + filename));

  auto file = BackupFile{
      id, filename, p, backup_path, GetTimestamp(), is_compressed, is_encrypted, enable_cloud,
  };

  backup_files_[file.id] = file;

  std::string err;
  if (auto ins = BackupImpl(file, key)) {
    err = ins.error();
    goto clean;
  }

  if (auto ins = UpdateConfig()) {
    err = ins.error();
    goto clean;
  }

  return Ok(file);

clean:
  backup_files_.erase(file.id);
  fs::remove_all(file.backup_path);
  return Err(err);
} catch (const fs::filesystem_error &e) {
  return Err("filesystem error: "s + e.what());
}

// `f` has already being inserted into config
Insidious<std::string> Bolo::BackupImpl(BackupFile &f, const std::string &key) {
  auto temp = f.path;

  if (f.is_compressed || f.is_encrypted) {
    temp = MakeTemp();
    if (auto tar = bolo_tar::Tar::Open(temp)) {
      if (auto res = tar.value()->Append(f.path)) return Danger("tar error: "s + res.error());
    } else {
      return Danger(tar.error());
    }

    if (f.is_compressed) {
      auto t = MakeTemp();
      std::ifstream ifs(temp, std::ios_base::binary);
      if (!ifs.good()) return Danger("failded to open "s + temp);
      std::ofstream ofs(t, std::ios_base::binary | std::ios_base::trunc);
      if (!ofs.good()) return Danger("failded to open "s + t);

      if (auto ins = bolo_compress::Compress(ifs, ofs, bolo_compress::Scheme::DEFLATE))
        return Danger("compression error: "s + ins.error());

      // update temp
      temp = t;
    }

    if (f.is_encrypted) {
      if (key == "") return Danger("the file is encrypted, but the key is empty"s);

      auto t = MakeTemp();
      std::ifstream ifs(temp, std::ios_base::binary);
      if (!ifs.good()) return Danger("failded to open "s + temp);
      std::ofstream ofs(t, std::ios_base::binary | std::ios_base::trunc);
      if (!ofs.good()) return Danger("failded to open "s + t);

      if (auto ins = bolo_crypto::Encrypt(ifs, ofs, key))
        return Danger("encrypt error: "s + ins.error());

      // update temp
      temp = t;
    }
  }

  // remove the old file
  // if (fs::exists(f.backup_path)) fs::remove_all(f.backup_path);

  // rename temp
  // cannot use rename: Invalid cross-device link
  std::thread([f, temp]() {
    try {
      bolo_copy::Copy copy;
      auto res = copy.FullCopy(temp, f.backup_path);
    } catch (const fs::filesystem_error &e) {
      Log(LogLevel::Error, "copy error: "s + e.what());
    }
  }).detach();
  // fs::copy(temp, f.backup_path, fs::copy_options::update_existing | fs::copy_options::recursive);
  return Safe;
}

// 删除一个备份文件
Insidious<std::string> Bolo::Remove(BackupFileId id) try {
  if (backup_files_.find(id) == backup_files_.end())
    return Danger("No backup file with a BackupFileId of "s + std::to_string(id));

  auto file = backup_files_[id];

  fs::remove_all(file.backup_path);

  backup_files_.erase(id);

  return UpdateConfig();
} catch (const fs::filesystem_error &e) {
  return Danger("filesystem error: "s + e.what());
}

// 更新一个备份文件
Insidious<std::string> Bolo::Update(BackupFileId id, const std::string &key) try {
  if (backup_files_.find(id) == backup_files_.end())
    return Danger("No backup file with a BackupFileId of "s + std::to_string(id));

  BackupFile &file = backup_files_[id];

  if (file.is_encrypted && key == "") return Danger("the file is encrypted, but the key is empty"s);

  if (auto ins = BackupImpl(file, key)) return ins;

  if (auto ins = UpdateConfig()) return ins;

  // update timestamp
  file.timestamp = GetTimestamp();
  return Safe;
} catch (const fs::filesystem_error &e) {
  return Danger("filesystem error: "s + e.what());
}

Insidious<std::string> Bolo::Restore(BackupFileId id, const fs::path &restore_dir,
                                     const std::string &key) try {
  using bolo_tar::Tar;

  // check if the restore dir exists
  if (!fs::exists(restore_dir)) return Danger("directory does not exist: "s + restore_dir.string());
  if (!fs::is_directory(restore_dir))
    return Danger("the restore_path should be a directory: "s + restore_dir.string());

  // check if the file id is right
  if (backup_files_.find(id) == backup_files_.end())
    return Danger("No backup file with a BackupFileId of "s + std::to_string(id));
  const BackupFile &file = backup_files_[id];

  // check if the backup file exists
  if (!fs::exists(file.backup_path))
    return Danger("backup file does not exist: "s + file.backup_path);

  // check is there is a conflict file in the restore_dir
  auto restore_path = restore_dir / file.filename;
  if (fs::exists(restore_path))
    return Danger("file `"s + file.filename + "` already exists in `" + restore_dir.string() + "`");

  if (file.is_encrypted && key == "") return Danger("the file is encrypted, but the key is empty"s);

  std::string temp = file.backup_path;

  if (file.is_encrypted) {
    auto t = MakeTemp();

    std::ifstream ifs(temp, std::ios_base::binary);
    if (!ifs.good()) return Danger("failded to open "s + file.backup_path);
    std::ofstream ofs(t, std::ios_base::binary | std::ios_base::trunc);
    if (!ofs.good()) return Danger("failded to open "s + temp);

    if (auto ins = bolo_crypto::Decrypt(ifs, ofs, key))
      return Danger("decrypto error: "s + ins.error());

    temp = t;
  }

  if (file.is_compressed) {
    auto t = MakeTemp();

    std::ifstream ifs(temp, std::ios_base::binary);
    if (!ifs.good()) return Danger("failded to open "s + file.backup_path);
    std::ofstream ofs(t, std::ios_base::binary | std::ios_base::trunc);
    if (!ofs.good()) return Danger("failded to open "s + temp);

    if (auto ins = bolo_compress::Uncompress(ifs, ofs, bolo_compress::Scheme::DEFLATE))
      return Danger("compression error: "s + ins.error());

    temp = t;
  }

  if (file.is_compressed || file.is_encrypted) {
    if (auto tar = Tar::Open(temp)) {
      if (auto res = tar.value()->Extract(restore_dir)) return Danger("tar error: "s + res.error());
    } else {
      return Danger("tar error: "s + tar.error());
    }
  } else {
    bolo_copy::Copy copy;
    auto res = copy.FullCopy(temp, restore_path);
  }
  return Safe;
} catch (const fs::filesystem_error &e) {
  return Danger("filesystem error: "s + e.what());
}

Insidious<std::string> Bolo::UpdateConfig() {
  try {
    config_["next_id"] = next_id_;
    config_["backup_list"] = json(backup_files_);
  } catch (const json::type_error &e) {
    return Danger("json type_error: "s + e.what());
  }

  if (enable_auto_update_)
    thread_ = std::make_shared<std::thread>([this](auto t) { this->UpdateMonitor(t); }, thread_);

  std::ofstream f(config_file_path_);
  if (!f.is_open()) return Danger("failed to open config file: "s + config_file_path_.string());

  f << config_.dump(4);
  if (!f.good()) return Danger("failed to write to config file: "s + config_file_path_.string());
  return Safe;
}
};  // namespace bolo

