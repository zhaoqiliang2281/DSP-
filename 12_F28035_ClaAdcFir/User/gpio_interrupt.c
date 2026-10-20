/*
 * gpio_interrupt.c
 *
 *  外部中断（XINT1 / XINT2 / XINT3）例程
 *      XINT1 <- GPIO0   下降沿触发（由 GPIO30 软件触发，需外部把 GPIO30 接到 GPIO0）
 *      XINT2 <- GPIO1   上升沿触发（由 GPIO31 软件触发，需外部把 GPIO31 接到 GPIO1）
 *      XINT3 <- GPIO8   下降沿触发（按键，按下接 GND，使用内部上拉）
 *
 *  Created on: 2026年10月7日
 *      Author: liang2281
 */

#include "gpio_interrupt.h"

__interrupt void xint1_isr(void);
__interrupt void xint2_isr(void);
__interrupt void key_isr(void);

/* 采样限定等待时间：QUALPRD=255 时每个采样周期为 510 个 SYSCLKOUT，6 次采样约 51us */
#define DELAY (CPU_RATE/1000*6*510)  //6 samples qual period

/* 各外部中断的进入次数（在中断中修改、在主循环中读取，所以要用 volatile） */
volatile Uint32 Xint1Count;
volatile Uint32 Xint2Count;
volatile Uint32 Xint3Count;

/* Gpio_Exti_Loop() 用的临时计数，用来判断中断有没有发生 */
Uint32 TempX1Count;
Uint32 TempX2Count;
Uint32 Gpio_Exti_LoopCount;

/*
 * 外部中断初始化
 *
 * 中断通道与 PIE 分组（TMS320F2803x）：
 *      XINT1 -> PIE 第 1 组 第 4 个 -> CPU INT1
 *      XINT2 -> PIE 第 1 组 第 5 个 -> CPU INT1
 *      XINT3 -> PIE 第 12 组第 1 个 -> CPU INT12
 * 注意 XINT3 不在第 1 组，必须单独使能 PIEIER12 和 IER 的 INT12，
 * 中断应答也要用 PIEACK_GROUP12。
 */
void Gpio_Exti(void)
{
    EALLOW; // 允许写受保护寄存器
    PieVectTable.XINT1 = &xint1_isr;
    PieVectTable.XINT2 = &xint2_isr;
    PieVectTable.XINT3 = &key_isr;

    EDIS;   // 禁止写受保护寄存器

    /* PIE 与 CPU 中断使能 */
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;          // 使能 PIE 模块
    PieCtrlRegs.PIEIER1.bit.INTx4 = 1;          // 使能 PIE 第 1 组 INT4（XINT1）
    PieCtrlRegs.PIEIER1.bit.INTx5 = 1;          // 使能 PIE 第 1 组 INT5（XINT2）
    PieCtrlRegs.PIEIER12.bit.INTx1 = 1;         // 使能 PIE 第 12 组 INT1（XINT3）
    IER |= M_INT1;                              // 使能 CPU INT1（对应 PIE 第 1 组）
    IER |= M_INT12;                             // 使能 CPU INT12（对应 PIE 第 12 组）
    EINT;                                       // 使能全局中断

    /* GPIO30 / GPIO31：给 XINT1、XINT2 提供测试信号，需要与 GPIO0、GPIO1 外部短接 */
    EALLOW;
    GpioDataRegs.GPASET.bit.GPIO30 = 1;         // 先置高（预置输出锁存器）
    GpioCtrlRegs.GPAMUX2.bit.GPIO30 = 0;        // 通用 I/O
    GpioCtrlRegs.GPADIR.bit.GPIO30 = 1;         // 输出

    GpioDataRegs.GPACLEAR.bit.GPIO31 = 1;       // 先置低（预置输出锁存器）
    GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 0;        // 通用 I/O
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;         // 输出
    EDIS;

    /* GPIO0 / GPIO1 / GPIO8 配置为输入 */
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;        // 通用 I/O
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 0;         // 输入
    GpioCtrlRegs.GPAQSEL1.bit.GPIO0 = 0;       // 只与 SYSCLKOUT 同步，不做采样限定

    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 0;         // 通用 I/O
    GpioCtrlRegs.GPADIR.bit.GPIO1 = 0;          // 输入
    GpioCtrlRegs.GPAQSEL1.bit.GPIO1 = 2;        // 6 次采样限定（去抖）

    GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 0;         // 通用 I/O
    GpioCtrlRegs.GPADIR.bit.GPIO8 = 0;          // 输入
    GpioCtrlRegs.GPAPUD.bit.GPIO8 = 0;          // 使能内部上拉（复位后 GPIO0~GPIO11 的上拉默认是关闭的）
    GpioCtrlRegs.GPAQSEL1.bit.GPIO8 = 2;        // 6 次采样限定（去抖）

    /*
     * 采样周期由 QUALPRD 决定：GPIO0~GPIO7 用 QUALPRD0，GPIO8~GPIO15 用 QUALPRD1。
     * QUALPRD0 = 0xFF 时采样周期 = 2*255*TSYSCLKOUT，6 次采样的窗口宽 5 个采样周期。
     * 注意：这里没有配置 QUALPRD1，所以 GPIO8 的采样周期仍为 1 个 SYSCLKOUT，
     *       若按键抖动明显，可再设置 GpioCtrlRegs.GPACTRL.bit.QUALPRD1 加大窗口。
     */
    GpioCtrlRegs.GPACTRL.bit.QUALPRD0 = 0xFF;
    EDIS;

    /* 选择每个外部中断对应的引脚（该寄存器受 EALLOW 保护） */
    EALLOW;
    GpioIntRegs.GPIOXINT1SEL.bit.GPIOSEL = 0;   // XINT1 选择 GPIO0
    GpioIntRegs.GPIOXINT2SEL.bit.GPIOSEL = 1;   // XINT2 选择 GPIO1
    GpioIntRegs.GPIOXINT3SEL.bit.GPIOSEL = 8;   // XINT3 选择 GPIO8
    EDIS;

    /* 触发边沿：00=下降沿，01=上升沿，10=下降沿，11=双边沿 */
    XIntruptRegs.XINT1CR.bit.POLARITY = 0;      // 下降沿触发
    XIntruptRegs.XINT2CR.bit.POLARITY = 1;      // 上升沿触发
    XIntruptRegs.XINT3CR.bit.POLARITY = 0;      // 下降沿触发（按键按下时 GPIO8 被拉到 GND）

    /* 使能外部中断 */
    XIntruptRegs.XINT1CR.bit.ENABLE = 1;        // 使能 XINT1
    XIntruptRegs.XINT2CR.bit.ENABLE = 1;        // 使能 XINT2
    XIntruptRegs.XINT3CR.bit.ENABLE = 1;        // 使能 XINT3

    /* GPIO34：每进一次中断就在中断里拉低，可用示波器观察中断是否发生 */
    EALLOW;
    GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0;        // 通用 I/O
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;         // 输出
    EDIS;

    Xint1Count = 0; // XINT1 计数清零
    Xint2Count = 0; // XINT2 计数清零
    Xint3Count = 0; // XINT3 计数清零

    Gpio_Exti_LoopCount = 0;  // 测试循环次数清零

}

