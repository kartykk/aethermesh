#include <Arduino.h>
#include "config.h"
#include "lora_mesh.h"
#include "settings.h"
#include "storage.h"
#include "net_server.h"

// ── LED ───────────────────────────────────────────────────────────────────────
static void ledTick() {
    static uint32_t lastToggle = 0;
    static bool     state      = false;
    // Fast (100ms) when LoRa is active, slow (500ms) when idle
    uint32_t interval = LoraMesh::isActive() ? 100 : 500;
    if (millis() - lastToggle >= interval) {
        state = !state;
        digitalWrite(LED_PIN, state ? HIGH : LOW);
        lastToggle = millis();
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

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
    Storage::tick();
    ledTick();
}
