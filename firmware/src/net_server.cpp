#include "net_server.h"
#include "config.h"
#include "settings.h"
#include "storage.h"
#include "lora_mesh.h"
#include "mesh_time.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <map>

// ── Embedded Web UI ───────────────────────────────────────────────────────────
static const char HTML[] PROGMEM = R"html(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>AetherMesh</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0d0d0d;color:#e0e0e0;display:flex;flex-direction:column;height:100dvh;height:100vh;overflow:hidden}
#h{background:#111;border-bottom:1px solid #1e1e1e;padding:10px 14px;display:flex;justify-content:space-between;align-items:center;flex-shrink:0}
#nn{color:#0ff;font-size:16px;font-weight:700}
#hm{color:#333;font-size:11px;margin-top:3px;display:flex;align-items:center;gap:6px}
#wsdot{width:6px;height:6px;border-radius:50%;background:#333;flex-shrink:0}
#wsdot.on{background:#0f0}
#hbtn{background:none;border:none;color:#444;font-size:24px;cursor:pointer;padding:6px;border-radius:6px;-webkit-tap-highlight-color:transparent;min-width:44px;min-height:44px;display:flex;align-items:center;justify-content:center}
#hbtn:active{background:#1e1e1e}
#log{flex:1;overflow-y:auto;padding:12px 10px;display:flex;flex-direction:column;gap:8px;-webkit-overflow-scrolling:touch}
.m{max-width:82%;padding:9px 13px;font-size:15px;line-height:1.45;word-break:break-word}
.m.r{align-self:flex-start;background:#1c1c1c;border-radius:4px 16px 16px 4px}
.m.s{align-self:flex-end;background:#0b2e0b;border-radius:16px 4px 4px 16px;text-align:right}
.mn{font-size:11px;font-weight:700;margin-bottom:4px}
.m.r .mn{color:#0ff}
.m.s .mn{color:#0f0}
.mb{color:#e0e0e0}
.mm{font-size:10px;color:#2a2a2a;margin-top:5px}
.ack{color:#2a2a2a;margin-left:4px}
.ack.ok{color:#0f0}
.ack.fail{color:#f44}
.dm-tag{font-size:10px;color:#444;margin-left:3px}
#dm-bar{display:none;background:#0a1f0a;border-top:1px solid #0f2a0f;padding:8px 14px;font-size:12px;color:#0c0;flex-shrink:0;align-items:center;gap:8px}
#dm-bar.v{display:flex}
#dm-cancel{margin-left:auto;background:none;border:none;color:#3a3a3a;cursor:pointer;font-size:20px;min-width:36px;min-height:36px;display:flex;align-items:center;justify-content:center;-webkit-tap-highlight-color:transparent}
#dm-cancel:active{color:#f44}
#inp{background:#111;border-top:1px solid #1e1e1e;padding:10px 12px;padding-bottom:max(10px,env(safe-area-inset-bottom));display:flex;gap:10px;align-items:center;flex-shrink:0}
#mi{flex:1;background:#1c1c1c;color:#e0e0e0;border:1.5px solid #252525;border-radius:22px;padding:11px 18px;font-size:16px;outline:none;-webkit-appearance:none;min-height:44px}
#mi:focus{border-color:#0f0}
#mi.dm-mode{border-color:#0a3a0a}
#txb{background:#0f0;color:#000;border:none;border-radius:50%;width:44px;height:44px;font-size:20px;font-weight:900;cursor:pointer;flex-shrink:0;display:flex;align-items:center;justify-content:center;-webkit-tap-highlight-color:transparent}
#txb:active{opacity:.65;transform:scale(.95)}
#ob{display:none;position:fixed;inset:0;background:rgba(0,0,0,.85);z-index:9;align-items:flex-end;justify-content:center}
#ob.v{display:flex}
#op{background:#111;border-radius:20px 20px 0 0;padding:20px 16px;padding-bottom:max(20px,env(safe-area-inset-bottom));width:100%;max-width:520px;max-height:92vh;overflow-y:auto}
.dh{width:36px;height:4px;background:#2a2a2a;border-radius:2px;margin:0 auto 18px}
#op h3{color:#0f0;font-size:14px;font-weight:700;letter-spacing:.5px;margin-bottom:18px;display:flex;justify-content:space-between;align-items:center}
#oc{background:none;border:none;color:#444;cursor:pointer;font-size:22px;min-width:44px;min-height:44px;display:flex;align-items:center;justify-content:center;border-radius:50%;-webkit-tap-highlight-color:transparent}
#oc:active{background:#1e1e1e}
.sr{margin-bottom:16px}
.sr label{display:block;color:#555;font-size:11px;font-weight:700;text-transform:uppercase;letter-spacing:.7px;margin-bottom:6px}
.sr small{color:#2a2a2a;font-size:11px;text-transform:none;letter-spacing:0;display:block;margin-top:4px}
.sr input{width:100%;background:#1a1a1a;color:#e0e0e0;border:1.5px solid #252525;border-radius:10px;padding:13px 14px;font-size:16px;-webkit-appearance:none;outline:none}
.sr input:focus{border-color:#0f0}
.sd{border-top:1px solid #1a1a1a;margin:18px 0 14px;color:#2a2a2a;font-size:11px;padding-top:14px;line-height:1.6}
.s2{display:flex;gap:10px}
.s2>.sr{flex:1;min-width:0}
#sv{width:100%;background:#0f0;color:#000;border:none;border-radius:12px;padding:15px;font-size:16px;font-weight:800;cursor:pointer;margin-top:4px;-webkit-tap-highlight-color:transparent}
#sv:active{opacity:.7}
.sa{display:flex;gap:10px;margin-top:14px}
.sa button{flex:1;background:none;border:1.5px solid #1e1e1e;border-radius:10px;color:#444;padding:12px;font-size:13px;cursor:pointer;-webkit-tap-highlight-color:transparent}
.sa button:active{background:#1a1a1a}
.dng{border-color:#3a0000!important;color:#663333!important}
.dng:active{background:#200!important;color:#f44!important}
#nodes-wrap{margin-top:4px}
.node-row{display:flex;justify-content:space-between;align-items:center;padding:10px 0;border-bottom:1px solid #1a1a1a;font-size:13px;cursor:pointer;-webkit-tap-highlight-color:transparent}
.node-row:last-child{border-bottom:none}
.node-row:active{opacity:.6}
.dm-hint{color:#0f0;font-size:10px;font-weight:700;margin-left:6px}
</style>
</head>
<body>
<div id="h">
 <div>
  <div id="nn">AetherMesh</div>
  <div id="hm">
   <div id="wsdot" title="WebSocket"></div>
   ch:<b id="hch" style="color:#1a6b1a">&#8212;</b> &nbsp;id:<span id="hid">&#8212;</span> &nbsp;<span id="st">connecting...</span>
  </div>
 </div>
 <button id="hbtn" onclick="openS()" title="Settings">&#9881;</button>
</div>
<div id="log"></div>
<div id="dm-bar">
 <span>&#8594;&nbsp;<b id="dm-name" style="color:#0f0"></b>&nbsp;<span style="color:#333;font-size:11px">private message</span></span>
 <button id="dm-cancel" onclick="clearDm()" title="Back to all">&#10005;</button>
</div>
<div id="inp">
 <input id="mi" placeholder="Message..." autocomplete="off" maxlength="180"
  onkeydown="if(event.key==='Enter'&&!event.shiftKey){event.preventDefault();tx()}">
 <button id="txb" onclick="tx()">&#8593;</button>
</div>
<div id="ob" onclick="if(event.target===this)closeS()">
<div id="op">
 <div class="dh"></div>
 <h3>SETTINGS <button id="oc" onclick="closeS()">&#10005;</button></h3>
 <div class="sr">
  <label>Your Name</label>
  <input id="sn" maxlength="30" placeholder="Node_XX" autocomplete="off">
  <small>Shown to others in chat</small>
 </div>
 <div class="sr">
  <label>Channel</label>
  <input id="sg" maxlength="30" placeholder="default" autocomplete="off">
  <small>Both nodes must use the same channel. No reboot needed.</small>
 </div>
 <div class="sr">
  <label>WiFi PIN &nbsp;<span style="color:#333;font-size:10px;font-weight:400;text-transform:none">(8 digits — also your WiFi password)</span></label>
  <input id="sp" type="tel" inputmode="numeric" pattern="[0-9]{8}" maxlength="8" placeholder="Leave blank to keep current" autocomplete="off">
  <small>Default first-time PIN: 12345678</small>
 </div>
 <div class="sr">
  <label>Mesh Key &nbsp;<span style="color:#333;font-size:10px;font-weight:400;text-transform:none">(shared passphrase — all nodes must match)</span></label>
  <input id="smk" type="password" maxlength="60" placeholder="Leave blank to keep current" autocomplete="new-password">
  <small>Default: AetherMesh &mdash; change for private network isolation. Update all nodes simultaneously.</small>
 </div>
 <div class="sd">&#x25CB; Router WiFi (optional)<br>Gives this node internet via your home router. Phone keeps mobile data.</div>
 <div class="s2">
  <div class="sr"><label>Router SSID</label><input id="srs" maxlength="60" placeholder="WiFi name" autocomplete="off"></div>
  <div class="sr"><label>Router Pass</label><input id="srp" type="password" maxlength="60" placeholder="&#9679;&#9679;&#9679;&#9679;&#9679;&#9679;" autocomplete="off"></div>
 </div>
 <button id="sv" onclick="saveS()">SAVE</button>
 <div class="sa">
  <button onclick="exp()">&#8659;&nbsp;Export Chat</button>
  <button class="dng" onclick="clr()">&#10006;&nbsp;Clear Messages</button>
 </div>
 <div class="sd">&#x25CF; Nodes on Mesh &nbsp;<span style="color:#333">(tap to DM)</span></div>
 <div id="nodes-wrap"><span style="color:#2a2a2a;font-size:13px">No nodes seen yet. Beacon every 30s.</span></div>
</div>
</div>
<script>
var last=0,myId='',myGrp='default',msgMap={};
var dmTarget='all',dmName='';
var log=document.getElementById('log');
function esc(s){return(''+s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;')}
function fts(t){
 if(t>1577836800){
  var d=new Date(t*1000);
  return d.toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'});
 }
 return String(Math.floor(t/3600)).padStart(2,'0')+':'+String(Math.floor((t%3600)/60)).padStart(2,'0');
}
function addMsg(m){
 if(msgMap['m'+m.id])return;
 var mine=(m.from===myId);
 var d=document.createElement('div');
 d.className='m '+(mine?'s':'r');
 var nm=esc(m.fromName||m.from);
 var rs=m.rssi?' \xb7 '+m.rssi+'dBm \xb7 '+m.hops+'hop':'';
 var isDM=(m.to&&m.to!=='all');
 var dmTag=isDM?'<span class="dm-tag">\u2192 '+(m.to===myId?'you':esc(m.to))+'</span>':'';
 var ackHtml=mine?'<span class="ack" id="ack-'+m.id+'">\u2713</span>':'';
 d.innerHTML='<div class="mn">'+(mine?'You':nm)+'</div>'
  +'<div class="mb">'+esc(m.text)+'</div>'
  +'<div class="mm">'+fts(m.ts)+rs+dmTag+ackHtml+'</div>';
 log.appendChild(d);
 msgMap['m'+m.id]=true;
 if(m.ts>last)last=m.ts;
 log.scrollTop=log.scrollHeight;
}
function markAck(id){
 var el=document.getElementById('ack-'+id);
 if(el){el.textContent='\u2713\u2713';el.className='ack ok';}
}
function markFail(id){
 var el=document.getElementById('ack-'+id);
 if(el){el.textContent='\u2717';el.className='ack fail';}
}
function updateNodes(nodes){
 var el=document.getElementById('nodes-wrap');
 if(!nodes||!nodes.length){
  el.innerHTML='<span style="color:#2a2a2a;font-size:13px">No nodes seen yet. Beacon every 30s.</span>';
  return;
 }
 el.innerHTML=nodes.map(function(n){
  return '<div class="node-row" onclick="setDm(\''+n.id+'\',\''+esc(n.name||n.id)+'\')">'
   +'<span><b style="color:#0ff">'+esc(n.name||n.id)+'</b>'
   +' <span style="color:#333">['+esc(n.id)+']</span>'
   +'<span class="dm-hint">DM \u2192</span></span>'
   +'<span style="color:#444">'+n.rssi+'dBm</span></div>';
 }).join('');
}
function setDm(id,name){
 dmTarget=id; dmName=name;
 document.getElementById('dm-name').textContent=name;
 document.getElementById('dm-bar').classList.add('v');
 document.getElementById('mi').classList.add('dm-mode');
 document.getElementById('mi').placeholder='DM \u2192 '+name+'...';
 closeS();
}
function clearDm(){
 dmTarget='all'; dmName='';
 document.getElementById('dm-bar').classList.remove('v');
 document.getElementById('mi').classList.remove('dm-mode');
 document.getElementById('mi').placeholder='Message...';
}

// WebSocket
var ws=null,wsOk=false;
function connectWS(){
 try{
  ws=new WebSocket('ws://'+location.host+'/ws');
  ws.onopen=function(){wsOk=true;document.getElementById('wsdot').className='on'};
  ws.onclose=function(){wsOk=false;document.getElementById('wsdot').className='';setTimeout(connectWS,3000)};
  ws.onerror=function(){ws.close()};
  ws.onmessage=function(e){
   try{
    var d=JSON.parse(e.data);
    if(d.type==='msg')addMsg(d);
    else if(d.type==='ack')markAck(d.id);
    else if(d.type==='fail')markFail(d.id);
    else if(d.type==='nodes')updateNodes(d.nodes);
   }catch(err){}
  };
 }catch(e){setTimeout(connectWS,3000);}
}
connectWS();

async function load(){
 try{var d=await(await fetch('/api/messages?after='+last)).json();
  if(d&&d.length)d.forEach(addMsg);}catch(e){}
}
async function tx(){
 var v=document.getElementById('mi').value.trim();
 if(!v)return;
 document.getElementById('mi').value='';
 try{await fetch('/api/send',{method:'POST',headers:{'Content-Type':'application/json'},
  body:JSON.stringify({to:dmTarget,msg:v})});}catch(e){}
}
async function clr(){
 if(!confirm('Clear all messages?'))return;
 try{await fetch('/api/clear',{method:'POST'});}catch(e){}
 log.innerHTML='';last=0;msgMap={};closeS();
}
function exp(){
 fetch('/api/export').then(function(r){return r.text();}).then(function(t){
  var a=document.createElement('a');
  a.href='data:text/plain;charset=utf-8,'+encodeURIComponent(t);
  a.download='aethermesh_'+myId+'_'+Date.now()+'.txt';
  a.click();
 });
}
async function stat(){
 try{var d=await(await fetch('/api/status')).json();
  myId=d.node;myGrp=d.group||'default';
  document.getElementById('nn').textContent=d.name||(d.node);
  document.getElementById('hid').textContent=d.node;
  document.getElementById('hch').textContent=myGrp;
  var ri=d.router_ip?'\u25cf ':'';
  document.getElementById('st').textContent=ri+'heap:'+Math.round(d.heap/1024)+'K msgs:'+d.msgs;
 }catch(e){}
}
function openS(){
 Promise.all([
  fetch('/api/settings').then(function(r){return r.json();}),
  fetch('/api/nodes').then(function(r){return r.json();})
 ]).then(function(res){
  var d=res[0];
  document.getElementById('sn').value=d.name||'';
  document.getElementById('sp').value='';
  document.getElementById('sg').value=d.group||'';
  document.getElementById('smk').value='';
  document.getElementById('srs').value=d.router_ssid||'';
  document.getElementById('srp').value='';
  updateNodes(res[1]);
  document.getElementById('ob').classList.add('v');
 });
}
function closeS(){document.getElementById('ob').classList.remove('v');}
async function saveS(){
 var pin=document.getElementById('sp').value;
 if(pin&&(pin.length!==8||!/^[0-9]{8}$/.test(pin))){
  alert('WiFi PIN must be exactly 8 digits');return;}
 var body={name:document.getElementById('sn').value,pass:pin,
  group:document.getElementById('sg').value,
  mesh_pass:document.getElementById('smk').value,
  router_ssid:document.getElementById('srs').value,
  router_pass:document.getElementById('srp').value};
 try{var d=await(await fetch('/api/settings',{method:'POST',
  headers:{'Content-Type':'application/json'},body:JSON.stringify(body)})).json();
  if(d.ok){
   if(d.reboot){alert('Saved! Rebooting...');}
   else{
    alert('Saved!');
    document.getElementById('nn').textContent=body.name||myId;
    document.getElementById('hch').textContent=body.group||'default';
   }
   closeS();
  }else alert('Error: '+(d.error||'?'));}
 catch(e){alert('Save failed.');}
}

fetch('/api/time',{method:'POST',headers:{'Content-Type':'application/json'},
 body:JSON.stringify({ts:Math.floor(Date.now()/1000)})}).catch(function(){});

load();
stat();
setInterval(function(){if(!wsOk)load();},3000);
setInterval(stat,20000);
</script>
</body>
</html>
)html";

// ── TCP & WebSocket state ─────────────────────────────────────────────────────
static AsyncServer*                   _tcpServer  = nullptr;
static std::vector<AsyncClient*>      _tcpClients;
static std::map<AsyncClient*, String> _rxBuf;
static SemaphoreHandle_t              _tcpMutex   = nullptr;
static AsyncWebSocket                 _ws("/ws");
static AsyncWebServer                 _http(HTTP_PORT);

struct BodyCtx { char buf[512]; size_t len = 0; };

static uint32_t _sendCount = 0; // counter for app-level message ids

// ── helpers ───────────────────────────────────────────────────────────────────
static String apSSID() {
    char s[28];
    snprintf(s, sizeof(s), "%s_%s", WIFI_SSID_PFX, LoraMesh::nodeId().c_str());
    return String(s);
}

static String msgToJson(const Msg& m) {
    JsonDocument doc;
    doc["id"]       = m.id;
    doc["from"]     = m.from;
    doc["fromName"] = m.fromName.isEmpty() ? m.from : m.fromName;
    doc["to"]       = m.to;
    doc["text"]     = m.text;
    doc["group"]    = m.group;
    doc["rssi"]     = m.rssi;
    doc["snr"]      = serialized(String(m.snr, 1));
    doc["hops"]     = m.hops;
    doc["ts"]       = m.ts;
    String out; serializeJson(doc, out);
    return out;
}

static void tcpSend(AsyncClient* c, const String& s) {
    if (c && c->connected() && c->canSend()) c->write(s.c_str(), s.length());
}

static void handleTcpLine(AsyncClient* c, const String& line) {
    JsonDocument doc;
    if (deserializeJson(doc, line) != DeserializationError::Ok) {
        tcpSend(c, "{\"error\":\"bad json\"}\n"); return;
    }
    const char* cmd = doc["cmd"] | "";

    if (strcmp(cmd, "send") == 0) {
        const char* to  = doc["to"]  | "all";
        const char* msg = doc["msg"] | "";
        if (!*msg) { tcpSend(c, "{\"error\":\"empty msg\"}\n"); return; }
        char clientId[16];
        snprintf(clientId, sizeof(clientId), "t%lu", ++_sendCount);
        LoraMesh::send(to, msg, clientId);
        Msg m; m.id=String(clientId); m.from=LoraMesh::nodeId(); m.fromName=Settings::nodeName();
        m.to=to; m.text=msg; m.group=Settings::group();
        m.rssi=0; m.snr=0; m.hops=0; m.ts=MeshTime::now();
        Storage::save(m); NetServer::notify(m);
        String out="{\"ok\":true,\"id\":\""+String(clientId)+"\"}\n";
        tcpSend(c, out);

    } else if (strcmp(cmd, "messages") == 0) {
        auto msgs = Storage::get();
        JsonDocument resp; JsonArray arr = resp.to<JsonArray>();
        for (const auto& m : msgs) {
            JsonObject o = arr.add<JsonObject>();
            o["id"]=m.id; o["from"]=m.from;
            o["fromName"]=m.fromName.isEmpty()?m.from:m.fromName;
            o["to"]=m.to; o["text"]=m.text; o["group"]=m.group;
            o["rssi"]=m.rssi; o["snr"]=serialized(String(m.snr,1));
            o["hops"]=m.hops; o["ts"]=m.ts;
        }
        String out; serializeJson(resp, out); out+="\n"; tcpSend(c, out);

    } else if (strcmp(cmd, "status") == 0) {
        JsonDocument resp;
        resp["node"]=LoraMesh::nodeId(); resp["name"]=Settings::nodeName();
        resp["group"]=Settings::group(); resp["heap"]=ESP.getFreeHeap();
        resp["uptime"]=millis()/1000; resp["msgs"]=(int)Storage::get().size();
        resp["router_ip"]=(WiFi.status()==WL_CONNECTED)?WiFi.localIP().toString():"";
        resp["time_synced"]=MeshTime::isSynced();
        String out; serializeJson(resp, out); out+="\n"; tcpSend(c, out);

    } else if (strcmp(cmd, "clear") == 0) {
        Storage::clear(); tcpSend(c, "{\"ok\":true}\n");
    } else {
        tcpSend(c, "{\"error\":\"unknown cmd\"}\n");
    }
}

static void captureBody(AsyncWebServerRequest* req,
                        uint8_t* data, size_t len, size_t index, size_t) {
    if (index == 0) req->_tempObject = new BodyCtx();
    BodyCtx* ctx = (BodyCtx*)req->_tempObject;
    if (ctx && ctx->len + len < sizeof(ctx->buf) - 1) {
        memcpy(ctx->buf + ctx->len, data, len);
        ctx->len += len; ctx->buf[ctx->len] = '\0';
    }
}

// ── HTTP routes ───────────────────────────────────────────────────────────────
static void setupRoutes() {
    _http.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", HTML);
    });

    _http.on("/api/messages", HTTP_GET, [](AsyncWebServerRequest* req) {
        uint32_t after = 0;
        if (req->hasParam("after")) after = (uint32_t)req->getParam("after")->value().toInt();
        auto msgs = Storage::get();
        JsonDocument doc; JsonArray arr = doc.to<JsonArray>();
        for (const auto& m : msgs) {
            if (m.ts <= after) continue;
            JsonObject o = arr.add<JsonObject>();
            o["id"]=m.id; o["from"]=m.from;
            o["fromName"]=m.fromName.isEmpty()?m.from:m.fromName;
            o["to"]=m.to; o["text"]=m.text; o["group"]=m.group;
            o["rssi"]=m.rssi; o["snr"]=serialized(String(m.snr,1)); o["hops"]=m.hops; o["ts"]=m.ts;
        }
        String out; serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    _http.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["node"]          = LoraMesh::nodeId();
        doc["name"]          = Settings::nodeName();
        doc["group"]         = Settings::group();
        doc["ip"]            = WiFi.softAPIP().toString();
        doc["router_ip"]     = (WiFi.status()==WL_CONNECTED)?WiFi.localIP().toString():"";
        doc["heap"]          = ESP.getFreeHeap();
        doc["uptime"]        = millis()/1000;
        doc["msgs"]          = (int)Storage::get().size();
        doc["store_used_kb"] = Storage::usedBytes()/1024;
        doc["store_total_kb"]= Storage::totalBytes()/1024;
        doc["ver"]           = FIRMWARE_VER;
        doc["time_synced"]   = MeshTime::isSynced();
        doc["time_now"]      = MeshTime::now();
        String out; serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    _http.on("/api/time", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            BodyCtx* ctx = (BodyCtx*)req->_tempObject;
            if (!ctx) { req->send(400); return; }
            JsonDocument doc;
            if (deserializeJson(doc, ctx->buf, ctx->len) != DeserializationError::Ok) {
                req->send(400); return;
            }
            uint32_t ts = doc["ts"] | 0;
            if (ts > 1577836800) {
                MeshTime::setFromBrowser(ts);
                req->send(200, "application/json", "{\"ok\":true}");
            } else {
                req->send(400, "application/json", "{\"error\":\"invalid ts\"}");
            }
        },
        nullptr, captureBody
    );

    _http.on("/api/send", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            BodyCtx* ctx = (BodyCtx*)req->_tempObject;
            if (!ctx) { req->send(400, "application/json", "{\"error\":\"no body\"}"); return; }
            JsonDocument doc;
            if (deserializeJson(doc, ctx->buf, ctx->len) != DeserializationError::Ok) {
                req->send(400, "application/json", "{\"error\":\"bad json\"}"); return;
            }
            const char* to  = doc["to"]  | "all";
            const char* msg = doc["msg"] | "";
            if (!*msg) { req->send(400, "application/json", "{\"error\":\"empty msg\"}"); return; }

            char clientId[16];
            snprintf(clientId, sizeof(clientId), "t%lu", ++_sendCount);

            LoraMesh::send(to, msg, clientId);

            Msg m; m.id=String(clientId); m.from=LoraMesh::nodeId();
            m.fromName=Settings::nodeName();
            m.to=to; m.text=msg; m.group=Settings::group();
            m.rssi=0; m.snr=0; m.hops=0; m.ts=MeshTime::now();
            Storage::save(m); NetServer::notify(m);
            req->send(200, "application/json", "{\"ok\":true}");
        },
        nullptr, captureBody
    );

    _http.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["name"]        = Settings::nodeName();
        doc["group"]       = Settings::group();
        doc["router_ssid"] = Settings::routerSSID();
        // mesh_pass is intentionally not returned (write-only for security)
        String out; serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    _http.on("/api/settings", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            BodyCtx* ctx = (BodyCtx*)req->_tempObject;
            if (!ctx) { req->send(400, "application/json", "{\"error\":\"no body\"}"); return; }
            JsonDocument doc;
            if (deserializeJson(doc, ctx->buf, ctx->len) != DeserializationError::Ok) {
                req->send(400, "application/json", "{\"error\":\"bad json\"}"); return;
            }
            const char* pin = doc["pass"] | "";
            if (pin && strlen(pin) > 0) {
                bool digits = true;
                for (size_t i = 0; i < strlen(pin); i++)
                    if (pin[i]<'0'||pin[i]>'9') { digits=false; break; }
                if (strlen(pin) != 8 || !digits) {
                    req->send(400, "application/json", "{\"error\":\"PIN must be 8 digits\"}"); return;
                }
            }
            bool wifiChanged = (pin && strlen(pin) == 8) ||
                               (String(doc["router_ssid"]|"") != Settings::routerSSID()) ||
                               (String(doc["router_pass"]|"").length() > 0);

            Settings::save(doc["name"]|"", pin, doc["group"]|"",
                           doc["router_ssid"]|"", doc["router_pass"]|"",
                           doc["mesh_pass"]|"");

            if (wifiChanged) {
                req->send(200, "application/json", "{\"ok\":true,\"reboot\":true}");
                xTaskCreate([](void*){ delay(1000); ESP.restart(); vTaskDelete(NULL); },
                            "reboot", 2048, NULL, 1, NULL);
            } else {
                req->send(200, "application/json", "{\"ok\":true,\"reboot\":false}");
            }
        },
        nullptr, captureBody
    );

    _http.on("/api/nodes", HTTP_GET, [](AsyncWebServerRequest* req) {
        auto nodes = LoraMesh::getNodes();
        JsonDocument doc; JsonArray arr = doc.to<JsonArray>();
        for (const auto& n : nodes) {
            JsonObject o = arr.add<JsonObject>();
            o["id"]=n.id; o["name"]=n.name; o["group"]=n.group;
            o["rssi"]=n.rssi; o["snr"]=serialized(String(n.snr,1));
            o["lastSeen"]=n.lastSeen;
        }
        String out; serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    _http.on("/api/export", HTTP_GET, [](AsyncWebServerRequest* req) {
        auto msgs = Storage::get();
        String out = "AetherMesh Chat Export\n";
        out += "Node: " + Settings::nodeName() + " [" + LoraMesh::nodeId() + "]\n";
        out += "Channel: " + Settings::group() + "\n";
        out += "Messages: " + String(msgs.size()) + "\n";
        out += "----------------------------------------\n";
        for (const auto& m : msgs) {
            uint32_t t = m.ts;
            char tbuf[24];
            if (t > 1577836800) {
                time_t tt = (time_t)t;
                struct tm* ti = gmtime(&tt);
                strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M", ti);
            } else {
                snprintf(tbuf, sizeof(tbuf), "%02u:%02u:%02u", t/3600, (t%3600)/60, t%60);
            }
            String nm = m.fromName.isEmpty() ? m.from : m.fromName;
            String toTag = (m.to != "all") ? " [DM→" + m.to + "]" : "";
            out += "[" + String(tbuf) + "] " + nm + toTag + ": " + m.text + "\n";
        }
        req->send(200, "text/plain", out);
    });

    _http.on("/api/clear", HTTP_POST, [](AsyncWebServerRequest* req) {
        Storage::clear(); req->send(200, "application/json", "{\"ok\":true}");
    });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    _http.onNotFound([](AsyncWebServerRequest* req) {
        if (req->method() == HTTP_OPTIONS) { req->send(204); return; }
        req->send(404, "application/json", "{\"error\":\"not found\"}");
    });
}

// ── Public ────────────────────────────────────────────────────────────────────
namespace NetServer {

void notify(const Msg& msg) {
    JsonDocument doc;
    doc["type"]     = "msg";
    doc["id"]       = msg.id;
    doc["from"]     = msg.from;
    doc["fromName"] = msg.fromName.isEmpty() ? msg.from : msg.fromName;
    doc["to"]       = msg.to;
    doc["text"]     = msg.text;
    doc["group"]    = msg.group;
    doc["rssi"]     = msg.rssi;
    doc["snr"]      = serialized(String(msg.snr, 1));
    doc["hops"]     = msg.hops;
    doc["ts"]       = msg.ts;
    String wsJson; serializeJson(doc, wsJson);
    _ws.textAll(wsJson);

    String tcpJson = msgToJson(msg) + "\n";
    if (_tcpMutex) xSemaphoreTake(_tcpMutex, portMAX_DELAY);
    for (auto c : _tcpClients) tcpSend(c, tcpJson);
    if (_tcpMutex) xSemaphoreGive(_tcpMutex);
}

void notifyAck(const String& msgId) {
    _ws.textAll("{\"type\":\"ack\",\"id\":\"" + msgId + "\"}");
}

void notifyFail(const String& msgId) {
    _ws.textAll("{\"type\":\"fail\",\"id\":\"" + msgId + "\"}");
}

void init() {
    _tcpMutex = xSemaphoreCreateMutex();

    String ssid  = apSSID();
    String apPw  = Settings::apPass();
    String rSSID = Settings::routerSSID();
    String rPass = Settings::routerPass();

    if (rSSID.length() > 0) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(ssid.c_str(), apPw.c_str(), WIFI_CHANNEL, 0, WIFI_MAX_CLIENTS);
        WiFi.begin(rSSID.c_str(), rPass.c_str());
        WiFi.onEvent([](WiFiEvent_t e, WiFiEventInfo_t info) {
            if (e == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
                Serial.printf("[WiFi] Router: %s\n", WiFi.localIP().toString().c_str());
                configTime(0, 0, "pool.ntp.org", "time.google.com");
                xTaskCreate([](void*) {
                    delay(3000);
                    MeshTime::setFromNTP();
                    vTaskDelete(NULL);
                }, "ntp", 3072, NULL, 1, NULL);
            } else if (e == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
                WiFi.reconnect();
            }
        });
        Serial.printf("[WiFi] AP: %-22s  AP IP: %s\n", ssid.c_str(), WiFi.softAPIP().toString().c_str());
        Serial.printf("[WiFi] Connecting to router: %s\n", rSSID.c_str());
    } else {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ssid.c_str(), apPw.c_str(), WIFI_CHANNEL, 0, WIFI_MAX_CLIENTS);
        Serial.printf("[WiFi] AP: %-22s  IP: %s\n", ssid.c_str(), WiFi.softAPIP().toString().c_str());
    }

    _ws.onEvent([](AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType type,
                   void*, uint8_t*, size_t) { (void)type; });
    _http.addHandler(&_ws);

    setupRoutes();
    _http.begin();
    Serial.printf("[HTTP] port %d\n", HTTP_PORT);

    _tcpServer = new AsyncServer(TCP_PORT);
    _tcpServer->onClient([](void*, AsyncClient* c) {
        xSemaphoreTake(_tcpMutex, portMAX_DELAY);
        if ((int)_tcpClients.size() >= WIFI_MAX_CLIENTS) {
            xSemaphoreGive(_tcpMutex); c->close(true); delete c; return;
        }
        _tcpClients.push_back(c); _rxBuf[c] = "";
        c->onDisconnect([](void*, AsyncClient* c) {
            xSemaphoreTake(_tcpMutex, portMAX_DELAY);
            _tcpClients.erase(std::remove(_tcpClients.begin(),_tcpClients.end(),c),_tcpClients.end());
            _rxBuf.erase(c);
            xSemaphoreGive(_tcpMutex);
            delete c;
        }, nullptr);
        c->onData([](void*, AsyncClient* c, void* data, size_t len) {
            xSemaphoreTake(_tcpMutex, portMAX_DELAY);
            _rxBuf[c] += String((char*)data, len);
            int nl;
            while ((nl = _rxBuf[c].indexOf('\n')) >= 0) {
                String line = _rxBuf[c].substring(0, nl);
                _rxBuf[c]   = _rxBuf[c].substring(nl + 1);
                line.trim();
                if (!line.isEmpty()) handleTcpLine(c, line);
            }
            xSemaphoreGive(_tcpMutex);
        }, nullptr);
        JsonDocument doc;
        doc["type"]="hello"; doc["node"]=LoraMesh::nodeId();
        doc["name"]=Settings::nodeName(); doc["group"]=Settings::group();
        doc["ver"]=FIRMWARE_VER;
        String w; serializeJson(doc,w); w+="\n"; tcpSend(c, w);
        xSemaphoreGive(_tcpMutex);
    }, nullptr);
    _tcpServer->begin();
    Serial.printf("[TCP]  port %d\n", TCP_PORT);
}

void loop() {
    static uint32_t lastClean = 0;
    if (millis() - lastClean > 10000) {
        _ws.cleanupClients();
        lastClean = millis();
    }
}

} // namespace NetServer
