/*
 * timer_pwm.c
 *
 *  Created on: Apr 2, 2026
 *      Author: Ozome
 */


#include "timer_pwm.h"

void MOSFET_Timer1_Init(void) {
    // 1. Enable Clocks for GPIOA and Timer 1
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;        // Enable GPIOA clock (1 << 0)
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;         // Enable TIM1 clock (84 MHz bus) (1 << 0)

    // 2. Configure PA8 for Alternate Function (Timer 1 Channel 1)
    GPIOA->MODER &= ~GPIO_MODER_MODER8;         // Clear bits 17:16
    GPIOA->MODER |= GPIO_MODER_MODER8_1;        // Set Alternate Function mode (2 << 16)

    // Set PA8 to High Speed to keep the square wave edges crisp
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR8;   // Set High Speed (3 << 16)

    // Select AF1 (TIM1_CH1) for pin PA8
    GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL8;         // Clear AF bits for PA8
    GPIOA->AFR[1] |= (1 << 0);                  // Set AF1 (1 << 0)

    // 3. Configure Timer 1 for 100 Hz PWM
    // The timer clock is 84 MHz.
    // Target Frequency = 84,000,000 / ((Prescaler + 1) * (AutoReload + 1))
    TIM1->PSC = 839;                            // Divides 84MHz down to 100,000 Hz ticks
    TIM1->ARR = 999;                            // Counts to 1000 ticks = 100 Hz wave

    // 4. Set Duty Cycle to 50%
    TIM1->CCR1 = 500;                           // 500 is exactly half of 1000

    // --- THE SNIPER TRIGGER (FIXED) ---
	TIM1->CCR2 = 250;                           // Set trigger exactly at tick 250
	TIM1->CCMR1 &= ~TIM_CCMR1_OC2M;             // Clear Channel 2 mode bits (bits 14:12)
	TIM1->CCMR1 |= (7 << 12);                   // Set Channel 2 to PWM Mode 2 (111) to create a Rising Edge

	// --- ADD THIS LINE: Un-mute the Channel 2 hardware event ---
	TIM1->CCER |= TIM_CCER_CC2E;

	// 5. Configure PWM Mode 1 on Channel 1
    TIM1->CCMR1 &= ~TIM_CCMR1_CC1S;             // Set channel as output
    TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;             // Clear output compare mode bits
    TIM1->CCMR1 |= (6 << 4);                    // Set PWM Mode 1 (110 in bits 6:4)
    TIM1->CCMR1 |= TIM_CCMR1_OC1PE;             // Enable Preload register

    // 6. Enable the Output on Channel 1
    TIM1->CCER |= TIM_CCER_CC1E;                // Enable Capture/Compare 1 output

    // 7. Advanced Timer Safety: Main Output Enable (MOE)
    // TIM1 is an "Advanced" timer, so it requires this extra safety bit to output anything.
    TIM1->BDTR |= TIM_BDTR_MOE;
}

void MOSFET_Timer1_Start(void) {
    TIM1->CR1 |= TIM_CR1_CEN;                   // Counter Enable
}

void MOSFET_Timer1_Stop(void) {
    TIM1->CR1 &= ~TIM_CR1_CEN;                  // Counter Disable
    TIM1->CNT = 0;                              // Reset counter to 0 for the next test
    GPIOA->BSRR = GPIO_BSRR_BR8;                // Force PA8 completely LOW (0V) for safety
}

void MOSFET_Timer1_SetDutyCycle(uint16_t duty) {
    TIM1->CCR1 = duty;  // Update the Compare register to change the ON time
}
