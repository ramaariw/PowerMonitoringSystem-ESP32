# ⚡ Power Monitoring System (PMS) v1.1 — PlatformIO Version

A modular, high-performance **ESP32 firmware** and server-side solution for monitoring both **AC and DC power systems**.  
This version (**v1.1**) is optimized for reliability, featuring a **multi-page LCD interface**, **remote relay control via MQTT**, and a **dual-sync mechanism** for Cloud and Local logging.

---

# ✨ Key Features (v1.1)

- **PlatformIO Migration** Optimized project structure with modular header files (`.h`).

- **Dual-Core Data Handling** Concurrent sensor reading and UI management for zero lag.

- **Dual Sync System**
  - **Cloud (MQTT)** — Secure data publishing for remote dashboards (Node-RED / Grafana).
  - **Local Sync (Si Bapuk)** — High-speed HTTP POST to local server for redundancy and local logging.

- **Advanced UI** 5-page LCD navigation:
  - DC Status (Volt & Battery %)
  - AC Status (V, A, W, kWh)
  - Time & Date
  - Relay Control Status
  - System Health (Uptime)

- **Interactive Controls** Advanced debounced button logic supporting:
  - Single click: Navigation
  - Double click: Toggle Relay 1
  - Triple click: Toggle Relay 2
  - Long press: Settings Menu

- **WiFi Manager** On-the-fly network configuration via captive portal: `PMS-V1.1-Setup`.

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

# 🖥️ Server-Side (Si Bapuk) & Visualization

This project includes a Python-based local server to handle high-frequency data logging and visualization.

### 1. Local Server (`server_bapuk.py`)
A Flask-based gateway that receives data from ESP32 and performs:
- **CSV Logging:** Saves all telemetry to `pms_data_log.csv`.
- **InfluxDB Integration:** Pushes real-time data to InfluxDB for time-series analysis.

**Quick Start:**
```bash
pip install flask influxdb-client
python server/server_bapuk.py
```
### 2. Grafana Dashboard
A pre-configured dashboard (grafana_dashboard_pms.json) is provided.
- Visuals: Gauges for Voltage, Power usage charts, and System status.
- Cost Calculator: Real-time electricity bill estimation based on IDR rates.
- Time Sync: Built-in Flux query to handle WIB (GMT+7) time shifts.

# ⚙️ Configuration & Installation
## 1. Requirements (PlatformIO)
Install these libraries: PZEM004Tv30, PubSubClient, WiFiManager, ArduinoJson, LiquidCrystal_I2C.
## 2. Setup Credentials
Open include/koneksi.h and update:
```
const char* mqttServer = "YOUR_MQTT_URL";
const char* mqttUser   = "YOUR_MQTT_USERNAME"; 
const char* mqttPass   = "YOUR_MQTT_PASSWORD";
#define LOCAL_SERVER_URL "http://your-local-ip:5000/data"
```
## 3. Deployment
1. Open project in VS Code + PlatformIO.
2. Build and Upload to ESP32.
3. If no WiFi, connect to: PMS-V1.1-Setup.

# 📊 Data Schema
```JSON
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
├── src/main.cpp            # Main firmware logic
├── include/                # Modular headers (.h)
├── server/
│   ├── server_bapuk.py     # Flask Server for InfluxDB
│   └── dashboard_pms.json  # Grafana Dashboard export
├── platformio.ini          # Dependencies
└── README.md
```
# 🤝 Contributing
Feel free to fork, report issues, or submit PRs.
`Developed by Rama Ari Wahyudi - Informatics Engineering Student & IoT Enthusiast`