/*
 * DCDC_Can.c
 *
 *  Created on: 2025年9月16日
 *      Author:
 */


#include "DCDC_GlobalHeadFile.h"

interrupt void canA_isr0(void);
interrupt void canA_isr1(void);


canSetPraStr canSetPra;


/*
typedef struct CAN_Rev{
	union
	  {
	    Uint16 All;
	    struct{
	            Uint16  RevFlag                  :1;  //接收到数据标志
	            Uint16  HWOVFlag                 :1;  //DSP硬件接收数据溢出标志
	            Uint16  rFifoDataflag            :1;  //接收内存非空
	            Uint16  rFifoOVFlag              :1;  //接收内存溢出

	    		}Status_Bits;
	  	}Mark_Para;
	Uint16 rxData[CANRxLEN][4];				     //接收缓存
	Uint16 rxReadIndex;                         //接收FIFO写入索引
	Uint16 rxWriteIndex;                        //接收FIFO读出索引
}RCAN_Msg;
*/
RCAN_Msg CANRxMsg[]={
		{0, {0},0,0}
};

RCAN_Msg *PtrCanRx = &CANRxMsg[0];


Uint32 lastSendMBox =0; //上一次发送的邮箱号码

TCAN_Msg  CANTxMsg[]={
		{0,0,0,0, 1,0xA0}
};

TCAN_Msg *PtrCanTx = &CANTxMsg[0];


//#################################################
//CAN的GPIO初始化
//这里采用GPIO30 GPIO31
//-----------------------------------------------
void Init_CANA_Gpio(void)
{
   EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO30 = 0;     // Enable pull-up for GPIO30 (CANRXA)
    GpioCtrlRegs.GPAPUD.bit.GPIO31 = 0;     // Enable pull-up for GPIO31 (CANTXA)

/* Set qualification for selected CAN pins to asynch only */
// Inputs are synchronized to SYSCLKOUT by default.
// This will select asynch (no qualification) for the selected pins.
    GpioCtrlRegs.GPAQSEL2.bit.GPIO30 = 3;   // Asynch qual for GPIO30 (CANRXA)

    //IO30 IO21配置为CAN外设引脚
    GpioCtrlRegs.GPAMUX2.bit.GPIO30 = 1;    // Configure GPIO30 for CANRXA operation
    GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 1;    // Configure GPIO31 for CANTXA operation

    EDIS;
}


//#################################################
//CAN初始化函数
//-----------------------------------------------
void CANInit()
{
	Init_CANA_Gpio(); //官方库函数
	InitECana_1();	   //配置CAN 波特率等
	canMailboxInit();  //配置发送、接收邮箱（MBOX16 ID= 0xA0）、ID
	CAN_Init_Interrupt();//配置CAN 接收中断
}



//#################################################
//复位邮箱、配置波特率等操作
//const Uint16 CANBrtTAB[]=
//{
// //BRPREG  TSEG2   TSEG1
//   199,    2,    10,    // 0-10K
//   99,        2,    10,    // 1-20K
//   39,        2,    10,    // 2-50K
//   19,        2,    10,    // 3-100K
//   15,        2,    10,    // 4-125K
//   11,        1,    6    ,    // 5-250K
//   5,        1,    6    ,    // 6-500K
//   2,        1,    6    ,    // 7-1M
//};
//-----------------------------------------------
void InitECana_1(void)        // Initialize eCAN-A module
{

/* Create a shadow register structure for the CAN control registers. This is
 needed, since only 32-bit access is allowed to these registers. 16-bit access
 to these registers could potentially corrupt the register contents or return
 false data. */

struct ECAN_REGS ECanaShadow;

    EALLOW;     // EALLOW enables access to protected bits

/* Configure eCAN RX and TX pins for CAN operation using eCAN regs*/

    ECanaShadow.CANTIOC.all = ECanaRegs.CANTIOC.all;
    ECanaShadow.CANTIOC.bit.TXFUNC = 1;
    ECanaRegs.CANTIOC.all = ECanaShadow.CANTIOC.all;

    ECanaShadow.CANRIOC.all = ECanaRegs.CANRIOC.all;
    ECanaShadow.CANRIOC.bit.RXFUNC = 1;
    ECanaRegs.CANRIOC.all = ECanaShadow.CANRIOC.all;

/* Configure eCAN for HECC mode - (reqd to access mailboxes 16 thru 31) */
                                    // HECC mode also enables time-stamping feature

    ECanaShadow.CANMC.all = ECanaRegs.CANMC.all;
    ECanaShadow.CANMC.bit.SCB = 1;               //P35  Select eCAN mode.
    ECanaRegs.CANMC.all = ECanaShadow.CANMC.all;

/* Initialize all bits of 'Message Control Register' to zero */
// Some bits of MSGCTRL register come up in an unknown state. For proper operation,
// all bits (including reserved bits) of MSGCTRL must be initialized to zero

    ECanaMboxes.MBOX0.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX1.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX2.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX3.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX4.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX5.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX6.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX7.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX8.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX9.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX10.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX11.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX12.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX13.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX14.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX15.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX16.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX17.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX18.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX19.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX20.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX21.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX22.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX23.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX24.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX25.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX26.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX27.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX28.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX29.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX30.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX31.MSGCTRL.all = 0x00000000;

// TAn, RMPn, GIFn bits are all zero upon reset and are cleared again
//  as a matter of precaution.
    ECanaRegs.CANTA.all = 0xFFFFFFFF;   /* Clear all TAn bits */
    ECanaRegs.CANRMP.all = 0xFFFFFFFF;  /* Clear all RMPn bits */
    ECanaRegs.CANGIF0.all = 0xFFFFFFFF; /* Clear all interrupt flag bits */
    ECanaRegs.CANGIF1.all = 0xFFFFFFFF;

/* Configure bit timing parameters for eCANA*/
    ECanaShadow.CANMC.all = ECanaRegs.CANMC.all;
    ECanaShadow.CANMC.bit.CCR = 1 ;            // Set CCR = 1
    ECanaRegs.CANMC.all = ECanaShadow.CANMC.all;

    // Wait until the CPU has been granted permission to change the configuration registers
    do
    {
      ECanaShadow.CANES.all = ECanaRegs.CANES.all;
    } while(ECanaShadow.CANES.bit.CCE != 1 );       // Wait for CCE bit to be set..

    ECanaShadow.CANBTC.all = 0;
    /* The following block is only for 60 MHz SYSCLKOUT. (30 MHz CAN module clock Bit rate = 1 Mbps
       See Note at end of file. */
/*  //TI 官方的
    ECanaShadow.CANBTC.bit.BRPREG = 2;
    ECanaShadow.CANBTC.bit.TSEG2REG = 1;
    ECanaShadow.CANBTC.bit.TSEG1REG = 6;

    ECanaShadow.CANBTC.bit.SAM = 1;
    ECanaRegs.CANBTC.all = ECanaShadow.CANBTC.all;
*/
    //-------------------------自定义的配置------------------------------------------
    //P47
    //rate Bit =(SYSCLKOUT/2)/(BRP * Bit-time )
    //BRP = BRPreg+1
    //Bit-time = (TSEG1reg + 1) + (TSEG2reg+ 1) + 1
    //配置TQ 和BRP ，
    //方案一 TQ为10， 则TSEG1REG = 6， BRP = 5 ，为500K
    //方案二，TQ为12，则TSEG1REG = 8， BRP = 4 ，为500K
    //500KHz

//    ECanaShadow.CANBTC.bit.BRPREG = 2; //  2 1 6:1M    5 1 6:500K   11,1,6, -250K  15,2,10, 4-125K
//    ECanaShadow.CANBTC.bit.TSEG2REG = 1;
//    ECanaShadow.CANBTC.bit.TSEG1REG = 6;

    #ifdef CAN_BAUD_125K
        ECanaShadow.CANBTC.bit.BRPREG = 15; //  2 1 6:1M    5 1 6:500K   11,1,6, -250K  15,2,10, 4-125K
        ECanaShadow.CANBTC.bit.TSEG2REG = 2;
        ECanaShadow.CANBTC.bit.TSEG1REG = 10;
    #endif

    #ifdef CAN_BAUD_250K
        ECanaShadow.CANBTC.bit.BRPREG = 11; //  2 1 6:1M    5 1 6:500K   11,1,6, -250K  15,2,10, 4-125K
        ECanaShadow.CANBTC.bit.TSEG2REG = 1;
        ECanaShadow.CANBTC.bit.TSEG1REG = 6;
    #endif

    #ifdef CAN_BAUD_500K
        ECanaShadow.CANBTC.bit.BRPREG = 5; //  2 1 6:1M    5 1 6:500K   11,1,6, -250K  15,2,10, 4-125K
        ECanaShadow.CANBTC.bit.TSEG2REG = 1;
        ECanaShadow.CANBTC.bit.TSEG1REG = 6;
    #endif

    #ifdef CAN_BAUD_1M
        ECanaShadow.CANBTC.bit.BRPREG = 2; //  2 1 6:1M    5 1 6:500K   11,1,6, -250K  15,2,10, 4-125K
        ECanaShadow.CANBTC.bit.TSEG2REG = 1;
        ECanaShadow.CANBTC.bit.TSEG1REG = 6;
    #endif

	ECanaShadow.CANBTC.bit.SAM = 1;
	ECanaRegs.CANBTC.all = ECanaShadow.CANBTC.all;

    ECanaShadow.CANMC.all = ECanaRegs.CANMC.all;
    ECanaShadow.CANMC.bit.CCR = 0 ;            // Set CCR = 0
    ECanaRegs.CANMC.all = ECanaShadow.CANMC.all;

    // Wait until the CPU no longer has permission to change the configuration registers
    do
    {
      ECanaShadow.CANES.all = ECanaRegs.CANES.all;
    } while(ECanaShadow.CANES.bit.CCE != 0 );       // Wait for CCE bit to be  cleared..

/* Disable all Mailboxes  */
    ECanaRegs.CANME.all = 0;        // Required before writing the MSGIDs

    EDIS;
}


