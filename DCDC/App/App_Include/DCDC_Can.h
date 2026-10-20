/*
 * DCDC_Relay.h
 *
 *  Created on: 2025年9月10日
 *      Author: phone
 */

#ifndef DCDC_Can_H_
#define DCDC_Can_H_


#define SM_MAX_V  100000

//#define CAN_BAUD_125K
//#define CAN_BAUD_250K
//#define CAN_BAUD_500K
#define CAN_BAUD_1M

#define CAN_SEND_BUF_LEN 20

// 定义状态枚举
enum DCDC_WORD_MODE
{
    WORK_MODE_IDLE=0X00, // 待机
    WORK_MODE_CV=0X01,   // 恒压（正值充电，负值放电）
    WORK_MODE_CC=0X02,   // 恒流（正值充电，负值放电）
    WORK_MODE_CP=0X03,   // 恒功率（正值充电，负值放电）
    WORK_MODE_CR=0X04,   // 恒阻（正值充电，负值放电）
    WORK_MODE_SPACE=0X05, // 搁置
    WORK_MODE_CCCV=0X06,  // 恒流恒压（恒压值，正值充电，负值放电）
    WORK_MODE_WCSIM=0x0A,  // 工况模拟  Working condition simulation
    WORK_MODE_ERR=0x0F  // 0x0F:故障
};

// 定义状态枚举
enum BAT_WORK_MODE
{
    MODE_STOP=0X00, // 0停机
    MODE_CHARGE=0X01,   // 1充电
    MODE_DISCHARGE=0X02,  // 放电
    MODE_SPACE =0X03      // 搁置
};


struct  BYTES2 {      // bits   description
   Uint8      BYTE0:8;     // 7:0
   Uint8      BYTE1:8;     // 15:8
};

struct  BYTES_4 {      // bits   description
   Uint8      BYTE0:8;     // 7:0
   Uint8      BYTE1:8;     // 15:8
   Uint8      BYTE2:8;     // 23:16
   Uint8      BYTE3:8;     // 31:24
};


struct  BYTES_8 {      // bits   description
   Uint8      BYTE0:8;     // 7:0
   Uint8      BYTE1:8;     // 15:8
   Uint8      BYTE2:8;     // 23:16
   Uint8      BYTE3:8;     // 31:24
   Uint8      BYTE4:8;
   Uint8      BYTE5:8;
   Uint8      BYTE6:8;
   Uint8      BYTE7:8;
};


typedef union ua16
{
   Uint16             all;
   struct BYTES2   byte;
}myUInt16;

typedef union a32
{
   int32             all;
   struct BYTES_4   byte;
}myInt32;

typedef union ua32
{
   Uint32             all;
   struct BYTES_4   byte;
}myUInt32;

typedef union a64
{
   Uint64             all;
   struct BYTES_8   byte;
}myUInt64;

