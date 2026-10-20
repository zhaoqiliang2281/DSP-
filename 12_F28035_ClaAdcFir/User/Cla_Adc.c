/*
 * Cla_Adc.c
 *
 *  Created on: 2026年10月10日
 *      Author: liang2281
 */

#include "Cla_Adc.h"
#include "CLAShared.h"

//FIR测试代码部分

// Defines:
//      如果需要低通滤波器，设置 LOWPASS = 1
//      如果需要高通滤波器，设置 HIGHPASS = 1
//      宏定义自动选择对应的FIR滤波系数
//
#define LOWPASS             1
#define HIGHPASS            0
//
// ADC_SAMPLE_PERIOD 用于配置ePWM1增计数模式，TBPRD基于SYSCLKOUT二分频得到30MHz的ePWM时钟
//
//      ePWM每一个周期产生一次ADC SOC采样触发
//      目标采样率20KHz：
//      -> Tpwm = (TBPRD + 1) x (1/30 MHz)
//      -> TBPRD = (1/20KHz)x(30MHz) = 1500
//
// PWM_PERIOD 是ePWM3的周期，ePWM3产生测试方波送入ADC采样
//
// PWM_DUTY_CYCLE 是ePWM3的比较值，决定方波占空比
//
// ADC_BUF_LEN 采集多少组样本后暂停程序
//
// FILTER_LEN FIR滤波器阶数（在CLAShared.h中定义，本例为5阶FIR）
//
#define ADC_SAMPLE_PERIOD   1500
#define PWM_PERIOD          30000
#define PWM_DUTY_CYCLE      15000
#define ADC_BUF_LEN         200


//
// Prototype statements 函数原型声明
//
void init_epwm1(void);          //初始化ePWM1，用于产生ADC采样触发SOC
void init_epwm3(void);          //初始化ePWM3，用于产生测试方波信号
void init_cla(void);            //初始化CLA协处理器
void init_adc(void);            //初始化ADC模数转换器
//
// Prototype statements for interrupt functions found within
// this file.  The main CPU will service this interrupt every
// every time task 7 completes.
// CLA任务7执行完成后，触发该CPU中断服务函数
//
__interrupt void cla1_isr7(void);
//
// Global variables 全局变量

Uint16 SampleCount;// 当前采样计数
Uint16 AdcBuf[ADC_BUF_LEN];//缓存ADC RESULT寄存器读出的原始ADC采样值
Uint16 AdcFiltBuf[ADC_BUF_LEN];//缓存CLA FIR滤波器输出结果，数据通过CLA-CPU消息RAM传递
//
// The DATA_SECTION pragma statements are used to place the
// variables in specific assembly sections.  These sections
// are linked to the message RAMs for the CLA in the
// linker command (.cmd) file.
// #pragma 指定变量存放的内存段，在链接脚本.cmd中分配CLA消息RAM
//
// The following will be placed in the CLA to CPU
// message RAM.  The CLA可以读写该区域；主CPU仅可读
//
// X is the filter sample delay line.  These will
// be populated by the CLA from the ADC RESULT1 register
// X[]：FIR滤波器延迟线，存放历史采样值，由CLA写入更新
//
// VoltFilt is the output from the CLA after the FIR filter
// Basically it is the filtered value
// VoltFilt：CLA运算完成后的滤波输出结果
//
#pragma DATA_SECTION(X,          "Cla1ToCpuMsgRAM");
#pragma DATA_SECTION(VoltFilt,   "Cla1ToCpuMsgRAM");
float32 X[FILTER_LEN];
Uint16 VoltFilt;


#pragma DATA_SECTION(VoltageCLA, "Cla1ToCpuMsgRAM");
#pragma DATA_SECTION(ConversionCount,"Cla1ToCpuMsgRAM");

//
// The following will be placed in the CPU to CLA
// message RAM. 主CPU可读可写；CLA仅可读
//
// A is the filter coefficients passed by the main
// CPU to the CLA.
// A[]：FIR滤波器系数数组，CPU预先配置，CLA读取使用
//
#pragma DATA_SECTION(A,          "CpuToCla1MsgRAM");
#if LOWPASS
    //5阶低通FIR系数，系数总和=1，直流信号可以完整通过
    float32 A [FILTER_LEN] = {0.0625L, 0.25L, 0.375L, 0.25L, 0.0625L};