//#################################################
//配置发送、接收邮箱等操作
//数据区8个长度字节
//发送邮箱   1到6
//接收邮箱   16， ID为0x0A
//-----------------------------------------------
void canMailboxInit(void)
{
   struct ECAN_REGS ECanaShadow;

   EALLOW;
    ECanaShadow.CANGAM.all = ECanaRegs.CANGAM.all;
	ECanaShadow.CANGAM.bit.AMI=1; // Standard and extended frames can be received.
	ECanaRegs.CANGAM.all = ECanaShadow.CANGAM.all;
   EDIS;

//P59
//In standard identifier mode,if the IDE bit(MSGID.31) =0,the message identifier is storedin bits ID.28:18.
//IDE =0:The RECEIVED message had a standard identifier
//Auto answer mode bit. AAM =0 ;  Normal transmit mode
//AME = 1 ;The corresponding acceptance mask is used

//MSGID.31=IDE; MSGID.30=AME; MSGID.29=AAM
	// Mailboxs can be written to 16-bits or 32-bits at a time

   //TX 1
  //    ECanaMboxes.MBOX1.MSGID.bit.AME = 1;
  //  ECanaMboxes.MBOX1.MSGID.bit.AAM = 0;
  //  ECanaMboxes.MBOX1.MSGID.bit.IDE = 0;
  //    ECanaMboxes.MBOX1.MSGID.bit.STDMSGID = 0x010;
      ECanaMboxes.MBOX1.MSGID.all = 0x0810ff41;

   //TX 2
  //    ECanaMboxes.MBOX2.MSGID.bit.AME = 1;
  //  ECanaMboxes.MBOX2.MSGID.bit.AAM = 0;
  //  ECanaMboxes.MBOX2.MSGID.bit.IDE = 0;
  //    ECanaMboxes.MBOX2.MSGID.bit.STDMSGID = 0x012;
      ECanaMboxes.MBOX2.MSGID.all = 0x0810ff41;

   //TX 3
  //    ECanaMboxes.MBOX3.MSGID.bit.AME = 1;
  //  ECanaMboxes.MBOX3.MSGID.bit.AAM = 0;
  //  ECanaMboxes.MBOX3.MSGID.bit.IDE = 0;
  //    ECanaMboxes.MBOX3.MSGID.bit.STDMSGID = 0x013;
      ECanaMboxes.MBOX3.MSGID.all = 0x0810ff41;

   //TX 4
  //    ECanaMboxes.MBOX4.MSGID.bit.AME = 1;
  //  ECanaMboxes.MBOX4.MSGID.bit.AAM = 0;
  //  ECanaMboxes.MBOX4.MSGID.bit.IDE = 0;
  //    ECanaMboxes.MBOX4.MSGID.bit.STDMSGID = 0x014;
      ECanaMboxes.MBOX4.MSGID.all = 0x0810ff41;

   //TX 5
  //    ECanaMboxes.MBOX5.MSGID.bit.AME = 1;
  //  ECanaMboxes.MBOX5.MSGID.bit.AAM = 0;
  //  ECanaMboxes.MBOX5.MSGID.bit.IDE = 0;
  //    ECanaMboxes.MBOX5.MSGID.bit.STDMSGID = 0x015;
      ECanaMboxes.MBOX5.MSGID.all = 0x0810ff41;

   //TX 6
  //    ECanaMboxes.MBOX6.MSGID.bit.AME = 1;
  //  ECanaMboxes.MBOX6.MSGID.bit.AAM = 0;
     ECanaMboxes.MBOX6.MSGID.bit.IDE = 1;
  //    ECanaMboxes.MBOX6.MSGID.bit.STDMSGID = 0x020;
      ECanaMboxes.MBOX6.MSGID.all = 0x0810ff41;

      ECanaMboxes.MBOX7.MSGID.all = 0x0810ff41;
      ECanaMboxes.MBOX8.MSGID.all = 0x0810ff41;
      ECanaMboxes.MBOX9.MSGID.all = 0x0810ff41;
      ECanaMboxes.MBOX10.MSGID.all = 0x0810ff41;
      ECanaMboxes.MBOX11.MSGID.all = 0x0810ff41;
      ECanaMboxes.MBOX12.MSGID.all = 0x0810ff41;
      ECanaMboxes.MBOX13.MSGID.all = 0x0810ff41;
      ECanaMboxes.MBOX14.MSGID.all = 0x0810ff41;
      ECanaMboxes.MBOX15.MSGID.all = 0x0810ff41;

  //RX
  //    ECanaMboxes.MBOX16.MSGID.bit.AME = 1;
  //  ECanaMboxes.MBOX16.MSGID.bit.AAM = 0;
  //  ECanaMboxes.MBOX16.MSGID.bit.IDE = 0;
  //    ECanaMboxes.MBOX16.MSGID.bit.STDMSGID = 0xA0;
      ECanaMboxes.MBOX16.MSGID.all = canSetPra.swRevID;  // 41是本机的ID  0x10204102
      ECanaMboxes.MBOX16.MSGID.bit.AME = 1;

      //P71  P64
      // ECanaLAMRegs.LAM16.bit.LAMI = 1;      // 标准帧与扩展帧都接收
      // ECanaLAMRegs.LAM16.bit.LAM_H= 0xffff;  //   LAM_H:13;     // 16:28  ,只接收符合ID的报文
      // ECanaLAMRegs.LAM16.bit.LAM_H= (0x7ff)<<2;;//   LAM_H:13;     // 16:28,标准的只用设置28:18
      // ECanaLAMRegs.LAM16.bit.LAM_L=0;          //   LAM_L:16;     // 0:15，扩展帧，28~15都要设置
       ECanaLAMRegs.LAM16.all = 0xFFFF00FF|(~canSetPra.swRevID); // 这个要重新赋值，在读到模块ID之后 这里00是，只接收41的帧
//      ECanaLAMRegs.LAM16.all = 0xFFFFFFFF; //

    // Specify that 8 bits will be sent/received //8字节数据
    ECanaMboxes.MBOX1.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX2.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX3.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX4.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX5.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX6.MSGCTRL.bit.DLC = 8;

    ECanaMboxes.MBOX7.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX8.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX9.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX10.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX11.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX12.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX13.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX14.MSGCTRL.bit.DLC = 8;
    ECanaMboxes.MBOX15.MSGCTRL.bit.DLC = 8;

	ECanaMboxes.MBOX16.MSGCTRL.bit.DLC = 8;
////////////////////////////////////////////////////////////////////////////////////
    EALLOW;
    ECanaShadow.CANMC.all = ECanaRegs.CANMC.all;
    ECanaShadow.CANMC.bit.DBO = 0;
    ECanaShadow.CANMC.bit.ABO = 1;   // ABO=1‌（启用自动恢复）：适用于需要快速恢复通信的场景，如实时数据传输
                                     // 在10个模块并CAN偶尔死机，或者H与L短接时，设置此位才能恢复
    ECanaShadow.CANMC.bit.SCB = 1;//Select eCAN mode
    ECanaRegs.CANMC.all = ECanaShadow.CANMC.all;
 // Configure  Mailbox Direction: MBOX1--6 as Tx, MBOX16 as Rx
    ECanaRegs.CANMD.all = 0x00010000;

    // Enable all Mailboxes */
    // Since this write is to the entire register (instead of a bit
    // field) a shadow register is not required.
//    ECanaRegs.CANME.all = 0x0001007E;

    ECanaRegs.CANME.all = 0x0001FFFE;

    ECanaShadow.CANMC.all = ECanaRegs.CANMC.all;
    ECanaShadow.CANMC.bit.STM = 0;    // The module is in normal mode
    ECanaRegs.CANMC.all = ECanaShadow.CANMC.all;
    EDIS;
}



//#################################################
//配置CAN接收中断

