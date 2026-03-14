#include <Arduino.h>
#include <PZEM004Tv30.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h> 
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h> 
#include <ArduinoJson.h>

#include "config.h" // Load credentials safely
#include "koneksi.h"
#include "lcd_display.h"
#include "tombol.h"

// --- Global Variables ---
int displayMode = 0; 
int menuIndex = 1;
bool isMenuMode = false;
bool perluUpdateLCD = true;
bool energyResetRequested = false;
bool configWiFiRequested = false; 

unsigned long lastMqttRetry = 0;
unsigned long lastWifiRetry = 0;

bool statusR1 = false;
bool statusR2 = false;

PZEM004Tv30 pzem(Serial2, 16, 17);
WiFiClientSecure espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// NTP Configuration
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200; // GMT+7
const int   daylightOffset_sec = 0;

unsigned long previousMillis = 0;
float lastVoltageAC=0, lastCurrentAC=0, lastPowerAC=0, lastEnergyAC=0;
float lastVoltageDC=0, PersenBaterai=0;

// --- Time & Uptime Helpers ---
String getClock() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return "00:00:00";
    char buffer[10];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    return String(buffer);
}

String getDate() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return "00/00/00";
    char buffer[12];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y", &timeinfo);
    return String(buffer);
}

String getUptime() {
    unsigned long totalSeconds = millis() / 1000;
    int days = totalSeconds / 86400;
    int hours = (totalSeconds % 86400) / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    char buffer[20];
    if (days > 0) snprintf(buffer, sizeof(buffer), "%dd %02d:%02d:%02d", days, hours, minutes, seconds);
    else snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, seconds);
    return String(buffer);
}

// --- MQTT Callback ---
void callback(char* topic, byte* payload, unsigned int len) {
    String msg = "";
    for (int i = 0; i < len; i++) msg += (char)payload[i];
    String t = String(topic);

    if (t == "esp32rm/r1/cmd") {
        statusR1 = (msg == "ON");
        digitalWrite(4, statusR1 ? LOW : HIGH);
    } else if (t == "esp32rm/r2/cmd") {
        statusR2 = (msg == "ON");
        digitalWrite(2, statusR2 ? LOW : HIGH);
    }
    
    // Sync status back to dashboard
    if (client.connected()) {
        client.publish("esp32rm/r1/stat", statusR1 ? "ON" : "OFF");
        client.publish("esp32rm/r2/stat", statusR2 ? "ON" : "OFF");
    }
    perluUpdateLCD = true;
}

void setup() {
    Serial.begin(115200);
    
    pinMode(4, OUTPUT); digitalWrite(4, HIGH);
    pinMode(2, OUTPUT); digitalWrite(2, HIGH);
    
    lcd.init(); lcd.backlight();
    tampilkanIntroLCD("PMS V1.1 by AME");

    WiFi.mode(WIFI_AP_STA); 
    setup_wifi(); 
    
    // AP for Local Connection (Si Bapuk)
    WiFi.softAP("Server_Bapuk_AP", "bapuk123"); 
    Serial.println("AP Active: Server_Bapuk_AP");

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); 
    
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
    setupButton();
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
}

void loop() {
    if (!configWiFiRequested) {
        keepConnected();
        client.loop();
    }
    
    checkButton(); 

    if (energyResetRequested) {
        pzem.resetEnergy();
        tampilkanIntroLCD("ENERGY CLEARED!");
        delay(1500);
        energyResetRequested = false;
        perluUpdateLCD = true;
    }

    if (configWiFiRequested) {
        startWiFiPortal(); 
        configWiFiRequested = false; 
        perluUpdateLCD = true;
    }

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 3000) {
        previousMillis = currentMillis;
        
        // Sensor Reading logic...
        float v = pzem.voltage(); float c = pzem.current();
        float p = pzem.power(); float e = pzem.energy();
        lastVoltageAC = isnan(v) ? 0 : v; lastCurrentAC = isnan(c) ? 0 : c;
        lastPowerAC = isnan(p) ? 0 : p; lastEnergyAC = isnan(e) ? 0 : e;

        int adc = analogRead(34);
        lastVoltageDC = (adc / 4095.0) * 3.3 * 5.0; 
        PersenBaterai = constrain(((lastVoltageDC - 10.5) / (12.7 - 10.5)) * 100.0, 0, 100);

        JsonDocument doc;
        doc["v_ac"] = lastVoltageAC;
        doc["a_ac"] = lastCurrentAC;
        doc["w_ac"] = lastPowerAC;
        doc["e_ac"] = lastEnergyAC;
        doc["v_dc"] = lastVoltageDC;
        doc["bat"]  = PersenBaterai;
        doc["time"] = getClock();
        doc["uptime"] = getUptime();

        char buffer[256];
        serializeJson(doc, buffer);

        // 1. Cloud Sync
        if (client.connected()) {
            client.publish("esp32rm/sensor", buffer);
        }

        // 2. Local Server Sync (Si Bapuk)
        if (WiFi.softAPgetStationNum() > 0) { 
            WiFiClient clientLokal;
            HTTPClient http;
            http.setTimeout(150); 
            http.begin(clientLokal, LOCAL_SERVER_URL); 
            http.addHeader("Content-Type", "application/json");
            
            int httpCode = http.POST(buffer); 
            if(httpCode < 0) Serial.println("Local Sync Failed");
            http.end();
        }
        
        perluUpdateLCD = true;
    }

    if (perluUpdateLCD) {
        if (isMenuMode) {
            tampilkanMenu(menuIndex);
        } else if (!configWiFiRequested) {
            tampilkanLCD(lastVoltageAC, lastCurrentAC, lastPowerAC, lastEnergyAC, 
                         lastVoltageDC, PersenBaterai, getClock(), getDate(), getUptime());
        }
        perluUpdateLCD = false;
    }
}
