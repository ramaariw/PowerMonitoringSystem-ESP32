#ifndef TOMBOL_H
#define TOMBOL_H
#include <Arduino.h>
#include <PubSubClient.h>

#define BUTTON_PIN 32
#define R1_PIN 4
#define R2_PIN 2

extern int displayMode, menuIndex;
extern bool isMenuMode, perluUpdateLCD, energyResetRequested, configWiFiRequested, otaModeActive;
extern bool statusR1, statusR2; 
extern PubSubClient client;    

inline void setupButton() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(R1_PIN, OUTPUT);
    pinMode(R2_PIN, OUTPUT);
    digitalWrite(R1_PIN, HIGH); 
    digitalWrite(R2_PIN, HIGH);
}

inline void checkButton() {
    static bool confirmedState = HIGH;
    static bool lastReading = HIGH;
    static unsigned long lastDebounceTime = 0;
    static unsigned long pressStartTime = 0;
    static bool longPressHandled = false;
    static int clickCount = 0;
    static unsigned long lastClickTime = 0;
    
    bool reading = digitalRead(BUTTON_PIN);

    // 1. Debounce (Gaya V1.1 yang stabil)
    if (reading != lastReading) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > 30) { // Kita turunin ke 30ms biar lebih "klik"
        if (reading != confirmedState) {
            confirmedState = reading;
            if (confirmedState == LOW) { 
                pressStartTime = millis();
                longPressHandled = false;
            } else { 
                unsigned long duration = millis() - pressStartTime;
                if (duration < 600 && !longPressHandled) { 
                    clickCount++;
                    lastClickTime = millis();
                }
            }
        }

        // 2. LONG PRESS (Tetap 3 Detik)
        if (confirmedState == LOW && !longPressHandled) {
            if (millis() - pressStartTime > 3000) {
                if (!isMenuMode) {
                    isMenuMode = true; menuIndex = 1;
                } else {
                    if (menuIndex == 1) energyResetRequested = true;
                    else if (menuIndex == 2) configWiFiRequested = true;
                    else if (menuIndex == 3) isMenuMode = false; 
                    // Tambahin reset ke false biar keluar menu
                    isMenuMode = false;
                }
                perluUpdateLCD = true;
                longPressHandled = true;
                clickCount = 0;
            }
        }
    }

    // 3. EXECUTION (Idle time kita set 350ms - Tengah-tengah antara V1.1 dan V1.4)
    if (clickCount > 0 && (millis() - lastClickTime > 350)) {
        if (clickCount == 1) { 
            if (isMenuMode) {
                menuIndex++;
                if (menuIndex > 3) menuIndex = 1; 
            } else {
                displayMode++;
                if (displayMode > 4) displayMode = 0;
            }
        } 
        else if (clickCount == 2) { 
            statusR1 = !statusR1;
            digitalWrite(R1_PIN, statusR1 ? LOW : HIGH);
            if(client.connected()) client.publish("esp32rm/r1/stat", statusR1 ? "ON" : "OFF");
        } 
        else if (clickCount == 3) { 
            statusR2 = !statusR2;
            digitalWrite(R2_PIN, statusR2 ? LOW : HIGH);
            if(client.connected()) client.publish("esp32rm/r2/stat", statusR2 ? "ON" : "OFF");
        }
        clickCount = 0;
        perluUpdateLCD = true;
    }
    lastReading = reading;
}
#endif