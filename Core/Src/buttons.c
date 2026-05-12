/*
 * buttons.c
 *
 *  Created on: Apr 6, 2026
 *      Author: Ozome
 */


#include "buttons.h"

void Buttons_Init(void) {
    // 1. Enable Clocks for GPIOB and SYSCFG
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // 2. Configure PB8 (Select), PB12 (Up), PB15 (Down) as Inputs
    GPIOB->MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER12 | GPIO_MODER_MODER15);

    // 3. Enable Internal Pull-Up Resistors
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD8 | GPIO_PUPDR_PUPD12 | GPIO_PUPDR_PUPD15);
    GPIOB->PUPDR |=  (GPIO_PUPDR_PUPD8_0 | GPIO_PUPDR_PUPD12_0 | GPIO_PUPDR_PUPD15_0);

    // 4. Route Pins to the EXTI lines using SYSCFG
    // PB8 goes to EXTI8 (which is in EXTICR[2])
    SYSCFG->EXTICR[2] &= ~(SYSCFG_EXTICR3_EXTI8);
    SYSCFG->EXTICR[2] |=  (SYSCFG_EXTICR3_EXTI8_PB);

    // PB12 and PB15 go to EXTI12 and EXTI15 (which are in EXTICR[3])
    SYSCFG->EXTICR[3] &= ~(SYSCFG_EXTICR4_EXTI12 | SYSCFG_EXTICR4_EXTI15);
    SYSCFG->EXTICR[3] |=  (SYSCFG_EXTICR4_EXTI12_PB | SYSCFG_EXTICR4_EXTI15_PB);

    // 5. Unmask (Enable) EXTI lines 8, 12, and 15
    EXTI->IMR |= (EXTI_IMR_IM8 | EXTI_IMR_IM12 | EXTI_IMR_IM15);

    // 6. Set Trigger to Falling Edge
    EXTI->FTSR |= (EXTI_FTSR_TR8 | EXTI_FTSR_TR12 | EXTI_FTSR_TR15);

    // 7. Enable Both Interrupts in the NVIC!
    NVIC_EnableIRQ(EXTI9_5_IRQn);    // For PB8 (Select)
    NVIC_SetPriority(EXTI9_5_IRQn, 2);

    NVIC_EnableIRQ(EXTI15_10_IRQn);  // For PB12 (Up) and PB15 (Down)
    NVIC_SetPriority(EXTI15_10_IRQn, 2);
}
