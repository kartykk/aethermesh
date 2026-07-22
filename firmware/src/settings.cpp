#include "settings.h"
#include "config.h"
#include "lora_mesh.h"
#include <Preferences.h>
#include <mbedtls/md.h>

namespace Settings {

static Preferences _p;

void init() {
    _p.begin("aether", false);

    if (!_p.isKey("name")) {
        String def = "Node_" + LoraMesh::nodeId();
        _p.putString("name", def);
    }
    if (!_p.isKey("pass"))  _p.putString("pass",  DEFAULT_AP_PASS);
    if (!_p.isKey("group")) _p.putString("group", DEFAULT_GROUP);
    if (!_p.isKey("mpass")) _p.putString("mpass", DEFAULT_MESH_PASS);
    if (!_p.isKey("rssid")) _p.putString("rssid", "");
    if (!_p.isKey("rpass")) _p.putString("rpass", "");

    Serial.printf("[Settings] name:\"%s\"  group:\"%s\"\n",
                  nodeName().c_str(), group().c_str());
}

String nodeName()   { return _p.getString("name",  "Node"); }
String apPass()     { return _p.getString("pass",  DEFAULT_AP_PASS); }
String group()      { return _p.getString("group", DEFAULT_GROUP); }
String meshPass()   { return _p.getString("mpass", DEFAULT_MESH_PASS); }
String routerSSID() { return _p.getString("rssid", ""); }
String routerPass() { return _p.getString("rpass", ""); }

void meshKey(uint8_t out[16]) {
    String p = meshPass();
    uint8_t hash[32];
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, info, 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const uint8_t*)p.c_str(), p.length());
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);
    memcpy(out, hash, 16);
}

bool save(const char* name, const char* pass, const char* grp,
          const char* rSSID, const char* rPass, const char* mpass) {
    if (name  && *name)              _p.putString("name",  name);
    if (pass  && strlen(pass) >= 8)  _p.putString("pass",  pass);
    if (grp   && *grp)               _p.putString("group", grp);
    if (mpass && *mpass)             _p.putString("mpass", mpass);
    _p.putString("rssid", rSSID ? rSSID : "");
    _p.putString("rpass", rPass ? rPass : "");
    return true;
}

} // namespace Settings
