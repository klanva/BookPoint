#include "PersistableStore.h"

#include <HalPowerManager.h>
#include <HalStorage.h>
#include <Logging.h>
#include <ObfuscationUtils.h>

#include <cstring>
#include <limits>

bool PersistableStoreBase::writeDocToFile(const char* path, const JsonDocument& doc) {
  const uint16_t vBat = powerManager.getBatteryVoltageMv();
  if (vBat > 0 && vBat < 3200) {
    LOG_ERR("PERSIST", "Brownout guard: battery voltage %u mV < 3200 mV, aborting write to %s", vBat, path);
    return false;
  }

  Storage.mkdir("/.crosspoint");
  String json;
  serializeJson(doc, json);

  char tmpPath[128];
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);

  if (!Storage.writeFile(tmpPath, json)) {
    LOG_ERR("PERSIST", "Failed to write %s", tmpPath);
    return false;
  }
  Storage.remove(path);
  if (!Storage.rename(tmpPath, path)) {
    LOG_ERR("PERSIST", "Failed to rename %s to %s", tmpPath, path);
    return false;
  }
  return true;
}

bool PersistableStoreBase::readDocFromFile(const char* path, JsonDocument& doc) {
  char tmpPath[128];
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);

  const char* readPath = path;
  if (!Storage.exists(path)) {
    if (Storage.exists(tmpPath)) {
      LOG_INF("PERSIST", "Primary %s missing, attempting recovery from %s", path, tmpPath);
      readPath = tmpPath;
    } else {
      return false;  // Expected on first boot — not an error.
    }
  }
  String json = Storage.readFile(readPath);
  if (json.isEmpty()) {
    if (readPath == path && Storage.exists(tmpPath)) {
      LOG_INF("PERSIST", "Primary %s empty, attempting recovery from %s", path, tmpPath);
      json = Storage.readFile(tmpPath);
    }
    if (json.isEmpty()) {
      LOG_ERR("PERSIST", "Failed to read %s (empty)", readPath);
      return false;
    }
  }
  auto error = deserializeJson(doc, json);
  if (error) {
    LOG_ERR("PERSIST", "JSON parse error in %s: %s", readPath, error.c_str());
    if (readPath == path && Storage.exists(tmpPath)) {
      LOG_INF("PERSIST", "Attempting JSON recovery from %s", tmpPath);
      String tmpJson = Storage.readFile(tmpPath);
      if (!tmpJson.isEmpty() && !deserializeJson(doc, tmpJson)) {
        return true;
      }
    }
    return false;
  }
  return true;
}

std::string PersistableStoreBase::extractPassword(JsonVariantConst doc, bool& needsResave) {
  bool valid = false;
  return extractPassword(doc, needsResave, std::numeric_limits<size_t>::max(), valid);
}

std::string PersistableStoreBase::extractPassword(JsonVariantConst doc, bool& needsResave, const size_t maxLength,
                                                  bool& valid) {
  valid = true;
  bool ok = false;
  bool tooLong = false;
  std::string pass = obfuscation::deobfuscateFromBase64(doc["password_obf"] | "", maxLength, &ok, &tooLong);
  if (tooLong) {
    valid = false;
    return "";
  }
  if (!ok) {
    // Deobfuscation failed — fall back to legacy plaintext password.
    const char* legacyPassword = doc["password"] | "";
    const size_t legacyLength = strlen(legacyPassword);
    if (legacyLength > maxLength) {
      valid = false;
      return "";
    }
    pass.assign(legacyPassword, legacyLength);
    if (!pass.empty()) needsResave = true;
  }
  // A successfully decoded empty string is a legitimate value; preserve as-is.
  return pass;
}
