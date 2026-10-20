/*
 * DCDC_BuckBoost.c
 *
 *  Created on: 2025年6月15日
 *      Author: phone
 */

#ifndef BUCKBOOST_C
#define BUCKBOOST_C
//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"
#include "stdlib.h"
//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------
/*与CLA通讯数据*/
#pragma DATA_SECTION(g_strClaIn_BuckBoostSetData,"CpuToCla1MsgRAM");
Struct_Buck_Boost_SettingPara g_strClaIn_BuckBoostSetData;
///////
Union_Flag_Buck_Boost unBuckBoostFlag;
Struct_Buck_Boost_ProtectPara g_strBuckBoostProtectPara;

int16 dDataMonitor[6];
Uint16 uwSoftFininishFlag;
//-----------------------------------------------------------------------------
//Adding Private Function
//-----------------------------------------------------------------------------
static inline void BuckBoost_Given(void);
static inline void UpdateControlPara(void);
static inline void BuckBoost_TzProtect(void);
static inline void BuckBoost_PwmOnOff(void);
static inline void BuckBoost_ControlLoop(void);
static inline void BuckBoost_StateLogic(void);
static inline void BuckBoost_LoadLimit(void);
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void BuckBoost_System_AppInit(void)
{
    EALLOW;
    EPwm1Regs.AQCSFRC.bit.CSFA = 1;
    EPwm1Regs.AQCSFRC.bit.CSFB = 1;
    EPwm1Regs.TZFRC.bit.OST = 1;//软件触发TZ
    EPwm1Regs.CMPA.half.CMPA = 0;
    EPwm1Regs.CMPB = 0;
    EDIS;

    unBuckBoostFlag.bit.bReWorkFlag = FALSE;


}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents: 50us
******************************************************************************/
#pragma CODE_SECTION(BuckBoost_Task1_RealTimeDone,"ramfuncs");
void BuckBoost_Task1_RealTimeDone(void)
{
    BuckBoost_TzProtect();    // Tz封波次数记录，只记录未做处理
    BuckBoost_PwmOnOff();     // 电压过高和工作模式切换控制PWM开关
    BuckBoost_StateLogic();   // 状态机控制，开环发波和正常发波
    BuckBoost_ControlLoop();  // 环路控制，使能CLA Task2
}
/******************************************************************************
Function Name: BuckBoost_Task_1ms
-------------------------------------
Function Descriptions: BuckBoost_Task_1ms
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void BuckBoost_Task_1ms(void)
{
    BuckBoost_Given();
}
/******************************************************************************
Function Name: BuckBoost_Task_20ms
-------------------------------------
Function Descriptions: BuckBoost_Task_20ms
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void BuckBoost_Task_20ms(void)
{
    UpdateControlPara();   // 更新PID参数
    BuckBoost_LoadLimit(); // 更新充放电电流大小
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:  1ms任务
******************************************************************************/
static inline void BuckBoost_Given(void)
{
    int32 dBustRefTmp;
    int32 dChargeBatVoltRefTmp;
    int32 dDisChargeBatVoltRefTmp;
    int32 dBusRefSoftStepTmp;
    int32 dBatRefSoftStepTmp;

    static Uint16 suwVoltRefSoftCnt = 0;

    //dBustRefTmp = (int32)((Uint32)g_uwSettingData_ModBus[eSettingData_VbusRef])*10;  // MODBUS输入值扩大十倍为mV
    //canSetPra.bufRef = 135000;
    canSetPra.bufRef = 150000;
    if(canSetPra.bufRef<15000)
        canSetPra.bufRef = 15100;

	if(canSetPra.preChargeFlag) 
		{
			dBustRefTmp = canSetPra.bufRef - 15000; // 暂时固定60V
			
		}
	else
		{
				if(canSetPra.batWorkMode == MODE_CHARGE)
						{
							dBustRefTmp = canSetPra.bufRef - 15000; // 暂时固定60V
						}
					else if(canSetPra.batWorkMode == MODE_DISCHARGE)
						{
							dBustRefTmp = canSetPra.bufRef + 15000; // 暂时固定60V
						}
	}
	
    //dChargeBatVoltRefTmp = (int32)((Uint32)g_uwSettingData_ModBus[eSettingData_VbatChrgRef])*10;// MODBUS输入值扩大十倍为mV
    
    //dDisChargeBatVoltRefTmp = (int32)((Uint32)g_uwSettingData_ModBus[eSettingData_VbatDisChrgRef])*10;// MODBUS输入值扩大十倍为mV
	//if(canSetPra.batWorkMode == MODE_CHARGE)

	if(canSetPra.preChargeFlag) // 预充
		{
		dChargeBatVoltRefTmp = g_strAdcData[eVbat].dRmsVal;
		dDisChargeBatVoltRefTmp= g_strAdcData[eVbat].dRmsVal;	// 充放电都用发过来的值

	}
	else
		{
		dChargeBatVoltRefTmp = canSetPra.batRef.all;
		dDisChargeBatVoltRefTmp= canSetPra.batRef.all;	// 充放电都用发过来的值

	}



 
    if(ENABLE == unBuckBoostFlag.bit.bBuckBoostEnable)//Logic_GetSysStatus() == eSysOperate
    {
		if(suwVoltRefSoftCnt < TSK1S_TIMER1MS) 
            suwVoltRefSoftCnt++;
		
        if(suwVoltRefSoftCnt < TSK1S_TIMER1MS)  // 使能1S后
        {
            // 母线电压环给定
            if(((g_strClaIn_BuckBoostSetData.dBusRef - dBustRefTmp) > Vbat_Set(30))//30V
             ||((dBustRefTmp - g_strClaIn_BuckBoostSetData.dBusRef) > Vbat_Set(30)))
            {
                dBusRefSoftStepTmp = Vbat_Set(3); // 软起步进3V
            }
            else
            {
                dBusRefSoftStepTmp = Vbat_Set(0.9); // 软起步进0.9V
            }

            if(g_strClaIn_BuckBoostSetData.dBusRef < (dBustRefTmp - dBusRefSoftStepTmp))
            {
                g_strClaIn_BuckBoostSetData.dBusRef += dBusRefSoftStepTmp;
            }
            else if(g_strClaIn_BuckBoostSetData.dBusRef > (dBustRefTmp + dBusRefSoftStepTmp)) // 原来用<，应该不对
            {
                g_strClaIn_BuckBoostSetData.dBusRef -= dBusRefSoftStepTmp;
            }
            else
            {
                g_strClaIn_BuckBoostSetData.dBusRef = dBustRefTmp;
            }

            // 充电环路给定
			// 步进上升或下降
            if(((g_strClaIn_BuckBoostSetData.dChargeBatVoltRef - dChargeBatVoltRefTmp) > Vbat_Set(30))  //30V
             ||((dChargeBatVoltRefTmp - g_strClaIn_BuckBoostSetData.dChargeBatVoltRef) > Vbat_Set(30)))
            {
                dBatRefSoftStepTmp = Vbat_Set(3); // 软起步进3V
            }
            else
            {
                dBatRefSoftStepTmp = Vbat_Set(0.9);  // 软起步进0.9V
            }

            if(g_strClaIn_BuckBoostSetData.dChargeBatVoltRef < (dChargeBatVoltRefTmp - dBatRefSoftStepTmp))
            {
                g_strClaIn_BuckBoostSetData.dChargeBatVoltRef += dBatRefSoftStepTmp;
            }
            else if(g_strClaIn_BuckBoostSetData.dChargeBatVoltRef > (dChargeBatVoltRefTmp + dBatRefSoftStepTmp)) // 原来用<，应该不对
            {
                g_strClaIn_BuckBoostSetData.dChargeBatVoltRef -= dBatRefSoftStepTmp;
            }
            else
            {
                g_strClaIn_BuckBoostSetData.dChargeBatVoltRef = dChargeBatVoltRefTmp;
            }
        }
        else  // 使能1S之后，直接给定
        {
            uwSoftFininishFlag = FINISHED;
            g_strClaIn_BuckBoostSetData.dBusRef = dBustRefTmp;   
            g_strClaIn_BuckBoostSetData.dChargeBatVoltRef = dChargeBatVoltRefTmp;
        }
    }
    else  // 未使能
    {
        suwVoltRefSoftCnt = 0;
        uwSoftFininishFlag = UNFINISHED;
        g_strClaIn_BuckBoostSetData.dBusRef = g_strAdcData[eVbus].dRmsVal;   // 未使能时跟随采集到的电压
        g_strClaIn_BuckBoostSetData.dChargeBatVoltRef = g_strAdcData[eVbat].dRmsVal;  // 未使能时跟随采集到的电压
    }

    // 放电电压参考直接赋值
    g_strClaIn_BuckBoostSetData.dDisChargeBatVoltRef = dDisChargeBatVoltRefTmp;

	unBuckBoostFlag.bit.preChargeFlag = canSetPra.preChargeFlag;  // 把预充标志传进去给控制环路

    g_strClaIn_BuckBoostSetData.unControlFlag.all = unBuckBoostFlag.all; // 使能位传到CLA
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
static inline void UpdateControlPara(void)
{
    g_strClaIn_BuckBoostSetData.uwVbusLoopKp = g_uwSettingData_ModBus[eSettingData_VbusLoopKp];
    g_strClaIn_BuckBoostSetData.uwVbusLoopKi = g_uwSettingData_ModBus[eSettingData_VbusLoopKi];
    g_strClaIn_BuckBoostSetData.uwVbatLoopKp = g_uwSettingData_ModBus[eSettingData_VbatLoopKp];
    g_strClaIn_BuckBoostSetData.uwVbatLoopKi = g_uwSettingData_ModBus[eSettingData_VbatLoopKi];
    g_strClaIn_BuckBoostSetData.uwIbatLoopKp = g_uwSettingData_ModBus[eSettingData_IbatLoopKp];
    g_strClaIn_BuckBoostSetData.uwIbatLoopKi = g_uwSettingData_ModBus[eSettingData_IbatLoopKi];

    g_strClaIn_BuckBoostSetData.uwDebugPara1 = g_uwSettingData_ModBus[eSettingData_DebugPara1];
    g_strClaIn_BuckBoostSetData.uwDebugPara2 = g_uwSettingData_ModBus[eSettingData_DebugPara2];
    g_strClaIn_BuckBoostSetData.uwDebugPara3 = g_uwSettingData_ModBus[eSettingData_DebugPara3];
    g_strClaIn_BuckBoostSetData.uwDebugPara4 = g_uwSettingData_ModBus[eSettingData_DebugPara4];
    g_strClaIn_BuckBoostSetData.uwDebugPara5 = g_uwSettingData_ModBus[eSettingData_DebugPara5];
    g_strClaIn_BuckBoostSetData.uwDebugPara6 = g_uwSettingData_ModBus[eSettingData_DebugPara6];
}
/******************************************************************************
Function Name: BuckBoost_TzProtect
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:  50us
******************************************************************************/
#pragma CODE_SECTION(BuckBoost_TzProtect, "ramfuncs");
static inline void BuckBoost_TzProtect(void)//Tz封波次数记录
{
    if(ENABLE == unBuckBoostFlag.bit.bBuckBoostEnable)
    {
        if(TRUE == EPwm1Regs.TZFLG.bit.CBC)
        {
            g_strBuckBoostProtectPara.uwTz1ProtectTimes++;
            if(g_strBuckBoostProtectPara.uwTz1ProtectTimes > 100)
            {
                g_strBuckBoostProtectPara.uwTz1ProtectTimes = 0;
            }

            g_strBuckBoostProtectPara.uwTzProtectTimes++;
        }

//        if(TRUE == EPwm2Regs.TZFLG.bit.CBC)
//        {
//            g_strBuckBoostProtectPara.uwTz2ProtectTimes++;
//            if(g_strBuckBoostProtectPara.uwTz2ProtectTimes > 100)
//            {
//                g_strBuckBoostProtectPara.uwTz2ProtectTimes = 0;
//            }
//
//            g_strBuckBoostProtectPara.uwTzProtectTimes++;
//        }
//
//        if(TRUE == EPwm3Regs.TZFLG.bit.CBC)
//        {
//            g_strBuckBoostProtectPara.uwTz3ProtectTimes++;
//            if(g_strBuckBoostProtectPara.uwTz3ProtectTimes > 100)
//            {
//                g_strBuckBoostProtectPara.uwTz3ProtectTimes = 0;
//            }
//
//            g_strBuckBoostProtectPara.uwTzProtectTimes++;
//        }
    }

    EALLOW;
    EPwm1Regs.TZCLR.bit.CBC = 1;
//    EPwm2Regs.TZCLR.bit.CBC = 1;
//    EPwm3Regs.TZCLR.bit.CBC = 1;
    EDIS;
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:  50us
******************************************************************************/
#pragma CODE_SECTION(BuckBoost_PwmOnOff, "ramfuncs");
static inline void BuckBoost_PwmOnOff(void)
{
    static Uint16 suwRecoverCnt = 0;
    static Uint16 suwRecoverTime = 0;
    static Uint16 suwReWorkFlagOld = 0;
    static Uint16 suwTzProtectTimesOld = 0;
    static Uint16 suwBatWorkModeOld = 0;

    if(Logic_GetSysStatus() == eSysOperate) // 状态机为运行  
    {
       // if((suwBatWorkModeOld != uwBatWorkMode)
        if((canSetPra.stRunFlag == 0) 
		 ||(g_strAdcData[eVbus].dRealVal > Vbat_Set(180))
         ||(g_strAdcData[eVbat].dRealVal > Vbat_Set(180))) // 无用？电压太大了
//        || (suwTzProtectTimesOld != g_strBuckBoostProtectPara.uwTzProtectTimes)
        {
            unBuckBoostFlag.bit.bReWorkFlag = TRUE;  // 为TRUE就停机，？？
            suwRecoverCnt = 0;

            if(suwBatWorkModeOld != uwBatWorkMode)
            {
                suwRecoverTime = 2;//3个中断周期
            }
            else
            {
                suwRecoverTime = TSK30MS_TIMER50US;
            }

            if(FALSE == suwReWorkFlagOld)
            {
                g_strBuckBoostProtectPara.uwReWorkTimes++;
                if(g_strBuckBoostProtectPara.uwReWorkTimes > 6000)
                {
                    g_strBuckBoostProtectPara.uwReWorkTimes = 0;
                }
            }
        }
        else
        {
            if(suwRecoverCnt >= suwRecoverTime)
            {
                unBuckBoostFlag.bit.bReWorkFlag = FALSE;
            }
            else
            {
                suwRecoverCnt++;
            }
        }
    }

    suwBatWorkModeOld = uwBatWorkMode;
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:  BuckBoost_StateLogic
******************************************************************************/
#pragma CODE_SECTION(BuckBoost_ControlLoop, "ramfuncs");
static inline void BuckBoost_ControlLoop(void)
{
    if(FALSE == unAdcFlag.bit.bCalcBiasFinish)
        return;

    g_strClaIn_BuckBoostSetData.unControlFlag.all = unBuckBoostFlag.all;
    Cla1ForceTask2();
}
/******************************************************************************
Function Name: BuckBoostStateLogic
-------------------------------------
Function Descriptions: BuckBoostStateLogic
Parameter Name list:
ReturnType:
Refer documents:  BuckBoost_StateLogic
******************************************************************************/
#pragma CODE_SECTION(BuckBoost_StateLogic, "ramfuncs");
static inline void BuckBoost_StateLogic(void)
{
    static Uint16 suwBenchTestDelayCnt = 0;
    static Uint16 suwRunningDelayCnt = 0;

    // 控制状态
    if(eOpenLoop == uwDeviceWorkMode) // 开环发波
    {
        if(Logic_GetSysStatus() == eSysOperate)
        {
            PWM_ENABLE;
            suwBenchTestDelayCnt++;
            if(suwBenchTestDelayCnt >= TSK500MS_TIMER50US) //延时500ms
            {
                suwBenchTestDelayCnt = TSK500MS_TIMER50US;
                unBuckBoostFlag.bit.bBuckBoostMode = eOpenMode;
                unBuckBoostFlag.bit.bBuckBoostEnable = ENABLE;  // 允许发波
                suwBenchTestDelayCnt = 0;
            }
			 
			// if((OFF == g_uwSettingData_ModBus[eSettingData_ManualSwitch])||(TRUE==unSysLogicFlag.bit.bStopCond))
			if((canSetPra.stRunFlag == 0)||(TRUE==unSysLogicFlag.bit.bStopCond))	// CAN控制
			{
				unBuckBoostFlag.bit.bBuckBoostEnable = DISABLE;  // 停止
				Logic_SetSysStatus(eSysStop);
			}
        }
        else
        {
            PWM_DISABLE;
            unBuckBoostFlag.bit.bBuckBoostMode = eBatModeInit;
            unBuckBoostFlag.bit.bBuckBoostEnable = DISABLE;
            suwBenchTestDelayCnt = 0;
        }
    }
    else if((eNormalRunning == uwDeviceWorkMode))  // 正常发波
    {
        if(Logic_GetSysStatus() == eSysOperate)
        {
            PWM_ENABLE;
            suwRunningDelayCnt++;
            if(suwRunningDelayCnt >= TSK500MS_TIMER50US)
            {
                suwRunningDelayCnt = TSK500MS_TIMER50US;
                if(eCharge == uwBatWorkMode) // 这里用bat
                {
                    unBuckBoostFlag.bit.bBuckBoostMode = eBatChargeMode;
                }
                else
                {
                    unBuckBoostFlag.bit.bBuckBoostMode = eBatDisChargeMode;
                }

                unBuckBoostFlag.bit.bBuckBoostEnable = !(unBuckBoostFlag.bit.bReWorkFlag);

				//if((OFF == g_uwSettingData_ModBus[eSettingData_ManualSwitch])||(TRUE==unSysLogicFlag.bit.bStopCond))
			    if((canSetPra.stRunFlag == 0)||(TRUE==unSysLogicFlag.bit.bStopCond))	// CAN控制
				{
					unBuckBoostFlag.bit.bBuckBoostEnable = DISABLE;  // 停止
					Logic_SetSysStatus(eSysStop);
				}


            }
        }
        else
        {
            PWM_DISABLE;
            unBuckBoostFlag.bit.bBuckBoostMode = eBatModeInit;
            unBuckBoostFlag.bit.bBuckBoostEnable = DISABLE;
            suwRunningDelayCnt = 0;
        }

    }
    else
    {
        if(Logic_GetSysStatus() == eSysOperate)
        {
            PWM_ENABLE;
            suwRunningDelayCnt++;
            if(suwRunningDelayCnt >= TSK500MS_TIMER50US)
            {
                suwRunningDelayCnt = TSK500MS_TIMER50US;
                unBuckBoostFlag.bit.bBuckBoostMode = eOffBatMode;
                unBuckBoostFlag.bit.bBuckBoostEnable = !(unBuckBoostFlag.bit.bReWorkFlag);
            }
        }
        else
        {
            PWM_DISABLE;
            unBuckBoostFlag.bit.bBuckBoostMode = eBatModeInit;
            unBuckBoostFlag.bit.bBuckBoostEnable = DISABLE;
            suwRunningDelayCnt = 0;
        }
    }

}

/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:    输出电流精度调节相关
******************************************************************************/
static inline void BuckBoost_LoadLimit(void)
{
    int32 dChargeBatCurrRefTmp,dChargeBatCurrLevelTmp;
    int32 dDisChargeBatCurrRefTmp,dDisChargeBatCurrLevelTmp;
    int32 dChargeCurrRefTmp,dDisChargeCurrRefTmp,dVoltRatioTmp;//计算电感电流的给定
    int32 dBatMaxPowerLimitTmp;

    static int32 sdChargeBatCurrAdjust = 0,sdDisChargeBatCurrAdjust = 0;

    if(Logic_GetSysStatus() == eSysOperate)
    {
        //电流设定值
       // dChargeBatCurrLevelTmp = (int32)((Uint32)g_uwSettingData_ModBus[eSettingData_IbatChrgRef])*10;//通讯扩大了十倍,故实际扩大了1000倍,mA

		if(canSetPra.preChargeFlag) // 预充
			{
				dChargeBatCurrLevelTmp = 200; 
		}
		else
			{
			dChargeBatCurrLevelTmp = canSetPra.curRef.all; 
		
		}
		
		UPDNLMT(dChargeBatCurrLevelTmp,Ibat_Set(50),0);
       // dDisChargeBatCurrLevelTmp = (int32)((Uint32)g_uwSettingData_ModBus[eSettingData_IbatDisChrgRef])*10;//mA
       dDisChargeBatCurrLevelTmp = canSetPra.curRef.all; 
        UPDNLMT(dDisChargeBatCurrLevelTmp,Ibat_Set(50),0);

        //功率电流限值计算
        dBatMaxPowerLimitTmp = (int32)((int64)g_strDerateOrder.dPowerLimintOut*1000*1000/g_strAdcData[eVbat].dRmsVal);//mA
        UPDNLMT(dBatMaxPowerLimitTmp,Ibat_Set(70),0);

        //充电取小
        dChargeBatCurrRefTmp = dBatMaxPowerLimitTmp;
        UPLMT(dChargeBatCurrRefTmp,dChargeBatCurrLevelTmp);

        //放电取小
        dDisChargeBatCurrRefTmp = dBatMaxPowerLimitTmp;
        UPLMT(dDisChargeBatCurrRefTmp,dDisChargeBatCurrLevelTmp);

        //计算电压比值，和电感电流的给定
//        if((g_strAdcData[eVbus].dRmsVal >= Vbat_Set(10))&&(g_strAdcData[eVbat].dRmsVal >= Vbat_Set(10)))
//        {
//            dVoltRatioTmp = g_strAdcData[eVbus].dRmsVal*100/g_strAdcData[eVbat].dRmsVal;
//        }
//        else
//        {
//            dVoltRatioTmp = 100;
//        }
//
		dDataMonitor[0] = sdChargeBatCurrAdjust;
//
//        dChargeCurrRefTmp = dVoltRatioTmp*dChargeBatCurrRefTmp/100;
//		UPDNLMT(dChargeBatCurrLevelTmp,Ibat_Set(70),0);
//        dDisChargeCurrRefTmp = dVoltRatioTmp*dDisChargeBatCurrRefTmp/100;
//		UPDNLMT(dChargeBatCurrLevelTmp,Ibat_Set(70),0);
        dChargeCurrRefTmp = dChargeBatCurrRefTmp;
        dDisChargeCurrRefTmp = dDisChargeBatCurrRefTmp;

        if(uwSoftFininishFlag == FINISHED)
        {
            if(eCharge == uwBatWorkMode)
            {
                sdDisChargeBatCurrAdjust = 0;

                if(dBatMaxPowerLimitTmp <= dChargeBatCurrLevelTmp) // 功率限制了电流
                {
                    if(g_strAdcData[ePbat].dRmsVal < (g_strDerateOrder.dPowerLimintOut - Pbat_Set(1000)))
                    {
                        sdChargeBatCurrAdjust += Ibat_Set(0.1);
                    }
                    else if(g_strAdcData[ePbat].dRmsVal < (g_strDerateOrder.dPowerLimintOut - Pbat_Set(100)))
                    {
                        sdChargeBatCurrAdjust += Ibat_Set(0.005);
                    }
                    else if(g_strAdcData[ePbat].dRmsVal > (g_strDerateOrder.dPowerLimintOut + Pbat_Set(1000)))
                    {
                        sdChargeBatCurrAdjust -= Ibat_Set(0.1);
                    }
                    else if(g_strAdcData[ePbat].dRmsVal > (g_strDerateOrder.dPowerLimintOut + Pbat_Set(100)))
                    {
                        sdChargeBatCurrAdjust -= Ibat_Set(0.005);
                    }
                    UPDNLMT(sdChargeBatCurrAdjust,Ibat_Set(3.0),Ibat_Set(-3.0));
                }
                else
                {
                    if(labs(g_strAdcData[eIbat].dRmsVal) < (dChargeBatCurrLevelTmp - Ibat_Set(0.5)))
                    {
                        sdChargeBatCurrAdjust += Ibat_Set(0.1);
                    }
                    else if(labs(g_strAdcData[eIbat].dRmsVal) < (dChargeBatCurrLevelTmp - Ibat_Set(0.05)))
                    {
                        sdChargeBatCurrAdjust += Ibat_Set(0.005);
                    }
                    else if(labs(g_strAdcData[eIbat].dRmsVal) > (dChargeBatCurrLevelTmp + Ibat_Set(0.5)))
                    {
                        sdChargeBatCurrAdjust -= Ibat_Set(0.1);
                    }
                    else if(labs(g_strAdcData[eIbat].dRmsVal) > (dChargeBatCurrLevelTmp + Ibat_Set(0.05)))
                    {
                        sdChargeBatCurrAdjust -= Ibat_Set(0.005);
                    }
                    UPDNLMT(sdChargeBatCurrAdjust,Ibat_Set(3.0),Ibat_Set(-3.0));
                }
            }
            else
            {
                sdChargeBatCurrAdjust = 0;

                if(dBatMaxPowerLimitTmp <= dDisChargeBatCurrLevelTmp)
                {
                    if(g_strAdcData[ePbat].dRmsVal < (g_strDerateOrder.dPowerLimintOut - Pbat_Set(1000)))
                    {
                        sdDisChargeBatCurrAdjust += Ibat_Set(0.1);
                    }
                    else if(g_strAdcData[ePbat].dRmsVal < (g_strDerateOrder.dPowerLimintOut - Pbat_Set(100)))
                    {
                        sdDisChargeBatCurrAdjust += Ibat_Set(0.005);
                    }
                    else if(g_strAdcData[ePbat].dRmsVal > (g_strDerateOrder.dPowerLimintOut + Pbat_Set(1000)))
                    {
                        sdDisChargeBatCurrAdjust -= Ibat_Set(0.1);
                    }
                    else if(g_strAdcData[ePbat].dRmsVal > (g_strDerateOrder.dPowerLimintOut + Pbat_Set(100)))
                    {
                        sdDisChargeBatCurrAdjust -= Ibat_Set(0.005);
                    }
                    UPDNLMT(sdDisChargeBatCurrAdjust,Ibat_Set(3.0),Ibat_Set(-3.0));
                }
                else
                {
                    if(labs(g_strAdcData[eIbat].dRmsVal) < (dDisChargeBatCurrLevelTmp - Ibat_Set(0.5)))
                    {
                        sdDisChargeBatCurrAdjust += Ibat_Set(0.1);
                    }
                    else if(labs(g_strAdcData[eIbat].dRmsVal) < (dDisChargeBatCurrLevelTmp - Ibat_Set(0.05)))
                    {
                        sdDisChargeBatCurrAdjust += Ibat_Set(0.005);
                    }
                    else if(labs(g_strAdcData[eIbat].dRmsVal) > (dDisChargeBatCurrLevelTmp + Ibat_Set(0.5)))
                    {
                        sdDisChargeBatCurrAdjust -= Ibat_Set(0.1);
                    }
                    else if(labs(g_strAdcData[eIbat].dRmsVal) > (dDisChargeBatCurrLevelTmp + Ibat_Set(0.05)))
                    {
                        sdDisChargeBatCurrAdjust -= Ibat_Set(0.005);
                    }
                    UPDNLMT(sdDisChargeBatCurrAdjust,Ibat_Set(3.0),Ibat_Set(-3.0));
                }
            }
        }
        else
        {
            sdDisChargeBatCurrAdjust = 0;
            sdChargeBatCurrAdjust = 0;
        }


        g_strClaIn_BuckBoostSetData.dBatDisChargeCurrLimit = dDisChargeCurrRefTmp + sdDisChargeBatCurrAdjust;
        UPDNLMT(g_strClaIn_BuckBoostSetData.dBatDisChargeCurrLimit,Ibat_Set(70),0);
		
        g_strClaIn_BuckBoostSetData.dBatChargeCurrLimit = dChargeCurrRefTmp + sdChargeBatCurrAdjust;
	 
        UPDNLMT(g_strClaIn_BuckBoostSetData.dBatChargeCurrLimit,Ibat_Set(70),0);
    }
    else
    {
        g_strClaIn_BuckBoostSetData.dBatDisChargeCurrLimit = Ibat_Set(70);
        g_strClaIn_BuckBoostSetData.dBatChargeCurrLimit = Ibat_Set(70);
        dDisChargeBatCurrRefTmp = 0;
        sdDisChargeBatCurrAdjust = 0;
        dDisChargeBatCurrLevelTmp = 0;
        dChargeBatCurrRefTmp = 0;
        sdChargeBatCurrAdjust = 0;
        dChargeBatCurrLevelTmp = 0;
        dBatMaxPowerLimitTmp = 0;
    }
}

#endif
