/*
 * DCDC_Logic.h
 *
 *  Created on: 2025Äê6ÔÂ20ÈÕ
 *      Author: phone
 */

#ifndef DCDC_LOGIC_H_
#define DCDC_LOGIC_H_

enum
{
    eSysInit,
    eSysStop,
    eSysStandby,
    eSysStart,
    eSysOperate,
    eSysFault,
    eSysEnd
};
//-----------------------------------------------------------------------------
// Public Enumerated and Structure Definitions
//-----------------------------------------------------------------------------
typedef union
{
    Uint16 all;
    struct
    {
       Uint16 bStopCond     :1;
       Uint16 bFaultCond    :1;
       Uint16 bSysStatus    :3;

       Uint16 bRsvd         :11;
    }bit;
}Union_Flag_SysLogic;
extern Union_Flag_SysLogic unSysLogicFlag;
static inline void Logic_SetSysStatus(Uint16 uwSetStatus_s)
{
    unSysLogicFlag.bit.bSysStatus = (uwSetStatus_s & 0x07);
}
static inline Uint16 Logic_GetSysStatus(void)
{
    return(unSysLogicFlag.bit.bSysStatus);
}

extern Uint16 uwBatWorkMode;
extern Uint16 uwDeviceWorkMode;
extern Uint16 uwStatusChangeDelayCnt;
extern Uint16 uwExitStopCondCnt;
//-----------------------------------------------------------------------------
// Public Function Prototypes
//-----------------------------------------------------------------------------
extern void Logic_System_AppInit(void);
extern void Logic_Task1_RealTimeDone(void);
extern void Logic_Task_1ms(void);

#endif /* DCDC_LOGIC_H_ */
