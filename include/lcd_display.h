#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <LiquidCrystal_I2C.h>
#include <stdio.h>

extern LiquidCrystal_I2C lcd;
extern int displayMode;   // 0=DC, 1=AC, 2=WAKTU, 3=RELAY
extern int menuIndex;     // 1=Reset Energy, 2=Config WiFi, 3=Exit
extern bool isMenuMode;   // Status apakah lagi di dalam menu atau tidak
extern bool perluUpdateLCD;
extern bool statusR1, statusR2; // Tambahan status relay dari main.cpp

// --- Helper: Print Baris Anti-Flicker ---
inline void printLine(int row, const char* format, ...) {
    char buffer[17];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    lcd.setCursor(0, row);
    lcd.print(buffer);
    // Isi sisa kolom dengan spasi agar karakter lama tertimpa bersih
    for (int i = strlen(buffer); i < 16; i++) lcd.print(" ");
}

// --- Tampilan 1-5: Monitoring & Control ---
inline void tampilkanLCD(float vAC, float cAC, float pAC, float eAC,
                         float vDC, float pBat, String jam, String tgl, String uptime) {
    
    if (displayMode == 0) { // TAMPILAN 1: DC
        printLine(0, "DC VOLT: %.2fV", vDC);
        printLine(1, "BATTERY: %.1f%%", pBat);
    } 
    else if (displayMode == 1) { // TAMPILAN 2: AC
        printLine(0, "V:%.1f A:%.3f", vAC, cAC);
        printLine(1, "W:%.1f kWh:%.2f", pAC, eAC);
    } 
    else if (displayMode == 2) { // TAMPILAN 3: JAM & TANGGAL
        printLine(0, "TIME:   %s", jam.c_str());
        printLine(1, "DATE: %s", tgl.c_str());
    }
    else if (displayMode == 3) { // TAMPILAN 4: RELAY CONTROL
        printLine(0, "TERMINAL 1: %s", statusR1 ? "ON" : "OFF");
        printLine(1, "TERMINAL 2: %s", statusR2 ? "ON" : "OFF");
    }
    else if (displayMode == 4) { // TAMPILAN 5: SYSTEM STATUS (UPTIME)
        printLine(0, "SYSTEM STATUS");
        printLine(1, "UPTM: %s", uptime.c_str());
    }
}

// --- Tampilan Menu: Setting Mode ---
inline void tampilkanMenu(int index) {
    printLine(0, "--- SETTINGS ---");
    
    if (index == 1) {
        printLine(1, "> RESET ENERGY  ");
    } else if (index == 2) {
        printLine(1, "> CONFIG WIFI   ");
    } else if (index == 3) {
        printLine(1, "> EXIT MENU     ");
    }
}

// --- Tampilan Intro: Startup ---
inline void tampilkanIntroLCD(const char* pesan) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PMS V1.1 BY AME");
    lcd.setCursor(0, 1);
    lcd.print(pesan);
}

#endif