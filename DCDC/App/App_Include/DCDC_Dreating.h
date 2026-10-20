/*
 * DCDC_Derating.h
 *
 *  Created on: 2025Äê6ÔÂ13ÈÕ
 *      Author: phone
 */

#ifndef DCDC_DERATING_H_
#define DCDC_DERATING_H_
//-----------------------------------------------------------------------------
// Public Enumerated and Structure Definitions
//-----------------------------------------------------------------------------
#define RATED_POWER     3000//7Kw

typedef struct
{
    int32 dPowerLimintOut;
    int32 dMaxPowerLimint;
    int32 dBatPercentSetDerating;
    int32 dHighTempDerating;
    int32 dLowBatVoltDerating;

    Uint16 uwDeratingMode;

}Struct_DerateOrder;
extern Struct_DerateOrder g_strDerateOrder;

//-----------------------------------------------------------------------------
// Public Function Prototypes
//-----------------------------------------------------------------------------
extern void Derating_System_AppInit(void);
extern void Derating_Task_1ms(void);
extern void Derating_Task_5ms(void);

#endif /* APP_APP_INCLUDE_DCDC_DERATING_H_ */
