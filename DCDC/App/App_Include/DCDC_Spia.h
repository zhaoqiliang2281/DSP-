/*
 * DCDC_Spia.h
 *
 *  Created on: 2025Äê7ÔÂ8ÈÕ
 *      Author: phone
 */

#ifndef DCDC_SPIA_H_
#define DCDC_SPIA_H_







//-----------------------------------------------------------------------------
// Public Function Prototypes
//-----------------------------------------------------------------------------
extern void SPIA_System_PinInit(void);
extern void SPIA_System_PeripheralInit(void);
extern void SPIA_System_AppInit(void);
extern void SPIA_Task1_RealTimeDone(void);
extern void SPIA_Task_1ms(void);
extern void SPIA_Task_20ms(void);

#endif /* DCDC_SPIA_H_ */
