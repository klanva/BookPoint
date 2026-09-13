#pragma once

#define SNTP_SYNC_STATUS_RESET 0
#define SNTP_SYNC_STATUS_COMPLETED 1
#define SNTP_SYNC_STATUS_IN_PROGRESS 2

inline void sntp_set_sync_status(int) {}
