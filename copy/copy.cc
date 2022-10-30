#include "copy.h"
namespace bolo_copy {

using namespace bolo;
using namespace std::string_literals;
namespace fs = std::filesystem;

Insidious<std::string> Copy::FullCopy(const fs::path& src, const fs::path& target) try {
  map_.clear();
  return RecursiveCopy(src, target);
} catch (const fs::filesystem_error& e) {
  return Danger("filesystem: "s + e.what());
}
Insidious<std::string> Copy::RecursiveCopy(const fs::path& src, const fs::path& target) {
  if (!fs::exists(src)) return Danger("file: `" + src.string() + "` does not exists"s);

  if (fs::is_symlink(src)) {
    std::error_code code;
    fs::copy_symlink(src, target, code);
    std::cout << "error" << code.value() << " " << code.message();
    return Safe;
  } else if (fs::is_regular_file(src))
    return CopyFile(src, target);
  else if (fs::is_directory(src))
    return CopyDirectory(src, target);
  else
    return Danger("Unsupported file type: "s +
                  std::to_string(static_cast<unsigned>(fs::status(src).type())));
}
Insidious<std::string> Copy::CopyFile(const fs::path& src, const fs::path& target) {
  struct stat sbuf;
  int res = stat(src.string().c_str(), &sbuf);
  if (sbuf.st_nlink > 1) {
    if (map_.count(sbuf.st_ino)) {
      fs::path target_path = map_[sbuf.st_ino];
      fs::path test1 = fs::relative(target_path, src.parent_path());
      fs::path test2 = target.parent_path();
      fs::path test3 = test2 / test1;
      std::error_code code;
      fs::create_hard_link(target.parent_path() / fs::relative(target_path, src.parent_path()),
                           target, code);
      std::cout << "error" << code.value() << " " << code.message();
      return Safe;
    } else
      map_[sbuf.st_ino] = src.string();
  }
  fs::copy(src, target, fs::copy_options::update_existing);
  return Safe;
}
Insidious<std::string> Copy::CopyDirectory(const fs::path& src, const fs::path& target) {
  auto ins = Insidious<std::string>(Safe);
  fs::create_directory(target);
  for (auto& p : fs::directory_iterator(src)) {
    fs::path test0 = p.path();
    fs::path test1 = p.path().lexically_relative(src);
    fs::path test2 = target / p.path().lexically_relative(src);
    fs::path test3 = p;
    ins = RecursiveCopy(p, target / p.path().lexically_relative(src));
  }
  return ins;
}
};  // namespace bolo_copy