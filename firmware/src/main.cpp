#include <Arduino.h>
#include "config.h"
#include "lora_mesh.h"
#include "settings.h"
#include "storage.h"
#include "net_server.h"

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.printf("\n=== AetherMesh v%s ===\n", FIRMWARE_VER);

    Storage::init();

    LoraMesh::onRx = [](const Msg& msg) {
        Storage::save(msg);
        NetServer::notify(msg);
    };

    LoraMesh::onAck = [](const String& id) {
        NetServer::notifyAck(id);
    };

    LoraMesh::onFail = [](const String& id) {
        NetServer::notifyFail(id);
    };

    LoraMesh::init();

    Settings::init();  // after LoraMesh so nodeId() is ready for default name

    NetServer::init(); // after Settings so WiFi uses correct credentials

    Serial.println("[Main] ready\n");
}

void loop() {
    LoraMesh::loop();
    NetServer::loop();
    Storage::tick();   // flush cache to flash if dirty
}
