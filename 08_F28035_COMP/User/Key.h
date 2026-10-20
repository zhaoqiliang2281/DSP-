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

extern uint8_t key1_flag;
extern uint8_t key2_flag;
extern uint8_t key3_flag;
extern uint8_t key4_flag;


void    KEY_Init(void);
void    KEY_Scan(void);          /* 由 1ms 定时器中断调用 */
KeyEvent_t KEY_GetEvent(void);   /* 主循环消费，读后自动清除 */

void key_Scan(void);


//        KeyEvent_t e = KEY_GetEvent();
//        switch (e)
//        {
//        case KEY_EVT_SHORT:
//            // 短按：比如切换 LED 状态
//            GpioDataRegs.GPASET.bit.GPIO21   = 1;
//            Scia_SendString("Hello World!");
//            break;
//        case KEY_EVT_LONG:
//            // 长按：比如进入设置模式
//            GpioDataRegs.GPACLEAR.bit.GPIO21 = 1;
//            break;
//        default:
//            break;
//        }

#endif /* USER_KEY_H_ */
