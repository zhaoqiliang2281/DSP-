/*
 * DCDC_Common.h
 *
 *  Created on: 2025Äê6ÔÂ13ÈÕ
 *      Author: phone
 */

#ifndef DCDC_COMMON_H_
#define DCDC_COMMON_H_

//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------

typedef enum
{
    eBatWorkModeInit,
    eDCDCStop = eBatWorkModeInit,
    eCharge,
    eDisCharge,
    eBatWorkModeEnd
}Bat_Mode;

typedef enum
{
    eNormalRunning,
    eOpenLoop,
    eBenchTest

}Device_Work_Mode;

#define TRUE    1
#define FALSE   0

#define ENABLE    1
#define DISABLE   0

#define ON    1
#define OFF   0

#define FINISHED     1
#define UNFINISHED   0

#define TIME_BASE   100//100us
#define TSAK_TICK   (1000/TIME_BASE)


#define MS_BASE_CNT_CALC(x) (unsigned long)(x*TSAK_TICK)

#define UPDNLMT(Val,Max,Min)    {Val = (((Val) >= (Max)) ? (Max) : (Val));Val = ((Val) <= (Min)) ? (Min) : (Val);}
#define UPLMT(Val,Max)  {Val = (((Val) >= (Max)) ? (Max) : (Val));}
#define DNLMT(Val,Min)  {Val = ((Val) <= (Min)) ? (MIn) : (Val);}

#define  CMPMAX2(a,b) ((a >= b)? a:b)
#define  CMPMIN2(a,b) ((a <= b)? a:b)

#define  CMPMAX3(a,b,c) ((CMPMAX2(a,b) >= c)? CMPMAX2(a,b):c)
#define  CMPMIN3(a,b,c) ((CMPMIN2(a,b) <= c)? CMPMIN2(a,b):c)

#define TSK30MS_TIMER50US          600
#define TSK500MS_TIMER50US         10000

#define TSK200US_TIMER100US       2
#define TSK5S_TIMER100US          50000

#define TSK20MS_TIMER1MS          20
#define TSK100MS_TIMER1MS         100
#define TSK500MS_TIMER1MS         500
#define TSK500MS_TIMER1MS         500
#define TSK1S_TIMER1MS            1000

#define TSK20MS_TIMER5MS          4
#define TSK100MS_TIMER5MS         20
#define TSK500MS_TIMER5MS         100
#define TSK1S_TIMER5MS            200

#define TSK20MS_TIMER20MS          1
#define TSK100MS_TIMER20MS         5
#define TSK500MS_TIMER20MS         25
#define TSK1S_TIMER20MS            50

#define TSK1S_TIMER100MS          10
#define TSK5S_TIMER100MS          50

#define TSK1S_TIMER1S          1
#define TSK10S_TIMER1S         10
#define TSK30S_TIMER1S         30
#define TSK1MIN_TIMER1S        60
#define TSK2MIN_TIMER1S        120
#define TSK500S_TIMER1S        500

#define Vbat_Set(Bat_Volt)      ((int32)((float32)Bat_Volt*1000.0))//V
#define Ibat_Set(Bat_Curr)      ((int32)((float32)Bat_Curr*1000.0))//A
#define Pbat_Set(Bat_Power)     ((int32)((float32)Bat_Power*1000.0))//W

//-----------------------------------------------------------------------------
//Adding Private Function Prototypes (i.e. static)
//-----------------------------------------------------------------------------
typedef struct
{
    float32 fKp;
    float32 fKi;

    float32 fErr;
    float32 fErrOld;
    float32 fRef;
    float32 fFdb;

    float32 fLoopOut;
    float32 fLoopOutOld;
    float32 fFdFControl;

    float32 fPIMax;
    float32 fPIMin;
}Struct_fIncPI;
//-----------------------------------------------------------------------------
inline float m_fIncPID(Struct_fIncPI *Controller_s)
{
    float32 fResult, fLoopOutVariable;

    fLoopOutVariable = Controller_s->fKi * Controller_s->fErr + Controller_s->fKp * (Controller_s->fErr - Controller_s->fErrOld);
//    fLoopOutVariable += Controller_s->fFdFControl;
    Controller_s->fErrOld = Controller_s->fErr;
    fResult = Controller_s->fLoopOutOld + fLoopOutVariable;

    if(fResult >= Controller_s->fPIMax)
    {
        fResult = Controller_s->fPIMax;
    }

    if(fResult <= Controller_s->fPIMin)
    {
        fResult = Controller_s->fPIMin;
    }

    Controller_s->fLoopOutOld = fResult;

    return (fResult);
}

#endif /* DCDC_COMMON_H_ */
