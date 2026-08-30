#include "BatteryLog.h"

#include <HalPowerManager.h>
#include <HalStorage.h>
#include <Logging.h>
#include <time.h>

#include "CrossPointSettings.h"

namespace BatteryLog {

void logEvent(const char* event) {
  if (!SETTINGS.batteryLogEnabled) return;

  constexpr char PATH[] = "/.crosspoint/battery_log.csv";
  HalFile f = Storage.open(PATH, O_WRITE | O_APPEND | O_CREAT);
  if (!f) {
    LOG_DBG("BLOG", "Could not open battery log for append");
    return;
  }
  if (f.size() == 0) {
    f.write("epoch,event,battery_percent\r\n", 29);
  }

  char row[64];
  const int len = snprintf(row, sizeof(row), "%lu,%s,%u\r\n", static_cast<unsigned long>(time(nullptr)), event,
                           powerManager.getBatteryPercentage());
  if (len > 0) f.write(row, len);
  f.close();
}

}  // namespace BatteryLog
