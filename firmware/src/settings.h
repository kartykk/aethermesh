#pragma once
#include <Arduino.h>

namespace Settings {
    void   init();

    String nodeName();
    String apPass();
    String group();
    String meshPass();               // network encryption passphrase
    void   meshKey(uint8_t out[16]); // SHA-256(meshPass)[0..15] — AES-128 key
    String routerSSID();
    String routerPass();

    bool save(const char* name, const char* pass, const char* grp,
              const char* routerSSID, const char* routerPass,
              const char* meshPass = nullptr);
}