//-----------------------------------------------
void CAN_Init_Interrupt(void)
{
/* Create a shadow register structure for the CAN control registers. This is
 needed, since only 32-bit access is allowed to these registers. 16-bit access
 to these registers could potentially corrupt the register contents or return
 false data. */
	struct ECAN_REGS ECanaShadow;

	EALLOW;  // This is needed to write to EALLOW protected registers
	PieVectTable.ECAN0INTA = &canA_isr0;    //------------中断配置步骤-----2
	PieVectTable.ECAN1INTA = &canA_isr1;
	EDIS;    // This is needed to disable write to EALLOW protected registers


    EALLOW;     // EALLOW enables access to protected bits
	ECanaShadow.CANMIL.all = ECanaRegs.CANMIL.all;
    ECanaShadow.CANMIL.all = 0xFFFFFFFF ;            //P76, mailbox interrupts to level 1
    ECanaRegs.CANMIL.all = ECanaShadow.CANMIL.all;


	ECanaShadow.CANMIM.all = ECanaRegs.CANMIM.all;
    ECanaShadow.CANMIM.all =0x00010000 ;            //P48,相应的邮箱中断使能位 Mailbox interrupt is enabled.
    ECanaRegs.CANMIM.all = ECanaShadow.CANMIM.all;

	ECanaShadow.CANGIM.all = ECanaRegs.CANGIM.all;
	ECanaShadow.CANGIM.bit.GIL =0;            //P47, Set CANGIM.GIL = 0;All global interrupts are mapped to the ECAN0INT interrupt line
											  //Globalinterrupt level for the interrupts TCOF,WDIF,WUIF,BOIF,EPIF,RMLIF,AAIFandWLIF.
	ECanaShadow.CANGIM.bit.AAIM=1;
	ECanaShadow.CANGIM.bit.WDIM=1;
	ECanaShadow.CANGIM.bit.WUIM=1;
	ECanaShadow.CANGIM.bit.BOIM=1;
	ECanaShadow.CANGIM.bit.EPIM=1;
	ECanaShadow.CANGIM.bit.WLIM=1;

//------------中断配置步骤-----1
	ECanaShadow.CANGIM.bit.I1EN = 1 ;            //P47, This bit globally enables all interrupts for the ECAN1INT line if the corresponding masks are set.
	ECanaShadow.CANGIM.bit.I0EN = 1 ;
	ECanaRegs.CANGIM.all = ECanaShadow.CANGIM.all;

// TAn, RMPn, GIFn bits are all zero upon reset and are cleared again
//  as a matter of precaution.

    ECanaRegs.CANTA.all = 0xFFFFFFFF;   /* Clear all TAn bits */
    ECanaRegs.CANRMP.all = 0xFFFFFFFF;  /* Clear all RMPn bits */
    ECanaRegs.CANGIF0.all = 0xFFFFFFFF; /* Clear all interrupt flag bits */
    ECanaRegs.CANGIF1.all = 0xFFFFFFFF;

//------------中断配置步骤-----3
// Enable CAN in PIE
//   PieCtrlRegs.PIEIER9.bit.INTx5 = 1;      // Enable INT 9.5 in the PIE
//   PieCtrlRegs.PIEIER9.bit.INTx6 = 1;      // Enable INT 9.6 in the PIE
//
////------------中断配置步骤-----4
//   IER |= M_INT9;                          // Enable CPU Interrupt 9

   EDIS;

}



void CAN_Send(Uint32 ExtID,Uint8* dat)
{
   	volatile struct MBOX *Mailbox;
   	struct ECAN_REGS ECanaShadow;
   	Uint32 CANData,NowMailBox;
   	Uint32 CANTaFlag = 1;
   	static Uint16 MBXnbr=1;

//   	canSetPra.canSendCount++;

    canSetPra.CANTEC = ECanaRegs.CANTEC.bit.TEC;
    canSetPra.CANREC = ECanaRegs.CANREC.bit.REC;

//    if(MBXnbr < 6) // 发送邮箱号为1~6
     if(MBXnbr < 15) // 发送邮箱号为1~6
    {
        MBXnbr++;//改变发送邮箱
//        PtrCanTx->RevMBXnID++;//改变接收邮箱
    }
    else
    {
       MBXnbr=1;
    }

   	NowMailBox = CANTaFlag<<MBXnbr;

#ifdef CANLASTWAIT
	//等待上一个邮箱发送完毕
	while(ECanaRegs.CANTA.all != lastSendMBox);
	//清空邮箱
	ECanaRegs.CANTA.all|= lastSendMBox;
#elif defined CANSENDNOWAIT
	//直接清空上一次的发送标志，忽略是否发送成功，测试用
	//清空邮箱
	ECanaRegs.CANTA.all|= lastSendMBox;

#endif

   	//选定发送邮箱
	if(MBXnbr < 16)
	     Mailbox = &ECanaMboxes.MBOX0 + MBXnbr;
	else
	     Mailbox = &ECanaMboxes.MBOX16 + (MBXnbr-16);

	//指定接收邮箱ID
	//先关闭ME，再配置ID
	//----------------------------配置接收ID开始--------------------------------------
	ECanaShadow.CANME.all = ECanaRegs.CANME.all;
	ECanaShadow.CANME.all &=~ NowMailBox;
	ECanaRegs.CANME.all = ECanaShadow.CANME.all ;

	ECanaShadow.CANTRR.all = ECanaRegs.CANTRR.all;
	ECanaShadow.CANTRR.all |= NowMailBox;
	ECanaRegs.CANTRR.all = ECanaShadow.CANTRR.all ;

//	Mailbox->MSGID.bit.STDMSGID = 0x0810ff41;// CANTx->RevMBXnID;
//	Mailbox->MSGID.all = ExtID;// CANTx->RevMBXnID;
	Mailbox->MSGID.all = ExtID;
	Mailbox->MSGID.bit.IDE = 1;

	ECanaShadow.CANME.all = ECanaRegs.CANME.all;
	ECanaShadow.CANME.all |= NowMailBox;
	ECanaRegs.CANME.all = ECanaShadow.CANME.all ;
	//----------------------------配置接收ID完毕--------------------------------------


	//装入数据
//	CANData = CANTx->DB;
//	CANData<<=16;
//	CANData +=  CANTx->DA;
//	Mailbox->MDL.all = CANData;

	Mailbox->MDL.byte.BYTE0 = *dat++;
    Mailbox->MDL.byte.BYTE1 = *dat++;
    Mailbox->MDL.byte.BYTE2 = *dat++;
    Mailbox->MDL.byte.BYTE3 = *dat++;

    Mailbox->MDH.byte.BYTE4 = *dat++;
    Mailbox->MDH.byte.BYTE5 = *dat++;
    Mailbox->MDH.byte.BYTE6 = *dat++;
    Mailbox->MDH.byte.BYTE7 = *dat++;

//	CANData = CANTx->DD;
//	CANData<<=16;
//	CANData +=  CANTx->DC;
//	Mailbox->MDH.all = CANData;

	//使能发送
	ECanaRegs.CANTRS.all |= (CANTaFlag<<MBXnbr);
//	printf("\r\n%0.8lx %0.8lx %0.8lx",(CANTaFlag<<CANTx->MBXnbr),NowMailBox,ECanaRegs.CANTRS.all);//打印测试

	//记录上次发送的邮箱号
	lastSendMBox = NowMailBox;

#ifdef CANNOWWAIT
	//等待上一个邮箱发送完毕
	while(ECanaRegs.CANTA.all != lastSendMBox);
	//清空邮箱
	ECanaRegs.CANTA.all|= lastSendMBox;
#endif


}


//#################################################
//CAN错误处理函数
//-----------------------------------------------
interrupt void canA_isr0(void) //用于系统中断，故消息部分的函数可去
{
	struct ECAN_REGS ECanaShadow;
	PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;

	//EALLOW;
 	ECanaShadow.CANGIF0.all = ECanaRegs.CANGIF0.all;//CANGIF0
//	ECanaShadow.CANGIF0.all = 0xffffffff;
	//EDIS;

//	canSetPra.canErrIntCount++;

	if((ECanaShadow.CANGIF0.all & 0x00037f00) != 0) //有错误产生中断
	{
		//添加 处理错误代码
		//添加 清除错误产生的标记 P76 Table3-4.eCANInterrupt Assertion/Clearing
		ECanaRegs.CANGIF0.all =	ECanaShadow.CANGIF0.all  & 0x00013700;
		ECanaRegs.CANTOS.all = 0x0000FFFF; //bits17,MTOF 因为低十六位用于发送，所以一次性全清除超时中断
		ECanaRegs.CANAA.all =  0x0000FFFF; //P29 Bit14 AAIF ,If the transmission of the message in mailbox n was   rted, the bit AA[n] is set and the AAIF (GIF.14) bit
		ECanaRegs.CANRMP.all=  0xFFFF0000; // Bit11,RMLIF   ,

	}
	//P45 ;  GMIF0/1 :: Global mailbox interrupt flag
	else if(ECanaShadow.CANGIF0.bit.GMIF0 == 1) //One of the mailboxes transmitted or received a message successfully
	{ 											//查看 MIV0/MIV1(GIF0.4-0/GIF1.4-0) ， 读取相应的mailbox 数据

		if(ECanaShadow.CANRMP.all != 0)//注意是 ECanaShadow，而ECanaRegs.CANRMP已被清除 == 0xFFFF0000
												//本例程的中断只做了mailbox16的接收中断，在16--32的mailbox中，优先级最低，
											  //则认为mailbox16接收到数据了，其他的mailbox也接收到数据了
		{
//		    printf("\r\n canA_isr0，中断！");  // 实测这里的中断不会进

		/*	CAN_REV(16,&REV_Data[0][1],&REV_Data[0][0]);
		    CAN_Reci_Data.DA=(int)(REV_Data[0][0]);
			CAN_Reci_Data.DB=(int)(REV_Data[0][0]>>16);
			CAN_Reci_Data.DC=(int)(REV_Data[0][1]);
			CAN_Reci_Data.DD=(int)(REV_Data[0][1]>>16);	*/
		}
		else
		{
		//EALLOW;
			ECanaShadow.CANRMP.all = ECanaRegs.CANRMP.all;
			ECanaShadow.CANRMP.all &= 0xFFFF0000;
			ECanaRegs.CANRMP.all = ECanaShadow.CANRMP.all;//清除中断标志
		//	EDIS;
		}

	}
}

