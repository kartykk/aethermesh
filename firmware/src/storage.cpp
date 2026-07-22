#include "storage.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <freertos/semphr.h>

namespace Storage {

static SemaphoreHandle_t  _mutex     = nullptr;
static std::vector<Msg>   _cache;          // in-memory ring buffer
static bool               _dirty     = false;
static uint32_t           _lastFlush = 0;

// ── serialise / deserialise helpers ──────────────────────────────────────────
static void msgToObj(JsonObject o, const Msg& m) {
    o["id"]   = m.id;
    o["from"] = m.from;
    o["fn"]   = m.fromName.isEmpty() ? m.from : m.fromName;
    o["to"]   = m.to;
    o["text"] = m.text;
    o["g"]    = m.group;
    o["rssi"] = m.rssi;
    o["snr"]  = serialized(String(m.snr, 1));
    o["hops"] = m.hops;
    o["ts"]   = m.ts;
}

static Msg objToMsg(JsonObject o) {
    Msg m;
    m.id       = o["id"]   | "";
    m.from     = o["from"] | "?";
    m.fromName = o["fn"]   | m.from.c_str();
    m.to       = o["to"]   | "all";
    m.text     = o["text"] | "";
    m.group    = o["g"]    | DEFAULT_GROUP;
    m.rssi     = o["rssi"] | 0;
    m.snr      = o["snr"]  | 0.0f;
    m.hops     = o["hops"] | 0;
    m.ts       = o["ts"]   | (uint32_t)0;
    return m;
}

// ── flush cache → flash ───────────────────────────────────────────────────────
static void flush() {
    File f = LittleFS.open(MSG_FILE, "w");
    if (!f) return;
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& m : _cache) msgToObj(arr.add<JsonObject>(), m);
    serializeJson(doc, f);
    f.close();
    _dirty     = false;
    _lastFlush = millis();
}

// ── Public ────────────────────────────────────────────────────────────────────
void init() {
    _mutex = xSemaphoreCreateMutex();

    if (!LittleFS.begin(true)) {
        Serial.println("[Storage] LittleFS FAILED");
        return;
    }

    // Load existing messages into cache
    File f = LittleFS.open(MSG_FILE, "r");
    if (f) {
        JsonDocument doc;
        if (deserializeJson(doc, f) == DeserializationError::Ok) {
            for (JsonObject o : doc.as<JsonArray>()) {
                Msg m = objToMsg(o);
                if (!m.text.isEmpty()) _cache.push_back(m);
            }
        }
        f.close();
    }

    Serial.printf("[Storage] ready | %d msgs | used: %u KB / %u KB\n",
                  (int)_cache.size(),
                  LittleFS.usedBytes() / 1024,
                  LittleFS.totalBytes() / 1024);
}

void save(const Msg& msg) {
    if (!_mutex) return;
    xSemaphoreTake(_mutex, portMAX_DELAY);

    _cache.push_back(msg);
    while ((int)_cache.size() > MSG_MAX)
        _cache.erase(_cache.begin());  // remove oldest

    _dirty = true;
    xSemaphoreGive(_mutex);
}

void tick() {
    if (!_dirty || !_mutex) return;
    if (millis() - _lastFlush < FLUSH_INTERVAL) return;

    xSemaphoreTake(_mutex, portMAX_DELAY);
    flush();
    xSemaphoreGive(_mutex);
}

std::vector<Msg> get(int limit) {
    std::vector<Msg> result;
    if (!_mutex) return result;

    xSemaphoreTake(_mutex, portMAX_DELAY);
    int start = max(0, (int)_cache.size() - limit);
    result = std::vector<Msg>(_cache.begin() + start, _cache.end());
    xSemaphoreGive(_mutex);
    return result;
}

void clear() {
    if (!_mutex) return;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _cache.clear();
    LittleFS.remove(MSG_FILE);
    _dirty = false;
    xSemaphoreGive(_mutex);
    Serial.println("[Storage] cleared");
}

size_t usedBytes()  { return LittleFS.usedBytes(); }
size_t totalBytes() { return LittleFS.totalBytes(); }

} // namespace Storage
