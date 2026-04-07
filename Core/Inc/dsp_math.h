/*
 * dsp_math.h
 *
 *  Created on: Apr 2, 2026
 *      Author: Ozome
 */

#ifndef DSP_MATH_H
#define DSP_MATH_H

#include "stm32f4xx.h"

// --- HARDWARE SPECS ---
#define V_REF 3.3f
#define ADC_MAX 4095.0f
#define DIVIDER_RATIO 2.0f       // (10k + 10k) / 10k
#define SHUNT_RESISTOR 0.05f     // Ohms
#define INA240_GAIN 50.0f        // Assuming INA240A2.

// --- FUNCTIONS ---
float Calculate_Battery_Voltage(uint16_t raw_adc);
float Calculate_Load_Current(uint16_t raw_adc);

#endif /* DSP_MATH_H */
