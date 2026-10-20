/*
 * DCDC_BuckBoost.h
 *
 *  Created on: 2025年6月15日
 *      Author: phone
 */

#ifndef DCDC_BUCKBOOST_H_
#define DCDC_BUCKBOOST_H_


//-----------------------------------------------------------------------------
// Public Enumerated and Structure Definitions
//-----------------------------------------------------------------------------

#define BUCKBOOST_PWM_DUTY_MAX 710
#define BUCKBOOST_PWM_DUTY_MIN 0


enum
{
    eBatModeInit  = 0,   // 初始状态
    eBatChargeMode = 1,     // 电池充电模式
    eBatDisChargeMode = 2,     // 电池放电模式
    eOpenMode = 3,      // 开环发波模式
    eOffBatMode = 4,    // 无电池-老化模式
    eBatModeEnd = eOffBatMode
};

typedef union
{
    Uint16 all;
    struct
    {
        Uint16 bBuckBoostMode         :3;  // BuckBoost控制模式
        Uint16 bBuckBoostEnable       :1;  // BuckBoost使能发波
        Uint16 bBuckBoostEnableOld    :1;
        Uint16 bReWorkFlag            :1;  //封波
        Uint16 bBurstFlag             :1;
        Uint16 bReserved              :8;
        Uint16 preChargeFlag          :1;  // 预充标志
    }bit;
}Union_Flag_Buck_Boost;
extern Union_Flag_Buck_Boost unBuckBoostFlag;
///
typedef struct
{
    Uint16 uwFdbKpFilter;
    Uint16 uwSystemMode;
    Uint16 uwChargeMode;

    //两开关BuckBoost
    float32 fComparePwmA;
    float32 fComparePwmB;
    float32 fComparePwmC;

    //环路给定的输入量
    float32 fDisChargeBatVoltRef;
    float32 fChargeBatVoltRef;
    float32 fBusRef;

    float32 fBatCurrRef;
    float32 fBatDisChargeCurrLimit;
    float32 fBatChargeCurrLimit;

    //前馈
    float32 fVoltFdForward;

    //软件逐波限流阈值
    float32 fBatCurrOcpVal;
    float32 fBatCurrRecVal;

    Union_Flag_Buck_Boost unControlFlag;

}Struct_Buck_Boost_ControlPara;
extern Struct_Buck_Boost_ControlPara g_strBuckBoostCtrlPara;
///
typedef struct
{
    Uint16 uwTz1ProtectTimes;
    Uint16 uwTz2ProtectTimes;
    Uint16 uwTz3ProtectTimes;

    Uint16 uwTzProtectTimes;
    Uint16 uwOcpOverFinalTimes;
    Uint16 uwOcpOverBaseTimes;
    Uint16 uwOcpTimes[25];

    Uint16 uwReWorkTimes;
    Uint16 uwOcpFaultTime;

    struct
    {
        Uint16 bOcpFaultFlag         :1;
        Uint16 bReserved             :15;
    }bit;

}Struct_Buck_Boost_ProtectPara;
extern Struct_Buck_Boost_ProtectPara g_strBuckBoostProtectPara;
///
typedef struct
{
    //环路给定的输入量
    int32 dBusRef;
    int32 dDisChargeBatVoltRef;
    int32 dChargeBatVoltRef;
    int32 dBatDisChargeCurrLimit;
    int32 dBatChargeCurrLimit;

    Uint16 uwVbusLoopKp;
    Uint16 uwVbusLoopKi;
    Uint16 uwVbatLoopKp;
    Uint16 uwVbatLoopKi;
    Uint16 uwIbatLoopKp;
    Uint16 uwIbatLoopKi;

    Uint16 uwDebugPara1;
    Uint16 uwDebugPara2;
    Uint16 uwDebugPara3;
    Uint16 uwDebugPara4;
    Uint16 uwDebugPara5;
    Uint16 uwDebugPara6;

    Union_Flag_Buck_Boost unControlFlag;

}Struct_Buck_Boost_SettingPara;
extern Struct_Buck_Boost_SettingPara g_strClaIn_BuckBoostSetData;
///
extern Struct_fIncPI g_strBusVolt_BuckBoost;
extern Struct_fIncPI g_strChargeBatVolt_BuckBoost;//Buck模式下，电池充电的限流
extern Struct_fIncPI g_strDisChargeBatVolt_BuckBoost;//Boost模式下，电池放电的限流
extern Struct_fIncPI g_strBatCurr_BuckBoost;
//-----------------------------------------------------------------------------
// Public Function Prototypes
//-----------------------------------------------------------------------------
extern void BuckBoost_System_AppInit(void);
extern void BuckBoost_Task1_RealTimeDone(void);
extern void BuckBoost_Task_1ms(void);
extern void BuckBoost_Task_5ms(void);
extern void BuckBoost_Task_20ms(void);

#endif /* DCDC_BUCKBOOST_H_ */
