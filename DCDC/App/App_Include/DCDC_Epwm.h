/*
 * DCDC_Epwm.h
 *
 *  Created on: 2025年6月13日
 *      Author: phone
 */

#ifndef DCDC_EPWM_H_
#define DCDC_EPWM_H_


//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------
#define ADC_PWM_TIMER_TBPRD         750//40K
#define BUCKBOOST_PWM_TIMER_TBPRD   750//40K
#define FAN_TBPRD                   750//40K


#define DEADTIME    60//死区时间待确认








//-----------------------------------------------------------------------------
// Public Function Prototypes
//-----------------------------------------------------------------------------
extern void Epwm_System_PeripheralInit(void);

extern void FansControl(Uint16 buckT,Uint16 boostT);



#endif /* DCDC_EPWM_H_ */
