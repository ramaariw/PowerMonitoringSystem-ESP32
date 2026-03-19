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

unsigned long lastMqttRetry = 0;
unsigned long lastWifiRetry = 0;

bool statusR1 = false;
bool statusR2 = false;

// === VARIABLE TIMER & SCHEDULER (NEW V1.2) ===
unsigned long timerStartR1 = 0, timerDurationR1 = 0;
bool timerR1Active = false;

unsigned long timerStartR2 = 0, timerDurationR2 = 0;
bool timerR2Active = false;

String scheduleR1 = ""; // Format: "HH:MM"
String scheduleR2 = ""; // Format: "HH:MM"
// ==============================================

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

// Helper baru buat Scheduler (Biar gampang dicompare misal "22:30")
String getShortClock() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return "00:00";
    char buffer[6];
    strftime(buffer, sizeof(buffer), "%H:%M", &timeinfo);
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

// --- OTA SETUP ---
void setup_ota() {
    ArduinoOTA.setHostname("pms-ota");
    ArduinoOTA.setPassword("bapuk123"); // Password biar aman pas update

    ArduinoOTA.onStart([]() {
        lcd.clear();
        lcd.print("OTA UPDATING...");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        lcd.setCursor(0, 1);
        lcd.print("Progress: ");
        lcd.print(progress / (total / 100));
        lcd.print("%");
    });
    ArduinoOTA.onEnd([]() {
        lcd.clear();
        lcd.print("UPDATE SUCCESS!");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        lcd.clear();
        lcd.print("UPDATE FAILED!");
    });
    ArduinoOTA.begin();
}

// --- MQTT Callback ---
void callback(char* topic, byte* payload, unsigned int len) {
    if (len == 0) return; // Proteksi payload kosong

    String msg = "";
    for (int i = 0; i < len; i++) msg += (char)payload[i];
    String t = String(topic);

    // 1. MANUAL COMMAND
    if (t == "esp32rm/r1/cmd") {
        statusR1 = (msg == "ON");
        digitalWrite(4, statusR1 ? LOW : HIGH);
        // Jika dinyalain manual, matikan sisa timer/jadwal biar gak bingung
        timerR1Active = false;
        scheduleR1 = ""; 
    } 
    else if (t == "esp32rm/r2/cmd") {
        statusR2 = (msg == "ON");
        digitalWrite(2, statusR2 ? LOW : HIGH);
        timerR2Active = false;
        scheduleR2 = "";
    }
    
    // 2. TIMER (Payload dalam MENIT)
    else if (t == "esp32rm/r1/timer") {
        long mins = msg.toInt();
        if (mins > 0) {
            mins = constrain(mins, 1, 1440); // Max 24 jam biar aman
            timerStartR1 = millis();
            timerDurationR1 = (unsigned long)mins * 60000;
            timerR1Active = true;
            Serial.printf("Timer R1 Set: %ld min\n", mins);
        } else {
            timerR1Active = false;
        }
    } 
    else if (t == "esp32rm/r2/timer") {
        long mins = msg.toInt();
        if (mins > 0) {
            mins = constrain(mins, 1, 1440);
            timerStartR2 = millis();
            timerDurationR2 = (unsigned long)mins * 60000;
            timerR2Active = true;
            Serial.printf("Timer R2 Set: %ld min\n", mins);
        } else {
            timerR2Active = false;
        }
    }

    // 3. SCHEDULE (Payload format "HH:MM")
    else if (t == "esp32rm/r1/schedule") {
        if (msg == "OFF" || msg.length() < 5) scheduleR1 = "";
        else scheduleR1 = msg;
    } 
    else if (t == "esp32rm/r2/schedule") {
        if (msg == "OFF" || msg.length() < 5) scheduleR2 = "";
        else scheduleR2 = msg;
    }
    
    // Publish balik status ke Flutter biar UI gak 'mental' (ghosting)
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
    tampilkanIntroLCD("PMS V1.3 by AME"); // Naik Versi Cuy!

    WiFi.mode(WIFI_AP_STA); 
    setup_wifi(); 
    
    // AP for Local Connection (Si Bapuk)
    WiFi.softAP("Server_Bapuk_AP", "bapuk123"); 
    Serial.println("AP Active: Server_Bapuk_AP");

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

    // =========================================================
    // EKSEKUSI TIMER MILLIS (Berjalan Non-Stop / Non-Blocking)
    // =========================================================
    unsigned long currentMillis = millis();

    // Cek Timer Relay 1
    if (timerR1Active && (currentMillis - timerStartR1 >= timerDurationR1)) {
        statusR1 = false;
        digitalWrite(4, HIGH); // Matiin Relay
        timerR1Active = false; // Reset Timer
        if (client.connected()) client.publish("esp32rm/r1/stat", "OFF");
        perluUpdateLCD = true;
    }

    // Cek Timer Relay 2
    if (timerR2Active && (currentMillis - timerStartR2 >= timerDurationR2)) {
        statusR2 = false;
        digitalWrite(2, HIGH); // Matiin Relay
        timerR2Active = false; // Reset Timer
        if (client.connected()) client.publish("esp32rm/r2/stat", "OFF");
        perluUpdateLCD = true;
    }

    // =========================================================
    // EKSEKUSI PEMBACAAN SENSOR & SCHEDULER (Tiap 3 Detik)
    // =========================================================
    if (currentMillis - previousMillis >= 3000) {
        previousMillis = currentMillis;
        
        // --- Eksekusi Scheduler (Biar gaperlu cek tiap milidetik) ---
        String currentHHMM = getShortClock();
        
        if (scheduleR1 != "" && currentHHMM == scheduleR1) {
            statusR1 = false;
            digitalWrite(4, HIGH);
            scheduleR1 = ""; // Clear jadwal biar gak kepanggil terus sampe menitnya ganti
            if (client.connected()) client.publish("esp32rm/r1/stat", "OFF");
            perluUpdateLCD = true;
        }

        if (scheduleR2 != "" && currentHHMM == scheduleR2) {
            statusR2 = false;
            digitalWrite(2, HIGH);
            scheduleR2 = ""; 
            if (client.connected()) client.publish("esp32rm/r2/stat", "OFF");
            perluUpdateLCD = true;
        }

        // --- Sensor Reading logic ---
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
        
        // (Optional) Kirim sisa waktu ke Flutter biar bisa update UI
        doc["t1_rem"] = timerR1Active ? ((timerDurationR1 - (currentMillis - timerStartR1)) / 1000) : 0;
        doc["t2_rem"] = timerR2Active ? ((timerDurationR2 - (currentMillis - timerStartR2)) / 1000) : 0;
        doc["sch_1"] = scheduleR1;
        doc["sch_2"] = scheduleR2;

        char buffer[350]; // Dibesarin dikit buffernya krn JSON nambah
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
            String serverUrl = "http://192.168.4.2:5000/data"; 
    
            http.begin(clientLokal, serverUrl); // <--- UDAH DIBENERIN
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