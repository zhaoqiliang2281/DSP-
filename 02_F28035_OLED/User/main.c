#include "DSP2803x_Device.h"   /* 包含 DSP 设备寄存器定义和外设宏 */
#include "DSP28x_Project.h"      /* 包含工程级配置与公共定义 */
#include <string.h>              /* 提供 memcpy 等内存操作函数 */
#include <stdint.h>              /* 提供固定宽度整数类型定义 */
#include "DSP2803x_Device.h"
#include "DSP2803x_Examples.h"

#include "OLED.h"




int32_t num;


/**
 * main.c - 纯 RAM 调试版
 * 工程链接文件使用 28035_RAM_lnk.cmd，程序全部在 SARAM 中运行，
 * 无需 Flash 拷贝，无需 InitFlash。
 */

/* 声明 RAM 函数段的起始地址、大小和运行时地址 */
extern Uint16 RamfuncsLoadStart;
extern Uint16 RamfuncsLoadSize;
extern Uint16 RamfuncsRunStart;

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

    /* 初始化 GPIO 模块 */
	InitGpio();

    /* 允许访问受保护的 GPIO 相关寄存器 */
	EALLOW;

    /* 打开 GPIO 输入使能时钟，确保 GPIO 模块时钟正常工作 */
	SysCtrlRegs.PCLKCR3.bit.GPIOINENCLK = 1;

    /* 配置 GPIO24 作为通用 I/O，不使用复用功能 */

	GpioCtrlRegs.GPAMUX2.bit.GPIO21=0;

    /* 设置 GPIO24 为输出方向，后续用作控制 LED 或输出信号 */

	GpioCtrlRegs.GPADIR.bit.GPIO21=1;

    /* 禁用 GPIO24、GPIO21 内部上拉电阻（输出模式下一般禁用上拉） */

	GpioCtrlRegs.GPAPUD.bit.GPIO21=1;

    /* 先将 GPIO24 输出清零，确保初始状态为低电平 */

	GpioDataRegs.GPACLEAR.bit.GPIO21=1;

    /* 结束对受保护寄存器的写操作 */
	EDIS;
	OLED_Init();
	OLED_ShowString(2, 1, "hello world!");

    /* 无限循环，程序持续运行 */
	while(1)
	{

	    /* 翻转 GPIO24 的电平，产生方波用于 LED 闪烁或示波器输出 */

//	    GpioDataRegs.GPASET.bit.GPIO21   = 1;
	    GpioDataRegs.GPATOGGLE.bit.GPIO21 =1;

	    num++;
	    OLED_ShowNum(1,1,num,5);

//	    /* 延时 250000 微秒，周期约 0.25s；需确保 CPU_RATE 配置为 60MHz */
//	    DELAY_US(500000);   // 确保 CPU_RATE 配置为 60MHz
//	    GpioDataRegs.GPACLEAR.bit.GPIO21 = 1;
//	    DELAY_US(500000);   // 确保 CPU_RATE 配置为 60MHz

	}
}

