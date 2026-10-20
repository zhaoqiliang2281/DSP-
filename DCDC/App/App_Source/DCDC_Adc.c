/*
 * DCDC_Adc.c
 *
 *  Created on: 2025年6月12日
 *      Author: ThinkPad
 */
#ifndef ADC_C
#define ADC_C

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"
//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------
#define ADC_usDELAY 1000L
/*与CLA通讯数据*/
#pragma DATA_SECTION(uwLoopOutData,"Cla1ToCpuMsgRAM");
Uint16 uwLoopOutData[16];
#pragma DATA_SECTION(g_strClaOut_AdcPool,"Cla1ToCpuMsgRAM");
Struct_AdcPool_Int g_strClaOut_AdcPool;
#pragma DATA_SECTION(g_strClaIn_AdcRawData,"CpuToCla1MsgRAM");
Struct_AdcRawData g_strClaIn_AdcRawData;
///////
Union_Flag_Adc unAdcFlag;
Struct_AdcData g_strAdcData[eAdcOtherEnd];

Str_NTC NTC_Buck;
Str_NTC NTC_Boost;

//-----------------------------------------------------------------------------
//Adding Private Function
//-----------------------------------------------------------------------------
static inline void Adc_DataProcess(void);
static inline void Adc_CalSampleBias(void);
static inline void Adc_CalRmsOrAve(void);
/******************************************************************************
Function Name:
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Adc_System_AppInit(void)
{

}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:  50us
******************************************************************************/
#pragma CODE_SECTION(Adc_Task1_RealTimeDone,"ramfuncs");
void Adc_Task1_RealTimeDone(void)
{
    if(FALSE == unAdcFlag.bit.bCalcBiasFinish)
        return;

    // 读取ADC采样值
    g_strClaIn_AdcRawData.udIbuckboostAdcVal = AdcResult.ADCRESULT0;
    g_strClaIn_AdcRawData.udIbatAdcVal = AdcResult.ADCRESULT3;
    g_strClaIn_AdcRawData.udVbatAdcVal = AdcResult.ADCRESULT1;
    g_strClaIn_AdcRawData.udVbusAdcVal = AdcResult.ADCRESULT2;
    g_strClaIn_AdcRawData.udVbatRelayAdcVal = AdcResult.ADCRESULT4;
    g_strClaIn_AdcRawData.udVportAdcVal = AdcResult.ADCRESULT5;
                                           // 温度ADCRESULT6  ADCRESULT7在其它地方采样
    // 启动CLA任务1 开始计算
    Cla1ForceTask1andWait();
//    while(Cla1Regs.MIRUN.bit.INT1 == 1);

    // 取出CLA计算后的值
    g_strAdcData[eVbus].dRealVal = g_strClaOut_AdcPool.dVbusRealVal;
    g_strAdcData[eVbat_Relay].dRealVal = g_strClaOut_AdcPool.dVbatRelayRealVal;
    g_strAdcData[eVport].dRealVal = g_strClaOut_AdcPool.dVportRealVal;
    g_strAdcData[eVbat].dRealVal = g_strClaOut_AdcPool.dVbatRealVal;
    g_strAdcData[eIbuckboost].dRealVal = g_strClaOut_AdcPool.dIbuckboostRealVal;
    g_strAdcData[eIbat].dRealVal = g_strClaOut_AdcPool.dIbatRealVal;

    g_strAdcData[ePbat].dRealVal = g_strClaOut_AdcPool.dPbatRealVal;

}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
#pragma CODE_SECTION(Adc_Task2_RealTimeDone,"ramfuncs");
void Adc_Task2_RealTimeDone(void) // 100us
{
    Adc_DataProcess();
    Adc_CalSampleBias();  // 未运行的时候偏移采集
}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Adc_Task_5ms(void)
{
    Adc_CalRmsOrAve();
}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Adc_Task_20ms(void)
{

}

