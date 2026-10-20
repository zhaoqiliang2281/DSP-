/*
 * ADC.C
 *
 *  Created on: 2026年9月28日
 *      Author: liang2281
 */
#include "DSP2803x_Device.h"
#include "ADC.h"
#include "OLED.h"

// 全局变量
Uint16 adc_result0;   // ADCINA0采样结果
float adc_voltage;    // 转换后的电压值

Uint16 adc_result1;   // ADCINA0采样结果
float adc_voltage1;    // 转换后的电压值

/* 下面两路是调试用的：验证电位器到底接在 COMP2A 还是 COMP2B */
Uint16 adc_result2;   // ADCINA4（与 COMP2A 共用引脚）
float  adc_voltage2;
Uint16 adc_result3;   // ADCINB4（与 COMP2B 共用引脚）
float  adc_voltage3;

/*===========================================================================
 * ADC 零点偏移自校准
 *
 * 原理：把 VREFLO(0V) 通过内部 MUX 短接到 B5 输入，用它做一次转换。
 *       此时输入电压已知为 0V，转换出来的码值就是 ADC 的零点偏移。
 *       先人为加 +80 偏移量（保证测出的是正数），算出均值后写入
 *       ADCOFFTRIM，把偏移抵消掉。
 *
 * 返回 0 = 成功，1 = 转换超时失败
 *=========================================================================*/
static Uint16 ADC_OffsetSelfCal(void)
{
    Uint32 sum  = 0;
    Uint32 to;
    Uint16 i;
    Uint16 mean;
    Uint16 fail = 0;

    EALLOW;
    AdcRegs.ADCCTL1.bit.VREFLOCONV = 1;      // VREFLO 内部接到 B5 输入
    AdcRegs.ADCOFFTRIM.bit.OFFTRIM = 110;     // 先加 +110 人工偏移，保证测出正数
    AdcRegs.ADCSOC0CTL.bit.CHSEL   = 13;     // CHSEL=13 → ADCINB5
    AdcRegs.ADCSOC0CTL.bit.ACQPS   = 14;
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 0;      // 软件触发
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    AdcRegs.ADCINTOVFCLR.bit.ADCINT1 = 1;
    EDIS;

    DELAY_US(1000);                          // 等内部短接稳定

    for(i = 0; i < 16; i++)
    {
        EALLOW;
        AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
        AdcRegs.ADCSOCFRC1.bit.SOC0 = 1;
        EDIS;

        to = 0;
        while((AdcRegs.ADCINTFLG.bit.ADCINT1 == 0) && (to < 50000))
        {
            to++;
        }
        if(to >= 50000)                      // 超时，放弃校准
        {
            fail = 1;
            break;
        }

        AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
        sum += AdcResult.ADCRESULT0;
    }

    EALLOW;
    AdcRegs.ADCCTL1.bit.VREFLOCONV = 0;      // 恢复：B5 接回外部引脚
    if(fail == 0)
    {
        mean = (Uint16)(sum / 16);
        // 去掉 +110 人工偏移，再取反 → 得到抵消零点偏移的修正量
        AdcRegs.ADCOFFTRIM.bit.OFFTRIM = 110 - (int16)mean;
    }
    else
    {
        AdcRegs.ADCOFFTRIM.bit.OFFTRIM = 0;
    }
    AdcRegs.ADCSOC0CTL.bit.CHSEL = 0;        // SOC0 恢复为 ADCINA0
    EDIS;

    return fail;
}


