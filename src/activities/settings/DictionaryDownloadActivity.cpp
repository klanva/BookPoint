#include "DictionaryDownloadActivity.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include "network/HttpDownloader.h"
#include <I18n.h>
#include <Logging.h>
#include <ZipFile.h>

#include <cstdio>

#include "components/UITheme.h"
#include "fontIds.h"
#include "MappedInputManager.h"

namespace {
constexpr char TMP_ZIP[] = "/.crosspoint/dict_dl.zip";
constexpr char DICT_DIR[] = "/dictionaries";
// The WikDict ZIPs hold one folder with three StarDict files each.
const char* const kSuffixes[] = {".ifo", ".idx", ".dict"};
constexpr int kSuffixCount = 3;
}  // namespace

const DictionaryDownloadActivity::Entry DictionaryDownloadActivity::kEntries[ENTRY_COUNT] = {
    {"https://download.wikdict.com/dictionaries/stardict/wikdict-ru-en.zip", "wikdict-ru-en"},
    {"https://download.wikdict.com/dictionaries/stardict/wikdict-en-ru.zip", "wikdict-en-ru"},
};

namespace {

// Streams one ZIP entry to a file on the SD card.
class FileSink : public Print {
 public:
  explicit FileSink(HalFile& f) : file_(f) {}
  size_t write(const uint8_t* data, size_t len) override {
    unpackedBytes_ += len;
    return file_.write(data, len) == len ? len : 0;
  }
  size_t write(uint8_t b) override { return write(&b, 1); }
  uint32_t unpackedBytes_ = 0;

 private:
  HalFile& file_;
};

}  // namespace

void DictionaryDownloadActivity::onEnter() {
  Activity::onEnter();
  selected_ = 0;
  state_ = State::LIST;
  requestUpdate();
}

bool DictionaryDownloadActivity::isInstalled(const int index) const {
  const std::string marker = std::string(DICT_DIR) + "/" + kEntries[index].folder + "/stardict.ifo";
  return Storage.exists(marker.c_str());
}

void DictionaryDownloadActivity::install(const int index) {
  activeEntry_ = index;
  const Entry& entry = kEntries[index];

  if (!Storage.exists(DICT_DIR)) Storage.mkdir(DICT_DIR);

  // --- download ---
  state_ = State::DOWNLOADING;
  fileProgress_ = 0;
  fileTotal_ = 0;
  requestUpdate(true);
  if (Storage.exists(TMP_ZIP)) Storage.remove(TMP_ZIP);

  const auto progress = [this](const size_t downloaded, const size_t total) {
    fileProgress_ = downloaded;
    fileTotal_ = total;
  };
  const auto result = HttpDownloader::downloadToFile(entry.url, TMP_ZIP, progress);
  if (result != HttpDownloader::OK) {
    LOG_ERR("DICTDL", "Download failed: %d", static_cast<int>(result));
    Storage.remove(TMP_ZIP);
    state_ = State::FAILED;
    requestUpdate();
    return;
  }

  // --- unpack ---
  state_ = State::UNPACKING;
  unpackedBytes_ = 0;
  requestUpdate(true);
  const std::string destDir = std::string(DICT_DIR) + "/" + entry.folder;
  if (!Storage.exists(destDir.c_str())) Storage.mkdir(destDir.c_str());

  ZipFile zip(TMP_ZIP);
  for (int i = 0; i < kSuffixCount; ++i) {
    std::string entryPath = std::string("/") + entry.folder + "/stardict" + kSuffixes[i];
    const std::string destPath = destDir + "/stardict" + kSuffixes[i];
    HalFile out;
    if (!Storage.openFileForWrite("DICTDL", destPath.c_str(), out)) {
      LOG_ERR("DICTDL", "Cannot open %s", destPath.c_str());
      Storage.remove(TMP_ZIP);
      state_ = State::FAILED;
      requestUpdate();
      return;
    }
    FileSink sink(out);
    if (!zip.readFileToStream(entryPath.c_str(), sink, 1024)) {
      LOG_ERR("DICTDL", "Cannot extract %s", entryPath.c_str());
      out.close();
      Storage.remove(TMP_ZIP);
      state_ = State::FAILED;
      requestUpdate();
      return;
    }
    out.flush();
    out.close();
    unpackedBytes_ = sink.unpackedBytes_;
  }

  Storage.remove(TMP_ZIP);
  LOG_INF("DICTDL", "Installed dictionary %s", entry.folder);
  state_ = State::DONE;
  requestUpdate();
}

void DictionaryDownloadActivity::loop() {
  if (state_ == State::DOWNLOADING || state_ == State::UNPACKING) {
    return;  // busy; input ignored until finished
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    selected_ = (selected_ + ENTRY_COUNT - 1) % ENTRY_COUNT;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    selected_ = (selected_ + 1) % ENTRY_COUNT;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm) ||
      mappedInput.wasReleased(MappedInputManager::Button::Power)) {
    if (state_ != State::DONE || true) {
      install(selected_);  // re-install overwrites; the ZIP is tiny enough
      return;
    }
  }
}

void DictionaryDownloadActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_DICT_DOWNLOAD));

  int y = metrics.topPadding + metrics.headerHeight + 18;

  if (state_ == State::DOWNLOADING || state_ == State::UNPACKING) {
    renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2 - 30, tr(STR_DOWNLOADING), true);
    char buf[64];
    if (state_ == State::DOWNLOADING) {
      if (fileTotal_ > 0) {
        snprintf(buf, sizeof(buf), "%u%%", static_cast<unsigned>(fileProgress_ * 100 / fileTotal_));
      } else {
        snprintf(buf, sizeof(buf), "%u KB", static_cast<unsigned>(fileProgress_ / 1024));
      }
    } else {
      snprintf(buf, sizeof(buf), "%u KB", static_cast<unsigned>(unpackedBytes_ / 1024));
    }
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 + 10, buf, true, EpdFontFamily::BOLD);
  } else {
    for (int i = 0; i < ENTRY_COUNT; ++i) {
      const bool selected = i == selected_ && state_ != State::DONE;
      if (selected) {
        renderer.fillRect(12, y - 4, pageWidth - 24, 34, true);
      }
      const bool invert = selected;
      const char* title = i == 0 ? tr(STR_DICT_RU_EN) : tr(STR_DICT_EN_RU);
      renderer.drawText(UI_12_FONT_ID, 20, y, title, !invert);
      const char* status = isInstalled(i) ? tr(STR_DICT_INSTALLED) : tr(STR_DICT_GET);
      const int sw = renderer.getTextAdvanceX(UI_10_FONT_ID, status, EpdFontFamily::REGULAR);
      renderer.drawText(UI_10_FONT_ID, pageWidth - 20 - sw, y + 4, status, !invert);
      y += 44;
    }

    if (state_ == State::DONE) {
      renderer.drawCenteredText(UI_10_FONT_ID, y + 16, tr(STR_DICT_DONE_OK), true, EpdFontFamily::BOLD);
    } else if (state_ == State::FAILED) {
      renderer.drawCenteredText(UI_10_FONT_ID, y + 16, tr(STR_DICT_FAILED), true, EpdFontFamily::BOLD);
    } else {
      renderer.drawText(UI_10_FONT_ID, 20, y + 16, tr(STR_DICT_HINT));
    }
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
