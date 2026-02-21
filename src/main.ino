/**
 * Project: Power Monitoring System (IoT)
 * Author: [ramaariw]
 * Description: Main logic for AC/DC monitoring, MQTT communication, and relay control.
 */

#include <PZEM004Tv30.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include "tombol.h"
#include "lcd_display.h"

// ----------------- WiFi & MQTT Config -----------------
const char* ssid = "[YOUR Wifi SSID]";
const char* password = "[YOUR WIFI PASSWORD]";
const char* mqttServer = "broker.hivemq.com";
const char* clientID = "ESP32IDBYYOU";
const char* topic = "YOUR/TOPICMQTTHERE";
const char* relayTopic = "YOUR/MQTTTOPICHERE";

// ----------------- Pin Configuration -----------------
#define RELAY1 4
#define RELAY2 2
#define VOLTAGE_PIN 34
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2

PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ----------------- DC Voltage Calibration -----------------
float adcMaxVoltage = 16.50;
int adcResolution = 4095;

// ----------------- Global Variables -----------------
unsigned long previousMillis = 0;
const long interval = 5000; // 5 seconds telemetry interval

float lastVoltageAC = 0, lastCurrentAC = 0, lastPowerAC = 0, lastEnergyAC = 0;
float lastVoltageDC = 0;
float batteryPercentage = 0;

// ----------------- LCD Intro Display Helper -----------------
void showLCDIntro(String message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Power Mon System");
  lcd.setCursor(0, 1);
  lcd.print(message);
}

// ----------------- WiFi Setup -----------------
void setup_wifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    showLCDIntro("Connecting WiFi..");
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
  showLCDIntro("WiFi Connected");
  delay(1500);
}

// ----------------- MQTT Reconnection -----------------
void reconnect() {
  while (!client.connected()) {
    showLCDIntro("MQTT Connecting");
    Serial.print("Connecting to MQTT...");
    if (client.connect(clientID)) {
      Serial.println("Connected!");
      client.subscribe(relayTopic);
      showLCDIntro("MQTT Connected");
      delay(1500);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      showLCDIntro("MQTT Failed...");
      delay(2000);
    }
  }
}

// ----------------- MQTT Callback (Incoming Commands) -----------------
void callback(char* topic, byte* payload, unsigned int length) {
  String data = "";
  for (int i = 0; i < length; i++) {
    data += (char)payload[i];
  }

  Serial.println("Message arrived: " + String(topic) + " -> " + data);

  // Relay Control Logic (Active Low)
  if (String(topic) == relayTopic) {
    if (data == "RELAY1_ON") digitalWrite(RELAY1, LOW);
    else if (data == "RELAY1_OFF") digitalWrite(RELAY1, HIGH);
    else if (data == "RELAY2_ON") digitalWrite(RELAY2, LOW);
    else if (data == "RELAY2_OFF") digitalWrite(RELAY2, HIGH);
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize Relays
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);

  pinMode(VOLTAGE_PIN, INPUT);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  setup_wifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  // Initialize PZEM Sensor
  Serial2.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
  
  setupButton();
  Serial.println("System Ready!");
}

void loop() {
  // Check for Energy Reset Request (from Button)
  if (energyResetRequested) {
    energyResetRequested = false;
    pzem.resetEnergy();
    showLCDIntro("Energy Reset!");
    Serial.println("PZEM Energy has been reset by user.");
    delay(1500); 
    perluUpdateLCD = true;
  }

  // Check for Energy Reset via Serial Monitor
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r') { 
      pzem.resetEnergy();
      Serial.println("Energy reset done!");
    }
  }

  checkButton();

  if (!client.connected()) reconnect();
  client.loop();

  // Telemetry Interval
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readSensorsAndPublish();
  }

  // Update LCD if flag is true
  if (perluUpdateLCD) {
    tampilkanLCD(lastVoltageAC, lastCurrentAC, lastPowerAC, lastEnergyAC,
                 lastVoltageDC, batteryPercentage);
    perluUpdateLCD = false;
  }
}

// ----------------- Sensor Reading & Data Publishing -----------------
void readSensorsAndPublish() {
  float voltageAC = pzem.voltage();
  float currentAC = pzem.current();
  float powerAC = pzem.power();
  float energyAC = pzem.energy();

  // Handle NaN (Not a Number) cases from sensor
  if (isnan(voltageAC)) voltageAC = 0;
  if (isnan(currentAC)) currentAC = 0;
  if (isnan(powerAC))   powerAC = 0;
  if (isnan(energyAC))  energyAC = 0;

  // DC Voltage Calculation
  int adcVoltage = analogRead(VOLTAGE_PIN);
  float voltageDC = adcVoltage * 16.50 / adcResolution;
  
  // Battery State of Charge (SOC) Mapping
  batteryPercentage = map(adcVoltage, 2480, 3475, 0, 100);
  if (adcVoltage > 3475) batteryPercentage = 100;
  if (adcVoltage < 2480) batteryPercentage = 0;

  // Prepare JSON-like payload for MQTT
  char payload[128];
  snprintf(payload, sizeof(payload), "%.2f,%.3f,%.2f,%.3f,%.2f,%.1f",
           voltageAC, currentAC, powerAC, energyAC, voltageDC, batteryPercentage);
  client.publish(topic, payload);

  // Debugging
  Serial.println("--- SENSOR DATA ---");
  Serial.printf("AC -> V: %.2f | A: %.3f | W: %.2f | kWh: %.3f\n", voltageAC, currentAC, powerAC, energyAC);
  Serial.printf("DC -> V: %.2f | Battery: %.1f%%\n", voltageDC, batteryPercentage);
  Serial.println("-------------------");

  lastVoltageAC = voltageAC;
  lastCurrentAC = currentAC;
  lastPowerAC = powerAC;
  lastEnergyAC = energyAC;
  lastVoltageDC = voltageDC;

  perluUpdateLCD = true;
}
