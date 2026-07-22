#pragma once
#include <Arduino.h>

#define FIRMWARE_VER      "0.3.0"

// LoRa — SX1278 RA-02
#define LORA_SS           5
#define LORA_RST          14
#define LORA_DIO0         26
#define LORA_FREQ         433E6
#define LORA_SF           9
#define LORA_BW           125E3
#define LORA_CR           5
#define LORA_TX_POWER     17
#define LORA_TTL          5      // max hops for regular messages

// Mesh protocol
#define ACK_TTL           3      // max hops for ACK packets
#define BEACON_INTERVAL   30000  // ms between beacon broadcasts
#define ACK_DELAY_MS      250    // ms delay before sending ACK (let sender switch to RX)
#define ACK_TIMEOUT_MS    30000  // ms before giving up on ACK
#define MAX_KNOWN_NODES   12     // max nodes tracked in memory

// WiFi AP defaults — overridden by NVS settings
#define WIFI_SSID_PFX     "AetherMesh"
#define DEFAULT_AP_PASS   "12345678"   // 8-digit PIN (WPA2 min is 8 chars)
#define DEFAULT_GROUP     "default"
#define WIFI_CHANNEL      6
#define WIFI_MAX_CLIENTS  4

// Network
#define HTTP_PORT         80
#define TCP_PORT          4403

// AES-128-CTR shared key — change before deploying
#define MESH_KEY { 0x4C,0x6F,0x52,0x61,0x4D,0x65,0x73,0x68, \
                   0x4B,0x65,0x79,0x32,0x30,0x32,0x35,0x21 }
// ASCII: "LoRaMeshKey2025!"

// Storage
#define MSG_FILE          "/msgs.json"
#define MSG_MAX           100    // ring buffer size
#define FLUSH_INTERVAL    10000  // ms between flash writes (batch saves)
#define DEDUP_SIZE        30
