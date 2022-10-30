#pragma once

#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "result.h"

namespace bolo_copy {
namespace fs = std::filesystem;

class Copy {
 public:
  Copy() = default;
  bolo::Insidious<std::string> FullCopy(const fs::path& src, const fs::path& target);

 private:
  bolo::Insidious<std::string> RecursiveCopy(const fs::path& src, const fs::path& target);
  bolo::Insidious<std::string> CopyFile(const fs::path& src, const fs::path& target);
  bolo::Insidious<std::string> CopyDirectory(const fs::path& src, const fs::path& target);
  std::map<short, std::string> map_;
};

};  // namespace bolo_copy