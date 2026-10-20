/*
 * zf_device_ips200.h
 *
 *  Created on: 2026年9月25日
 *      Author: liang2281
 */

#ifndef USER_ZF_DEVICE_IPS200_H_
#define USER_ZF_DEVICE_IPS200_H_

#include "main.h"

//========================== 用户配置区域 ==========================

//-------------------------------------------------------------------------------------------------------------------
// 引脚编号（要换引脚，只改下面这 6 行！改完把液晶的线插到新编号对应的针上再烧录）
//
//   限制：只能用 GPIO0 ~ GPIO31（只有这 32 个脚有 GPASET / GPACLEAR 输出位）
//         6 个脚必须互不相同，也不能和别的外设撞车
//         本工程已被占用的脚：GPIO21(LED)、GPIO27+GPIO31(OLED 的 I2C)
//
//   当前接线（已用 LED 探针在模块端逐根实测，6 根全部正常）：
//     CS=GPIO19   DC=GPIO14   RST=GPIO22   BLK=GPIO23   SCL=GPIO18   SDA=GPIO16
//
//   ★ 曾经的坑：DC 原来写的 GPIO20，而底板上那根线并不在 GPIO20 上（悬空），
//     导致所有字节都被当成命令处理，表现为“花屏一下然后全黑”。改到 GPIO14 后正常。
//
//   改接线时只需改下面这 6 个数字，再把线插到新编号对应的针上。
//   备用可选的空闲脚：GPIO12 GPIO13 GPIO15 GPIO24 GPIO25 GPIO26 GPIO28 GPIO29
//-------------------------------------------------------------------------------------------------------------------
#define IPS200_PIN_CS       19      /* 片选：低电平有效              */
#define IPS200_PIN_DC       14      /* 数据/命令：高=数据，低=命令   */
#define IPS200_PIN_RST      22      /* 复位：低电平有效              */
#define IPS200_PIN_BLK      23      /* 背光：高电平=点亮（背光已知正常，这个脚尽量不要动）*/
#define IPS200_PIN_SCL      18      /* SPI 时钟                      */
#define IPS200_PIN_SDA      16      /* SPI 数据 (MOSI)               */

/* 底层：把 GPIO n 输出成 x（1=高，0=低）。GPASET/GPACLEAR 是数据寄存器，不受 EALLOW 保护
 * 说明：n 用“& 0x1F”夹到 0~31 再移位，一是让 n 可以传变采用，二是避免编译器报
 *       “shift count is too large/negative”（C 允许这种写法，但会报 warning）。 */
#define IPS200_PIN_OUT(n, x)    do { if (x) { GpioDataRegs.GPASET.all   = (Uint32)(1UL << ((n) & 0x1F)); } \
                                    else   { GpioDataRegs.GPACLEAR.all = (Uint32)(1UL << ((n) & 0x1F)); } } while(0)

/* 底层：把 GPIO n 配成普通推挽输出并关掉内部上拉。必须在 EALLOW / EDIS 之间调用！
 * 注意：GPAMUX1/GPAMUX2 的 .all 是 32 位（每个引脚占 2 位），不能用 Uint16 截断；
 *       移位量统一用 (n & 0x0F) 夹到 0~15，两种分支都合法，不会报越界移位警告。 */
#define IPS200_PIN_INIT(n)      do {                                                                          \
                                 if ((n) < 16) { GpioCtrlRegs.GPAMUX1.all &= ~((Uint32)3UL << (((n) & 0x0F) * 2)); } \
                                 else          { GpioCtrlRegs.GPAMUX2.all &= ~((Uint32)3UL << (((n) & 0x0F) * 2)); } \
                                 GpioCtrlRegs.GPADIR.all |= (Uint32)(1UL << ((n) & 0x1F));                          \
                                 GpioCtrlRegs.GPAPUD.all |= (Uint32)(1UL << ((n) & 0x1F)); } while(0)

#define IPS200_CS(x)     IPS200_PIN_OUT(IPS200_PIN_CS,  x)
#define IPS200_DC(x)     IPS200_PIN_OUT(IPS200_PIN_DC,  x)
#define IPS200_RST(x)    IPS200_PIN_OUT(IPS200_PIN_RST, x)
#define IPS200_BLK(x)    IPS200_PIN_OUT(IPS200_PIN_BLK, x)
#define IPS200_SCL(x)    IPS200_PIN_OUT(IPS200_PIN_SCL, x)
#define IPS200_SDA(x)    IPS200_PIN_OUT(IPS200_PIN_SDA, x)

