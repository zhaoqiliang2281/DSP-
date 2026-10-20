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
    GpioCtrlRegs.GPBMUX1.bit.GPIO41 = 0;
    GpioCtrlRegs.GPBMUX1.bit.GPIO44 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 0;

    /* 0 = 输入 */
    GpioCtrlRegs.GPBDIR.bit.GPIO39  = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO41  = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO44  = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO8  = 0;

    /* 0 = 使能内部上拉，按键一端接地时松开为高、按下为低 */
    GpioCtrlRegs.GPBPUD.bit.GPIO39  = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO41  = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO44  = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO8  = 0;
    /* 输入限定：2 = 6 采样，硬件层面滤掉一部分毛刺 */
    GpioCtrlRegs.GPBQSEL1.bit.GPIO39 = 2;
    GpioCtrlRegs.GPBQSEL1.bit.GPIO41 = 2;
    GpioCtrlRegs.GPBQSEL1.bit.GPIO44 = 2;
    GpioCtrlRegs.GPAQSEL1.bit.GPIO8 = 2;

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

//扫描按键
//==============================================LED和按键变量定义区===================================================
uint8_t key1_state = 1;                                // 按键动作状态
uint8_t key2_state = 1;                                // 按键动作状态
uint8_t key3_state = 1;                                // 按键动作状态
uint8_t key4_state = 1;                                // 按键动作状态

uint8_t key1_state_last = 0;                           // 上一次按键动作状态
uint8_t key2_state_last = 0;                           // 上一次按键动作状态
uint8_t key3_state_last = 0;                           // 上一次按键动作状态
uint8_t key4_state_last = 0;                           // 上一次按键动作状态

//按键开关标志
uint8_t key1_flag = 0;
uint8_t key2_flag = 0;
uint8_t key3_flag = 0;
uint8_t key4_flag = 0;

/************************************************
 *  函数名称：按键扫描：key_scan(void)
 *  功能说明：保证调用时间不小于10ms
 *  参数说明：
 *  参   数：无
 *  函数返回：无
 *  修改时间：2025年6月12日
 *  备   注：标志位置位之后，可以使用标志位执行自己想要做的事件
 ************************************************/
void key_Scan(void)//按键扫描
{
    //使用此方法优点在于，不需要使用while(1) 等待，避免处理器资源浪费
    //保存按键状态
    key1_state_last = key1_state;
    key2_state_last = key2_state;
    key3_state_last = key3_state;
    key4_state_last = key4_state;

    //读取当前按键状态
    key1_state = GpioDataRegs.GPBDAT.bit.GPIO39;
    key2_state = GpioDataRegs.GPBDAT.bit.GPIO41;
    key3_state = GpioDataRegs.GPBDAT.bit.GPIO44;
    key4_state = GpioDataRegs.GPADAT.bit.GPIO8;


    //检测到按键按下之后  并放开置位标志位
    if(key1_state && !key1_state_last)   {key1_flag = 1;}//按键按下完成对应标志位会置1
    if(key2_state && !key2_state_last)   {key2_flag = 1;}
    if(key3_state && !key3_state_last)   {key3_flag = 1;}
    if(key4_state && !key4_state_last)   {key4_flag = 1;}


}


