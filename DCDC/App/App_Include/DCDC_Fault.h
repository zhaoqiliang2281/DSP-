/*
 * DCDC_Fault.h
 *
 *  Created on: 2025年6月21日
 *      Author: phone
 */

#ifndef DCDC_FAULT_H_
#define DCDC_FAULT_H_
//
// Adding Constants and Macro Definitions
//

// 上报错误
#define ERR 1
#define OK  0
typedef union
{
    struct
    {
        Uint16 bBusOvVoltAlarm         :1;  // 母线过压，0：否，1：是
        Uint16 bBusLowVoltAlarm        :1;  // 母线欠压，0：否，1：是
        Uint16 bBatRelayOpenFault      :1;  // 电池继电器开路故障，0：否，1：是
        Uint16 bBattOvVoltAlarm        :1;  // 电池过压，0：否，1：是
        Uint16 bBattLowVoltAlarm       :1;  // 电池欠压，0：否，1：是
        Uint16 bBattOvCurrAlarm        :1;  // 电池过流，0：否，1：是
        Uint16 bBattLostAlarm          :1;  // 电池未接入，0：否，1：是
        Uint16 bIGBTtempover           :1;  // 模块过热，0：否，1：是
        Uint16 bbussoftfail            :1;  // BUS软启动失败，0：否，1：是
        Uint16 bmodparacommfail        :1;  // 模块并机通讯故障，0：否，1：是
        Uint16 bneigdcdcfaultflag      :1;  // 邻机故障，0：否，1：是
        Uint16 bPowerFault             :1;  // 辅助电源故障，0：否，1：是
        Uint16 bFanFault               :1;  // 风扇故障，0：否，1：是
        Uint16 bBusRelayShortFault     :1;  // BUS继电器短路故障，0：否，1：是
        Uint16 bBusRelayOpenFault      :1;  // BUS继电器开路故障，0：否，1：是
        Uint16 bBatRelayShortFault     :1;  // 电池继电器短路故障，0：否，1：是
        Uint16 bcommfail               :1;  // 系统通讯故障，0：否，1：是
        Uint16 bstartfail              :1;  // 启动失败，0：否，1：是
        Uint16 bInitSettingFail        :1;  // 模块初始化失败，0：否，1：是
        Uint16 bEPO                    :1;  // 紧急停机，0：否，1：是
        Uint16 bmodleconfig            :1;  // 模块配置错误，0：否，1：是
        Uint16 bBusCurrentUpRange      :1;  // 母线过流，0：否，1：是
        Uint16 bBattCurrentLineInv     :1;  // 电流线反接，0：否，1：是
        Uint16 bBattCurrentLineOpen    :1;  // 电流线断路，0：否，1：是
        Uint16  bPhaseCurrentUnbalance :1;  // 模块三相电流不平衡   0：否 ， 1：是
        Uint16  bUserDefWorkModeFail   :1;  // 自定义工作模式配置失败 0：否，1：是
        Uint16 bBattVoltageLineInv     :1;  // 电压线反接
        Uint16 bInputOvVoltAlarm       :1;  // 输入端口过压
        Uint16 bInputLowVoltAlarm      :1;  // 输入端口欠压
        Uint16 bChError                :1;  // 通道异常
        Uint16 bIDFault                :1;  // ID拨码错误
        Uint16 bTranTempOver           :1;  // 变压器过温
        Uint16 bBatCellOver            :1;  // 单体过压
        Uint16 bBatCellunder           :1;  // 单体欠压
        Uint16 bBatOverTemp            :1;  // 过温保护
        Uint16 bBatLowTemp             :1;  // 低温保护
        Uint16 bBattVoltageLineOpen    :1;  // 电压线断路
        Uint16 bBootLoaderError        :1;  // 引导程序未烧录
        Uint16 bBattVoltTrendError     :1;  // 电池电压趋势异常
        Uint16 bNegCurrentError        :1;  // 负电流异常

        Uint16 bRsvd0                  :8;
        Uint16 bRsvd1                  :8;
        Uint16 bRsvd2                  :8;
    }bit;
   struct
    {
        Uint16 byte0                   :8;
        Uint16 byte1                   :8;
        Uint16 byte2                   :8;
        Uint16 byte3                   :8;
        Uint16 byte4                   :8;
        Uint16 byte5                   :8;
        Uint16 byte6                   :8;
        Uint16 byte7                   :8;
    }byte;
}Union_Alarm;
extern Union_Alarm upAlarm;


enum RecoverType
{
    eAutoRecovery,
    eNeverRecovery
};
///
enum FaultStatus
{
    eNormal,
    eFaulted,
    eWarning
};
///
enum FaultType
{
    eINT,
    e1ms,
    e5ms,
    e20ms,
    e100ms,
};
///
enum Fault_ID
{
    //中断型故障
    eFault_ID_Begin = 0,
    eINTFault_ID_Begin = eFault_ID_Begin,
    eVbat_ShortCircuit_ID = eINTFault_ID_Begin,
    eVbus_ShortCircuit_ID,
    eIinstLimit_ID,
    eVbat_OV_ID,
    eVbus_OV_ID,
    eIbat_OC_ID,
    eINTFault_ID_End,

