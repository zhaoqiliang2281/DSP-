/*
 * DCDC_IO.c
 *
 *  Created on: 2025年6月13日
 *      Author: phone
 */

#ifndef IO_C
#define IO_C

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"




//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------



/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void IO_System_PinInit(void)
{
    EALLOW;

    //
    // Each GPIO pin can be:
    // a) a GPIO input/output
    // b) peripheral function 1
    // c) peripheral function 2
    // d) peripheral function 3
    // By default, all are GPIO Inputs
    //
    GpioCtrlRegs.GPAMUX1.all = 0x0000;   // GPIO functionality GPIO0-GPIO15
    GpioCtrlRegs.GPAMUX2.all = 0x0000;   // GPIO functionality GPIO16-GPIO31
    GpioCtrlRegs.GPBMUX1.all = 0x0000;   // GPIO functionality GPIO32-GPIO44

    //
    // Dig.IO funct. applies to AIO2,4,6,10,12,14
    //
    GpioCtrlRegs.AIOMUX1.all = 0x0000;

    GpioCtrlRegs.GPADIR.all = 0x0000;   // GPIO0-GPIO31 are GP inputs
    GpioCtrlRegs.GPBDIR.all = 0x0000;   // GPIO32-GPIO44 are inputs
    GpioCtrlRegs.AIODIR.all = 0x0000;   // AIO2,4,6,19,12,14 are digital inputs

    //
    // Each input can have different qualification
    // a) input synchronized to SYSCLKOUT
    // b) input qualified by a sampling window
    // c) input sent asynchronously (valid for peripheral inputs only)
    //
    GpioCtrlRegs.GPAQSEL1.all = 0x0000;    // GPIO0-GPIO15 Synch to SYSCLKOUT
    GpioCtrlRegs.GPAQSEL2.all = 0x0000;    // GPIO16-GPIO31 Synch to SYSCLKOUT
    GpioCtrlRegs.GPBQSEL1.all = 0x0000;    // GPIO32-GPIO44 Synch to SYSCLKOUT

    //
    // Pull-ups can be enabled or disabled.
    //
    //GpioCtrlRegs.GPAPUD.all = 0x0000;      // Pullup's enabled GPIO0-GPIO31
    //GpioCtrlRegs.GPBPUD.all = 0x0000;      // Pullup's enabled GPIO32-GPIO44
    GpioCtrlRegs.GPAPUD.all = 0xFFFF;    // Pullup's disabled GPIO0-GPIO31
    GpioCtrlRegs.GPBPUD.all = 0xFFFF;    // Pullup's disabled GPIO32-GPIO44


    //系统上电时PWM先作为普通IO使用,确保电平正常,避免误打波损坏机器
    /***EPWM1***/
    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 1;      // Disable pull-up on GPIO0 (GPIO0)
    GpioDataRegs.GPACLEAR.bit.GPIO0 = 1; // Set GPIO0 lower
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;   // Configure GPIO0 as GPIO0
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;      // GPIO0 = output,0-Input(default),1-Output

    GpioCtrlRegs.GPAPUD.bit.GPIO1 = 1;      // Disable pull-up on GPIO1 (GPIO1)
    GpioDataRegs.GPACLEAR.bit.GPIO1 = 1; // Set GPIO1 lower
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 0;   // Configure GPIO0 as GPIO1
    GpioCtrlRegs.GPADIR.bit.GPIO1 = 1;      // GPIO1 = output,0-Input(default),1-Output

    /***EPWM2**FAN***/
    GpioCtrlRegs.GPAPUD.bit.GPIO2 = 1;      // Disable pull-up on GPIO2 (GPIO2)
    GpioDataRegs.GPACLEAR.bit.GPIO2 = 1; // Set GPIO2 lower
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 0;   // Configure GPIO2 as GPIO2
    GpioCtrlRegs.GPADIR.bit.GPIO2 = 1;      // GPIO2 = output,0-Input(default),1-Output

    /***RELAY C+***/
    GpioCtrlRegs.GPAPUD.bit.GPIO3 = 1;      // Disable pull-up on GPIO3 (GPIO3)
    GpioDataRegs.GPACLEAR.bit.GPIO3 = 1; // Set GPIO3 lower
    GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 0;   // Configure GPIO3 as GPIO3
    GpioCtrlRegs.GPADIR.bit.GPIO3 = 1;      // GPIO3 = output,0-Input(default),1-Output

    /***RELAY C-***/
    GpioCtrlRegs.GPAPUD.bit.GPIO14 = 1;      // Disable pull-up on GPIO14 (GPIO14)
    GpioDataRegs.GPACLEAR.bit.GPIO14 = 1; // Set GPIO14 lower
    GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 0;   // Configure GPIO14 as GPIO14
    GpioCtrlRegs.GPADIR.bit.GPIO14 = 1;      // GPIO14 = output,0-Input(default),1-Output

    /***RELAY CV***/
    GpioCtrlRegs.GPAPUD.bit.GPIO12 = 1;      // Disable pull-up on GPIO12 (GPIO12)
    GpioDataRegs.GPACLEAR.bit.GPIO12 = 1; // Set GPIO12 lower
    GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 0;   // Configure GPIO12 as GPIO12
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 1;      // GPIO12 = output,0-Input(default),1-Output

    /***RELAY P+***/
    GpioCtrlRegs.GPAPUD.bit.GPIO13 = 1;      // Disable pull-up on GPIO3 (GPIO3)
    GpioDataRegs.GPACLEAR.bit.GPIO13 = 1; // Set GPIO3 lower
    GpioCtrlRegs.GPAMUX1.bit.GPIO13 = 0;   // Configure GPIO3 as GPIO3
    GpioCtrlRegs.GPADIR.bit.GPIO13 = 1;      // GPIO3 = output,0-Input(default),1-Output

    /***RELAY P-***/
    GpioCtrlRegs.GPAPUD.bit.GPIO20 = 1;      // Disable pull-up on GPIO20 (GPIO20)
    GpioDataRegs.GPACLEAR.bit.GPIO20 = 1; // Set GPIO20 lower
    GpioCtrlRegs.GPAMUX2.bit.GPIO20 = 0;   // Configure GPIO20 as GPIO20
    GpioCtrlRegs.GPADIR.bit.GPIO20 = 1;      // GPIO20 = output,0-Input(default),1-Output



    /***RELAY PV***/
    GpioCtrlRegs.GPAPUD.bit.GPIO25 = 1;      // Disable pull-up on GPIO25 (GPIO25)
    GpioDataRegs.GPACLEAR.bit.GPIO25 = 1; // Set GPIO25 lower
    GpioCtrlRegs.GPAMUX2.bit.GPIO25 = 0;   // Configure GPIO25 as GPIO25
    GpioCtrlRegs.GPADIR.bit.GPIO25 = 1;      // GPIO25 = output,0-Input(default),1-Output

    /***RELAY USER***/
    GpioCtrlRegs.GPAPUD.bit.GPIO9 = 1;      // Disable pull-up on GPIO25 (GPIO25)
    GpioDataRegs.GPACLEAR.bit.GPIO9 = 1; // Set GPIO25 lower
    GpioCtrlRegs.GPAMUX1.bit.GPIO9 = 0;   // Configure GPIO25 as GPIO25
    GpioCtrlRegs.GPADIR.bit.GPIO9 = 1;      // GPIO25 = output,0-Input(default),1-Output

    /***IOCP***/
    GpioCtrlRegs.GPAPUD.bit.GPIO15 = 0;      // Enable pull-up on GPIO15 (GPIO15)
    GpioCtrlRegs.GPAQSEL1.bit.GPIO15 = 1;    // 3 samples,50ns
    GpioCtrlRegs.GPAMUX1.bit.GPIO15 = 1;     // Configure GPIO15 as TZ1
    GpioCtrlRegs.GPADIR.bit.GPIO15 = 0;      // GPIO15 = output,0-Input(default),1-Output

    /***LED 空脚***/
    GpioCtrlRegs.GPBPUD.bit.GPIO34 = 1;      // Disable pull-up on GPIO34 (GPIO34)
    GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1; // Set GPIO34 lower
    GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0;   // Configure GPIO34 as GPIO34
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;      // GPIO34 = output,0-Input(default),1-Output

    GpioCtrlRegs.GPBPUD.bit.GPIO41 = 1;      // Disable pull-up on GPIO41 (GPIO9)
    GpioDataRegs.GPBCLEAR.bit.GPIO41 = 1; // Set GPIO41 lower
    GpioCtrlRegs.GPBMUX1.bit.GPIO41 = 0;   // Configure GPIO41 as GPIO9
    GpioCtrlRegs.GPBDIR.bit.GPIO41 = 1;      // GPIO41 = output,0-Input(default),1-Output

    GpioCtrlRegs.GPBPUD.bit.GPIO43 = 1;      // Disable pull-up on GPIO43 (GPIO43)
    GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1; // Set GPIO43 lower
    GpioCtrlRegs.GPBMUX1.bit.GPIO43 = 0;   // Configure GPIO9 as GPIO43
    GpioCtrlRegs.GPBDIR.bit.GPIO43 = 1;      // GPIO43 = output,0-Input(default),1-Output

    /***REDLED***/
    GpioCtrlRegs.GPAPUD.bit.GPIO26 = 1;      // Disable pull-up on GPIO26 (GPIO26)
    GpioDataRegs.GPACLEAR.bit.GPIO26 = 1; // Set GPIO26 lower
    GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 0;   // Configure GPIO26 as GPIO26
    GpioCtrlRegs.GPADIR.bit.GPIO26 = 1;      // GPIO266 = output,0-Input(default),1-Output

    /***GREEDLED***/
    GpioCtrlRegs.GPAPUD.bit.GPIO27 = 1;      // Disable pull-up on GPIO27 (GPIO27)
    GpioDataRegs.GPACLEAR.bit.GPIO27 = 1; // Set GPIO27 lower
    GpioCtrlRegs.GPAMUX2.bit.GPIO27 = 0;   // Configure GPIO27 as GPIO27
    GpioCtrlRegs.GPADIR.bit.GPIO27 = 1;      // GPIO27 = output,0-Input(default),1-Output

    R_PP_OFF;
    R_PN_OFF;
    R_CP_OFF;
    R_CN_OFF;
    R_USER_OFF;

    EDIS;
}

#endif
