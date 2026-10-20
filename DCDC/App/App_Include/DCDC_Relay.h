/*
 * DCDC_Relay.h
 *
 *  Created on: 2025Äê9ÔÂ10ÈÕ
 *      Author: phone
 */

#ifndef DCDC_Relay_H_
#define DCDC_Relay_H_


    #define SW1_Value  GpioDataRegs.GPADAT.bit.GPIO11
    #define SW2_Value  GpioDataRegs.GPADAT.bit.GPIO5
    #define SW3_Value  GpioDataRegs.GPADAT.bit.GPIO4
    #define SW4_Value  GpioDataRegs.GPBDAT.bit.GPIO40
    #define SW5_Value  GpioDataRegs.GPADAT.bit.GPIO10

struct U8_BITS {          // bits   description
    Uint16 Bit0:1;            // 0
    Uint16 Bit1:1;            // 1
    Uint16 Bit2:1;            // 2
    Uint16 Bit3:1;            // 3
    Uint16 Bit4:1;            // 4
    Uint16 Bit5:1;            // 5
    Uint16 Bit6:1;            // 6
    Uint16 Bit7:1;            // 7
};

typedef union  u8u{
    Uint8           all;
    struct U8_BITS  bit;
}U8_Union;

extern U8_Union swID;


// GPA (GPIO0 to 31)
// GPB (GPIO32 to 44)

/***RELAY C+***/
#define R_CP_ON  GpioDataRegs.GPASET.bit.GPIO3 = 1
#define R_CP_OFF GpioDataRegs.GPACLEAR.bit.GPIO3 = 1

/***RELAY C-***/
#define R_CN_ON  GpioDataRegs.GPASET.bit.GPIO14 = 1
#define R_CN_OFF GpioDataRegs.GPACLEAR.bit.GPIO14 = 1 

/***RELAY P+***/
#define R_PP_ON  GpioDataRegs.GPASET.bit.GPIO13 = 1
#define R_PP_OFF GpioDataRegs.GPACLEAR.bit.GPIO13 = 1

/***RELAY P-***/
#define R_PN_ON  GpioDataRegs.GPASET.bit.GPIO20 = 1
#define R_PN_OFF GpioDataRegs.GPACLEAR.bit.GPIO20 = 1 
 
/***RELAY CV***/
#define R_CV_ON  GpioDataRegs.GPASET.bit.GPIO12 = 1
#define R_CV_OFF GpioDataRegs.GPACLEAR.bit.GPIO12 = 1

/***RELAY PV***/
#define R_PV_ON  GpioDataRegs.GPASET.bit.GPIO25 = 1
#define R_PV_OFF GpioDataRegs.GPACLEAR.bit.GPIO25 = 1
 
/***RELAY USER***/
#define R_USER_ON  GpioDataRegs.GPASET.bit.GPIO9 = 1
#define R_USER_OFF GpioDataRegs.GPACLEAR.bit.GPIO9 = 1
 
extern void RelayInit(void);
extern void Relay_P_C_ON(void);
extern void Relay_P_C_OFF(void);

extern void Relay_Charge_ON(void);
extern void Relay_Charge_OFF(void);
extern void Relay_DisCharge_ON(void);
extern void Relay_DisCharge_OFF(void);

extern void SwInit(void);
extern void ledCotrol100msTask(void);

extern void RelayTask1S(void);


#endif /* DCDC_Relay_H_ */