    //20ms型故障
    e20msFault_ID_Begin = eINTFault_ID_End,
    eRelayBreak_ID = e20msFault_ID_Begin,
    eBusBuildFail_ID,
    eVbat_OV1_ID,
    eVbat_UV1_ID,
    eVbus_OV1_ID,
    eVbus_UV1_ID,
    eIbat_OC1_ID,
    eIbat_OC2_ID,
    eNoBat_ID,
    e20msFault_ID_End,
    eFault_ID_End = e20msFault_ID_End

};
///
#define FAULTCHECK_EN          (0x0001)
#define FAULTRECOVER_EN        (0x0040)

#define LT_CHECKFLAG           (0x0004)
#define HT_CHECKFLAG           (0x0008)
#define EQ_CHECKFLAG           (0x0010)
///
#define LT_CHECKFLAG_START     (0x0003)//针对CheckFlag
#define HT_CHECKFLAG_START     (0x0005)
#define EQ_CHECKFLAG_START     (0x0009)

#define LT_FAULTFLAG           (0x0009)//针对CheckFlag高四位
#define HT_FAULTFLAG           (0x000A)
#define EQ_FAULTFLAG           (0x000C)
///
typedef union
{
    Uint16 all;
    struct
    {
        Uint16 bVbat_ShortCircuit       :1;
        Uint16 bVbus_ShortCircuit       :1;
        Uint16 bVbat_OV                 :1;
        Uint16 bVbus_OV                 :1;

        Uint16 bIinstLimit              :1; //逐波限流
        Uint16 bIbat_OC                 :1;
        Uint16 bRsvd                    :10;
    }bit;
}Union_INTFAULT;

typedef union
{
    Uint16 all;
    struct
    {
        Uint16 bVbat_OV1                  :1;
        Uint16 bVbat_UV1                  :1;
        Uint16 bVbus_OV1                  :1;
        Uint16 bVbus_UV1                  :1;

        Uint16 bIbat_OC1                 :1;
        Uint16 bIbat_OC2                 :1;
        Uint16 bNoBat                    :1;
        Uint16 bRelayBreak               :1;

        Uint16 bBusBuildFail             :1;
        Uint16 bRsvd                     :7;
    }bit;
}Union_20msFAULT;

typedef union
{
    Uint16 all;

    struct
    {
        Uint16 bCheckEn                  :1;    //检测使能

        Uint16 CheckFlag                 :5;

        Uint16 RecoverFlag               :3;

        Uint16 bFaultFlag                :4;

        Uint16 b3Rsvd                    :3;    //预留
    }tag;

    struct
    {
        Uint16 bCheckEn                  :1;    //检测使能

        Uint16 bChkStartFlag             :1;    //检测开始使能
        Uint16 bLowerThan                :1;    //判断少于等于
        Uint16 bHigherThan               :1;    //判断大于等于
        Uint16 bEqualTo                  :1;    //判断标志是否为TRUE
        Uint16 bFaulted                  :1;    //是否触发故障

        Uint16 bRecoverEn                :1;    //故障恢复使能
        Uint16 bRecoverType              :2;    //恢复类型


        Uint16 bFaultType                :2;    //故障类型
        Uint16 bFaultStatus              :2;

        Uint16 b3Rsvd                    :3;    //预留
    }bit;
}Union_ControlFlag;

typedef struct
{
    Union_INTFAULT unINTFault;
    Union_20msFAULT un20msFault;

    Uint16 uwSysFaultFlag;
    Uint16 uwMaxCountDownTime;
    Uint16 uwErrorCode;
    Uint16 uwWarningCode;

}Struct_SysFaultReg;
extern Struct_SysFaultReg g_strSysFaultReg;
typedef struct
{
    int32 dInputVal;      //fInputVal

    Uint16 uwCheckCnt;
    Uint16 uwCheckTime;
    Uint16 uwFaultTimes;
    int32 dChkProtectLimit;   //fCheckProtectLimit;

    Uint16 uwRecoverCnt;
    Uint16 uwRecoverTime;
    Uint16 uwRecoverTimes;
    Uint16 uwCountDownTime;
    int32 dChkRecoverLmit;    //fChkRecoverLmit;

    Union_ControlFlag unCtrlFlag;
}Struct_FaultPara;
extern Struct_FaultPara g_strFaultSettingData[eFault_ID_End];
//-----------------------------------------------------------------------------
// Public Function Prototypes
//-----------------------------------------------------------------------------
extern void Fault_System_AppInit(void);
extern void Fault_Task2_RealTimeDone(void);
extern void Fault_Task_1ms(void);
extern void Fault_Task_5ms(void);
extern void Fault_Task_20ms(void);
extern void Fault_Task_1s(void);

#endif /* DCDC_FAULT_H_ */
