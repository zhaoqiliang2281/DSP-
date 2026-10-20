/*
 * DCDC_Version.c
 *
 *  Created on: 2025Äê6ÔÂ15ÈÕ
 *      Author: phone
 */
#ifndef VERSION_C
#define VERSION_C
//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"
//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------

Uint16 uwBuckBoostVersion;
Uint16 uwBuckBoostVersionDate;

/******************************************************************************
Function Name:
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
 ******************************************************************************/
void Version_System_AppInit(void)
{
    uwBuckBoostVersionDate = 615;
    uwBuckBoostVersion = 0x1;
}

#endif
