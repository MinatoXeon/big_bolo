#include "bolo.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <thread>
#include <unordered_set>

#include "libfswatch/c++/libfswatch_exception.hpp"
#include "libfswatch/c++/monitor.hpp"
#include "libfswatch/c++/monitor_factory.hpp"
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

