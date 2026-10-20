/*
 * zf_device_ips200.c
 *
 *  Created on: 2026年9月25日
 *      Author: liang2281
 */


/***********************************************************************************************************
* IPS200 LCD 驱动 - TMS320F28035 (C2000) 移植版本
* 基于逐飞科技 IPS200 驱动移植，原版为 STM32F407 + HAL 库
*
* 硬件连接：
*   SPI-A  GPIO16(SPISIMOA) -> IPS200 SDA(MOSI)
*          GPIO18(SPICLKA)  -> IPS200 SCL(SCK)
*          GPIO17(SPISOMIA) -> 用不到，可以不接
*   普通IO GPIO19 -> CS，GPIO20 -> DC，GPIO22 -> RST，GPIO23 -> BLK
*   （引脚如与实际接线不同，改 zf_device_ips200.h 的“用户配置区域”即可）
*
* 使用说明：
*   1. GPIO 与 SPI-A 都由本驱动内部初始化，main 里只要调用一次 ips200_init()
*   2. 本驱动不是可重入的，不要在中断中调用
*   3. C28x 是 16bit 定点 CPU(int = 16bit, long = 32bit) 且没有 FPU，
*      ips200_show_float() 用的是软件浮点，速度慢，建议尽量用整数显示函数
****************************************************************************************************/

#include "zf_device_ips200.h"
#include "zf_common_font.h"
#include <string.h>
#include <stdlib.h>

//========================== 内部变量 ==========================
uint16  ips200_width_max    = 240;
uint16  ips200_height_max   = 320;
static uint16               ips200_pencolor     = IPS200_DEFAULT_PENCOLOR;
static uint16               ips200_bgcolor      = IPS200_DEFAULT_BGCOLOR;
static ips200_dir_enum      ips200_display_dir  = IPS200_DEFAULT_DISPLAY_DIR;
static ips200_font_size_enum ips200_display_font = IPS200_DEFAULT_DISPLAY_FONT;
static uint8                ips200_inited       = 0;    /* 0 = 还未初始化 */

/* 公用行缓冲：C28x 不支持变长数组(VLA)，清屏/填屏/刷图/字符显示都复用这一块
 * 因为屏用不到 DMA 且各函数不会嵌套调用，所以共用不会产生冲突
 * （注意：本驱动不是可重入的，不要在中断里调用） */
static uint16               ips200_line_buffer[IPS200_LINE_BUFFER_SIZE];

/* ms 级延时：依赖 DSP2803x_Examples.h 中的 DELAY_US 与 DSP28x_usDelay.asm */
#define IPS200_DELAY_MS(x)      DELAY_US((x) * 1000UL)


/* ================== 底层 SPI 实现方式 ==================
 * IPS200_USE_SOFT_SPI = 1 -> 用 GPIO 模拟 SPI（默认，最稳：只用到普通 IO）
 *                     = 0 -> 用硬件 SPI-A（速度快，但本工程调试时一直没能出图）
 * 两种方式用的是同一对引脚：SCL = GPIO18，SDA = GPIO16
 *
 * 自检诊断变量（仅硬件 SPI 模式使用，见 ips200_spi_selftest()） */
uint16 ips200_diag_tx_after = 0;
uint16 ips200_diag_rx_after = 0;
uint16 ips200_diag_fftx     = 0;
uint16 ips200_diag_echo     = 0xFFFF;

/* SPI 时钟模式：(CLKPOLARITY, CLK_PHASE)，也就是常说的 (CPOL, CPHA)
 *   0/0 = 模式 0   0/1 = 模式 1   1/0 = 模式 2   1/1 = 模式 3
 *
 * ★★ 实测结论（2026-09，LED 探针确认 6 根线全通后重测）★★
 *   本屏只能用 **CPOL = 1**（SCL 空闲为高）：
 *     模式扫描时 (1,0) 能刷出绿色、(1,1) 能刷出黄色；
 *     而 (0,0) 蓝色 / (0,1) 红色 两者都不出图。
 *   所以这里必须写 1，写成 0 就完全没反应（这是调了很久才找到的根因之一）。 */
#define IPS200_SPI_CLK_POLARITY 1
#define IPS200_SPI_CLK_PHASE    0

/* 当前使用的 SPI 模式（硬件/软件 SPI 共用） */
static uint8 ips200_spi_cpol = IPS200_SPI_CLK_POLARITY;
static uint8 ips200_spi_cpha = IPS200_SPI_CLK_PHASE;

#if (0 == IPS200_USE_SOFT_SPI)
//-------------------------------------------------------------------------------------------------------------------
// 【方案一】硬件 SPI-A 实现
//   GPIO16 = SPISIMOA (MOSI/SDA)  -> 接 IPS200 的 SDA
//   GPIO17 = SPISOMIA (MISO)      -> IPS200 用不到，可以不接
//   GPIO18 = SPICLKA  (SCK/SCL)   -> 接 IPS200 的 SCL
// C28x 的 SPI 只有 8bit 收发缓冲 + 16 级 FIFO，没有 STM32 那样的 DMA，
// 这里采用 "FIFO + 查询收发计数" 的方式，保证数据完整移出后才返回。
//-------------------------------------------------------------------------------------------------------------------
#define IPS200_SPI_TX_LIMIT     8           /* 发送 FIFO 中超过该字数就暂停写入，防止接收 FIFO 溢出 */

/* 低速外设时钟 LSPCLK：SPI 初始化里会把 LOSPCP 设成 0x0002，
 * 所以 LSPCLK = SYSCLKOUT/4 = 60MHz/4 = 15MHz（不依赖 InitPeripheralClocks 是否被调用）。
 * 如果以后改了 LOSPCP，请同步修改 IPS200_LSPCLK_HZ */
#define IPS200_LSPCLK_HZ        (15000000UL)
#define IPS200_SPI_CLK_HZ       (3000000UL)     /* SPI 时钟 3MHz：花屏就调小，想更快就调大 */
#define IPS200_SPI_BRR          ((Uint16)(IPS200_LSPCLK_HZ / IPS200_SPI_CLK_HZ - 1UL))

/* 轮询超时保护：等不到接收数据时循环这么多次就放弃（60MHz 下大约 1~2ms）。
 * 正常情况下用不到（SPI 主机自己产生时钟，接不接屏幕接收 FIFO 都会有数据），
 * 它只用于防止「忘了调用 ips200_init() / SPI 时钟没使能」时在这里死等。 */
