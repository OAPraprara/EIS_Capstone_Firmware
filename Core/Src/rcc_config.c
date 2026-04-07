/*
 * rcc_config.c
 *
 *  Created on: Apr 2, 2026
 *      Author: Ozome
 */

#include "rcc_config.h"

void SystemClock_Config_84MHz(void) {
    // 1. Enable High-Speed Internal (HSI) clock
    RCC->CR |= RCC_CR_HSION;                       // (1 << 0)
    while (!(RCC->CR & RCC_CR_HSIRDY));            // Wait for HSI ready

    // 2. Enable Power Controller clock
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;             // (1 << 28)

    // 3. Set Voltage Scaling to Scale 2
    PWR->CR &= ~PWR_CR_VOS;                        // Clear bits 15:14 (3 << 14)
    PWR->CR |= PWR_CR_VOS_1;                       // Set bit 15 (2 << 14)

    // 4. Configure Flash Memory Latency
    FLASH->ACR &= ~FLASH_ACR_LATENCY;              // Clear latency bits (15 << 0)
    FLASH->ACR |= FLASH_ACR_LATENCY_2WS;           // Set 2 Wait States (2 << 0)
    FLASH->ACR |= FLASH_ACR_ICEN;                  // Enable Instruction Cache (1 << 9)
    FLASH->ACR |= FLASH_ACR_DCEN;                  // Enable Data Cache (1 << 10)
    FLASH->ACR |= FLASH_ACR_PRFTEN;                // Enable Prefetch Buffer (1 << 8)

    // 5. Configure Main PLL (16MHz / 16 * 336 / 4 = 84MHz)
    RCC->PLLCFGR = 0;                              // Clear register
    RCC->PLLCFGR |= (16 << 0);                     // PLLM = 16
    RCC->PLLCFGR |= (336 << 6);                    // PLLN = 336
    RCC->PLLCFGR |= (1 << 16);                     // PLLP = 4 (01 in bits 17:16, so 1 << 16)
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSI;        // PLLSRC = HSI (0 << 22)

    // 6. Enable Main PLL
    RCC->CR |= RCC_CR_PLLON;                       // (1 << 24)
    while (!(RCC->CR & RCC_CR_PLLRDY));            // Wait for PLL locked

    // 7. Configure APB/AHB Prescalers
    RCC->CFGR &= ~RCC_CFGR_HPRE;                   // AHB = 1 (0 << 4)
    RCC->CFGR &= ~RCC_CFGR_PPRE1;                  // Clear APB1
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;              // APB1 = 2 (Max is 42MHz) (4 << 10)
    RCC->CFGR &= ~RCC_CFGR_PPRE2;                  // APB2 = 1 (Max is 84MHz) (0 << 13)

    // 8. Select PLL as System Clock
    RCC->CFGR &= ~RCC_CFGR_SW;                     // Clear switch bits (3 << 0)
    RCC->CFGR |= RCC_CFGR_SW_PLL;                  // Select PLL (2 << 0)
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL); // Wait for switch completion
}
