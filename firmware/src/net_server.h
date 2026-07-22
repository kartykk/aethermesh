#pragma once
#include "lora_mesh.h"

namespace NetServer {
    void init();
    void loop();
    void notify(const Msg& msg);       // push new message to all clients
    void notifyAck(const String& id);  // push ACK confirmation to WebSocket clients
}
