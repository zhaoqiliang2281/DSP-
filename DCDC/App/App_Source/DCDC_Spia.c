/*
 * DCDC_Spia.c
 *
 *  Created on: 2025年7月8日
 *      Author: phone
 */
#ifndef SPIA_C
#define SPIA_C

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"
//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//


/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents:
******************************************************************************/
void SPIA_System_AppInit(void)
{

}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents:
******************************************************************************/
void SPIA_Task1_RealTimeDone(void)
{

}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents:
******************************************************************************/
void SPIA_Task_20ms(void)
{
//    if(SpiaRegs.SPIFFRX.bit.RXFFINT == 1) // 接收中断标志位
//    {
//        Data = SpiaRegs.SPIRXBUF; // 将8位数据更新进去
//
//        SpiaRegs.SPIFFRX.bit.RXFIFORESET = 0;               //复位FIFO，清相关寄存器
//        SpiaRegs.SPIFFRX.bit.RXFFINTCLR  = 1;
//        SpiaRegs.SPIFFRX.bit.RXFIFORESET = 1;
//    }
}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents:
******************************************************************************/
void SPIA_System_PinInit(void)
{
    InitSpiaGpio();
}
/******************************************************************************
Function Name:
--------------------
Function Descriptions:

Parameter Name list:

ReturnType:

Refer documents:
******************************************************************************/
void SPIA_System_PeripheralInit(void)
{
    SpiaRegs.SPICCR.all = 0x0007;       // Reset on, output on rising,input on falling edge, 8-bit char bits
    SpiaRegs.SPICTL.all = 0x000E;       // Enable master mode, special phase, enable talk, and SPI int disabled.

    SpiaRegs.SPIBRR = 0x000E;           //SPI Baud Rate--->SPCLK/(SPIBRR + 1)--->30/(14+1) = 2M

    SpiaRegs.SPIFFTX.all = 0xE040;      //禁止TXRDY中断,使能FIFO
    SpiaRegs.SPIFFRX.all = 0x2044;      //禁止RXRDY中断,使能FIFO
    SpiaRegs.SPIFFCT.all = 0x0;

    SpiaRegs.SPICCR.all =0x0087;        // Relinquish SPI from Reset
    SpiaRegs.SPIPRI.bit.FREE = 1;       // Set so breakpoints don't disturb xmission
}

#endif
