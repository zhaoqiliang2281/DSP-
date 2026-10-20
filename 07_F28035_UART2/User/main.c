#include "main.h"
#include "vofa.h"

uint32_t num = 0;


/**
 * main.c - RAM 调试程序入口
 * 本工程使用 28035_RAM_lnk.cmd，将程序放入片上 SARAM 运行。
 * 若改为 Flash 运行，需要将 Flash 中的 RAM 函数复制到 RAM。
 */

/* RAM 函数在 Flash 中的加载地址、长度和 RAM 中的运行地址。 */
extern Uint16 RamfuncsLoadStart;
extern Uint16 RamfuncsLoadSize;
extern Uint16 RamfuncsRunStart;

//static uint16_t comper1 = 0;
//static uint16_t comper3 = 0;

/* 程序入口：初始化外设后进入主循环。 */
void main(void)
{
    /* 初始化系统控制、时钟和看门狗等基础模块。 */
    InitSysCtrl();

    /* 将需要在 RAM 中运行的函数从 Flash 加载区复制到 RAM 运行区。 */
    memcpy((uint16_t *)&RamfuncsRunStart, (uint16_t *)&RamfuncsLoadStart, (unsigned long)&RamfuncsLoadSize);

    /* 初始化 Flash 等待状态，确保 Flash 访问时序正确。 */
    InitFlash();

    /* 初始化 PIE 中断控制器前，先关闭全局中断。 */
    DINT;

    /* 开启受保护寄存器写入权限。 */
    EALLOW;

        /* 初始化 PIE 中断控制器及其默认配置。 */
        InitPieCtrl();

        /* 清除 CPU 中断使能寄存器。 */
        IER = 0x0000;

        /* 清除待处理中断标志。 */
        IFR = 0x0000;

        /* 初始化 PIE 向量表。 */
        InitPieVectTable();

    /* 关闭受保护寄存器写入权限。 */
    EDIS;

    /* 初始化 GPIO。 */
    InitGpio();

    LED_Init();
    OLED_Init();
    ips200_init();

    /* 清屏并设置显示背景。 */
    ips200_full(RGB565_BLACK);

    /* 设置屏幕前景色和背景色。 */
    ips200_set_color(RGB565_WHITE, RGB565_BLACK);
    ips200_show_string(10, 10, "TMS320F28035");
    ips200_show_string(10, 30, "IPS200 240x320");

//  ips200_set_color(RGB565_YELLOW, RGB565_BLUE);
    ips200_show_string(10, 60, "Hello IPS200!");
    ips200_show_string(10, 90, "num =");

    /* 绘制屏幕边框，检查显示区域和颜色是否正常。 */
    ips200_draw_line(0,   0,   239, 0,   RGB565_RED);
    ips200_draw_line(0,   0,   0,   319, RGB565_RED);
    ips200_draw_line(239, 0,   239, 319, RGB565_RED);
    ips200_draw_line(0,   319, 239, 319, RGB565_RED);

    KEY_Init();

    EPwm1_Init();
    EPwm3_Init();

    ADC_Init();

    InitSciA();          // 初始化 SCI-A 串口。

    Timer0_Init();
    Timer1_Init();
    Timer2_Init();
    vofa_init();

    /* 初始化完成后进入主循环，持续更新计数并发送遥测数据。 */
    while(1)
    {
        num++;
        if(key1_flag == 1)
        {
            key1_flag=0;

        }
        if(key2_flag == 1)
        {
            key2_flag=0;

        }
        if(key3_flag == 1)
        {
            key3_flag=0;

        }
        if(key4_flag == 1)
        {
            key4_flag=0;
//            printf("%d,%d,%d\n\r",(int)timer0_count,timer1_count,(int)timer2_count);
        }
        telemetry_process();
        ips200_show_int(60, 90,  num, 6);
        ips200_show_int(60, 120, timer0_count, 8);
        ips200_show_int(60, 140, timer1_count, 8);
        ips200_show_int(60, 160, timer2_count, 8);



    }
}
