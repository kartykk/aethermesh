#include "lora_mesh.h"
#include "config.h"
#include "settings.h"
#include "mesh_time.h"
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <mbedtls/aes.h>

namespace LoraMesh {

std::function<void(const Msg&)>    onRx;
std::function<void(const String&)> onAck;
std::function<void(const String&)> onFail;

// ── state ─────────────────────────────────────────────────────────────────────
static uint8_t  _nodeId = 0;
static uint16_t _seq    = 0;

static uint32_t _dedup[DEDUP_SIZE] = {};
static uint8_t  _dedupIdx = 0;

// TX queue — from async web/tcp tasks
struct TxItem { char to[16]; char text[200]; char clientId[16]; };
static QueueHandle_t _txQueue = nullptr;

// Pending outbound ACKs (delayed to let sender switch to RX)
struct AckItem { uint8_t src; uint16_t seq; uint32_t sendAt; };
static AckItem _ackQueue[8];
static int     _ackQueueLen = 0;

// Messages awaiting ACK — supports retry
struct PendingAck {
    uint32_t uid;          // current packet uid (updated per retry)
    char     clientId[16]; // app-level id for callbacks (never changes)
    uint32_t nextRetry;    // millis() when to retry / expire
    uint8_t  retries;      // retries done so far
    char     to[16];
    char     text[200];
};
static PendingAck _pending[16];
static int        _pendingLen = 0;

// Known nodes (seen via beacon or message)
static NodeInfo _nodes[MAX_KNOWN_NODES];
static int      _nodeCount = 0;

static uint32_t _lastBeacon    = 0;
static uint32_t _activityUntil = 0; // LED fast-blink window

// ── helpers ───────────────────────────────────────────────────────────────────
static bool seenBefore(uint32_t uid) {
    for (int i = 0; i < DEDUP_SIZE; i++) if (_dedup[i] == uid) return true;
    _dedup[_dedupIdx++ % DEDUP_SIZE] = uid;
    return false;
}

// AES-128-CTR using key derived from mesh passphrase (SHA-256 first 16 bytes)
static void aes_ctr(const uint8_t* in, uint8_t* out, size_t len,
                    uint8_t src, uint16_t seq) {
    uint8_t KEY[16];
    Settings::meshKey(KEY);
    uint8_t nonce[16] = {};
    nonce[0] = src; nonce[1] = (uint8_t)(seq >> 8); nonce[2] = (uint8_t)(seq & 0xFF);
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, KEY, 128);
    size_t nc_off = 0; uint8_t stream[16] = {};
    mbedtls_aes_crypt_ctr(&ctx, len, &nc_off, nonce, stream, const_cast<uint8_t*>(in), out);
    mbedtls_aes_free(&ctx);
}

static void updateNode(const String& id, const String& name, const String& grp,
                       int rssi, float snr) {
    for (int i = 0; i < _nodeCount; i++) {
        if (_nodes[i].id == id) {
            _nodes[i].name = name; _nodes[i].group = grp;
            _nodes[i].rssi = rssi; _nodes[i].snr   = snr;
            _nodes[i].lastSeen = millis() / 1000;
            return;
        }
    }
    if (_nodeCount < MAX_KNOWN_NODES)
        _nodes[_nodeCount++] = {id, name, grp, rssi, snr, millis() / 1000};
}

// ── Packet layout ─────────────────────────────────────────────────────────────
// [0-1] magic 0xAB 0xCD
// [2-3] seq   uint16 big-endian
// [4]   src   source node ID
// [5]   dst   0xFF = broadcast, else specific node ID
// [6]   ttl
// [7]   hops
// [8+]  AES-128-CTR encrypted JSON payload

static void transmitRaw(uint8_t* header, const uint8_t* payload, size_t plen) {
    uint8_t cipher[220];
    uint16_t seq = ((uint16_t)header[2] << 8) | header[3];
    aes_ctr(payload, cipher, plen, header[4], seq);
    LoRa.beginPacket();
    LoRa.write(header, 8);
    LoRa.write(cipher, plen);
    LoRa.endPacket();
    _activityUntil = millis() + 300;
}

