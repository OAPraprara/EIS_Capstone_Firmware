/*
 * oled_ui.c
 *
 *  Created on: Apr 2, 2026
 *      Author: Ozome
 */

#include "oled_ui.h"
#include "i2c.h"
#include <stdio.h> // For sprintf

// Standard 5x8 pixel font (ASCII 32 to 126)
// To keep this file clean, I've compressed the font array to the essentials.
static const uint8_t Font5x8[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00}, {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14},
    {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62}, {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00}, {0x08,0x2A,0x1C,0x2A,0x08}, {0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02},
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39}, {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}, {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00},
    {0x00,0x08,0x14,0x22,0x41}, {0x14,0x14,0x14,0x14,0x14}, {0x41,0x22,0x14,0x08,0x00}, {0x02,0x01,0x51,0x09,0x06},
    {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x01,0x01}, {0x3E,0x41,0x41,0x51,0x32},
    {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x04,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63}, {0x03,0x04,0x78,0x04,0x03}, {0x61,0x51,0x49,0x45,0x43}, {0x00,0x00,0x7F,0x41,0x41},
    {0x02,0x04,0x08,0x10,0x20}, {0x41,0x41,0x7F,0x00,0x00}, {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40},
    {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78}, {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20},
    {0x38,0x44,0x44,0x48,0x7F}, {0x38,0x54,0x54,0x54,0x18}, {0x08,0x7E,0x09,0x01,0x02}, {0x08,0x14,0x54,0x54,0x3C},
    {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00}, {0x20,0x40,0x44,0x3D,0x00}, {0x00,0x7F,0x10,0x28,0x44},
    {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x18,0x04,0x78}, {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38},
    {0x7C,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x18,0x7C}, {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x40,0x20,0x7C}, {0x1C,0x20,0x40,0x20,0x1C}, {0x3C,0x40,0x30,0x40,0x3C},
    {0x44,0x28,0x10,0x28,0x44}, {0x0C,0x50,0x50,0x50,0x3C}, {0x44,0x64,0x54,0x4C,0x44}, {0x00,0x08,0x36,0x41,0x00},
    {0x00,0x00,0x7F,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00}, {0x08,0x08,0x2A,0x1C,0x08}, {0x08,0x1C,0x2A,0x08,0x08}
};

void OLED_Init(void) {
    // Standard SH1106 Initialization (Assuming I2C is already init)
    I2C1_WriteCommand(0x78, 0xAE); // Display OFF
    I2C1_WriteCommand(0x78, 0x8D); // Charge pump
    I2C1_WriteCommand(0x78, 0x14); // Enable
    I2C1_WriteCommand(0x78, 0x20); // Addressing mode
    I2C1_WriteCommand(0x78, 0x00); // Page addressing
    // Set normal color (not inverted)
    I2C1_WriteCommand(0x78, 0xA6);
    I2C1_WriteCommand(0x78, 0xAF); // Display ON
}

void OLED_SetCursor(uint8_t x, uint8_t page) {
    // SH1106 Quirk: Shift X right by 2 pixels so it fits on the physical glass
    x += 2;

    I2C1_WriteCommand(0x78, 0xB0 | (page & 0x07)); // Set Page (Row 0-7)
    I2C1_WriteCommand(0x78, 0x00 | (x & 0x0F));    // Set Lower Column
    I2C1_WriteCommand(0x78, 0x10 | (x >> 4));      // Set Upper Column
}

void OLED_Clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        OLED_SetCursor(0, page);
        for (uint8_t col = 0; col < 128; col++) {
            // Write "0" to clear the pixel.
            // 0x40 is the Data Mode prefix for I2C OLEDs.
            while(I2C1->SR2 & I2C_SR2_BUSY);
            I2C1->CR1 |= I2C_CR1_START;
            while(!(I2C1->SR1 & I2C_SR1_SB));
            I2C1->DR = 0x78;
            while(!(I2C1->SR1 & I2C_SR1_ADDR)); (void)I2C1->SR1; (void)I2C1->SR2;
            I2C1->DR = 0x40; // Data mode!
            while(!(I2C1->SR1 & I2C_SR1_TXE));
            I2C1->DR = 0x00; // Blank pixel
            while(!(I2C1->SR1 & I2C_SR1_TXE));
            while(!(I2C1->SR1 & I2C_SR1_BTF));
            I2C1->CR1 |= I2C_CR1_STOP;
        }
    }
}

