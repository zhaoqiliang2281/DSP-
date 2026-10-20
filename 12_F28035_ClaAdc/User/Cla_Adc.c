/*
 * Cla_Adc.c
 *
 *  Created on: 2026年10月10日
 *      Author: liang2281
 */

#include "Cla_Adc.h"

__interrupt void cla1_isr2(void);

// 放到 Cla1ToCpuMsgRAM 共享内存：CPU和CLA都能访问这块RAM交换数据
#pragma DATA_SECTION(ConversionCount, "Cla1ToCpuMsgRAM");
#pragma DATA_SECTION(VoltageCLA,      "Cla1ToCpuMsgRAM");

Uint16 ConversionCount;// CLA维护的采样点计数
Uint16 Cla_adc_test_LoopCount;// CPU空闲循环计数器，用于观测CPU负载
Uint16 VoltageCLA[NUM_DATA_POINTS];// 环形缓冲区，保存最近NUM_DATA_POINTS个ADC采样值

// 链接器生成的符号：CLA程序的加载地址、大小、运行地址
extern Uint16 Cla1funcsLoadStart;
extern Uint16 Cla1funcsLoadSize;
extern Uint16 Cla1funcsRunStart;
extern Uint16 Cla1Prog_Start;
extern Uint16 Cla1Task2;
extern Uint16 Cla1Task8;


