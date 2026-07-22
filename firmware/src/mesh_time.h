#pragma once
#include <Arduino.h>

namespace MeshTime {
    void     setFromBrowser(uint32_t unixTs);  // called when browser POSTs its time
    void     setFromNTP();                      // called after router WiFi connects
    uint32_t now();                             // Unix timestamp, or uptime if not synced
    bool     isSynced();
}
