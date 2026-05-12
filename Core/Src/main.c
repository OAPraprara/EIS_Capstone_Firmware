#include "stm32f4xx.h"
#include "rcc_config.h"
#include "timer_pwm.h"
#include "adc_dma.h"
#include "dsp_math.h"
#include "i2c.h"
#include "oled_ui.h"
#include "buttons.h" // NEW
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

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

// --- THE SELECT BUTTON HANDLER (PB8) ---
void EXTI9_5_IRQHandler(void) {
    uint32_t pr = EXTI->PR;
    EXTI->PR = EXTI_PR_PR8; // Clear flag immediately!

    if ((msTicks - last_button_press) > 300) {
        if (pr & EXTI_PR_PR8) {
            if (current_state == STATE_MENU) {
                if (cursor_pos == 0) current_state = STATE_DCIR;
                if (cursor_pos == 1) {
                    current_state = STATE_NYQUIST;
                    nyquist_view = 0;
                }
            } else {
                current_state = STATE_MENU; // Exit current test
            }
            ui_needs_update = 1;
            last_button_press = msTicks;
        }
    }
}

// --- THE UP & DOWN BUTTONS HANDLER (PB12, PB15) ---
void EXTI15_10_IRQHandler(void) {
    uint32_t pr = EXTI->PR;
    EXTI->PR = (EXTI_PR_PR12 | EXTI_PR_PR15); // Clear flags immediately!

    if ((msTicks - last_button_press) > 300) {
        if (pr & EXTI_PR_PR12) {
            // --- UP BUTTON (PB12) ---
            if (current_state == STATE_MENU) {
                if (cursor_pos > 0) cursor_pos--;
            } else if (current_state == STATE_NYQUIST) {
                nyquist_view = 1;
            }
            ui_needs_update = 1;
            last_button_press = msTicks;
        }
        else if (pr & EXTI_PR_PR15) {
            // --- DOWN BUTTON (PB15) ---
            if (current_state == STATE_MENU) {
                if (cursor_pos < 1) cursor_pos++;
            } else if (current_state == STATE_NYQUIST) {
                nyquist_view = 0;
            }
            ui_needs_update = 1;
            last_button_press = msTicks;
        }
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

    // --- NEW: INITIALIZE THE BLUE LED (PC13) ---
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	GPIOC->MODER &= ~GPIO_MODER_MODER13;
	GPIOC->MODER |= GPIO_MODER_MODER13_0;
	// ------------------------------------------

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

		// IMPORTANT: Clear the flag *BEFORE* we draw!
		// If a button is pressed while the screen is drawing or doing math,
		// the interrupt will safely set this back to 1 and it will loop again.
		ui_needs_update = 0;

		switch (current_state) {

			case STATE_MENU:
				OLED_ClearBuffer();
				OLED_DrawMenu(cursor_pos);
				break;

			case STATE_DCIR:
				OLED_ClearBuffer();
				OLED_UpdateStatus("Status: [ PULSING ]");
				OLED_UpdateScreen();
				break;

			case STATE_NYQUIST:
				if (nyquist_view == 0) {
					// --- PAGE 0: THE PLOT GRAPH ---
					OLED_ClearBuffer();
					OLED_DrawNyquistResults(25.0f, 15.0f);
					OLED_UpdateScreen(); // <-- ADDED: This fixes the blank screen!

				} else {
					// --- PAGE 1: DSP RESULTS ---
					OLED_ClearBuffer();
					OLED_SetCursor(0, 0);
					OLED_PrintString("DSP Sim Running...");
					OLED_UpdateScreen();

					// --- SOFTWARE-IN-THE-LOOP DSP SIMULATION ---
					#define NUM_SAMPLES 512
					float v_samples[NUM_SAMPLES];
					float i_samples[NUM_SAMPLES];

					float sim_freq = 1000.0f;
					float sample_rate = 50000.0f;

					for (int n = 0; n < NUM_SAMPLES; n++) {
						float time = (float)n / sample_rate;
						float clean_v = 0.015f * sinf(2.0f * 3.14159f * sim_freq * time);
						float clean_i = 1.0f * sinf(2.0f * 3.14159f * sim_freq * time - 0.785f);
						float noise = ((float)(rand() % 400) / 100000.0f) - 0.002f;

						v_samples[n] = clean_v + noise;
						i_samples[n] = clean_i + noise;
					}

					float v_real, v_imag, i_real, i_imag;
					Goertzel_Filter(v_samples, NUM_SAMPLES, sim_freq, sample_rate, &v_real, &v_imag);
					Goertzel_Filter(i_samples, NUM_SAMPLES, sim_freq, sample_rate, &i_real, &i_imag);

					float denominator = (i_real * i_real) + (i_imag * i_imag);
					float z_real = ((v_real * i_real) + (v_imag * i_imag)) / denominator;
					float z_imag = ((v_imag * i_real) - (v_real * i_imag)) / denominator;

					float calc_rs = z_real * 1000.0f;
					float calc_rct = -z_imag * 1000.0f;

					// Ensure the user hasn't pressed SELECT to exit while the math was running!
					if (current_state == STATE_NYQUIST && nyquist_view == 1) {
						OLED_ClearBuffer();
						OLED_SetCursor(0, 3);
						OLED_PrintString("DSP Calculation:");

						char buf[20];
						sprintf(buf, "Z':  %d mOhm", (int)calc_rs);
						OLED_SetCursor(0, 5);
						OLED_PrintString(buf);

						sprintf(buf, "-Z'': %d mOhm", (int)calc_rct);
						OLED_SetCursor(0, 7);
						OLED_PrintString(buf);

						OLED_UpdateScreen();
					}
				}
				break;
			}
		}
}
