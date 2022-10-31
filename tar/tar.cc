#include "tar.h"

#include <errno.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <system_error>
#include <type_traits>

#include "result.h"

namespace bolo_tar {

using namespace bolo;
using namespace std::string_literals;
namespace fs = std::filesystem;

Result<std::shared_ptr<Tar>, std::string> Tar::Open(const fs::path &path) {
  std::fstream fs(path, std::ios_base::in | std::ios_base::app | std::ios_base::out);

  if (!fs) return Err("failed to open "s + path.string());

  return Ok(std::shared_ptr<Tar>(new Tar(std::move(fs))));
}

Insidious<std::string> Tar::Write() {
  fs_.flush();
  if (!fs_) return Danger("failed to flush"s);
  return Safe;
}

namespace {
/*
    Field offset	Field size	Field
    0	    100	File name
    100	  8	File mode
    108	  8	Owner's numeric user ID
    116	  8	Group's numeric user ID
    124	  12	File size in bytes (octal base)
    136	  12	Last modification time in numeric Unix time format (octal)
    148	  8	Checksum for header record
    156	  1	Link indicator (file type)
    157	  100	Name of linked file
*/
#pragma pack(push)
#pragma pack(1)

constexpr int FileAlignment = 512;

struct TarHeader {
  union {
    char _[FileAlignment];
    struct {
      char filename[100] = {0};
      char filemode[8] = {0};
      char owner_id[8] = {0};
      char group_id[8] = {0};
      char file_size[12] = {0};               // octal base
      char last_modification_time[12] = {0};  // octal
      char checksum[8] = {0};
      char filetype[1] = {0};  // '0': normal file, '1': symlink, '2': hard , '3':directory
      char linked_file[100] = {0};
    };
  };

  TarHeader() { std::memset(this, 0, sizeof(TarHeader)); }

  static std::unique_ptr<TarHeader> CreateHeader(const std::string &filename, int file_size,
                                                 fs::perms perms, fs::file_type type,
                                                 const std::string &linkfile) {
    auto size_s = std::to_string(file_size);
    auto perms_s = std::to_string(static_cast<unsigned>(perms));
    if (filename.length() + 1 > sizeof(TarHeader::filename)) return nullptr;
    if (size_s.size() + 1 > sizeof(TarHeader::file_size)) return nullptr;
    if (perms_s.size() + 1 > sizeof(TarHeader::filemode)) return nullptr;
    if (linkfile.length() + 1 > sizeof(TarHeader::linked_file)) return nullptr;

    auto header = std::make_unique<TarHeader>();
    std::strncpy(header->file_size, size_s.c_str(), sizeof(TarHeader::file_size));
    std::strncpy(header->filename, filename.c_str(), sizeof(TarHeader::filename));
    std::strncpy(header->filemode, perms_s.c_str(), sizeof(TarHeader::filemode));

    if (type == fs::file_type::symlink)
      header->filetype[0] = '1';
    else if (type == fs::file_type::unknown)
      header->filetype[0] = '2';
    else if (type == fs::file_type::regular)
      header->filetype[0] = '0';
    else
      header->filetype[0] = '3';
    if (!linkfile.empty())
      std::strncpy(header->linked_file, linkfile.c_str(), sizeof(TarHeader::linked_file));
    return header;
  }

  std::string file_name() const { return filename; }

  std::string linked_file_name() const { return linked_file; }

  fs::file_type file_type() const {
    if (filetype[0] == '0')
      return fs::file_type::regular;
    else if (filetype[0] == '1')
      return fs::file_type::symlink;
    else if (filetype[0] == '2')
      return fs::file_type::unknown;
    else if (filetype[0] == '3')
      return fs::file_type::directory;
    else
      return fs::file_type::none;
  }

  Maybe<fs::perms> perms() const {
    try {
      return Just(static_cast<fs::perms>(std::stoul(filemode)));
    } catch (...) {
      return Nothing;
    }
  }

