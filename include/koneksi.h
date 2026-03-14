#ifndef KONEKSI_H
#define KONEKSI_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include "lcd_display.h"

// Objek eksternal
extern WiFiClientSecure espClient;
extern PubSubClient client;
extern bool configWiFiRequested; // Ganti nama variabel dari bluetoothModeActive

// Konfigurasi MQTT HiveMQ Cloud
const char* mqttServer = "c3a71c8ed6244283a52bcf948e798390.s1.eu.hivemq.cloud";
const char* mqttUser = "rama_ame69";
const char* mqttPass = "Ramaariwahyudi_27";
const int mqttPort = 8883;

// Timer non-blocking
extern unsigned long lastMqttRetry;
extern unsigned long lastWifiRetry;
const unsigned long retryInterval = 5000; 

// --- Fungsi Start-up Awal ---
inline void setup_wifi() {
    WiFiManager wm;
    
    // Set timeout agar jika WiFi lama tidak ada, dia tetap masuk ke loop utama (offline mode)
    wm.setConfigPortalTimeout(120); // 2 menit batas waktu portal jika dipicu
    
    Serial.println("Mencoba auto-connect ke WiFi terakhir...");
    tampilkanIntroLCD("Connecting...");

    // Mencoba konek ke WiFi yang tersimpan di flash memory otomatis
    if (!wm.autoConnect("PMS-V1.1-Setup")) {
        Serial.println("Gagal konek/Timeout. Masuk mode monitoring.");
        tampilkanIntroLCD("Offline Mode");
    } else {
        Serial.println("WiFi Terkoneksi!");
        tampilkanIntroLCD("WiFi Connected");
    }
    
    espClient.setInsecure(); 
    delay(1000);
}

// --- Fungsi Pemicu Portal (Dijalankan dari Menu) ---
inline void startWiFiPortal() {
    WiFiManager wm;
    
    // Hapus tampilan sensor, ganti info portal
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CONNECT TO WIFI:");
    lcd.setCursor(0, 1);
    lcd.print("PMS-V1.1-CONFIG");

    // Membuat ESP32 jadi Access Point
    // HP lo nanti konek ke "PMS-V1.1-CONFIG" tanpa password
    if (!wm.startConfigPortal("PMS-V1.1-CONFIG")) {
        Serial.println("Portal gagal/timeout");
        delay(2000);
    } else {
        // Jika berhasil simpan WiFi baru via browser
        Serial.println("WiFi Baru Tersimpan! Restarting...");
        lcd.clear();
        lcd.print("WiFi Saved!");
        lcd.setCursor(0, 1);
        lcd.print("Restarting...");
        delay(3000);
        ESP.restart();
    }
}

// --- Logika Penjaga Koneksi ---
inline void keepConnected() {
    unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED) {
        if (now - lastWifiRetry > retryInterval) {
            lastWifiRetry = now;
            // WiFiManager menghandle penyimpanan kredensial secara otomatis, 
            // jadi kita cukup panggil WiFi.begin() tanpa parameter untuk reconnect
            WiFi.begin(); 
        }
    }

    if (WiFi.status() == WL_CONNECTED && !client.connected()) {
        if (now - lastMqttRetry > retryInterval) {
            lastMqttRetry = now;
            espClient.setInsecure();
            if (client.connect("ESP32_PMSv1.1_Client", mqttUser, mqttPass)) {
                // --- INI SAKTI NYA, TAMBAHIN DI SINI ---
                client.subscribe("esp32rm/r1/cmd");
                client.subscribe("esp32rm/r2/cmd");
                Serial.println("Subscribed to Relay Topics!");
            // ---------------------------------------
            }
        }
    }
}

#endif