void ADC_Init(void)
{
    // 打开ADC 外设时钟
    EALLOW;
        SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 1;
    EDIS;

    // 2) 调用 TI 标准初始化：执行 Device_cal 校准，并按正确时序给
    //    带隙(ADCBGPWD)、参考(ADCREFPWD)、内核(ADCPWDN)、使能(ADCENABLE) 上电
    //    注意：ADCCTL1 里这几个位都是 1 = 上电 / 0 = 掉电！
    InitAdc();

    // 3) 上电后必须等待模拟电路稳定（TI 要求至少 5ms）才能开始第一次转换
    DELAY_US(5000);

    // 4) 零点偏移自校准：这一步决定了 0V 输入时读数能不能接近 0
    ADC_OffsetSelfCal();

    EALLOW;
        // 5) 强制GPIO0切换为模拟功能（ADCINA0）
        GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;
        GpioCtrlRegs.GPADIR.bit.GPIO0 = 0;
        GpioCtrlRegs.GPAPUD.bit.GPIO0  = 1;      // 关闭内部上拉，避免引入额外偏移

        // 强制GPIO1切换为模拟功能（ADCINA1）
        GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 1;
        GpioCtrlRegs.GPADIR.bit.GPIO1 = 0;
        GpioCtrlRegs.GPAPUD.bit.GPIO1  = 1;      // 关闭内部上拉，避免引入额外偏移

        AdcRegs.ADCCTL1.bit.INTPULSEPOS = 1;     // EOC在结果锁存后产生脉冲

        // SOC0：ADCINA0（PNP 80pin第18脚），软件触发
        AdcRegs.ADCSOC0CTL.bit.CHSEL = 0;        // CHSEL=0 → ADCINA0
        AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 0;      // 0 = 软件触发（配合ADCSOCFRC1）
        AdcRegs.ADCSOC0CTL.bit.ACQPS = 14;       // 采样保持 ACQPS+1个ADCCLK

        // SOC0：ADCINA1（PNP 80pin第17脚），软件触发
        AdcRegs.ADCSOC1CTL.bit.CHSEL = 1;        // CHSEL=1 → ADCINA1
        AdcRegs.ADCSOC1CTL.bit.TRIGSEL = 0;      // 0 = 软件触发（配合ADCSOCFRC1）
        AdcRegs.ADCSOC1CTL.bit.ACQPS = 14;       // 采样保持 ACQPS+1个ADCCLK

        // SOC2：ADCINA4（与 COMP2A 共用引脚），软件触发
        AdcRegs.ADCSOC2CTL.bit.CHSEL = 4;        // CHSEL=4  → ADCINA4 / COMP2A
        AdcRegs.ADCSOC2CTL.bit.TRIGSEL = 0;      // 0 = 软件触发
        AdcRegs.ADCSOC2CTL.bit.ACQPS = 14;

        // SOC3：ADCINB4（与 COMP2B 共用引脚），软件触发
        AdcRegs.ADCSOC3CTL.bit.CHSEL = 12;       // CHSEL=12 → ADCINB4 / COMP2B
        AdcRegs.ADCSOC3CTL.bit.TRIGSEL = 0;      // 0 = 软件触发
        AdcRegs.ADCSOC3CTL.bit.ACQPS = 14;

        // SOC0完成触发ADCINT1，SOC1完成触发ADCINT2（两路各自中断）
        AdcRegs.INTSEL1N2.bit.INT1SEL = 0;       // ADCINT1来源SOC0
        AdcRegs.INTSEL1N2.bit.INT1E = 1;         // 使能ADCINT1

        AdcRegs.INTSEL1N2.bit.INT2SEL = 1;       // ADCINT2来源SOC1
        AdcRegs.INTSEL1N2.bit.INT2E = 1;         // 使能ADCINT2

        AdcRegs.INTSEL1N2.bit.INT1CONT = 0;
        AdcRegs.INTSEL1N2.bit.INT2CONT = 0;

        // SOC2完成触发ADCINT3，SOC3完成触发ADCINT4（调试用的 A4 / B4 两路）
        AdcRegs.INTSEL3N4.bit.INT3SEL = 2;       // ADCINT3来源SOC2
        AdcRegs.INTSEL3N4.bit.INT3E = 1;
        AdcRegs.INTSEL3N4.bit.INT3CONT = 0;
        AdcRegs.INTSEL3N4.bit.INT4SEL = 3;       // ADCINT4来源SOC3
        AdcRegs.INTSEL3N4.bit.INT4E = 1;
        AdcRegs.INTSEL3N4.bit.INT4CONT = 0;

        AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
        AdcRegs.ADCINTFLGCLR.bit.ADCINT2 = 1;
        AdcRegs.ADCINTFLGCLR.bit.ADCINT3 = 1;
        AdcRegs.ADCINTFLGCLR.bit.ADCINT4 = 1;
        AdcRegs.ADCINTOVFCLR.bit.ADCINT1 = 1;
        AdcRegs.ADCINTOVFCLR.bit.ADCINT2 = 1;
        AdcRegs.ADCINTOVFCLR.bit.ADCINT3 = 1;
        AdcRegs.ADCINTOVFCLR.bit.ADCINT4 = 1;
    EDIS;
}


