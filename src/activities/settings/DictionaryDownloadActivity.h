#pragma once

#include "../Activity.h"

// Downloads curated StarDict dictionaries (WikDict RU-EN / EN-RU) to
// /dictionaries/ on the SD card over Wi-Fi. One entry per dictionary; the
// ZIP is streamed to a temp file on SD and unpacked entry by entry with the
// existing ZipFile reader, so the multi-megabyte payload never sits in RAM.
class DictionaryDownloadActivity final : public Activity {
 public:
  DictionaryDownloadActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("DictDownload", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  struct Entry {
    const char* url;
    const char* folder;  // target folder name under /dictionaries/
  };

  static const Entry kEntries[2];
  static constexpr int ENTRY_COUNT = 2;

  enum class State : uint8_t { LIST, DOWNLOADING, UNPACKING, DONE, FAILED };

  bool isInstalled(int index) const;
  void install(int index);

  int selected_ = 0;
  State state_ = State::LIST;
  int activeEntry_ = -1;
  uint32_t fileProgress_ = 0;
  uint32_t fileTotal_ = 0;
  uint32_t unpackedBytes_ = 0;
};
