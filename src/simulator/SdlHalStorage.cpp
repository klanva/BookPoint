#include "SdlHalStorage.h"
#include <cstdio>
#include <iostream>
#include <algorithm>

class HalFile::Impl {
 public:
  FILE* fp = nullptr;
  fs::path fullPath;
  bool isDir = false;
  fs::directory_iterator dirIt;
  size_t fSize = 0;
  bool open = false;

  ~Impl() {
    close();
  }

  void openRead(const fs::path& p) {
    close();
    fullPath = p;
    std::error_code ec;
    if (fs::is_directory(p, ec)) {
      isDir = true;
      dirIt = fs::directory_iterator(p, ec);
      open = !ec;
      return;
    }
    fp = fopen(p.string().c_str(), "rb");
    if (!fp) return;
    fseek(fp, 0, SEEK_END);
    fSize = static_cast<size_t>(ftell(fp));
    fseek(fp, 0, SEEK_SET);
    open = true;
  }

  void openWrite(const fs::path& p, oflag_t oflag) {
    close();
    fullPath = p;
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);

    const char* mode = "wb+";
    if (oflag & O_APPEND) {
      mode = "ab+";
    } else if (oflag & O_CREAT) {
      if (fs::exists(p, ec) && !(oflag & O_TRUNC)) {
        mode = "rb+";
      }
    }
    fp = fopen(p.string().c_str(), mode);
    if (!fp && strcmp(mode, "rb+") == 0) {
      fp = fopen(p.string().c_str(), "wb+");
    }
    if (fp) {
      fseek(fp, 0, SEEK_END);
      fSize = static_cast<size_t>(ftell(fp));
      fseek(fp, 0, SEEK_SET);
      open = true;
    }
  }

  void close() {
    if (fp) {
      fclose(fp);
      fp = nullptr;
    }
    isDir = false;
    open = false;
    fSize = 0;
  }
};

HalFile::HalFile() : impl(std::make_shared<Impl>()) {}
HalFile::HalFile(std::shared_ptr<Impl> impl) : impl(std::move(impl)) {}
HalFile::~HalFile() = default;
HalFile::HalFile(HalFile&&) noexcept = default;
HalFile& HalFile::operator=(HalFile&&) noexcept = default;

void HalFile::flush() {
  if (impl && impl->fp) fflush(impl->fp);
}

size_t HalFile::getName(char* name, size_t len) {
  if (!impl || !name || len == 0) return 0;
  std::string filename = impl->fullPath.filename().string();
  strncpy(name, filename.c_str(), len - 1);
  name[len - 1] = '\0';
  return strlen(name);
}

size_t HalFile::size() {
  if (!impl || !impl->open || impl->isDir) return 0;
  return impl->fSize;
}

bool HalFile::seek(size_t pos) {
  if (!impl || !impl->fp) return false;
  return fseek(impl->fp, static_cast<long>(pos), SEEK_SET) == 0;
}

bool HalFile::seekCur(int64_t offset) {
  if (!impl || !impl->fp) return false;
  return fseek(impl->fp, static_cast<long>(offset), SEEK_CUR) == 0;
}

int HalFile::available() const {
  if (!impl || !impl->fp) return 0;
  long cur = ftell(impl->fp);
  if (cur < 0 || static_cast<size_t>(cur) >= impl->fSize) return 0;
  return static_cast<int>(impl->fSize - cur);
}

size_t HalFile::position() const {
  if (!impl || !impl->fp) return 0;
  long cur = ftell(impl->fp);
  return (cur >= 0) ? static_cast<size_t>(cur) : 0;
}

int HalFile::read(void* buf, size_t count) {
  if (!impl || !impl->fp || !buf || count == 0) return -1;
  size_t bytesRead = fread(buf, 1, count, impl->fp);
  return static_cast<int>(bytesRead);
}

int HalFile::read() {
  uint8_t b = 0;
  if (read(&b, 1) <= 0) return -1;
  return b;
}

size_t HalFile::write(const uint8_t* buf, size_t count) {
  if (!impl || !impl->fp || !buf || count == 0) return 0;
  size_t written = fwrite(buf, 1, count, impl->fp);
  long cur = ftell(impl->fp);
  if (cur >= 0 && static_cast<size_t>(cur) > impl->fSize) {
    impl->fSize = static_cast<size_t>(cur);
  }
  return written;
}

size_t HalFile::write(uint8_t b) {
  return write(&b, 1);
}

bool HalFile::rename(const char* newPath) {
  if (!impl || impl->fullPath.empty()) return false;
  fs::path np = HalStorage::resolve(newPath);
  std::error_code ec;
  fs::rename(impl->fullPath, np, ec);
  if (!ec) {
    impl->fullPath = np;
    return true;
  }
  return false;
}

bool HalFile::isDirectory() const {
  return impl && impl->isDir;
}

bool HalFile::getModifyDateTime(uint16_t* pdate, uint16_t* ptime) {
  if (pdate) *pdate = 0;
  if (ptime) *ptime = 0;
  return true;
}

void HalFile::rewindDirectory() {
  if (impl && impl->isDir) {
    std::error_code ec;
    impl->dirIt = fs::directory_iterator(impl->fullPath, ec);
  }
}

bool HalFile::close() {
  if (impl) impl->close();
  return true;
}

