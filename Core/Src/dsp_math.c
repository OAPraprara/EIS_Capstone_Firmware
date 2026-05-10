/*
 * dsp_math.c
 *
 *  Created on: Apr 2, 2026
 *      Author: Ozome
 */


#include "dsp_math.h"
#include <math.h>

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


// The Goertzel Algorithm: Extracts the Real and Imaginary vectors of a specific frequency
void Goertzel_Filter(float* samples, int N, float target_freq, float sample_rate, float* real_out, float* imag_out) {
    // 1. Calculate the target frequency bin
    int k = (int)(0.5f + ((N * target_freq) / sample_rate));

    // 2. Pre-compute the cosine and sine coefficients
    float omega = (2.0f * 3.14159265f * k) / (float)N;
    float cosine = cosf(omega);
    float sine = sinf(omega);
    float coeff = 2.0f * cosine;

    // 3. The fast processing loop
    float q0 = 0.0f, q1 = 0.0f, q2 = 0.0f;
    for (int i = 0; i < N; i++) {
        q0 = (coeff * q1) - q2 + samples[i];
        q2 = q1;
        q1 = q0;
    }

    // 4. Calculate final Real and Imaginary components
    // We scale by (N/2) to get the true amplitude of the wave
    *real_out = (q1 - (q2 * cosine)) / ((float)N / 2.0f);
    *imag_out = (q2 * sine) / ((float)N / 2.0f);
}