void Adc_Task_1ms(void)
{
    static Uint16 suwNtcRmsCnt = 0;

    NTC_Buck.dRealVal = AdcResult.ADCRESULT7;
    NTC_Boost.dRealVal = AdcResult.ADCRESULT6;

    if(FALSE == unAdcFlag.bit.bCalcBiasFinish)
        return;

    NTC_Buck.dRealSum += NTC_Buck.dRealVal;
    NTC_Boost.dRealSum += NTC_Boost.dRealVal;

    suwNtcRmsCnt++;
    if(suwNtcRmsCnt >= 100) //100ms  // 200次滤波
    {
        NTC_Buck.dRealSumBak = NTC_Buck.dRealSum;
        NTC_Boost.dRealSumBak = NTC_Boost.dRealSum;
        NTC_Buck.dRealSum = 0;
        NTC_Boost.dRealSum = 0;
        suwNtcRmsCnt = 0;
        unAdcFlag.bit.bCalcRmsFlagNtc = TRUE;
    }

}



/******************************************************************************
Function Name:
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:  100us调用
******************************************************************************/
static inline void Adc_DataProcess(void) 
{
    static Uint16 suwCalcDataRmsCnt = 0;
	static Uint16 suwCalcDataRmsCntBitFilter = 0;

    if(FALSE == unAdcFlag.bit.bCalcBiasFinish)
        return;

    g_strAdcData[eIbat].dRealSum += g_strAdcData[eIbat].dRealVal;
    g_strAdcData[eVbat].dRealSum += g_strAdcData[eVbat].dRealVal;
    g_strAdcData[eVbat_Relay].dRealSum += g_strAdcData[eVbat_Relay].dRealVal;
    g_strAdcData[eVport].dRealSum += g_strAdcData[eVport].dRealVal;
    g_strAdcData[eVbus].dRealSum += g_strAdcData[eVbus].dRealVal;
    g_strAdcData[eIbuckboost].dRealSum += g_strAdcData[eIbuckboost].dRealVal;

    suwCalcDataRmsCnt++;
    if(suwCalcDataRmsCnt >= 200)//20ms  // 200次滤波
    {
        g_strAdcData[eIbat].dRealSumBak = g_strAdcData[eIbat].dRealSum;
        g_strAdcData[eVbat].dRealSumBak = g_strAdcData[eVbat].dRealSum;
        g_strAdcData[eVbat_Relay].dRealSumBak = g_strAdcData[eVbat_Relay].dRealSum;
        g_strAdcData[eVport].dRealSumBak = g_strAdcData[eVport].dRealSum;
        g_strAdcData[eVbus].dRealSumBak = g_strAdcData[eVbus].dRealSum;
        g_strAdcData[eIbuckboost].dRealSumBak = g_strAdcData[eIbuckboost].dRealSum;
        g_strAdcData[eIbat].dRealSum = 0;
        g_strAdcData[eVbat].dRealSum = 0;
        g_strAdcData[eVbat_Relay].dRealSum = 0;
        g_strAdcData[eVport].dRealSum = 0;
        g_strAdcData[eVbus].dRealSum = 0;
        g_strAdcData[eIbuckboost].dRealSum = 0;
        suwCalcDataRmsCnt = 0;
        unAdcFlag.bit.bCalcRmsFlag = TRUE;
    }

	
    g_strAdcData[eIbat].dRealSumBitFilter += g_strAdcData[eIbat].dRealVal;
    g_strAdcData[eVbat].dRealSumBitFilter += g_strAdcData[eVbat].dRealVal;
    g_strAdcData[eVbat_Relay].dRealSumBitFilter += g_strAdcData[eVbat_Relay].dRealVal;
    g_strAdcData[eVport].dRealSumBitFilter += g_strAdcData[eVport].dRealVal;
    g_strAdcData[eVbus].dRealSumBitFilter += g_strAdcData[eVbus].dRealVal;
    g_strAdcData[eIbuckboost].dRealSumBitFilter += g_strAdcData[eIbuckboost].dRealVal;

    g_strAdcData[eIbat].ADC_SumBitFilter += g_strClaIn_AdcRawData.udIbatAdcVal;  // ADC未校准的值
    g_strAdcData[eVbat].ADC_SumBitFilter += g_strClaIn_AdcRawData.udVbatAdcVal;  // ADC未校准的值

//    int32 ADC_SumBitFilter;       //累计值 循环累加2000次，一次100us
//    int32 ADC_SumBakBitFilter;    //累计值备份 2000次累加后取值
//    int32 ADC_BitFilter;        //有效值     累加2000次后/2000

    suwCalcDataRmsCntBitFilter++;
    if(suwCalcDataRmsCntBitFilter >= 2000)//200ms  // 2000次滤波
    {
        g_strAdcData[eIbat].ADC_SumBakBitFilter = g_strAdcData[eIbat].ADC_SumBitFilter;
        g_strAdcData[eVbat].ADC_SumBakBitFilter = g_strAdcData[eVbat].ADC_SumBitFilter;

        g_strAdcData[eIbat].dRealSumBakBitFilter = g_strAdcData[eIbat].dRealSumBitFilter;
        g_strAdcData[eVbat].dRealSumBakBitFilter = g_strAdcData[eVbat].dRealSumBitFilter;
        g_strAdcData[eVbat_Relay].dRealSumBakBitFilter = g_strAdcData[eVbat_Relay].dRealSumBitFilter;
        g_strAdcData[eVport].dRealSumBakBitFilter = g_strAdcData[eVport].dRealSumBitFilter;
        g_strAdcData[eVbus].dRealSumBakBitFilter = g_strAdcData[eVbus].dRealSumBitFilter;
        g_strAdcData[eIbuckboost].dRealSumBakBitFilter = g_strAdcData[eIbuckboost].dRealSumBitFilter;
        g_strAdcData[eIbat].dRealSumBitFilter = 0;
        g_strAdcData[eVbat].dRealSumBitFilter = 0;
        g_strAdcData[eVbat_Relay].dRealSumBitFilter = 0;
        g_strAdcData[eVport].dRealSumBitFilter = 0;
        g_strAdcData[eVbus].dRealSumBitFilter = 0;
        g_strAdcData[eIbuckboost].dRealSumBitFilter = 0;
        g_strAdcData[eIbat].ADC_SumBitFilter = 0;
        g_strAdcData[eVbat].ADC_SumBitFilter = 0;
        suwCalcDataRmsCntBitFilter = 0;
        unAdcFlag.bit.bCalcRmsFlagBitFilter = TRUE;
    }

}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents: 5ms任务调用
******************************************************************************/
static inline void Adc_CalRmsOrAve(void)
{
    int32 temp32;

    if(TRUE == unAdcFlag.bit.bCalcRmsFlag)
    {
        g_strAdcData[eVbus].dRmsVal = g_strAdcData[eVbus].dRealSumBak/200;
        g_strAdcData[eVbat_Relay].dRmsVal = g_strAdcData[eVbat_Relay].dRealSumBak/200;
        g_strAdcData[eVport].dRmsVal = g_strAdcData[eVport].dRealSumBak/200;
        g_strAdcData[eVbat].dRmsVal = g_strAdcData[eVbat].dRealSumBak/200;
        g_strAdcData[eIbuckboost].dRmsVal = g_strAdcData[eIbuckboost].dRealSumBak/200;
        g_strAdcData[eIbat].dRmsVal = g_strAdcData[eIbat].dRealSumBak/200;

        unAdcFlag.bit.bCalcRmsFlag = FALSE;
    }

	if(TRUE == unAdcFlag.bit.bCalcRmsFlagBitFilter )
	{
        g_strAdcData[eVbus].dRmsValBitFilter = g_strAdcData[eVbus].dRealSumBakBitFilter/2000;
        g_strAdcData[eVbat_Relay].dRmsValBitFilter = g_strAdcData[eVbat_Relay].dRealSumBakBitFilter/2000;
        g_strAdcData[eVport].dRmsValBitFilter = g_strAdcData[eVport].dRealSumBakBitFilter/2000;
        g_strAdcData[eVbat].dRmsValBitFilter = g_strAdcData[eVbat].dRealSumBakBitFilter/2000;
        g_strAdcData[eIbuckboost].dRmsValBitFilter = g_strAdcData[eIbuckboost].dRealSumBakBitFilter/2000;
        g_strAdcData[eIbat].dRmsValBitFilter = g_strAdcData[eIbat].dRealSumBakBitFilter/2000;

        g_strAdcData[eIbat].ADC_BitFilter = g_strAdcData[eIbat].ADC_SumBakBitFilter/2000;
        g_strAdcData[eVbat].ADC_BitFilter = g_strAdcData[eVbat].ADC_SumBakBitFilter/2000;

		unAdcFlag.bit.bCalcRmsFlagBitFilter = FALSE;

		if(canSetPra.getBufRefFlag<5)
		{
		    canSetPra.getBufRefFlag++;
		    canSetPra.bufRef = g_strAdcData[eVbus].dRmsValBitFilter;
		}
    }

    if(TRUE == unAdcFlag.bit.bCalcRmsFlagNtc ) // ntc
    {
        temp32 = NTC_Buck.dRealSumBak/100;
        NTC_Buck.dRmsVal = transAdcToT((Uint16)temp32);
        temp32 = NTC_Boost.dRealSumBak/100;
        NTC_Boost.dRmsVal = transAdcToT((Uint16)temp32);

		FansControl(NTC_Buck.dRmsVal,NTC_Boost.dRmsVal);

        unAdcFlag.bit.bCalcRmsFlagNtc = FALSE;
    }
}


