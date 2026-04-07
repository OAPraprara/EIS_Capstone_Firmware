/*
 * buttons.c
 *
 *  Created on: Apr 6, 2026
 *      Author: Ozome
 */


#include "buttons.h"

void Buttons_Init(void) {
    // 1. Enable Clocks for GPIOB and SYSCFG (System Configuration Controller)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Required to route pins to EXTI lines

    // 2. Configure PB12, PB13, PB14 as Inputs
    GPIOB->MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER14);

    // 3. Enable Internal Pull-Up Resistors (01 in PUPDR)
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD12 | GPIO_PUPDR_PUPD13 | GPIO_PUPDR_PUPD14);
    GPIOB->PUPDR |=  (GPIO_PUPDR_PUPD12_0 | GPIO_PUPDR_PUPD13_0 | GPIO_PUPDR_PUPD14_0);

    // 4. Route PB12, PB13, PB14 to the EXTI lines using SYSCFG
    // SYSCFG_EXTICR4 controls lines 12 to 15. Port B is code '0001' (1).
    SYSCFG->EXTICR[3] &= ~(SYSCFG_EXTICR4_EXTI12 | SYSCFG_EXTICR4_EXTI13 | SYSCFG_EXTICR4_EXTI14);
    SYSCFG->EXTICR[3] |=  (SYSCFG_EXTICR4_EXTI12_PB | SYSCFG_EXTICR4_EXTI13_PB | SYSCFG_EXTICR4_EXTI14_PB);

    // 5. Unmask (Enable) EXTI lines 12, 13, and 14
    EXTI->IMR |= (EXTI_IMR_IM12 | EXTI_IMR_IM13 | EXTI_IMR_IM14);

    // 6. Set Trigger to Falling Edge (Button goes DOWN to Ground)
    EXTI->FTSR |= (EXTI_FTSR_TR12 | EXTI_FTSR_TR13 | EXTI_FTSR_TR14);

    // 7. Enable EXTI15_10 Interrupt in the NVIC (Nested Vectored Interrupt Controller)
    // Lines 10 through 15 all share a single interrupt channel on the STM32
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_SetPriority(EXTI15_10_IRQn, 2); // Set priority lower than SysTick
}
