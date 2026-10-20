/*
 * Key.h
 *
 *  Created on: 2026年9月26日
 *      Author: liang2281
 */

#ifndef USER_KEY_H_
#define USER_KEY_H_

#include "main.h"

/* 按键引脚，改这里即可 */
#define KEY_GPIO_NUM    39
#define KEY_ACTIVE_LOW  1        /* 1=按下为低电平（按键一端接地） */

typedef enum {
    KEY_EVT_NONE = 0,
    KEY_EVT_SHORT,               /* 短按 */
    KEY_EVT_LONG                 /* 长按，可选 */
} KeyEvent_t;

void    KEY_Init(void);
void    KEY_Scan(void);          /* 由 1ms 定时器中断调用 */
KeyEvent_t KEY_GetEvent(void);   /* 主循环消费，读后自动清除 */


#endif /* USER_KEY_H_ */