//#################################################
//CAN中断接收函数
//-----------------------------------------------
interrupt void canA_isr1(void) //用于 Message 中断 ，故系统部分的函数可取消　 经常用的中断
{
	struct ECAN_REGS ECanaShadow;
	Uint32 RxDH,RxDL;
//	Uint32 RevID=0x11223344;
	Uint8  TmpUint8;
	Uint32 rID_Temp;
 	PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
// 	Uint8   RxBuf[8];

//  	canSetPra.canRevIntCount++;

	EALLOW;
    ECanaShadow.CANGIF1.all = ECanaRegs.CANGIF1.all;
//	ECanaShadow.CANGIF1.all = 0xffffffff;
	EDIS;

	if((ECanaShadow.CANGIF1.all & 0x00037f00) != 0) //有错误产生的中断
	{
		//添加处理错误代码
		//添加清除错误产生的标记 P76 Table3-4.eCANInterrupt Assertion/Clearing
		ECanaRegs.CANGIF1.all =	ECanaShadow.CANGIF1.all  & 0x00013700;
		ECanaRegs.CANTOS.all = 0x0000FFFF; //bits17,MTOF 因为低十六位用于发送，所以一次性全清除超时中断
		ECanaRegs.CANAA.all =  0x0000FFFF; //P29 Bit14 AAIF ,If the transmission of the message in mailbox n was aborted, the bit AA[n] is set and the AAIF (GIF.14) bit
		ECanaRegs.CANRMP.all=  0xFFFF0000; //

//		canSetPra.canRevIntErrCount++;

	}

	else if(ECanaShadow.CANGIF1.bit.GMIF1 == 1) //接收或者发送中断			//查看 MIV0/MIV1(GIF0.4-0/GIF1.4-0) ， 读取相应的mailbox 数据
  	{
		ECanaRegs.CANRMP.all = 0Xffff0000;

		rID_Temp = ECanaMboxes.MBOX16.MSGID.all & 0x3fffffff;  // 接收的ID
//		if(((rID_Temp&0x0000ff00)==canSetPra.swRevID)||((rID_Temp&0x0000ffff)==0x0000ff02))
        if(((rID_Temp&0x0000ff00)==canSetPra.swRevID)||(rID_Temp==0x1020ff02))
		{

//            PtrCanRx->Mark_Para.Status_Bits.rFifoDataflag = 1;
//            if((PtrCanRx->rxWriteIndex + 1 ) != PtrCanRx->rxReadIndex )
//            {
                //接收数据
                RxDL = ECanaMboxes.MBOX16.MDL.all;
                RxDL = ECanaMboxes.MBOX16.MDL.byte.BYTE0;
                RxDL = ECanaMboxes.MBOX16.MDL.byte.BYTE1;
                RxDL = ECanaMboxes.MBOX16.MDL.byte.BYTE2;
                RxDL = ECanaMboxes.MBOX16.MDL.byte.BYTE3;

                canSetPra.RxBuf[0] = ECanaMboxes.MBOX16.MDL.byte.BYTE0;
                canSetPra.RxBuf[1] = ECanaMboxes.MBOX16.MDL.byte.BYTE1;
                canSetPra.RxBuf[2] = ECanaMboxes.MBOX16.MDL.byte.BYTE2;
                canSetPra.RxBuf[3] = ECanaMboxes.MBOX16.MDL.byte.BYTE3;
                canSetPra.RxBuf[4] = ECanaMboxes.MBOX16.MDH.byte.BYTE4;
                canSetPra.RxBuf[5] = ECanaMboxes.MBOX16.MDH.byte.BYTE5;
                canSetPra.RxBuf[6] = ECanaMboxes.MBOX16.MDH.byte.BYTE6;
                canSetPra.RxBuf[7] = ECanaMboxes.MBOX16.MDH.byte.BYTE7;

    //			printf("\r\nBYTE0: %x",RxBuf[0]);
    //			printf("\r\nBYTE1: %x",RxBuf[1]);
    //			printf("\r\nBYTE2: %x",RxBuf[2]);
    //			printf("\r\nBYTE3: %x",RxBuf[3]);
    //            printf("\r\nBYTE4: %x",RxBuf[4]);
    //            printf("\r\nBYTE5: %x",RxBuf[5]);
    //            printf("\r\nBYTE6: %x",RxBuf[6]);
    //            printf("\r\nBYTE7: %x",RxBuf[7]);


                PtrCanRx->rxWriteIndex=(++PtrCanRx->rxWriteIndex)%(CANRxLEN);

                canSetPra.RevID = ECanaMboxes.MBOX16.MSGID.all & 0x3fffffff;  // 接收的ID

    //			TmpUint8=(Uint8)((RevID>>24)&0xff);
    //			printf("\r\n CAN 接收到ID:%x",(Uint16)(RevID>>16));
    //    		printf("\r\n CAN 接收到ID:%x",(Uint16)(RevID));  // %02x也会打印出来16位数 如1122 (Uint8)也无用 // 必要&0xff，否则TmpUint8也是16位数

                canSetPra.RxDatFlag = 1;
              //  CAN_RX_Fun(Uint32 revId,Uint8 *dat)
                CAN_RX_Fun(canSetPra.RevID,canSetPra.RxBuf); // 处理接收到的数据

//                canSetPra.canREVCount++;

//            }
//            else//接收缓存满
//            {
//                //用户这里做缓存满的处理,
//                PtrCanRx->Mark_Para.Status_Bits.rFifoFullFlag = 1;
//            }
		}

	 }
}




//#################################################
//CAN接收缓存处理函数
//-----------------------------------------------
void handleCANRxFIFO()
{
//	Uint16 wrIndex =0;
//	if(PtrCanRx->Mark_Para.Status_Bits.rFifoDataflag == 1 )
//	{
//		PtrCanRx->Mark_Para.Status_Bits.rFifoDataflag =0;
//		while(PtrCanRx->rxReadIndex != PtrCanRx->rxWriteIndex)
//		{
//			//这里处理接收到的数据
//			wrIndex = (PtrCanRx->rxWriteIndex!=0)?(PtrCanRx->rxWriteIndex-1):(CANRxLEN-1); //获取写入FIFO指针，上传到PC
//			//先显示低位 (B1B0 B3B2 B5B4 B7B6顺序显示)
//			printf("\r\nCAN Rx Index :%d ,Data :0x%0.4x 0x%0.4x 0x%0.4x 0x%0.4x    wr Index :%d",PtrCanRx->rxReadIndex\
//					,PtrCanRx->rxData[PtrCanRx->rxReadIndex][0]\
//					,PtrCanRx->rxData[PtrCanRx->rxReadIndex][1]\
//					,PtrCanRx->rxData[PtrCanRx->rxReadIndex][2]\
//					,PtrCanRx->rxData[PtrCanRx->rxReadIndex][3]\
//					,wrIndex);
//			//点亮LED
//			switch(PtrCanRx->rxData[PtrCanRx->rxReadIndex][0])
//			{
//			   case '1':  D401TOGGLE();break;
//			   case '2':  D402TOGGLE();break;
//			   default:break;
//			}
//			//数码管显示低2个字节的数据
//			Running.showhex[1]= (PtrCanRx->rxData[PtrCanRx->rxReadIndex][0]>>4)&0x0f;
//			Running.showhex[0]= PtrCanRx->rxData[PtrCanRx->rxReadIndex][0]&0x0f;
//			PtrCanRx->rxReadIndex=(++PtrCanRx->rxReadIndex)%(CANRxLEN);
//		}
//
//	}
//
//	//这里做CAN缓存溢出处理
//	if(PtrCanRx->Mark_Para.Status_Bits.rFifoFullFlag ==1)
//	{
//		PtrCanRx->Mark_Para.Status_Bits.rFifoFullFlag = 0;
//		printf("\r\n--------------CAN Rx FIFO overflow-------------------------------------");
//
//		PtrCanRx->rxReadIndex  = 0;
//		PtrCanRx->rxWriteIndex = 0;
//	}
//
//	//这里做硬件CAN接收溢出处理
//	if(PtrCanRx->Mark_Para.Status_Bits.HWOVFlag == 1)
//	{
//		PtrCanRx->Mark_Para.Status_Bits.HWOVFlag = 0;
//		printf("\r\n---------------CAN Rx HW Overflow ISR Clear-------------------------");
//	}

}


/****************************************************************************
 * @brief： CAN测试上传电压电流给中位机
 * @param：
 * @retval：
 * @note：
 *
 *
 ****************************************************************************/