// Build and transmit a message packet. Returns uid (0 on failure).
// Does NOT touch _pending — caller decides whether to track.
static uint32_t _doTransmit(const char* to, const char* text) {
    JsonDocument doc;
    doc["f"]  = nodeId();
    doc["fn"] = Settings::nodeName();
    doc["t"]  = to;
    doc["m"]  = text;
    doc["g"]  = Settings::group();

    uint8_t plain[220];
    size_t  plen = serializeJson(doc, (char*)plain, sizeof(plain));
    if (!plen) return 0;

    uint16_t seq = _seq++;
    uint32_t uid = ((uint32_t)_nodeId << 16) | seq;
    seenBefore(uid); // mark so we don't relay our own packet

    // Parse destination: "all" → 0xFF broadcast, else hex string → byte
    uint8_t dstByte = 0xFF;
    if (to && strcmp(to, "all") != 0 && strlen(to) > 0)
        dstByte = (uint8_t)strtol(to, nullptr, 16);

    uint8_t header[8];
    header[0]=0xAB; header[1]=0xCD;
    header[2]=(uint8_t)(seq>>8); header[3]=(uint8_t)(seq&0xFF);
    header[4]=_nodeId; header[5]=dstByte; header[6]=LORA_TTL; header[7]=0;

    transmitRaw(header, plain, plen);
    return uid;
}

// Transmit and register for ACK / retry tracking
static void transmit(const TxItem& item) {
    uint32_t uid = _doTransmit(item.to, item.text);
    if (!uid) return;
    if (_pendingLen < 16) {
        PendingAck& pa = _pending[_pendingLen++];
        pa.uid = uid;
        // clientId: use provided or fall back to uid hex
        if (item.clientId[0])
            strlcpy(pa.clientId, item.clientId, sizeof(pa.clientId));
        else
            snprintf(pa.clientId, sizeof(pa.clientId), "%lx", uid);
        pa.nextRetry = millis() + RETRY_INTERVAL_MS;
        pa.retries   = 0;
        strlcpy(pa.to,   item.to,   sizeof(pa.to));
        strlcpy(pa.text, item.text, sizeof(pa.text));
    }
}

static void sendAck(uint8_t origSrc, uint16_t origSeq) {
    char uidHex[12];
    snprintf(uidHex, sizeof(uidHex), "%lx", ((uint32_t)origSrc << 16) | origSeq);

    JsonDocument doc;
    doc["type"] = "ack";
    doc["id"]   = uidHex;

    uint8_t plain[64];
    size_t  plen = serializeJson(doc, (char*)plain, sizeof(plain));
    if (!plen) return;

    uint16_t seq = _seq++;
    uint32_t uid = ((uint32_t)_nodeId << 16) | seq;
    seenBefore(uid);

    uint8_t header[8];
    header[0]=0xAB; header[1]=0xCD;
    header[2]=(uint8_t)(seq>>8); header[3]=(uint8_t)(seq&0xFF);
    header[4]=_nodeId; header[5]=origSrc; header[6]=ACK_TTL; header[7]=0;

    transmitRaw(header, plain, plen);
}

static void sendBeacon() {
    JsonDocument doc;
    doc["type"] = "beacon";
    doc["f"]    = nodeId();
    doc["fn"]   = Settings::nodeName();
    doc["g"]    = Settings::group();

    uint8_t plain[80];
    size_t  plen = serializeJson(doc, (char*)plain, sizeof(plain));
    if (!plen) return;

    uint16_t seq = _seq++;
    uint32_t uid = ((uint32_t)_nodeId << 16) | seq;
    seenBefore(uid);

    uint8_t header[8];
    header[0]=0xAB; header[1]=0xCD;
    header[2]=(uint8_t)(seq>>8); header[3]=(uint8_t)(seq&0xFF);
    header[4]=_nodeId; header[5]=0xFF; header[6]=LORA_TTL; header[7]=0;

    transmitRaw(header, plain, plen);
    Serial.printf("[LoRa] beacon tx | node:%s group:%s\n",
                  nodeId().c_str(), Settings::group().c_str());
}

