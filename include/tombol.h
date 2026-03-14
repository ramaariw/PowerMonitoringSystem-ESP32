#ifndef TOMBOL_H
#define TOMBOL_H
#include <Arduino.h>
#include <PubSubClient.h> // For MQTT status publishing

#define BUTTON_PIN 32
#define R1_PIN 4
#define R2_PIN 2

// External variables for synchronization with main.cpp
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

    // 1. Debounce Logic
    if (reading != lastReading) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > 50) {
        if (reading != confirmedState) {
            confirmedState = reading;

            if (confirmedState == LOW) { 
                pressStartTime = millis();
                longPressHandled = false;
            } else { 
                unsigned long duration = millis() - pressStartTime;

                // If not a Long Press, count as a click
                if (duration < 600 && !longPressHandled) { 
                    clickCount++;
                    lastClickTime = millis();
                }
            }
        }

        // 2. LONG PRESS Logic (3 Seconds) - Enter/Exit Menu
        if (confirmedState == LOW && !longPressHandled) {
            if (millis() - pressStartTime > 3000) {
                if (!isMenuMode) {
                    isMenuMode = true;
                    menuIndex = 1;
                } else {
                    // Confirm selection in Menu Mode
                    if (menuIndex == 1) energyResetRequested = true;
                    else if (menuIndex == 2) configWiFiRequested = true;
                    else if (menuIndex == 3) isMenuMode = false;
                    isMenuMode = false;
                }
                perluUpdateLCD = true;
                longPressHandled = true;
                clickCount = 0; // Reset click count after long press
            }
        }
    }

    // 3. Execution Based on Click Count (Triggered after 400ms idle)
    if (clickCount > 0 && (millis() - lastClickTime > 400)) {
        
        if (clickCount == 1) { 
            // SINGLE CLICK: Navigation
            if (isMenuMode) {
                menuIndex++;
                if (menuIndex > 3) menuIndex = 1; // Updated to match menu size (3 items)
            } else {
                displayMode++;
                if (displayMode > 4) displayMode = 0;
            }
        } 
        else if (clickCount == 2) { 
            // DOUBLE CLICK: Toggle Relay 1
            statusR1 = !statusR1;
            digitalWrite(R1_PIN, statusR1 ? LOW : HIGH);
            // Sync status with Dashboard (MQTT)
            if(client.connected()) client.publish("esp32rm/r1/stat", statusR1 ? "ON" : "OFF");
        } 
        else if (clickCount == 3) { 
            // TRIPLE CLICK: Toggle Relay 2
            statusR2 = !statusR2;
            digitalWrite(R2_PIN, statusR2 ? LOW : HIGH);
            // Sync status with Dashboard (MQTT)
            if(client.connected()) client.publish("esp32rm/r2/stat", statusR2 ? "ON" : "OFF");
        }

        clickCount = 0; // Reset counter
        perluUpdateLCD = true;
    }

    lastReading = reading;
}
#endif
