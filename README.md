# BeaconDeauth Flashing Guide for ESP8266

Follow these steps to successfully flash **BeaconDeauth** onto your **ESP8266** device.

## Step 1: Install the Required Libraries

To ensure smooth flashing and operation, you need to install the following libraries in the **Arduino IDE**:

- **ESP8266WiFi**
- **ESPAsyncTCP**
- **ESPAsyncWebServer**
- **DNSServer**

### In the Arduino IDE:

1. Go to **Sketch** > **Include Library** > **Manage Libraries**.
2. Search for the libraries above and install them one by one.

---

## Step 2: Configure Arduino IDE for ESP8266

1. Open the **Arduino IDE**.
2. Go to **File** > **Preferences**.
3. In the **Additional Boards Manager URL** field, add the following URL: ```http://arduino.esp8266.com/stable/package_esp8266com_index.json```

4. Click **OK** to save the preferences.

---

## Step 3: Select the ESP8266 Board

1. Go to **Tools** > **Board** > **Boards Manager**.
2. Search for **ESP8266** and click **Install**.
3. After installation, select the appropriate **ESP8266** board from the **Tools** menu (e.g., **NodeMCU 1.0 (ESP-12E Module)**).

---

## Step 4: Connect Your ESP8266

1. Connect your **ESP8266** to your computer using a USB cable.
2. Select the correct **port** under **Tools** > **Port**.

---

## Step 5: Open the BeaconDeauth Project in Arduino IDE

1. Open the **BeaconDeauth.ino** file from the repository you downloaded.
2. You should now see the complete code in the **Arduino IDE**.

---

## Step 6: Flash the Code to Your ESP8266

1. Click on the **Upload** button (right arrow icon) in the **Arduino IDE**.
2. Wait for the flashing process to complete. You should see a **Done uploading** message once the process is successful.

---

## Step 7: Connect to the ESP8266 Wi-Fi Network

Once flashing is done, your **ESP8266** will automatically create a Wi-Fi network called **WiFi_Attack_Panel**. Here’s what you need to do next:

1. Open the Wi-Fi settings on your phone or computer.
2. Connect to the **WiFi_Attack_Panel** network (no password required).

---

## Step 8: Access the Web Interface

1. After connecting to the **ESP8266 Wi-Fi network**, open a browser and go to **192.168.4.1**.
2. This will load the **BeaconDeauth Web Interface** where you can:
- Scan for nearby WiFi networks.
- Start a **Beacon Flooding** attack.
- Start a **Deauthentication** attack.
- Stop all attacks.

---

## Troubleshooting Tips:

- **Not able to upload code?** Ensure that you have selected the correct **ESP8266** board and port.
- **Can't connect to the ESP8266 Wi-Fi?** Restart your **ESP8266** and try again.
- **The web page is not loading?** Make sure your device is connected to the **WiFi_Attack_Panel** network.

---

## Conclusion

Once you’ve followed these steps, you’ll have **BeaconDeauth** up and running on your **ESP8266** device. Now you can start testing WiFi networks and perform **Beacon Flooding** or **Deauthentication Attacks** directly from the web interface.
