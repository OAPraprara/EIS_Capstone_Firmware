/*
 * dsp_math.c
 *
 *  Created on: Apr 2, 2026
 *      Author: Ozome
 */


#include "dsp_math.h"

float Calculate_Battery_Voltage(uint16_t raw_adc) {
    // 1. Calculate voltage at the physical STM32 pin
    float v_pin = ((float)raw_adc * V_REF) / ADC_MAX;

    // 2. Multiply by the voltage divider ratio to get the true 18650 voltage
    return v_pin * DIVIDER_RATIO;
}

float Calculate_Load_Current(uint16_t raw_adc) {
    // 1. Calculate the voltage coming out of the INA240 into the STM32 pin
    float v_pin = ((float)raw_adc * V_REF) / ADC_MAX;

    // 2. Reverse the amplifier gain to find the voltage actually crossing the shunt
    float v_shunt = v_pin / INA240_GAIN;

    // 3. Ohm's Law (I = V / R) to find the actual Amps flowing through the circuit
    return v_shunt / SHUNT_RESISTOR;
}