#elif HIGHPASS
    //5阶高通FIR系数，系数总和=0，直流分量会被滤除，仅信号边沿有输出
    float32 A [FILTER_LEN] = {0.0625L, -0.25L, 0.375L, -0.25L, 0.0625L};
//    float32 A [FILTER_LEN] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
#endif
//
// These are defined by the linker file and used to copy
// the CLA code from its load address to its run address
// in CLA program memory in the CLA initialization function
// 链接脚本定义的外部符号，用于将CLA汇编程序从Flash拷贝到CLA程序RAM运行
//
extern Uint16 Cla1funcsLoadStart;
extern Uint16 Cla1funcsLoadSize;
extern Uint16 Cla1funcsRunStart;

void Cla_Fir_Init(void)
{
    Uint16 i;
    //
    // Step 1. Initialize System Control:
    // PLL, WatchDog, enable Peripheral Clocks
    // This example function is found in the DSP2803x_SysCtrl.c file.
    // 步骤1：系统初始化，配置PLL锁相环、看门狗、开启外设时钟
    //
    InitSysCtrl();
    //
    // Step 2. Initialize GPIO:
    // This example function is found in the DSP2803x_Gpio.c file and
    // illustrates how to set the GPIO to it's default state.
    // 步骤2：GPIO初始化，恢复GPIO到默认状态
    //
    InitGpio();
    //
    // Specific setup for this example: 本工程专属GPIO配置
    //
    EALLOW;

    //
    // GPIO34 is an output (connected to LED on F28027 ControlStick)
    // GPIO34配置为输出，开发板LED引脚
    //
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;

    //
    // GPIO34 pin is set to 1 (turn LED on)
    // LED输出高电平，点亮LED
    //
    GpioDataRegs.GPBSET.bit.GPIO34 = 1;

    //
    // GPIO18 is an output (used for test - pin toggle)
    // GPIO18配置为输出，用于示波器观测中断触发频率，每次中断翻转电平
    //
    GpioCtrlRegs.GPADIR.bit.GPIO18 = 1;
    EDIS;
    //
    // Step 3. Clear all interrupts and initialize PIE vector table:
    // Disable CPU interrupts
    // 步骤3：关闭CPU全局中断，初始化PIE中断向量表
    //
    DINT;
    //
    // Initialize the PIE control registers to their default state.
    // The default state is all PIE interrupts disabled and flags
    // are cleared.
    // This function is found in the DSP2803x_PieCtrl.c file.
    // 初始化PIE中断控制器，所有中断关闭、中断标志清零
    //
    InitPieCtrl();
    //
    // Disable CPU interrupts and clear all CPU interrupt flags:
    // 关闭CPU中断，清除CPU中断标志寄存器
    //
    IER = 0x0000;
    IFR = 0x0000;
    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    // This will populate the entire table, even if the interrupt
    // is not used in this example.  This is useful for debug purposes.
    // The shell ISR routines are found in DSP2803x_DefaultIsr.c.
    // This function is found in DSP2803x_PieVect.c.
    // 初始化PIE中断向量表，填充默认中断服务函数
    //
    InitPieVectTable();
    //
    // Interrupts that are used in this example are re-mapped to
    // ISR functions found within this file.
    // EALLOW: is needed to write to EALLOW protected registers
    // EDIS: is needed to disable write to EALLOW protected registers
    // 将CLA1_INT7中断向量绑定到自定义中断服务函数cla1_isr7
    //
    EALLOW;
    PieVectTable.CLA1_INT7 = &cla1_isr7;
    EDIS;
    //
    // Step 5. User specific code, enable interrupts:
    // 开启PIE11组第7路中断(CLA1_INT7)，开启CPU INT11中断，开启全局中断与实时调试中断
    //
    PieCtrlRegs.PIEIER11.bit.INTx7 = 1;
    IER |= M_INT11;
    EINT;
    ERTM;
    //
    // Clear the output buffers 清空采样缓存数组
    //
    for (i = 0; i< ADC_BUF_LEN; i++)
    {
        AdcBuf[i] = 0x0000;
        AdcFiltBuf[i] = 0x0000;
    }
    init_cla();    //初始化CLA协处理器
    init_adc();    //初始化ADC模块
    //
    // Assumes ePWM1 clock is already enabled in InitSysCtrl();
    //
    // Before configuring the ePWMs, halt the counters
    // After configuration they can all be started again
    // in synchronization by setting this bit.
    // 配置PWM前，关闭ePWM时钟同步，暂停所有PWM计数器
    //
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;
    init_epwm1();  //初始化ePWM1，产生ADC采样触发信号
    init_epwm3();  //初始化ePWM3，产生测试方波
    //
    // Start the ePWM counters
    // Note: this should be done after all ePWM modules are configured
    // to ensure synchronization between the ePWM modules.
    // 全部ePWM配置完成后，开启时钟同步，启动所有PWM计数器
    //
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;
    //
    // The main CPU will receive an interrupt from the
    // CLA each time task 7 (CLA FIR filter) completes
    //
    // In the meantime the main CPU can do other work
    //
    // After the CLA has fired ADC_BUF_LEN interrupts,
    // the main CPU  will halt here
    //
    // You can view the results by graphing AdcBuf and
    // AdcFiltBuf.  Both are unsigned int arrays.
    //
    SampleCount = 0;
    InitSciA();    //初始化SCI串口，用于打印采样数据
}

