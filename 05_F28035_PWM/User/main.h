/*
 * main.h
 *
 *  Created on: 2026年9月28日
 *      Author: liang2281
 */

#ifndef USER_MAIN_H_
#define USER_MAIN_H_



#include "DSP2803x_Device.h"   /* 包含 DSP 设备寄存器定义和外设宏 */
#include "DSP28x_Project.h"      /* 包含工程级配置与公共定义 */
#include <string.h>              /* 提供 memcpy 等内存操作函数 */
#include <stdint.h>              /* 提供固定宽度整数类型定义 */


#include "LED.h"
#include "Key.h"
#include "OLED.h"
#include "zf_device_ips200.h"    /* IPS200 液晶驱动 (TMS320F28035 版) */
#include "Timer.h"
#include "PWM.h"

extern uint32_t num;


#endif /* USER_MAIN_H_ */
