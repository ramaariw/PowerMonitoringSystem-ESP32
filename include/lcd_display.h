#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <LiquidCrystal_I2C.h>
#include <stdio.h>

extern LiquidCrystal_I2C lcd;
extern int displayMode; 
extern int menuIndex;   
extern bool isMenuMode; 
extern bool perluUpdateLCD;
extern bool statusR1, statusR2;

inline void printLine(int row, const char* format, ...) {
    char buffer[17];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    lcd.setCursor(0, row);
    lcd.print(buffer);
    for (int i = strlen(buffer); i < 16; i++) lcd.print(" ");
}

inline void tampilkanLCD(float vAC, float cAC, float pAC, float eAC,
                         float vDC, float pBat, String clock, String date, String uptime) {
    
    if (displayMode == 0) { // Page 1: DC System
        printLine(0, "DC VOLT: %.2fV", vDC);
        printLine(1, "BATTERY: %.1f%%", pBat);
    } 
    else if (displayMode == 1) { // Page 2: AC System
        printLine(0, "V:%.1f A:%.3f", vAC, cAC);
        printLine(1, "W:%.1f kWh:%.2f", pAC, eAC);
    } 
    else if (displayMode == 2) { // Page 3: Time & Date (Balik Lagi Cuy!)
        printLine(0, "TIME:   %s", clock.c_str());
        printLine(1, "DATE: %s", date.c_str());
    }
    else if (displayMode == 3) { // Page 4: Control Status
        printLine(0, "TERMINAL 1: %s", statusR1 ? "ON" : "OFF");
        printLine(1, "TERMINAL 2: %s", statusR2 ? "ON" : "OFF");
    }
    else if (displayMode == 4) { // Page 5: System Health
        printLine(0, "SYSTEM HEALTH");
        printLine(1, "UPTM: %s", uptime.c_str());
    }
}

inline void tampilkanMenu(int index) {
    printLine(0, "--- SETTINGS ---");
    if (index == 1)      printLine(1, "> RESET ENERGY  ");
    else if (index == 2) printLine(1, "> CONFIG WIFI   ");
    else if (index == 3) printLine(1, "> EXIT MENU     ");
}

inline void tampilkanIntroLCD(const char* msg) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PMS V1.3 - AME");
    lcd.setCursor(0, 1);
    lcd.print(msg);
}

#endif