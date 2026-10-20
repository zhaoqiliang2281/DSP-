/*
 * DCDC_Task.c
 *
 *  Created on: 2025年6月11日
 *      Author: ChenHF
 */

#ifndef TASK_C
#define TASK_C

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include "DCDC_GlobalHeadFile.h"
//-----------------------------------------------------------------------------
// Adding Constants and Macro Definitions
//-----------------------------------------------------------------------------


typedef struct
{
    Uint16 uwTimeCnt;
    Uint16 uwSetTimeCnt;
    Uint16 uwTaskInvokeFlag;
}Struct_TimerTaskBox;
enum
{
    TIMER_BASED_TASK_0,
    TIMER_BASED_TASK_1,
    TIMER_BASED_TASK_2,
    TIMER_BASED_TASK_3,
    TIMER_BASED_TASK_4,
    TIMER_BASED_TASK_5,
    TIMER_BASED_TASK_END
};


//-----------------------------------------------------------------------------
//Adding Private Function Prototypes (i.e. static)
//-----------------------------------------------------------------------------
static void Task_TimerBaseFunc_0(void);
static void Task_TimerBaseFunc_1(void);
static void Task_TimerBaseFunc_2(void);
static void Task_TimerBaseFunc_3(void);
static void Task_TimerBaseFunc_4(void);

//-----------------------------------------------------------------------------
//Adding Private Variables (i.e. static)
//-----------------------------------------------------------------------------
const Struct_TimerTaskBox cTaskBoxTable[TIMER_BASED_TASK_END] =
{
    {0, MS_BASE_CNT_CALC(1), FALSE},
    {0, MS_BASE_CNT_CALC(5), FALSE},
    {0, MS_BASE_CNT_CALC(20), FALSE},
    {0, MS_BASE_CNT_CALC(100), FALSE},
    {0, MS_BASE_CNT_CALC(1000), FALSE},
    {0, (unsigned long)2, FALSE}
};
Struct_TimerTaskBox g_strTaskBoxTable[TIMER_BASED_TASK_END];
/******************************************************************************
Function Name: Task_TimerBaseFunc_0
--------------------
Function Descriptions:1ms task
Parameter Name list:
ReturnType:
Refer documents:
 ******************************************************************************/
static void Task_TimerBaseFunc_0(void)
{
    ServiceDog();          // 喂狗
    Logic_Task_1ms();      // 状态机判断切换 
    BuckBoost_Task_1ms();  // 环路输入电压电流给定
    Adc_Task_1ms();

}
/******************************************************************************
Function Name: Task_TimerBaseFunc_1
--------------------
Function Descriptions:5ms task
Parameter Name list:
ReturnType:
Refer documents:
 ******************************************************************************/
static void Task_TimerBaseFunc_1(void)
{
    Adc_Task_5ms();       // 采样取平均值
    SCIA_Task_5ms();      // 串口处理
    Derating_Task_5ms();  // 降载处理 暂时未用
    canTask5ms();
}
/******************************************************************************
Function Name: Task_TimerBaseFunc_2
--------------------
Function Descriptions:20ms task
Parameter Name list:
ReturnType:
Refer documents: 20ms
 ******************************************************************************/
static void Task_TimerBaseFunc_2(void)
{
    Adc_Task_20ms();        // NTC采样
    SCIA_Task_20ms();       // 串口 加载串口数据到运行
							// 串口接收数据的处理
    Fault_Task_20ms();      // 控制开始检测过流、短路等  
    BuckBoost_Task_20ms();  // 更新PID参数，更新充放电电流大小
}
/******************************************************************************
Function Name: Task_TimerBaseFunc_3
--------------------
Function Descriptions:100ms task
Parameter Name list:
ReturnType:
Refer documents:
 ******************************************************************************/
static void Task_TimerBaseFunc_3(void)
{
    LED3_TOGGLE;
//    REDLED_TOGGLE;          // 控制LED
//     canTask100ms();

    ledCotrol100msTask();
    Flash_Task_100ms();
}
/******************************************************************************
Function Name: Task_TimerBaseFunc_4
--------------------
Function Descriptions:1s task
Parameter Name list:
ReturnType:
Refer documents:
 ******************************************************************************/
static void Task_TimerBaseFunc_4(void)
{
    Fault_Task_1s();       // 错误恢复
    canTask1s();
    RelayTask1S();
}

/******************************************************************************
Function Name: Task_TimerBaseFunc_5
--------------------
Function Descriptions:1s task
Parameter Name list:
ReturnType:
Refer documents:  100us
 ******************************************************************************/
static void Task_TimerBaseFunc_5(void)
{
//    canTask100us();
}

/******************************************************************************
Function Name: Task_Initialize
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
 ******************************************************************************/
void Task_System_AppInit(void)
{
    Uint16 index;

    for(index = 0; index < TIMER_BASED_TASK_END; index++)
    {
        g_strTaskBoxTable[index].uwTimeCnt = cTaskBoxTable[index].uwTimeCnt;
        g_strTaskBoxTable[index].uwSetTimeCnt = cTaskBoxTable[index].uwSetTimeCnt;
        g_strTaskBoxTable[index].uwTaskInvokeFlag = cTaskBoxTable[index].uwTaskInvokeFlag;
    }
}
/******************************************************************************
Function Name: Task_Task2_RealTimeDone
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
 ******************************************************************************/
