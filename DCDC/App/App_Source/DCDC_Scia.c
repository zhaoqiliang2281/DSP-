/*
 * DCDC_Scia.c
 *
 *  Created on: 2025年6月13日
 *      Author: ThinkPad
 */


#ifndef SCIA_C
#define SCIA_C

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"
//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//
#define Scia_SlaveAddress       1
#define Scia_MAX_RevLength      30
#define Scia_MAX_SendLength     100
Uint16 g_uwSciaRevData[Scia_MAX_RevLength];
Uint16 g_uwSciaSendData[Scia_MAX_SendLength];

Struct_CommPara g_strCommPara;
Uint16 g_uwRunningData_ModBus[eRunningData_END];
Uint16 g_uwSettingData_ModBus[eSettingData_END];
const Uint16 constSettingData_Table[eSettingData_END][3] =
{
     //默认值，最小值，最大值
     {0,0,1},         //00 eSettingData_ManualSwitch   通过MODBUS启动 写1启动
     {0,0,1000},      //01 eSettingData_BatWorkMode    工作模式：0初始化 1停机 2充电 3放电  // 改变就停机 
     {0,0,1000},      //02 eSettingData_DeviceMode     设备运行模式：0正常           1开环   
     {100,0,1000},    //03 eSettingData_PowerRateSet   功率百分比，程序里面最大3000W（RATED_POWER）
     {0,0,1000},      //04 eSettingData_SystemConnect  未知？
     {200,0,20000},    //05 eSettingData_VbusRef        母线电压：单位10mV 
     {0,0,1000},      //06 eSettingData_VbusLoopKp  母线电压Kp
     {0,0,1000},      //07 eSettingData_VbusLoopKi  母线电压Ki
     {0,0,1000},      //08 eSettingData_VbatLoopKp  电池电压Kp
     {0,0,1000},      //09 eSettingData_VbatLoopKi  电池电压Ki
     {0,0,1000},      //10 eSettingData_IbatLoopKp  电池电流Kp
     {0,0,1000},      //11 eSettingData_IbatLoopKi  电池电流Ki
     {100,0,20000},    //12 eSettingData_VbatChrgRef    充电电压：单位10mV 
     {200,0,15000},    //13 eSettingData_IbatChrgRef    充电电流：单位10mA
     {100,0,20000},    //14 eSettingData_VbatDisChrgRef 放电电压：单位10mV 
     {200,0,15000},    //15 eSettingData_IbatDisChrgRef  放电电流：单位10mA 
     
     {0,0,65535},     //16 eSettingData_DebugPara1  CLA调用了 放电百分比
     {0,0,65535},     //17 eSettingData_DebugPara2  CLA调用了 充电百分比
     
     {0,0,65535},     //18 eSettingData_DebugPara3
     {0,0,65535},     //19 eSettingData_DebugPara4
     {0,0,65535},     //20 eSettingData_DebugPara5
     {0,0,65535},     //21 eSettingData_DebugPara6
     {0,0,65535},     //22 eSettingData_Rsvd1
     {0,0,65535},     //23 eSettingData_Rsvd2
     {0,0,65535},     //24 eSettingData_Rsvd3
     {0,0,65535},     //25 eSettingData_Rsvd4
     {0,0,65535},     //26 eSettingData_Rsvd5
     {0,0,65535},     //27 eSettingData_Rsvd6	eSettingData_Rsvd7, //28
//     {0,0,65535},     //	eSettingData_Rsvd8, //29
//     {0,0,65535},     //eSettingData_Rsvd9, //30
//     {0,0,65535},     //eBackData_dVbusRealVal,
//     {0,0,65535},     //eBackData_dVbatRelayRealVal,
//     {0,0,65535},     //eBackData_dVportRealVal,
//     {0,0,65535},     //eBackData_dVbatRealVal,
//     {0,0,65535},     //eBackData_dIbuckboostRealVal, // 35
//     {0,0,65535},     //eBackData_dIbatRealVal,
//     {0,0,65535},     //eBackData_dPbatRealVal, // 37
//     {0,0,65535},     //eBackData_eVbus_dRmsVal,
//     {0,0,65535},     //eBackData_eVbat_Relay_dRmsVal,
//     {0,0,65535},     //eBackData_eVport_dRmsVal, // 40
//     {0,0,65535},     //eBackData_eVbat_dRmsVal,
//     {0,0,65535},     //eBackData_eIbuckboost_dRmsVal,
//     {0,0,65535},     //eBackData_eIbat_dRmsVal, // 43
//     {0,0,65535},     //eBackData_Rsvd1,
//     {0,0,65535},     //eBackData_Rsvd2, // 45
//     {0,0,65535},     //eBackData_Rsvd3,
//     {0,0,65535},     //eBackData_Rsvd4,
//     {0,0,65535},     //eBackData_Rsvd5,
//     {0,0,65535},     //eBackData_Rsvd6,
//     {0,0,65535},     //eBackData_Rsvd7, // 50
     
};


