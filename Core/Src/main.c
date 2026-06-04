#include "stm32f4xx.h"
#include "rcc_config.h"
#include "timer_pwm.h"
#include "adc_dma.h"
#include "dsp_math.h"
#include "i2c.h"
#include "oled_ui.h"
#include "buttons.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

volatile uint32_t msTicks = 0;

// --- STATE MACHINE VARIABLES ---
typedef enum {
    STATE_MENU,
    STATE_SQUARE_SETUP_LOCKED, // NEW: Replaced single setup state with Locked
    STATE_SQUARE_SETUP_EDIT,   // NEW: Edit state for blinking digits
    STATE_SQUARE_RUN,
    STATE_NYQUIST
} SystemState_t;

volatile SystemState_t current_state = STATE_MENU;
volatile uint8_t cursor_pos = 0;
volatile uint8_t nyquist_view = 0;
volatile uint8_t ui_needs_update = 1;
volatile uint32_t last_button_press = 0;

// --- SQUARE WAVE USER VARIABLES ---
volatile uint8_t sq_freq_digits[4] = {0, 1, 0, 0};
volatile uint8_t sq_cursor = 0;
volatile uint8_t setup_menu_cursor = 0; // 0 = Run Test, 1 = Back
volatile uint32_t select_press_time = 0;
volatile uint8_t  select_is_pressed = 0;
volatile uint8_t  select_handled = 0; // NEW: Prevents double-triggers on release

// --- BLINK TIMER VARIABLES ---
uint32_t last_blink_time = 0;
uint8_t cursor_visible = 1;

// --- THE SELECT BUTTON HANDLER (PB8) ---
void EXTI9_5_IRQHandler(void) {
    uint32_t pr = EXTI->PR;
    EXTI->PR = EXTI_PR_PR8;

    if (pr & EXTI_PR_PR8) {
        if ((GPIOB->IDR & GPIO_IDR_ID8) == 0) {
            // FALLING EDGE (Pressed)
            select_press_time = msTicks;
            select_is_pressed = 1;
            select_handled = 0; // Reset the handled flag!
            last_button_press = msTicks;
        } else {
            // RISING EDGE (Released)
            if (select_is_pressed) {
                select_is_pressed = 0;

                // ONLY execute a short press if the Immediate Hold didn't already trigger!
                if (!select_handled) {
                    uint32_t hold_duration = msTicks - select_press_time;
                    last_button_press = msTicks;

                    if (hold_duration > 50) {
                        if (current_state == STATE_MENU) {
                            if (cursor_pos == 0) {
                                current_state = STATE_SQUARE_SETUP_LOCKED;
                                OLED_Clear(); // Wipe the main menu away once
                            }
                            else if (cursor_pos == 1) {
                                current_state = STATE_NYQUIST;
                                nyquist_view = 0;
                            }
                        }
                        else if (current_state == STATE_SQUARE_SETUP_LOCKED) {
                            if (setup_menu_cursor == 0) current_state = STATE_SQUARE_RUN;
                            else current_state = STATE_MENU;
                        }
                        else if (current_state == STATE_SQUARE_SETUP_EDIT) {
                            sq_cursor++;
                            if (sq_cursor > 3) sq_cursor = 0;
                            cursor_visible = 1; // Force solid when you move it
                            last_blink_time = msTicks;
                        }
                        else {
                            current_state = STATE_MENU;
                        }
                        ui_needs_update = 1;
                    }
                }
            }
        }
    }
}

