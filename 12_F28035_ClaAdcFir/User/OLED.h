/*
 * OLED.h
 *
 *  Created on: 2026年9月25日
 *      Author: liang2281
 */

#ifndef USER_OLED_H_
#define USER_OLED_H_

#include "main.h"



//========================== IIC 通信方式选择（用户配置区）==========================
// 两种方式用的是同一对引脚（SDA = GPIO32/SDAA，SCL = GPIO33/SCLA），切换宏不用改接线：
//   OLED_USE_SOFT_I2C = 1 -> GPIO 模拟 IIC（默认，就是原来的实现，一行都没改）
//                     = 0 -> 硬件 I2C-A（时序由外设产生，速度快、不占 CPU 时间）
// 注意：I2C 是开漏总线，SDA/SCL 必须有上拉电阻才能出高电平；OLED 模块板上一般自带
//       4.7k 上拉，硬件模式里也打开了芯片内部上拉作为兜底。
#define OLED_USE_SOFT_I2C       0

#if (0 == OLED_USE_SOFT_I2C)
/* 硬件 I2C 模式的诊断计数（可在 CCS 的 Watch 窗口查看） */
extern uint16_t oled_i2c_diag_nack;         /* 从机无应答次数（地址/接线/上拉问题） */
extern uint16_t oled_i2c_diag_timeout;      /* 等 STOP 完成或等 FIFO 空位超时次数 */
extern uint16_t oled_i2c_diag_overflow;     /* 一帧字节数超过缓冲被丢弃的次数 */
#endif

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
