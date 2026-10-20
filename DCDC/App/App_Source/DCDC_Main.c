/*
 * DCDC_Main.c
 *
 *  Created on: 2025年6月12日
 *      Author: ChenHF
 */


#ifndef MAIN_C
#define MAIN_C
//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"



void InitRam(void);
/******************************************************************************
Function Name: main
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void main(void)
    {
    // Step 1. Initialize System Control Registers
    System_PinInit();        // 初始化GPIO,提前到这里，因为IO默认会有上拉电阻，下面的初始化有100ms
                               // 这100ms会输出高电平导致继电器误操作
    InitRam();
    InitSysCtrl();  // 初始化系统时钟，外设时钟，看门狗等，将系统时钟设置为60M

    MemCopy(&RamfuncsLoadStart,&RamfuncsLoadEnd,&RamfuncsRunStart); // 初始化在RAM运行代码
    //InitFlash();

    // Step 2. Initialize GPIO Status and Device Peripherals
    System_PeripheralInit(); // 初始化外设

    // Step 3. Initialize PIE vector table
    DINT;
    IER = 0x0000;  // 12组中断使能寄存器，都关掉
    IFR = 0x0000;  // 12组中断的标志位，清零
    InitPieCtrl(); // 初始化PIE控制寄存器为默认值，默认的值是所有的中断静止，标志清零，中断通过pie控制器来管理中断响应
    InitPieVectTable();  // 初始化中断向量表，将指派各中断的中断函数入口地址

    // Step 4. Initialize App Parameter
    System_AppInit();  // APP初始化

    SwInit();
    getSwID();

    CANInit();  //CAN初始化

    // Step 5. Enable Interrupts Type
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;  // Enable PIE Gropu 1 INT1 ADCINT1 由PWM3触发50us一次
    IER |= M_INT1;
    PieCtrlRegs.PIEIER5.bit.INTx8 = 1;  // DSP_REALTIME_ISR 由ADC中断中软件触发100us一次
    IER |= M_INT5;
    PieCtrlRegs.PIEIER9.bit.INTx1 = 1;  // SCIA_INT    SCIRXINTA 接收中断
    PieCtrlRegs.PIEIER9.bit.INTx2 = 1;  // SCIA_TX_INT SCITXINTA 发送中断

    PieCtrlRegs.PIEIER9.bit.INTx5 = 1;      // Enable INT 9.5 in the PIE
    PieCtrlRegs.PIEIER9.bit.INTx6 = 1;      // Enable INT 9.6 in the PIE

 //------------中断配置步骤-----4
//    IER |= M_INT9;

    IER |= M_INT9;
    EnableInterrupts();

    EINT;  // Enable Global interrupt INTM
    ERTM;  // Enable Global realtime interrupt DBGM

    // Step 6. Start Working
    EALLOW;  // 允许对寄存器操作
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = TB_ENABLE;// Turn on the EPWM clock，启动计数
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;//make sure INT1 flag is cleared  adc中断
    EDIS;    // 禁止操作被保护的寄存器
    ServiceDog();  // 重置看门狗定时器
    EnableDog();   // 使能看门狗


    RelayInit();
    InitFlashCalPra();
    canRevInit();

    REDLED_ON;

    while(1)
    {
        Task_InvokingProcess();
    }
}
/******************************************************************************
Function Name: main
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void InitRam(void)
{
    Uint16 *uwAddrTmp;
    Uint32 udAddrIndexTmp = 0;

    uwAddrTmp = (Uint16 *)0x008010;

    for(udAddrIndexTmp = 0;udAddrIndexTmp < 0x001FF0;udAddrIndexTmp++)
    {
        *(uwAddrTmp + udAddrIndexTmp) = 0x0000;
    }

    uwAddrTmp = (Uint16 *)0x000400;

    for(udAddrIndexTmp = 0;udAddrIndexTmp < 0x0400;udAddrIndexTmp++)
    {
        *(uwAddrTmp + udAddrIndexTmp) = 0x0000;
    }

    uwAddrTmp = (Uint16 *)0x001500;
    for(udAddrIndexTmp = 0;udAddrIndexTmp < 0x0080;udAddrIndexTmp++)
    {
        *(uwAddrTmp + udAddrIndexTmp) = 0x0000;
    }
}
#endif