  Maybe<int> filesize() const {
    try {
      return Just(std::stoi(file_size));
    } catch (...) {
      return Nothing;
    }
  }
};

#pragma pack(pop)
static_assert(sizeof(TarHeader) == FileAlignment, "tar header size");
};  // namespace

Insidious<std::string> Tar::AppendFile(const std::filesystem::path &path,
                                       const fs::path &relative_dir) {
  struct stat sbuf;
  // std::cout << path.lexically_relative(relative_dir).string() << std::endl;
  int res = stat(path.string().c_str(), &sbuf);
  std::cout << "error: " << std::strerror(errno) << std::endl;
  std::cout << "res: " << res << " " << sbuf.st_nlink << " " << sbuf.st_ino << std::endl;
  if (sbuf.st_nlink > 1) {
    std::cout << map_.count(sbuf.st_ino) << std::endl;
    if (map_.count(sbuf.st_ino)) {
      std::cout << map_[sbuf.st_ino] << std::endl;
      fs::path target_path = map_[sbuf.st_ino];
      // header
      auto header = TarHeader::CreateHeader(path.lexically_relative(relative_dir).string(), 0,
                                            fs::status(path).permissions(), fs::file_type::unknown,
                                            target_path.lexically_relative(relative_dir).string());
      if (header == nullptr) return Danger("failed to create file header"s);
      fs_.write(reinterpret_cast<const char *>(header.get()), FileAlignment);

      auto ins = fs_.good() ? Insidious<std::string>(Safe)
                            : Danger("failed to write symlink header: "s + path.string());

      std::cout << "tar/hardlink" << path.string() << std::endl;
      return ins;
    } else
      map_[sbuf.st_ino] = path.string();
  }

  // header
  auto header =
      TarHeader::CreateHeader(path.lexically_relative(relative_dir).string(), fs::file_size(path),
                              fs::status(path).permissions(), fs::file_type::regular, "");
  if (header == nullptr) return Danger("failed to create file header"s);
  fs_.write(reinterpret_cast<const char *>(header.get()), FileAlignment);

  std::ifstream ifs(path);
  if (!ifs) return Danger("failed to open "s + path.string());

  // Copy file content
  while (ifs.good() && fs_.good()) {
    char buf[FileAlignment] = {0};
    ifs.read(buf, FileAlignment);
    if (ifs.gcount() == 0) break;

    fs_.write(buf, FileAlignment);
  }

  if (!fs_.good()) return Danger("failed to write to output file"s);
  if (!ifs.eof()) return Danger("failed to read from input file"s);
  std::cout << "tar/regular" << path.string() << std::endl;

  return Safe;
}

Insidious<std::string> Tar::AppendSymlink(const std::filesystem::path &path,
                                          const fs::path &relative_dir) {
  //        std::cout<<"0:"<<fs::current_path()<<std::endl;
  //        std::cout<<"1:"<<fs::read_symlink(path).string()<<std::endl;
  //        std::cout<<"2:"<<path.string()<<std::endl;
  //        std::cout<<"3:"<<relative_dir.string()<<std::endl;
  //        std::cout<<"4:"<<fs::current_path()<<std::endl;
  // header
  auto header = TarHeader::CreateHeader(path.lexically_relative(relative_dir).string(), 0,
                                        fs::status(path).permissions(), fs::file_type::symlink,
                                        fs::read_symlink(path).string());
  if (header == nullptr) return Danger("failed to create file header"s);
  fs_.write(reinterpret_cast<const char *>(header.get()), FileAlignment);

  auto ins = fs_.good() ? Insidious<std::string>(Safe)
                        : Danger("failed to write symlink header: "s + path.string());
  // std::cout << "tar/symlink"<< path.string() << std::endl;
  return ins;
}

Insidious<std::string> Tar::AppendDirectory(const std::filesystem::path &path,
                                            const fs::path &relative_dir) {
  // directory header
  auto header =
      TarHeader::CreateHeader(path.lexically_relative(relative_dir).string() + "/", 0,
                              fs::status(path).permissions(), fs::file_type::directory, "");
  if (header == nullptr) return Danger("failed to create file header"s);
  fs_.write(reinterpret_cast<const char *>(header.get()), FileAlignment);

  auto ins = fs_.good() ? Insidious<std::string>(Safe)
                        : Danger("failed to write directory header: "s + path.string());

  for (auto &p : fs::directory_iterator(path)) {
    if (ins) break;
    ins = AppendImpl(p, relative_dir);
  }
  // std::cout << "tar/directory"<< path.string() << std::endl;
  return ins;
}

Insidious<std::string> Tar::AppendImpl(const fs::path &path, const fs::path &relative_dir) {
  if (!fs::exists(path)) return Danger("file: `" + path.string() + "` does not exists"s);

  if (fs::is_symlink(path))
    return AppendSymlink(path, relative_dir);
  else if (fs::is_regular_file(path))
    return AppendFile(path, relative_dir);
  else if (fs::is_directory(path))
    return AppendDirectory(path, relative_dir);
  else
    return Danger("Unsupported file type: "s +
                  std::to_string(static_cast<unsigned>(fs::status(path).type())));
}

Insidious<std::string> Tar::Append(const fs::path &path) try {
  map_.clear();
  return AppendImpl(path, path.parent_path());
} catch (const fs::filesystem_error &e) {
  return Danger("filesystem: "s + e.what());
}

Result<std::vector<Tar::TarFile>, std::string> Tar::List() {
  std::vector<Tar::TarFile> files;

  // move to begin of tar file
  fs_.seekg(0);
  while (fs_.good()) {
    // try to read a tar header
    TarHeader header;
    fs_.read(reinterpret_cast<char *>(&header), FileAlignment);
    if (fs_.gcount() == 0 && fs_.eof()) break;

    // get tar file info from header
    auto filename = header.file_name();
    auto size = header.filesize();
    if (!size) return Err("failed to extract size of `"s + filename + "` from tar header");
    auto perms = header.perms();
    if (!perms) return Err("failed to extract perms of `"s + filename + "` from tar header");
    auto type = filename.back() == '/' ? fs::file_type::directory : fs::file_type::regular;

    files.push_back(Tar::TarFile{filename, size.value(), perms.value(), type});

    int off = (size.value() + FileAlignment - 1) / FileAlignment * FileAlignment;
    fs_.seekg(off, std::ios_base::cur);
  }

  if (!fs_.eof()) return Err("failed to list tar"s);

  return Ok(files);
}

Insidious<std::string> Tar::Extract(const fs::path &dir) {
  std::cout << "Extract" << std::endl;
  // move to the begin of tar file
  fs_.seekg(0);

  while (fs_.good()) {
    //  try to read a header
    TarHeader header;
    fs_.read(reinterpret_cast<char *>(&header), FileAlignment);
    if (fs_.gcount() == 0 && fs_.eof()) break;

    auto filename = header.file_name();
    auto size = header.filesize();
    if (!size) return Danger("failed to extract size of `"s + filename + "` from tar header");

    auto perms = header.perms();
    if (!perms) return Danger("failed to extract perms of `"s + filename + "` from tar header");

    if (header.file_type() == fs::file_type::directory) {
      fs::create_directories(dir / filename);
      std::cout << "directory: " << (dir / filename).string() << std::endl;
    } else if (header.file_type() == fs::file_type::symlink) {
      std::cout << "symlink: " << (dir / filename).string() << std::endl;
      fs::create_symlink(header.linked_file_name(), dir / filename);
    } else if (header.file_type() == fs::file_type::unknown) {
      std::cout << "hardlink: " << (dir / filename).string() << std::endl;
      fs::create_hard_link(dir / header.linked_file_name(), dir / filename);
    } else if (header.file_type() == fs::file_type::regular) {
      std::cout << "regular: " << (dir / filename).string() << std::endl;
      auto ins = ExtractFile(dir / filename, size.value());
      if (ins) return ins;
    } else {
      return Danger("filetype is unexpected"s);
    }

    fs::permissions(dir / filename, perms.value());
  }

  if (!fs_.eof()) return Danger("failed to extract"s);
  return Safe;
}

Insidious<std::string> Tar::ExtractFile(const std::filesystem::path &path, int size) {
  std::ofstream ofs(path);
  if (!ofs) return Danger("failed to open: "s + path.string());

  while (size > 0 && ofs.good() && fs_.good()) {
    char buf[FileAlignment] = {0};
    fs_.read(buf, FileAlignment);
    ofs.write(buf, std::min(size, FileAlignment));
    size -= FileAlignment;
  }

  if (!ofs) return Danger("failed to write to "s + path.string());
  if (!fs_) return Danger("failed to read from tar file"s);

  return Safe;
}

};  // namespace bolo_tar
