#include <functional>
#include <string>
#include <optional>
#include "Stream.h"

#include "activities/ActivityManager.h"
#include "activities/network/WifiSelectionActivity.h"
#include "activities/reader/KOReaderSyncActivity.h"
#include "activities/settings/SdFirmwareUpdateActivity.h"
#include "network/HttpDownloader.h"
#include "network/OtaUpdater.h"

extern ActivityManager activityManager;

// Silent restart stubs
void silentRestart() {
  activityManager.goHome();
}

void silentRestartToReader() {
  activityManager.goHome();
}

// HttpDownloader stubs
bool HttpDownloader::fetchUrl(const std::string&, std::string&, const std::string&, const std::string&) {
  return false;
}

bool HttpDownloader::fetchUrl(const std::string&, Stream&, const std::string&, const std::string&) {
  return false;
}

bool HttpDownloader::fetchUrl(const std::string&, const DataCallback&, const std::string&, const std::string&) {
  return false;
}

HttpDownloader::DownloadError HttpDownloader::downloadToFile(const std::string&, const std::string&,
                                                            ProgressCallback, bool*,
                                                            const std::string&, const std::string&) {
  return DownloadError::HTTP_ERROR;
}

// OtaUpdater stubs
bool OtaUpdater::isUpdateNewer() const {
  return false;
}

static const std::string s_simVersion = "2.0.0";
const std::string& OtaUpdater::getLatestVersion() const {
  return s_simVersion;
}

OtaUpdater::OtaUpdaterError OtaUpdater::checkForUpdate() {
  return OtaUpdater::NO_UPDATE;
}

OtaUpdater::OtaUpdaterError OtaUpdater::installUpdate(ProgressCallback, void*) {
  return OtaUpdater::NO_UPDATE;
}

// WifiSelectionActivity stubs
WifiSelectionActivity::WifiSelectionActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, bool autoConnect)
    : Activity("WifiSelection", renderer, mappedInput),
      UiAppHost(renderer),
      allowAutoConnect(autoConnect) {}

void WifiSelectionActivity::onEnter() {
  finish();
}

void WifiSelectionActivity::onExit() {}
void WifiSelectionActivity::loop() {}
void WifiSelectionActivity::render(RenderLock&&) {}

// KOReaderSyncActivity stubs
KOReaderSyncActivity::KOReaderSyncActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                           const std::string& epubPath, int currentSpineIndex, int currentPage,
                                           int totalPagesInSpine, SavedProgressPosition localKoPos,
                                           std::string localChapterName,
                                           std::optional<uint16_t> currentParagraphIndex)
    : Activity("KOReaderSync", renderer, mappedInput),
      UiAppHost(renderer),
      epubPath(epubPath),
      localChapterName(localChapterName),
      currentSpineIndex(currentSpineIndex),
      currentPage(currentPage),
      totalPagesInSpine(totalPagesInSpine),
      currentParagraphIndex(currentParagraphIndex) {}

void KOReaderSyncActivity::onEnter() {
  finish();
}

void KOReaderSyncActivity::onExit() {}
void KOReaderSyncActivity::loop() {}
void KOReaderSyncActivity::render(RenderLock&&) {}

// SdFirmwareUpdateActivity stubs
void SdFirmwareUpdateActivity::onEnter() {
  finish();
}

void SdFirmwareUpdateActivity::loop() {}
void SdFirmwareUpdateActivity::render(RenderLock&&) {}