// ── Public API ────────────────────────────────────────────────────────────────
String nodeId() {
    if (_nodeId == 0) _nodeId = (uint8_t)(ESP.getEfuseMac() & 0xFF);
    char buf[3];
    snprintf(buf, sizeof(buf), "%02X", _nodeId);
    return String(buf);
}

std::vector<NodeInfo> getNodes() {
    std::vector<NodeInfo> result;
    uint32_t now = millis() / 1000;
    for (int i = 0; i < _nodeCount; i++) {
        if (now - _nodes[i].lastSeen < 120) // seen in last 2 minutes
            result.push_back(_nodes[i]);
    }
    return result;
}

void init() {
    _txQueue = xQueueCreate(8, sizeof(TxItem));
    _nodeId  = (uint8_t)(ESP.getEfuseMac() & 0xFF);

    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(LORA_FREQ)) {
        Serial.println("[LoRa] FAILED — check wiring");
        return;
    }
    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setCodingRate4(LORA_CR);
    LoRa.setTxPower(LORA_TX_POWER);

    Serial.printf("[LoRa] ready | node:%s | 433MHz SF%d | AES-CTR | retry:%d\n",
                  nodeId().c_str(), LORA_SF, RETRY_COUNT);
}

bool isActive() { return millis() < _activityUntil; }

bool send(const char* to, const char* text, const char* clientId) {
    if (!_txQueue) return false;
    TxItem item;
    strlcpy(item.to,       to,                  sizeof(item.to));
    strlcpy(item.text,     text,                sizeof(item.text));
    strlcpy(item.clientId, clientId ? clientId : "", sizeof(item.clientId));
    return xQueueSend(_txQueue, &item, 0) == pdTRUE;
}

