/*
 * i2c.h
 *
 *  Created on: Apr 5, 2026
 *      Author: Ozome
 */

#ifndef I2C_H
#define I2C_H

#include "stm32f4xx.h"

void I2C1_Init(void);
uint8_t I2C1_ScanBus(uint8_t address); // NEW: The hardware scanner
void I2C1_WriteCommand(uint8_t address, uint8_t cmd); // Notice we pass the address now

#endif /* I2C_H */
