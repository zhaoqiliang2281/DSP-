/*
 * PWM.c
 *
 *  Created on: 2026年9月28日
 *      Author: liang2281
 */

#include "PWM.h"

interrupt void epwm1_isr(void);
interrupt void epwm3_isr(void);

void EPwm1_Init(void)
{
    EALLOW;
        SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;    // 关闭TBCLK同步，修改ePWM寄存器前必须关
        SysCtrlRegs.PCLKCR1.bit.EPWM1ENCLK = 1;   // 使能EPWM1模块时钟
    EDIS;
    EPwm1Regs.TBPRD = 6000 - 1;                 // 时基周期寄存器，计数0~5999，共6000个TBCLK
    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;  // 向上计数模式：从0递增到TBPRD后归零
    EPwm1Regs.TBCTL.bit.PRDLD = TB_SHADOW;       // TBPRD使用影子寄存器，在同步点加载
    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE; // 关闭ePWM同步信号
    EPwm1Regs.CMPA.half.CMPA = 3000;             // CMPA比较值 = 3000
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;  // CMPA使用影子模式
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;// 影子CMPA在计数器归0时更新到活动寄存器
    // 动作限定模块A通道（EPWM1A）
    EPwm1Regs.AQCTLA.bit.ZRO = AQ_SET;           // CTR=0（计数器归零）→ EPWM1A置高
    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;         // CTR=CMPA向上匹配 → EPWM1A拉低

    EPwm1Regs.AQCTLB.all = 0;    // B通道动作寄存器清零！死区模块接管B输出，AQ不要干预B

    // 死区配置，放在TB、CMP、AQ寄存器之后
    EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;  // 死区模块全开
    EPwm1Regs.DBCTL.bit.POLSEL   = DB_ACTV_HIC;     // A通道高有效，B通道互补翻转
    EPwm1Regs.DBCTL.bit.IN_MODE  = DBA_ALL;         // 死区输入源来自EPWM1A
    EPwm1Regs.DBRED = 60;   // 上升沿延迟(RED)  60个TBCLK
    EPwm1Regs.DBFED = 60;   // 下降沿延迟(FED)  60个TBCLK
    // TBCLK=60MHz，1TBCLK≈16.67ns，60个tick ≈ 1us死区时间
/*
    目标死区    寄存器数值
    200ns       12
    500ns       30
    1us         60
    1.5us       90
    2us         120
*/
// ========== ePWM事件触发ET 周期中断配置 ==========
    EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;    // 计数器归零触发中断
    EPwm1Regs.ETSEL.bit.INTEN  = 0;             // 使能ePWM中断
    EPwm1Regs.ETPS.bit.INTPRD  = ET_1ST;        // 每1次事件触发中断
    EPwm1Regs.ETCLR.bit.INT    = 1;             // 清除中断标志

    // GPIO配置：GPIO0复用为EPWM1A
    EALLOW;
        GpioCtrlRegs.GPAPUD.bit.GPIO0  = 0;      // 关闭GPIO0上拉
        GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;      // GPIO0功能选择：mode1 = EPWM1A
        GpioCtrlRegs.GPADIR.bit.GPIO0  = 1;      // GPIO方向设为输出
    EDIS;

    // GPIO1配置，复用为EPWM1B
    EALLOW;
        GpioCtrlRegs.GPAPUD.bit.GPIO1  = 0;
        GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 1;    // GPIO1 mode1: EPWM1B
        GpioCtrlRegs.GPADIR.bit.GPIO1  = 1;
    EDIS;

    EALLOW;
        SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;    // 开启TBCLK同步，ePWM开始计数输出PWM
    EDIS;

// ========== PIE中断向量配置 ==========
//    EALLOW;
//        PieVectTable.EPWM1_INT = &epwm1_isr;
//    EDIS;
//    IER |= M_INT1;
//    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;
//    EINT;
//    ERTM;

}
interrupt void epwm1_isr(void)
{
    // 在这里放控制算法代码

    EPwm1Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
// duty: 0~100 百分比
void EPWM1_SetDuty(float duty)
{
    if(duty < 1.0f) duty = 1.0f;
    if(duty > 99.0f) duty = 99.0f;
    uint16_t cmp_val = (uint16_t)((EPwm1Regs.TBPRD + 1) * duty / 100.0f);
    EPwm1Regs.CMPA.half.CMPA = cmp_val;
}


void EPwm3_Init(void)
{
    EALLOW;
        SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;    // 关闭TBCLK同步，修改ePWM寄存器前必须关
        SysCtrlRegs.PCLKCR1.bit.EPWM3ENCLK = 1;   // 使能EPWM3模块时钟
    EDIS;
    EPwm3Regs.TBPRD = 6000 - 1;                 // 时基周期寄存器，计数0~5999，共6000个TBCLK
    EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;  // 向上计数模式：从0递增到TBPRD后归零
    EPwm3Regs.TBCTL.bit.PRDLD = TB_SHADOW;       // TBPRD使用影子寄存器，在同步点加载
    EPwm3Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE; // 关闭ePWM同步信号
    EPwm3Regs.CMPA.half.CMPA = 3000;             // CMPA比较值 = 3000
    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;  // CMPA使用影子模式
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;// 影子CMPA在计数器归0时更新到活动寄存器
    // 动作限定模块A通道（EPWM3A）
    EPwm3Regs.AQCTLA.bit.ZRO = AQ_SET;           // CTR=0（计数器归零）→ EPWM3A置高
    EPwm3Regs.AQCTLA.bit.CAU = AQ_CLEAR;         // CTR=CMPA向上匹配 → EPWM3A拉低

    EPwm3Regs.AQCTLB.all = 0;    // B通道动作寄存器清零！死区模块接管B输出，AQ不要干预B

    // 死区配置，放在TB、CMP、AQ寄存器之后
    EPwm3Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;  // 死区模块全开
    EPwm3Regs.DBCTL.bit.POLSEL   = DB_ACTV_HIC;     // A通道高有效，B通道互补翻转
    EPwm3Regs.DBCTL.bit.IN_MODE  = DBA_ALL;         // 死区输入源来自EPWM3A
    EPwm3Regs.DBRED = 60;   // 上升沿延迟(RED)  60个TBCLK
    EPwm3Regs.DBFED = 60;   // 下降沿延迟(FED)  60个TBCLK
    // TBCLK=60MHz，1TBCLK≈16.67ns，60个tick ≈ 1us死区时间
/*
    目标死区    寄存器数值
    200ns       12
    500ns       30
    1us         60
    1.5us       90
    2us         120
*/
// ========== ePWM事件触发ET 周期中断配置 ==========
    EPwm3Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;    // 计数器归零触发中断
    EPwm3Regs.ETSEL.bit.INTEN  = 1;             // 使能ePWM中断
    EPwm3Regs.ETPS.bit.INTPRD  = ET_1ST;        // 每1次事件触发中断
    EPwm3Regs.ETCLR.bit.INT    = 1;             // 清除中断标志

    // GPIO配置：GPIO4复用为EPWM3A
    EALLOW;
        GpioCtrlRegs.GPAPUD.bit.GPIO4  = 0;      // 关闭GPIO4上拉
        GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;      // GPIO4功能选择：mode1 = EPWM3A
        GpioCtrlRegs.GPADIR.bit.GPIO4  = 1;      // GPIO方向设为输出
    EDIS;

    // GPIO5配置，复用为EPWM3B
    EALLOW;
        GpioCtrlRegs.GPAPUD.bit.GPIO5  = 0;
        GpioCtrlRegs.GPAMUX1.bit.GPIO5 = 1;    // GPIO5 mode1: EPWM3B
        GpioCtrlRegs.GPADIR.bit.GPIO5  = 1;
    EDIS;

    EALLOW;
        SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;    // 开启TBCLK同步，ePWM开始计数输出PWM
    EDIS;

// ========== PIE中断向量配置 ==========
    EALLOW;
        PieVectTable.EPWM3_INT = &epwm3_isr;
    EDIS;
    IER |= M_INT3;
    PieCtrlRegs.PIEIER3.bit.INTx3 = 1;
    EINT;
    ERTM;

}

interrupt void epwm3_isr(void)
{
    // 在这里放控制算法代码
    // EPwm3Regs.CMPA.half.CMPA = xxx; // 修改占空比
//    static float duty = 20.0f;
//    static uint16_t cnt = 0;
//
//    cnt++;
//    if(cnt >= 5000)
//    {
//        cnt = 0;
//        duty += 5.0f;
//        if(duty > 80) duty = 20;
//        EPWM3_SetDuty(duty);
//    }

//    num++;
    EPwm3Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}

// duty: 0~100 百分比
void EPWM3_SetDuty(float duty)
{
    if(duty < 1.0f) duty = 1.0f;
    if(duty > 99.0f) duty = 99.0f;
    uint16_t cmp_val = (uint16_t)((EPwm3Regs.TBPRD + 1) * duty / 100.0f);
    EPwm3Regs.CMPA.half.CMPA = cmp_val;
}


