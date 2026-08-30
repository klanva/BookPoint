#pragma once

// Tiny append-only CSV diagnostic for battery drain investigations
// (/.crosspoint/battery_log.csv): one row per boot and per deep-sleep entry
// with the wall-clock epoch and battery percentage. Enabled by the
// "Battery log (diagnostics)" setting; disabled by default. Rows are a few
// dozen bytes written a handful of times per day — no battery cost to speak
// of, but enough to catch devices that wake or never sleep overnight.
namespace BatteryLog {

void logEvent(const char* event);

}  // namespace BatteryLog
