/*
 * i2c.c
 *
 *  Created on: Apr 5, 2026
 *      Author: Ozome
 */


#include "i2c.h"

void I2C1_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    GPIOB->MODER &= ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
    GPIOB->MODER |= (GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1);
    GPIOB->OTYPER |= (GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7);
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPD6_0 | GPIO_PUPDR_PUPD7_0);
    GPIOB->AFR[0] &= ~((15 << 24) | (15 << 28));
    GPIOB->AFR[0] |= ((4 << 24) | (4 << 28));

    // --- THE FIX: FORCE CLEAR THE I2C BUS ---
    // If the bus locked up during a previous test, this forcefully resets the silicon.
    I2C1->CR1 |= I2C_CR1_SWRST;
    for(volatile int i = 0; i < 10000; i++); // Wait a microsecond
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    I2C1->CR2 = 42;
    I2C1->CCR = 210;
    I2C1->TRISE = 43;
    I2C1->CR1 |= I2C_CR1_PE;
}

// --- NEW: THE BUS SCANNER ---
// Returns 1 if a device answers, 0 if no response
uint8_t I2C1_ScanBus(uint8_t address) {
    // Wait for bus to be free (with a timeout so we don't freeze)
    uint32_t timeout = 20000;
    while(I2C1->SR2 & I2C_SR2_BUSY) {
        if(--timeout == 0) return 0;
    }

    I2C1->CR1 |= I2C_CR1_START;
    timeout = 20000;
    while(!(I2C1->SR1 & I2C_SR1_SB)) {
        if(--timeout == 0) return 0;
    }

    I2C1->DR = address;
    timeout = 20000;
    while(!(I2C1->SR1 & I2C_SR1_ADDR)) {
        if(I2C1->SR1 & I2C_SR1_AF) { // NACK DETECTED!
            I2C1->SR1 &= ~I2C_SR1_AF; // Clear error flag
            I2C1->CR1 |= I2C_CR1_STOP; // Release the bus safely
            return 0; // Device is not here
        }
        if(--timeout == 0) return 0;
    }

    // If we reach here, the device ACK'd! Clear the flags and release the bus.
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    I2C1->CR1 |= I2C_CR1_STOP;
    return 1; // Device found!
}

// Updated to accept the verified address safely
void I2C1_WriteCommand(uint8_t address, uint8_t cmd) {
    while(I2C1->SR2 & I2C_SR2_BUSY);
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));

    I2C1->DR = address;
    while(!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    I2C1->DR = 0x00;
    while(!(I2C1->SR1 & I2C_SR1_TXE));

    I2C1->DR = cmd;
    while(!(I2C1->SR1 & I2C_SR1_TXE));
    while(!(I2C1->SR1 & I2C_SR1_BTF));

    I2C1->CR1 |= I2C_CR1_STOP;
}