void Cla_adc_Fir_test_Loop(void)
{

}

//
// cla1_isr7 - ISR for Task 7
// This interrupt will be taken by the main CPU when CLA task 7 completes.
// CLA Task7执行完成后触发该CPU中断服务函数
//
__interrupt void
cla1_isr7()
{
    //
    // Clear the ADC interrupt flag so the next SOC can occur
    // Clear the IACK bits so another interrupt
    // can be taken
    // Toggle GPIO for status
    //
    AdcRegs.ADCINTFLGCLR.bit.ADCINT7 = 1;  //清除ADCINT7中断标志，允许下一次ADC采样触发
    PieCtrlRegs.PIEACK.all = 0xFFFF;       //PIE中断应答，释放中断通道，接收下一次中断
    GpioDataRegs.GPATOGGLE.bit.GPIO18 = 1;//翻转GPIO18电平，示波器观测中断频率
    //
    // Read the raw ADC RESULT1 register value and
    // put it into the AdcBuf buffer
    // This can be compared to the CLA filtered value
    // 读取ADC原始采样值存入缓存数组
    //
    AdcBuf[SampleCount] = AdcResult.ADCRESULT1;
    //
    // Read the CLA filtered value and put it in the
    // AdcFiltBuf buffer
    // 读取CLA输出的滤波结果存入缓存数组
    //
    AdcFiltBuf[SampleCount] = VoltFilt;
    //=====在这里增加串口打印！每一次FIR完成打印一组数据=====
    //⚠️注意：20kHz中断内部调用串口发送，会出现串口数据包碎裂问题
    Serial_SendNumber(AdcResult.ADCRESULT1,5);
    Scia_SendString(",");
    Serial_SendNumber(VoltFilt,5);
    Scia_SendString("\n\r");
    //
    // Make sure that the buffer does not overflow
    // the buffer size.  If it is larger than ADC_BUF_LEN
    // then rewind back to the sample 0
    // 采样计数自增，到达缓存长度清零，防止数组溢出
    //
    SampleCount++;
    if( SampleCount == ADC_BUF_LEN )
    {
        SampleCount = 0;
    }
}
//
// init_adc - EPWM1 will be used to generate the ADC Start of conversion
// ADC初始化函数，由ePWM1产生ADC开始转换SOC信号
//
void
init_adc(void)
{
    //
    // Assumes ADC clock is already enabled in InitSysCtrl();
    //
    //
    // Call the InitAdc function in the DSP2803x_Adc.c file
    //
    // This function calibrates and powers up the ADC to
    // into a known state.
    // ADC上电初始化，执行ADC偏移自校准，消除ADC偏移误差
    //
    InitAdc();
//    AdcOffsetSelfCal();
    //
    // Specific ADC configuration for this example
    //
    // ADC interrupt will trigger early - before the ADC conversion starts
    // Enable ADCINT7
    // Disable ADC 7 continuous mode
    // Set the SOC1 channel select to ADCINTA2.
    // This interrupt triggers task 7
    // which is where the CLA FIR filter is located
    //
    EALLOW;
    //
    // ADC interrupt comes early (end of sample window)
    // SOC1 will trigger ADCINT7
    // Enable ADCINT7
    // Disable ADCINT7 Continuous mode
    // INTPULSEPOS=0：采样窗口结束就产生ADCINT7，不需要等ADC转换完成，用来唤醒CLA
    //
    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 0;
    AdcRegs.INTSEL7N8.bit.INT7SEL   = 1;
    AdcRegs.INTSEL7N8.bit.INT7E     = 1;    //使能ADCINT7中断
    AdcRegs.INTSEL7N8.bit.INT7CONT  = 0;    //关闭连续中断模式，每次SOC仅触发一次
    //
    // set SOC1 channel select to ADCINA2
    // set SOC1 start trigger on EPWM1A interrupt
    // set SOC1 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    // SOC1通道选择ADCINA2；触发源选择EPWM1A事件；采样保持窗口ACQPS=6
    //
    AdcRegs.ADCSOC1CTL.bit.CHSEL    = 2;//通道2
    AdcRegs.ADCSOC1CTL.bit.TRIGSEL  = 5;
    AdcRegs.ADCSOC1CTL.bit.ACQPS    = 6;
    EDIS;
}
//
// init_epwm1 - EPWM1 will be used to generate the ADC Start of conversion
// ePWM1初始化：用来周期性产生ADC SOC采样触发信号
//
void
init_epwm1(void)
{
    //
    // EALLOW: is needed to write to EALLOW protected registers
    // EDIS: is needed to disable write to EALLOW protected registers
    //
    // Enable start of conversion (SOC) on A
    // An SOC event will occur when the ePWM counter is zero
    // The ePWM will generate an SOC on the first event
    //
    EALLOW;
    //
    // Set the period for ePWM1
    // By default TBPRD = 1/2 SYSCLKOUT
    // Set the counter for up count mode
    // 配置ePWM周期，增计数模式
    //
    EPwm1Regs.TBPRD                 = ADC_SAMPLE_PERIOD;
    EPwm1Regs.TBCTL.bit.CTRMODE     = TB_COUNT_UP;
    EPwm1Regs.ETSEL.bit.SOCAEN  = 1;             //开启SOCA，允许产生ADC开始转换信号SOC
    EPwm1Regs.ETSEL.bit.SOCASEL = ET_CTR_ZERO;   //计数器归零时刻产生SOC触发信号
    EPwm1Regs.ETPS.bit.SOCAPRD  = ET_1ST;        //每一次事件都产生SOC
    //
    // For testing - monitor the EPWM1A output
    // which toggles once every ePWM period (i.e at the start of conversion)
    // GPIO0复用为EPWM1A，可以外接示波器观测采样触发信号
    //
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;
    EPwm1Regs.AQCTLA.bit.ZRO = AQ_TOGGLE;
    EPwm1Regs.TBCTL.bit.FREE_SOFT = 3; //仿真调试时PWM持续运行，不受断点暂停影响
    EDIS;
}
//
// init_epwm3 - ePWM3初始化，生成测试方波输出到GPIO4(EPWM3A)
//
void
init_epwm3(void)
{
    //
    // EALLOW: is needed to write to EALLOW protected registers
    // EDIS: is needed to disable write to EALLOW protected registers
    // Configure the GPIO4 pin to be EPWM3A output

    //
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;// GPIO4复用为EPWM3A输出引脚
    EDIS;
    //
    // Disable the timer (counter mode is halt)
    // Set the free/soft emulation bits to ignore the
    // emulation halt (PWM will continue to count when
    // the CPU is halted)

    EPwm3Regs.TBCTL.bit.CTRMODE = 0x3;// 配置仿真模式，CPU断点暂停时PWM仍然继续输出
    EPwm3Regs.TBCTL.bit.FREE_SOFT = 3;
    //
    // Clear the counter
    // Set the period and timer phase
    // Specify when the compare A event will occur
    // 计数器清零，配置周期、相位、比较值
    EPwm3Regs.TBCTR = 0x0000;
    EPwm3Regs.TBPRD = PWM_PERIOD;
    EPwm3Regs.TBPHS.half.TBPHS = 0x0000;
    EPwm3Regs.CMPA.half.CMPA = PWM_DUTY_CYCLE;
    //
    // On compare A, when counting up, pull the EPWM A output high
    // On compare A, when counting down, pull the EPWM A output low
    // Set the counter to up/down count mode
    // 增减计数模式，对称方波；增计数到CMPA置高，减计数到CMPA置低
    EPwm3Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm3Regs.AQCTLA.bit.CAD = AQ_CLEAR;
    EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
}
//
// init_cla - CLA module initialization
// 1) Enable the CLA clock
// 2) Init the CLA interrupt vectors
// 3) Allow the IACK instruction to flag a CLA interrupt/start a task
//    This is used to force Task 8 in this example
// 4) Copy CLA code from its load address to its run address
//    in CLA program memory
//
//    Note: during debug the load and run addresses could be
//    the same as Code Composer Studio can load the CLA program
//    RAM directly.
//
//    The ClafuncsLoadStart, ClafuncsLoadEnd, and ClafuncsRunStart
//    symbols are created by the linker.
// 4) Assign CLA program memory to the CLA
// 5) Enable CLA interrupts (MIER)
// CLA协处理器初始化函数
//
void
init_cla()
{
    //
    // This code assumes the CLA clock is already enabled in
    // the call to InitSysCtrl();
    //
    // EALLOW: is needed to write to EALLOW protected registers
    // EDIS: is needed to disable write to EALLOW protected registers
    //
    // Initialize the interrupt vectors for Task 7 (CLA FIR Filter)
    // and for Task 8 (FIR filter initialization)
    // 设置CLA任务7、任务8的程序入口向量
    //
    EALLOW;
    Cla1Regs.MVECT7 = (Uint16)((Uint32)&Cla1Task7 - (Uint32)&Cla1Prog_Start);
    Cla1Regs.MVECT8 = (Uint16)((Uint32)&Cla1Task8 - (Uint32)&Cla1Prog_Start);
    //
    // Task 7 has the option to be started by either EPWM7_INT or ADCINT7
    // In this case we will allow ADCINT7 to start CLA Task 7
    // 指定CLA Task7的触发源：ADCINT7信号
    //
    Cla1Regs.MPISRCSEL1.bit.PERINT7SEL = CLA_INT7_ADCINT7;
    //
    // Copy the CLA program code from its load address to the CLA program
    // memory. Once done, assign the program memory to the CLA
    //
    // Make sure there are at least two SYSCLKOUT cycles between assigning
    // the memory to the CLA and when an interrupt comes in
    // 将CLA汇编程序从Flash拷贝到CLA程序RAM中运行
    memcpy((uint16_t *)&Cla1funcsRunStart, (uint16_t *)&Cla1funcsLoadStart,
            (unsigned long)&Cla1funcsLoadSize);
    Cla1Regs.MMEMCFG.bit.PROGE = 1; //开启CLA程序内存访问权限
    //
    // Enable the IACK instruction to start a task
    // Enable the CLA interrupt 8 and interrupt 7
    // 开启IACK指令，允许软件强制启动CLA任务；使能CLA任务7、任务8中断
    //
    Cla1Regs.MCTL.bit.IACKE = 1;
    Cla1Regs.MIER.all = (M_INT8 | M_INT7);
    //
    // Force CLA task 8 using the IACK instruction
    // Task 8 will initialize the filter input delay
    // line to zero (X[0] - X[4]).
    //
    // No need to wait, the task will finish by the time
    // we configure the ePWM and ADC modules
    // 软件强制启动CLA Task8，作用：FIR延迟线数组X[]全部清零，仅上电执行一次
    //
    Cla1ForceTask8();
}







