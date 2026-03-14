# ⚡ Power Monitoring System (PMS) v1.1 — PlatformIO Version

A modular, high-performance **ESP32 firmware** for monitoring both **AC and DC power systems**.  
This version (**v1.1**) is optimized for reliability, featuring a **multi-page LCD interface**, **remote relay control via MQTT**, and a **dual-sync mechanism for Cloud and Local logging**.

---

# ✨ Key Features (v1.1)

- **PlatformIO Migration**  
  Optimized project structure with modular header files (`.h`).

- **Dual-Core Data Handling**  
  Concurrent sensor reading and UI management for zero lag.

- **Dual Sync System**
  - **Cloud (MQTT)** — Secure data publishing for remote dashboards (Node-RED / Grafana)
  - **Local Sync (Si Bapuk)** — High-speed HTTP POST to local server for redundancy

- **Advanced UI**  
  5-page LCD navigation:
  - DC Status
  - AC Status
  - Time
  - Relay Control
  - System Uptime

- **Interactive Controls**  
  Advanced debounced button logic supporting:
  - single click
  - double click
  - triple click
  - long press

- **WiFi Manager**  
  On-the-fly network configuration via captive portal  
  `PMS-V1.1-Setup`

---

# 🛠️ Hardware Specifications

| Component | Description |
|---|---|
| **Microcontroller** | ESP32 DevKit |
| **AC Sensor** | PZEM-004T v3.0 (Serial2 — Pins 16,17) |
| **DC Sensor** | Analog Voltage Divider (GPIO 34) |
| **Outputs** | 2-Channel Relay Module (GPIO 4 & 2 — Active LOW) |
| **Display** | 16x2 LCD with I2C Module (Address `0x27`) |
| **Control** | Physical Push Button (GPIO 32) |

---

# 🎮 Button Controls

Manage the entire system using **just one button**.

| Action | Function |
|------|------|
| **Single Click** | Cycle through LCD pages / Navigate menu |
| **Double Click** | Toggle Relay 1 (Terminal 1) |
| **Triple Click** | Toggle Relay 2 (Terminal 2) |
| **Long Press (3s)** | Enter / Exit settings menu or confirm selection |

---

# ⚙️ Configuration & Installation

## 1. Requirements

Install the following libraries in **PlatformIO**:

- `PZEM004Tv30`
- `PubSubClient`
- `WiFiManager`
- `ArduinoJson`
- `LiquidCrystal_I2C`

---

## 2. Setup Credentials

Open the file:

`include/koneksi.h`

Update your MQTT and local server configuration:
`const char* mqttServer = "YOUR_MQTT_URL";`
`const char* mqttUser   = "YOUR_MQTT_USERNAME";`
`const char* mqttPass   = "YOUR_MQTT_PASSWORD";`

#define LOCAL_SERVER_URL "http://your-local-ip:port/api"

## 3. Deployment

1. Open the project in **VS Code with PlatformIO**
2. **Build and Upload** to your ESP32
3. If WiFi is not configured, connect to hotspot:

`PMS-V1.1-Setup`

Then configure WiFi from your phone.

---

# 📊 Data Schema

The system transmits telemetry in **JSON format**.
```
{
  "v_ac": 220.5,
  "a_ac": 0.45,
  "w_ac": 99.2,
  "e_ac": 12.4,
  "v_dc": 12.6,
  "bat": 95,
  "time": "14:20:00",
  "uptime": "01:23:45"
}
```
# 📂 Project Structure
```.
├── src/
│   └── main.cpp        # Main application logic & loops
├── include/
│   ├── koneksi.h       # WiFi, MQTT, and Watchdog logic
│   ├── lcd_display.h   # Multi-page UI rendering
│   └── tombol.h        # Debounced multi-click button logic
├── platformio.ini      # Project dependencies and config
└── README.md           # Documentation
```
| File            | Description                         |
| --------------- | ----------------------------------- |
| `main.cpp`      | Main application logic              |
| `koneksi.h`     | WiFi, MQTT, and watchdog logic      |
| `lcd_display.h` | Multi-page LCD UI                   |
| `tombol.h`      | Debounced multi-click button system |

# 🤝 Contributing
Feel free to fork this project, report issues, or submit pull requests. For major changes, please open an issue first to discuss what you would like to change.
`Developed by Rama Ari Wahyudi Informatics Engineering Student & IoT Enthusiast`

