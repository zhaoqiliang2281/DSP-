/*
 * OLED.h
 *
 *  Created on: 2026年9月25日
 *      Author: liang2281
 */

#ifndef USER_OLED_H_
#define USER_OLED_H_

#include "DSP2803x_Device.h"   /* 包含 DSP 设备寄存器定义和外设宏 */
#include "DSP28x_Project.h"      /* 包含工程级配置与公共定义 */
#include <string.h>              /* 提供 memcpy 等内存操作函数 */
#include <stdint.h>              /* 提供固定宽度整数类型定义 */
#include "DSP2803x_Device.h"
#include "DSP2803x_Examples.h"

typedef unsigned char  uint8_t;

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

extern const uint8_t OLED_F8x16[][16];

#endif /* USER_OLED_H_ */
