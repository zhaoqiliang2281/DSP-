/*
 * Timer.h
 *
 *  Created on: 2026年9月26日
 *      Author: liang2281
 */

#ifndef USER_TIMER_H_
#define USER_TIMER_H_

#include "DSP2803x_Device.h"   /* 包含 DSP 设备寄存器定义和外设宏 */
#include "DSP28x_Project.h"      /* 包含工程级配置与公共定义 */
#include "DSP2803x_Device.h"
#include "DSP2803x_Examples.h"
#include "LED.h"
#include "KEY.h"

extern volatile Uint32 timer0_count;
extern volatile Uint32 timer1_count;
extern volatile Uint32 timer2_count;


void Timer0_Init(void);
void Timer1_Init(void);
void Timer2_Init(void);

#endif /* USER_TIMER_H_ */