#define IPS200_SPI_TIMEOUT      (20000UL)

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     SPI-A 初始化 (GPIO 复用 + 主机模式 + FIFO)
//-------------------------------------------------------------------------------------------------------------------
static void ips200_spi_init(void)
{
    EALLOW;

    /* 使能 SPI-A 时钟 (若 main 里没有调用 InitPeripheralClocks，这一句必须要有) */
    SysCtrlRegs.PCLKCR0.bit.SPIAENCLK = 1;

    /* 低速外设时钟分频：LSPCLK = SYSCLKOUT/4 = 15MHz
     * 显式设置，避免“有没有调用过 InitPeripheralClocks”导致 SPI 波特率不确定 */
    SysCtrlRegs.LOSPCP.all = 0x0002;

    /* GPIO16/17/18 复用为 SPI-A 功能脚，关闭内部上拉 */
    GpioCtrlRegs.GPAPUD.bit.GPIO16 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO17 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO18 = 1;
    GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 1;        /* SPISIMOA */
    GpioCtrlRegs.GPAMUX2.bit.GPIO17 = 1;        /* SPISOMIA */
    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 1;        /* SPICLKA  */
    GpioCtrlRegs.GPAQSEL2.bit.GPIO17 = 3;       /* SOMI 输入不做限定(异步) */

    EDIS;

    /* SPICCR = 0x0007：SPICHAR = 7 (8bit/次)，CLKPOLARITY = 0，SPISWRESET = 0（先停在复位状态）
     * SPICTL = 0x0006：SPIINTENA = 0，TALK = 1，MASTER_SLAVE = 1 (主机)，CLK_PHASE = 0
     * 默认 (CLKPOLARITY, CLK_PHASE) = (0, 0) 就是 SPI 模式 0。
     * 屏幕一直白屏或花屏时，改上面两个宏把四种模式各试一遍。 */
    SpiaRegs.SPICCR.all = (Uint16)(0x0007 | ((Uint16)ips200_spi_cpol << 6));
    SpiaRegs.SPICTL.all = (Uint16)(0x0006 | ((Uint16)ips200_spi_cpha << 3));
    SpiaRegs.SPIBRR     = IPS200_SPI_BRR;       /* SPI 时钟 = LSPCLK / (BRR + 1) */
    SpiaRegs.SPIPRI.all = 0x0010;               /* 仿真器暂停时 SPI 继续工作 */

    /* FIFO 必须在「SPI 还处于复位状态」时配置好，最后再释放复位。
     * 否则 FIFO 增强功能可能没生效，变成非 FIFO 模式：TXFFST 恒为 0，
     * 驱动会以为发送 FIFO 一直有空位，把数据全冲掉，屏幕就永远白屏。 */
    SpiaRegs.SPIFFTX.all = 0xE040;              /* 使能 FIFO，复位发送 FIFO，清中断标志 */
    SpiaRegs.SPIFFRX.all = 0x2044;              /* 复位接收 FIFO，清溢出标志 */
    SpiaRegs.SPIFFCT.all = 0x0000;              /* 发送间隔延时为 0 */

    SpiaRegs.SPICCR.bit.SPISWRESET = 1;         /* 最后释放 SPI 软件复位 */
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     清空接收 FIFO (让收发计数从 0 开始，保证收发一一对应)
//-------------------------------------------------------------------------------------------------------------------
static void ips200_spi_drain_rx(void)
{
    while(0 != SpiaRegs.SPIFFRX.bit.RXFFST)
    {
        (void)SpiaRegs.SPIRXBUF;
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     在运行时切换 SPI 时钟模式 (CPOL, CPHA)
// 备 注        屏幕没反应时可以依次试 (0,0)/(0,1)/(1,0)/(1,1) 四种模式；
//              改完之后需要再调用一次 ips200_init() 重新初始化屏幕
//-------------------------------------------------------------------------------------------------------------------
void ips200_set_spi_mode(uint8 cpol, uint8 cpha)
{
    ips200_spi_cpol = cpol;
    ips200_spi_cpha = cpha;

    /* 如果 SPI 已经初始化过了，这里立即生效（模式位最好在复位状态下改） */
    SpiaRegs.SPICCR.bit.SPISWRESET = 0;                 /* 进入复位 */
    SpiaRegs.SPICCR.bit.CLKPOLARITY = (cpol ? 1 : 0);
    SpiaRegs.SPICTL.bit.CLK_PHASE   = (cpha ? 1 : 0);
    SpiaRegs.SPICCR.bit.SPISWRESET = 1;                 /* 释放复位 */

    ips200_spi_drain_rx();
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     等待接收 FIFO 里出现 need 个数据（带超时保护）
// 返 回 值     1 = 等到；0 = 超时
// 备 注        SPI 处于主机模式时时钟是自己产生的，即使屏幕上什么都没接，
//              接收 FIFO 也照样会有数据进来（收到的是悬空引脚的噪声），
//              所以“没接屏幕”不会超时；超时只可能是 SPI 没工作。
//-------------------------------------------------------------------------------------------------------------------
static uint8 ips200_spi_wait_rx(uint16 need)
{
    uint32 guard = IPS200_SPI_TIMEOUT;

    while(need > SpiaRegs.SPIFFRX.bit.RXFFST)
    {
        if(0 == --guard)
        {
            return 0;                               /* 超时，直接返回，不死等 */
        }
    }
    return 1;
}

void ips200_spi_write_8bit(uint8 dat)
{
    ips200_spi_drain_rx();
    SpiaRegs.SPITXBUF = (uint16)(dat & 0x00FF);
    if(0 != ips200_spi_wait_rx(1))                  /* 等待本字节收发完成 */
    {
        (void)SpiaRegs.SPIRXBUF;
    }
}

void ips200_spi_write_16bit(uint16 dat)
{
    ips200_spi_drain_rx();
    SpiaRegs.SPITXBUF = (uint16)((dat >> 8) & 0x00FF);  /* 高字节先发 */
    SpiaRegs.SPITXBUF = (uint16)(dat & 0x00FF);         /* 低字节后发 */
    if(0 != ips200_spi_wait_rx(2))                      /* 等待两个字节都收发完成 */
    {
        (void)SpiaRegs.SPIRXBUF;
        (void)SpiaRegs.SPIRXBUF;
    }
}

void ips200_spi_write_8bit_array(const uint8 *dat, uint32 len)
{
    uint32 tx = 0;                                  /* 已写入 FIFO 的字节数 */
    uint32 rx = 0;                                  /* 已收完的字节数     */
    uint32 guard = IPS200_SPI_TIMEOUT;              /* 超时计数           */

    if((NULL == dat) || (0 == len)) return;

    ips200_spi_drain_rx();
    while(rx < len)
    {
        if((tx < len) && (IPS200_SPI_TX_LIMIT > SpiaRegs.SPIFFTX.bit.TXFFST))
        {
            SpiaRegs.SPITXBUF = (uint16)(dat[tx] & 0x00FF);
            tx++;
        }
        if(0 != SpiaRegs.SPIFFRX.bit.RXFFST)
        {
            (void)SpiaRegs.SPIRXBUF;
            rx++;
            guard = IPS200_SPI_TIMEOUT;             /* 有进展，重新计时 */
        }
        else if(0 == --guard)
        {
            return;                                 /* 超时退出，不再死等 */
        }
    }
}

void ips200_spi_write_16bit_array(const uint16 *dat, uint32 len)
{
    uint32 tx = 0;                                  /* 已写入 FIFO 的字节数 */
    uint32 rx = 0;                                  /* 已收完的字节数     */
    uint32 total = 0;
    uint32 guard = IPS200_SPI_TIMEOUT;              /* 超时计数           */
    uint16 word = 0;

    if((NULL == dat) || (0 == len)) return;

    total = len * 2;                                /* 每个像素 2 个字节 */

    ips200_spi_drain_rx();
    while(rx < total)
    {
        if((tx < total) && (IPS200_SPI_TX_LIMIT > SpiaRegs.SPIFFTX.bit.TXFFST))
        {
            if(0 == (tx & 1))
            {
                word = dat[tx >> 1];
                SpiaRegs.SPITXBUF = word >> 8;              /* 高字节先发 */
            }
            else
            {
                SpiaRegs.SPITXBUF = word & 0x00FF;          /* 低字节后发 */
            }
            tx++;
        }
        if(0 != SpiaRegs.SPIFFRX.bit.RXFFST)
        {
            (void)SpiaRegs.SPIRXBUF;
            rx++;
            guard = IPS200_SPI_TIMEOUT;                 /* 有进展，重新计时 */
        }
        else if(0 == --guard)
        {
            return;                                     /* 超时退出，不再死等 */
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     SPI-A 回环自检（不依赖屏幕）
// 返 回 值     1 = SPI 通道正常；0 = 异常
// 备 注        内部把 SPISIMOA 和 SPISOMIA 短接(回环)，发什么就应该收到什么：
//              发出去收不回来  -> SPI 时钟/配置问题（时钟没使能、没初始化…）
//              收到的和发的不一样 -> 波特率/相位问题
//              回环正常而屏幕还是白屏 -> 问题在接线或屏幕本身，不在 DSP 这边
//              注意：必须在 ips200_init() 之后调用（里面会先初始化 SPI-A）
//-------------------------------------------------------------------------------------------------------------------
uint8 ips200_spi_loopback_test(void)
{
    uint16 tx[4];
    uint8 ok = 1;
    uint8 i = 0;

    tx[0] = 0xA5;
    tx[1] = 0x5A;
    tx[2] = 0x3C;
    tx[3] = 0xC3;

    ips200_spi_drain_rx();

    /* 回环位(SPILBK)最好在 SPI 处于复位时设置，和 TI 例程的做法一致 */
    SpiaRegs.SPICCR.bit.SPISWRESET = 0;         /* 进入复位 */
    SpiaRegs.SPICCR.bit.SPILBK = 1;             /* 使能内部回环: SIMO 内部接到 SOMI */
    SpiaRegs.SPICCR.bit.SPISWRESET = 1;         /* 释放复位 */

    for(i = 0; i < 4; i++)
    {
        SpiaRegs.SPITXBUF = tx[i];
        if(0 == ips200_spi_wait_rx(1))
        {
            ok = 0;                             /* 连数据都没收到 */
            break;
        }
        if(tx[i] != SpiaRegs.SPIRXBUF)
        {
            ok = 0;                             /* 收到的和发出去的不一样 */
            break;
        }
    }

    SpiaRegs.SPICCR.bit.SPISWRESET = 0;
    SpiaRegs.SPICCR.bit.SPILBK = 0;             /* 关闭回环，恢复正常输出 */
    SpiaRegs.SPICCR.bit.SPISWRESET = 1;

    ips200_spi_drain_rx();

    return ok;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     SPI-A 综合自检（结果放在 ips200_diag_xxx 变量里）
// 备 注        必须在 ips200_init() 之后调用；不依赖屏幕
//              三项检查分别对应“FIFO 是否生效”、“能不能移位”、“接收链路通不通”
//-------------------------------------------------------------------------------------------------------------------
void ips200_spi_selftest(void)
{
    ips200_diag_tx_after = 0;
    ips200_diag_rx_after = 0;
    ips200_diag_echo     = 0xFFFF;

    ips200_spi_drain_rx();

    /* 1. 普通发送一个字节，看它能不能被移出 TX FIFO
     *    TXFFST 变回 0  -> 数据发出去了
     *    TXFFST 还是 1  -> SPI 根本没在移位(时钟/复位/从机模式) */
    SpiaRegs.SPITXBUF = 0x00A5;
    DELAY_US(200);                                  /* 3MHz 下 1 个字节约 3us，200us 足够 */
    ips200_diag_tx_after = SpiaRegs.SPIFFTX.bit.TXFFST;
    ips200_diag_rx_after = SpiaRegs.SPIFFRX.bit.RXFFST;
    ips200_diag_fftx     = SpiaRegs.SPIFFTX.all;

    ips200_spi_drain_rx();

    /* 2. 回环自检：发什么就应该收到什么 */
    if(0 != ips200_spi_loopback_test())
    {
        ips200_diag_echo = 0x00A5;                  /* 回环通过 */
    }
    else
    {
        ips200_diag_echo = 0xFFFF;                  /* 回环失败 */
    }
}

#else   /* ================== 【方案二】GPIO 模拟 SPI（默认） ================== */

/* 位延时：数值越大时钟越慢。花屏/不稳定就调大（2 约 2MHz，8 约 0.6MHz）。
 * ★ 性能注意：这里刻意用「宏」而不是函数。
 *   -O0 下一个函数调用要几十个周期，而每个字节要调 24 次，整屏 15 万个字节，
 *   光函数调用开销就吃掉一大半时间。改成宏内联展开后整屏刷新快约 3 倍。 */
#define IPS200_SOFT_SPI_DLY     0

#if (IPS200_SOFT_SPI_DLY > 0)
#define IPS200_SPI_DLY()        do { volatile uint16 _n = IPS200_SOFT_SPI_DLY; \
                                     while(_n > 0) { _n--; } } while(0)
#else
#define IPS200_SPI_DLY()        do { } while(0)     /* 0 = 不加延时，最快 */
#endif

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     软件 SPI 初始化：把 GPIO18/GPIO16 配成普通输出
//-------------------------------------------------------------------------------------------------------------------
static void ips200_spi_init(void)
{
    EALLOW;
    IPS200_SCL_INIT();                          /* SCL = GPIO18，普通输出 */
    IPS200_SDA_INIT();                          /* SDA = GPIO16，普通输出 */
    EDIS;

    /* 空闲电平按 CPOL 决定 */
    if(0 != ips200_spi_cpol)
    {
        IPS200_SCL(1);
    }
    else
    {
        IPS200_SCL(0);
    }
    IPS200_SDA(0);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     软件 SPI 发送一个字节（MSB 先发，支持 4 种 CPOL/CPHA 时序）
// 备 注        模式 0/1：SCL 空闲低；模式 2/3：SCL 空闲高
//              四种模式各写一个紧凑的内层循环，把模式判断提到循环外面 ——
//              否则每字节约要判断 8 次 (cpol, cpha)，-O0 下开销很大
//-------------------------------------------------------------------------------------------------------------------
static void ips200_soft_spi_byte(uint8 dat)
{
    uint8 i = 0;
    uint8 b = 0;

    if((0 == ips200_spi_cpol) && (0 == ips200_spi_cpha))            /* 模式 0 */
    {
        for(i = 0; i < 8; i++)
        {
            b = ((dat & 0x80) != 0) ? 1 : 0;
            dat <<= 1;

            IPS200_SDA(b);          IPS200_SPI_DLY();
            IPS200_SCL(1);          IPS200_SPI_DLY();      /* 上升沿采样 */
            IPS200_SCL(0);          IPS200_SPI_DLY();
        }
    }
    else if((0 == ips200_spi_cpol) && (1 == ips200_spi_cpha))       /* 模式 1 */
    {
        for(i = 0; i < 8; i++)
        {
            b = ((dat & 0x80) != 0) ? 1 : 0;
            dat <<= 1;

            IPS200_SCL(1);          IPS200_SPI_DLY();
            IPS200_SDA(b);          IPS200_SPI_DLY();
            IPS200_SCL(0);          IPS200_SPI_DLY();      /* 下降沿采样 */
        }
    }
    else if((1 == ips200_spi_cpol) && (0 == ips200_spi_cpha))       /* 模式 2 */
    {
        for(i = 0; i < 8; i++)
        {
            b = ((dat & 0x80) != 0) ? 1 : 0;
            dat <<= 1;

            IPS200_SDA(b);          IPS200_SPI_DLY();
            IPS200_SCL(0);          IPS200_SPI_DLY();      /* 下降沿采样 */
            IPS200_SCL(1);          IPS200_SPI_DLY();
        }
    }
    else                                                           /* 模式 3 */
    {
        for(i = 0; i < 8; i++)
        {
            b = ((dat & 0x80) != 0) ? 1 : 0;
            dat <<= 1;

            IPS200_SCL(0);          IPS200_SPI_DLY();
            IPS200_SDA(b);          IPS200_SPI_DLY();
            IPS200_SCL(1);          IPS200_SPI_DLY();      /* 上升沿采样 */
        }
    }
}

void ips200_spi_write_8bit(uint8 dat)
{
    ips200_soft_spi_byte(dat);
}

void ips200_spi_write_16bit(uint16 dat)
{
    ips200_soft_spi_byte((uint8)((dat >> 8) & 0x00FF));     /* 高字节先发 */
    ips200_soft_spi_byte((uint8)(dat & 0x00FF));            /* 低字节后发 */
}

void ips200_spi_write_8bit_array(const uint8 *dat, uint32 len)
{
    uint32 i = 0;

    if(NULL == dat) return;

    for(i = 0; i < len; i++)
    {
        ips200_soft_spi_byte(dat[i]);
    }
}

void ips200_spi_write_16bit_array(const uint16 *dat, uint32 len)
{
    uint32 i = 0;

    if(NULL == dat) return;

    for(i = 0; i < len; i++)
    {
        ips200_soft_spi_byte((uint8)((dat[i] >> 8) & 0x00FF));
        ips200_soft_spi_byte((uint8)(dat[i] & 0x00FF));
    }
}

// 软件 SPI 下切换时钟模式：(CPOL, CPHA)，改完需要重新调用 ips200_init() 初始化屏幕
void ips200_set_spi_mode(uint8 cpol, uint8 cpha)
{
    ips200_spi_cpol = (cpol ? 1 : 0);
    ips200_spi_cpha = (cpha ? 1 : 0);

    if(0 != ips200_spi_cpol)                    /* 同步一下空闲电平 */
    {
        IPS200_SCL(1);
    }
    else
    {
        IPS200_SCL(0);
    }
}

uint8 ips200_spi_loopback_test(void)
{
    return 1;                                   /* 软件 SPI 不自环 */
}

void ips200_spi_selftest(void)
{
    ips200_diag_tx_after = 0;                   /* 软件 SPI 不走硬件 FIFO，这几项无意义 */
    ips200_diag_rx_after = 0;
    ips200_diag_fftx     = 0;
    ips200_diag_echo     = 0;
}

#endif  /* IPS200_USE_SOFT_SPI */



//========================== 内部 GPIO 控制宏 ==========================
// IPS200_RST / IPS200_DC / IPS200_CS / IPS200_BLK 以及对应的引脚初始化宏
// 都定义在 zf_device_ips200.h 中，修改接线只需要改头文件里那一块。

//========================== 内部 SPI 写数据宏 ==========================
#define ips200_write_8bit_data_spi(data)                (ips200_spi_write_8bit(data))
#define ips200_write_8bit_data_spi_array(data, len)     (ips200_spi_write_8bit_array((data), (len)))
#define ips200_write_16bit_data_spi(data)               (ips200_spi_write_16bit(data))
#define ips200_write_16bit_data_spi_array(data, len)    (ips200_spi_write_16bit_array((data), (len)))

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 写命令
//-------------------------------------------------------------------------------------------------------------------
static void ips200_write_command(const uint8 command)
{
    IPS200_DC(0);
    ips200_write_8bit_data_spi(command);
    IPS200_DC(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 写 8bit 数据 (单字节)
//-------------------------------------------------------------------------------------------------------------------
static void ips200_write_8bit_data(const uint8 dat)
{
    ips200_write_8bit_data_spi(dat);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 写 16bit 数据
//-------------------------------------------------------------------------------------------------------------------
static void ips200_write_16bit_data(const uint16 dat)
{
    ips200_write_16bit_data_spi(dat);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 写 16bit 数据数组
//-------------------------------------------------------------------------------------------------------------------
static void ips200_write_16bit_data_array(const uint16 *dat, uint32 len)
{
    ips200_write_16bit_data_spi_array(dat, len);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     设置显示区域
//-------------------------------------------------------------------------------------------------------------------
static void ips200_set_region(uint16 x1, uint16 y1, uint16 x2, uint16 y2)
{
    ips200_write_command(0x2a);
    ips200_write_16bit_data(x1);
    ips200_write_16bit_data(x2);

    ips200_write_command(0x2b);
    ips200_write_16bit_data(y1);
    ips200_write_16bit_data(y2);

    ips200_write_command(0x2c);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 清屏函数
//-------------------------------------------------------------------------------------------------------------------
void ips200_clear(void)
{
    uint16 i = 0, j = 0;

    IPS200_CS(0);
    ips200_set_region(0, 0, ips200_width_max - 1, ips200_height_max - 1);
    for(i = 0; i < ips200_width_max; i++)
    {
        ips200_line_buffer[i] = ips200_bgcolor;
    }
    for(j = 0; j < ips200_height_max; j++)
    {
        ips200_write_16bit_data_array(ips200_line_buffer, ips200_width_max);
    }
    IPS200_CS(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 屏幕填充函数
//-------------------------------------------------------------------------------------------------------------------
void ips200_full(const uint16 color)
{
    uint16 i = 0, j = 0;

    IPS200_CS(0);
    ips200_set_region(0, 0, ips200_width_max - 1, ips200_height_max - 1);
    for(i = 0; i < ips200_width_max; i++)
    {
        ips200_line_buffer[i] = color;
    }
    for(j = 0; j < ips200_height_max; j++)
    {
        ips200_write_16bit_data_array(ips200_line_buffer, ips200_width_max);
    }
    IPS200_CS(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     写入扫描方向寄存器 MADCTL (0x36)
//-------------------------------------------------------------------------------------------------------------------
static void ips200_write_madctl(void)
{
    ips200_write_command(0x36);                 // Memory access control
    switch(ips200_display_dir)
    {
        case IPS200_PORTAIT:        ips200_write_8bit_data(0x00);   break;
        case IPS200_PORTAIT_180:    ips200_write_8bit_data(0xC0);   break;
        case IPS200_CROSSWISE:      ips200_write_8bit_data(0x70);   break;
        case IPS200_CROSSWISE_180:  ips200_write_8bit_data(0xA0);   break;
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     设置显示方向
// 备注信息     既可以在 ips200_init() 之前调用（只记下方向，由 init 写入屏幕），
//              也可以在 ips200_init() 之后调用（会立即刷新 MADCTL 并清屏）
//-------------------------------------------------------------------------------------------------------------------
void ips200_set_dir(ips200_dir_enum dir)
{
    ips200_display_dir = dir;
    switch(ips200_display_dir)
    {
        case IPS200_PORTAIT:
        case IPS200_PORTAIT_180:
        {
            ips200_width_max = 240;
            ips200_height_max = 320;
        }break;
        case IPS200_CROSSWISE:
        case IPS200_CROSSWISE_180:
        {
            ips200_width_max = 320;
            ips200_height_max = 240;
        }break;
    }

    if(0 != ips200_inited)
    {
        /* 屏幕已经初始化过，这里把新方向真正写进屏幕 */
        IPS200_CS(0);
        ips200_write_madctl();
        IPS200_CS(1);
        ips200_clear();
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     设置显示字体
//-------------------------------------------------------------------------------------------------------------------
void ips200_set_font(ips200_font_size_enum font)
{
    ips200_display_font = font;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     设置显示颜色
//-------------------------------------------------------------------------------------------------------------------
void ips200_set_color(const uint16 pen, const uint16 bgcolor)
{
    ips200_pencolor = pen;
    ips200_bgcolor = bgcolor;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 画点
//-------------------------------------------------------------------------------------------------------------------
void ips200_draw_point(uint16 x, uint16 y, const uint16 color)
{
    if(x >= ips200_width_max || y >= ips200_height_max) return;

    IPS200_CS(0);
    ips200_set_region(x, y, x, y);
    ips200_write_16bit_data(color);
    IPS200_CS(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 画线
//-------------------------------------------------------------------------------------------------------------------
void ips200_draw_line(uint16 x_start, uint16 y_start, uint16 x_end, uint16 y_end, const uint16 color)
{
    if(x_start >= ips200_width_max || y_start >= ips200_height_max ||
       x_end >= ips200_width_max || y_end >= ips200_height_max) return;

    int16 x_dir = (x_start < x_end ? 1 : -1);
    int16 y_dir = (y_start < y_end ? 1 : -1);
    float temp_rate = 0;
    float temp_b = 0;

    do
    {
        if(x_start != x_end)
        {
            temp_rate = (float)(y_start - y_end) / (float)(x_start - x_end);
            temp_b = (float)y_start - (float)x_start * temp_rate;
        }
        else
        {
            while(y_start != y_end)
            {
                ips200_draw_point(x_start, y_start, color);
                y_start += y_dir;
            }
            ips200_draw_point(x_start, y_start, color);
            break;
        }
        if(abs(y_start - y_end) > abs(x_start - x_end))
        {
            while(y_start != y_end)
            {
                ips200_draw_point(x_start, y_start, color);
                y_start += y_dir;
                x_start = (int16)(((float)y_start - temp_b) / temp_rate);
            }
            ips200_draw_point(x_start, y_start, color);
        }
        else
        {
            while(x_start != x_end)
            {
                ips200_draw_point(x_start, y_start, color);
                x_start += x_dir;
                y_start = (int16)((float)x_start * temp_rate + temp_b);
            }
            ips200_draw_point(x_start, y_start, color);
        }
    } while(0);
}


//-------------------------------------------------------------------------------------------------------------------
// 简单的 int32 转字符串 (内部使用)
//-------------------------------------------------------------------------------------------------------------------
static void ips200_int_to_str(char *buf, int32 dat)
{
    char temp[12];
    int i = 0, j = 0;
    uint8 negative = 0;
    uint32 udat;

    if(dat < 0)
    {
        negative = 1;
        udat = (uint32)(-(dat + 1)) + 1;
    }
    else
    {
        udat = (uint32)dat;
    }

    if(udat == 0)
    {
        temp[i++] = '0';
    }
    else
    {
        while(udat > 0)
        {
            temp[i++] = '0' + (char)(udat % 10);
            udat /= 10;
        }
    }
    if(negative)
    {
        temp[i++] = '-';
    }
    for(j = 0; j < i; j++)
    {
        buf[j] = temp[i - 1 - j];
    }
    buf[i] = '\0';
}

//-------------------------------------------------------------------------------------------------------------------
// 简单的 uint32 转字符串 (内部使用)
//-------------------------------------------------------------------------------------------------------------------
static void ips200_uint_to_str(char *buf, uint32 dat)
{
    char temp[12];
    int i = 0, j = 0;

    if(dat == 0)
    {
        temp[i++] = '0';
    }
    else
    {
        while(dat > 0)
        {
            temp[i++] = '0' + (char)(dat % 10);
            dat /= 10;
        }
    }
    for(j = 0; j < i; j++)
    {
        buf[j] = temp[i - 1 - j];
    }
    buf[i] = '\0';
}

//-------------------------------------------------------------------------------------------------------------------
// 简单的 double 转字符串 (内部使用)
//-------------------------------------------------------------------------------------------------------------------
static void ips200_double_to_str(char *buf, double dat, uint8 pointnum)
{
    int32 int_part;
    uint32 frac_part;
    uint32 pow10 = 1;
    int i;
    int len = 0;
    int is_neg = 0;

    if(pointnum > 8) pointnum = 8;      /* pow10 是 uint32，最多只能到 1e9，防止溢出 */

    if(dat < 0)
    {
        is_neg = 1;
        dat = -dat;
    }

    int_part = (int32)dat;
    for(i = 0; i < pointnum; i++) pow10 *= 10;
    frac_part = (uint32)((dat - (double)int_part) * pow10 + 0.5);

    if(is_neg) buf[len++] = '-';

    if(int_part == 0)
    {
        buf[len++] = '0';
    }
    else
    {
        char int_buf[12];
        int ilen = 0;
        uint32 udat = (uint32)int_part;
        while(udat > 0)
        {
            int_buf[ilen++] = '0' + (char)(udat % 10);
            udat /= 10;
        }
        for(i = ilen - 1; i >= 0; i--)
        {
            buf[len++] = int_buf[i];
        }
    }

    if(pointnum > 0)
    {
        buf[len++] = '.';
        uint32 frac_pow10 = pow10 / 10;
        while(frac_pow10 > 0 && frac_part < frac_pow10)
        {
            buf[len++] = '0';
            frac_pow10 /= 10;
        }
        if(frac_part == 0)
        {
            for(i = 0; i < pointnum; i++) buf[len++] = '0';
        }
        else
        {
            char frac_buf[12];
            int flen = 0;
            while(frac_part > 0)
            {
                frac_buf[flen++] = '0' + (char)(frac_part % 10);
                frac_part /= 10;
            }
            for(i = flen - 1; i >= 0; i--)
            {
                buf[len++] = frac_buf[i];
            }
        }
    }
    buf[len] = '\0';
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 显示字符
//-------------------------------------------------------------------------------------------------------------------
void ips200_show_char(uint16 x, uint16 y, const char dat)
{
    if(x >= ips200_width_max || y >= ips200_height_max) return;
    if(dat < 32 || dat > 126) return;

    uint8 i = 0, j = 0;

    IPS200_CS(0);
    switch(ips200_display_font)
    {
        case IPS200_6X8_FONT:
        {
            ips200_set_region(x, y, x + 5, y + 7);
            for(i = 0; 6 > i; i++)
            {
                uint8 temp_top = ascii_font_6x8[dat - 32][i];
                for(j = 0; 8 > j; j++)
                {
                    ips200_line_buffer[i + j * 6] = (temp_top & 0x01) ? ips200_pencolor : ips200_bgcolor;
                    temp_top >>= 1;
                }
            }
            ips200_write_16bit_data_array(ips200_line_buffer, 6 * 8);
        }break;
        // 本工程字库里只有 6x8 和 8x16 两种 ASCII 字模（见 zf_common_font.c），
        // 所以 16x16 / 32x32 暂时按 8x16 显示；需要大字体时请先把字模补进字库。
        case IPS200_16X16_FONT:
        case IPS200_32X32_FONT:
        case IPS200_8X16_FONT:
        {
            ips200_set_region(x, y, x + 7, y + 15);
            for(i = 0; 8 > i; i++)
            {
                uint8 temp_top = ascii_font_8x16[dat - 32][i];
                uint8 temp_bottom = ascii_font_8x16[dat - 32][i + 8];
                for(j = 0; 8 > j; j++)
                {
                    ips200_line_buffer[i + j * 8] = (temp_top & 0x01) ? ips200_pencolor : ips200_bgcolor;
                    temp_top >>= 1;
                }
                for(j = 0; 8 > j; j++)
                {
                    ips200_line_buffer[i + j * 8 + 4 * 16] = (temp_bottom & 0x01) ? ips200_pencolor : ips200_bgcolor;
                    temp_bottom >>= 1;
                }
            }
            ips200_write_16bit_data_array(ips200_line_buffer, 8 * 16);
        }break;
        default:
        {
        }break;
    }
    IPS200_CS(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 显示字符串
//-------------------------------------------------------------------------------------------------------------------
void ips200_show_string(uint16 x, uint16 y, const char dat[])
{
    uint16 j = 0;
    uint16 font_width = 0;

    if(x >= ips200_width_max || y >= ips200_height_max) return;

    /* 字模宽度：6x8 字体下是 6，其余情况都按 8 处理 */
    font_width = (IPS200_6X8_FONT == ips200_display_font) ? 6 : 8;

    while('\0' != dat[j])
    {
        ips200_show_char(x + font_width * j, y, dat[j]);
        j++;
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 显示32位有符号整数
//-------------------------------------------------------------------------------------------------------------------
void ips200_show_int(uint16 x, uint16 y, const int32 dat, uint8 num)
{
    if(x >= ips200_width_max || y >= ips200_height_max) return;

    int32  dat_temp = dat;
    int32  offset = 1;
    char   num_str[12];         /* 纯数字串，例如 "-123"             */
    char   data_buffer[12];     /* 右对齐后的显示串，例如 "  -123"   */
    uint8  i;
    uint8  len = 0;             /* 数字串长度                        */
    uint8  start = 0;           /* 从第几个字符开始显示（超宽时截头）*/
    uint8  show_len = 0;
    uint8  pad = 0;

    if(11 < num) num = 11;                      /* 缓冲区只有 12 个字，防止越界 */

    if(10 > num)
    {
        for(i = 0; i < num; i++) offset *= 10;
        if(0 != offset) dat_temp %= offset;
    }

    ips200_int_to_str(num_str, dat_temp);

    while(num_str[len] != '\0') { len++; }      /* 不用 strlen，省一个库函数 */
    if(len > num) { start = (uint8)(len - num); }
    show_len = (uint8)(len - start);
    pad = (uint8)(num - show_len);              /* 左边补空格，凑够 num 位 */

    /* ★ 关键：一定要画满 num 个字符（空格也算），
     *   否则数据从 548 变成 54 时，原来那一位不会被覆盖，
     *   屏幕上就会残留一个 '8'。 */
    for(i = 0; i < pad; i++)
    {
        data_buffer[i] = ' ';
    }
    for(i = 0; i < show_len; i++)
    {
        data_buffer[pad + i] = num_str[start + i];
    }
    data_buffer[num] = '\0';

    ips200_show_string(x, y, data_buffer);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 显示32位无符号整数
//-------------------------------------------------------------------------------------------------------------------
void ips200_show_uint(uint16 x, uint16 y, const uint32 dat, uint8 num)
{
    if(x >= ips200_width_max || y >= ips200_height_max) return;

    uint32 dat_temp = dat;
    int32  offset = 1;
    char   num_str[12];
    char   data_buffer[12];
    uint8  i;
    uint8  len = 0;
    uint8  start = 0;
    uint8  show_len = 0;
    uint8  pad = 0;

    if(11 < num) num = 11;

    if(10 > num)
    {
        for(i = 0; i < num; i++) offset *= 10;
        if(0 != offset) dat_temp %= offset;
    }

    ips200_uint_to_str(num_str, dat_temp);

    while(num_str[len] != '\0') { len++; }
    if(len > num) { start = (uint8)(len - num); }
    show_len = (uint8)(len - start);
    pad = (uint8)(num - show_len);

    /* 和 ips200_show_int 一样：固定画满 num 位，避免旧数据残留 */
    for(i = 0; i < pad; i++)
    {
        data_buffer[i] = ' ';
    }
    for(i = 0; i < show_len; i++)
    {
        data_buffer[pad + i] = num_str[start + i];
    }
    data_buffer[num] = '\0';

    ips200_show_string(x, y, data_buffer);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 显示浮点数
//-------------------------------------------------------------------------------------------------------------------
void ips200_show_float(uint16 x, uint16 y, const double dat, uint8 num, uint8 pointnum)
{
    if(x >= ips200_width_max || y >= ips200_height_max) return;

    double dat_temp = dat;
    double offset = 1.0;
    char data_buffer[24];
    uint8 i;

    if(num > 9) num = 9;                /* offset 最大 1e9，再大就会溢出 */

    for(i = 0; i < num; i++) offset *= 10.0;
    /* 取模：只保留 num 位整数以内的小数部分（丢掉更高的整数位）
     * 注意 1：C28x 的 int 只有 16bit，这里必须用 int32 取整，否则大数会溢出
     * 注意 2：必须先做整数除法，再转 double 相乘！
     *         若写成 ((double)a / (double)b) * b，会变成浮点除法，
     *         3.0 / 1000.0 * 1000.0 = 3.0，等于没有取模，
     *         3.3 会被减成 0.3（整数部分丢失） */
    dat_temp = dat_temp - (double)((int32)dat_temp / (int32)offset) * offset;

    ips200_double_to_str(data_buffer, dat_temp, pointnum);
    ips200_show_string(x, y, data_buffer);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 显示二值图像
//-------------------------------------------------------------------------------------------------------------------
void ips200_show_binary_image(uint16 x, uint16 y, const uint8 *image, uint16 width, uint16 height, uint16 dis_width, uint16 dis_height)
{
    if(x >= ips200_width_max || y >= ips200_height_max || NULL == image) return;
    if(dis_width > IPS200_LINE_BUFFER_SIZE) return;     /* 超过行缓冲大小，直接返回 */

    uint32 i = 0, j = 0;
    uint8 temp = 0;
    uint32 width_index = 0;
    const uint8 *image_temp;

    IPS200_CS(0);
    ips200_set_region(x, y, x + dis_width - 1, y + dis_height - 1);

    for(j = 0; j < dis_height; j++)
    {
        /* C28x 的 uint16 只有 16bit，乘法前必须先转成 uint32，否则会溢出 */
        image_temp = image + (uint32)j * height / dis_height * width / 8;
        for(i = 0; i < dis_width; i++)
        {
            width_index = (uint32)i * width / dis_width;
            temp = *(image_temp + width_index / 8);
            ips200_line_buffer[i] = (0x80 & (temp << (width_index % 8))) ? RGB565_WHITE : RGB565_BLACK;
        }
        ips200_write_16bit_data_array(ips200_line_buffer, dis_width);
    }
    IPS200_CS(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 显示 8bit 灰度图像
//-------------------------------------------------------------------------------------------------------------------
void ips200_show_gray_image(uint16 x, uint16 y, const uint8 *image, uint16 width, uint16 height, uint16 dis_width, uint16 dis_height, uint8 threshold)
{
    if(x >= ips200_width_max || y >= ips200_height_max || NULL == image) return;
    if(dis_width > IPS200_LINE_BUFFER_SIZE) return;     /* 超过行缓冲大小，直接返回 */

    uint32 i = 0, j = 0;
    uint16 color = 0, temp = 0;
    const uint8 *image_temp;

    IPS200_CS(0);
    ips200_set_region(x, y, x + dis_width - 1, y + dis_height - 1);

    for(j = 0; j < dis_height; j++)
    {
        /* C28x 的 uint16 只有 16bit，乘法前必须先转成 uint32，否则会溢出 */
        image_temp = image + (uint32)j * height / dis_height * width;
        for(i = 0; i < dis_width; i++)
        {
            temp = *(image_temp + (uint32)i * width / dis_width);
            if(threshold == 0)
            {
                color = (0x001f & ((temp) >> 3)) << 11;
                color = color | (((0x003f) & ((temp) >> 2)) << 5);
                color = color | (0x001f & ((temp) >> 3));
                ips200_line_buffer[i] = color;
            }
            else if(temp < threshold)
            {
                ips200_line_buffer[i] = RGB565_BLACK;
            }
            else
            {
                ips200_line_buffer[i] = RGB565_WHITE;
            }
        }
        ips200_write_16bit_data_array(ips200_line_buffer, dis_width);
    }
    IPS200_CS(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 显示 RGB565 彩色图像
//-------------------------------------------------------------------------------------------------------------------
void ips200_show_rgb565_image(uint16 x, uint16 y, const uint16 *image, uint16 width, uint16 height, uint16 dis_width, uint16 dis_height, uint8 color_mode)
{
    if(x >= ips200_width_max || y >= ips200_height_max || NULL == image) return;
    if(dis_width > IPS200_LINE_BUFFER_SIZE) return;     /* 超过行缓冲大小，直接返回 */

    uint32 i = 0, j = 0;
    const uint16 *image_temp;

    IPS200_CS(0);
    ips200_set_region(x, y, x + dis_width - 1, y + dis_height - 1);

    for(j = 0; j < dis_height; j++)
    {
        /* C28x 的 uint16 只有 16bit，乘法前必须先转成 uint32，否则会溢出 */
        image_temp = image + (uint32)j * height / dis_height * width;
        for(i = 0; i < dis_width; i++)
        {
            ips200_line_buffer[i] = *(image_temp + (uint32)i * width / dis_width);
        }
        if(color_mode)
        {
            /* color_mode != 0：按低字节在前逐字节发送
             * （C28x 上 char 占 1 个字，不能像 STM32 那样用 (uint8*) 强转直接发字节） */
            for(i = 0; i < dis_width; i++)
            {
                ips200_write_8bit_data((uint8)(ips200_line_buffer[i] & 0x00FF));
                ips200_write_8bit_data((uint8)((ips200_line_buffer[i] >> 8) & 0x00FF));
            }
        }
        else
        {
            ips200_write_16bit_data_array(ips200_line_buffer, dis_width);
        }
    }
    IPS200_CS(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IPS200 显示波形
//-------------------------------------------------------------------------------------------------------------------
void ips200_show_wave(uint16 x, uint16 y, const uint16 *wave, uint16 width, uint16 value_max, uint16 dis_width, uint16 dis_value_max)
{
    if(x >= ips200_width_max || y >= ips200_height_max || NULL == wave) return;
    if(dis_width > IPS200_LINE_BUFFER_SIZE) return;     /* 超过行缓冲大小，直接返回 */
    if(0 == value_max || 0 == dis_value_max) return;

    uint32 i = 0, j = 0;
    uint32 width_index = 0, value_max_index = 0;

    IPS200_CS(0);
    ips200_set_region(x, y, x + dis_width - 1, y + dis_value_max - 1);
    for(j = 0; j < dis_value_max; j++)
    {
        for(i = 0; i < dis_width; i++)
        {
            ips200_line_buffer[i] = ips200_bgcolor;
        }
        ips200_write_16bit_data_array(ips200_line_buffer, dis_width);
    }
    IPS200_CS(1);

    for(i = 0; i < dis_width; i++)
    {
        width_index = (uint32)i * width / dis_width;
        /* 先转 uint32 再乘，避免 16bit 溢出（要求 wave 的值 <= value_max） */
        value_max_index = (uint32)(*(wave + width_index)) * (dis_value_max - 1) / value_max;
        ips200_draw_point((uint16)(i + x), (uint16)((dis_value_max - 1) - value_max_index + y), ips200_pencolor);
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     汉字显示
//-------------------------------------------------------------------------------------------------------------------
void ips200_show_chinese(uint16 x, uint16 y, uint8 size, const uint8 *chinese_buffer, uint8 number, const uint16 color)
{
    if(x >= ips200_width_max || y >= ips200_height_max || NULL == chinese_buffer) return;

    int i = 0, j = 0, k = 0;
    uint8 temp = 0, temp1 = 0, temp2 = 0;
    const uint8 *p_data = chinese_buffer;

    temp2 = size / 8;

    IPS200_CS(0);
    ips200_set_region(x, y, number * size - 1 + x, y + size - 1);

    for(i = 0; i < size; i++)
    {
        temp1 = number;
        p_data = chinese_buffer + i * temp2;
        while(temp1--)
        {
            for(k = 0; k < temp2; k++)
            {
                for(j = 8; 0 < j; j--)
                {
                    temp = (*p_data >> (j - 1)) & 0x01;
                    ips200_write_16bit_data(temp ? color : ips200_bgcolor);
                }
                p_data++;
            }
            p_data = p_data - temp2 + temp2 * size;
        }
    }
    IPS200_CS(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     32x32 汉字显示
// 备注信息     chinese_buffer 格式: 逐行式, 阴码, 顺向, 每字符 128 个元素
//              每行 4 个元素 (MSB=左), 共 32 行，所以字模数组要声明成 [][128]，
//              也就是 const uint8 chinese_font_32x32[汉字个数][128]
//              （注意：C28x 上 uint8 占 1 个字，字模会比较占 Flash，慎用大字号）
//-------------------------------------------------------------------------------------------------------------------
void ips200_show_chinese_32x32(uint16 x, uint16 y, const uint8 *chinese_buffer, uint8 number, const uint16 color)
{
    if(x >= ips200_width_max || y >= ips200_height_max || NULL == chinese_buffer) return;

    int i = 0, j = 0, k = 0;
    uint8 ch = 0;
    const uint8 *p_data;

    IPS200_CS(0);
    for(ch = 0; ch < number; ch++)
    {
        p_data = chinese_buffer + ch * 128;  // 每字符128字节(32行x4字节)
        ips200_set_region(x + ch * 32, y, x + ch * 32 + 31, y + 31);
        for(i = 0; i < 32; i++)  // 32行
        {
            for(k = 0; k < 4; k++)  // 每行4字节=32像素
            {
                uint8 temp = *p_data++;
                for(j = 0; j < 8; j++)
                {
                    ips200_line_buffer[k * 8 + j] = (temp & 0x80) ? color : ips200_bgcolor;
                    temp <<= 1;
                }
            }
            ips200_write_16bit_data_array(ips200_line_buffer, 32);   // 逐行发送，省下大块缓存
        }
    }
    IPS200_CS(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     2寸 IPS液晶初始化
// 备注信息     内部已经完成了 GPIO 控制脚 + SPI-A + 屏幕的初始化，
//              所以 main 里只要在 InitSysCtrl() 等系统初始化之后调用一次即可，
//              不需要（也不要）另外再配置 SPI。
//-------------------------------------------------------------------------------------------------------------------
void ips200_init(void)
{
    /* 1. 控制引脚初始化 (GPAMUX/GPADIR/GPAPUD 是受 EALLOW 保护的寄存器) */
    EALLOW;
    SysCtrlRegs.PCLKCR3.bit.GPIOINENCLK = 1;    /* 使能 GPIO 模块时钟 */
    IPS200_CS_INIT();
    IPS200_DC_INIT();
    IPS200_RST_INIT();
    IPS200_BLK_INIT();
    EDIS;

    /* 2. SPI-A 初始化 */
    ips200_spi_init();

    /* 3. 屏幕硬件复位时序 */
    IPS200_BLK(1);
    IPS200_RST(0);
    IPS200_DELAY_MS(5);                         /* 复位低电平保持 5ms */
    IPS200_RST(1);
    IPS200_DELAY_MS(120);                       /* 等待内部上电稳定 120ms */

    IPS200_CS(0);

    /* 先发一次软复位：就算 RES 那根线没接好/接错了，屏幕也能被复位一次
     * （很多模块把 SDA/SCL 走缓冲器，读不到 ID，但写命令是能收到的） */
    ips200_write_command(0x01);                 // Software reset
    IPS200_DELAY_MS(120);

    ips200_write_command(0x11);                 // Sleep out
    IPS200_DELAY_MS(120);

    ips200_write_madctl();                      // Memory access control (0x36)

    ips200_write_command(0x3A);                 // Interface pixel format
    ips200_write_8bit_data(0x05);               // 16bit/pixel

    ips200_write_command(0xB2);                 // Porch control
    ips200_write_8bit_data(0x0C);
    ips200_write_8bit_data(0x0C);
    ips200_write_8bit_data(0x00);
    ips200_write_8bit_data(0x33);
    ips200_write_8bit_data(0x33);

    ips200_write_command(0xB7);                 // Gate control
    ips200_write_8bit_data(0x35);

    ips200_write_command(0xBB);                 // VCOM setting
    ips200_write_8bit_data(0x29);

    ips200_write_command(0xC2);                 // Power control 1
    ips200_write_8bit_data(0x01);

    ips200_write_command(0xC3);                 // Power control 2
    ips200_write_8bit_data(0x19);

    ips200_write_command(0xC4);                 // VDV/VRH command enable
    ips200_write_8bit_data(0x20);

    ips200_write_command(0xC5);                 // VCOMH/VCOML control
    ips200_write_8bit_data(0x1A);

    ips200_write_command(0xC6);                 // Frame rate control
    ips200_write_8bit_data(0x0F);

    ips200_write_command(0xD0);                 // Power control 1
    ips200_write_8bit_data(0xA4);
    ips200_write_8bit_data(0xA1);

    ips200_write_command(0xE0);                 // Positive voltage gamma control
    ips200_write_8bit_data(0xD0);
    ips200_write_8bit_data(0x08);
    ips200_write_8bit_data(0x0E);
    ips200_write_8bit_data(0x09);
    ips200_write_8bit_data(0x09);
    ips200_write_8bit_data(0x05);
    ips200_write_8bit_data(0x31);
    ips200_write_8bit_data(0x33);
    ips200_write_8bit_data(0x48);
    ips200_write_8bit_data(0x17);
    ips200_write_8bit_data(0x14);
    ips200_write_8bit_data(0x15);
    ips200_write_8bit_data(0x31);
    ips200_write_8bit_data(0x34);

    ips200_write_command(0xE1);                 // Negative voltage gamma control
    ips200_write_8bit_data(0xD0);
    ips200_write_8bit_data(0x08);
    ips200_write_8bit_data(0x0E);
    ips200_write_8bit_data(0x09);
    ips200_write_8bit_data(0x09);
    ips200_write_8bit_data(0x15);
    ips200_write_8bit_data(0x31);
    ips200_write_8bit_data(0x33);
    ips200_write_8bit_data(0x48);
    ips200_write_8bit_data(0x17);
    ips200_write_8bit_data(0x14);
    ips200_write_8bit_data(0x15);
    ips200_write_8bit_data(0x31);
    ips200_write_8bit_data(0x34);

    ips200_write_command(0x21);                 // Display inversion on
    ips200_write_command(0x29);                 // Display on

    IPS200_CS(1);

    ips200_clear();

    ips200_inited = 1;                          /* 标记初始化完成 */
}


