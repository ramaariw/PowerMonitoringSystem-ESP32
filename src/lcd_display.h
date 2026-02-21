#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <LiquidCrystal_I2C.h>

extern LiquidCrystal_I2C lcd;
extern bool showAC;           // Toggle: true = AC Data, false = DC+SOC Data
extern bool perluUpdateLCD;

inline void showStatusLCD(const char* message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Status:");
  lcd.setCursor(0, 1);
  lcd.print(message);
}

inline void tampilkanLCD(float voltageAC, float currentAC, float powerAC, float energyAC,
                         float voltageDC, float batteryPercentage) {
  lcd.clear();

  if (showAC) {
    // Mode: AC Monitoring
    lcd.setCursor(0, 0);
    lcd.print("V:");
    lcd.print(voltageAC, 1);
    lcd.print(" A:");
    lcd.print(currentAC, 3);

    lcd.setCursor(0, 1);
    lcd.print("W:");
    lcd.print(powerAC, 1);
    lcd.print("kWh:");
    lcd.print(energyAC, 2);
  } else {
    // Mode: DC Monitoring & Battery Percentage
    lcd.setCursor(0, 0);
    lcd.print("DC Voltage:");
    lcd.print(voltageDC, 1);
    lcd.print("V");
    
    lcd.setCursor(0, 1);
    lcd.print("Battery: ");
    lcd.print(batteryPercentage, 1);
    lcd.print("%");
  }
}

#endif
