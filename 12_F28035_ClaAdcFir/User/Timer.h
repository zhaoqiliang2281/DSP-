/*
 * Timer.h
 *
 *  Created on: 2026Äê9ÔÂ26ÈÕ
 *      Author: liang2281
 */

#ifndef USER_TIMER_H_
#define USER_TIMER_H_

#include "main.h"

extern volatile Uint32 timer0_count;
extern volatile Uint32 timer1_count;
extern volatile Uint32 timer2_count;


void Timer0_Init(void);
void Timer1_Init(void);
void Timer2_Init(void);

#endif /* USER_TIMER_H_ */
