#include "settings.h"
#include "config.h"
#include "lora_mesh.h"
#include <Preferences.h>

namespace Settings {

static Preferences _p;

void init() {
    _p.begin("aether", false);

    // Set defaults on first boot
    if (!_p.isKey("name")) {
        String def = "Node_" + LoraMesh::nodeId();
        _p.putString("name", def);
    }
    if (!_p.isKey("pass"))  _p.putString("pass",  DEFAULT_AP_PASS);
    if (!_p.isKey("group")) _p.putString("group", DEFAULT_GROUP);
    if (!_p.isKey("rssid")) _p.putString("rssid", "");
    if (!_p.isKey("rpass")) _p.putString("rpass", "");

    Serial.printf("[Settings] name:\"%s\"  group:\"%s\"\n",
                  nodeName().c_str(), group().c_str());
}

String nodeName()   { return _p.getString("name",  "Node"); }
String apPass()     { return _p.getString("pass",  DEFAULT_AP_PASS); }
String group()      { return _p.getString("group", DEFAULT_GROUP); }
String routerSSID() { return _p.getString("rssid", ""); }
String routerPass() { return _p.getString("rpass", ""); }

bool save(const char* name, const char* pass, const char* grp,
          const char* rSSID, const char* rPass) {
    if (name && *name)  _p.putString("name",  name);
    if (pass && strlen(pass) >= 8) _p.putString("pass", pass);
    if (grp  && *grp)   _p.putString("group", grp);
    _p.putString("rssid", rSSID ? rSSID : "");
    _p.putString("rpass", rPass ? rPass : "");
    return true;
}

} // namespace Settings
