/*
 * DCDC_Scia.h
 *
 *  Created on: 2025年6月13日
 *      Author: ChenHF
 */

#ifndef DCDC_SCIA_H_
#define DCDC_SCIA_H_



typedef enum
{
    eModBus_Init,
    eGetRunningData = 0x03,
    eSendSingleSetting = 0x06,
    eSendMultiSetting = 0x10,
    eUpgrade = 0x50,
    eModBus_End

}Protrotol;

typedef enum
{
    eRunningData_BEGIN,
	eBackData_dVbusRealVal = eRunningData_BEGIN,
	eBackData_dVbatRelayRealVal,
	eBackData_dVportRealVal,
	eBackData_dVbatRealVal, 
	eBackData_dIbuckboostRealVal, // 
	eBackData_dIbatRealVal,
	eBackData_dPbatRealVal, // 6
	
	eBackData_eVbus_dRmsVal,
	eBackData_eVbat_Relay_dRmsVal,
	eBackData_eVport_dRmsVal, 
	eBackData_eVbat_dRmsVal,	
	eBackData_eIbuckboost_dRmsVal,	
	eBackData_eIbat_dRmsVal, // 12
	
	eBackData_eVbus_dRmsValBitFilter,
	eBackData_eVbat_Relay_dRmsValBitFilter,
	eBackData_eVport_dRmsValBitFilter, 
	eBackData_eVbat_dRmsValBitFilter,	
	eBackData_eIbuckboost_dRmsValBitFilter,	
	eBackData_eIbat_dRmsValBitFilter, // 18	
	
    eRunningData_VBusRef,     // 设置
    eRunningData_VBatChrgRef,
    eRunningData_VBatDisChrgRef,
    eRunningData_IbatRef,  
    eRunningData_Pbat,  // 23
    
    eRunningData_Ploadlimit, 
    eRunningData_BuckPWM,
    eRunningData_BoostPWM,
    eRunningData_SystemStatus,   
    eRunningData_VersionData,  // 28
    
    eRunningData_FaultCode, 
    eRunningData_FaultRecoverTime, // 30
    
    eRunningData_Rsvd1,
    eRunningData_Rsvd2, 
    eRunningData_Rsvd3, 
    eRunningData_Rsvd4,
    eRunningData_Rsvd5,
    eRunningData_Rsvd6,
    eRunningData_Rsvd7,
    eRunningData_Rsvd8,
    eRunningData_Rsvd9,
    eRunningData_Rsvd10, // 40
    eRunningData_END

}ModBus_RunningData;

typedef enum
{
    eSettingData_BEGIN,
    eSettingData_ManualSwitch = eSettingData_BEGIN,
    eSettingData_BatWorkMode,
    eSettingData_DeviceMode,
    eSettingData_PowerRateSet,
    eSettingData_SystemConnect,
    eSettingData_VbusRef,      // 5
    
    eSettingData_VbusLoopKp,  // 6
    eSettingData_VbusLoopKi,  // 7
    eSettingData_VbatLoopKp,  // 8
    eSettingData_VbatLoopKi,  // 9
    eSettingData_IbatLoopKp,  // 10
    eSettingData_IbatLoopKi,  // 11
    
    eSettingData_VbatChrgRef, // 12  充电电压
    eSettingData_IbatChrgRef, // 13  充电电流
    eSettingData_VbatDisChrgRef, // 14 放电电压
    eSettingData_IbatDisChrgRef, // 15 放电电流
    eSettingData_DebugPara1, // 16 开环充电占空比
    eSettingData_DebugPara2, // 17 开环放电占空比
    
    eSettingData_DebugPara3,
    eSettingData_DebugPara4,
    eSettingData_DebugPara5,
    eSettingData_DebugPara6,
    eSettingData_Rsvd1,
    eSettingData_Rsvd2,
    eSettingData_Rsvd3,
    eSettingData_Rsvd4,
    eSettingData_Rsvd5,
    eSettingData_Rsvd6, //27
//	eSettingData_Rsvd7, //28
//	eSettingData_Rsvd8, //29
//	eSettingData_Rsvd9, //30
//	eBackData_dVbusRealVal, 
//	eBackData_dVbatRelayRealVal,
//	eBackData_dVportRealVal,
//	eBackData_dVbatRealVal,	
//	eBackData_dIbuckboostRealVal, // 35
//	eBackData_dIbatRealVal,
//	eBackData_dPbatRealVal, // 37
//	eBackData_eVbus_dRmsVal,
//	eBackData_eVbat_Relay_dRmsVal,
//	eBackData_eVport_dRmsVal, // 40	
//	eBackData_eVbat_dRmsVal,	
//	eBackData_eIbuckboost_dRmsVal,	
//// 	eBackData_eIbat_dRmsVal, // 43	
// 	eBackData_Rsvd1,
// 	eBackData_Rsvd2, // 45
// 	eBackData_Rsvd3,
// 	eBackData_Rsvd4,
// 	eBackData_Rsvd5,
// 	eBackData_Rsvd6,
// 	eBackData_Rsvd7, // 50
    eSettingData_END

}ModBus_SettingData;

typedef enum
{
    eCrcWrong,
    eRunningDataWrong,
    eSettingDataWrong
}Fault_Code;
typedef struct
{
    Uint16 uwFuncCode;

    Uint16 uwDataRecvOverCnt;
    Uint16 uwDataRecvOverTime;
    Uint16 uwDataSendOverCnt;
    Uint16 uwDataSendOverTime;

    Uint16 uwDataRecvFlag;
    Uint16 uwDataRecvNum;
    Uint16 uwDataRecvLen;

    Uint16 uwStartSendDataFlag;
    Uint16 uwDataSendNum;
    Uint16 uwDataSendLen;
}Struct_CommPara;
extern Struct_CommPara g_strCommPara;
extern Uint16 g_uwRunningData_ModBus[eRunningData_END];
extern Uint16 g_uwSettingData_ModBus[eSettingData_END];
extern int16 dDataMonitor[6];
//-----------------------------------------------------------------------------
// Public Function Prototypes
//-----------------------------------------------------------------------------
extern void SCIA_System_PinInit(void);
extern void SCIA_System_PeripheralInit(void);
extern void SCIA_System_AppInit(void);
extern void SCIA_RXINT_RealTimeDone(void);
extern void SCIA_TXINT_RealTimeDone(void);
extern void SCIA_Task_1ms(void);
extern void SCIA_Task_5ms(void);
extern void SCIA_Task_20ms(void);
extern void SCIA_Task_100ms(void);
extern void SCIA_Task_1s(void);


#endif /* DCDC_SCIA_H_ */
