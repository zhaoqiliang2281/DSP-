/*
 * DCDC_Adc.h
 *
 *  Created on: 2025年6月13日
 *      Author: phone
 */

#ifndef DCDC_ADC_H_
#define DCDC_ADC_H_

//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------
enum AdcChannel
{
    eAdcBegin = 0,
    eIbuckboost = eAdcBegin,
    eIbat,
    eVbus,      //Vin
    eVbat,
    eVbat_Relay,
    eVport,      //VDisChrg||VChrg
    eBuckTemp,
    eBoostTemp,
    eAdcEnd,
    eAdcOtherBegin = eAdcEnd,
    ePbat = eAdcOtherBegin,
    eAdcOtherEnd
};
///
typedef union
{
    Uint16 all;
	Uint16 bCalcRmsFlag2;

    struct
    {
        Uint16 bCalcBiasFinish         :1;
        Uint16 bCalcRmsFlag            :1;
        Uint16 bCalcRmsFlagBitFilter   :1;
        Uint16 bCalcRmsFlagNtc         :1;

        Uint16 bRsvd                   :12;
        Uint16 bRsvd1                  :16;
    }bit;
}Union_Flag_Adc;
extern Union_Flag_Adc unAdcFlag;

//typedef struct
//{
//    float32 fRealVal;       //瞬时值
//    float32 fRealSum;       //累计值
//    float32 fRealSumBak;    //累计值备份
//    float32 fRealFilter;    //瞬时值滤波
//
//    float32 fRmsVal;        //有效值
//    float32 fRmsSum;        //累计值
//    float32 fRmsSumBak;     //累计值备份
//    float32 fRmsFilter;     //有效值滤波
//
//    float32 favgVal;        //平均值
//
//    float32 fAdcProCoeff;   // 采样比例系数
//    float32 fAdcBiasCoeff;  // 采样偏置系数
//    float32 fCaliProCoeff;  // 校正比例系数
//    float32 fCaliBiasCoeff; // 校正偏置系数
//}Struct_AdcData;

typedef struct
{
    int32 dRealVal;       //瞬时值 从CLA校准后取出来的值
    int32 dRealSum;       //累计值 循环累加200次，一次100us
    int32 dRealSumBak;    //累计值备份 200次累加后取值
    int32 dRealSumBitFilter;       //累计值 循环累加2000次，一次100us
    int32 dRealSumBakBitFilter;    //累计值备份 2000次累加后取值    	

//    int32 dRealFilter;    //瞬时值滤波 暂时没用
 
    int32 dRmsVal;        //有效值     累加200次后/200
    int32 dRmsValBitFilter;        //有效值     累加2000次后/2000
//    int32 dRmsSum;        //累计值 暂时没用
//    int32 dRmsSumBak;     //累计值备份  暂时没用
//    int32 dRmsFilter;     //有效值滤波  暂时没用

    int32 ADC_SumBitFilter;       //累计值 循环累加2000次，一次100us
    int32 ADC_SumBakBitFilter;    //累计值备份 2000次累加后取值
    int32 ADC_BitFilter;        //有效值     累加2000次后/2000

//    int32 davgVal;        //平均值

//    int32 dAdcProCoeff;   // 采样比例系数
    int32 dAdcBiasCoeff;  // 采样偏置系数
//    int32 dCaliProCoeff;  // 校正比例系数
//    int32 dCaliBiasCoeff; // 校正偏置系数
}Struct_AdcData;
extern Struct_AdcData g_strAdcData[eAdcOtherEnd];
///
typedef struct
{
    int32 dIbuckboostRealVal;    //电感电流实时值
    int32 dIbatRealVal;          //电池电流实时值
    int32 dVbatRealVal;          //电池电压实时值
    int32 dVbatRelayRealVal;     //电池继电器前电压实时值
    int32 dVportRealVal;         //端口电压实时值
    int32 dVbusRealVal;          //母线电压实时值,输入电压
    int32 dPbatRealVal;          //电池功率实时值

}Struct_AdcPool_Int;
extern Struct_AdcPool_Int g_strClaOut_AdcPool;  // Cla计算后输出
///
typedef struct
{
    /**BuckBoost电流**/
    Uint32 udIbuckboostAdcVal;  //电感电流AD值

    /**Bus电流**/
    Uint32 udVbusAdcVal;           //母线电压AD值
    /**电池电流**/
    Uint32 udIbatAdcVal;       //电感电流AD值
//    Uint16 uwIbatCaliProCoeff;  //校准比例系数
//    Uint16 uwIbatCaliBiasCoeff;

    /**电池电压**/
    Uint32 udVbatAdcVal;           //电池电压AD值
//    Uint16 uwVbatCaliProCoeff;      //校准比例系数
//    Uint16 uwVbatCaliBiasCoeff;

    /**电池继电器前电压**/
    Uint32 udVbatRelayAdcVal;           //电池继电器前电压AD值

    /**端口电压**/
    Uint32 udVportAdcVal;           //充电或者放电电压AD值

    float floatVK;
    float floatVB;
    float floatIK;
    float floatIB;

}Struct_AdcRawData;
extern Struct_AdcRawData g_strClaIn_AdcRawData;
extern Uint16 uwLoopOutData[16];


#define MAX_MOS_T    121
#define NO_MOS_NTC   65535

typedef struct
{
    int32 dRealVal;       //瞬时值
    int32 dRealSum;       //累计值
    int32 dRealSumBak;    //累计值备份

    int16 dRmsVal;        //有效值

}Str_NTC;
extern Str_NTC NTC_Buck;
extern Str_NTC NTC_Boost;



//-----------------------------------------------------------------------------
// Public Function Prototypes
//-----------------------------------------------------------------------------
extern void Adc_System_PeripheralInit(void);
extern void Adc_System_AppInit(void);
extern void Adc_Task1_RealTimeDone(void);
extern void Adc_Task2_RealTimeDone(void);
extern void Adc_Task_1ms(void);
extern void Adc_Task_5ms(void);
extern void Adc_Task_20ms(void);

extern void Adc_Task_1s(void);

extern Uint16 transAdcToT(Uint16 adcValue);

#endif /* DCDC_ADC_H_ */
