/*
 * DCDC_IO.h
 *
 *  Created on: 2025Äê6ÔÂ13ÈÕ
 *      Author: phone
 */

#ifndef DCDC_IO_H_
#define DCDC_IO_H_


//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------
#define PWM_ENABLE  1
#define PWM_DISABLE 0

#define LED1_TOGGLE  GpioDataRegs.GPBTOGGLE.bit.GPIO41 = 1
#define LED1_ON  GpioDataRegs.GPBCLEAR.bit.GPIO41 =1
#define LED1_OFF GpioDataRegs.GPBSET.bit.GPIO41 =1

#define LED2_TOGGLE  GpioDataRegs.GPBTOGGLE.bit.GPIO34 = 1
#define LED2_ON  GpioDataRegs.GPBCLEAR.bit.GPIO34 =1
#define LED2_OFF GpioDataRegs.GPBSET.bit.GPIO34 =1

#define LED3_TOGGLE  GpioDataRegs.GPBTOGGLE.bit.GPIO43 = 1
#define LED3_ON  GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1
#define LED3_OFF GpioDataRegs.GPBSET.bit.GPIO43 = 1

#define GREEDLED_TOGGLE  GpioDataRegs.GPATOGGLE.bit.GPIO27 = 1
#define GREEDLED_ON  GpioDataRegs.GPACLEAR.bit.GPIO27 = 1
#define GREEDLED_OFF GpioDataRegs.GPASET.bit.GPIO27 = 1

#define REDLED_TOGGLE  GpioDataRegs.GPATOGGLE.bit.GPIO26 = 1
#define REDLED_ON  GpioDataRegs.GPACLEAR.bit.GPIO26 = 1
#define REDLED_OFF GpioDataRegs.GPASET.bit.GPIO26 = 1

#define IOCP  GpioDataRegs.GPADAT.bit.GPIO15

//-----------------------------------------------------------------------------
// Public Function Prototypes
//-----------------------------------------------------------------------------
extern void IO_System_PinInit(void);
extern void IO_Task1_RealTimeDone(void);
extern void IO_Task_1ms(void);
extern void IO_Task_5ms(void);



#endif /* DCDC_IO_H_ */
