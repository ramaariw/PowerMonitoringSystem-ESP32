#ifndef TOMBOL_H
#define TOMBOL_H
#include <Arduino.h>
#include <PubSubClient.h>

#define BUTTON_PIN 32
#define R1_PIN 4
#define R2_PIN 2

// External variables biar sinkron sama main.cpp
extern int displayMode;
extern int menuIndex;
extern bool isMenuMode;
extern bool perluUpdateLCD;
extern bool energyResetRequested;
extern bool configWiFiRequested;
extern bool statusR1, statusR2; 
extern PubSubClient client;    

inline void setupButton() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(R1_PIN, OUTPUT);
    pinMode(R2_PIN, OUTPUT);
    // Standard relay module logic (Active LOW)
    digitalWrite(R1_PIN, HIGH); 
    digitalWrite(R2_PIN, HIGH);
}

inline void checkButton() {
    static bool confirmedState = HIGH;
    static bool lastReading = HIGH;
    static unsigned long lastDebounceTime = 0;
    static unsigned long pressStartTime = 0;
    static bool longPressHandled = false;
    
    // Multi-Click detection variables
    static int clickCount = 0;
    static unsigned long lastClickTime = 0;
    
    bool reading = digitalRead(BUTTON_PIN);

    // 1. Debounce Logic (Stabil ala V1.2)
    if (reading != lastReading) {
        lastDebounceTime = millis();
    }

    // Debounce 40ms udah cukup kenceng tapi tetep stabil
    if ((millis() - lastDebounceTime) > 40) {
        if (reading != confirmedState) {
            confirmedState = reading;

            if (confirmedState == LOW) { 
                pressStartTime = millis();
                longPressHandled = false;
            } else { 
                unsigned long duration = millis() - pressStartTime;

                // Single/Multi Click detection
                if (duration < 600 && !longPressHandled) { 
                    clickCount++;
                    lastClickTime = millis();
                }
            }
        }

        // 2. LONG PRESS Logic (3 Detik) - Masuk/Keluar Menu
        if (confirmedState == LOW && !longPressHandled) {
            if (millis() - pressStartTime > 3000) {
                if (!isMenuMode) {
                    isMenuMode = true;
                    menuIndex = 1;
                } else {
                    // Eksekusi pilihan menu
                    if (menuIndex == 1) energyResetRequested = true;
                    else if (menuIndex == 2) configWiFiRequested = true;
                    else if (menuIndex == 3) isMenuMode = false;
                    
                    isMenuMode = false; // Keluar menu setelah pilih
                }
                perluUpdateLCD = true;
                longPressHandled = true;
                clickCount = 0; 
            }
        }
    }

    // 3. Execution (Triggered setelah 350ms idle - Lebih responsif dari V1.2)
    if (clickCount > 0 && (millis() - lastClickTime > 350)) {
        
        if (clickCount == 1) { 
            // SINGLE CLICK: Navigasi Halaman / Menu
            if (isMenuMode) {
                menuIndex++;
                if (menuIndex > 3) menuIndex = 1; 
            } else {
                displayMode++;
                if (displayMode > 4) displayMode = 0;
            }
        } 
        else if (clickCount == 2) { 
            // DOUBLE CLICK: Toggle Relay 1
            statusR1 = !statusR1;
            digitalWrite(R1_PIN, statusR1 ? LOW : HIGH);
            if(client.connected()) client.publish("esp32rm/r1/stat", statusR1 ? "ON" : "OFF");
        } 
        else if (clickCount == 3) { 
            // TRIPLE CLICK: Toggle Relay 2
            statusR2 = !statusR2;
            digitalWrite(R2_PIN, statusR2 ? LOW : HIGH);
            if(client.connected()) client.publish("esp32rm/r2/stat", statusR2 ? "ON" : "OFF");
        }

        clickCount = 0; // Reset counter
        perluUpdateLCD = true;
    }

    lastReading = reading;
}
#endif