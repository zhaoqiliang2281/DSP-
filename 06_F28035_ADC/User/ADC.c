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

    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 1;     // EOC在结果锁存后产生脉冲

    // SOC0：ADCINA0（PNP 80pin第18脚），软件触发
    AdcRegs.ADCSOC0CTL.bit.CHSEL = 0;        // CHSEL=0 → ADCINA0
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 0;      // 0 = 软件触发（配合ADCSOCFRC1）
    AdcRegs.ADCSOC0CTL.bit.ACQPS = 14;       // 采样保持 ACQPS+1个ADCCLK

    // ADC中断选择：SOC0的EOC驱动ADCINT1
    AdcRegs.INTSEL1N2.bit.INT1SEL = 0;
    AdcRegs.INTSEL1N2.bit.INT1E = 1;         // 使能ADCINT1
    AdcRegs.INTSEL1N2.bit.INT1CONT = 0;

    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;    // 清ADCINT1标志
    AdcRegs.ADCINTOVFCLR.bit.ADCINT1 = 1;    // 清除溢出标志
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
    return 0;
}

void ADC_Get_value(void)
{
    adc_result0 = ADC_Read_Channel(0);
    if(adc_result0 != 0xFFFF) // 没有超时，有效采样
    {
        adc_voltage = (float)adc_result0 * 3.3f / 4095.0f;
    }
    else
    {
        adc_voltage = -1; // 标记超时故障
    }
    OLED_ShowNum(3,1,adc_result0,5);
    OLED_ShowNum(4,1,(Uint16)(adc_voltage * 100),5);
//    ips200_show_int(60, 220, adc_result0, 5);
//    ips200_show_int(60, 250, (Uint16)(adc_voltage * 100), 5);
}
