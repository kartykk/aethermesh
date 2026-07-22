#pragma once
#include <Arduino.h>

namespace Settings {
    void   init();           // load from NVS; call after LoraMesh::init()

    String nodeName();       // user display name, e.g. "Alice"
    String apPass();         // AP WiFi password
    String group();          // channel — both nodes must match to communicate
    String routerSSID();     // optional: router to join for internet
    String routerPass();     // optional: router password

    // Save all fields and restart.
    // Pass empty string for pass to keep existing password.
    bool save(const char* name, const char* pass, const char* grp,
              const char* routerSSID, const char* routerPass);
}
