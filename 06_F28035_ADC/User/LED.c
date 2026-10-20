/*
 * LED.c
 *
 *  Created on: 2026年9月26日
 *      Author: liang2281
 */

#include "LED.h"


void LED_Init(void)
{
    /* 允许访问受保护的 GPIO 相关寄存器 */
    EALLOW;

    /* 打开 GPIO 输入使能时钟，确保 GPIO 模块时钟正常工作 */
    SysCtrlRegs.PCLKCR3.bit.GPIOINENCLK = 1;

    /* 配置 GPIO21 作为通用 I/O，不使用复用功能 */
    GpioCtrlRegs.GPAMUX2.bit.GPIO21=0;

    /* 设置 GPIO21 为输出方向，1:输出 0 输入 后续用作控制 LED 或输出信号 */
    GpioCtrlRegs.GPADIR.bit.GPIO21=1;

    /* 禁用 GPIO21 内部上拉电阻（输出模式下一般禁用上拉） */
    GpioCtrlRegs.GPAPUD.bit.GPIO21=1;

    /* 先将 GPIO21 输出清零，确保初始状态为低电平 */
    GpioDataRegs.GPACLEAR.bit.GPIO21=1;

    /* 结束对受保护寄存器的写操作 */
    EDIS;
}




