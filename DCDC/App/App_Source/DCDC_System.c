/*
 * DCDC_System.c
 *
 *  Created on: 2025年6月13日
 *      Author: phone
 */


#ifndef System_C
#define System_C

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
void System_PinInit(void)
{
    IO_System_PinInit();    // 继电器等IO初始化
    SCIA_System_PinInit();  // 串口管脚初始化
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void System_PeripheralInit(void)
{

    Adc_System_PeripheralInit();  // ADC初始化
    Epwm_System_PeripheralInit(); // PWM初始化 
    SCIA_System_PeripheralInit(); // 串口初始化
    Cla_System_PeripheralInit();  // Cla初始化
    Flash_PeripheralInit();       // Flash初始化
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void System_AppInit(void)
{
     Task_System_AppInit();      // 任务控制结构体初始化
     SCIA_System_AppInit();      // 串口MODBUS寄存器初始化
     Logic_System_AppInit();     // 错误状态与控制状态机初始化
     Version_System_AppInit();   // 版本号初始化
     Fault_System_AppInit();     // 错误检测时间初始化
     BuckBoost_System_AppInit(); // BuckBoost PWM初始化
     Derating_System_AppInit();  // 降载控制初始化
}

#endif