Uint8 CAN_TestUpVI(void)
{
    Uint8   Buf[8];
    Uint32 extID;
    Uint32 batVol=24123; // mV
    int32 batCur=500; // mA

//    Uint8 workMode=0x02; // 恒流

//    batVol *= 10;
//    batCur *= 10;

    batVol = g_strAdcData[eVbat].dRmsValBitFilter*10; // 上传精度高，单位是0.1mV
    batCur = g_strAdcData[eIbat].dRmsValBitFilter*10; // 上传精度高，单位是0.1mV



    // 这里把小电流去掉
    if(canSetPra.batWorkMode==MODE_STOP)
    {
        if(batCur<1500) // 150mA以内去掉
            batCur = 0;
    }
//        batCur = 0;
//    else
//    {
//        if( canSetPra.batWorkMode == MODE_CHARGE)
//            batCur = batCur + timer0Base.msCounter%1000;
//        else if( canSetPra.batWorkMode == MODE_DISCHARGE)
//            batCur = 0-(batCur + timer0Base.msCounter%1000);
//        else  if( canSetPra.batWorkMode == MODE_SPACE)
//            batCur = 0;
//    }



    //  0811 充放电累加计数
    extID = 0x0811FF00|canSetPra.swSendID;
    Buf[0]=(Uint8)canSetPra.chargeCapacity.byte.BYTE0;
    Buf[1]=(Uint8)canSetPra.chargeCapacity.byte.BYTE1;
    Buf[2]=(Uint8)canSetPra.chargeCapacity.byte.BYTE2;
    Buf[3]=(Uint8)canSetPra.chargeCapacity.byte.BYTE3;
    Buf[4]=(Uint8)canSetPra.dischargeCapacity.byte.BYTE0;
    Buf[5]=(Uint8)canSetPra.dischargeCapacity.byte.BYTE1;
    Buf[6]=(Uint8)canSetPra.dischargeCapacity.byte.BYTE2;
    Buf[7]=(Uint8)canSetPra.dischargeCapacity.byte.BYTE3;
    CAN_Send_Inbuff(extID,Buf);


    //  0810电压电流上传
    extID = 0x0810FF00|canSetPra.swSendID;

    // 第一个字节为序列号+工作模式

    if(canSetPra.getConfigFlag)
    {
        Buf[0]=canSetPra.workModeTmp;
//        if(canSetPra.batWorkMode!=MODE_STOP)
                Buf[0] |= canSetPra.runSerialNo;
    }
    else
    {
        Buf[0]=0x0f;
    }

    if(canSetPra.batWorkMode==MODE_CHARGE)
    {
        if(batCur<0)  // 充电时不能为负值,否则容量计算错误会报错
        {
            batCur=0;
        }
    }
    else if(canSetPra.batWorkMode==MODE_DISCHARGE)
    {
        if(batCur>0) // 放电时不能为正值,否则容量计算错误会报错
        {
            batCur=0;
        }
    }

    Buf[1]=batVol&0xff;
    Buf[2]=(batVol>>8)&0xff;
    Buf[3]=(batVol>>16)&0xff;

    if(batCur<0)
        batCur = 4294967296 + batCur;

    Buf[4]=batCur&0xff;
    Buf[5]=(batCur>>8)&0xff;
    Buf[6]=(batCur>>16)&0xff;
    Buf[7]=(batCur>>24)&0xff;


    CAN_Send_Inbuff(extID,Buf);



    return 0;
}


/****************************************************************************
 * @brief： 故障显示
 * @param：
 * @retval：
 * @note：
 *
 *          要按照3S上传一次，如果1S上传一次，上传0都会报错
 ****************************************************************************/
Uint8 CAN_TestUpErr(void)
{
    Uint8   Buf[8];
    Uint32 extID;

//    //  故障显示
//    extID = 0x1040FF00|canSetPra.swSendID;
//    Buf[0]=0x00;Buf[1]=0x00;Buf[2]=0x00;Buf[3]=0x00;
//    Buf[4]=0x00;Buf[5]=0x00;Buf[6]=0x00;Buf[7]=0x00;
//    CAN_Send_Inbuff(extID,Buf);

}


/****************************************************************************
 * @brief： CAN测试收到CAN数据帧之后的处理
 * @param：
 * @retval：
 * @note：
 *
 *
 ****************************************************************************/
