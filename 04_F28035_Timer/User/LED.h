/*
 * LED.h
 *
 *  Created on: 2026年9月26日
 *      Author: liang2281
 */

#ifndef USER_LED_H_
#define USER_LED_H_

#include "DSP2803x_Device.h"   /* 包含 DSP 设备寄存器定义和外设宏 */
#include "DSP28x_Project.h"      /* 包含工程级配置与公共定义 */
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "OLED.h"

void LED_Init(void);

#endif /* USER_LED_H_ */
