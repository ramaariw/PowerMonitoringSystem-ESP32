#include <Arduino.h>
#include <PZEM004Tv30.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h> 
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h> 
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
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
bool otaModeActive = false;
WiFiManager wm;

unsigned long lastMqttRetry = 0;
unsigned long lastWifiRetry = 0;
unsigned long previousMillis = 0;

bool statusR1 = false;
bool statusR2 = false;

// === VARIABLE TIMER & SCHEDULER ===
unsigned long timerStartR1 = 0, timerDurationR1 = 0;
bool timerR1Active = false;
unsigned long timerStartR2 = 0, timerDurationR2 = 0;
bool timerR2Active = false;

String scheduleR1 = ""; 
String scheduleR2 = ""; 

PZEM004Tv30 pzem(Serial2, 16, 17);
WiFiClientSecure espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// NTP Configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200; 
const int daylightOffset_sec = 0;

float lastVoltageAC=0, lastCurrentAC=0, lastPowerAC=0, lastEnergyAC=0;
float lastVoltageDC=0, PersenBaterai=0;

// --- Time Helpers ---
String getClock() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return "--:--:--";
    char buffer[10];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    return String(buffer);
}

String getShortClock() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return "--:--";
    char buffer[6];
    strftime(buffer, sizeof(buffer), "%H:%M", &timeinfo);
    return String(buffer);
}

String getDate() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return "--/--/--";
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
    if (len == 0) return;
    String msg = "";
    for (int i = 0; i < len; i++) msg += (char)payload[i];
    String t = String(topic);

    if (t == "esp32rm/r1/cmd") {
        statusR1 = (msg == "ON");
        digitalWrite(4, statusR1 ? LOW : HIGH);
        timerR1Active = false; scheduleR1 = ""; 
    } 
    else if (t == "esp32rm/r2/cmd") {
        statusR2 = (msg == "ON");
        digitalWrite(2, statusR2 ? LOW : HIGH);
        timerR2Active = false; scheduleR2 = "";
    }
    else if (t == "esp32rm/r1/timer") {
        long mins = msg.toInt();
        if (mins > 0) {
            timerStartR1 = millis();
            timerDurationR1 = (unsigned long)mins * 60000;
            timerR1Active = true;
        } else { timerR1Active = false; }
    } 
    else if (t == "esp32rm/r2/timer") {
        long mins = msg.toInt();
        if (mins > 0) {
            timerStartR2 = millis();
            timerDurationR2 = (unsigned long)mins * 60000;
            timerR2Active = true;
        } else { timerR2Active = false; }
    }
    
    if (client.connected()) {
        client.publish("esp32rm/r1/stat", statusR1 ? "ON" : "OFF");
        client.publish("esp32rm/r2/stat", statusR2 ? "ON" : "OFF");
    }
    perluUpdateLCD = true;
}

// --- OTA SETUP ---
void setup_ota() {
    ArduinoOTA.setHostname("pms-ota-ame");
    ArduinoOTA.setPassword("bapuk123");
    ArduinoOTA.begin();
}

void setup() {
    Serial.begin(115200);
    
    pinMode(4, OUTPUT); digitalWrite(4, HIGH);
    pinMode(2, OUTPUT); digitalWrite(2, HIGH);
    
    lcd.init(); lcd.backlight();
    tampilkanIntroLCD("PMS V1.4 STABLE");

    setup_wifi(); 

    setup_ota();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); 
    
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
    setupButton();
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
}

