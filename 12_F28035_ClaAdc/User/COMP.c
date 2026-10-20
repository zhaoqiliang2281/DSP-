/*
 * COMP.c
 *
 *  Created on: 2026年10月1日
 *      Author: liang2281
 */

#include "COMP.h"

void COMP2_Simple_Init(void)
{
    EALLOW;
        // 打开COMP2外设时钟
        SysCtrlRegs.PCLKCR3.bit.COMP2ENCLK = 1;
        // GPIO3 → COMP2OUT（F2803x 定义：0=GPIO,1=EPWM2B,2=SPISOMI-A,3=COMP2OUT）
        // 注意是 3 不是 2，写 2 会变成 SPI 的 SPISOMI-A
        GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 3;
        GpioCtrlRegs.GPADIR.bit.GPIO3 = 1;
        GpioCtrlRegs.GPAPUD.bit.GPIO3 = 1;

        // ============ 把 COMP2A 的模拟输入引脚切成"模拟"模式 ============
        // ★ 内部DAC 固定接在"反相输入(B端)"，所以外部信号必须走 A 端
        // AIO4  = ADCINA4 = COMP2A  ← 电位器要接在这里
        // AIO12 = ADCINB4 = COMP2B  （COMPSOURCE=0 时用不到它）
        // InitGpio() 会把 AIOMUX1 清 0（数字IO），必须写 2（模拟输入）
        GpioCtrlRegs.AIOMUX1.bit.AIO4  = 2;
        GpioCtrlRegs.AIOMUX1.bit.AIO12 = 2;

        // ================= COMPCTL：只放控制位，不要在这里写 DAC 值 =================
        Comp2Regs.COMPCTL.all = 0;              // 先清零，避免误写 QUALSEL/SYNCSEL/保留位
        Comp2Regs.COMPCTL.bit.COMPDACEN  = 1;   // 使能比较器 + 内部DAC
        Comp2Regs.COMPCTL.bit.COMPSOURCE = 0;   // ★0=反相输入接内部DAC(阈值)；1=接外部COMP2B(此时DAC断开!)
        Comp2Regs.COMPCTL.bit.CMPINV     = 0;   // 不翻转：COMP2A(电位器) > DAC → COMP2OUT 高
        Comp2Regs.COMPCTL.bit.SYNCSEL    = 0;   // 异步输出，响应最快（示波器观测用）
        Comp2Regs.COMPCTL.bit.QUALSEL    = 0;   // 异步时该项无效

        // ================= DAC 阈值：独立寄存器，10 位（0~1023）=================
        // VDAC ≈ DACVAL / 1024 × VDDA，约 3.22mV/LSB（VDDA=3.3V）
        // 512 → 约 1.65V，电位器转到中间附近就会翻转
        Comp2Regs.DACVAL.bit.DACVAL = 512;
    EDIS;
}


//void COMP2_Simple_Init(void)
//{
//    Uint16 compCtlVal;
//    EALLOW;
//        // 打开COMP2外设时钟
//        SysCtrlRegs.PCLKCR3.bit.COMP2ENCLK = 1;
//        // GPIO3 配置为 COMP2OUT 输出
//        GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 2;
//        GpioCtrlRegs.GPADIR.bit.GPIO3 = 1;
//
//        // 先读出现有COMPCTL的值
//        compCtlVal = Comp2Regs.COMPCTL.all;
//        compCtlVal &= ~(0x3F << 6);   // 清除原来DACVAL 6位
//        compCtlVal |=  (31 << 6);     // DACVAL=31 → 约1.62V
//
//        compCtlVal |= (1 << 0);       // COMPDACEN=1 开启内部DAC
//        compCtlVal |= (1 << 1);       // COMPSOURCE=1，外部输入COMP2B
//        compCtlVal &= ~(1 << 2);      // CMPINV=0，不反转
//        compCtlVal &= ~(1 << 3);      // SYNCSEL=0，异步
//
//        Comp2Regs.COMPCTL.all = compCtlVal;
//    EDIS;
//}


//void COMP2_Simple_Init(void)
//{
//    EALLOW;
//        // 打开COMP2外设时钟
//        SysCtrlRegs.PCLKCR3.bit.COMP2ENCLK = 1;
//        // GPIO3 配置为 COMP2OUT 输出，示波器观测
//        GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 2;
//        GpioCtrlRegs.GPADIR.bit.GPIO3 = 1;
//
//        // ========= COMP2 【内部DAC阈值模式】 =========
//        Comp2Regs.COMPCTL.bit.COMPDACEN = 1;    // 使能片内DAC，作为比较器一路输入
//        Comp2Regs.COMPCTL.bit.COMPSOURCE = 1;   // 外部模拟输入选择 COMP2B引脚
//        Comp2Regs.COMPCTL.bit.CMPINV = 0;       // 输出不反转：DAC电压 > COMP2B电压 → COMP2OUT高
//        Comp2Regs.COMPCTL.bit.SYNCSEL = 0;      // 异步输出，响应快，适合示波器测试
//
//        // 设置内部DAC阈值，0~4095，对应0~3.3V，这里举例设1.65V
////        Comp2Regs.COMPDAC.bit.DACVAL = 2048;
//    EDIS;
//}


//void COMP2_Simple_Init(void)
//{
//    EALLOW;
//        // 打开COMP2外设时钟，必须加
//        SysCtrlRegs.PCLKCR3.bit.COMP2ENCLK = 1;
//
//        // GPIO3 配置为 COMP2OUT 输出
//        GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 2;
//        GpioCtrlRegs.GPADIR.bit.GPIO3 = 1;
//
//        // ========= COMP2 寄存器 =========
//        Comp2Regs.COMPCTL.bit.COMPDACEN = 1;    // 比较器上电
//        Comp2Regs.COMPCTL.bit.COMPSOURCE = 1;   // COMPB 使用外部COMP2B引脚（27脚）
//        Comp2Regs.COMPCTL.bit.CMPINV = 0;       // 输出不反转：COMP2A > COMP2B → COMP2OUT高
//        Comp2Regs.COMPCTL.bit.SYNCSEL = 0;      // 异步输出，延迟最小，适合示波器测试
//    EDIS;
//}
