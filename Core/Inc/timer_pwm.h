/*
 * timer_pwm.h
 *
 *  Created on: Apr 3, 2026
 *      Author: Ozome
 */

#ifndef TIMER_PWM_H
#define TIMER_PWM_H

#include "stm32f4xx.h"

void MOSFET_Timer1_Init(void);
void MOSFET_Timer1_Start(void);
void MOSFET_Timer1_Stop(void);
void MOSFET_Timer1_SetDutyCycle(uint16_t duty);

// --- NEW SPWM FUNCTIONS ---
void SPWM_Start_Frequency(float target_hz);
void SPWM_Stop(void);

#endif /* TIMER_PWM_H */
