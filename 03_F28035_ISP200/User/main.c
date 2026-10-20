#include "DSP2803x_Device.h"   /* 包含 DSP 设备寄存器定义和外设宏 */
#include "DSP28x_Project.h"      /* 包含工程级配置与公共定义 */
#include <string.h>              /* 提供 memcpy 等内存操作函数 */
#include <stdint.h>              /* 提供固定宽度整数类型定义 */
#include "DSP2803x_Device.h"
#include "DSP2803x_Examples.h"

#include "OLED.h"
#include "zf_device_ips200.h"    /* IPS200 液晶驱动 (TMS320F28035 版) */




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

    /* ★★★ 尽早把 IPS200 的 6 根引脚拉到安全电平 ★★★
     * 屏幕对上电时的引脚电平很敏感（你在 STM32 版本里就发现：那 4 行初始化电平
     * 不加就不显示）。F28035 上还多一个坑：InitSysCtrl() 里的 InitPeripheralClocks()
     * 会把 GPIO18 配成 XCLKOUT(输出 15MHz)，而 GPIO18 正是屏幕的 SCL。
     * 所以这里在其它事情之前，先把 6 根脚配成输出并给上安全电平：
     *   RST=1  DC=0  CS=1  BLK=0  SCL=0  SDA=1  （与 STM32 版本完全一致） */
    EALLOW;
    SysCtrlRegs.PCLKCR3.bit.GPIOINENCLK = 1;                  /* 打开 GPIO 时钟 */
    IPS200_SDA_INIT();                                        /* SDA */
    IPS200_SCL_INIT();                                        /* SCL */
    IPS200_CS_INIT();                                         /* CS  */
    IPS200_DC_INIT();                                         /* DC  */
    IPS200_RST_INIT();                                        /* RST */
    IPS200_BLK_INIT();                                        /* BL  */
    EDIS;

    IPS200_RST(1);      /* 复位脚先释放(高) */
    IPS200_DC(0);       /* 命令模式 */
    IPS200_CS(1);       /* 先不选中屏幕 */
    IPS200_BLK(0);      /* 背光先关 */
    IPS200_SCL(0);      /* 时钟空闲低 */
    IPS200_SDA(1);      /* 数据空闲高 */

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
    /* ★ InitGpio() 刚把所有引脚恢复成了“输入 + 上拉”，会覆盖掉最前面设的安全电平，
     *   所以这里必须再配置一次，保证进 OLED_Init()/ips200_init() 前屏幕 6 根线就已经稳定。 */
    EALLOW;

    IPS200_SDA_INIT();                                        /* SDA */
    IPS200_SCL_INIT();                                        /* SCL */
    IPS200_CS_INIT();                                         /* CS  */
    IPS200_DC_INIT();                                         /* DC  */
    IPS200_RST_INIT();                                        /* RST */
    IPS200_BLK_INIT();                                        /* BL  */
    EDIS;
    IPS200_RST(1);      /* 复位脚先释放(高) */
    IPS200_DC(0);       /* 命令模式 */
    IPS200_CS(1);       /* 先不选中屏幕 */
    IPS200_BLK(0);      /* 背光先关 */
    IPS200_SCL(0);      /* 时钟空闲低 */
    IPS200_SDA(1);      /* 数据空闲高 */
	OLED_Init();

	/* ==================== IPS200 初始化 ==================== */
	ips200_init();


	ips200_show_string(10, 10, "TMS320F28035");
	ips200_show_string(10, 30, "IPS200 240x320");

//	ips200_set_color(RGB565_YELLOW, RGB565_BLUE);
	ips200_show_string(10, 60, "Hello IPS200!");
	ips200_show_string(10, 90, "num =");

	/* 画一个红色边框，验证画线功能 */
	ips200_draw_line(0,   0,   239, 0,   RGB565_RED);
	ips200_draw_line(0,   0,   0,   319, RGB565_RED);
	ips200_draw_line(239, 0,   239, 319, RGB565_RED);
	ips200_draw_line(0,   319, 239, 319, RGB565_RED);

    /* 无限循环，程序持续运行 */
	while(1)
	{
	    num++;
	    /* 在 IPS200 上显示计数值 */
	    ips200_show_int(90, 90, num, 6);
        GpioDataRegs.GPATOGGLE.bit.GPIO21 =1;
        OLED_ShowNum(1,1,num,5);

	}
}