/*
 * 外部中断测试循环
 * 先软件触发 XINT1、再触发 XINT2 并等待中断计数增加，
 * 同时把 XINT3（按键）的中断次数显示到屏幕上。
 */
void Gpio_Exti_Loop(void)
{
    TempX1Count = Xint1Count;
    TempX2Count = Xint2Count;

    ips200_show_int(60, 130, Xint3Count, 8);   // 显示 XINT3（按键）中断次数
    /* 触发 XINT1：拉低 GPIO30，在 GPIO0 上产生下降沿 */
    GpioDataRegs.GPBSET.bit.GPIO34 = 1;   // GPIO34 置高
    GpioDataRegs.GPACLEAR.bit.GPIO30 = 1; // 拉低 GPIO30，触发 XINT1

    while(Xint1Count == TempX1Count)      // 等待 XINT1 中断
    {
//        ips200_show_int(60, 130, Xint1Count, 8);
//        ips200_show_int(60, 150, TempX1Count, 8);
    }

    /* 触发 XINT2：等过采样限定时间后拉高 GPIO31，在 GPIO1 上产生上升沿 */
    GpioDataRegs.GPBSET.bit.GPIO34 = 1;   // GPIO34 置高
    DELAY_US(DELAY);                      // 等待采样限定周期
    GpioDataRegs.GPASET.bit.GPIO31 = 1;   // 拉高 GPIO31，触发 XINT2

    while(Xint2Count == TempX2Count)      // 等待 XINT2 中断
    {
//        ips200_show_int(60, 200, Xint2Count, 8);
//        ips200_show_int(60, 220, TempX2Count, 8);
    }

    /* 检查两个中断是否都只进了一次，然后把触发信号复位，准备下一轮 */
    if(Xint1Count == TempX1Count+1 && Xint2Count == TempX2Count+1)
    {
        Gpio_Exti_LoopCount++;
        GpioDataRegs.GPASET.bit.GPIO30 = 1;   // 拉高 GPIO30
        GpioDataRegs.GPACLEAR.bit.GPIO31 = 1; // 拉低 GPIO31
    }
    else
    {
        __asm("      ESTOP0");              // 计数异常时在此断住
    }
}
//
// XINT1 中断服务函数（GPIO0，下降沿）
//
__interrupt void xint1_isr(void)
{
    GpioDataRegs.GPBCLEAR.all = 0x4;   // GPIO34 拉低
    Xint1Count++;

    /* 应答 PIE 第 1 组，否则本组中断无法再次响应 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//
// XINT2 中断服务函数（GPIO1，上升沿）
//
__interrupt void xint2_isr(void)
{
    GpioDataRegs.GPBCLEAR.all = 0x4;   // GPIO34 拉低
    Xint2Count++;

    /* 应答 PIE 第 1 组，否则本组中断无法再次响应 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
//
// XINT3 中断服务函数（GPIO8，下降沿，按键按下时触发）
//
__interrupt void key_isr(void)
{
    Xint3Count++;
    GpioDataRegs.GPATOGGLE.bit.GPIO24 =1;  // 翻转 LED，指示按键中断

    /* 应答 PIE 第 12 组，否则本组中断无法再次响应 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;
}
