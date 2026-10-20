/*
 * DCDC_Dreating.c
 *
 *  Created on: 2025年6月13日
 *      Author: phone
 */
#ifndef Derating_C
#define Derating_C
//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"
//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------

Struct_DerateOrder g_strDerateOrder;
//-----------------------------------------------------------------------------
//Adding Private Function
//-----------------------------------------------------------------------------
static inline void Derate_Calclation(void);
static inline void Derate_Calc_SetPercent(void);
static inline void Derate_Calc_Temperature(void);
static inline void Derate_Calc_BatVolt(void);
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Derating_System_AppInit(void)
{
    g_strDerateOrder.dPowerLimintOut = RATED_POWER;
    g_strDerateOrder.dMaxPowerLimint = RATED_POWER;
    g_strDerateOrder.dBatPercentSetDerating = RATED_POWER;
    g_strDerateOrder.dHighTempDerating = RATED_POWER;
    g_strDerateOrder.dLowBatVoltDerating = RATED_POWER;
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Derating_Task_5ms(void)
{
    Derate_Calc_SetPercent();   // 设定百分比降载，设定功率降载
    Derate_Calc_Temperature();  // 过温降载
    Derate_Calc_BatVolt();      // 低电池电压降载

    Derate_Calclation();        // 执行降载操作

//    g_strDerateOrder.dPowerLimintOut = g_strDerateOrder.dMaxPowerLimint;
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
static inline void Derate_Calclation(void)
{
    g_strDerateOrder.dMaxPowerLimint = RATED_POWER;

    if(g_strDerateOrder.dMaxPowerLimint > g_strDerateOrder.dBatPercentSetDerating)
    {
        g_strDerateOrder.dMaxPowerLimint = g_strDerateOrder.dBatPercentSetDerating;
        g_strDerateOrder.uwDeratingMode = 1;  // 降载原因，暂时未用
    }

    if(g_strDerateOrder.dMaxPowerLimint > g_strDerateOrder.dHighTempDerating)
    {
        g_strDerateOrder.dMaxPowerLimint = g_strDerateOrder.dHighTempDerating;
        g_strDerateOrder.uwDeratingMode = 2;
    }

    if(g_strDerateOrder.dMaxPowerLimint > g_strDerateOrder.dLowBatVoltDerating)
    {
        g_strDerateOrder.dMaxPowerLimint = g_strDerateOrder.dLowBatVoltDerating;
        g_strDerateOrder.uwDeratingMode = 3;  
    }
    UPDNLMT(g_strDerateOrder.dMaxPowerLimint,RATED_POWER,0);

    if(g_strDerateOrder.dMaxPowerLimint >= RATED_POWER)
    {
        g_strDerateOrder.uwDeratingMode = 0;  // 无需降载
    }
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
static inline void Derate_Calc_SetPercent(void)
{
    //g_strDerateOrder.dBatPercentSetDerating = (int32)g_uwSettingData_ModBus[eSettingData_PowerRateSet]*RATED_POWER/100;
	    g_strDerateOrder.dBatPercentSetDerating = RATED_POWER; // 暂时不需要通过通讯去降额
    UPDNLMT(g_strDerateOrder.dBatPercentSetDerating,RATED_POWER,0);
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents: 温度降载，暂时还没做，这里填写最大功率
******************************************************************************/
static inline void Derate_Calc_Temperature(void)
{
    g_strDerateOrder.dHighTempDerating = (int32)RATED_POWER;
    UPDNLMT(g_strDerateOrder.dHighTempDerating,RATED_POWER,0);
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
static inline void Derate_Calc_BatVolt(void)
{
    g_strDerateOrder.dLowBatVoltDerating = (g_strAdcData[eVbat].dRmsVal/1000)*(Ibat_Set(20)/1000);//转换到W为单位
    UPDNLMT(g_strDerateOrder.dLowBatVoltDerating,RATED_POWER,0);
}
#endif
