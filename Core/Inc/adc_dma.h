/*
 * adc_dma_handler.h
 *
 *  Created on: Apr 2, 2026
 *      Author: Ozome
 */

#ifndef ADC_DMA_H
#define ADC_DMA_H

#include "stm32f4xx.h"

void ADC_DMA_Init(void);

// This is the magical array that DMA will continuously update in the background.
// Index [0] = PA0 (Voltage), Index [1] = PA1 (Current)
extern volatile uint16_t adc_raw[2];

#endif /* ADC_DMA_H */