void loop() {
    // --- 1. PRIORITAS UTAMA: TOMBOL & OTA ---
    // Harus dipanggil tiap milidetik tanpa penghalang!
    checkButton(); 
    ArduinoOTA.handle();

    unsigned long currentMillis = millis();

    // --- 2. LOGIC KONEKSI (KITA BATASI BANGET) ---
    // Biar gak ganggu tombol pas WiFi lagi bapuk/nyari sinyal
    static unsigned long lastNetUpdate = 0;
    if (currentMillis - lastNetUpdate > 2000) { 
        if (!configWiFiRequested && !otaModeActive) {
            keepConnected(); // Pake versi gentle reconnect (jeda 30 detik)
            if (WiFi.status() == WL_CONNECTED) {
                client.loop();
            }
        }
        lastNetUpdate = currentMillis;
    }

    // --- 3. LOGIC SETTING & RESET (Nyontek V1.1) ---
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

    // --- 4. OTA MODE PROTEKSI ---
    if (otaModeActive) {
        static unsigned long lastOtaLCD = 0;
        if (currentMillis - lastOtaLCD > 1000) {
            lastOtaLCD = currentMillis;
            lcd.clear();
            lcd.print("OTA MODE ACTIVE");
            lcd.setCursor(0, 1);
            lcd.print("WAITING FILE...");
        }
        return; // Berhenti di sini kalau lagi update OTA
    }

    // --- 5. TIMER RELAY LOGIC (V1.4) ---
    if (timerR1Active && (currentMillis - timerStartR1 >= timerDurationR1)) {
        statusR1 = false; digitalWrite(4, HIGH);
        timerR1Active = false; perluUpdateLCD = true;
        if (WiFi.status() == WL_CONNECTED && client.connected()) client.publish("esp32rm/r1/stat", "OFF");
    }
    if (timerR2Active && (currentMillis - timerStartR2 >= timerDurationR2)) {
        statusR2 = false; digitalWrite(2, HIGH);
        timerR2Active = false; perluUpdateLCD = true;
        if (WiFi.status() == WL_CONNECTED && client.connected()) client.publish("esp32rm/r2/stat", "OFF");
    }

    // --- 6. PEMBACAAN SENSOR (Tiap 3 Detik) ---
    if (currentMillis - previousMillis >= 3000) {
        previousMillis = currentMillis;
        
        // Baca PZEM (Tambahkan pengecekan nan biar gak crash)
        float v = pzem.voltage(); float c = pzem.current();
        float p = pzem.power(); float e = pzem.energy();
        lastVoltageAC = isnan(v) ? 0 : v; lastCurrentAC = isnan(c) ? 0 : c;
        lastPowerAC = isnan(p) ? 0 : p; lastEnergyAC = isnan(e) ? 0 : e;

        // Baca DC (Baterai)
        int adc = analogRead(34);
        lastVoltageDC = (adc / 4095.0) * 3.3 * 5.0; 
        PersenBaterai = constrain(((lastVoltageDC - 10.5) / (12.7 - 10.5)) * 100.0, 0, 100);

        // SYNC DATA HANYA JIKA WIFI CONNECTED
        if (WiFi.status() == WL_CONNECTED) { 
            JsonDocument doc;
            doc["v_ac"] = lastVoltageAC; doc["a_ac"] = lastCurrentAC;
            doc["w_ac"] = lastPowerAC; doc["e_ac"] = lastEnergyAC;
            doc["v_dc"] = lastVoltageDC; doc["bat"] = PersenBaterai;
            doc["time"] = getClock(); doc["uptime"] = getUptime();

            char buffer[384];
            serializeJson(doc, buffer);

            if (client.connected()) client.publish("esp32rm/sensor", buffer);

            WiFiClient clientLokal;
            HTTPClient http;
            http.setTimeout(80); 
            if(http.begin(clientLokal, "http://bapuk-server.local:5000/data")) {
                http.addHeader("Content-Type", "application/json");
                http.POST(buffer);
                http.end();
            }
        }
        perluUpdateLCD = true;
    }

    // --- 7. UPDATE LCD ---
    if (perluUpdateLCD) {
        if (isMenuMode) {
            tampilkanMenu(menuIndex);
        } else if (!configWiFiRequested) {
            // Ambil fungsi tampilan dari lcd_display.h
            tampilkanLCD(lastVoltageAC, lastCurrentAC, lastPowerAC, lastEnergyAC, 
                         lastVoltageDC, PersenBaterai, getClock(), getDate(), getUptime());
        }
        perluUpdateLCD = false;
    }
}