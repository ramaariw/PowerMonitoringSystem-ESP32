#ifndef KONEKSI_H
#define KONEKSI_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include "lcd_display.h"

// External Objects (Sinkronisasi dengan main.cpp)
extern WiFiClientSecure espClient;
extern PubSubClient client;
extern bool configWiFiRequested; 
extern bool perluUpdateLCD;

// --- MQTT Configuration (Sudah Sesuai Data Lo) ---
const char* mqttServer = "c3a71c8ed6244283a52bcf948e798390.s1.eu.hivemq.cloud";
const char* mqttUser   = "rama_ame69"; 
const char* mqttPass   = "Ramaariwahyudi_27";
const int mqttPort     = 8883;

// Connection Watchdog Timers
extern unsigned long lastMqttRetry;
extern unsigned long lastWifiRetry;
const unsigned long retryInterval = 5000; 

// --- 1. Initial Startup Function (Non-Blocking) ---
inline void setup_wifi() {
    WiFi.mode(WIFI_STA); 
    
    // Langsung pancing koneksi ke memory WiFi terakhir
    // Ini cuma butuh waktu mikro-detik, jadi gak bikin delay di awal
    WiFi.begin(); 
    
    Serial.println("Connecting in background...");
    tampilkanIntroLCD("Connecting...");
    
    espClient.setInsecure(); // Wajib buat HiveMQ Cloud
}

// --- 2. WiFi Portal (Hanya aktif kalo dipanggil dari Menu LCD) ---
inline void startWiFiPortal() {
    WiFiManager wm;
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CONFIG MODE:");
    lcd.setCursor(0, 1);
    lcd.print("PMS-V1.3-SETUP");

    // Portal ini BLOCKING sengaja biar lo fokus setting pas demo
    if (!wm.startConfigPortal("PMS-V1.3-SETUP")) {
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

// --- 3. Watchdog Koneksi (The "Gentle" Logic) ---
inline void keepConnected() {
    unsigned long now = millis();

    // A. Handle WiFi Down
    if (WiFi.status() != WL_CONNECTED) {
        // Cek tiap 30 detik biar CPU gak abis buat scanning WiFi
        // Ini rahasia biar Tombol gak budek pas Offline
        if (now - lastWifiRetry > 30000) { 
            lastWifiRetry = now;
            Serial.println("WiFi Offline, checking again...");
            WiFi.begin(); 
        }
        return; // Keluar, jangan urus MQTT kalo WiFi mati
    }

    // B. Handle MQTT Down (Hanya kalo WiFi Ready)
    if (WiFi.status() == WL_CONNECTED && !client.connected()) {
        if (now - lastMqttRetry > retryInterval) {
            lastMqttRetry = now;
            Serial.println("Attempting MQTT Connection...");
            
            // Client ID unik biar gak tabrakan sama Flutter
            if (client.connect("ESP32_PMS_AME_V13", mqttUser, mqttPass)) {
                // SUBSCRIBE SEMUA TOPIK SAKTI (V1.3)
                client.subscribe("esp32rm/+/cmd");      // Relay ON/OFF
                client.subscribe("esp32rm/+/timer");    // Timer Menit
                client.subscribe("esp32rm/+/schedule"); // Jam Jadwal
                
                Serial.println("MQTT Connected & Subscribed!");
            }
        }
    }
}

#endif