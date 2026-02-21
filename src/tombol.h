#ifndef TOMBOL_H
#define TOMBOL_H

#define BUTTON_PIN 32

#include <Arduino.h>

inline bool showAC = false;
inline bool perluUpdateLCD = true;
inline bool energyResetRequested = false;

inline void setupButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

inline void checkButton() {
  static bool lastButtonState = HIGH;
  static unsigned long pressStartTime = 0;
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Button Pressed
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    pressStartTime = millis();
  }

  // Button Held Down (Long Press)
  if (lastButtonState == LOW && currentButtonState == LOW) {
    if (millis() - pressStartTime > 5000) {  // 5 seconds hold
      energyResetRequested = true;
      Serial.println("Long press detected: Requesting Energy Reset!");
      pressStartTime = millis();  // Reset timer to prevent multiple triggers
    }
  }

  // Button Released
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    if (millis() - pressStartTime < 5000) {  // Short press
      showAC = !showAC;
      perluUpdateLCD = true;
      Serial.print("Short press detected! Mode: ");
      Serial.println(showAC ? "AC" : "DC/Battery");
    }
  }

  lastButtonState = currentButtonState;
}

#endif
