#pragma once

#include "Arduino.h"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

using oflag_t = uint32_t;
#ifndef O_RDONLY
#define O_RDONLY 0x00
#endif
#ifndef O_READ
#define O_READ 0x00
#endif
#ifndef O_WRONLY
#define O_WRONLY 0x01
#endif
#ifndef O_WRITE
#define O_WRITE 0x01
#endif
#ifndef O_RDWR
#define O_RDWR 0x02
#endif
#ifndef O_APPEND
#define O_APPEND 0x08
#endif
#ifndef O_CREAT
#define O_CREAT 0x0100
#endif
#ifndef O_TRUNC
#define O_TRUNC 0x0200
#endif

enum class UsbDriveState : uint8_t {
  Unsupported,
  WaitingForHost,
  Connected,
  Ejected,
  Disconnected,
  IoError,
};

class HalStorage;

class HalFile : public Print {
  friend class HalStorage;
 public:
  class Impl;

  HalFile();
  explicit HalFile(std::shared_ptr<Impl> impl);
  ~HalFile() override;
  HalFile(HalFile&&) noexcept;
  HalFile& operator=(HalFile&&) noexcept;
  HalFile(const HalFile&) = delete;
  HalFile& operator=(const HalFile&) = delete;

  void flush();
  size_t getName(char* name, size_t len);
  size_t size();
  size_t fileSize() { return size(); }
  uint64_t fileSize64() { return size(); }
  bool seek(size_t pos);
  bool seek64(uint64_t pos) { return seek(static_cast<size_t>(pos)); }
  bool seekCur(int64_t offset);
  bool seekSet(size_t offset) { return seek(offset); }
  int available() const;
  size_t position() const;
  int read(void* buf, size_t count);
  int read();
  size_t write(const uint8_t* buf, size_t count) override;
  size_t write(const void* buf, size_t count) {
    return write(reinterpret_cast<const uint8_t*>(buf), count);
  }
  size_t write(uint8_t b) override;
  bool rename(const char* newPath);
  bool isDirectory() const;
  bool getModifyDateTime(uint16_t* pdate, uint16_t* ptime);
  void rewindDirectory();
  bool close();
  HalFile openNextFile();
  bool isOpen() const;
  operator bool() const { return isOpen(); }

 private:
  std::shared_ptr<Impl> impl;
};

class HalStorage {
 public:
  static HalStorage& getInstance() {
    static HalStorage instance;
    return instance;
  }

  bool begin();
  bool ready() const { return true; }
  void prepareForDeepSleep() {}
  UsbDriveState usbDriveState() const { return UsbDriveState::Unsupported; }

  HalFile open(const char* path, oflag_t oflag = O_RDONLY);
  bool exists(const char* path) const;
  bool mkdir(const char* path, bool pFlag = true);
  bool remove(const char* path);
  bool rename(const char* oldPath, const char* newPath);
  bool rmdir(const char* path);
  bool ensureDirectoryExists(const char* path);

  bool openFileForRead(const char* moduleName, const char* path, HalFile& file);
  bool openFileForRead(const char* moduleName, const std::string& path, HalFile& file);
  bool openFileForRead(const char* moduleName, const String& path, HalFile& file);

  bool openFileForWrite(const char* moduleName, const char* path, HalFile& file);
  bool openFileForWrite(const char* moduleName, const std::string& path, HalFile& file);
  bool openFileForWrite(const char* moduleName, const String& path, HalFile& file);

  bool removeDir(const char* path);
  std::vector<String> listFiles(const char* path = "/", int maxFiles = 200);
  String readFile(const char* path);
  bool readFileToStream(const char* path, Print& out, size_t chunkSize = 256);
  size_t readFileToBuffer(const char* path, char* buffer, size_t bufferSize, size_t maxBytes = 0);
  bool writeFile(const char* path, const String& content);

  uint64_t sdTotalBytes() const { return 32ULL * 1024 * 1024 * 1024; }
  uint64_t sdFreeBytes() const { return 16ULL * 1024 * 1024 * 1024; }
  uint64_t sdUsedBytes() const { return 16ULL * 1024 * 1024 * 1024; }

  static fs::path resolve(const char* p);

 private:
  HalStorage() = default;
};

#define Storage HalStorage::getInstance()
using SdlHalStorage = HalStorage;
