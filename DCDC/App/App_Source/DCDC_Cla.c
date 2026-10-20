/*
 * DCDC_Cla.c
 *
 *  Created on: 2025年6月15日
 *      Author: phone
 */


#ifndef CLA_C
#define CLA_C

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"
//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------

/******************************************************************************
Function Name:
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Cla_System_PeripheralInit(void)
{
    EALLOW;
    PieVectTable.CLA1_INT1 = &CLA1_INT1_ISR;    //cla中断向量定义
    PieVectTable.CLA1_INT2 = &CLA1_INT2_ISR;
    PieVectTable.CLA1_INT3 = &CLA1_INT3_ISR;
    PieVectTable.CLA1_INT4 = &CLA1_INT4_ISR;
    PieVectTable.CLA1_INT5 = &CLA1_INT5_ISR;
    PieVectTable.CLA1_INT6 = &CLA1_INT6_ISR;
    PieVectTable.CLA1_INT7 = &CLA1_INT7_ISR;
    PieVectTable.CLA1_INT8 = &CLA1_INT8_ISR;
    EDIS;

    EALLOW;
    Cla1Regs.MVECT1 = ((Uint16)Cla1Task1 - (Uint16)&Cla1Prog_Start);//事件任务定义
    Cla1Regs.MVECT2 = ((Uint16)Cla1Task2 - (Uint16)&Cla1Prog_Start);
    Cla1Regs.MVECT3 = ((Uint16)Cla1Task3 - (Uint16)&Cla1Prog_Start);
    Cla1Regs.MVECT4 = ((Uint16)Cla1Task4 - (Uint16)&Cla1Prog_Start);
    Cla1Regs.MVECT5 = ((Uint16)Cla1Task5 - (Uint16)&Cla1Prog_Start);
    Cla1Regs.MVECT6 = ((Uint16)Cla1Task6 - (Uint16)&Cla1Prog_Start);
    Cla1Regs.MVECT7 = ((Uint16)Cla1Task7 - (Uint16)&Cla1Prog_Start);
    Cla1Regs.MVECT8 = ((Uint16)Cla1Task8 - (Uint16)&Cla1Prog_Start);

    //CLA内存和clamath库分配内存。
    MemCopy(&Cla1funcsLoadStart,&Cla1funcsLoadEnd,&Cla1funcsRunStart);
    MemCopy(&Cla1mathTablesLoadStart,&Cla1mathTablesLoadEnd,&Cla1mathTablesRunStart);

     asm("   RPT #4 || NOP");

     Cla1Regs.MMEMCFG.bit.PROGE = 1;     // Configure the RAM as CLA program memory
     Cla1Regs.MCTL.bit.IACKE = 1;
     Cla1Regs.MMEMCFG.bit.RAM0E = 1;     // configure RAM L1, F28035 as CLA Data memory 0
//     Cla1Regs.MMEMCFG.bit.RAM1E = 1;     // Configure RAM L2, F28035 as CLA data memory 1
     Cla1Regs.MMEMCFG.bit.RAM1E = 0;     // Configure RAM L2, F28035 as CLA data memory 1
     // Enable the IACK instruction to start a task
     // Enable the CLA interrupt 8 and interrupt 2
     asm("   RPT #4 || NOP");

//     Cla1Regs.MIER.all =  M_INT8;
//     // No need to wait, the task will finish by the time
//     // we configure the ePWM and ADC modules
//     //手动触发一次任务8
//     Cla1ForceTask8();

     EDIS;


     // Set Up CLA Task
     // Task 1 has the option to be started by either EPWM1_INT or ADCINT1
     // In this case we will allow EPMWM1INT to start CLA Task 1
     EALLOW;
     Cla1Regs.MPISRCSEL1.bit.PERINT1SEL = CLA_INT1_NONE;//CLA_INT1_ADCINT1 ;     // 0=ADCINT1    1=none    2=EPWM1INT
     Cla1Regs.MPISRCSEL1.bit.PERINT2SEL = CLA_INT2_NONE;
     Cla1Regs.MPISRCSEL1.bit.PERINT3SEL = CLA_INT3_NONE;
     Cla1Regs.MPISRCSEL1.bit.PERINT4SEL = CLA_INT4_NONE;
     Cla1Regs.MPISRCSEL1.bit.PERINT5SEL = CLA_INT5_NONE;
     Cla1Regs.MPISRCSEL1.bit.PERINT6SEL = CLA_INT6_NONE;
     Cla1Regs.MPISRCSEL1.bit.PERINT7SEL = CLA_INT7_NONE;
     Cla1Regs.MPISRCSEL1.bit.PERINT8SEL = CLA_INT8_NONE;
     // Configure the interrupt that woud occur each control cycles

     Cla1Regs.MIER.all =  M_INT1|M_INT2|M_INT8;
     asm("   RPT #4 || NOP");

     /* Enable CLA interrupts at the group and subgroup levels */
//     PieCtrlRegs.PIEIER11.bit.INTx1 = 1;
//     IER |= (M_INT11 );

     EDIS;
}

#endif
