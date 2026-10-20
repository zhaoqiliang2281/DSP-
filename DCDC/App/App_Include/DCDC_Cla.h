/*
 * DCDC_Cla.h
 *
 *  Created on: 2025Äê6ÔÂ15ÈÕ
 *      Author: phone
 */

#ifndef DCDC_CLA_H_
#define DCDC_CLA_H_

#include <stdint.h>
#include "DSP2803x_Cla_defines.h"
#include "DSP2803x_Cla_typedefs.h"

//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------

//
// The following are symbols defined in the CLA assembly code
// Including them in the shared header file makes them
// .global and the main CPU can make use of them.
//
extern __interrupt void Cla1Task1(void);
extern __interrupt void Cla1Task2(void);
extern __interrupt void Cla1Task3(void);
extern __interrupt void Cla1Task4(void);
extern __interrupt void Cla1Task5(void);
extern __interrupt void Cla1Task6(void);
extern __interrupt void Cla1Task7(void);
extern __interrupt void Cla1Task8(void);
//-----------------------------------------------------------------------------
// Public Function Prototypes
//-----------------------------------------------------------------------------
extern void Cla_System_PeripheralInit(void);
//------------------------------------------------------------------------------

#endif /* DCDC_CLA_H_ */