HalFile HalFile::openNextFile() {
  if (!impl || !impl->isDir || impl->dirIt == fs::directory_iterator()) return HalFile();
  const auto entry = *impl->dirIt;
  ++impl->dirIt;
  auto child = std::make_shared<Impl>();
  child->openRead(entry.path());
  return HalFile(child);
}

bool HalFile::isOpen() const {
  return impl && impl->open;
}

// HalStorage implementation
fs::path HalStorage::resolve(const char* p) {
  if (!p || p[0] == '\0') return fs::path("fs_");
  std::string s(p);
  while (!s.empty() && (s.front() == '/' || s.front() == '\\')) {
    s.erase(0, 1);
  }
  return fs::path("fs_") / s;
}

bool HalStorage::begin() {
  std::error_code ec;
  fs::create_directories("fs_/books", ec);
  fs::create_directories("fs_/.crosspoint/stats/books", ec);
  fs::create_directories("fs_/.crosspoint/bookmarks", ec);
  fs::create_directories("fs_/.crosspoint/clippings", ec);
  return true;
}

HalFile HalStorage::open(const char* path, oflag_t oflag) {
  auto file = std::make_shared<HalFile::Impl>();
  const fs::path p = resolve(path);
  if ((oflag & O_WRONLY) || (oflag & O_RDWR) || (oflag & O_CREAT)) {
    file->openWrite(p, oflag);
  } else {
    file->openRead(p);
  }
  return HalFile(file);
}

bool HalStorage::exists(const char* path) const {
  std::error_code ec;
  return fs::exists(resolve(path), ec);
}

bool HalStorage::mkdir(const char* path, bool) {
  std::error_code ec;
  return fs::create_directories(resolve(path), ec);
}

bool HalStorage::remove(const char* path) {
  std::error_code ec;
  return fs::remove(resolve(path), ec);
}

bool HalStorage::rename(const char* oldPath, const char* newPath) {
  std::error_code ec;
  fs::rename(resolve(oldPath), resolve(newPath), ec);
  return !ec;
}

bool HalStorage::rmdir(const char* path) {
  return remove(path);
}

bool HalStorage::ensureDirectoryExists(const char* path) {
  return mkdir(path);
}

bool HalStorage::openFileForRead(const char*, const char* path, HalFile& file) {
  file = open(path, O_RDONLY);
  return file.isOpen();
}

bool HalStorage::openFileForRead(const char* moduleName, const std::string& path, HalFile& file) {
  return openFileForRead(moduleName, path.c_str(), file);
}

bool HalStorage::openFileForRead(const char* moduleName, const String& path, HalFile& file) {
  return openFileForRead(moduleName, path.c_str(), file);
}

bool HalStorage::openFileForWrite(const char*, const char* path, HalFile& file) {
  file = open(path, O_WRONLY | O_CREAT | O_TRUNC);
  return file.isOpen();
}

bool HalStorage::openFileForWrite(const char* moduleName, const std::string& path, HalFile& file) {
  return openFileForWrite(moduleName, path.c_str(), file);
}

bool HalStorage::openFileForWrite(const char* moduleName, const String& path, HalFile& file) {
  return openFileForWrite(moduleName, path.c_str(), file);
}

bool HalStorage::removeDir(const char* path) {
  std::error_code ec;
  return fs::remove_all(resolve(path), ec) > 0;
}

std::vector<String> HalStorage::listFiles(const char* path, int maxFiles) {
  std::vector<String> result;
  std::error_code ec;
  fs::path p = resolve(path);
  if (!fs::exists(p, ec) || !fs::is_directory(p, ec)) return result;

  for (const auto& entry : fs::directory_iterator(p, ec)) {
    result.push_back(String(entry.path().filename().string()));
    if (static_cast<int>(result.size()) >= maxFiles) break;
  }
  return result;
}

String HalStorage::readFile(const char* path) {
  HalFile f = open(path, O_RDONLY);
  if (!f) return String("");
  size_t sz = f.size();
  std::string s;
  s.resize(sz);
  f.read(&s[0], sz);
  return String(s);
}

bool HalStorage::readFileToStream(const char* path, Print& out, size_t chunkSize) {
  HalFile f = open(path, O_RDONLY);
  if (!f) return false;
  std::vector<uint8_t> buf(chunkSize);
  while (f.available() > 0) {
    int bytes = f.read(buf.data(), chunkSize);
    if (bytes <= 0) break;
    out.write(buf.data(), bytes);
  }
  return true;
}

size_t HalStorage::readFileToBuffer(const char* path, char* buffer, size_t bufferSize, size_t maxBytes) {
  if (!buffer || bufferSize == 0) return 0;
  HalFile f = open(path, O_RDONLY);
  if (!f) {
    buffer[0] = '\0';
    return 0;
  }
  size_t toRead = bufferSize - 1;
  if (maxBytes > 0 && maxBytes < toRead) toRead = maxBytes;
  int bytes = f.read(buffer, toRead);
  if (bytes < 0) bytes = 0;
  buffer[bytes] = '\0';
  return static_cast<size_t>(bytes);
}

bool HalStorage::writeFile(const char* path, const String& content) {
  HalFile f = open(path, O_WRONLY | O_CREAT | O_TRUNC);
  if (!f) return false;
  f.write(reinterpret_cast<const uint8_t*>(content.c_str()), content.length());
  f.close();
  return true;
}