void Cla_adc_test_Init(void)
{
    EALLOW;             // This is needed to write to EALLOW protected register
    PieVectTable.CLA1_INT2 = &cla1_isr2;// 重映射CLA1_INT2中断服务函数
    EDIS;      // This is needed to disable write to EALLOW protected registers

    InitAdc();         // For this example, init the ADC
//    DELAY_US(5000);   // !!!必须加5ms延时，给模拟部分上电稳定时间
//    AdcOffsetSelfCal();

    /*
     * 官方例程这里是 AdcOffsetSelfCal()：它内部的 AdcConversion() 用 ADCINT1/2 做乒乓
     * 采样自校准，在本工程里会永久停在 while (AdcRegs.ADCINTFLG.bit.ADCINT1 == 0){}
     * 上（ADC 始终没有产生 EOC6/ADCINT1），现象就是程序卡死，所以先不调用它。
     * ADC 偏置校准值已由 InitAdc() 里的 ROM 校准（Device_cal）写入 ADCOFFTRIM；
     * 想要更精确的偏置补偿，可换成带超时的简化自校准。
     */
//    AdcOffsetSelfCal();

    PieCtrlRegs.PIEIER11.bit.INTx2 = 1; // Enable INT 11.2 in PIE (CLA Task2)
    IER |= M_INT11;                     // Enable CPU Interrupt 11
    EINT;                               // Enable Global interrupt INTM
    ERTM;                              // Enable Global realtime interrupt DBGM

    // 将CLA代码从Flash拷贝到CLA程序RAM（CLA只能在自己的Program RAM中运行，不能直接跑Flash）
    memcpy((uint16_t *)&Cla1funcsRunStart,(uint16_t *)&Cla1funcsLoadStart,
            (unsigned long)&Cla1funcsLoadSize);


    //
    // Initialize the CLA registers
    //
    EALLOW;
    // MVECT2：CLA任务2入口地址偏移；MVECT8任务8入口
    Cla1Regs.MVECT2 = (Uint16)((Uint32)&Cla1Task2 - (Uint32)&Cla1Prog_Start);
    Cla1Regs.MVECT8 = (Uint16)((Uint32)&Cla1Task8 - (Uint32)&Cla1Prog_Start);

    //
    // ADCINT2 starts Task 2
    //
    // 把CLA任务2的触发源选为 ADCINT2
    Cla1Regs.MPISRCSEL1.bit.PERINT2SEL = CLA_INT2_ADCINT2;

    Cla1Regs.MMEMCFG.bit.PROGE = 1;  // Map CLA program memory to the CLA// 使能CLA程序存储器
    Cla1Regs.MCTL.bit.IACKE = 1;     // Enable IACK to start tasks via software// 允许软件IACK启动CLA任务
    Cla1Regs.MIER.all = (M_INT8 | M_INT2);   // Enable Task 8 and Task 2// 使能CLA Task8、Task2
    Cla1ForceTask8andWait();    // Force CLA task 8.// 强制运行Task8，初始化ConversionCount=0（在CLA代码里实现）
                                // This will initialize ConversionCount to zero

    //
    // ADCINT trips when ADC begins conversion
    //
    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 0;// 中断脉冲在转换开始时产生（原版例程特性，一般工程常用1：转换结束产生）

    AdcRegs.INTSEL1N2.bit.INT2E = 1;    //Enable ADCINT2// 使能ADCINT2
    AdcRegs.INTSEL1N2.bit.INT2CONT = 0; //Disable ADCINT2 Continuous mode// 单次模式，不是连续触发
    AdcRegs.INTSEL1N2.bit.INT2SEL = 1;  //setup EOC1 to trigger ADCINT2 to fire// EOC1（SOC1转换结束）触发ADCINT2
    AdcRegs.ADCSOC1CTL.bit.CHSEL = 2;   //set SOC1 channel select to ADCINA2// SOC1通道 ADCINA2
    AdcRegs.ADCSOC1CTL.bit.TRIGSEL = 5;  //set SOC1 start trigger on EPWM1A// SOC触发源：EPWM1A

    //
    // set SOC1 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    //
    AdcRegs.ADCSOC1CTL.bit.ACQPS    = 6;    // 采样保持窗口：ACQPS+1=7个ADC时钟周期
    EDIS;

      // Assumes ePWM1 clock is already enabled in InitSysCtrl();
      //
      EALLOW;
      SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;// 关闭TBCLK同步，修改TBPRD前建议关闭
      EDIS;
      EPwm1Regs.ETSEL.bit.SOCAEN  = 1;    // Enable SOC on A group// 开启EPWM SOCA（ADC启动信号）
      EPwm1Regs.ETSEL.bit.SOCASEL = 4;    // Select SOC from from CPMA on upcount// SOCA触发事件：CTR=CMPA，增计数时
      EPwm1Regs.ETPS.bit.SOCAPRD  = 1;    // Generate pulse on 1st event// 每次事件都产生SOC脉冲
      EPwm1Regs.CMPA.half.CMPA = 0x2EE;   // Set compare A value// CMPA比较值

      //
      // Set period for ePWM1 - this will determine the sampling frequency(20KHz)
      //
      EPwm1Regs.TBPRD = 0x5DC;// 时基周期，决定PWM周期=采样频率20kHz

      EPwm1Regs.TBCTL.bit.CTRMODE     = 0;        // count up and start// 增计数模式
      EALLOW;
      SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;// 重新同步TBCLK
      EDIS;

      Cla_adc_test_LoopCount = 0;
}

void Cla_adc_test_Loop(void)
{
    Uint16 i;

    Cla_adc_test_LoopCount++;
    ips200_show_int(60, 200, Cla_adc_test_LoopCount, 8);

    /*
     * CLA Task2 每采一个点就把 ConversionCount 加 1，并把 ADCRESULT1 写进 VoltageCLA[]；
     * 这里把 20 个原始采样值(0~4095)按 4 列 x 5 行显示出来，
     * Cnt 是 CLA 当前写入位置(0~19)，它循环变化就说明 CLA 在运行。
     */
    ips200_show_string(10, 218, "Cnt");
    ips200_show_int(38, 218, ConversionCount, 2);

    for(i = 0; i < NUM_DATA_POINTS; i++)
    {
        ips200_show_int(10 + (i % 4) * 56, 238 + (i / 4) * 16, VoltageCLA[i], 4);
    }
}

__interrupt void cla1_isr2()
{
    //
    // Clear ADCINT2 flag reinitialize for next SOC
    //
    AdcRegs.ADCINTFLGCLR.bit.ADCINT2 = 1;   // 清除ADCINT2中断标志，解锁下一次触发
    PieCtrlRegs.PIEACK.all = 0xFFFF;// PIE应答，释放PIE中断
}