#pragma CODE_SECTION(Task_Task2_RealTimeDone, "ramfuncs");
void Task_Task2_RealTimeDone(void)
{
    // 200us 任务触发
    canTask100us();
//    g_strTaskBoxTable[TIMER_BASED_TASK_5].uwTimeCnt++;
//    if(g_strTaskBoxTable[TIMER_BASED_TASK_5].uwTimeCnt >= g_strTaskBoxTable[TIMER_BASED_TASK_5].uwSetTimeCnt)
//    {
//        g_strTaskBoxTable[TIMER_BASED_TASK_5].uwTimeCnt = 0;
//        g_strTaskBoxTable[TIMER_BASED_TASK_5].uwTaskInvokeFlag = TRUE;
//    }

    // 1ms 任务触发
    g_strTaskBoxTable[TIMER_BASED_TASK_0].uwTimeCnt++;
    if(g_strTaskBoxTable[TIMER_BASED_TASK_0].uwTimeCnt >= g_strTaskBoxTable[TIMER_BASED_TASK_0].uwSetTimeCnt)
    {
        g_strTaskBoxTable[TIMER_BASED_TASK_0].uwTimeCnt = 0;
        g_strTaskBoxTable[TIMER_BASED_TASK_0].uwTaskInvokeFlag = TRUE;
    }
    // 5ms 任务触发
    g_strTaskBoxTable[TIMER_BASED_TASK_1].uwTimeCnt++;
    if(g_strTaskBoxTable[TIMER_BASED_TASK_1].uwTimeCnt >= g_strTaskBoxTable[TIMER_BASED_TASK_1].uwSetTimeCnt)
    {
        g_strTaskBoxTable[TIMER_BASED_TASK_1].uwTimeCnt = 0;
        g_strTaskBoxTable[TIMER_BASED_TASK_1].uwTaskInvokeFlag = TRUE;
    }
    // 20ms 任务触发
    g_strTaskBoxTable[TIMER_BASED_TASK_2].uwTimeCnt++;
    if(g_strTaskBoxTable[TIMER_BASED_TASK_2].uwTimeCnt >= g_strTaskBoxTable[TIMER_BASED_TASK_2].uwSetTimeCnt)
    {
        g_strTaskBoxTable[TIMER_BASED_TASK_2].uwTimeCnt = 0;
        g_strTaskBoxTable[TIMER_BASED_TASK_2].uwTaskInvokeFlag = TRUE;
    }
    // 100ms 任务触发
    g_strTaskBoxTable[TIMER_BASED_TASK_3].uwTimeCnt++;
    if(g_strTaskBoxTable[TIMER_BASED_TASK_3].uwTimeCnt >= g_strTaskBoxTable[TIMER_BASED_TASK_3].uwSetTimeCnt)
    {
        g_strTaskBoxTable[TIMER_BASED_TASK_3].uwTimeCnt = 0;
        g_strTaskBoxTable[TIMER_BASED_TASK_3].uwTaskInvokeFlag = TRUE;
    }
    // 1s 任务触发
    g_strTaskBoxTable[TIMER_BASED_TASK_4].uwTimeCnt++;
    if(g_strTaskBoxTable[TIMER_BASED_TASK_4].uwTimeCnt >= g_strTaskBoxTable[TIMER_BASED_TASK_4].uwSetTimeCnt)
    {
        g_strTaskBoxTable[TIMER_BASED_TASK_4].uwTimeCnt = 0;
        g_strTaskBoxTable[TIMER_BASED_TASK_4].uwTaskInvokeFlag = TRUE;
    }


}
/******************************************************************************
Function Name: TASK_InvokingProcess
--------------------
Function Descriptions:
Parameter Name list:
ReturnType:
Refer documents:
 ******************************************************************************/
void Task_InvokingProcess(void)
{
    if(TRUE == g_strTaskBoxTable[TIMER_BASED_TASK_5].uwTaskInvokeFlag) // 200uS 任务
    {
        g_strTaskBoxTable[TIMER_BASED_TASK_5].uwTaskInvokeFlag = FALSE;
        Task_TimerBaseFunc_5();
    }
    else if(TRUE == g_strTaskBoxTable[TIMER_BASED_TASK_0].uwTaskInvokeFlag) // 1ms 任务
    {
        g_strTaskBoxTable[TIMER_BASED_TASK_0].uwTaskInvokeFlag = FALSE;
        Task_TimerBaseFunc_0();
    }
    else if(TRUE == g_strTaskBoxTable[TIMER_BASED_TASK_1].uwTaskInvokeFlag) // 5ms 任务
    {
        g_strTaskBoxTable[TIMER_BASED_TASK_1].uwTaskInvokeFlag = FALSE;
        Task_TimerBaseFunc_1();
    }
    else if(TRUE == g_strTaskBoxTable[TIMER_BASED_TASK_2].uwTaskInvokeFlag)  // 20ms 任务
    {
        g_strTaskBoxTable[TIMER_BASED_TASK_2].uwTaskInvokeFlag = FALSE;
        Task_TimerBaseFunc_2();
    }
    else if(TRUE == g_strTaskBoxTable[TIMER_BASED_TASK_3].uwTaskInvokeFlag)   // 100ms 任务
    {
        g_strTaskBoxTable[TIMER_BASED_TASK_3].uwTaskInvokeFlag = FALSE;
        Task_TimerBaseFunc_3();
    }
    else if(TRUE == g_strTaskBoxTable[TIMER_BASED_TASK_4].uwTaskInvokeFlag)   // 1s 任务
    {
        g_strTaskBoxTable[TIMER_BASED_TASK_4].uwTaskInvokeFlag = FALSE;
        Task_TimerBaseFunc_4();

    }
}


#endif