void OLED_PrintChar(char c) {
    if (c < 32 || c > 126) c = 32; // Default to space if unknown character

    while(I2C1->SR2 & I2C_SR2_BUSY);
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = 0x78;
    while(!(I2C1->SR1 & I2C_SR1_ADDR)); (void)I2C1->SR1; (void)I2C1->SR2;

    I2C1->DR = 0x40; // Data mode
    while(!(I2C1->SR1 & I2C_SR1_TXE));

    for (uint8_t i = 0; i < 5; i++) {
        I2C1->DR = Font5x8[c - 32][i]; // Send the 5 columns of the letter
        while(!(I2C1->SR1 & I2C_SR1_TXE));
    }
    I2C1->DR = 0x00; // 1 pixel gap between letters
    while(!(I2C1->SR1 & I2C_SR1_TXE));
    while(!(I2C1->SR1 & I2C_SR1_BTF));
    I2C1->CR1 |= I2C_CR1_STOP;
}

void OLED_PrintString(char* str) {
    while (*str) {
        OLED_PrintChar(*str++);
    }
}

//void OLED_UpdateEIS(float v_rest, float v_load, float i_load, float impedance) {
//    char buffer[20];
//
//    // Convert floats to strings safely without requiring IDE Linker Float Flags!
//    int v_whole = (int)v_rest;
//    int v_frac = (int)((v_rest - v_whole) * 100); // 2 decimal places
//
//    int i_whole = (int)i_load;
//    int i_frac = (int)((i_load - i_whole) * 100);
//
//    int z_whole = (int)impedance;
//    int z_frac = (int)((impedance - z_whole) * 1000); // 3 decimal places for Ohms
//
//    OLED_SetCursor(0, 0);
//    OLED_PrintString("=== EIS MACHINE ===");
//
//    OLED_SetCursor(0, 2);
//    sprintf(buffer, "V_bat: %d.%02d V  ", v_whole, v_frac);
//    OLED_PrintString(buffer);
//
//    OLED_SetCursor(0, 4);
//    sprintf(buffer, "I_out: %d.%02d A  ", i_whole, i_frac);
//    OLED_PrintString(buffer);
//
//    OLED_SetCursor(0, 6);
//    sprintf(buffer, "Z: %d.%03d Ohms  ", z_whole, z_frac);
//    OLED_PrintString(buffer);
//}


// --- THE CUSTOM BATTERY ICON ---
// A 16x8 pixel drawing of a battery shell
static const uint8_t BatteryIcon[16] = {
    0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x81, 0xFF, 0x3C, 0x3C
};

void OLED_DrawBatteryIcon(uint8_t x, uint8_t page) {
    OLED_SetCursor(x, page);

    // Switch to Data Mode
    while(I2C1->SR2 & I2C_SR2_BUSY);
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = 0x78;
    while(!(I2C1->SR1 & I2C_SR1_ADDR)); (void)I2C1->SR1; (void)I2C1->SR2;

    I2C1->DR = 0x40; // Data Mode
    while(!(I2C1->SR1 & I2C_SR1_TXE));

    // Draw the 16 columns of the battery
    for (uint8_t i = 0; i < 16; i++) {
        I2C1->DR = BatteryIcon[i];
        while(!(I2C1->SR1 & I2C_SR1_TXE));
    }

    while(!(I2C1->SR1 & I2C_SR1_BTF));
    I2C1->CR1 |= I2C_CR1_STOP;
}

void OLED_UpdateStatus(char* status) {
    OLED_SetCursor(0, 1); // Row 1 is dedicated to the status message
    OLED_PrintString(status);
}

void OLED_UpdateEIS(float v_rest, float v_load, float i_load, float impedance) {
    char buffer[20];

    int v_whole = (int)v_rest;
    int v_frac = (int)((v_rest - v_whole) * 100);

    int i_whole = (int)i_load;
    int i_frac = (int)((i_load - i_whole) * 100);

    int z_whole = (int)impedance;
    int z_frac = (int)((impedance - z_whole) * 1000);

    // Row 0: Header and Battery Icon
    OLED_SetCursor(0, 0);
    OLED_PrintString("EIS MACHINE ");
    OLED_DrawBatteryIcon(100, 0); // Draw battery at the far right

    // Row 3, 5, 7: Data
    OLED_SetCursor(0, 3);
    sprintf(buffer, "V_bat: %d.%02d V  ", v_whole, v_frac);
    OLED_PrintString(buffer);

    OLED_SetCursor(0, 5);
    sprintf(buffer, "I_out: %d.%02d A  ", i_whole, i_frac);
    OLED_PrintString(buffer);

    OLED_SetCursor(0, 7);
    sprintf(buffer, "Z: %d.%03d Ohms  ", z_whole, z_frac);
    OLED_PrintString(buffer);
}