Uint8 CAN_RX_Fun(Uint32 revId,Uint8 *dat)
{
    Uint8 i;
    Uint8   revDat[8];
    Uint8   sendDat[8];
    Uint32 sendID;
    Uint32 revIdTmp;
    myUInt16 myU16Temp;
    myUInt32 myU32Temp;


    revIdTmp = revId;
    revIdTmp = revIdTmp & 0xffff00ff;

    for(i=0;i<8;i++)
    {
        revDat[i]=*dat++;
    }

    if(revIdTmp==0x04900002)  // 中位机写数据  0x04904102
    {
            switch(revDat[0])   // 根据第一个数据，确认是写什么内容
            {
                case 0x01:
                        if((revDat[1]==0x00)&&(revDat[2]==0x00))  // 01 00 00为停机
                        {
                            canSetPra.workMode=WORK_MODE_IDLE;
                            canSetPra.workModeTmp = canSetPra.workMode;
                            canSetPra.batWorkMode = MODE_STOP;
    //                        printf("设置为停机 \r\n");
                            canSetPra.stRunFlag = 0;
                            canSetPra.runSerialNo=0;
                        }
                        else if((revDat[1]==0x00)&&(revDat[2]==0x02))  // 01 00 02为中位机启动恒流测试
                        {
                            canSetPra.tmpInt32.byte.BYTE0 = revDat[4];
                            canSetPra.tmpInt32.byte.BYTE1 = revDat[5];
                            canSetPra.tmpInt32.byte.BYTE2 = revDat[6];
                            canSetPra.tmpInt32.byte.BYTE3 = revDat[7];

                            if(canSetPra.tmpInt32.all<0) // 放电
                            {
                                canSetPra.batWorkMode = MODE_DISCHARGE;
                                canSetPra.curRef.all=0-canSetPra.tmpInt32.all;  // 变成正数
                                canSetPra.batRef.all=0;  // 恒流放电目标电压为0
                            }
                            else
                            {
                                canSetPra.batWorkMode = MODE_CHARGE;
                                canSetPra.curRef.all=canSetPra.tmpInt32.all;
                                canSetPra.batRef.all=SM_MAX_V; // 恒流充电目标电压为最大值
                            }

                            canSetPra.stRunFlag = 1;  // 启动测试
                            canSetPra.dischargeCapacity.all=0;
                            canSetPra.chargeCapacity.all=0;
                        }
                        else if((revDat[1]==0x00)&&(revDat[2]==0x06))  // 01 00 06为中位机启动恒压恒流测试
                        {
                            canSetPra.tmpInt32.byte.BYTE0 = revDat[4];
                            canSetPra.tmpInt32.byte.BYTE1 = revDat[5];
                            canSetPra.tmpInt32.byte.BYTE2 = revDat[6];
                            canSetPra.tmpInt32.byte.BYTE3 = revDat[7];
                            canSetPra.batRef.all=canSetPra.tmpInt32.all;

                            canSetPra.stRunFlag = 1;  // 启动测试
                            canSetPra.dischargeCapacity.all=0;
                            canSetPra.chargeCapacity.all=0;
                        }

                    break;
                case 0x02:

                    break;
                case 0x04:  // 设置电流
                        canSetPra.tmpInt32.byte.BYTE0 = revDat[2];
                        canSetPra.tmpInt32.byte.BYTE1 = revDat[3];
                        canSetPra.tmpInt32.byte.BYTE2 = revDat[4];
                        canSetPra.tmpInt32.byte.BYTE3 = revDat[5];

                        if(canSetPra.tmpInt32.all<0) // 放电
                        {
                            canSetPra.batWorkMode = MODE_DISCHARGE;
                            canSetPra.curRef.all=0-canSetPra.tmpInt32.all;  // 变成正数
                        }
                        else
                        {
                            canSetPra.batWorkMode = MODE_CHARGE;
                            canSetPra.curRef.all=canSetPra.tmpInt32.all;
                        }

                    break;
                case 0x3C: // 电池电压--电压高点
                        myU16Temp.byte.BYTE0 = revDat[2];
                        myU16Temp.byte.BYTE1 = revDat[3];
                        CalPra.V_Meter_H=myU16Temp.all;                     // 上位机下发值：mV
                        CalPra.V_ADC_H=g_strAdcData[eVbat].ADC_BitFilter;   // ADC经过滤波的采样值

                        canSetPra.needReflashCalPraFlag=1;
                    break;
                case 0x3E: // 电压零点--电压低点
                        myU16Temp.byte.BYTE0 = revDat[2];
                        myU16Temp.byte.BYTE1 = revDat[3];
                        CalPra.V_Meter_L=myU16Temp.all/10;
                        CalPra.V_ADC_L=g_strAdcData[eVbat].ADC_BitFilter;      // ADC经过滤波的采样值

                        canSetPra.needReflashCalPraFlag=1;
                    break;
                case 0x3D: // 电池电流--电流高点
                        myU16Temp.byte.BYTE0 = revDat[2];
                        myU16Temp.byte.BYTE1 = revDat[3];
                        CalPra.I_Meter_H=myU16Temp.all;
                        CalPra.I_ADC_H=g_strAdcData[eIbat].ADC_BitFilter;      // ADC经过滤波的采样值

                        canSetPra.needReflashCalPraFlag=1;
                    break;
                case 0x3F: // 电流零点--电流低点
                        myU16Temp.byte.BYTE0 = revDat[2];
                        myU16Temp.byte.BYTE1 = revDat[3];
                        CalPra.I_Meter_L=myU16Temp.all/10;
                        CalPra.I_ADC_L=g_strAdcData[eIbat].ADC_BitFilter;      // ADC经过滤波的采样值

                        canSetPra.needReflashCalPraFlag=1;
                    break;
                default:

                    break;
            }

            canSetPra.getConfigFlag=1;  // 收到配置标志

            // 收到0x0490  回0496 返回与写的数据一样

//            if(canSetPra.tmpRev0490 != revDat[0])
//            {
//                canSetPra.tmpRev0490_Count = 0;
//            }
//
//            canSetPra.tmpRev0490_Count++;
//            if(canSetPra.tmpRev0490_Count>=3)
//            {
//                canSetPra.tmpRev0490_Count = 0;

                sendID = 0x0496FF00|canSetPra.swSendID;   // 0x0496FF41
                CAN_Send_Inbuff(sendID,revDat);
//            }


//            canSetPra.tmpRev0490=revDat[0];

    }
    else if(revIdTmp==0x04910002)  // 上位机写数据  0x04914102
    {
//        printf("revDat[0]: %x\r\n",revDat[0]);
//        printf("revDat[1]: %x\r\n",revDat[1]);
//        printf("revDat[2]: %x\r\n",revDat[2]);
//        printf("revDat[3]: %x\r\n",revDat[3]);
//        printf("revDat[4]: %x\r\n",revDat[4]);
//        printf("revDat[5]: %x\r\n",revDat[5]);
//        printf("revDat[6]: %x\r\n",revDat[6]);
//        printf("revDat[7]: %x\r\n",revDat[7]);

        if((revDat[0]>0x08)&&(revDat[0]==revDat[1]))  // 全局参数发送完成
        {
            sendID = 0x0497FF00|canSetPra.swSendID;
            sendDat[0]=0x08;sendDat[1]=0x00;sendDat[2]=0x00;sendDat[3]=0x00;
            sendDat[4]=0xFF;sendDat[5]=0x07;sendDat[6]=0x00;sendDat[7]=0x00;
            CAN_Send_Inbuff(sendID,sendDat);

//            canSetPra.runSerialNoTmp=0x00;
        }
        else if((revDat[0]==0x07) &&(revDat[1]==0x05))
        {
            canSetPra.tmpCmd0705.byte.BYTE0 = revDat[4];  // 暂存
            canSetPra.tmpCmd0705.byte.BYTE1 = revDat[5];
            canSetPra.tmpCmd0705.byte.BYTE2 = revDat[6];
            canSetPra.tmpCmd0705.byte.BYTE3 = revDat[7];

        }
        else if((revDat[0]==0x07) &&(revDat[1]==0x06))
        {
            canSetPra.workMode = revDat[4];  // 设置的工作模式
            canSetPra.tmpCmd0706_6=revDat[6];
            canSetPra.tmpCmd0706_7=revDat[7]; // 保存下来，因为要2帧发收取完电压或者电流
        }
        else if((revDat[0]==0x07) &&(revDat[1]==0x07) )
        {
            sendID = 0x0497FF00|canSetPra.swSendID;
            sendDat[0]=0x07;sendDat[1]=0x00;sendDat[2]=0x00;sendDat[3]=0x00;
            sendDat[4]=0x7F;sendDat[5]=0x00;sendDat[6]=0x00;sendDat[7]=0x00;
            CAN_Send_Inbuff(sendID,sendDat);

            // 序列号加1
            if(canSetPra.runSerialNo==0xC0)
                canSetPra.runSerialNo = 0x40;
            else
                canSetPra.runSerialNo += 0x40;

            // 接收电压电流值
            if(canSetPra.workMode==WORK_MODE_CC) // 恒流放电
               {
   //                printf("设置工作模式WORK_MODE_CC \r\n");
                   canSetPra.batRef.all =  canSetPra.tmpCmd0705.all;

                   canSetPra.tmpInt32.byte.BYTE0 = canSetPra.tmpCmd0706_6;
                   canSetPra.tmpInt32.byte.BYTE1 = canSetPra.tmpCmd0706_7;
                   canSetPra.tmpInt32.byte.BYTE2 = revDat[2];
                   canSetPra.tmpInt32.byte.BYTE3 = revDat[3];

                   if(canSetPra.tmpInt32.all<0) // 放电
                   {
                       canSetPra.batWorkMode = MODE_DISCHARGE;
                       canSetPra.curRef.all=0-canSetPra.tmpInt32.all;  // 变成正数
   //                    printf("放电,电压：%d \r\n",canSetPra.batRef);
   //                    printf("放电,电流：%d \r\n",canSetPra.curRef);
                   }
                   else
                   {
                       canSetPra.batWorkMode = MODE_CHARGE;
                       canSetPra.curRef.all=canSetPra.tmpInt32.all;

                       if(canSetPra.batRef.all==0) // 充电时电压参考为0时，赋值为最大
                       {
                           canSetPra.batRef.all = 30000; // 暂时设置为30V
                       }
   //                    printf("充电,电压：%d \r\n",canSetPra.batRef);
   //                    printf("充电,电流：%d \r\n",canSetPra.curRef);
                   }
               }
               else  if(canSetPra.workMode==WORK_MODE_CCCV) // 暂时只考虑恒流恒压充电
               {
                   canSetPra.batRef.byte.BYTE0 = canSetPra.tmpCmd0706_6;
                   canSetPra.batRef.byte.BYTE1 = canSetPra.tmpCmd0706_7;
                   canSetPra.batRef.byte.BYTE2 = revDat[2];
                   canSetPra.batRef.byte.BYTE3 = revDat[3];

                   canSetPra.batWorkMode = MODE_CHARGE;

                    canSetPra.curRef.all=canSetPra.tmpCmd0705.all;
               }
               else{

                   canSetPra.workModeTmp = canSetPra.workMode;
               }

//            printf("启动测试！！ \r\n");
               if((canSetPra.batWorkMode  ==  MODE_CHARGE)||(canSetPra.batWorkMode  ==  MODE_DISCHARGE))
               {
                   canSetPra.stRunFlag = 1;
                   canSetPra.dischargeCapacity.all=0;
                   canSetPra.chargeCapacity.all=0;
               }


            /* 此处上位机启动测试
            模式： 恒流充电--：    参数充电电流byte【1】=6时，取【6】【7】，截止电压byte【5】=6时，取【4】【5】 截止电压可能没有
            模式： 恒流放电--：    参数放电电流byte【1】=6时，取【6】【7】，截止电压byte【5】=6时，取【4】【5】 电流为负值
            模式： 恒流恒压充电--：参数充电电压byte【1】=6时，取【6】【7】，充电电流byte【5】=6时，取【4】【5】
            模式： 恒流恒压放电--：参数放电电压byte【1】=6时，取【6】【7】，放电电流byte【5】=6时，取【4】【5】 电压电流都为负值

            模式： 恒压充电--：    参数充电电压byte【1】=5时，取【6】【7】 byte【0】为0x06

            byte【1】=6时，byte【4】定义如下：（恒压时byte【1】=5时）
            0x00：待机,     0x01：恒压（正值充电，负值放电）
            0x02：恒流（正值充电，负值放电）     0x03：恒功率（正值充电，负值放电）
            0x04: 恒阻（正值充电，负值放电）     0x05: 搁置
            0x06: 恒流恒压（恒压值，正值充电，负值放电）
            0x0A:工况模拟，
            */

        }
        else if((revDat[0]==0x06) &&(revDat[1]==0x05)) //
        {
            canSetPra.workMode = revDat[4];  // 设置的工作模式
            if(canSetPra.workMode==WORK_MODE_SPACE) //  搁置
            {
                canSetPra.batWorkMode  =  MODE_SPACE;

            }
            else if(canSetPra.workMode==WORK_MODE_CV) //  恒压
            {


            }
            else
            {
                canSetPra.workModeTmp = canSetPra.workMode;
            }

        }
        else if((revDat[0]==0x06) &&(revDat[1]==0x06) )  // 恒压，搁置 启动
        {
            sendID = 0x0497FF00|canSetPra.swSendID;
            sendDat[0]=0x07;sendDat[1]=0x00;sendDat[2]=0x00;sendDat[3]=0x00;
            sendDat[4]=0x3F;sendDat[5]=0x00;sendDat[6]=0x00;sendDat[7]=0x00;
            CAN_Send_Inbuff(sendID,sendDat);

            if(canSetPra.runSerialNo==0xC0)
                canSetPra.runSerialNo = 0x40;
            else
                canSetPra.runSerialNo += 0x40;
//            printf("启动测试！！ \r\n");
            // 此处上位机启动测试

            if((canSetPra.batWorkMode  ==  MODE_CHARGE)||(canSetPra.batWorkMode  ==  MODE_DISCHARGE))
            {
                canSetPra.stRunFlag = 1;
                canSetPra.dischargeCapacity.all=0;
                canSetPra.chargeCapacity.all=0;
            }
            else if(canSetPra.batWorkMode  ==  MODE_SPACE) // 启动搁置需要停机,因为放电--搁置--放电,中间没有停止帧
            {
                canSetPra.stRunFlag = 0;
                canSetPra.workModeTmp = canSetPra.workMode;; // 搁置直接赋值，充放电要等预充完成再赋值
            }
        }

    }

    if(revId==0x1020FF02)  // 心跳
    {
//        printf("收到心跳帧 \r\n");
        canSetPra.getHeartFlag=1;  // 收到心跳帧标志
        canSetPra.hearTimeCount=0; // 收到心跳帧，清0心跳计数

        if(canSetPra.heartCount>1)  // 错误需要持续2~3S,让中位机知道
        {
            if(canSetPra.workMode==WORK_MODE_ERR)
            {
                canSetPra.workMode=WORK_MODE_IDLE;
//                printf("当工作模式为故障时，回到待机状态 \r\n");
            }
        }
        canSetPra.heartCount++;
    }



	if(canSetPra.stRunFlag == 1) 
		{
	    uwDeviceWorkMode = eNormalRunning; // 正常发波
		}


	if(canSetPra.batWorkMode==MODE_STOP)
	{
	    uwBatWorkMode = eDCDCStop;
	}
	else if((canSetPra.batWorkMode==MODE_CHARGE)&&(!canSetPra.preChargeFlag))
	{
	    uwBatWorkMode = eCharge;
	}
	else if((canSetPra.batWorkMode==MODE_DISCHARGE)&&(!canSetPra.preChargeFlag))
	{
	    uwBatWorkMode = eDisCharge;
	}
	else if(canSetPra.batWorkMode==MODE_SPACE)
	{
	    uwBatWorkMode = eDCDCStop;
	}
		

return 0;
	
}

/****************************************************************************
 * @brief： 心跳帧超时处理
 * @param：
 * @retval：
 * @note：
 *
 ****************************************************************************/
