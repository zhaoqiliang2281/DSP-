/*
 * Timer.c
 *
 *  Created on: 2026年9月26日
 *      Author: liang2281
 */

#include "Timer.h"

/* 定时器中断计数，用于观察是否进入中断 */
volatile Uint32 timer0_count = 0;
volatile Uint32 timer1_count = 0;
volatile Uint32 timer2_count = 0;

/* 中断服务函数声明 */
__interrupt void cpu_timer0_isr(void);
__interrupt void cpu_timer1_isr(void);
__interrupt void cpu_timer2_isr(void);

/* ---------------------------------------------------------------------------
 * 【重要】InitCpuTimers() 是“全局一次性”初始化函数：
 * 它会把 CpuTimer0 / CpuTimer1 / CpuTimer2 三个定时器一起复位，
 * 包括把每个定时器的 PRD 置 0xFFFFFFFF、把 TCR.TSS 置 1（停止定时器）。
 *
 * 所以整个程序里它只能执行一次！
 * 如果在 Timer1_Init() 里再调用一次，就会把刚刚启动好的 Timer0 又停掉，
 * 表现就是“加了 Timer1 之后 Timer0 再也不进中断了”。
 *
 * 下面这个包装函数保证它只执行一次，并且与调用先后顺序无关。
 * ------------------------------------------------------------------------- */
static Uint16 s_cpuTimersInited = 0;

static void InitCpuTimers_Once(void)
{
    if (s_cpuTimersInited == 0)
    {
        InitCpuTimers();          /* 复位全部 3 个 CPU 定时器，只做这一次 */
        s_cpuTimersInited = 1;
    }
}


void Timer0_Init(void)
{
    /* ---------- 配置 CPU Timer0 ---------- */
    InitCpuTimers_Once();                  /* 三个 CPU 定时器只需初始化一次 */

    /* ConfigCpuTimer(定时器指针, CPU频率MHz, 周期us)
     * 计数初值 = Freq(MHz) * Period(us)，60MHz * 10000us = 600000 计数 -> 10ms 中断一次
     * 若要 1s 中断一次，Period 写成 1000000 */
    ConfigCpuTimer(&CpuTimer0, 60, 1000);   /* 60MHz, 10000us = 10ms */

    /* ★★ 关键：ConfigCpuTimer() 内部会把 TCR.TSS 置 1(停止定时器)，
     *   配置完成后必须手动启动，否则定时器不计数、永远不会进中断 ★★ */
    CpuTimer0Regs.TCR.bit.TSS = 0;           /* 启动 CPU Timer0，等价于 StartCpuTimer0() */

    /* 把 TINT0 的中断服务函数挂到 PIE 向量表里 */
    EALLOW;
    PieVectTable.TINT0 = &cpu_timer0_isr;
    EDIS;

    /* 使能 PIE 组1 的第7个中断 (TINT0 -> INT1.7) */
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;

    /* 使能 CPU 中断1 (INT1) */
    IER |= M_INT1;

    /* 使能全局中断 */
    EINT;      /* 等价于 asm(" CLRC INTM")，允许可屏蔽中断 */
    ERTM;      /* 允许实时调试中断 DBGM，纯运行时也可加 */
}

__interrupt void cpu_timer0_isr(void)
{
    timer0_count++;                    /* 计数，可在主循环里用 IPS200 显示 */
    KEY_Scan();
//    /* ★ 关键：清除中断标志 ★ */
//    CpuTimer0.InterruptCount++;        /* 若用 ConfigCpuTimer，这一行可选 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;   /* 应答 PIE 组1，允许下次中断 */

    /* CPU Timer 的标志由硬件在响应时自动清，无需手动清 TIF */
}

void Timer1_Init(void)
{
    /* ---------- 配置 CPU Timer1 ---------- */
    InitCpuTimers_Once();                  /* 不要再调 InitCpuTimers()，会停掉 Timer0 */

    /* ConfigCpuTimer(定时器指针, CPU频率MHz, 周期us)
     * 计数初值 = Freq(MHz) * Period(us)，60MHz * 1000000us = 60000000 计数 -> 1s 中断一次 */
    ConfigCpuTimer(&CpuTimer1, 60, 1000000);   /* 60MHz, 1000000us = 1s */

    /* ★★ 关键：ConfigCpuTimer() 内部会把 TCR.TSS 置 1(停止定时器)，
     *   配置完成后必须手动启动，否则定时器不计数、永远不会进中断 ★★ */
    CpuTimer1Regs.TCR.bit.TSS = 0;           /* 启动 CPU Timer1，等价于 StartCpuTimer1() */

    /* 把 TINT1 的中断服务函数挂到 PIE 向量表里。
     * 注意：TINT1/TINT2 属于 CPU 级中断(INT13/INT14)，不在 PIE 的 1~12 组里，
     *       所以既不需要写 PIEIER，也不需要写 PIEACK，直接用 IER 使能即可。 */
    EALLOW;
    PieVectTable.TINT1 = &cpu_timer1_isr;
    EDIS;

    /* 使能 CPU 中断13 (INT13 = TINT1，CPU Timer1 专用) */
    IER |= M_INT13;

    /* 使能全局中断 */
    EINT;      /* 等价于 asm(" CLRC INTM")，允许可屏蔽中断 */
    ERTM;      /* 允许实时调试中断 DBGM，纯运行时也可加 */
}

__interrupt void cpu_timer1_isr(void)
{
    timer1_count++;
//    GpioDataRegs.GPATOGGLE.bit.GPIO24 =1;
    /* Timer1 直接走 CPU 中断，无需 PIEACK */
}

void Timer2_Init(void)
{
    /* ---------- 配置 CPU Timer1 ---------- */
    InitCpuTimers_Once();                  /* 不要再调 InitCpuTimers()，会停掉 Timer0 */

    /* ConfigCpuTimer(定时器指针, CPU频率MHz, 周期us)
     * 计数初值 = Freq(MHz) * Period(us)，60MHz * 1000000us = 60000000 计数 -> 1s 中断一次 */
    ConfigCpuTimer(&CpuTimer2, 60, 100000);   /* 60MHz, 1000000us = 1s */

    /* ★★ 关键：ConfigCpuTimer() 内部会把 TCR.TSS 置 1(停止定时器)，
     *   配置完成后必须手动启动，否则定时器不计数、永远不会进中断 ★★ */
    CpuTimer2Regs.TCR.bit.TSS = 0;           /* 启动 CPU Timer1，等价于 StartCpuTimer1() */

    /* 把 TINT1 的中断服务函数挂到 PIE 向量表里。
     * 注意：TINT1/TINT2 属于 CPU 级中断(INT13/INT14)，不在 PIE 的 1~12 组里，
     *       所以既不需要写 PIEIER，也不需要写 PIEACK，直接用 IER 使能即可。 */
    EALLOW;
    PieVectTable.TINT2 = &cpu_timer2_isr;
    EDIS;

    /* 使能 CPU 中断13 (INT13 = TINT1，CPU Timer1 专用) */
    IER |= M_INT14;

    /* 使能全局中断 */
    EINT;      /* 等价于 asm(" CLRC INTM")，允许可屏蔽中断 */
    ERTM;      /* 允许实时调试中断 DBGM，纯运行时也可加 */
}

__interrupt void cpu_timer2_isr(void)
{
    timer2_count++;
    key_Scan();
    /* Timer1 直接走 CPU 中断，无需 PIEACK */
}