void OLED_DrawMenu(uint8_t cursor_pos) {
    OLED_Clear();
    OLED_SetCursor(0, 0);
    OLED_PrintString("=== SELECT MODE ===");

    // Option 0: DCIR Test
    OLED_SetCursor(0, 3);
    if (cursor_pos == 0) OLED_PrintString(" > 1. DCIR Test");
    else                 OLED_PrintString("   1. DCIR Test");

    // Option 1: Nyquist Sweep
    OLED_SetCursor(0, 5);
    if (cursor_pos == 1) OLED_PrintString(" > 2. Nyquist Sweep");
    else                 OLED_PrintString("   2. Nyquist Sweep");
}


// --- THE FRAMEBUFFER ---
// 128 columns * 64 rows = 8192 pixels. 8192 / 8 bits = 1024 bytes.
static uint8_t OLED_Buffer[1024];

void OLED_ClearBuffer(void) {
    for (int i = 0; i < 1024; i++) {
        OLED_Buffer[i] = 0x00;
    }
}

// Blasts the entire RAM array to the physical screen
void OLED_UpdateScreen(void) {
    for (uint8_t page = 0; page < 8; page++) {
        OLED_SetCursor(0, page);

        while(I2C1->SR2 & I2C_SR2_BUSY);
        I2C1->CR1 |= I2C_CR1_START;
        while(!(I2C1->SR1 & I2C_SR1_SB));
        I2C1->DR = 0x78;
        while(!(I2C1->SR1 & I2C_SR1_ADDR)); (void)I2C1->SR1; (void)I2C1->SR2;

        I2C1->DR = 0x40; // Data Mode
        while(!(I2C1->SR1 & I2C_SR1_TXE));

        // Push 128 bytes for this specific row
        for (uint8_t col = 0; col < 128; col++) {
            I2C1->DR = OLED_Buffer[col + (page * 128)];
            while(!(I2C1->SR1 & I2C_SR1_TXE));
        }

        while(!(I2C1->SR1 & I2C_SR1_BTF));
        I2C1->CR1 |= I2C_CR1_STOP;
    }
}

// The core geometry function: Turns on one pixel at (X, Y)
void OLED_DrawPixel(uint8_t x, uint8_t y) {
    // Screen bounds check (0-127, 0-63)
    if (x >= 128 || y >= 64) return;

    // Find the correct byte in the array, then use bitwise OR to flip the specific pixel ON
    OLED_Buffer[x + (y / 8) * 128] |= (1 << (y % 8));
}

// Bresenham's Line Algorithm: Draws a straight line between any two points
void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y0 - y1) : (y1 - y0); // Invert Y because OLED Y=0 is the top!
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        OLED_DrawPixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Draws the X and Y axes for the Nyquist plot
void OLED_DrawNyquistGrid(void) {
    OLED_ClearBuffer();

    // Draw Y Axis (-Z'')
    OLED_DrawLine(10, 0, 10, 53);
    // Draw X Axis (Z')
    OLED_DrawLine(10, 53, 127, 53);

    // Axis Tick Marks
    OLED_DrawPixel(9, 10); OLED_DrawPixel(11, 10);
    OLED_DrawPixel(9, 30); OLED_DrawPixel(11, 30);
    OLED_DrawPixel(40, 52); OLED_DrawPixel(40, 54);
    OLED_DrawPixel(80, 52); OLED_DrawPixel(80, 54);
}

void OLED_DrawNyquistResults(float rs, float rct) {
    OLED_Clear();
    OLED_SetCursor(0, 0);
    OLED_PrintString("== NYQUIST DATA ==");

    char buffer[20];

    // Convert floats to strings without FPU compiler flags
    int rs_w = (int)rs;
    int rs_f = (int)((rs - rs_w) * 10);
    OLED_SetCursor(0, 3);
    sprintf(buffer, "Rs:  %d.%d mOhm", rs_w, rs_f);
    OLED_PrintString(buffer);

    int rct_w = (int)rct;
    int rct_f = (int)((rct - rct_w) * 10);
    OLED_SetCursor(0, 5);
    sprintf(buffer, "Rct: %d.%d mOhm", rct_w, rct_f);
    OLED_PrintString(buffer);

    // Navigation Hint
    OLED_SetCursor(0, 7);
    OLED_PrintString("[UP] Graph [SEL] Exit");
}
