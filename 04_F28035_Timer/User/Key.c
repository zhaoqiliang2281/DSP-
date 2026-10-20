/*
 * Key.c
 *
 *  Created on: 2026年9月26日
 *      Author: liang2281
 */

#include "KEY.h"

/* 消抖与长按阈值，单位：扫描周期（1ms） */
#define KEY_DEBOUNCE_MS   20
#define KEY_LONG_MS       1000

typedef enum {
    KEY_ST_IDLE = 0,
    KEY_ST_DEBOUNCE,
    KEY_ST_PRESSED,
    KEY_ST_RELEASE
} KeyState_t;

static KeyState_t s_state = KEY_ST_IDLE;
static uint16_t   s_cnt   = 0;
static volatile KeyEvent_t s_event = KEY_EVT_NONE;
static uint8_t    s_longFired = 0;   /* 本次按下是否已发过长按 */


void KEY_Init(void)
{
    EALLOW;
    /* 通用 I/O，不复用外设功能 */
    GpioCtrlRegs.GPBMUX1.bit.GPIO39 = 0;
    /* 0 = 输入 */
    GpioCtrlRegs.GPBDIR.bit.GPIO39  = 0;
    /* 0 = 使能内部上拉，按键一端接地时松开为高、按下为低 */
    GpioCtrlRegs.GPBPUD.bit.GPIO39  = 0;
    /* 输入限定：2 = 6 采样，硬件层面滤掉一部分毛刺 */
    GpioCtrlRegs.GPBQSEL1.bit.GPIO39 = 2;
    /* 每个采样窗口周期，值越大采样越慢、滤波越强 */
//    GpioCtrlRegs.GPBCTRL.bit.QUALPRD1 = 0xFF;

    EDIS;
}


/* 读取按键“有效按下”状态，统一处理高/低电平有效差异 */
static uint8_t KEY_IsActive(void)
{
    uint8_t lvl = GpioDataRegs.GPBDAT.bit.GPIO39;
#if KEY_ACTIVE_LOW
    return (lvl == 0);
#else
    return (lvl == 1);
#endif
}

void KEY_Scan(void)
{
    uint8_t act = KEY_IsActive();

    switch (s_state) {
    case KEY_ST_IDLE:
        if (act) { s_state = KEY_ST_DEBOUNCE; s_cnt = 0; }
        break;

    case KEY_ST_DEBOUNCE:
        if (act) {
            if (++s_cnt >= KEY_DEBOUNCE_MS) {
                s_state     = KEY_ST_PRESSED;
                s_cnt       = 0;
                s_longFired = 0;
            }
        } else { s_state = KEY_ST_IDLE; s_cnt = 0; }
        break;

    case KEY_ST_PRESSED:
        if (!act) { s_state = KEY_ST_RELEASE; s_cnt = 0; }
        else {
            if (s_cnt < KEY_LONG_MS) {
                if (++s_cnt == KEY_LONG_MS) {
                    s_event     = KEY_EVT_LONG;
                    s_longFired = 1;
                }
            }
        }
        break;

    case KEY_ST_RELEASE:
        if (!act) {
            if (++s_cnt >= KEY_DEBOUNCE_MS) {
                if (!s_longFired) s_event = KEY_EVT_SHORT;
                s_longFired = 0;
                s_state = KEY_ST_IDLE;
                s_cnt   = 0;
            }
        } else { s_state = KEY_ST_PRESSED; s_cnt = 0; }
        break;

    default:
        s_state = KEY_ST_IDLE;
        s_cnt   = 0;
        break;
    }
}

KeyEvent_t KEY_GetEvent(void)
{
    KeyEvent_t e = s_event;
    s_event = KEY_EVT_NONE;
    return e;
}


