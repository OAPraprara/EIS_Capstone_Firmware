/*
 * adc_dma.c
 *
 *  Created on: Apr 2, 2026
 *      Author: Ozome
 */


#include "adc_dma.h"

// Define the global array here. The DMA hardware will stream data directly into this memory location.
volatile uint16_t adc_raw[2] = {0, 0};

void ADC_DMA_Init(void) {
    // 1. Enable Clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    // 2. Configure PA0 and PA1 as Analog Mode
    GPIOA->MODER |= GPIO_MODER_MODER0;
    GPIOA->MODER |= GPIO_MODER_MODER1;

    // 3. Configure DMA2 Stream 0 Channel 0
    DMA2_Stream0->CR = 0;
    while(DMA2_Stream0->CR & DMA_SxCR_EN);

    DMA2_Stream0->CR |= (0 << 25);              // Select Channel 0
    DMA2_Stream0->CR |= DMA_SxCR_MSIZE_0;       // Memory size 16-bit
    DMA2_Stream0->CR |= DMA_SxCR_PSIZE_0;       // Peripheral size 16-bit
    DMA2_Stream0->CR |= DMA_SxCR_MINC;          // Memory Increment ON
    DMA2_Stream0->CR |= DMA_SxCR_CIRC;          // Circular Mode ON
    DMA2_Stream0->CR &= ~DMA_SxCR_DIR;          // Peripheral-to-Memory

    DMA2_Stream0->NDTR = 2;                     // 2 pieces of data
    DMA2_Stream0->PAR = (uint32_t)&ADC1->DR;
    DMA2_Stream0->M0AR = (uint32_t)adc_raw;

    DMA2_Stream0->CR |= DMA_SxCR_EN;            // Turn on DMA Stream

    // --- ADD THIS LINE: Clear all hidden DMA error flags for Stream 0 ---
    DMA2->LIFCR |= 0x3D;

    // 4. Configure ADC1 for Scan Mode (NO CONTINUOUS MODE)
    ADC1_COMMON->CCR &= ~ADC_CCR_ADCPRE;
    ADC1_COMMON->CCR |= ADC_CCR_ADCPRE_0;       // Prescaler /4
    ADC1->CR1 |= ADC_CR1_SCAN;                  // Scan Mode ON
    // Notice: ADC_CR2_CONT is entirely gone!

    // 5. Build the Sequence List
    ADC1->SQR1 |= (1 << 20);                    // Sequence Length = 2
    ADC1->SQR3 = 0;
    ADC1->SQR3 |= (0 << 0);                     // 1st = PA0
    ADC1->SQR3 |= (1 << 5);                     // 2nd = PA1

    // 6. Set Sampling Time
    ADC1->SMPR2 |= (4 << 0);                    // PA0 84 cycles
    ADC1->SMPR2 |= (4 << 3);                    // PA1 84 cycles

    // --- THE HARDWARE TRIGGER (FIXED) ---
	// Tell the ADC to listen for Timer 1 Channel 2 (EXTSEL = 0001)
	ADC1->CR2 &= ~ADC_CR2_EXTSEL;
	ADC1->CR2 |= (1 << 24);

	// Enable the trigger to fire on the Rising Edge (EXTEN = 01)
	ADC1->CR2 &= ~ADC_CR2_EXTEN;
	ADC1->CR2 |= (1 << 28);

    // 7. Link ADC to DMA
    ADC1->CR2 |= ADC_CR2_DMA;
    ADC1->CR2 |= ADC_CR2_DDS;

    // 8. Power ON
    ADC1->CR2 |= ADC_CR2_ADON;

    // Notice: ADC_CR2_SWSTART is entirely gone! The Timer will fire the gun.
}