//-----------------------------------------------------------------------------
//Adding Private Function Prototypes (i.e. static)
//-----------------------------------------------------------------------------
static inline void SCIA_UploadData(void);
static inline void SCIA_ModBus_RevCom(void);
static inline void SCIA_DealModBusRevData(void);
static inline void SCIA_ModBusDataProcess(Uint16 uwFuncCode_s,Uint16 uwRegAddr_s,Uint16 uwRegNum_s);
static inline void SCIA_SendFaultMsg(Uint16 uwFaultCode_s);
static inline void SCIA_CheckCommLink(void);
static inline Uint16 SCI_CRCCheck(Uint16 *uwPuchMsg_s,Uint16 uwDataLen_s,Uint16 uwCRCHi_s,Uint16 uwCRCLo_s);
/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents:
******************************************************************************/
void SCIA_System_AppInit(void)
{
    Uint16 uwSetDataIndexTmp;

    g_strCommPara.uwDataRecvOverTime = 200;
    g_strCommPara.uwDataSendOverTime = 200;

    for(uwSetDataIndexTmp = 0;uwSetDataIndexTmp < eSettingData_END;uwSetDataIndexTmp++)
    {
        g_uwSettingData_ModBus[uwSetDataIndexTmp] = constSettingData_Table[uwSetDataIndexTmp][0];
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
void SCIA_Task_5ms(void)
{
    SCIA_CheckCommLink();
}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents: 20ms
******************************************************************************/
void SCIA_Task_20ms(void)
{
    SCIA_UploadData();    // 加载串口数据到运行
    SCIA_ModBus_RevCom(); // 串口接收数据的处理
}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents:
******************************************************************************/
extern void SCIA_Task_1s(void)
{

}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents:  20ms更新
******************************************************************************/
static inline void SCIA_UploadData(void)
 {
	
	g_uwRunningData_ModBus[eBackData_dVbusRealVal] = (Uint16)(g_strAdcData[eVbus].dRealVal /10); 
	g_uwRunningData_ModBus[eBackData_dVbatRelayRealVal] = (Uint16)(g_strAdcData[eVbat_Relay].dRealVal/10);
	g_uwRunningData_ModBus[eBackData_dVportRealVal] = (Uint16)(g_strAdcData[eVport].dRealVal/10);
	g_uwRunningData_ModBus[eBackData_dVbatRealVal] = (Uint16)( g_strAdcData[eVbat].dRealVal/10);
	g_uwRunningData_ModBus[eBackData_dIbuckboostRealVal] = (Uint16)(g_strAdcData[eIbuckboost].dRealVal);
	g_uwRunningData_ModBus[eBackData_dIbatRealVal] = (Uint16)(g_strAdcData[eIbat].dRealVal);
	g_uwRunningData_ModBus[eBackData_dPbatRealVal] = (Uint16)(g_strAdcData[ePbat].dRealVal/10);  // 6
	
	g_uwRunningData_ModBus[eBackData_eVbus_dRmsVal] = (Uint16)(g_strAdcData[eVbus].dRmsVal/10);
	g_uwRunningData_ModBus[eBackData_eVbat_Relay_dRmsVal] = (Uint16)(g_strAdcData[eVbat_Relay].dRmsVal/10);
	g_uwRunningData_ModBus[eBackData_eVport_dRmsVal] = (Uint16)(g_strAdcData[eVport].dRmsVal/10); 
	g_uwRunningData_ModBus[eBackData_eVbat_dRmsVal] = (Uint16)(g_strAdcData[eVbat].dRmsVal/10);
	g_uwRunningData_ModBus[eBackData_eIbuckboost_dRmsVal] = (Uint16)(g_strAdcData[eIbuckboost].dRmsVal);
	g_uwRunningData_ModBus[eBackData_eIbat_dRmsVal] = (Uint16)(g_strAdcData[eIbat].dRmsVal);	 // 12
	
	g_uwRunningData_ModBus[eBackData_eVbus_dRmsValBitFilter] = (Uint16)(g_strAdcData[eVbus].dRmsValBitFilter/10);
	g_uwRunningData_ModBus[eBackData_eVbat_Relay_dRmsValBitFilter] = (Uint16)(g_strAdcData[eVbat_Relay].dRmsValBitFilter/10);
	g_uwRunningData_ModBus[eBackData_eVport_dRmsValBitFilter] = (Uint16)(g_strAdcData[eVport].dRmsValBitFilter/10); 
	g_uwRunningData_ModBus[eBackData_eVbat_dRmsValBitFilter] = (Uint16)(g_strAdcData[eVbat].dRmsValBitFilter/10);
	g_uwRunningData_ModBus[eBackData_eIbuckboost_dRmsValBitFilter] = (Uint16)(g_strAdcData[eIbuckboost].dRmsValBitFilter);
	g_uwRunningData_ModBus[eBackData_eIbat_dRmsValBitFilter] = (Uint16)(g_strAdcData[eIbat].dRmsValBitFilter); // 18


//    g_uwRunningData_ModBus[eRunningData_VBusRef] = g_strClaIn_BuckBoostSetData.dBusRef/10; // 电压参考
//    g_uwRunningData_ModBus[eRunningData_VBatChrgRef] = g_strClaIn_BuckBoostSetData.dChargeBatVoltRef/10; // 充电电压
//    g_uwRunningData_ModBus[eRunningData_VBatDisChrgRef] =g_strClaIn_BuckBoostSetData.dDisChargeBatVoltRef/10; // 放电电压
//    g_uwRunningData_ModBus[eRunningData_IbatRef] = g_strClaIn_BuckBoostSetData.dBatChargeCurrLimit/10; // 充电电流
//    g_uwRunningData_ModBus[eRunningData_Pbat] = g_strClaIn_BuckBoostSetData.dBatDisChargeCurrLimit/10;      // 23 放电电流
//    g_uwRunningData_ModBus[eRunningData_Ploadlimit] = canSetPra.tack1sCount;

    g_uwRunningData_ModBus[eRunningData_VBusRef] = (Uint16)(canSetPra.bufRef/10); // 电压参考
    g_uwRunningData_ModBus[eRunningData_VBatChrgRef] = (Uint16)CalPra.V_ADC_H; // 充电电压
    g_uwRunningData_ModBus[eRunningData_VBatDisChrgRef] =(Uint16)CalPra.V_Meter_L; // 放电电压
    g_uwRunningData_ModBus[eRunningData_IbatRef] = (Uint16)CalPra.V_ADC_L; // 充电电流
    g_uwRunningData_ModBus[eRunningData_Pbat] = (Uint16)(CalPra.floatVK*100);      // 23 放电电流
    g_uwRunningData_ModBus[eRunningData_Ploadlimit] = (Uint16)(CalPra.floatVB*100);


    g_uwRunningData_ModBus[eRunningData_BuckPWM] = uwBatWorkMode; // 0停机，1充电，2放电
    g_uwRunningData_ModBus[eRunningData_BoostPWM] = uwDeviceWorkMode; // 0正常运行，1开环
    g_uwRunningData_ModBus[eRunningData_SystemStatus] = Logic_GetSysStatus();
    g_uwRunningData_ModBus[eRunningData_VersionData] = (Uint16)canSetPra.swSendID; // 28

//    g_uwRunningData_ModBus[eRunningData_FaultCode] = g_uwSettingData_ModBus[eSettingData_ManualSwitch];
//    g_uwRunningData_ModBus[eRunningData_FaultRecoverTime] = g_strClaIn_BuckBoostSetData.unControlFlag.all; // 30

    g_uwRunningData_ModBus[eRunningData_FaultCode] = (Uint16)g_strClaIn_AdcRawData.udVbatAdcVal;
    g_uwRunningData_ModBus[eRunningData_FaultRecoverTime] =(Uint16)g_strClaIn_AdcRawData.udIbatAdcVal; // 30

//    g_uwRunningData_ModBus[eRunningData_Rsvd1] = (Uint16)(uwLoopOutData[0]);
//    g_uwRunningData_ModBus[eRunningData_Rsvd2] = (Uint16)(uwLoopOutData[1]);
//    g_uwRunningData_ModBus[eRunningData_Rsvd3] = (Uint16)(uwLoopOutData[2]);
//    g_uwRunningData_ModBus[eRunningData_Rsvd4] = (Uint16)(uwLoopOutData[3]);
//	g_uwRunningData_ModBus[eRunningData_Rsvd5] = (Uint16)(uwLoopOutData[4]); // 35
//
//	g_uwRunningData_ModBus[eRunningData_Rsvd6] = (Uint16)(uwLoopOutData[5]);

    g_uwRunningData_ModBus[eRunningData_Rsvd1] = (Uint16)(CalPra.I_Meter_H);
    g_uwRunningData_ModBus[eRunningData_Rsvd2] = (Uint16)(CalPra.I_ADC_H);
    g_uwRunningData_ModBus[eRunningData_Rsvd3] = (Uint16)(CalPra.I_Meter_L);
    g_uwRunningData_ModBus[eRunningData_Rsvd4] = (Uint16)(CalPra.I_ADC_L);
    g_uwRunningData_ModBus[eRunningData_Rsvd5] = (Uint16)(CalPra.floatIK*100); // 35
    g_uwRunningData_ModBus[eRunningData_Rsvd6] = (Uint16)((0-CalPra.floatIB)/100);


	g_uwRunningData_ModBus[eRunningData_Rsvd7] = (Uint16)(NTC_Boost.dRmsVal);
	g_uwRunningData_ModBus[eRunningData_Rsvd8] = (Uint16)(canSetPra.stRunFlag);
	g_uwRunningData_ModBus[eRunningData_Rsvd9] = EPwm1Regs.CMPA.half.CMPA;  // 充电
	g_uwRunningData_ModBus[eRunningData_Rsvd10] = EPwm1Regs.CMPB;  // 放电

 //   uwBatWorkMode = g_uwSettingData_ModBus[eSettingData_BatWorkMode];
 //   uwDeviceWorkMode = g_uwSettingData_ModBus[eSettingData_DeviceMode];
}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents: 20ms
******************************************************************************/
static inline void SCIA_ModBus_RevCom(void)
{
    if(TRUE == g_strCommPara.uwDataRecvFlag)
    {
        if(Scia_SlaveAddress == g_uwSciaRevData[0])
        {
            SCIA_DealModBusRevData();
        }

        g_strCommPara.uwDataRecvNum = 0;
        g_strCommPara.uwDataRecvFlag = FALSE;
//        g_strCommPara.uwStartSendDataFlag = TRUE;

        SciaRegs.SCICTL2.bit.TXINTENA = 1;
        g_strCommPara.uwDataSendNum = 1;
        SciaRegs.SCITXBUF = g_uwSciaSendData[0] & 0xFF;
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
static inline void SCIA_DealModBusRevData(void)
{
    Uint16 uwCRCTmp,uwCRCHiTmp,uwCRCLoTmp;
    Uint16 uwRegAddrTmp,uwRegNumTmp,uwRegEndTmp;

    uwCRCTmp = SCI_CRCCheck(g_uwSciaRevData,(g_strCommPara.uwDataRecvLen - 2),0xFF,0xFF);
    uwCRCHiTmp = uwCRCTmp >> 8;
    uwCRCLoTmp = uwCRCTmp & 0xFF;

    // CRC正确
    if((uwCRCHiTmp == g_uwSciaRevData[g_strCommPara.uwDataRecvLen - 2])&&(uwCRCLoTmp == g_uwSciaRevData[g_strCommPara.uwDataRecvLen - 1]))
    {
        uwRegAddrTmp = (Uint16)((Uint16)(g_uwSciaRevData[2]<<8) + (Uint16)(g_uwSciaRevData[3]&0xFF));
        uwRegNumTmp = (Uint16)((Uint16)(g_uwSciaRevData[4]<<8) + (Uint16)(g_uwSciaRevData[5]&0xFF));
        uwRegEndTmp = uwRegAddrTmp + uwRegNumTmp;

        if(eGetRunningData == g_uwSciaRevData[1]) // 03功能码读寄存器
        {
            if(uwRegEndTmp <= eRunningData_END)
            {
                SCIA_ModBusDataProcess(g_uwSciaRevData[1],uwRegAddrTmp,uwRegNumTmp);
            }
            else
            {
                SCIA_SendFaultMsg(eRunningDataWrong); // 长度错误
            }
        }
		else if(eSendSingleSetting == g_uwSciaRevData[1]) // 06 功能码写寄存器  
		{
              SCIA_ModBusDataProcess(g_uwSciaRevData[1],uwRegAddrTmp,uwRegNumTmp);
		}
        else if(eSendMultiSetting == g_uwSciaRevData[1]) // 0x10功能码写寄存器  
        {
            if(uwRegEndTmp <= eSettingData_END)
            {
                SCIA_ModBusDataProcess(g_uwSciaRevData[1],uwRegAddrTmp,uwRegNumTmp);
            }
            else
            {
                SCIA_SendFaultMsg(eSettingDataWrong); // 长度错误
            }
        }
    }
    else
    {
        SCIA_SendFaultMsg(eCrcWrong);
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
static inline void SCIA_ModBusDataProcess(Uint16 uwFuncCode_s,Uint16 uwRegAddr_s,Uint16 uwRegNum_s)
{
    Uint16 uwRegIndexTmp,uwRegPosTmp,uwRegValTmp;
    Uint16 uwStrLenTmp;
    Uint16 uwCRCTmp;

    g_uwSciaSendData[0] = g_uwSciaRevData[0];//从机地址
    g_uwSciaSendData[1] = g_uwSciaRevData[1];//功能码
    g_uwSciaSendData[2] = g_uwSciaRevData[2];//起始寄存器号
    g_uwSciaSendData[3] = g_uwSciaRevData[3];
    g_uwSciaSendData[4] = g_uwSciaRevData[4];//寄存器数量
    g_uwSciaSendData[5] = g_uwSciaRevData[5];

    if(eGetRunningData == uwFuncCode_s) // 03读寄存器
    {
        uwStrLenTmp = 2;
		g_uwSciaSendData[uwStrLenTmp++] = uwRegNum_s*2;  // 字节数
        for(uwRegIndexTmp = 0;uwRegIndexTmp < uwRegNum_s;uwRegIndexTmp++)
        {
            uwRegPosTmp = uwRegAddr_s + uwRegIndexTmp;
            g_uwSciaSendData[uwStrLenTmp++] = g_uwRunningData_ModBus[uwRegPosTmp] >> 8;
            g_uwSciaSendData[uwStrLenTmp++] = g_uwRunningData_ModBus[uwRegPosTmp] & 0xFF;
        }
    }
    else if((eSendSingleSetting == uwFuncCode_s)||(eSendMultiSetting == uwFuncCode_s)) // 06写寄存器
    {
        if(eSendSingleSetting == uwFuncCode_s)
        {
			uwStrLenTmp = 4;
			uwRegNum_s=1;
        }
		else if(eSendMultiSetting == uwFuncCode_s)
		{
			uwStrLenTmp = 6;
		}
		
        for(uwRegIndexTmp = 0;uwRegIndexTmp < uwRegNum_s;uwRegIndexTmp++)
        {
            uwRegPosTmp = uwRegAddr_s + uwRegIndexTmp;

            uwRegValTmp = g_uwSciaRevData[uwStrLenTmp++] << 8;
            uwRegValTmp += g_uwSciaRevData[uwStrLenTmp++] & 0xFF;

            // 在最大值、最小值范围内才赋值
            if((uwRegValTmp >= constSettingData_Table[uwRegPosTmp][1])&&(uwRegValTmp <= constSettingData_Table[uwRegPosTmp][2]))
            {
                g_uwSettingData_ModBus[uwRegPosTmp] = uwRegValTmp;
            }

        }
    }

    uwCRCTmp = SCI_CRCCheck(g_uwSciaSendData,uwStrLenTmp,0xFF,0xFF);
    g_uwSciaSendData[uwStrLenTmp++] = uwCRCTmp >> 8;
    g_uwSciaSendData[uwStrLenTmp++] = uwCRCTmp & 0xFF;

    g_strCommPara.uwDataSendLen = uwStrLenTmp;
}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents:
******************************************************************************/
static inline void SCIA_SendFaultMsg(Uint16 uwFaultCode_s)
{
    Uint16 uwStrLenTmp = 0;
    Uint16 uwCRCTmp;

    g_uwSciaSendData[uwStrLenTmp++] = g_uwSciaRevData[0];//从机地址
    g_uwSciaSendData[uwStrLenTmp++] = g_uwSciaRevData[1] + 0x80;//功能码
    g_uwSciaSendData[uwStrLenTmp++] = uwFaultCode_s & 0xFF;

    uwCRCTmp = SCI_CRCCheck(g_uwSciaSendData,uwStrLenTmp,0xFF,0xFF);
    g_uwSciaSendData[uwStrLenTmp++] = uwCRCTmp >> 8;
    g_uwSciaSendData[uwStrLenTmp++] = uwCRCTmp & 0xFF;

    g_strCommPara.uwDataSendLen = uwStrLenTmp;
}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents:
******************************************************************************/
static inline void SCIA_CheckCommLink(void)
{
    if(g_strCommPara.uwDataRecvNum > 0)
    {
        g_strCommPara.uwDataRecvOverCnt++;
        if(g_strCommPara.uwDataRecvOverCnt >= g_strCommPara.uwDataRecvOverTime)
        {
            g_strCommPara.uwDataRecvOverCnt = 0;
            g_strCommPara.uwDataRecvNum = 0;
        }
    }
    else
    {
        g_strCommPara.uwDataRecvOverCnt = 0;
    }

    if(g_strCommPara.uwDataSendNum > 0)
    {
        g_strCommPara.uwDataSendOverCnt++;
        if(g_strCommPara.uwDataSendOverCnt >= g_strCommPara.uwDataSendOverTime)
        {
            g_strCommPara.uwDataSendOverCnt = 0;
            g_strCommPara.uwDataSendNum = 0;
        }
    }
    else
    {
        g_strCommPara.uwDataSendOverCnt = 0;
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
 /*static inline Uint16 SCI_CRCCheck(Uint16 *uwPuchMsg_s,Uint16 uwDataLen_s,Uint16 uwCRCHi_s,Uint16 uwCRCLo_s)
//static inline Uint16 SCI_CRCCheck(Uint16 *updata,u16 len)  
	{  
	  Uint8 uchCRCHi=0xff;  
	  Uint8 uchCRCLo=0xff;  
	  Uint16  uindex;	
	  while(uwDataLen_s--)	
	  {  
	  uindex=uchCRCHi^*uwPuchMsg_s++;  
	  uchCRCHi=uchCRCLo^auchCRCHi[uindex];	
	  uchCRCLo=auchCRCLo[uindex];  
	  }  
	  return (uchCRCHi<<8|uchCRCLo);  
	} */


   const uint16_t crctalbeabs[] = { 
       0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401, 
       0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400 
   };
   
static inline Uint16 SCI_CRCCheck(Uint16 *uwPuchMsg_s,Uint16 uwDataLen_s,Uint16 uwCRCHi_s,Uint16 uwCRCLo_s)
   {
       Uint16 crc = 0xffff; 
       Uint16 i;
      Uint8 ch;
	  Uint16 ret1;
   
      for (i = 0; i < uwDataLen_s; i++) {
          ch = *uwPuchMsg_s++;
          crc = crctalbeabs[(ch ^ crc) & 15] ^ (crc >> 4);
         crc = crctalbeabs[((ch >> 4) ^ crc) & 15] ^ (crc >> 4);
      } 
      
      ret1 = crc>> 8;
      ret1 = ret1 | (crc<< 8); 
     
      return ret1 ;
  }



/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents:
******************************************************************************/
void SCIA_System_PinInit(void)
{
    EALLOW;

    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;   //Enable pull-up for GPIO28 (SCIRXDA)
    //GpioCtrlRegs.GPAPUD.bit.GPIO7 = 0;  //Enable pull-up for GPIO7  (SCIRXDA)
    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;   //Enable pull-up for GPIO29 (SCITXDA)
    //GpioCtrlRegs.GPAPUD.bit.GPIO12 = 0; //Enable pull-up for GPIO12 (SCITXDA)

    // Set qualification for selected pins to asynch only
    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3;  // Asynch input GPIO28 (SCIRXDA)
    //GpioCtrlRegs.GPAQSEL1.bit.GPIO7 = 3;   // Asynch input GPIO7 (SCIRXDA)

    // Configure SCI-A pins using GPIO regs
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;   // Configure GPIO28 for SCIRXDA
    //GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 2;  // Configure GPIO7 for SCIRXDA
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;   // Configure GPIO29 for SCITXDA
    //GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 2;   // Configure GPIO12 for SCITXDA

    EDIS;
}
//******************************************************************************
void SCIA_System_PeripheralInit(void)
{
    /////////////RS485通讯初始化////////////////
    SciaRegs.SCICCR.all             = 0x0007;//1停止位,禁止校验及自测,一异步空闲线,8位
    SciaRegs.SCICTL1.all            = 0x0003;//使能发送/接收
    SciaRegs.SCICTL2.all            = 0x0003;//使能RXRDY/TXRDY中断
    //SciaRegs.SCICTL2.bit.TXINTENA = 0;       //禁止TXRDY中断
    //SciaRegs.SCICTL2.bit.RXBKINTENA = 0;     //禁止RXRDY中断

    SciaRegs.SCIHBAUD = 0x00;  //194-->9600 ;   97--> 19200 ;  48-->38400;  15--->115200    14-->128000
    SciaRegs.SCILBAUD = 0xC2; // C2

    SciaRegs.SCICTL1.all            = 0x0023;//软件复位
    //SciaRegs.SCIFFTX.all            = 0x8040;
    //SciaRegs.SCIFFRX.all            = 0x2040;
    //SciaRegs.SCIFFCT.all            = 0x0;   //禁止波特率自动检验
    //ScibRegs.SCIPRI.bit.FREESOFT  = 0x1;      //在挂起状态立即停止
    SciaRegs.SCIPRI.bit.SOFT        = 0x0;          //在挂起状态立即停止
    //SciaRegs.SCIPRI.bit.FREE        = 0x1;
}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:中断处理函数

Parameter Name list:

ReturnType:

Refer documents:
******************************************************************************/
void SCIA_RXINT_RealTimeDone(void)
{
    //Communicate Wrong
    if((TRUE == SciaRegs.SCIRXST.bit.BRKDT)||(TRUE == SciaRegs.SCIRXST.bit.RXERROR))
    {
        SciaRegs.SCICCR.all             = 0x0007;//1停止位,禁止校验及自测,一异步空闲线,8位
        SciaRegs.SCICTL1.all            = 0x0003;//使能发送/接收
        SciaRegs.SCIHBAUD = 0x00;  //194-->9600 ;   97--> 19200 ;  48-->38400;  15--->115200    14-->128000
        SciaRegs.SCILBAUD = 0xC2;
        SciaRegs.SCICTL1.all            = 0x0023;//软件复位

        g_strCommPara.uwDataRecvNum = 0;
    }

    if(0 == g_strCommPara.uwDataRecvNum)
    {
        g_uwSciaRevData[0] = SciaRegs.SCIRXBUF.all & 0xFF;
        if(Scia_SlaveAddress == g_uwSciaRevData[0])
        {
            g_strCommPara.uwDataRecvNum++;
        }
    }
    else if(1 == g_strCommPara.uwDataRecvNum)
    {
        g_uwSciaRevData[g_strCommPara.uwDataRecvNum++] = SciaRegs.SCIRXBUF.all & 0xFF;
        g_strCommPara.uwFuncCode = g_uwSciaRevData[1];
    }
    else if(g_strCommPara.uwDataRecvNum < 5)
    {
        g_uwSciaRevData[g_strCommPara.uwDataRecvNum++] = SciaRegs.SCIRXBUF.all & 0xFF;
    }
    else if(g_strCommPara.uwDataRecvNum == 5)
    {
        g_uwSciaRevData[g_strCommPara.uwDataRecvNum++] = SciaRegs.SCIRXBUF.all & 0xFF;

        if(eSendSingleSetting == g_strCommPara.uwFuncCode) // 06功能码写
        {
//            g_strCommPara.uwDataRecvLen = ((g_uwSciaRevData[5]<<8) + (g_uwSciaRevData[5]&0xFF))*2 + 8;
            g_strCommPara.uwDataRecvLen =  8; // 06功能写无字节数
        }
		else if(eSendMultiSetting == g_strCommPara.uwFuncCode)
		{
//			  g_strCommPara.uwDataRecvLen = ((g_uwSciaRevData[5]<<8) + (g_uwSciaRevData[5]&0xFF))*2 + 8;
			g_strCommPara.uwDataRecvLen = (g_uwSciaRevData[5] << 1) + 9; // 16功能码有字节数
 		} 		
        else//eGetRunningData
        {
            g_strCommPara.uwDataRecvLen = 8;//g_strCommPara.uwDataRecvNum + 2;
            g_strCommPara.uwDataRecvFlag = TRUE;
        }

    }
    else if(g_strCommPara.uwDataRecvNum < g_strCommPara.uwDataRecvLen)
    {
        g_uwSciaRevData[g_strCommPara.uwDataRecvNum++] = SciaRegs.SCIRXBUF.all & 0xFF;
    }

    if(g_strCommPara.uwDataRecvNum >= g_strCommPara.uwDataRecvLen)
    {
        g_strCommPara.uwDataRecvFlag = TRUE;
    }

    //SciaRegs.SCIFFRX.bit.RXFFOVRCLR=1;   // Clear Overflow flag
    //SciaRegs.SCIFFRX.bit.RXFFINTCLR=1;   // Clear Interrupt flag
}
//******************************************************************************
void SCIA_TXINT_RealTimeDone(void)
{
    if(g_strCommPara.uwDataSendNum < g_strCommPara.uwDataSendLen)
    {
        SciaRegs.SCICTL2.bit.TXINTENA = 1;
        SciaRegs.SCITXBUF = g_uwSciaSendData[g_strCommPara.uwDataSendNum++] & 0xFF;
    }
    else
    {
        SciaRegs.SCICTL2.bit.TXINTENA = 0;
        g_strCommPara.uwDataSendNum = 0;
    }

    //SciaRegs.SCIFFTX.bit.TXFIFOXRESET=1;   // Clear Overflow flag
    //SciaRegs.SCIFFTX.bit.TXFFINTCLR=1;   // Clear Interrupt flag
}
#endif