/* 引脚初始化宏：必须在 EALLOW / EDIS 之间调用（GPAMUX/GPADIR/GPAPUD 是受保护寄存器） */
#define IPS200_CS_INIT()     do { IPS200_PIN_INIT(IPS200_PIN_CS);   IPS200_CS(1);  } while(0)
#define IPS200_DC_INIT()     do { IPS200_PIN_INIT(IPS200_PIN_DC);   IPS200_DC(0);  } while(0)
#define IPS200_RST_INIT()    do { IPS200_PIN_INIT(IPS200_PIN_RST);  IPS200_RST(1); } while(0)
#define IPS200_BLK_INIT()    do { IPS200_PIN_INIT(IPS200_PIN_BLK);  IPS200_BLK(0); } while(0)
#define IPS200_SCL_INIT()    do { IPS200_PIN_INIT(IPS200_PIN_SCL);  IPS200_SCL(1); } while(0)
#define IPS200_SDA_INIT()    do { IPS200_PIN_INIT(IPS200_PIN_SDA);  IPS200_SDA(1); } while(0)

//-------------------------------------------------------------------------------------------------------------------
// 底层 SPI 实现方式（两种方式用的是同一对引脚：SCL=GPIO18，SDA=GPIO16，接线不用动）
//   IPS200_USE_SOFT_SPI = 1 -> GPIO 模拟 SPI：稳，但慢（整屏刷新几秒）
//                       = 0 -> 硬件 SPI-A：快很多
//                              （2026-09-25 曾"全黑"，2026-10 已定位根因并修复：
//                               8bit 字符时 SPITXBUF 的数据必须左对齐，详见 .c 文件）
//-------------------------------------------------------------------------------------------------------------------
#define IPS200_USE_SOFT_SPI     0

// --- 显示默认参数 ---
#define IPS200_DEFAULT_DISPLAY_DIR      (IPS200_PORTAIT)
#define IPS200_DEFAULT_PENCOLOR         (RGB565_RED    )
#define IPS200_DEFAULT_BGCOLOR          (RGB565_WHITE  )
#define IPS200_DEFAULT_DISPLAY_FONT     (IPS200_8X16_FONT)

//========================== 颜色定义 ==========================
#ifndef RGB565_WHITE
#define RGB565_WHITE    0xFFFF
#endif
#ifndef RGB565_BLACK
#define RGB565_BLACK    0x0000
#endif
#ifndef RGB565_RED
#define RGB565_RED      0xF800
#endif
#ifndef RGB565_GREEN
#define RGB565_GREEN    0x07E0
#endif
#ifndef RGB565_BLUE
#define RGB565_BLUE     0x001F
#endif
#ifndef RGB565_YELLOW
#define RGB565_YELLOW   0xFFE0
#endif
#ifndef RGB565_CYAN
#define RGB565_CYAN     0x07FF
#endif
#ifndef RGB565_MAGENTA
#define RGB565_MAGENTA  0xF81F
#endif
#ifndef RGB565_GRAY
#define RGB565_GRAY     0x8410


#endif

/* 原写法 "#define 名字 = (值)" 语法错误，已修正为合法的宏定义 */
#define RGB565_39C5BB   0x3E37      /* RGB(0x39,0xC5,0xBB) -> ((7<<11)|(49<<5)|23) 初音绿 */
#define RGB565_66CCFF   0x667F      /* RGB(0x66,0xCC,0xFF) -> ((12<<11)|(51<<5)|31) 天依蓝 */

//========================== 数据类型 ==========================
// TMS320F28035 是 16bit 的定点 DSP：int = 16bit，long = 32bit
// 注意：int16 / int32 已经在 DSP2803x_Device.h 里用 DSP28_DATA_TYPES 定义过了，
//       如果再 typedef 一次会报 "redefinition of typedef"，所以这里要先判断。
#ifndef DSP28_DATA_TYPES
typedef int                 int16;          /* 有符号 16 bits */
typedef long                int32;          /* 有符号 32 bits */
#endif

#ifndef IPS200_DATA_TYPES
#define IPS200_DATA_TYPES
typedef unsigned char       uint8;          /* 无符号 8  bits (C28x 里 char 占 1 个字) */
typedef unsigned int        uint16;         /* 无符号 16 bits */
typedef unsigned long       uint32;         /* 无符号 32 bits */
#endif


//========================== 参数枚举 ==========================
typedef enum
{
    IPS200_PORTAIT          = 0,    // 竖屏模式
    IPS200_PORTAIT_180      = 1,    // 竖屏模式  旋转180
    IPS200_CROSSWISE        = 2,    // 横屏模式
    IPS200_CROSSWISE_180    = 3,    // 横屏模式  旋转180
} ips200_dir_enum;

typedef enum
{
    IPS200_6X8_FONT     = 0,        // 6x8  字体
    IPS200_8X16_FONT    = 1,        // 8x16 字体
    IPS200_16X16_FONT   = 2,        // 16x16 字体
    IPS200_32X32_FONT   = 3,        // 32x32 字体
} ips200_font_size_enum;

