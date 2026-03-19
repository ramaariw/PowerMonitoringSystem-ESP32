#ifndef KONEKSI_H
#define KONEKSI_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include "lcd_display.h"

// External Objects
extern WiFiClientSecure espClient;
extern PubSubClient client;
extern bool configWiFiRequested; 

// --- MQTT Configuration (PLEASE FILL BEFORE UPLOADING) ---
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
    WiFiManager wm;
    
    // Set portal timeout (if triggered via menu)
    wm.setConfigPortalTimeout(120); 
    
    Serial.println("Attempting auto-connect to stored WiFi...");
    tampilkanIntroLCD("Connecting...");

    // Try to connect using stored credentials
    if (!wm.autoConnect("PMS-V1.1-Setup")) {
        Serial.println("Connection Failed/Timeout. Entering Offline Mode.");
        tampilkanIntroLCD("Offline Mode");
    } else {
        Serial.println("WiFi Connected!");
        tampilkanIntroLCD("WiFi Connected");
    }
    
    espClient.setInsecure(); 
    delay(1000);
}

// --- Portal Trigger Function (Triggered from Settings Menu) ---
inline void startWiFiPortal() {
    WiFiManager wm;
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CONNECT TO WIFI:");
    lcd.setCursor(0, 1);
    lcd.print("PMS-V1.1-CONFIG");

    if (!wm.startConfigPortal("PMS-V1.1-CONFIG")) {
        Serial.println("Portal Failed/Timeout");
        delay(2000);
    } else {
        Serial.println("New WiFi Saved! Restarting...");
        lcd.clear();
        lcd.print("WiFi Saved!");
        lcd.setCursor(0, 1);
        lcd.print("Restarting...");
        delay(3000);
        ESP.restart();
    }
}

// --- Connection Watchdog (Non-Blocking) ---
inline void keepConnected() {
    unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED) {
        if (now - lastWifiRetry > retryInterval) {
            lastWifiRetry = now;
            WiFi.begin(); 
        }
    }

    if (WiFi.status() == WL_CONNECTED && !client.connected()) {
        if (now - lastMqttRetry > retryInterval) {
            lastMqttRetry = now;
            espClient.setInsecure();
            
            if (client.connect("ESP32_PMSv1.1_Client", mqttUser, mqttPass)) {
                client.subscribe("esp32rm/r1/cmd");
                client.subscribe("esp32rm/r2/cmd");
                client.subscribe("esp32rm/r1/timer");
                client.subscribe("esp32rm/r2/timer");
                client.subscribe("esp32rm/r1/schedule");
                client.subscribe("esp32rm/r2/schedule");
                Serial.println("MQTT Connected & Subscribed!");
            }
        }
    }
}

#endif
