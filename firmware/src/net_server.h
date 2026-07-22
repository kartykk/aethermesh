#pragma once
#include "lora_mesh.h"

namespace NetServer {
    void init();
    void loop();
    void notify(const Msg& msg);
    void notifyAck(const String& id);   // push ACK ✓✓ to WebSocket clients
    void notifyFail(const String& id);  // push FAIL ✗ to WebSocket clients
}
