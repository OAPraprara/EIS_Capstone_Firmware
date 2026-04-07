/*
 * timer_pwm.h
 *
 *  Created on: Apr 3, 2026
 *      Author: Ozome
 */

#ifndef INC_TIMER_PWM_H_
#define INC_TIMER_PWM_H_

#include "stm32f4xx.h"

void MOSFET_Timer1_Init(void);
void MOSFET_Timer1_Start(void);
void MOSFET_Timer1_Stop(void);
void MOSFET_Timer1_SetDutyCycle(uint16_t duty);

#endif /* INC_TIMER_PWM_H_ */
