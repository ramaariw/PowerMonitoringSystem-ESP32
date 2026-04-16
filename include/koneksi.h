#ifndef KONEKSI_H
#define KONEKSI_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ESPmDNS.h> // <--- Tambahin ini buat fitur Auto-Find Laptop Bapuk
#include "lcd_display.h"

// External Objects
extern WiFiClientSecure espClient;
extern PubSubClient client;
extern bool configWiFiRequested; 
extern bool perluUpdateLCD;

// --- MQTT Configuration ---
const char* mqttServer = "c3a71c8ed6244283a52bcf948e798390.s1.eu.hivemq.cloud";
const char* mqttUser   = "rama_ame69"; 
const char* mqttPass   = "Ramaariwahyudi_27";
const int mqttPort     = 8883;

// Connection Watchdog Timers
extern unsigned long lastMqttRetry;
extern unsigned long lastWifiRetry;
const unsigned long retryInterval = 5000; 

// --- Initial Startup Function ---
inline void setup_wifi() {
    WiFi.mode(WIFI_STA); // Set sebagai client saja biar enteng
    
    // LANGSUNG suruh konek pake memori WiFi yang tersimpan
    // Ini Non-Blocking murni, cuma butuh 1ms buat eksekusi
    WiFi.begin(); 
    
    Serial.println("Connecting in background...");
    tampilkanIntroLCD("Connecting...");
    
    espClient.setInsecure(); 
    
    // DAFTARKAN mDNS di sini biar si Bapuk bisa dikenalin
    if (!MDNS.begin("pms-ame")) {
        Serial.println("Error setting up MDNS responder!");
    }
}

// --- Fungsi Pantau IP (Biar gak budek pas Offline) ---
inline void checkInitialConnection() {
    static bool firstCheckDone = false;
    if (!firstCheckDone && millis() > 10000) { // Cek setelah 10 detik
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Offline Mode Active.");
            tampilkanIntroLCD("Offline Mode");
            perluUpdateLCD = true;
        } else {
            Serial.print("Connected! IP: ");
            Serial.println(WiFi.localIP());
        }
        firstCheckDone = true;
    }
}

// --- Portal WiFi (Hanya Aktif Kalo Dipanggil dari Menu) ---
inline void startWiFiPortal() {
    WiFiManager wm;
    
    lcd.clear();
    lcd.print("PORTAL ACTIVE");
    lcd.setCursor(0, 1);
    lcd.print("PMS-V1.3-CONFIG");

    // Blocking portal biar fokus setting pas di depan dosen
    if (!wm.startConfigPortal("PMS-V1.3-CONFIG")) {
        Serial.println("Portal Timeout");
    } else {
        Serial.println("WiFi Saved! Restarting...");
        lcd.clear();
        lcd.print("WiFi Saved!");
        lcd.setCursor(0, 1);
        lcd.print("Restarting...");
        delay(2000);
        ESP.restart();
    }
}

// --- Watchdog Koneksi (Non-Blocking) ---
inline void keepConnected() {
    unsigned long now = millis();

    // 1. Logic Reconnect WiFi yang Gak Galak
    if (WiFi.status() != WL_CONNECTED) {
        // Cuma coba konek tiap 30 detik sekali kalau lagi offline
        // Biar CPU fokus ke Tombol & Sensor
        if (now - lastWifiRetry > 30000) { 
            lastWifiRetry = now;
            Serial.println("WiFi Down, trying to reconnect gently...");
            WiFi.begin(); 
        }
        return; // LANGSUNG KELUAR, jangan lanjut ke MQTT kalau WiFi aja mati
    }

    // 2. Kalau WiFi Konek, baru urus MQTT
    if (WiFi.status() == WL_CONNECTED && !client.connected()) {
        if (now - lastMqttRetry > retryInterval) {
            lastMqttRetry = now;
            if (client.connect("ESP32_PMS_V14", mqttUser, mqttPass)) {
                client.subscribe("esp32rm/+/cmd");
                Serial.println("MQTT Connected!");
            }
        }
    }
}

#endif