void heartTimeOutFun(void)
{
    StopTheSM();

    canSetPra.batWorkMode = MODE_STOP;
  //  printf("心跳帧超时处理 \r\n");
    canSetPra.heartCount=0;
    canSetPra.getHeartFlag=0;

    canSetPra.workMode=WORK_MODE_IDLE;
    canSetPra.workModeTmp=WORK_MODE_IDLE;
    canSetPra.batWorkMode = MODE_STOP;
    canSetPra.chargeCapacity.all=0;
    canSetPra.dischargeCapacity.all=0;
    canSetPra.u64CountChargeCapacity.all=0;
    canSetPra.u64CountDisChargeCapacity.all=0;
    canSetPra.backRunCountSpace=0;
    canSetPra.back1032Byte=0x9D;
    canSetPra.back1032Byte_P_N_Flag=1;
    canSetPra.preChargeFlag=0;
    canSetPra.runReadyFlag=0;
    canSetPra.getConfigFlag=0;
    canSetPra.back1031LastByte=0;

    canSetPra.tmpRev0490=0;
    canSetPra.tmpRev0490_Count=0;

    canSetPra.enSendCount=0;
    canSetPra.enSendFlag=0;
}


/****************************************************************************
 * @brief： CAN接收初始化参数
 * @param：
 * @retval：
 * @note：
 *
 ****************************************************************************/
void canRevInit(void)
{
    canSetPra.workMode=WORK_MODE_IDLE;
    canSetPra.workModeTmp = canSetPra.workMode;
    canSetPra.batWorkMode = MODE_STOP;
    canSetPra.getHeartFlag=0;
    canSetPra.heartCount=0;
    canSetPra.chargeCapacity.all=0;
    canSetPra.dischargeCapacity.all=0;
    canSetPra.u64CountChargeCapacity.all=0;
    canSetPra.u64CountDisChargeCapacity.all=0;
    canSetPra.backRunCountSpace=0;
    canSetPra.back1032Byte=0x9D;
    canSetPra.back1032Byte_P_N_Flag=1;
	canSetPra.tack1sCount=0;
	canSetPra.preChargeFlag=0;
	canSetPra.runReadyFlag=0;
    canSetPra.canSendBuf_SaveP=0; // 保存位置
    canSetPra.canSendBuf_SendP=0; // 发送位置
    canSetPra.getConfigFlag=0;
    canSetPra.back1031LastByte=0;

    canSetPra.getBufRefFlag=0;
    canSetPra.bufRef = 0;

    canSetPra.tmpRev0490=0;
    canSetPra.tmpRev0490_Count=0;

    canSetPra.enSendCount=0;
    canSetPra.enSendFlag=0;

    canSetPra.tack5msCount=0;

    canSetPra.tack5msCount_500ms=0;
    canSetPra.tack5msCount_55ms=0;

    canSetPra.runSerialNo=0;
    canSetPra.needReflashCalPraFlag=0;

//    canSetPra.canRevIntCount=0;
//    canSetPra.canRevIntErrCount=0;
//    canSetPra.canErrIntCount=0;
//    canSetPra.canREVCount=0;
//    canSetPra.canSendCount=0;

    // 上传报警数据
    upAlarm.byte.byte0=0;
    upAlarm.byte.byte1=0;
    upAlarm.byte.byte2=0;
    upAlarm.byte.byte3=0;
    upAlarm.byte.byte4=0;
    upAlarm.byte.byte5=0;
    upAlarm.byte.byte6=0;
    upAlarm.byte.byte7=0;
}


/****************************************************************************
 * @brief： CAN进发送队列函数
 * @param：
 * @retval：
 * @note：
 *
 ****************************************************************************/
void CAN_Send_Inbuff(Uint32 ExtID,Uint8* dat)
{
    Uint8 i;

    canSetPra.canSendBuf_ExtID[canSetPra.canSendBuf_SaveP]=ExtID;
    for(i=0;i<8;i++)
    {
        canSetPra.canSendBuf[canSetPra.canSendBuf_SaveP][i]=*dat++;
    }
    canSetPra.canSendBuf_SaveP++;
    if(canSetPra.canSendBuf_SaveP>=(CAN_SEND_BUF_LEN-1))
        canSetPra.canSendBuf_SaveP = 0;
}

/****************************************************************************
 * @brief： CAN 100us处理函数 -- 发送队列
 * @param：
 * @retval：
 * @note：
 *
 ****************************************************************************/
void canTask100us(void)
{
    if(canSetPra.canSendBuf_SendP != canSetPra.canSendBuf_SaveP)  // 缓存区有数据要发送
    {
        CAN_Send(canSetPra.canSendBuf_ExtID[canSetPra.canSendBuf_SendP],canSetPra.canSendBuf[canSetPra.canSendBuf_SendP]);

        canSetPra.canSendBuf_SendP++;
        if(canSetPra.canSendBuf_SendP>=(CAN_SEND_BUF_LEN-1))
            canSetPra.canSendBuf_SendP = 0;
    }
}

/****************************************************************************
 * @brief： CAN100ms处理函数
 * @param：
 * @retval：
 * @note：
 *          电压电流未运行时1S发一次，运行时这里先固定100ms发送一次  500ms
 ****************************************************************************/
void canTask100ms(void)
{
	if(canSetPra.batWorkMode != MODE_STOP)
	{
    	CAN_TestUpVI();
	}

    if(canSetPra.runReadyFlag==1)
    {
        canSetPra.runReadyFlag=0;
        canSetPra.workModeTmp = canSetPra.workMode;
    }


}


/****************************************************************************
 * @brief： CAN5ms处理函数
 * @param：
 * @retval：
 * @note：
 *          5ms  此函数根据模块ID值，避开多个模块同时上传数据
 ****************************************************************************/
void canTask5ms(void)
{
    if(canSetPra.getHeartFlag)
    {
        if(canSetPra.enSendFlag==0)
        {
            if(canSetPra.enSendCount<(canSetPra.swValue*3))
            {
                canSetPra.enSendCount ++;
            }
            else
            {
                canSetPra.enSendFlag=1;
            }
        }
    }

    canSetPra.tack5msCount_55ms++;
    if(canSetPra.tack5msCount_55ms==11)
    {
        canSetPra.tack5msCount_55ms=0;
        canTask55ms();
    }


    canSetPra.tack5msCount_500ms++;
    if(canSetPra.tack5msCount_500ms==100)
    {
        canSetPra.tack5msCount_500ms=0;
        canTask100ms();
    }

    if(canSetPra.enSendFlag)
    {
        canSetPra.tack5msCount++;
        if(canSetPra.tack5msCount==200)
        {
            canSetPra.tack5msCount=0;
            can5msTo1s();

        }
    }
}


/****************************************************************************
 * @brief： CAN 1S处理函数
 * @param：
 * @retval：
 * @note：
 *
 *
 ****************************************************************************/
void canTask1s(void)
{
	if(canSetPra.batWorkMode == MODE_STOP)
	{
    	CAN_TestUpVI();
	}

   // 心跳超时处理
    canSetPra.hearTimeCount++;
    if(canSetPra.hearTimeCount>10)
    {
        canSetPra.hearTimeCount=0;
        heartTimeOutFun();  // 超时处理
    }

    // 搁置计数
    if( canSetPra.batWorkMode == MODE_SPACE)
    {
        canSetPra.backRunCountSpace++;
    }
    else
    {
        canSetPra.backRunCountSpace=0;

    }

//    if(canSetPra.runReadyFlag==1)
//    {
//        canSetPra.runReadyFlag=0;
//        canSetPra.workModeTmp = canSetPra.workMode;
//
//    }


}


/****************************************************************************
 * @brief： CAN5ms函数计数到1S后调用
 * @param：
 * @retval：
 * @note：
 *
 *
 ****************************************************************************/
void can5msTo1s(void)
{

    CheckStopLogic();  // 有错误时停机

    canSetPra.tack1sCount++;
    if(canSetPra.tack1sCount==24)
    {
        canSetPra.tack1sCount=0;
    }

    if(canSetPra.tack1sCount%3==0)
    {
//      if((canSetPra.batWorkMode != MODE_STOP)&&( canSetPra.batWorkMode != MODE_SPACE))
//      {
//            if(canSetPra.back1032Byte_P_N_Flag==1)
//                canSetPra.back1032Byte++;
//            else
//                canSetPra.back1032Byte--;
//
//            if(canSetPra.back1032Byte==0xA1)
//                canSetPra.back1032Byte_P_N_Flag=0;
//            else if(canSetPra.back1032Byte==0x00)
//                canSetPra.back1032Byte_P_N_Flag=1;
//      }

        if(canSetPra.getHeartFlag)
        {
            canTask3S(); // 这个帧3S上传一次
        }



    }


//    // 充电计数
//    if(canSetPra.batWorkMode == MODE_CHARGE)
//        canSetPra.chargeCapacity++;
//    else
//        canSetPra.chargeCapacity=0;
//
//    if(canSetPra.tack1sCount%7==0)
//    {
//            // 放电计数
//        if(canSetPra.batWorkMode == MODE_DISCHARGE)
//            canSetPra.dischargeCapacity++;
//        else
//            canSetPra.dischargeCapacity=0;
//    }

    if(canSetPra.tack1sCount%12==0)
    {
        if(canSetPra.getHeartFlag)
        {
            canTask12S();
        }
    }

}



/****************************************************************************
 * @brief： CAN测试上传数据帧给中位机
 * @param：
 * @retval：
 * @note：
 *          3S发送一次
 *          发送邮箱不够，一次性发太多导致有些帧发不出来
 ****************************************************************************/