typedef struct
{
    Uint8  swValue;  // 设备的接收ID  0x00004100
    Uint32 swRevID;  // 设备的接收ID  0x00004100
    Uint32 swSendID;  // 设备的接收ID 0x00000041

    Uint8 batWorkMode; // 工作模式，0停机，1充电，2放电
    Uint8 workMode; // 工作模式，02恒流 06恒流恒压，00待机，05搁置
    Uint8 workModeTmp; // 工作模式，02恒流 06恒流恒压，00待机，05搁置

    Uint8  getBufRefFlag;  // 读取母线电压标志
    Uint32 bufRef;  // 母线电压，暂时自适应，上电后初始化时读取母线电压，放电时用

    myInt32 batRef;
    myInt32 curRef;
    myInt32 tmpInt32;
    myInt32 tmpCmd0705;
    Uint8   tmpCmd0706_6;
    Uint8   tmpCmd0706_7;

    Uint8   tmpRev0490;
    Uint8   tmpRev0490_Count;

    Uint32 RevID;
    Uint8  RxBuf[8];
    Uint8  RxDatFlag;

    Uint32 heartCount;    // 心跳计数
    Uint32 hearTimeCount; // 心跳超时计数
    Uint8  getHeartFlag;  // 收到心跳帧标志，收到心跳帧之后才发送1030帧
    Uint16 enSendFlag;    // 允许主动发送1030帧
    Uint16 enSendCount;


    Uint8  errByte;       // 报警字节，与电压电流一起上传

    Uint8 back1032Byte; // 0x1032FF41 返回的第一个字节
    Uint8 back1032Byte_P_N_Flag; // 0x1032FF41 返回的第一个字节

    myUInt32  chargeCapacity;      // 08 11 返回充电容量
    myUInt32  dischargeCapacity;   // 08 11 返回放电容量

    myUInt64 u64CountChargeCapacity;
    myUInt64 u64CountDisChargeCapacity;

    Uint32  backRunCountSpace;
    Uint8  back1031LastByte;

    Uint16  tack1sCount;
    Uint16  tack5msCount;
    Uint16  tack5msCount_500ms;
    Uint16  tack5msCount_55ms;

//    Uint16  canRevIntCount;  // can中断计数
//    Uint16  canRevIntErrCount;
//    Uint16  canErrIntCount;
//    Uint16  canREVCount;
//    Uint16  canSendCount;
    Uint16  CANTEC;
    Uint16  CANREC;

	Uint16  stRunFlag;  // 开始运行
	Uint16  preChargeFlag; // 预充标志

    Uint16  runReadyFlag; // 运行准备完成标志

    Uint8  runSerialNo;  // 运行序列号，不运行时为0，运行时为0x40 -> 0X80 - >0XC0 ->0X40
    Uint8  getConfigFlag; // 中位机发配置标志

    Uint8  needReflashCalPraFlag;


    Uint32 canSendBuf_ExtID[CAN_SEND_BUF_LEN];
    Uint8  canSendBuf[CAN_SEND_BUF_LEN][8];
    Uint8  canSendBuf_SaveP; // 保存位置
    Uint8  canSendBuf_SendP; // 保存位置

}canSetPraStr;
extern canSetPraStr canSetPra;



#define CANSENDNOWAIT   //CAN 发送不等待发送完毕 TA 标志 (默认测试程序选用)
//#define CANNOWWAIT     //CAN 发送等待当前邮箱发送完毕 TA 标志
//#define CANLASTWAIT	// CAN 发送等待上一个ID发送完毕 TA 标志，推荐方式

#define CANRxLEN 20

typedef struct CAN_Rev{
	union
	  {
	    Uint16 All;
	    struct{
	            Uint16  RevFlag                  :1;  //接收到数据标志
	            Uint16  HWOVFlag                 :1;  //DSP硬件接收数据溢出标志
	            Uint16  rFifoDataflag            :1;  //接收内存非空
	            Uint16  rFifoFullFlag             :1;  //接收内存溢出

	    		}Status_Bits;
	  	}Mark_Para;
	Uint16 rxData[CANRxLEN][4];				     //接收缓存, [0]放较低位数据
	Uint16 rxReadIndex;                         //接收FIFO写入索引
	Uint16 rxWriteIndex;                        //接收FIFO读出索引
}RCAN_Msg;

extern RCAN_Msg CANRxMsg[];
extern RCAN_Msg *PtrCanRx;



typedef struct CAN_Tx{
	Uint16 DA;
	Uint16 DB;
	Uint16 DC;
	Uint16 DD;

	Uint16 MBXnbr;                      //指定发送的邮箱
	Uint16 RevMBXnID ;                  //指定发送的ID号,取值范围 0x00 到 0x3FF , 11bit 标准帧ID
}TCAN_Msg;

extern TCAN_Msg  CANTxMsg[];
extern TCAN_Msg *PtrCanTx;


void CAN_Init_Interrupt(void);
void canMailboxInit(void);
void InitECana_1(void);

void CANInit(void);
void CAN_Send(Uint32 ExtID,Uint8* dat);
void handleCANRxFIFO(void);

Uint8 CAN_TestUpVI(void);

Uint8 CAN_TestUpErr(void);

Uint8 CAN_RX_Fun(Uint32 ExtId,Uint8 *dat);

void heartTimeOutFun(void);

void canRevInit(void);

void CAN_Send_Inbuff(Uint32 ExtID,Uint8* dat);

void canTask100us(void);
void canTask5ms(void);
void canTask1s(void);
void can5msTo1s(void);
Uint8 canTask3S(void);
Uint8 canTask12S(void);

void FilterCanReceiveID(void);

void StopTheSM(void);
void CheckStopLogic(void);

void canTask55ms(void);




#endif /* DCDC_Can_H_ */