/******************************************************************************
Function Name:
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
static inline void Adc_CalSampleBias(void)
{
    static Uint32 sudCalcBiasCnt = 0;
    static Uint16 suwDelaySampCnt = 0;

    if(TRUE == unAdcFlag.bit.bCalcBiasFinish)
        return;


    if(suwDelaySampCnt < TSK5S_TIMER100US)//start Sample after Power up 5s
    {
        suwDelaySampCnt++;
        return;
    }

    if(sudCalcBiasCnt >= 32768)//2^15
    {
        g_strAdcData[eIbuckboost].dAdcBiasCoeff = g_strAdcData[eIbuckboost].dRealSum >> 15;
        g_strAdcData[eIbat].dAdcBiasCoeff = g_strAdcData[eIbat].dRealSum >> 15;
        g_strAdcData[eIbat].dRealSum = 0;
        g_strAdcData[eIbuckboost].dRealSum = 0;

        unAdcFlag.bit.bCalcBiasFinish = TRUE;
        sudCalcBiasCnt = 0;

        g_strClaIn_AdcRawData.udIbuckboostAdcVal = g_strAdcData[eIbuckboost].dAdcBiasCoeff;
        g_strClaIn_AdcRawData.udIbatAdcVal = g_strAdcData[eIbat].dAdcBiasCoeff;

        Cla1ForceTask8();//初始化CLA文件相关参数

    }
    else
    {
        g_strAdcData[eIbuckboost].dRealSum += AdcResult.ADCRESULT0;
        g_strAdcData[eIbat].dRealSum += AdcResult.ADCRESULT3;
        sudCalcBiasCnt++;
    }
}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Adc_System_PeripheralInit(void)
{
    extern void DSP28x_usDelay(Uint32 Count);

    //
    // *IMPORTANT*
    // The Device_cal function, which copies the ADC calibration values from TI
    // reserved OTP into the ADCREFSEL and ADCOFFTRIM registers, occurs
    // automatically in the Boot ROM. If the boot ROM code is bypassed during
    // the debug process, the following function MUST be called for the ADC to
    // function according to specification. The clocks to the ADC MUST be
    // enabled before calling this function.
    // See the device data manual and/or the ADC Reference
    // Manual for more information.
    //

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 1;  // 打开ADC时钟
    (*Device_cal)();
    EDIS;

    //
    // To powerup the ADC the ADCENCLK bit should be set first to enable
    // clocks, followed by powering up the bandgap, reference circuitry,
    // and ADC core. Before the first conversion is performed a 5ms delay must
    // be observed after power up to give all analog circuits time to power up
    // and settle
    //

    //
    // Please note that for the delay function below to operate correctly the
    // CPU_RATE define statement in the DSP2803x_Examples.h file must
    // contain the correct CPU clock period in nanoseconds.
    //
    EALLOW;

    // 打开ADC模拟电压供电
    AdcRegs.ADCCTL1.bit.ADCBGPWD  = 1;      // Power ADC BG
    AdcRegs.ADCCTL1.bit.ADCREFPWD = 1;      // Power reference
    AdcRegs.ADCCTL1.bit.ADCPWDN   = 1;      // Power ADC
    AdcRegs.ADCCTL1.bit.ADCENABLE = 1;      // Enable ADC

    // 打开ADC基准源，这里我们选择外部基准源VREF
    AdcRegs.ADCCTL1.bit.ADCREFSEL = 1;      // Select external BG
    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 1;    //ADCINT1 trips after AdcResults latch
    AdcRegs.ADCCTL2.bit.CLKDIV2EN = 1;       // ADC clock = CPU clock/2 or ADC clock = CPU clock/4

    EDIS;

    DELAY_US(ADC_usDELAY);         // Delay before converting ADC channels

    EALLOW;
    AdcRegs.ADCSAMPLEMODE.all = 0;            // Sequential Sampling Mode
    AdcRegs.INTSEL1N2.bit.INT1E = 1;           //Enabled ADCINT1
    AdcRegs.INTSEL1N2.bit.INT1CONT = 0;    //Disable ADCINT1 Continuous mode
    AdcRegs.INTSEL1N2.bit.INT1SEL = 0x05;    //setup EOC5 to trigger ADCINT1 to fire
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //make sure INT1 flag is cleared
    EDIS;

    DELAY_US(ADC_usDELAY);         // Delay before converting ADC channels

    EALLOW;

    // 采样通道选择
    AdcRegs.ADCSOC0CTL.bit.CHSEL = 0x1; //set SOC0 channel select to A1 IBuckBoost  电感电流
    AdcRegs.ADCSOC1CTL.bit.CHSEL = 0x2; //set SOC1 channel select to A2 Vbat        电池电压
    AdcRegs.ADCSOC2CTL.bit.CHSEL = 0xC; //set SOC2 channel select to B4 Vbus        母线电压
    AdcRegs.ADCSOC3CTL.bit.CHSEL = 0x4; //set SOC3 channel select to A4 Ibat        电池电流
    AdcRegs.ADCSOC4CTL.bit.CHSEL = 0xA; //set SOC4 channel select to B2 Vbat_Realy  继电器前端电压
    AdcRegs.ADCSOC5CTL.bit.CHSEL = 0xB; //set SOC5 channel select to B3 Vport       端口电压
    AdcRegs.ADCSOC6CTL.bit.CHSEL = 0x8; //set SOC6 channel select to B0 NTC_Boost   NTC_Boost
    AdcRegs.ADCSOC7CTL.bit.CHSEL = 0x9; //set SOC7 channel select to B1 NTC_Buck    NTC_Buck

    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 0x09; //set SOC0 start trigger on EPWM3A 采样触发源的选择
    AdcRegs.ADCSOC1CTL.bit.TRIGSEL = 0x09; //set SOC0 start trigger on EPWM3A
    AdcRegs.ADCSOC2CTL.bit.TRIGSEL = 0x09; //set SOC0 start trigger on EPWM3A
    AdcRegs.ADCSOC3CTL.bit.TRIGSEL = 0x09; //set SOC0 start trigger on EPWM3A
    AdcRegs.ADCSOC4CTL.bit.TRIGSEL = 0x09; //set SOC0 start trigger on EPWM3A
    AdcRegs.ADCSOC5CTL.bit.TRIGSEL = 0x09; //set SOC0 start trigger on EPWM3A
    AdcRegs.ADCSOC6CTL.bit.TRIGSEL = 0x09; //set SOC0 start trigger on EPWM3A
    AdcRegs.ADCSOC7CTL.bit.TRIGSEL = 0x09; //set SOC0 start trigger on EPWM3A

    AdcRegs.ADCSOC0CTL.bit.ACQPS    = 6; //set SOC0 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    AdcRegs.ADCSOC1CTL.bit.ACQPS    = 6; //set SOC0 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    AdcRegs.ADCSOC2CTL.bit.ACQPS    = 6; //set SOC0 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    AdcRegs.ADCSOC3CTL.bit.ACQPS    = 10; //set SOC0 S/H Window to 11 ADC Clock Cycles, (10 ACQPS plus 1)
    AdcRegs.ADCSOC4CTL.bit.ACQPS    = 6; //set SOC0 S/H Window to 11 ADC Clock Cycles, (6 ACQPS plus 1)
    AdcRegs.ADCSOC5CTL.bit.ACQPS    = 6; //set SOC0 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    AdcRegs.ADCSOC6CTL.bit.ACQPS    = 6; //set SOC0 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    AdcRegs.ADCSOC7CTL.bit.ACQPS    = 6; //set SOC0 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    EDIS;

    DELAY_US(ADC_usDELAY);         // Delay before converting ADC channels

}



#define TABLE_SIZE              121

//NTC-10K-3950-B值
//-55~125°C对应的电阻阻值表(单位: Ω)
//阻值~温度换算: 数组下标索引减去55即为温度值
const Uint16 g_rvTable[TABLE_SIZE] = {
    32919,                                                                              //0
    31270, 29715, 28246, 26858, 25547, 24307, 23135, 22026, 20977, 19987,               //1~10
    19044, 18154, 17310, 16510, 15752, 15034, 14352, 13705, 13090, 12507,               //11~20
    11953, 11427, 10927, 10452, 10000, 9570, 9161, 8771, 8401, 8048,                    //21~30
    7712, 7391, 7086, 6795, 6518, 6254, 6001, 5761, 5531, 5311,                         //31~40
    5102, 4902, 4710, 4528, 4353, 4186, 4026, 3874, 3728, 3588,                         //41~50
    3454, 3326, 3203, 3085, 2973, 2865, 2761, 2662, 2567, 2476,                         //51~60
    2388, 2304, 2223, 2146, 2072, 2000, 1932, 1866, 1803, 1742,                         //61~70
    1684, 1627, 1573, 1521, 1471, 1423, 1377, 1332, 1289, 1248,                         //71~80
    1208, 1170, 1133, 1097, 1063, 1030, 998, 968, 938, 909,                             //81~90
    882, 855, 829, 805, 781, 758, 735, 714, 693, 673,                                   //91~100
    653, 635, 616, 599, 582, 565, 550, 534, 519, 505,                                   //101~110
    491, 478, 465, 452, 440, 428, 416, 405, 395, 384                                   //111~120
};

/*---------------------------------------------------------------------------
 * @brief： transAdcToT
 * @param：
 * @retval：
 * @note：
 *          断开时为-1度（65535），最大值为121度
 ---------------------------------------------------------------------------*/
Uint16 transAdcToT(Uint16 adcValue)
{
    Uint16 i;
    Uint32 temp32;
    Uint16 temp16;
    Uint16  backT=0;

    // 先由采样值换算电压值
    temp32 = (Uint32)adcValue;
    temp32 = 3000 * temp32 / 4096;

    // 先由电压值算出电阻值大小
    if(temp32>2900)
    {
        temp32 = 50000; // 直接等于50K, 0度
    }
    else
    {
        temp32 = (6000*temp32-3000000)/(3000-temp32);
    }

    temp16 = (Uint16)temp32;

    // 根据电阻值大小查表得出温度值
     for(i = 0; i < TABLE_SIZE; i++)
     {
          if(temp16 > g_rvTable[i])
            {
                backT = i;

//                if(backT==0) // 断开NTC，也可能没接NTC时
//                {
//
//
//                }
                backT -=1; // 人为校正1度  断开时为-1度

                return backT;
             }
     }

     backT = MAX_MOS_T;  // 表里查不到表示很高温,这里写100度

     return backT;
}






#endif
