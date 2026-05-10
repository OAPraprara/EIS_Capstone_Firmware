/*
 * timer_pwm.c
 *
 *  Created on: Apr 2, 2026
 *      Author: Ozome
 */


#include "timer_pwm.h"

// --- THE VINYL RECORD (Sine Wave Lookup Table) ---
// 256 points. Center is 420 (50% Duty Cycle). Amplitude is 200.
// This means the duty cycle will smoothly swing between 220 and 620.
static const uint16_t Sine_LUT[256] = {
    420, 424, 429, 434, 439, 444, 449, 454, 459, 464, 468, 473, 478, 482, 487, 492,
    496, 501, 505, 509, 514, 518, 522, 526, 530, 534, 538, 542, 546, 549, 553, 556,
    559, 563, 566, 569, 572, 575, 578, 580, 583, 585, 588, 590, 592, 594, 596, 598,
    600, 601, 603, 604, 606, 607, 608, 609, 610, 611, 612, 613, 614, 614, 615, 615,
    615, 615, 615, 614, 614, 613, 612, 611, 610, 609, 608, 607, 606, 604, 603, 601,
    600, 598, 596, 594, 592, 590, 588, 585, 583, 580, 578, 575, 572, 569, 566, 563,
    559, 556, 553, 549, 546, 542, 538, 534, 530, 526, 522, 518, 514, 509, 505, 501,
    496, 492, 487, 482, 478, 473, 468, 464, 459, 454, 449, 444, 439, 434, 429, 424,
    420, 415, 410, 405, 400, 395, 390, 385, 380, 375, 371, 366, 361, 357, 352, 347,
    343, 338, 334, 330, 325, 321, 317, 313, 309, 305, 301, 297, 293, 290, 286, 283,
    280, 276, 273, 270, 267, 264, 261, 259, 256, 254, 251, 249, 247, 245, 243, 241,
    239, 238, 236, 235, 233, 232, 231, 230, 229, 228, 227, 226, 225, 225, 224, 224,
    224, 224, 224, 225, 225, 226, 227, 228, 229, 230, 231, 232, 233, 235, 236, 238,
    239, 241, 243, 245, 247, 249, 251, 254, 256, 259, 261, 264, 267, 270, 273, 276,
    280, 283, 286, 290, 293, 297, 301, 305, 309, 313, 317, 321, 325, 330, 334, 338,
    343, 347, 352, 357, 361, 366, 371, 375, 380, 385, 390, 395, 400, 405, 410, 415
};

// DDS Variables
volatile uint32_t phase_accumulator = 0;
volatile uint32_t tuning_word = 0;
volatile uint8_t  spwm_active = 0;

void MOSFET_Timer1_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER &= ~GPIO_MODER_MODER8;
    GPIOA->MODER |= GPIO_MODER_MODER8_1;
    GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL8;
    GPIOA->AFR[1] |= (1 << 0);

    // PWM Base Frequency: 100 kHz (84MHz / 840)
    TIM1->PSC = 0;
    TIM1->ARR = 840 - 1;

    TIM1->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
    TIM1->CCMR1 |= TIM_CCMR1_OC1PE;
    TIM1->CCER |= TIM_CCER_CC1E;
    TIM1->BDTR |= TIM_BDTR_MOE;

    // --- NEW: ENABLE TIM1 UPDATE INTERRUPT ---
    TIM1->DIER |= TIM_DIER_UIE; // Update Interrupt Enable
    NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
    NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 1); // High priority!
}

// --- THE DDS ENGINE (Runs 100,000 times per second!) ---
void TIM1_UP_TIM10_IRQHandler(void) {
    if (TIM1->SR & TIM_SR_UIF) {
        TIM1->SR &= ~TIM_SR_UIF; // Clear flag

        if (spwm_active) {
            // 1. Advance the needle
            phase_accumulator += tuning_word;

            // 2. Look up the sine wave value based on the top 8 bits (0-255 index)
            uint8_t index = phase_accumulator >> 24;

            // 3. Immediately update the MOSFET duty cycle
            TIM1->CCR1 = Sine_LUT[index];
        }
    }
}

void MOSFET_Timer1_SetDutyCycle(uint16_t duty) {
    TIM1->CCR1 = duty;
}

void MOSFET_Timer1_Start(void) {
    TIM1->CR1 |= TIM_CR1_CEN;
}

void MOSFET_Timer1_Stop(void) {
    TIM1->CR1 &= ~TIM_CR1_CEN;
    TIM1->CCR1 = 0; // Force OFF
}

// --- NEW: START A SPECIFIC SINE WAVE FREQUENCY ---
void SPWM_Start_Frequency(float target_hz) {
    // Formula: Tuning Word = (Target_Freq * 2^32) / PWM_Freq
    // PWM_Freq is exactly 100,000 Hz.
    tuning_word = (uint32_t)((target_hz * 4294967296.0f) / 100000.0f);

    phase_accumulator = 0; // Reset wave to 0 degrees
    spwm_active = 1;
    MOSFET_Timer1_Start();
}

void SPWM_Stop(void) {
    spwm_active = 0;
    MOSFET_Timer1_Stop();
}
