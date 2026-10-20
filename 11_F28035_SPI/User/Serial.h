/*
 * Serial.h
 *
 *  Created on: 2026年3月23日
 *      Author: LiangZhaoqi
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include "main.h"

#define Buffer1_Max 64

extern int32_t m;
extern int32_t x;
extern int32_t y;
extern int32_t z;

/* 调试计数：sciParseBytes = 已进解析器的字节数；sciParseOk = 成功解析的帧数。
 * sciParseBytes 一直是 0 → 说明数据根本没到解析器（串口/环形缓冲区那边的问题）。 */
extern volatile uint32_t sciParseBytes;
extern volatile uint32_t sciParseOk;

uint8_t Serial_Data_Resolving(int32_t *m, int32_t *x, int32_t *y, int32_t *z);

//        Serial_Data_Resolving(&m,&x,&y,&z);
//        ips200_show_int(    16*10 , 16*9,   m, 5);
//        ips200_show_int(    16*10 , 16*10,   (int32)x, 5);
//        ips200_show_int(    16*10 , 16*11,   (int32)y, 5);
//        ips200_show_int(    16*10 , 16*12,   (int32)z, 5);
//        ips200_show_int(    16*10 , 16*13,   sciParseOk, 5);   /* 成功解析帧数 */

#endif /* SERIAL_H_ */
