/*
 * DCDC_Relay.c
 *
 *  Created on: 2025年9月10日
 *      Author:
 */


#include "DCDC_GlobalHeadFile.h"


//#define SWID_OFFSET 0
#define SWID_OFFSET 0

U8_Union swID;

/******************************************************************************
Function Name:
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
******************************************************************************/
void RelayInit(void)
{
    R_CP_OFF;
    R_CN_OFF;
    R_PP_OFF;
    R_PN_OFF;
    R_CV_OFF;
    R_PV_OFF;
    R_USER_OFF;

//    R_CP_ON;
//    R_CN_ON;
//    R_PP_ON;
//    R_PN_ON;
    //R_CV_ON;
    R_PV_ON;
//    R_USER_ON;
}

void Relay_P_C_ON(void)
{
   R_CP_ON;
   R_CN_ON;
   R_PP_ON;
   R_PN_ON;
}

void Relay_P_C_OFF(void)
{
   R_CP_OFF;
   R_CN_OFF;
   R_PP_OFF;
   R_PN_OFF;
}

// 充电时
void Relay_Charge_ON(void)
{
   R_CP_ON;
   R_CN_ON;
   R_PV_OFF;
   R_CV_ON;
}
void Relay_Charge_OFF(void)
{
   R_CP_OFF;
   R_CN_OFF;
   R_CV_OFF;
   R_PV_ON;
}

// 放电时
void Relay_DisCharge_ON(void)
{
   R_PP_ON;
   R_PN_ON;
   R_CV_OFF;
   R_PV_ON;
}
void Relay_DisCharge_OFF(void)
{
   R_PP_OFF;
   R_PN_OFF;
   R_CV_OFF;
   R_PV_ON;  // 放电结束，继续采购
}


void SwInit(void)
{
    // SW1 GPIO11
    GpioCtrlRegs.GPAMUX1.bit.GPIO11 = 0; // GPIO功能
    GpioCtrlRegs.GPADIR.bit.GPIO11 = 0;  // 设为输入
    GpioCtrlRegs.GPAPUD.bit.GPIO11 = 1; // 由于外置了上拉电阻，禁用DSP内部上拉

    // SW2 GPIO5
    GpioCtrlRegs.GPAMUX1.bit.GPIO5 = 0; // GPIO功能
    GpioCtrlRegs.GPADIR.bit.GPIO5 = 0;  // 设为输入
    GpioCtrlRegs.GPAPUD.bit.GPIO5 = 1; // 由于外置了上拉电阻，禁用DSP内部上拉

    // SW3 GPIO4
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 0; // GPIO功能
    GpioCtrlRegs.GPADIR.bit.GPIO4 = 0;  // 设为输入
    GpioCtrlRegs.GPAPUD.bit.GPIO4 = 1; // 由于外置了上拉电阻，禁用DSP内部上拉

    // SW4 GPIO40
    GpioCtrlRegs.GPBMUX1.bit.GPIO40 = 0; // GPIO功能
    GpioCtrlRegs.GPBDIR.bit.GPIO40 = 0;  // 设为输入
    GpioCtrlRegs.GPBPUD.bit.GPIO40 = 1; // 由于外置了上拉电阻，禁用DSP内部上拉

    // SW5 GPIO10
    GpioCtrlRegs.GPAMUX1.bit.GPIO10 = 0; // GPIO功能
    GpioCtrlRegs.GPADIR.bit.GPIO10 = 0;  // 设为输入
    GpioCtrlRegs.GPAPUD.bit.GPIO10 = 1; // 由于外置了上拉电阻，禁用DSP内部上拉

}


void getSwID(void)
{
     swID.all=0x00;
     swID.bit.Bit4=SW5_Value;
     swID.bit.Bit3=SW4_Value;
     swID.bit.Bit2=SW3_Value;
     swID.bit.Bit1=SW2_Value;
     swID.bit.Bit0=SW1_Value;

//     swID.bit.Bit4=1;
//     swID.bit.Bit3=0;
//     swID.bit.Bit2=1;
//     swID.bit.Bit1=1;
//     swID.bit.Bit0=1;  // 48通道ID不对直接写死


     swID.all = swID.all + SWID_OFFSET; // ID加上偏移量

     canSetPra.swValue = swID.all+1;

     canSetPra.swRevID = (Uint32)((swID.all + 0x41)<<8);
     canSetPra.swSendID = (Uint32)((swID.all + 0x41)&0xff);

  //  canSetPra

}



void ledCotrol100msTask(void)
{
    static redLedCount=0;
    static greedLedCount=0;

    LED1_TOGGLE;

    // 绿灯控制 充电：常亮  放电：0.5S亮0.5S暗
    greedLedCount++;
    if(greedLedCount>=5)
    {
        greedLedCount=0;
        if(canSetPra.batWorkMode  ==  MODE_CHARGE)
                GREEDLED_ON;
        else if(canSetPra.batWorkMode  ==  MODE_DISCHARGE)
                GREEDLED_TOGGLE;          // 控制LED
        else
        {
            GREEDLED_OFF;
        }
    }

    // 红灯控制 慢闪：亮0.5S，暗1.5S为电源指示   急闪为报错：亮0.3S暗0.3S
    redLedCount++;
//    if(err==)
    if(redLedCount>=20)
    {
        redLedCount=0;
        REDLED_ON;
    }
    else
    {
        REDLED_OFF;
    }


}



/****************************************************************************
 * @brief： 搁置时处理继电器，以激活电池
 * @param：
 * @retval：
 * @note：
 *
 ****************************************************************************/
void RelayTask1S(void)
{
    static Uint16 count=0;

    if(canSetPra.batWorkMode  ==  MODE_SPACE)
    {
        count++;
//       if((count>4)&&(count<8)) // 导通3S
//       if(count>9) // 导通3S
//        if((count>9)&&(count<13)) // 导通3S
//        {
//            R_USER_ON;
//        }
//        else
       if(count>15)// 导通
        {
          if( g_strAdcData[eVbat].dRmsValBitFilter<18000)  // 电压低的时候才能开通继电器
              R_USER_ON;
        }
        else
        {
            R_USER_OFF;
        }
    }
    else
    {
        count=0;
        R_USER_OFF;
    }
}