extern uint16 ips200_width_max;
extern uint16 ips200_height_max;

//========================== 内部缓存配置 ==========================
// C28x 编译器不支持变长数组(VLA)，驱动内部使用一个公用的行缓冲，
// 该值必须 >= 最大显示宽度(横屏 320)，同时也是刷图函数允许的最大宽度。
#define IPS200_LINE_BUFFER_SIZE         320

//========================== SPI 接口函数 (已在本工程中实现) ==========================
// TMS320F28035 使用硬件 SPI-A：GPIO16 = SPISIMOA (MOSI)，GPIO18 = SPICLKA (SCK)，
// 8bit 传输 + 16 级发送/接收 FIFO，收发都用查询方式，保证时序正确。
// 如果你想改用 SPI-B 或者 GPIO 模拟 SPI，只需要重写下面 4 个函数，其它代码不用动。
void    ips200_spi_write_8bit           (uint8 dat);
void    ips200_spi_write_16bit          (uint16 dat);
void    ips200_spi_write_8bit_array     (const uint8 *dat, uint32 len);
void    ips200_spi_write_16bit_array    (const uint16 *dat, uint32 len);

// 运行时切换 SPI 时钟模式 (CPOL, CPHA)，改完要重新调用一次 ips200_init()
//void    ips200_set_spi_mode             (uint8 cpol, uint8 cpha);

// SPI-A 回环自检：不依赖屏幕，用于区分「SPI/配置问题」和「接线/屏幕问题」
// 返回值 1 = 正常，0 = 异常
uint8   ips200_spi_loopback_test        (void);

// 自检用的诊断变量（先调用 ips200_spi_selftest()，再读这三个值）
extern uint16 ips200_diag_tx_after;     // 发 1 字节后 TX FIFO 剩余字数，0 = 已经移出去了
extern uint16 ips200_diag_rx_after;     // 同一时刻 RX FIFO 里的字数，>=1 = 接收链路正常
extern uint16 ips200_diag_fftx;         // SPIFFTX 回读值，bit14(0x4000) = FIFO 使能
extern uint16 ips200_diag_echo;         // 回环收到的第一个字节，0xFFFF = 没收到

// SPI-A 综合自检（内部会刷新上面的诊断变量）
void    ips200_spi_selftest             (void);

//========================== IPS200 显示函数 ==========================
// ips200_init() 内部已经包含 GPIO 控制脚初始化 + SPI-A 初始化 + 屏幕初始化，
// 所以 main 里只需要在 InitSysCtrl() 等系统初始化之后调用一次即可，
// 不需要（也不要）另外再去配置 SPI。
void    ips200_init                     (void);
void    ips200_clear                    (void);
void    ips200_full                     (const uint16 color);
void    ips200_set_dir                  (ips200_dir_enum dir);
void    ips200_set_font                 (ips200_font_size_enum font);
void    ips200_set_color                (const uint16 pen, const uint16 bgcolor);
void    ips200_draw_point               (uint16 x, uint16 y, const uint16 color);
void    ips200_draw_line                (uint16 x_start, uint16 y_start, uint16 x_end, uint16 y_end, const uint16 color);

void    ips200_show_char                (uint16 x, uint16 y, const char dat);
void    ips200_show_string              (uint16 x, uint16 y, const char dat[]);
void    ips200_show_int                 (uint16 x, uint16 y, const int32 dat, uint8 num);
void    ips200_show_uint                (uint16 x, uint16 y, const uint32 dat, uint8 num);
void    ips200_show_float               (uint16 x, uint16 y, const double dat, uint8 num, uint8 pointnum);

void    ips200_show_binary_image        (uint16 x, uint16 y, const uint8 *image, uint16 width, uint16 height, uint16 dis_width, uint16 dis_height);
void    ips200_show_gray_image          (uint16 x, uint16 y, const uint8 *image, uint16 width, uint16 height, uint16 dis_width, uint16 dis_height, uint8 threshold);
void    ips200_show_rgb565_image        (uint16 x, uint16 y, const uint16 *image, uint16 width, uint16 height, uint16 dis_width, uint16 dis_height, uint8 color_mode);

void    ips200_show_wave                (uint16 x, uint16 y, const uint16 *wave, uint16 width, uint16 value_max, uint16 dis_width, uint16 dis_value_max);
void    ips200_show_chinese             (uint16 x, uint16 y, uint8 size, const uint8 *chinese_buffer, uint8 number, const uint16 color);
void    ips200_show_chinese_32x32       (uint16 x, uint16 y, const uint8 *chinese_buffer, uint8 number, const uint16 color);


#endif /* USER_ZF_DEVICE_IPS200_H_ */
