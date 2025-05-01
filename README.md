# ESP32 WiFi Security Panel - Complete Guide

## 📌 Table of Contents
- [Introduction](#-introduction)
- [Prerequisites](#-prerequisites)
- [Installation](#-installation)
- [Flashing Instructions](#-flashing-instructions)
- [Usage Guide](#-usage-guide)
- [Features](#-features)
- [Legal Disclaimer](#⚠️-legal-disclaimer)

## 🌟 Introduction
This project turns an ESP32 into a WiFi security testing tool with capabilities for network scanning, deauthentication attacks, beacon flooding, and evil twin attacks.

## 📋 Prerequisites
- ESP8266 board (NodeMCU/Wemos D1 Mini) or
- NodeMCU ESP32
- Micro USB cable
- Computer with Arduino IDE
- Basic knowledge of WiFi networks

## 💻 Installation
""
### Software Setup
1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - File > Preferences > Additional Boards Manager URLs
   - Add: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
3. Install required libraries:
   - ESP8266WiFi
   - DNSServer
   - ESPAsyncTCP
   - ESPAsyncWebServer

## 🔥 Flashing Instructions

1. **Connect your ESP32** via USB
2. **Select board**:
   - Tools > Board > ESP32 Boards > Your Board (e.g., "NodeMCU 1.0")
3. **Configure settings**:
   - Flash Size: "4M (1M SPIFFS)"
   - CPU Frequency: "80 MHz"
   - Upload Speed: "115200"
4. **Upload the code**:
   - Copy the complete sketch
   - Click Upload (→ button)
5. **Wait for completion** until you see "Leaving... Hard resetting..."

## 🛠 Usage Guide

### Initial Access
1. After flashing, the ESP will create a WiFi network:
   - SSID: `SecurityScanPanel`
   - Password: `S3cur3P@ssw0rd!`
2. Connect to this network
3. Open browser to `http://192.168.4.1`
4. Login with:
   - Username: `admin`
   - Password: `S3cur3P@ssw0rd!`

### Interface Overview
- **Network Scanner**: Lists nearby WiFi networks
- **Deauth Attack**: Disconnects devices from selected network
- **Beacon Flood**: Creates fake networks
- **Evil Twin**: Creates clone of selected network to capture credentials

### Changing Defaults
Edit these lines in code before flashing:
```cpp
const char* ap_ssid = "SecurityScanPanel";
const char* ap_password = "S3cur3P@ssw0rd!";
```

## 🚀 Features

### Network Scanner
- Scans and displays all nearby WiFi networks
- Shows BSSID, SSID, and channel information

### Deauthentication Attack
- Targets specific WiFi networks
- Disconnects all devices from selected network
- Adjustable attack intensity

### Beacon Flood
- Creates hundreds of fake WiFi networks
- Customizable SSID patterns
- Channel hopping capability

### Evil Twin
- Creates clone of target network
- Captures login credentials
- DNS redirection for phishing

## ⚠️ Legal Disclaimer

**WARNING:** This tool is for **educational purposes only** and **security research** on **your own networks**. Unauthorized use on networks you don't own may violate:
- Computer Fraud and Abuse Act (CFAA)
- Wiretap Act
- Various international laws

The developer assumes **no responsibility** for any misuse of this software. Use at your own risk and only with explicit permission from network owners.

---
