#pragma once

#include <cstdint>

#include "lib/jsonlib.h"

namespace bolo {
using Timestamp = std::uint64_t;
using BackupFileId = std::uint64_t;
using json = nlohmann::json;
};  // namespace bolo
