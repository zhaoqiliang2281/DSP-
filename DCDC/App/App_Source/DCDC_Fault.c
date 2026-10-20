/*
 * DCDC_Fault.c
 *
 *  Created on: 2025年6月21日
 *      Author: phone
 */

#ifndef Fault_C
#define Fault_C
//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"
//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------


Union_Alarm upAlarm;  // CAN上传的报警字节

Struct_SysFaultReg g_strSysFaultReg;
Struct_FaultPara g_strFaultSettingData[eFault_ID_End];
///
const Uint16 cFaultInitTable[eFault_ID_End][2] =
{
     //中断型故障
     {eVbat_ShortCircuit_ID,        (FAULTCHECK_EN + EQ_CHECKFLAG + FAULTRECOVER_EN)},
     {eVbus_ShortCircuit_ID,        (FAULTCHECK_EN + EQ_CHECKFLAG + FAULTRECOVER_EN)},
     {eIinstLimit_ID,               (FAULTCHECK_EN + EQ_CHECKFLAG + FAULTRECOVER_EN)},
     {eVbat_OV_ID,                  (FAULTCHECK_EN + HT_CHECKFLAG + FAULTRECOVER_EN)},
     {eVbus_OV_ID,                  (FAULTCHECK_EN + HT_CHECKFLAG + FAULTRECOVER_EN)},
     {eIbat_OC_ID,                  (FAULTCHECK_EN + HT_CHECKFLAG + FAULTRECOVER_EN)},
     //20ms型故障
     {eRelayBreak_ID,               (FAULTCHECK_EN + EQ_CHECKFLAG + FAULTRECOVER_EN)},
     {eBusBuildFail_ID,             (FAULTCHECK_EN + EQ_CHECKFLAG + FAULTRECOVER_EN)},
     {eVbat_OV1_ID,                 (FAULTCHECK_EN + HT_CHECKFLAG + FAULTRECOVER_EN)},
     {eVbat_UV1_ID,                 (FAULTCHECK_EN + LT_CHECKFLAG + FAULTRECOVER_EN)},
     {eVbus_OV1_ID,                 (FAULTCHECK_EN + HT_CHECKFLAG + FAULTRECOVER_EN)},
     {eVbus_UV1_ID,                 (FAULTCHECK_EN + LT_CHECKFLAG + FAULTRECOVER_EN)},
     {eIbat_OC1_ID,                 (FAULTCHECK_EN + HT_CHECKFLAG + FAULTRECOVER_EN)},
     {eIbat_OC2_ID,                 (FAULTCHECK_EN + HT_CHECKFLAG + FAULTRECOVER_EN)},
     {eNoBat_ID,                    (FAULTCHECK_EN + LT_CHECKFLAG + FAULTRECOVER_EN)},
};
//-----------------------------------------------------------------------------
//Adding Private Function
//-----------------------------------------------------------------------------
static inline void Fault_Identify(Struct_FaultPara *p_strFaultCheckPara_s);
static inline void Fault_Recover(Struct_FaultPara *p_strFaultCheckPara_s);
static inline void Fault_GetInput(Uint16 uwFaultType_s);
static inline void Fault_ChkStartCond(void);
static inline void Update_FaultPara(void);
static inline void Update_FaultCode(void);
static inline void Update_FaultRecoverTime(void);
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Fault_System_AppInit(void)
{
    Uint16 uwIndexTmp = 0;

    for(uwIndexTmp = 0;uwIndexTmp < eFault_ID_End;uwIndexTmp++)
    {
        g_strFaultSettingData[uwIndexTmp].unCtrlFlag.all = cFaultInitTable[uwIndexTmp][1];
        g_strFaultSettingData[uwIndexTmp].uwCheckCnt = 0;
        g_strFaultSettingData[uwIndexTmp].uwRecoverCnt = 0;
    }

    g_strFaultSettingData[eVbat_ShortCircuit_ID].uwCheckTime = TSK200US_TIMER100US;
    g_strFaultSettingData[eVbat_ShortCircuit_ID].dChkProtectLimit = TRUE;
    g_strFaultSettingData[eVbat_ShortCircuit_ID].uwRecoverTime = TSK30S_TIMER1S;
    g_strFaultSettingData[eVbat_ShortCircuit_ID].dChkRecoverLmit = FALSE;

    g_strFaultSettingData[eVbus_ShortCircuit_ID].uwCheckTime = TSK200US_TIMER100US;
    g_strFaultSettingData[eVbus_ShortCircuit_ID].dChkProtectLimit = TRUE;
    g_strFaultSettingData[eVbus_ShortCircuit_ID].uwRecoverTime = TSK30S_TIMER1S;
    g_strFaultSettingData[eVbus_ShortCircuit_ID].dChkRecoverLmit = FALSE;

    g_strFaultSettingData[eVbat_OV_ID].uwCheckTime = TSK200US_TIMER100US;
    g_strFaultSettingData[eVbat_OV_ID].dChkProtectLimit = Vbat_Set(180);
    g_strFaultSettingData[eVbat_OV_ID].uwRecoverTime = TSK30S_TIMER1S;
    g_strFaultSettingData[eVbat_OV_ID].dChkRecoverLmit = Vbat_Set(200);

    g_strFaultSettingData[eVbus_OV_ID].uwCheckTime = TSK200US_TIMER100US;
    g_strFaultSettingData[eVbus_OV_ID].dChkProtectLimit = Vbat_Set(220);
    g_strFaultSettingData[eVbus_OV_ID].uwRecoverTime = TSK30S_TIMER1S;
    g_strFaultSettingData[eVbus_OV_ID].dChkRecoverLmit = Vbat_Set(180);

    g_strFaultSettingData[eIbat_OC_ID].uwCheckTime = TSK200US_TIMER100US;
    g_strFaultSettingData[eIbat_OC_ID].dChkProtectLimit = Ibat_Set(12);
    g_strFaultSettingData[eIbat_OC_ID].uwRecoverTime = TSK30S_TIMER1S;
    g_strFaultSettingData[eIbat_OC_ID].dChkRecoverLmit = Ibat_Set(10);
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
#pragma CODE_SECTION(Fault_Task2_RealTimeDone,"ramfuncs");
void Fault_Task2_RealTimeDone(void)
{
    Uint16 uwIndexTmp = 0;

    Fault_GetInput(eINT);

    for(uwIndexTmp = eINTFault_ID_Begin;uwIndexTmp < eINTFault_ID_End;uwIndexTmp++)
    {
        Fault_Identify(&g_strFaultSettingData[uwIndexTmp]);
    }

    Update_FaultCode();
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Fault_Task_20ms(void)
{
//    Uint16 uwIndexTmp = 0;

//    Fault_GetInput(e20ms);
//
//    for(uwIndexTmp = e20msFault_ID_Begin;uwIndexTmp < e20msFault_ID_End;uwIndexTmp++)
//    {
//        Fault_Identify(&g_strFaultSettingData[uwIndexTmp]);
//    }

    Update_FaultPara();
    Fault_ChkStartCond(); // 控制开始检测过流短路等
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Fault_Task_1s(void)
{
    Uint16 uwIndexTmp = 0;

    Update_FaultRecoverTime();  // 错误恢复

    for(uwIndexTmp = eFault_ID_Begin;uwIndexTmp < eFault_ID_End;uwIndexTmp++)
    {
        Fault_Recover(&g_strFaultSettingData[uwIndexTmp]);
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
static inline void Update_FaultPara(void)
{

}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
static inline void Fault_ChkStartCond(void)
{
    if(eOpenLoop == uwDeviceWorkMode)
    {
        g_strFaultSettingData[eVbat_ShortCircuit_ID].unCtrlFlag.bit.bChkStartFlag = FALSE;
        g_strFaultSettingData[eVbus_ShortCircuit_ID].unCtrlFlag.bit.bChkStartFlag = FALSE;
        g_strFaultSettingData[eVbat_OV_ID].unCtrlFlag.bit.bChkStartFlag = FALSE;
        g_strFaultSettingData[eVbus_OV_ID].unCtrlFlag.bit.bChkStartFlag = FALSE;
        g_strFaultSettingData[eIbat_OC_ID].unCtrlFlag.bit.bChkStartFlag = FALSE;

        g_strFaultSettingData[eVbat_ShortCircuit_ID].unCtrlFlag.bit.bCheckEn = FALSE;
        g_strFaultSettingData[eVbus_ShortCircuit_ID].unCtrlFlag.bit.bCheckEn = FALSE;
        g_strFaultSettingData[eVbat_OV_ID].unCtrlFlag.bit.bCheckEn = FALSE;
        g_strFaultSettingData[eVbus_OV_ID].unCtrlFlag.bit.bCheckEn = FALSE;
        g_strFaultSettingData[eIbat_OC_ID].unCtrlFlag.bit.bCheckEn = FALSE;
    }
    else
    {
        g_strFaultSettingData[eVbat_ShortCircuit_ID].unCtrlFlag.bit.bChkStartFlag = TRUE;
        g_strFaultSettingData[eVbus_ShortCircuit_ID].unCtrlFlag.bit.bChkStartFlag = TRUE;
        g_strFaultSettingData[eVbat_OV_ID].unCtrlFlag.bit.bChkStartFlag = TRUE;
        g_strFaultSettingData[eVbus_OV_ID].unCtrlFlag.bit.bChkStartFlag = TRUE;
        g_strFaultSettingData[eIbat_OC_ID].unCtrlFlag.bit.bChkStartFlag = TRUE;
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
static inline void Update_FaultRecoverTime(void)
{
    Uint16 uwIndexTmp = 0;

    g_strSysFaultReg.uwMaxCountDownTime = 0;

    for(uwIndexTmp = eFault_ID_Begin;uwIndexTmp < eFault_ID_End;uwIndexTmp++)
    {
        if(TRUE == g_strFaultSettingData[uwIndexTmp].unCtrlFlag.bit.bFaulted)
        {
            if(g_strSysFaultReg.uwMaxCountDownTime < g_strFaultSettingData[uwIndexTmp].uwCountDownTime)
            {
                g_strSysFaultReg.uwMaxCountDownTime = g_strFaultSettingData[uwIndexTmp].uwCountDownTime;
            }
        }
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
#pragma CODE_SECTION(Update_FaultCode,"ramfuncs");
static inline void Update_FaultCode(void)
{
    g_strSysFaultReg.unINTFault.bit.bVbat_ShortCircuit = g_strFaultSettingData[eVbat_ShortCircuit_ID].unCtrlFlag.bit.bFaulted;
    g_strSysFaultReg.unINTFault.bit.bVbus_ShortCircuit = g_strFaultSettingData[eVbus_ShortCircuit_ID].unCtrlFlag.bit.bFaulted;
    g_strSysFaultReg.unINTFault.bit.bVbat_OV = g_strFaultSettingData[eVbat_OV_ID].unCtrlFlag.bit.bFaulted;
    g_strSysFaultReg.unINTFault.bit.bVbus_OV = g_strFaultSettingData[eVbus_OV_ID].unCtrlFlag.bit.bFaulted;

    g_strSysFaultReg.un20msFault.bit.bVbat_OV1 = g_strFaultSettingData[eVbat_OV1_ID].unCtrlFlag.bit.bFaulted;
    g_strSysFaultReg.un20msFault.bit.bVbus_OV1 = g_strFaultSettingData[eVbus_OV1_ID].unCtrlFlag.bit.bFaulted;

    if((0 != g_strSysFaultReg.unINTFault.all)||(0 != g_strSysFaultReg.un20msFault.all))
    {
        g_strSysFaultReg.uwSysFaultFlag = TRUE;
    }
    else
    {
        g_strSysFaultReg.uwSysFaultFlag = FALSE;
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
#pragma CODE_SECTION(Fault_GetInput,"ramfuncs");
static inline void Fault_GetInput(Uint16 uwFaultType_s)
{
    switch(uwFaultType_s)
    {
        case eINT:
        {
            g_strFaultSettingData[eVbat_ShortCircuit_ID].dInputVal = 0;
            g_strFaultSettingData[eVbus_ShortCircuit_ID].dInputVal = 0;
            g_strFaultSettingData[eVbat_OV_ID].dInputVal = g_strAdcData[eVbat].dRealVal;
            g_strFaultSettingData[eVbus_OV_ID].dInputVal = g_strAdcData[eVbus].dRealVal;
            g_strFaultSettingData[eIbat_OC_ID].dInputVal = 0;
        }
        break;

        case e20ms:
        {

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
Refer documents:
******************************************************************************/
#pragma CODE_SECTION(Fault_Identify,"ramfuncs");
static inline void Fault_Identify(Struct_FaultPara *p_strFaultCheckPara_s)
{

    if(FALSE == p_strFaultCheckPara_s->unCtrlFlag.bit.bCheckEn)
    {
        p_strFaultCheckPara_s->uwCheckCnt = 0;
        p_strFaultCheckPara_s->uwCheckTime = 0;
        return;
    }

    if(EQ_CHECKFLAG_START == p_strFaultCheckPara_s->unCtrlFlag.tag.CheckFlag)
    {
        if(p_strFaultCheckPara_s->dInputVal == TRUE)
        {
//            p_strFaultCheckPara_s->uwCheckCnt++;
//            if(p_strFaultCheckPara_s->uwCheckCnt >= p_strFaultCheckPara_s->uwCheckTime)
//            {
                p_strFaultCheckPara_s->uwCheckCnt = 0;
                p_strFaultCheckPara_s->unCtrlFlag.bit.bFaulted = TRUE;
//            }
        }
        else
        {
            p_strFaultCheckPara_s->uwCheckCnt = 0;
        }
    }
    else if(HT_CHECKFLAG_START == p_strFaultCheckPara_s->unCtrlFlag.tag.CheckFlag)
    {
        if(p_strFaultCheckPara_s->dInputVal >= p_strFaultCheckPara_s->dChkProtectLimit)
        {
            p_strFaultCheckPara_s->uwCheckCnt++;
            if(p_strFaultCheckPara_s->uwCheckCnt >= p_strFaultCheckPara_s->uwCheckTime)
            {
                p_strFaultCheckPara_s->uwCheckCnt = 0;
                p_strFaultCheckPara_s->unCtrlFlag.bit.bFaulted = TRUE;
            }
        }
        else
        {
            if(p_strFaultCheckPara_s->uwCheckCnt > 0)
            {
                p_strFaultCheckPara_s->uwCheckCnt--;
            }
        }
    }
    else if(LT_CHECKFLAG_START == p_strFaultCheckPara_s->unCtrlFlag.tag.CheckFlag)
    {
        if(p_strFaultCheckPara_s->dInputVal <= p_strFaultCheckPara_s->dChkProtectLimit)
        {
            p_strFaultCheckPara_s->uwCheckCnt++;
            if(p_strFaultCheckPara_s->uwCheckCnt >= p_strFaultCheckPara_s->uwCheckTime)
            {
                p_strFaultCheckPara_s->uwCheckCnt = 0;
                p_strFaultCheckPara_s->unCtrlFlag.bit.bFaulted = TRUE;
            }
        }
        else
        {
            if(p_strFaultCheckPara_s->uwCheckCnt > 0)
            {
                p_strFaultCheckPara_s->uwCheckCnt--;
            }
        }
    }
    else
    {
        p_strFaultCheckPara_s->uwCheckCnt = 0;
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
#pragma CODE_SECTION(Fault_Recover,"ramfuncs");
static inline void Fault_Recover(Struct_FaultPara *p_strFaultCheckPara_s)
{
    Uint16 uwCheckFaultFlagTmp = 0;

    if(FALSE == p_strFaultCheckPara_s->unCtrlFlag.bit.bCheckEn)
    {
        p_strFaultCheckPara_s->uwRecoverCnt = 0;
        p_strFaultCheckPara_s->uwRecoverTimes = 0;
        p_strFaultCheckPara_s->uwCountDownTime = 0;
        p_strFaultCheckPara_s->unCtrlFlag.bit.bFaulted = FALSE;
        return;
    }

    uwCheckFaultFlagTmp = (p_strFaultCheckPara_s->unCtrlFlag.tag.CheckFlag & 0x1F) >> 1;
    if((EQ_FAULTFLAG == uwCheckFaultFlagTmp)
     &&(TRUE == p_strFaultCheckPara_s->unCtrlFlag.bit.bRecoverEn))
    {
        if(p_strFaultCheckPara_s->dInputVal == FALSE)
        {
            p_strFaultCheckPara_s->uwRecoverCnt++;
            if(p_strFaultCheckPara_s->uwRecoverCnt >= p_strFaultCheckPara_s->uwRecoverTime)
            {
                p_strFaultCheckPara_s->uwRecoverCnt = 0;
                p_strFaultCheckPara_s->unCtrlFlag.bit.bFaulted = FALSE;
            }
        }
    }
    else if((HT_FAULTFLAG == uwCheckFaultFlagTmp)
          &&(TRUE == p_strFaultCheckPara_s->unCtrlFlag.bit.bRecoverEn))
    {
        if(p_strFaultCheckPara_s->dInputVal <= p_strFaultCheckPara_s->dChkRecoverLmit)
        {
            p_strFaultCheckPara_s->uwRecoverCnt++;
            if(p_strFaultCheckPara_s->uwRecoverCnt >= p_strFaultCheckPara_s->uwRecoverTime)
            {
                p_strFaultCheckPara_s->uwRecoverCnt = 0;
                p_strFaultCheckPara_s->unCtrlFlag.bit.bFaulted = FALSE;
            }
        }
        else
        {
            if(p_strFaultCheckPara_s->uwRecoverCnt > 0)
            {
                p_strFaultCheckPara_s->uwRecoverCnt--;
            }
        }
    }
    else if((LT_FAULTFLAG == uwCheckFaultFlagTmp)
          &&(TRUE == p_strFaultCheckPara_s->unCtrlFlag.bit.bRecoverEn))
    {
        if(p_strFaultCheckPara_s->dInputVal >= p_strFaultCheckPara_s->dChkRecoverLmit)
        {
            p_strFaultCheckPara_s->uwRecoverCnt++;
            if(p_strFaultCheckPara_s->uwRecoverCnt >= p_strFaultCheckPara_s->uwRecoverTime)
            {
                p_strFaultCheckPara_s->uwRecoverCnt = 0;
                p_strFaultCheckPara_s->unCtrlFlag.bit.bFaulted = FALSE;
            }
        }
        else
        {
            if(p_strFaultCheckPara_s->uwRecoverCnt > 0)
            {
                p_strFaultCheckPara_s->uwRecoverCnt--;
            }
        }
    }
    else//bRecoverEn||bFaulted == FALSE
    {
        p_strFaultCheckPara_s->uwRecoverCnt = 0;
    }


    if(TRUE == p_strFaultCheckPara_s->unCtrlFlag.bit.bFaulted)//计算清除故障的剩余时间
    {
        p_strFaultCheckPara_s->uwCountDownTime = p_strFaultCheckPara_s->uwRecoverTime - p_strFaultCheckPara_s->uwRecoverCnt;
    }
    else
    {
        p_strFaultCheckPara_s->uwCountDownTime = 0;
    }
}
#endif  //Fault_C