Uint8 canTask3S(void)
{
    Uint8   Buf[8];
    Uint32 extID;

    //  报错 1040
    extID = 0x1040FF00|canSetPra.swSendID;
    Buf[0]=upAlarm.byte.byte0;Buf[1]=upAlarm.byte.byte1;
    if(canSetPra.workMode==WORK_MODE_ERR)
    {
        Buf[2]=0x01;
    }
    else
    {
        Buf[2]=upAlarm.byte.byte2;
    }
    Buf[3]=upAlarm.byte.byte3;
    Buf[4]=upAlarm.byte.byte4;Buf[5]=upAlarm.byte.byte5;Buf[6]=upAlarm.byte.byte6;Buf[7]=upAlarm.byte.byte7;
    CAN_Send_Inbuff(extID,Buf);

    // 1030
    extID = 0x1030FF00|canSetPra.swSendID;
    if(canSetPra.getConfigFlag==0)
    {
        Buf[0]=0x2F;  // 申请配置
    }
    else
    {
        Buf[0]=0x00;  // 配置完成
    }
    Buf[1]=0x40;Buf[2]=canSetPra.swValue;Buf[3]=0x00;
    Buf[4]=0x00;Buf[5]=0x00;Buf[6]=0x00;Buf[7]=0x00;
    CAN_Send_Inbuff(extID,Buf);

    //  未知 1031
    extID = 0x1031FF00|canSetPra.swSendID;
//    Buf[0]=0x36;Buf[1]=0x06;Buf[2]=0xFF;Buf[3]=0xFF;
//    Buf[4]=0x50;Buf[5]=0x01;Buf[6]=0x53;Buf[7]=0x01;
//    CAN_Send_Inbuff(extID,Buf);
    Buf[0]=0xD8;Buf[1]=0x05;Buf[2]=0xFC;Buf[3]=0x01;
//    Buf[4]=0x1D;
    Buf[5]=0x01;
    if((canSetPra.batWorkMode == MODE_CHARGE)||(canSetPra.batWorkMode == MODE_DISCHARGE))
    {
        canSetPra.back1031LastByte--;
        Buf[4]=canSetPra.back1031LastByte+8;
        Buf[6]=canSetPra.back1031LastByte;
    }
    else
    {
        canSetPra.back1031LastByte++;
        Buf[4]=canSetPra.back1031LastByte+8;
        Buf[6]=canSetPra.back1031LastByte;
    }
    Buf[7]=0x01;

    CAN_Send_Inbuff(extID,Buf);

    //  未知 1032
    extID = 0x1032FF00|canSetPra.swSendID;
    //Buf[0]=0xA1;
    if((canSetPra.batWorkMode == MODE_CHARGE)||(canSetPra.batWorkMode == MODE_DISCHARGE))
    {
        canSetPra.back1032Byte--;
    }
    else
    {
        canSetPra.back1032Byte++;
    }
    //      if((canSetPra.batWorkMode != MODE_STOP)&&( canSetPra.batWorkMode != MODE_SPACE))
    //      {
    //            if(canSetPra.back1032Byte_P_N_Flag==1)
    //                canSetPra.back1032Byte++;
    //            else
    //                canSetPra.back1032Byte--;
    //
    //            if(canSetPra.back1032Byte==0xA1)
    //                canSetPra.back1032Byte_P_N_Flag=0;
    //            else if(canSetPra.back1032Byte==0x00)
    //                canSetPra.back1032Byte_P_N_Flag=1;
    //      }
    Buf[0]=canSetPra.back1032Byte;
//    if(canSetPra.batWorkMode == MODE_SPACE) // 搁置时这个字节不改变
//    {
//        Buf[0]=0x93;
//    }
    Buf[1]=0x01;Buf[2]=0x00;Buf[3]=0x00;
    Buf[4]=0x00;Buf[5]=0x00;Buf[6]=0x00;Buf[7]=0x00;
    CAN_Send_Inbuff(extID,Buf);

    //  未知 1033
    extID = 0x1033FF00|canSetPra.swSendID;
    Buf[0]=0x00;Buf[1]=0x00;Buf[2]=0x00;Buf[3]=0x00;
    Buf[4]=0x00;Buf[5]=0x00;Buf[6]=0x00;Buf[7]=0x00;
    CAN_Send_Inbuff(extID,Buf);

    //  未知 1035
    extID = 0x1035FF00|canSetPra.swSendID;
    Buf[0]=0x00;Buf[1]=0x00;Buf[2]=0x04;Buf[3]=0x00;
    Buf[4]=0xFE;Buf[5]=0xFF;Buf[6]=0xAD;Buf[7]=0xFA;
    CAN_Send_Inbuff(extID,Buf);

    //  未知 1036
    extID = 0x1036FF00|canSetPra.swSendID;
    Buf[0]=0x00;Buf[1]=0x00;Buf[2]=0x00;Buf[3]=0x00;
    Buf[4]=0x00;Buf[5]=0x00;Buf[6]=0x00;Buf[7]=0x00;
    CAN_Send_Inbuff(extID,Buf);

    return 0;
}


/****************************************************************************
 * @brief： CAN测试上传数据帧给中位机
 * @param：
 * @retval：
 * @note：
 *          3S发送一次
 *          发送邮箱不够，一次性发太多导致有些帧发不出来
 ****************************************************************************/
Uint8 canTask12S(void)
{
    Uint8   Buf[8];
    Uint32 extID;

    //  未知 1046
    extID = 0x1046FF00|canSetPra.swSendID;
    Buf[0]=0x96;Buf[1]=0x00;Buf[2]=0x32;Buf[3]=0x00;
    Buf[4]=0xD3;Buf[5]=0x07;Buf[6]=0xF6;Buf[7]=0x03;
    CAN_Send_Inbuff(extID,Buf);

    //  未知 1047
    extID = 0x1047FF00|canSetPra.swSendID;
    Buf[0]=0x04;Buf[1]=0x00;Buf[2]=0xEC;Buf[3]=0x03;
    Buf[4]=0x00;Buf[5]=0x00;Buf[6]=0x00;Buf[7]=0x00;
    CAN_Send_Inbuff(extID,Buf);

    return 0;
}


/****************************************************************************
 * @brief： 过滤CAN接收ID
 * @param：
 * @retval：
 * @note：
 *
 *
 ****************************************************************************/
void FilterCanReceiveID(void)
{
//    ECanaMboxes.MBOX16.MSGID.all = canSetPra.swRevID;  //  本机的ID
//    ECanaMboxes.MBOX16.MSGID.bit.AME = 1;
//    ECanaLAMRegs.LAM16.all = 0xFFFF00FF|(~canSetPra.swRevID); // 这个要重新赋值，在读到模块ID之后 这里00是，只接收41的帧
}


/****************************************************************************
 * @brief： 模块停机
 * @param：
 * @retval：
 * @note：
 *
 *
 ****************************************************************************/
void StopTheSM(void)
{

    canSetPra.workMode=WORK_MODE_IDLE;
    canSetPra.workModeTmp = canSetPra.workMode;
    canSetPra.batWorkMode = MODE_STOP;
    //                        printf("设置为停机 \r\n");
    canSetPra.stRunFlag = 0;
    canSetPra.runSerialNo=0;

//    Relay_P_C_OFF();
}

/****************************************************************************
 * @brief： 检测是否有错误要停机
 * @param：
 * @retval：
 * @note：
 *
 ****************************************************************************/
void CheckStopLogic(void)
{
    if( upAlarm.byte.byte0||upAlarm.byte.byte1||upAlarm.byte.byte2||upAlarm.byte.byte3||
        upAlarm.byte.byte4||upAlarm.byte.byte5||upAlarm.byte.byte6||upAlarm.byte.byte7  )
    {
        StopTheSM();
    }
}

/****************************************************************************
 * @brief： 计算容量
 * @param：
 * @retval：
 * @note：
 *          55ms计数一次
 ****************************************************************************/
void canTask55ms(void)
{
    int32 temp;

    // 充电容量
   if(canSetPra.batWorkMode == MODE_CHARGE)
   {
       canSetPra.u64CountChargeCapacity.all += g_strAdcData[eIbat].dRmsVal; // 容量积分
       canSetPra.chargeCapacity.byte.BYTE0 = canSetPra.u64CountChargeCapacity.byte.BYTE2;
       canSetPra.chargeCapacity.byte.BYTE1 = canSetPra.u64CountChargeCapacity.byte.BYTE3;
       canSetPra.chargeCapacity.byte.BYTE2 = canSetPra.u64CountChargeCapacity.byte.BYTE4;
       canSetPra.chargeCapacity.byte.BYTE3 = canSetPra.u64CountChargeCapacity.byte.BYTE5;
//       myInt64 u64CountChargeCapacity;
//       myInt64 u64CountDisChargeCapacity;

   }
   else
   {
       canSetPra.chargeCapacity.all=0;
       canSetPra.u64CountChargeCapacity.all=0;
   }

    // 放电容量
   if(canSetPra.batWorkMode == MODE_DISCHARGE)
   {
       temp = g_strAdcData[eIbat].dRmsVal;
       if(temp<0)
       {
           temp = 0 - temp;
       }
       canSetPra.u64CountDisChargeCapacity.all += temp; // 容量积分
       canSetPra.dischargeCapacity.byte.BYTE0 = canSetPra.u64CountDisChargeCapacity.byte.BYTE2;
       canSetPra.dischargeCapacity.byte.BYTE1 = canSetPra.u64CountDisChargeCapacity.byte.BYTE3;
       canSetPra.dischargeCapacity.byte.BYTE2 = canSetPra.u64CountDisChargeCapacity.byte.BYTE4;
       canSetPra.dischargeCapacity.byte.BYTE3 = canSetPra.u64CountDisChargeCapacity.byte.BYTE5;

   }
   else
   {
       canSetPra.dischargeCapacity.all=0;
       canSetPra.u64CountDisChargeCapacity.all=0;
   }
}



