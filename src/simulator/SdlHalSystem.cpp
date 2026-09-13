#include "SdlHalSystem.h"

namespace HalSystem {

void begin() {}
void checkPanic() {}
void clearPanic() {}

std::string getPanicInfo(bool) { return ""; }
bool hasPanicStack() { return false; }
std::string getPanicStackSummary() { return ""; }
bool isRebootFromPanic() { return false; }

}  // namespace HalSystem
