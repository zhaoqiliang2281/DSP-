/*
 * OLED.h
 *
 *  Created on: 2026Äê9ÔÂ25ÈÕ
 *      Author: liang2281
 */

#ifndef USER_OLED_H_
#define USER_OLED_H_

#include "main.h"



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
