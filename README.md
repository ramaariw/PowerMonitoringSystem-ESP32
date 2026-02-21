# ⚡ Power Monitoring System (IoT)

[![Framework](https://img.shields.io/badge/Framework-Arduino_IDE-00979D?style=flat&logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32-E67E22?style=flat&logo=espressif&logoColor=white)](https://www.espressif.com/)
[![Protocol](https://img.shields.io/badge/Protocol-MQTT-3C5280?style=flat&logo=mqtt&logoColor=white)](https://mqtt.org/)

A complete **IoT Power Monitoring & Control System**. This project measures AC/DC parameters and enables remote relay control via MQTT dashboards (Node-RED/Mobile App).

---

## 🚀 Features
- **Dual Monitoring:** Real-time AC (Voltage/Current) via PZEM-004T & DC Voltage monitoring.
- **Remote Control:** 2-channel relay control via MQTT subscription.
- **Smart Connectivity:** Auto-reconnect system for WiFi and MQTT broker.
- **Local Display:** LCD 16x2 with a mode selector button to switch between AC/DC data.
- **Dashboard Ready:** Integrated seamlessly with Node-RED for visualization.

---

## 🛠️ Hardware Components

| Component | Function |
| :--- | :--- |
| **ESP32 DevKit v1** | Main MCU & Gateway IoT |
| **PZEM-004T v3.0** | AC Voltage & Current Sensor |
| **DC Voltage Sensor** | Up to 25V measurement |
| **Relay 2-Channel** | Remote switch for AC/DC loads |
| **LCD 16x2 (I2C)** | Visualizing data without a dashboard |
| **LM2596 Buck Converter** | Voltage regulator for stable 5V input |
| **5V Mini Cooling Fan** | Thermal management for the enclosure |

> **Note:** The system uses a DC Barrel Jack for flexible power input and Terminal Blocks for secure high-voltage wiring.

---

## 🔌 System Architecture
1. **Sensing:** ESP32 reads data from PZEM-004T (AC) and Analog Pins (DC).
2. **Processing:** Data is formatted into JSON/Strings.
3. **Communication:** ESP32 publishes data to the MQTT Broker via WiFi.
4. **Action:** Node-RED subscribes to telemetry and publishes commands to control the relays.

---

## 📸 Demo & Screenshots
![Dashboard Demo](assets/nodered-dashboard.png)

---

## ⚙️ Installation & Setup
1. Clone this repository.
2. Install libraries: `PubSubClient`, `PZEM004Tv30`, `LiquidCrystal_I2C`.
3. Input your WiFi and MQTT credentials in the code.
4. Upload to ESP32.

---

## Pinout Connection

| Component | PIN ESP32 | Description
| :--- | :--- | :--- |
| **PZEM-004T v3.0** | RX (16), TX (17) | Serial Communication |
| **DC Voltage Sensor** | GPIO 34 (Analog) | Voltage Divider Input |
| **Relay CH1** | GPIO 25 | AC Load Control |
| **Relay CH2** | GPIO 24 | AC Load Control |
| **LCD 16x2 (I2C)** | SDA (21), SCL (22) | Data Display |
| **Push Button** | GPIO 12 | Input Pullup |

---

Markdown
### 🔧 Wiring Diagram
![Wiring Diagram](assets/wiring-diagram.png)


