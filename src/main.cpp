#include <Arduino.h>
#include <PZEM004Tv30.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h> 
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h> 
#include <ArduinoJson.h>

#include "koneksi.h"
#include "lcd_display.h"
#include "tombol.h"

// --- Global Vars ---
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

// NTP Config
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200; 
const int   daylightOffset_sec = 0;

unsigned long previousMillis = 0;
float lastVoltageAC=0, lastCurrentAC=0, lastPowerAC=0, lastEnergyAC=0;
float lastVoltageDC=0, PersenBaterai=0;

// --- Fungsi Waktu & Uptime ---
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

// --- Callback MQTT ---
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
    
    // Aktifkan WiFi AP untuk si Bapuk
    WiFi.softAP("Server_Bapuk", "bapuk123"); 
    Serial.println("AP Aktif: Server_Bapuk");

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); 
    
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
    setupButton();
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
}

void loop() {
    if (!configWiFiRequested) {
        keepConnected();
        client.loop();
    }
    
    checkButton(); // Tombol sekarang dipanggil lebih lancar karena nggak ada blocking panjang

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

        // 1. Kirim ke Cloud (MQTT)
        if (client.connected()) {
            client.publish("esp32rm/sensor", buffer);
        }

        // 2. Kirim ke si Bapuk (Lokal via HTTP POST) - ANTI LAGGING
        if (WiFi.softAPgetStationNum() > 0) { 
            WiFiClient clientLokal;
            HTTPClient http;
            
            // Timeout pendek 150ms biar loop tetep jalan kenceng
            http.setTimeout(150); 
            
            // PENTING: Pake IP lokal si Bapuk (biasanya 192.168.4.2)
            // Bukan IP Tailscale biar gak nyangkut di internet yang lemot
            http.begin(clientLokal, "http://192.168.4.2:5000/data"); 
            http.addHeader("Content-Type", "application/json");
            
            int httpCode = http.POST(buffer); 
            if(httpCode < 0) Serial.println("Lokal Post Timeout/Error");
            
            http.end();
        }
        
        perluUpdateLCD = true;
    }

    static unsigned long lastSec = 0;
    if (!isMenuMode && (displayMode == 2 || displayMode == 4) && millis() - lastSec > 1000) {
        lastSec = millis();
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