void loop() {
    uint32_t now = millis();

    // 1. Drain TX queue
    TxItem tx;
    while (xQueueReceive(_txQueue, &tx, 0) == pdTRUE)
        transmit(tx);

    // 2. Drain delayed ACK queue
    for (int i = 0; i < _ackQueueLen; ) {
        if (now >= _ackQueue[i].sendAt) {
            sendAck(_ackQueue[i].src, _ackQueue[i].seq);
            _ackQueue[i] = _ackQueue[--_ackQueueLen];
        } else {
            i++;
        }
    }

    // 3. Pending ACK timeouts: retry up to RETRY_COUNT times, then fail
    for (int i = 0; i < _pendingLen; ) {
        if (now >= _pending[i].nextRetry) {
            if (_pending[i].retries < RETRY_COUNT) {
                _pending[i].retries++;
                uint32_t newUid = _doTransmit(_pending[i].to, _pending[i].text);
                if (newUid) _pending[i].uid = newUid; // track new uid for ACK matching
                _pending[i].nextRetry = now + RETRY_INTERVAL_MS;
                Serial.printf("[LoRa] retry %d/%d for %s\n",
                              _pending[i].retries, RETRY_COUNT, _pending[i].clientId);
                i++;
            } else {
                Serial.printf("[LoRa] FAIL (no ACK) for %s\n", _pending[i].clientId);
                if (onFail) onFail(String(_pending[i].clientId));
                _pending[i] = _pending[--_pendingLen];
            }
        } else {
            i++;
        }
    }

    // 4. Beacon
    if (now - _lastBeacon > BEACON_INTERVAL) {
        _lastBeacon = now;
        sendBeacon();
    }

    // 5. Receive
    int pktSize = LoRa.parsePacket();
    if (pktSize < 9) return;

    _activityUntil = millis() + 300;

    uint8_t buf[256];
    int n = 0;
    while (LoRa.available() && n < (int)sizeof(buf) - 1)
        buf[n++] = (uint8_t)LoRa.read();

    if (buf[0] != 0xAB || buf[1] != 0xCD) return;

    uint16_t seq  = ((uint16_t)buf[2] << 8) | buf[3];
    uint8_t  src  = buf[4];
    uint8_t  dst  = buf[5];
    uint8_t  ttl  = buf[6];
    uint8_t  hops = buf[7];
    int      plen = n - 8;

    uint32_t uid = ((uint32_t)src << 16) | seq;
    if (seenBefore(uid)) return;

    uint8_t plain[220];
    if (plen > (int)sizeof(plain)) return;
    aes_ctr(buf + 8, plain, plen, src, seq);
    plain[plen] = '\0';

    JsonDocument doc;
    if (deserializeJson(doc, plain) != DeserializationError::Ok) return;

    const char* pktType = doc["type"] | "msg";

    // ── Beacon ──
    if (strcmp(pktType, "beacon") == 0) {
        String nId   = doc["f"]  | "?";
        String nName = doc["fn"] | nId.c_str();
        String nGrp  = doc["g"]  | DEFAULT_GROUP;
        updateNode(nId, nName, nGrp, LoRa.packetRssi(), LoRa.packetSnr());
        Serial.printf("[LoRa] beacon from:%s(\"%s\") rssi:%d\n",
                      nId.c_str(), nName.c_str(), LoRa.packetRssi());
        if (ttl > 1) {
            buf[6] = ttl - 1; buf[7] = hops + 1;
            LoRa.beginPacket(); LoRa.write(buf, n); LoRa.endPacket();
        }
        return;
    }

    // ── ACK ──
    if (strcmp(pktType, "ack") == 0) {
        if (dst == _nodeId || dst == 0xFF) {
            String ackId = doc["id"] | "";
            for (int i = 0; i < _pendingLen; ) {
                char uidHex[12];
                snprintf(uidHex, sizeof(uidHex), "%lx", _pending[i].uid);
                if (ackId == String(uidHex)) {
                    Serial.printf("[LoRa] ACK for %s (client:%s) rssi:%d\n",
                                  uidHex, _pending[i].clientId, LoRa.packetRssi());
                    if (onAck) onAck(String(_pending[i].clientId));
                    _pending[i] = _pending[--_pendingLen];
                } else {
                    i++;
                }
            }
        }
        if (ttl > 1) {
            buf[6] = ttl - 1; buf[7] = hops + 1;
            LoRa.beginPacket(); LoRa.write(buf, n); LoRa.endPacket();
        }
        return;
    }

    // ── Regular message ──
    Msg msg;
    msg.id       = String(uid, HEX);
    msg.from     = doc["f"]  | "?";
    msg.fromName = doc["fn"] | msg.from.c_str();
    msg.to       = doc["t"]  | "all";
    msg.text     = doc["m"]  | "";
    msg.group    = doc["g"]  | DEFAULT_GROUP;
    msg.rssi     = LoRa.packetRssi();
    msg.snr      = LoRa.packetSnr();
    msg.hops     = hops;
    msg.ts       = MeshTime::now();

    if (msg.text.isEmpty()) return;

    updateNode(msg.from, msg.fromName, msg.group, msg.rssi, msg.snr);

    // Deliver if we are the intended recipient (broadcast or directed to us)
    bool isForMe = (dst == 0xFF || dst == _nodeId);
    if (msg.group == Settings::group() && isForMe) {
        Serial.printf("[LoRa] msg from:%s(\"%s\") to:%s \"%s\" rssi:%d hops:%d\n",
                      msg.from.c_str(), msg.fromName.c_str(),
                      msg.to.c_str(), msg.text.c_str(), msg.rssi, hops);
        if (onRx) onRx(msg);

        // Queue ACK with delay so sender has time to switch to RX
        if (_ackQueueLen < 8)
            _ackQueue[_ackQueueLen++] = {src, seq, millis() + ACK_DELAY_MS};
    }

    // Relay if TTL remaining and we are not the final destination
    if (ttl > 1 && dst != _nodeId) {
        buf[6] = ttl - 1; buf[7] = hops + 1;
        LoRa.beginPacket(); LoRa.write(buf, n); LoRa.endPacket();
    }
}

} // namespace LoraMesh
