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
    static bool confirmedState = HIGH, lastReading = HIGH;
    static unsigned long lastDebounceTime = 0, pressStartTime = 0;
    static bool longPressHandled = false;
    static int clickCount = 0;
    static unsigned long lastClickTime = 0;
    
    bool reading = digitalRead(BUTTON_PIN);
    if (reading != lastReading) lastDebounceTime = millis();

    if ((millis() - lastDebounceTime) > 50) {
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

        // Long Press (3s)
        if (confirmedState == LOW && !longPressHandled && (millis() - pressStartTime > 3000)) {
            if (!isMenuMode) {
                isMenuMode = true; menuIndex = 1;
            } else {
                if (menuIndex == 1) energyResetRequested = true;
                else if (menuIndex == 2) configWiFiRequested = true;
                else if (menuIndex == 3) otaModeActive = true;
                else if (menuIndex == 4) isMenuMode = false;
                if (menuIndex != 4) isMenuMode = false; // Exit after action
            }
            perluUpdateLCD = true; longPressHandled = true; clickCount = 0;
        }
    }

    if (clickCount > 0 && (millis() - lastClickTime > 400)) {
        if (clickCount == 1) { 
            if (isMenuMode) {
                menuIndex++; if (menuIndex > 4) menuIndex = 1;
            } else {
                displayMode++; if (displayMode > 4) displayMode = 0;
            }
        } 
        else if (clickCount == 2) { 
            statusR1 = !statusR1; digitalWrite(R1_PIN, statusR1 ? LOW : HIGH);
            if(client.connected()) client.publish("esp32rm/r1/stat", statusR1 ? "ON" : "OFF");
        } 
        else if (clickCount == 3) { 
            statusR2 = !statusR2; digitalWrite(R2_PIN, statusR2 ? LOW : HIGH);
            if(client.connected()) client.publish("esp32rm/r2/stat", statusR2 ? "ON" : "OFF");
        }
        clickCount = 0; perluUpdateLCD = true;
    }
    lastReading = reading;
}
#endif