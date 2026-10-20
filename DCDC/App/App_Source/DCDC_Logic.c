/*
 * DCDC_Logic.c
 *
 *  Created on: 2025年6月20日
 *      Author: phone
 */
#ifndef Logic_C
#define Logic_C
//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"
//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------

Union_Flag_SysLogic unSysLogicFlag;
Uint16 uwBatWorkMode;
Uint16 uwDeviceWorkMode;
Uint16 uwStatusChangeDelayCnt;
Uint16 uwExitStopCondCnt;
//-----------------------------------------------------------------------------
//Adding Private Function
//-----------------------------------------------------------------------------
void Logic_FsmSwitch(void);
void Logic_CheckStopCond(void);
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents: 50US
******************************************************************************/
#pragma CODE_SECTION(Logic_Task1_RealTimeDone,"ramfuncs");
void Logic_Task1_RealTimeDone(void)
{
    Logic_CheckStopCond();
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
extern void Logic_Task_1ms(void)
{
    Logic_FsmSwitch();
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Logic_System_AppInit(void)
{
    unSysLogicFlag.bit.bStopCond = TRUE;
    unSysLogicFlag.bit.bFaultCond = FALSE;
    unSysLogicFlag.bit.bSysStatus = eSysInit;

    uwBatWorkMode = eDCDCStop;
    uwDeviceWorkMode = eNormalRunning;
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:  1ms
******************************************************************************/
void Logic_FsmSwitch(void)
{
	int32 vTMP=0;
	int16  preCokFlag=0;
	int16  uwPreDelayCntEND=0;
		
    switch(Logic_GetSysStatus())
    {
        case eSysInit:
        {
          if(TRUE == unAdcFlag.bit.bCalcBiasFinish)
           {
                Logic_SetSysStatus(eSysStop);
           }
        }
        break;

        case eSysStop: // 这里启动
        {
            if(FALSE == unSysLogicFlag.bit.bStopCond)
            {
 
                    Logic_SetSysStatus(eSysStandby);
            
            }
			
		 uwStatusChangeDelayCnt++;
		 if(uwStatusChangeDelayCnt >= TSK20MS_TIMER1MS)  // 20ms停机
		 {
			 uwStatusChangeDelayCnt = 0;
			 //Relay_P_C_OFF();
			 Relay_Charge_OFF();
			 Relay_DisCharge_OFF();
		 }	
			canSetPra.preChargeFlag=0;
        }
        break;

        case eSysStandby:
        {
            uwStatusChangeDelayCnt++;
            if(uwStatusChangeDelayCnt >= TSK20MS_TIMER1MS)
            {
                uwStatusChangeDelayCnt = 0;
                Logic_SetSysStatus(eSysStart);
            }
        }
        break;

        case eSysStart:
        {
            uwStatusChangeDelayCnt++;
            if(uwStatusChangeDelayCnt >= TSK20MS_TIMER1MS)
            {
                uwStatusChangeDelayCnt = 0;
                Logic_SetSysStatus(eSysOperate);

				// 先预充
				if((g_strAdcData[eVport].dRmsVal>5000)||(g_strAdcData[eVbat].dRmsVal>5000)) // ?5V时需要缓启
					{
						canSetPra.preChargeFlag=1;
						uwBatWorkMode = eCharge;
					}
				else
					{
					// Relay_P_C_ON();
                    if(canSetPra.batWorkMode==MODE_CHARGE)
                    {
                        Relay_Charge_ON();
                    }
                    else if(canSetPra.batWorkMode==MODE_DISCHARGE)
                    {
                        Relay_DisCharge_ON();
                    }
					canSetPra.preChargeFlag=0;
				}
			}
        }
        break;

        case eSysOperate:
        {
			if(canSetPra.preChargeFlag==1)
			{
			            preCokFlag=0;
						if(uwStatusChangeDelayCnt <= (TSK1S_TIMER1MS*3)) // 最大预充3S
							uwStatusChangeDelayCnt++;
						if(uwStatusChangeDelayCnt == TSK1S_TIMER1MS*3) // 预充缓启
 						{
						     preCokFlag=1;
						}

                        if(g_strAdcData[eVport].dRmsVal>g_strAdcData[eVbat_Relay].dRmsVal)
                            {
                                vTMP = g_strAdcData[eVport].dRmsVal-g_strAdcData[eVbat_Relay].dRmsVal;
                        }
                        else
                            {
                                vTMP = g_strAdcData[eVbat_Relay].dRmsVal - g_strAdcData[eVport].dRmsVal;
                        }

                        if(vTMP<5000) // 2V内为预充完成
                        {
                            preCokFlag=1;
                        }

						if(preCokFlag==1)  // 预充完成
						{
						      // Relay_P_C_ON();
							    uwStatusChangeDelayCnt = 0;
								if(canSetPra.batWorkMode==MODE_STOP)  // 预充完成后再回到充电或者放电
								{
									uwBatWorkMode = eDCDCStop;
								}
								else if(canSetPra.batWorkMode==MODE_CHARGE)
								{
									uwBatWorkMode = eCharge;
									Relay_Charge_ON();
								}
								else if(canSetPra.batWorkMode==MODE_DISCHARGE)
								{
									uwBatWorkMode = eDisCharge;
									Relay_DisCharge_ON();
								}
								else if(canSetPra.batWorkMode==MODE_SPACE)
								{
									uwBatWorkMode = eDCDCStop;
								}
								canSetPra.preChargeFlag=0;
						}
			}
			else
			{
			    canSetPra.runReadyFlag=1;
			}

            //中断里面处理
        }
        break;

        case eSysFault:
        {
            if(FALSE == unSysLogicFlag.bit.bFaultCond)
            {
                Logic_SetSysStatus(eSysStop);
            }
        }
        break;

        default:
        break;
    }
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents: 50US
******************************************************************************/
void Logic_CheckStopCond(void)
{
 //   if((ON == g_uwSettingData_ModBus[eSettingData_ManualSwitch])
 if((canSetPra.stRunFlag == 1) // 用CAN的启动
     &&(FALSE == g_strSysFaultReg.uwSysFaultFlag))//没有故障
    {
        uwExitStopCondCnt++;
        if(uwExitStopCondCnt >= TSK500MS_TIMER50US)
        {
            uwExitStopCondCnt = 0;
            unSysLogicFlag.bit.bStopCond = FALSE;
        }

    }
    else
    {
        unSysLogicFlag.bit.bStopCond = TRUE;
    }

    if(TRUE == g_strSysFaultReg.uwSysFaultFlag)
    {
        Logic_SetSysStatus(eSysFault);
    }
}

#endif
