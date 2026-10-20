/*
 * DCDC_Epwm.c
 *
 *  Created on: 2025年6月11日
 *      Author: ChenHF
 */

#ifndef EPWM_C
#define EPWM_C

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"
//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------
Uint16 status;
Uint16 MEP_ScaleFactor;
const volatile struct EPWM_REGS *ePWM[PWM_CH] = {&EPwm1Regs, &EPwm1Regs, &EPwm2Regs};
//-----------------------------------------------------------------------------
//Adding Private Function
//-----------------------------------------------------------------------------
void Epwm_InitPWM1(void);
void Epwm_InitPWM2(void);
void Epwm_InitPWM3(void);
void Epwm_PinInit(void);

/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Epwm_System_PeripheralInit(void)
{
    Epwm_InitPWM1();  // PWM1A为BUCK PWM  PWM1B为BOOST PWM 
    Epwm_InitPWM2();  // PWM2A为风扇控制
    Epwm_InitPWM3();  // ADC采样触发20us
    Epwm_PinInit();   // IO绑定
}

/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Epwm_InitPWM1(void)
{
    EALLOW;
    // EPWM Module 1 config
    EPwm1Regs.TBPRD = BUCKBOOST_PWM_TIMER_TBPRD; // Period  TBCLK counts
    EPwm1Regs.TBPHS.half.TBPHS = 0; // Set Phase register to zero
    EPwm1Regs.TBCTR = 0;
    EPwm1Regs.CMPA.half.CMPA = 0;
    EPwm1Regs.CMPB = 0;//BUCKBOOST_PWM_TIMER_TBPRD;

    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm1Regs.TBCTL.bit.PHSEN = TB_ENABLE;
    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;
    EPwm1Regs.TBCTL.bit.PHSDIR = TB_UP;
    EPwm1Regs.TBCTL.bit.PRDLD = TB_SHADOW;
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;
    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO_PRD;

    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm1Regs.AQCTLA.bit.CAD = AQ_SET;
//    EPwm1Regs.AQCTLB.bit.CAU = AQ_CLEAR;
//    EPwm1Regs.AQCTLB.bit.CAD = AQ_SET;
    EPwm1Regs.AQCTLB.bit.CBU = AQ_CLEAR;
    EPwm1Regs.AQCTLB.bit.CBD = AQ_SET;
    EPwm1Regs.AQCSFRC.bit.CSFA = 1;                 //force low
    EPwm1Regs.AQCSFRC.bit.CSFB = 1;//2;                 //force low
    EPwm1Regs.AQSFRC.bit.RLDCSF = 0x3;              //Load AQCSFRC immediately.

    //死区模块使用CMPA比较值发互补的波形
//    EPwm1Regs.DBCTL.bit.IN_MODE  = DBA_RED_DBB_FED;
//    EPwm1Regs.DBCTL.bit.POLSEL   = DB_ACTV_HIC;
//    EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
//    EPwm1Regs.DBRED = DEADTIME;
//    EPwm1Regs.DBFED = DEADTIME;
    EPwm1Regs.DBCTL.bit.OUT_MODE = DB_DISABLE;//死区模块关闭

    //故障信号处理TZ设置
    /**********************************cycle by cycle**************************************/
    /*****一旦触发立即装载TZCTL中TZA,TZB的配置的PWM状态（高，低，高阻，或者不变）**********/
    /***每次PWM计数器为0时,检测TZ引脚的输入信号是否仍然被触发，如果仍然被触发，就继续保护**/
    /*******************如果TZ引脚的输入不再被触发，就恢复PWM输出**************************/
    /****Enable TZ1 as cycle-by-cycle trip source for this ePWM module*********************/
    EPwm1Regs.TZSEL.bit.CBC1 = TZ_ENABLE;//待确认
    EPwm1Regs.TZCTL.bit.TZA = TZ_FORCE_LO;          //Force EPWMxA to a low state
    EPwm1Regs.TZCTL.bit.TZB = TZ_FORCE_LO;          //Force EPWMxB to a low state
    EPwm1Regs.TZFRC.bit.CBC = 0;
    EPwm1Regs.TZEINT.bit.CBC = 0;                           //Disable cycle by cycle interrupt generation

    /**********************************One-shot**************************************/
    /****************总的来说区别就是，一旦触发就无法自动恢复************************/
    /****************会始终处于保护状态，直到手动清除TZFLG中的OST位******************/
    /****************就是必须在TZCLR中给OST位赋值1，才能再次输出PWM波形**************/
    /****Disable TZ1 as one-shot trip source for this ePWM module********************/
    //EPwm1Regs.TZSEL.bit.OSHT1 = TZ_ENABLE;
    //EPwm1Regs.TZCTL.bit.TZA = TZ_FORCE_LO;        //Force EPWMxA to a low state
    //EPwm1Regs.TZCTL.bit.TZB = TZ_FORCE_LO;        //Force EPWMxB to a low state
    EPwm1Regs.TZFRC.bit.OST = 1;
    EPwm1Regs.TZEINT.bit.OST = 0;                            //Disable one-shot interrupt generation
    //HRPWM配置
    EPwm1Regs.HRCNFG.all = 0x0;
    EPwm1Regs.HRCNFG.bit.EDGMODE = HR_FEP;  // MEP control on falling edge
    EPwm1Regs.HRCNFG.bit.CTLMODE = HR_CMP;
    EPwm1Regs.HRCNFG.bit.HRLOAD  = HR_CTR_ZERO_PRD;
    EPwm1Regs.HRCNFG.bit.AUTOCONV = 1;     // Enable auto-conversion logic
    EPwm1Regs.HRPCTL.bit.HRPE = 0;         // Turn off high-resolution period control.
    EDIS;
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Epwm_InitPWM2(void)
{
    EALLOW;
    // EPWM Module 2 config
    EPwm2Regs.TBPRD = FAN_TBPRD; // Period  TBCLK counts
    EPwm2Regs.TBPHS.half.TBPHS = 0; // Set Phase register to zero
    EPwm2Regs.TBCTR = 0;
    EPwm2Regs.CMPA.half.CMPA = 0;
    EPwm2Regs.CMPB = 0;//BUCKBOOST_PWM_TIMER_TBPRD;

    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm2Regs.TBCTL.bit.PHSEN = TB_ENABLE;
    EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;
   // EPwm2Regs.TBCTL.bit.PHSDIR = TB_UP;
  //  EPwm2Regs.TBCTL.bit.PRDLD = TB_SHADOW;
    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    EPwm2Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm2Regs.AQCTLA.bit.CAD = AQ_CLEAR;
 //   EPwm2Regs.AQCTLB.bit.CBU = AQ_CLEAR;
 //   EPwm2Regs.AQCTLB.bit.CBD = AQ_SET;
 //   EPwm2Regs.AQCSFRC.bit.CSFA = 1;                 //force low
  //  EPwm2Regs.AQCSFRC.bit.CSFB = 1;                 //force low
    EPwm2Regs.AQSFRC.bit.RLDCSF = 0x3;              //Load AQCSFRC immediately.

    EPwm2Regs.DBCTL.bit.OUT_MODE = DB_DISABLE;//死区模块关闭
   // EPwm2Regs.TZFRC.bit.OST = 1;
  //  EPwm2Regs.TZEINT.bit.OST = 0;                            //Disable one-shot interrupt generation

	//EPwm2Regs.TZCTL.bit.TZA = TZ_FORCE_LO;          //Force EPWMxA to a low state
  //  EPwm2Regs.TZCTL.bit.TZB = TZ_FORCE_LO;          //Force EPWMxB to a low state
    EDIS;
}
//******************************************************************************
void Epwm_InitPWM3(void)
{
    EALLOW;
    // EPWM Module 3 config
    EPwm3Regs.TBPRD = ADC_PWM_TIMER_TBPRD;  // Period  TBCLK counts
    EPwm3Regs.TBPHS.half.TBPHS = 0;                     // Set Phase register to zero
    EPwm3Regs.TBCTR = 0;
    EPwm3Regs.CMPA.half.CMPA = 0;

    EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm3Regs.TBCTL.bit.PHSEN = TB_ENABLE;//TB_DISABLE;

//    EPwm3Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;//TB_SYNC_IN;//TB_CTR_ZERO;
    EPwm3Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;//TB_SYNC_IN;//TB_CTR_ZERO;

    EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm3Regs.TBCTL.bit.CLKDIV = TB_DIV1;
    EPwm3Regs.TBCTL.bit.PHSDIR = TB_UP;
    EPwm3Regs.TBCTL.bit.PRDLD = TB_SHADOW;
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO_PRD;
    EPwm3Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO_PRD;

    EPwm3Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm3Regs.AQCTLA.bit.CAD = AQ_SET;
    EPwm3Regs.AQCSFRC.bit.CSFA = 1;                 //force low
    EPwm3Regs.AQCSFRC.bit.CSFB = 1;                 //force low
    EPwm3Regs.AQSFRC.bit.RLDCSF = 0x3;            //Load AQCSFRC immediately.

    // ADC 采样触发设置
    EPwm3Regs.ETSEL.bit.SOCAEN = 1; // able SOCA
    EPwm3Regs.ETSEL.bit.SOCASEL = ET_CTR_PRD;//ET_CTR_ZERO; // Select SOC on ET_CTR_PRD
    EPwm3Regs.ETPS.bit.SOCAPRD = 2;//1; // Generate pulse on 1st event 40K/2
    EPwm3Regs.ETCLR.bit.SOCA = 0x1;
    EPwm3Regs.ETSEL.bit.INTSEL = ET_CTR_PRDZERO;    // Select INT on period event
    EPwm3Regs.ETSEL.bit.INTEN = 0;                      // Disable EPWM INT
    EPwm5Regs.ETPS.bit.INTPRD = ET_1ST;             // Generate INT on first event
    EDIS;
}
/******************************************************************************
Function Name:
-------------------------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void Epwm_PinInit(void)
{
    EALLOW;

    //Epwm1
    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 1;    // Disable pull-up on GPIO0 (EPWM1A)
    GpioCtrlRegs.GPAPUD.bit.GPIO1 = 1;    // Disable pull-up on GPIO1 (EPWM1B)
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;   // Configure GPIO0 as EPWM1A
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 1;   // Configure GPIO1 as EPWM1B

    //Epwm2
    GpioCtrlRegs.GPAPUD.bit.GPIO2 = 1;    // Disable pull-up on GPIO0 (EPWM2A)
    //GpioCtrlRegs.GPAPUD.bit.GPIO3 = 1;    // Disable pull-up on GPIO1 (EPWM2B)
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;   // Configure GPIO0 as EPWM2A
    //GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 1;   // Configure GPIO1 as EPWM2B

    //Epwm3
    //GpioCtrlRegs.GPAPUD.bit.GPIO4 = 1;    // Disable pull-up on GPIO8 (EPWM3A)
    //GpioCtrlRegs.GPAPUD.bit.GPIO5 = 1;    // Disable pull-up on GPIO9 (EPWM3B)
    //GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;   // Configure GPIO8 as EPWM3A
    //GpioCtrlRegs.GPAMUX1.bit.GPIO5 = 1;   // Configure GPIO9 as EPWM3B

    EDIS;
}


/*---------------------------------------------------------------------------
 * @brief： FansControl
 * @param：
 * @retval：
 * @note：
 *			控制风扇
 ---------------------------------------------------------------------------*/
void FansControl(Uint16 buckT,Uint16 boostT)
{
	Uint16 temp;
	Uint16 suwBuckDutyTmp;

	if(buckT>boostT)
		temp=buckT;
	else
		temp=boostT;

	if(temp<35) // 温度太低不开启风扇  缓慢开启风扇以观察风扇有无问题
		{
		suwBuckDutyTmp=20;
	}
	else
		{
		suwBuckDutyTmp = (temp - 35)*10+250; // 85度开满
	}

	if(suwBuckDutyTmp>FAN_TBPRD)
	suwBuckDutyTmp = FAN_TBPRD;
	
 	EPwm2Regs.CMPA.half.CMPA = suwBuckDutyTmp;
 	
 	/* 最大温度报警 */
    if(temp == MAX_MOS_T)
    {
        upAlarm.bit.bIGBTtempover = ERR;
    }
    if(temp == NO_MOS_NTC)
    {
        upAlarm.bit.bBatLowTemp = ERR;
    }

    else
    {
        upAlarm.bit.bIGBTtempover = OK;
        upAlarm.bit.bBatLowTemp = OK;
    }


}



#endif
