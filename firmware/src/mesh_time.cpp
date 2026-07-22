#include "mesh_time.h"
#include <time.h>

namespace MeshTime {

static int64_t _offset = 0;
static bool    _synced = false;

void setFromBrowser(uint32_t unixTs) {
    _offset = (int64_t)unixTs - (int64_t)(millis() / 1000);
    _synced = true;
    Serial.printf("[Time] synced from browser | unix: %u\n", unixTs);
}

void setFromNTP() {
    struct tm ti;
    if (getLocalTime(&ti, 5000)) {
        time_t t = mktime(&ti);
        _offset  = (int64_t)t - (int64_t)(millis() / 1000);
        _synced  = true;
        Serial.printf("[Time] synced from NTP | unix: %ld\n", (long)t);
    } else {
        Serial.println("[Time] NTP sync failed");
    }
}

uint32_t now() {
    return (uint32_t)((int64_t)(millis() / 1000) + _offset);
}

bool isSynced() { return _synced; }

} // namespace MeshTime
