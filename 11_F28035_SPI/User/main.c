#include "main.h"

uint32_t num = 0;


/**
 * main.c - 纯 RAM 调试版
 * 工程链接文件使用 28035_RAM_lnk.cmd，程序全部在 SARAM 中运行，
 * 无需 Flash 拷贝，无需 InitFlash。
 */

/* 声明 RAM 函数段的起始地址、大小和运行时地址 */
extern Uint16 RamfuncsLoadStart;
extern Uint16 RamfuncsLoadSize;
extern Uint16 RamfuncsRunStart;

//static uint16_t comper1 = 0;
//static uint16_t comper3 = 0;

/* 主函数入口，程序从这里开始执行 */
void main(void)
{
    /* 初始化系统控制模块，包括时钟、看门狗等基础配置 */
    InitSysCtrl();

    /* 将加载到 Flash 中的 RAM 函数拷贝到 RAM 中，以便在 DSP 上执行 */
    memcpy((uint16_t *)&RamfuncsRunStart, (uint16_t *)&RamfuncsLoadStart, (unsigned long)&RamfuncsLoadSize);

    /* 初始化 Flash 控制器，确保程序执行和数据存储正常 */
    InitFlash();

    /* 禁止全局中断，防止初始化过程中被其他中断打断 */
    DINT;

    /* 允许对受保护寄存器进行写操作 */
    EALLOW;

        /* 初始化 PIE 控制器，用于配置中断控制器 */
        InitPieCtrl();

        /* 清除中断使能寄存器，关闭所有中断 */
        IER = 0x0000;

        /* 清除中断标志寄存器，清空待处理中断 */
        IFR = 0x0000;

        /* 初始化 PIE 向量表，建立中断入口映射 */
        InitPieVectTable();

    /* 结束对受保护寄存器的写操作 */
    EDIS;

    /* 系统初始化 GPIO 模块 */
    InitGpio();

    LED_Init();     //初始化LED
    OLED_Init();    //初始化OLED
    ips200_init();  //初始化ips屏幕

    /* 整屏刷成黑色 */
    ips200_full(RGB565_BLACK);

    /* 黑底白字显示文字（字色和底色不能用同一种，否则看不见） */
    ips200_set_color(RGB565_WHITE, RGB565_BLACK);
    ips200_show_string(10, 10, "TMS320F28035");
    ips200_show_string(10, 30, "IPS200 240x320");
//  ips200_set_color(RGB565_YELLOW, RGB565_BLUE);
    ips200_show_string(10, 50, "Hello IPS200!");
//    ips200_show_string(10, 90, "num =");

    /* 画一个红色边框，验证画线功能 */
    ips200_draw_line(0,   0,   239, 0,   RGB565_RED);
    ips200_draw_line(0,   0,   0,   319, RGB565_RED);
    ips200_draw_line(239, 0,   239, 319, RGB565_RED);
    ips200_draw_line(0,   319, 239, 319, RGB565_RED);

//    KEY_Init();             //初始化按键
//    EPwm1_Init();
//    EPwm3_Init();         //初始化EPWM
//    ADC_Init();           //初始化ADC
//    InitSciA();           //初始化SCI-A
//    COMP2_Simple_Init();  //初始化比较器
    Timer0_Init();          //初始化定时器
    Timer1_Init();
    Timer2_Init();

//    Watchdog_Init();        //初始化看门狗

//    Gpio_Exti();          //开启外部中断

    /* 使能全局中断 */
    EINT;      /* 等价于 asm(" CLRC INTM")，允许可屏蔽中断 */
    ERTM;      /* 允许实时调试中断 DBGM，纯运行时也可加 */

    /* 无限循环，程序持续运行 */
    while(1)
    {
//        Gpio_Exti_Loop();
        num++;
        OLED_ShowNum(1,1,num,5);
//        if(key1_flag == 1)
//        {
//
//            key1_flag=0;
//
//        }
//        if(key2_flag == 1)
//        {
//
//            key2_flag=0;
//
//        }
//        if(key3_flag == 1)
//        {
//            key3_flag=0;
//
//        }
//        if(key4_flag == 1)
//        {
//            key4_flag=0;
//
//        }
//        if(GpioDataRegs.GPADAT.bit.GPIO8 == 1)
//        {
//
//        }

//        LoopCount++;

        ips200_show_int(60, 90,  num, 6);
        ips200_show_int(60, 120, timer0_count, 8);
        ips200_show_int(60, 140, timer1_count, 8);
        ips200_show_int(60, 160, timer2_count, 8);

//        ips200_show_int(60, 200, LoopCount, 8);
//        ips200_show_int(60, 220, WakeCount, 8);

    }
}