// --- THE UP & DOWN BUTTONS HANDLER (PB12, PB15) ---
void EXTI15_10_IRQHandler(void) {
    uint32_t pr = EXTI->PR;
    EXTI->PR = (EXTI_PR_PR12 | EXTI_PR_PR15);

    if (select_is_pressed) return;

    if ((msTicks - last_button_press) > 200) {
        if (pr & EXTI_PR_PR12) {
            // --- UP BUTTON ---
            if (current_state == STATE_MENU) {
                if (cursor_pos > 0) cursor_pos--;
            }
            else if (current_state == STATE_SQUARE_SETUP_LOCKED) {
                if (setup_menu_cursor > 0) setup_menu_cursor--; // Move to Run Test
            }
            else if (current_state == STATE_SQUARE_SETUP_EDIT) {
                sq_freq_digits[sq_cursor]++;
                if (sq_freq_digits[sq_cursor] > 9) sq_freq_digits[sq_cursor] = 0;
                cursor_visible = 1;
                last_blink_time = msTicks;
            }
            else if (current_state == STATE_NYQUIST) {
                nyquist_view = 1;
            }
            ui_needs_update = 1;
            last_button_press = msTicks;
        }
        else if (pr & EXTI_PR_PR15) {
            // --- DOWN BUTTON ---
            if (current_state == STATE_MENU) {
                if (cursor_pos < 1) cursor_pos++;
            }
            else if (current_state == STATE_SQUARE_SETUP_LOCKED) {
                if (setup_menu_cursor < 1) setup_menu_cursor++; // Move to Back
            }
            else if (current_state == STATE_SQUARE_SETUP_EDIT) {
                if (sq_freq_digits[sq_cursor] == 0) sq_freq_digits[sq_cursor] = 9;
                else sq_freq_digits[sq_cursor]--;
                cursor_visible = 1;
                last_blink_time = msTicks;
            }
            else if (current_state == STATE_NYQUIST) {
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

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER &= ~GPIO_MODER_MODER13;
    GPIOC->MODER |= GPIO_MODER_MODER13_0;

    SysTick_Init();
    MOSFET_Timer1_Init();
    ADC_DMA_Init();
    I2C1_Init();
    Buttons_Init();

    delay_ms(100);
    OLED_Init();

    while(1) {
        // --- 500ms BLINK LOGIC ---
    	// --- NEW FIX: 1.5 SECOND IMMEDIATE HOLD DETECTION ---
		// We actively watch the clock while the button is physically held down
		if (select_is_pressed && !select_handled) {
			if ((msTicks - select_press_time) > 1500) {
				if (current_state == STATE_SQUARE_SETUP_LOCKED) {
					current_state = STATE_SQUARE_SETUP_EDIT;
					sq_cursor = 0;
					OLED_Clear(); // Clean wipe when entering edit mode
				} else if (current_state == STATE_SQUARE_SETUP_EDIT) {
					current_state = STATE_SQUARE_SETUP_LOCKED;
					OLED_Clear(); // Clean wipe when saving
				}
				select_handled = 1; // Mark as handled so releasing the button does nothing!
				ui_needs_update = 1;
			}
		}

		// --- 500ms BLINK LOGIC ---
		if (current_state == STATE_SQUARE_SETUP_EDIT) {
			if ((msTicks - last_blink_time) > 500) {
				cursor_visible = !cursor_visible;
				last_blink_time = msTicks;
				ui_needs_update = 1;
			}
		} else {
			cursor_visible = 1; // Always solid when not editing
		}

        if (!ui_needs_update) {
            __WFI();
            continue;
        }

        ui_needs_update = 0;

        switch (current_state) {

            case STATE_MENU:
                OLED_DrawMenu(cursor_pos);
                break;

            case STATE_SQUARE_SETUP_LOCKED:
                // Pass flags: is_edit_mode = 0, show_digit_cursor = 0
                OLED_DrawFreqSetup((uint8_t*)sq_freq_digits, sq_cursor, 0, setup_menu_cursor, 0);
                break;

            case STATE_SQUARE_SETUP_EDIT:
                // Pass flags: is_edit_mode = 1, show_digit_cursor = controlled by timer
                OLED_DrawFreqSetup((uint8_t*)sq_freq_digits, sq_cursor, 1, setup_menu_cursor, cursor_visible);
                break;

            case STATE_SQUARE_RUN: {
				OLED_Clear();
				OLED_SetCursor(0, 0);
				OLED_PrintString("Executing Test...");

				float base_freq = (float)(
					(sq_freq_digits[0] * 1000) +
					(sq_freq_digits[1] * 100) +
					(sq_freq_digits[2] * 10) +
					(sq_freq_digits[3])
				);

				if (base_freq < 10.0f) base_freq = 10.0f; // Minimum 10Hz to fit in RAM

				// --- PREPARE THE HARDWARE ---
				NVIC_DisableIRQ(TIM1_UP_TIM10_IRQn);
				MOSFET_Timer1_Start();
				ADC1->CR2 &= ~ADC_CR2_EXTEN;

				// --- THE "ANTI-FREEZE" CLEAN SLATE ---
				ADC1->SR = 0;
				DMA2->LIFCR |= 0x3D;

				float sample_rate = 100000.0f;

				// --- THE HARDWARE WARM-UP PHASE ---
				int warmup_samples = (int)(sample_rate * 0.3f); // 200 milliseconds of ticks
				int half_period_ticks = (int)(sample_rate / base_freq / 2.0f);
				int tick_counter = 0;
				int mosfet_on = 0;

				for (int w = 0; w < warmup_samples; w++) {
					if (tick_counter >= half_period_ticks) {
						mosfet_on = !mosfet_on;
						if (mosfet_on) TIM1->CCR1 = TIM1->ARR;
						else           TIM1->CCR1 = 0;
						tick_counter = 0;
					}
					tick_counter++;

					// Wait for the timer tick, but DO NOT trigger the ADC or DMA!
					TIM1->SR &= ~TIM_SR_UIF;
					while(!(TIM1->SR & TIM_SR_UIF));
				}

				// --- RESET FOR THE ACTUAL RECORDING ---
				// Calculate exact number of samples needed for full, perfect waves
				#define MAX_SAMPLES 2000
				float sq_v_samples[MAX_SAMPLES];
				float sq_i_samples[MAX_SAMPLES];

				int samples_per_wave = (int)(sample_rate / base_freq);
				int num_waves = MAX_SAMPLES / samples_per_wave;
				if (num_waves < 1) num_waves = 1;
				int exact_samples = num_waves * samples_per_wave;
				if (exact_samples > MAX_SAMPLES) exact_samples = MAX_SAMPLES;

				// COMPILER FIX: Removed the "int" keyword here so we don't redefine them!
				half_period_ticks = samples_per_wave / 2;
				tick_counter = 0;
				mosfet_on = 0;

				int timeout_counter = 0; // Added for the Anti-Freeze logic

				// 1. CAPTURE EXACTLY 'exact_samples' OF DATA
				for (int n = 0; n < exact_samples; n++) {

					if (tick_counter >= half_period_ticks) {
						mosfet_on = !mosfet_on;
						if (mosfet_on) TIM1->CCR1 = TIM1->ARR;
						else           TIM1->CCR1 = 0;
						tick_counter = 0;
					}
					tick_counter++;

					// --- TIMEOUT FIX 1: THE TIMER LOOP ---
					TIM1->SR &= ~TIM_SR_UIF;
					timeout_counter = 20000;
					while(!(TIM1->SR & TIM_SR_UIF) && --timeout_counter);
					if (timeout_counter == 0) break;

					// --- TIMEOUT FIX 2: THE KELVIN-GRADE DMA LOOP ---
					ADC1->CR2 |= ADC_CR2_SWSTART;

					timeout_counter = 20000;
					while(!(DMA2->LISR & DMA_LISR_TCIF0) && --timeout_counter) {
						if (ADC1->SR & ADC_SR_OVR) {
							ADC1->SR &= ~ADC_SR_OVR;
						}
					}
					DMA2->LIFCR |= DMA_LIFCR_CTCIF0;
					if (timeout_counter == 0) break;

					// Safely grab the locked data
					sq_v_samples[n] = ((float)adc_raw[0] * 3.3f) / 4095.0f;
					sq_i_samples[n] = ((float)adc_raw[1] * 3.3f) / 4095.0f;
				}

				// --- CLEAN UP ---
				MOSFET_Timer1_Stop();
				NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
				ADC1->CR2 |= (1 << 28);

				// --- REMOVE THE DC OFFSET! ---
				float v_sum = 0.0f, i_sum = 0.0f;
				for (int n = 0; n < exact_samples; n++) {
					v_sum += sq_v_samples[n];
					i_sum += sq_i_samples[n];
				}
				float v_mean = v_sum / (float)exact_samples;
				float i_mean = i_sum / (float)exact_samples;

				// Subtract the mean to perfectly center the AC waves at 0V
				for (int n = 0; n < exact_samples; n++) {
					sq_v_samples[n] -= v_mean;
					sq_i_samples[n] -= i_mean;
				}

				// 3. Extract the 1st, 3rd, and 5th Harmonics!
				float harmonics[3] = {base_freq, base_freq * 3.0f, base_freq * 5.0f};
				float z_rs[3], z_rct[3];

				for (int h = 0; h < 3; h++) {
					float v_real, v_imag, i_real, i_imag;
					Goertzel_Filter(sq_v_samples, exact_samples, harmonics[h], sample_rate, &v_real, &v_imag);
					Goertzel_Filter(sq_i_samples, exact_samples, harmonics[h], sample_rate, &i_real, &i_imag);

					// --- APPLY HARDWARE CALIBRATION ---
					// 1. Voltage AFE: 21x Gain Op-Amp
					v_real = v_real / 21.0f;
					v_imag = v_imag / 21.0f;

					// 2. Current AFE: 50x Gain INA240 with 0.05 Ohm Shunt (Vout = I * 2.5)
					i_real = i_real / 2.5f;
					i_imag = i_imag / 2.5f;

					// 3. Phase Correction:
					i_real = -i_real;
					i_imag = -i_imag;

					float denominator = (i_real * i_real) + (i_imag * i_imag);
					if (denominator < 0.000001f) denominator = 0.000001f;

					float z_real = ((v_real * i_real) + (v_imag * i_imag)) / denominator;
					float z_imag = ((v_imag * i_real) - (v_real * i_imag)) / denominator;

					z_rs[h]  = z_real * 1000.0f; // Convert Ohms to milliOhms
					z_rct[h] = -z_imag * 1000.0f;
				}

				if (current_state == STATE_SQUARE_RUN) {
					OLED_DrawSquareResults(z_rs[0], z_rct[0], z_rs[1], z_rct[1], z_rs[2], z_rct[2]);
				}
				break;
			} // End STATE_SQUARE_RUN

            case STATE_NYQUIST: {
                if (nyquist_view == 0) {
                    OLED_DrawNyquistGrid();
                    OLED_UpdateScreen();

                } else {
                    OLED_Clear();
                    OLED_SetCursor(0, 0);
                    OLED_PrintString("Running Sweep...");

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

                    if (current_state == STATE_NYQUIST && nyquist_view == 1) {
                        OLED_DrawNyquistResults(calc_rs, calc_rct);
                    }
                }
                break;
            } // End STATE_NYQUIST
        }
    }
}
