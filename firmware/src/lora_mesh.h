#pragma once
#include <Arduino.h>
#include <functional>
#include <vector>

struct Msg {
    String   id;
    String   from;      // sender node hex ID, e.g. "28"
    String   fromName;  // sender display name, e.g. "Alice"
    String   to;        // "all" or recipient node hex ID
    String   text;
    String   group;
    int      rssi;
    float    snr;
    uint8_t  hops;
    uint32_t ts;        // Unix timestamp (or uptime seconds if not synced)
};

struct NodeInfo {
    String   id;
    String   name;
    String   group;
    int      rssi;
    float    snr;
    uint32_t lastSeen;  // uptime seconds
};

namespace LoraMesh {
    extern std::function<void(const Msg&)>    onRx;   // message received
    extern std::function<void(const String&)> onAck;  // ACK received (arg = clientId)
    extern std::function<void(const String&)> onFail; // all retries exhausted (arg = clientId)

    void   init();
    void   loop();
    // clientId: app-level id for onAck/onFail callbacks; if empty, uid hex is used
    bool   send(const char* to, const char* text, const char* clientId = "");
    String nodeId();
    std::vector<NodeInfo> getNodes();
}
