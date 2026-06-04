/*
 * oled_ui.h
 *
 *  Created on: Apr 2, 2026
 *      Author: Ozome
 */

#ifndef OLED_UI_H
#define OLED_UI_H

#include "stm32f4xx.h"

void OLED_Init(void);
void OLED_Clear(void);
void OLED_SetCursor(uint8_t x, uint8_t page);
void OLED_PrintString(char* str);

// --- NEW FRAMEBUFFER GRAPHICS ---
void OLED_ClearBuffer(void);
void OLED_UpdateScreen(void);
void OLED_DrawPixel(uint8_t x, uint8_t y);
void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void OLED_DrawNyquistGrid(void);
void OLED_DrawMenu(uint8_t cursor_pos);

// (Keep your existing UpdateStatus and UpdateEIS functions here)
void OLED_UpdateStatus(char* status);
void OLED_UpdateEIS(float v_rest, float v_load, float i_load, float impedance);

void OLED_DrawNyquistResults(float rs, float rct);
void OLED_DrawFreqSetup(uint8_t* digits, uint8_t digit_cursor, uint8_t is_edit_mode, uint8_t menu_cursor, uint8_t show_digit_cursor);
void OLED_DrawSquareResults(float z1_rs, float z1_rct, float z3_rs, float z3_rct, float z5_rs, float z5_rct);

#endif /* OLED_UI_H */
