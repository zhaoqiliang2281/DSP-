/*
 * PWM.h
 *
 *  Created on: 2026Äê9ÔÂ28ÈÕ
 *      Author: liang2281
 */

#ifndef USER_PWM_H_
#define USER_PWM_H_

#include "main.h"

void EPwm3_Init(void);
void EPWM3_SetDuty(float duty);
interrupt void epwm3_isr(void);

#endif /* USER_PWM_H_ */