Uint16 ADC_Read_Channel(Uint16 ch)
{
    if(ch == 0)
    {
        Uint32 timeout = 0;
        EALLOW;
        AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; // 先清ADCINT1标志
        AdcRegs.ADCSOCFRC1.bit.SOC0 = 1;      // 软件触发SOC0
        EDIS;

        // ===== 带超时轮询，防止卡死！最多等待50000次循环 =====
        while( (AdcRegs.ADCINTFLG.bit.ADCINT1 == 0) && (timeout < 50000) )
        {
            timeout++;
        }
        if(timeout >= 50000)
        {
            // 超时，ADC转换失败，返回0
            return 0xFFFF;
        }
        AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
        return AdcResult.ADCRESULT0;
    }
    if(ch == 1)
    {
        Uint32 timeout = 0;
        EALLOW;
        AdcRegs.ADCINTFLGCLR.bit.ADCINT2 = 1; // 先清ADCINT2标志
        AdcRegs.ADCSOCFRC1.bit.SOC1 = 1;      // 软件触发SOC1
        EDIS;

        // ===== 带超时轮询，防止卡死！最多等待50000次循环 =====
        while( (AdcRegs.ADCINTFLG.bit.ADCINT2 == 0) && (timeout < 50000) )
        {
            timeout++;
        }
        if(timeout >= 50000)
        {
            // 超时，ADC转换失败，返回0
            return 0xFFFF;
        }
        AdcRegs.ADCINTFLGCLR.bit.ADCINT2 = 1;
        return AdcResult.ADCRESULT1;
    }
    if(ch == 2)   // ADCINA4 / COMP2A
    {
        Uint32 timeout = 0;
        EALLOW;
        AdcRegs.ADCINTFLGCLR.bit.ADCINT3 = 1;
        AdcRegs.ADCSOCFRC1.bit.SOC2 = 1;
        EDIS;
        while( (AdcRegs.ADCINTFLG.bit.ADCINT3 == 0) && (timeout < 50000) )
        {
            timeout++;
        }
        if(timeout >= 50000)
        {
            return 0xFFFF;
        }
        AdcRegs.ADCINTFLGCLR.bit.ADCINT3 = 1;
        return AdcResult.ADCRESULT2;
    }
    if(ch == 3)   // ADCINB4 / COMP2B
    {
        Uint32 timeout = 0;
        EALLOW;
        AdcRegs.ADCINTFLGCLR.bit.ADCINT4 = 1;
        AdcRegs.ADCSOCFRC1.bit.SOC3 = 1;
        EDIS;
        while( (AdcRegs.ADCINTFLG.bit.ADCINT4 == 0) && (timeout < 50000) )
        {
            timeout++;
        }
        if(timeout >= 50000)
        {
            return 0xFFFF;
        }
        AdcRegs.ADCINTFLGCLR.bit.ADCINT4 = 1;
        return AdcResult.ADCRESULT3;
    }
    return 0;
}

void ADC_Get_value(void)
{
    adc_result0 = ADC_Read_Channel(0);
    adc_result1 = ADC_Read_Channel(1);
    if(adc_result0 != 0xFFFF) // 没有超时，有效采样
    {
        adc_voltage = (float)adc_result0 * 3.3f / 4095.0f;
    }
    else
    {
        adc_voltage = -1; // 标记超时故障
    }
    if(adc_result1 != 0xFFFF) // 没有超时，有效采样
    {
        adc_voltage1 = (float)adc_result1 * 3.3f / 4095.0f;
    }
    else
    {
        adc_voltage1 = -1; // 标记超时故障
    }
//    OLED_ShowNum(3,1,adc_result0,5);
//    OLED_ShowNum(4,1,(Uint16)(adc_voltage * 100),5);
    ips200_show_int(60, 220, adc_result0, 5);
    ips200_show_float(60, 240, adc_voltage, 3,5);

    ips200_show_int(60, 280, adc_result1, 5);
    ips200_show_float(60, 300, adc_voltage1, 3,5);

    /* 调试用：A4（COMP2A）和 B4（COMP2B）两个引脚的读数 */
    adc_result2 = ADC_Read_Channel(2);
    adc_result3 = ADC_Read_Channel(3);
    if(adc_result2 != 0xFFFF) { adc_voltage2 = (float)adc_result2 * 3.3f / 4095.0f; }
    else                      { adc_voltage2 = -1; }
    if(adc_result3 != 0xFFFF) { adc_voltage3 = (float)adc_result3 * 3.3f / 4095.0f; }
    else                      { adc_voltage3 = -1; }

    ips200_show_string(10, 180, "A4");
    ips200_show_int(60, 180, adc_result2, 5);
    ips200_show_string(10, 200, "B4");
    ips200_show_int(60, 200, adc_result3, 5);
}
