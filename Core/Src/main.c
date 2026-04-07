#include "stm32f4xx.h"
#include "rcc_config.h"
#include "timer_pwm.h"
#include "adc_dma.h"
#include "dsp_math.h"
#include "i2c.h"
#include "oled_ui.h"
#include "buttons.h" // NEW
#include <math.h>

volatile uint32_t msTicks = 0;

// --- STATE MACHINE VARIABLES ---
typedef enum {
    STATE_MENU,
    STATE_DCIR,
    STATE_NYQUIST
} SystemState_t;

volatile SystemState_t current_state = STATE_MENU;
volatile uint8_t cursor_pos = 0;
volatile uint8_t nyquist_view = 0;     // NEW: 0 = Plot, 1 = Results
volatile uint8_t ui_needs_update = 1;
volatile uint32_t last_button_press = 0;

// --- THE HARDWARE INTERRUPT HANDLER ---
void EXTI15_10_IRQHandler(void) {
    if ((msTicks - last_button_press) > 150) { // Software Debounce

        // --- UP BUTTON (PB12) ---
        if (EXTI->PR & EXTI_PR_PR12) {
            if (current_state == STATE_MENU) {
                if (cursor_pos > 0) cursor_pos--;
            } else if (current_state == STATE_NYQUIST) {
                nyquist_view = 1; // Flip to the Results Page
            }
            ui_needs_update = 1;
            EXTI->PR |= EXTI_PR_PR12;
        }

        // --- DOWN BUTTON (PB13) ---
        if (EXTI->PR & EXTI_PR_PR13) {
            if (current_state == STATE_MENU) {
                if (cursor_pos < 1) cursor_pos++;
            } else if (current_state == STATE_NYQUIST) {
                nyquist_view = 0; // Flip back to the Plot Page
            }
            ui_needs_update = 1;
            EXTI->PR |= EXTI_PR_PR13;
        }

        // --- SELECT BUTTON (PB14) ---
        if (EXTI->PR & EXTI_PR_PR14) {
            if (current_state == STATE_MENU) {
                if (cursor_pos == 0) current_state = STATE_DCIR;
                if (cursor_pos == 1) {
                    current_state = STATE_NYQUIST;
                    nyquist_view = 0; // Always start on the plot
                }
                OLED_Clear();
            } else {
                current_state = STATE_MENU; // Exit current test
            }
            ui_needs_update = 1;
            EXTI->PR |= EXTI_PR_PR14;
        }
        last_button_press = msTicks;
    } else {
        EXTI->PR |= (EXTI_PR_PR12 | EXTI_PR_PR13 | EXTI_PR_PR14);
    }
}

void SysTick_Init(void) {
    SysTick->LOAD = 84000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

void delay_ms(uint32_t delay) {
    uint32_t startTicks = msTicks;
    while ((msTicks - startTicks) < delay) {
        __WFI();
    }
}

void SysTick_Handler(void) {
    msTicks++;
}

int main(void) {
    SystemClock_Config_84MHz();
    SysTick_Init();
    MOSFET_Timer1_Init();
    ADC_DMA_Init();
    I2C1_Init();
    Buttons_Init();

    delay_ms(100);
    OLED_Init();

    while(1) {
        // If nothing needs updating, sleep the CPU to save power!
        if (!ui_needs_update) {
            __WFI();
            continue;
        }

        switch (current_state) {

            case STATE_MENU:
                OLED_DrawMenu(cursor_pos);
                ui_needs_update = 0;
                break;

            case STATE_DCIR:
                // (Your existing DCIR Pulse test logic)
                OLED_UpdateStatus("Status: [ PULSING ]");
                // ...
                ui_needs_update = 0; // Prevent rapid redraws if paused
                break;

            case STATE_NYQUIST:
                if (nyquist_view == 0) {
                    // --- PAGE 1: THE GRAPH ---
                    OLED_ClearBuffer();

                    // Draw Axes
                    OLED_DrawLine(10, 0, 10, 53);   // Y-Axis
                    OLED_DrawLine(10, 53, 127, 53); // X-Axis

                    // Draw X-Axis Ticks (Scale: 2 pixels = 1 mOhm)
                    OLED_DrawLine(50, 51, 50, 55); // 20 mOhm mark (10 + 20*2)
                    OLED_DrawLine(90, 51, 90, 55); // 40 mOhm mark (10 + 40*2)

                    // Draw Math Simulation (Rs=25, Rct=15)
                    float x_val, y_val;
                    for (float t = 3.14159f; t >= 0; t -= 0.05f) {
                        x_val = 65.0f - (15.0f * cos(t)); // Center=65, Rad=15
                        y_val = 15.0f * sin(t);
                        OLED_DrawPixel(10 + (uint8_t)x_val, 53 - (uint8_t)y_val);
                    }
                    for (float w = 0; w < 22.0f; w += 1.0f) {
                        OLED_DrawPixel(10 + (uint8_t)(80.0f + w), 53 - (uint8_t)w);
                    }

                    OLED_UpdateScreen(); // Blast drawing to screen

                    // Overlay Text Labels (Direct to OLED RAM)
                    OLED_SetCursor(12, 0);  OLED_PrintString("-Z''");
                    OLED_SetCursor(115, 6); OLED_PrintString("Z'");

                    // Align the numbers perfectly under the tick marks on Page 7
                    OLED_SetCursor(44, 7);  OLED_PrintString("20");
                    OLED_SetCursor(84, 7);  OLED_PrintString("40");

                } else {
                    // --- PAGE 2: THE ENUMERATION ---
                    // Simulated exact values based on our math model above
                    OLED_DrawNyquistResults(25.0f, 15.0f);
                }

                ui_needs_update = 0;
                break;
        }
    }
}
