# PowerMonitoringSystem
This project is a complete IoT power monitoring and control system built with ESP32, PZEM-004T, DC Voltage Sensor, and 2-channel Relay Module, integrated with MQTT for real-time data communication.  The system measures AC &amp; DC parameters and allows remote relay control via an MQTT dashboard (Node-RED / mobile app).

+ Features :
- Real-time AC Voltage Monitoring (PZEM-004T)
- Real-time AC Current Monitoring
- DC Voltage Sensor Integration
- MQTT Telemetry (publish sensor data)
- MQTT Control (subscribe relay commands)
- Auto-Reconnect WiFi & MQTT
- Node-RED Dashboard ready

+ Hardware Components :
Below is the complete hardware list used in this Power Monitoring & Control System project, including the core IoT components and all supporting modules integrated inside the device.
                                                                   //Description :
- ESP32 DevKit v1	Main controller                                  Main Controller
- PZEM-004T v3.0	AC voltage & current sensor                      AC Voltage & Current Sensor
- DC Voltage Sensor	Up to 25V measurement                          DC Voltage Sensor
- Relay Module 2-Channel	Remote switch control                    Remote Switch Control    
- Wiring	Jumpers                                                  Internal wiring for sensor and module connections
- LM2596 Buck Converter (Step-Down)                                Used to step down various input power sources (e.g., 9V, 12V, 24V) to a stable 5V output.
                                                                   Ensures the entire PMS device can accept flexible adapter inputs safely.
- DC Barrel Jack Connector 3.5                                     Standard DC connector used as the main power input for the system. Allows the device to be                                                                          powered using any DC wall adapter.
- Terminal Block Connectors                                        Provides secure and organized wiring for all high and low voltage lines:
                                                                   * DC + / – input and output
                                                                   * AC L & N for PZEM-004T measurement input
                                                                   * AC L & N output for load after relay switching
- 5V Mini Cooling Fan                                              Mounted on the enclosure to exhaust hot air.
- LCD 16×2 (I2C Interface)                                         Displays real-time measurement
- Push Button (Mode Selector)                                      Used to switch LCD display modes:
                                                                   * AC Monitoring Mode
                                                                   * DC Monitoring Mode


