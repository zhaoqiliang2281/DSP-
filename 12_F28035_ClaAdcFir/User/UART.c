/*
 * UART.c  ——  SCI-A (TMS320F28035, 115200 8N1)
 *
 *  Created on: 2026年9月29日
 *      Author: liang2281
 *
 * ============================ 关键点（之前不通就是下面这几条） ============================
 *
 * 【1】引脚是芯片固定的，不能自己分配！
 *      F2803x(F28035) 的 SCI-A 只有一组复用脚：
 *          GPIO28 = SCIRXDA   接收输入(RX)   <- 接串口模块的 TX
 *          GPIO29 = SCITXDA   发送输出(TX)   -> 接串口模块的 RX
 *      接线务必“交叉”：DSP_RX(GPIO28) <- 模块TX，DSP_TX(GPIO29) -> 模块RX，并且共地。
 *
 * 【2】波特率：BRR = LSPCLK / (8 * 波特率) - 1
 *      InitPeripheralClocks() 里 LOSPCP = 2  ->  LSPCLK = SYSCLKOUT/4
 *      SYSCLKOUT = 60MHz (PLLCR=12, DIVSEL=2) ->  LSPCLK = 15MHz
 *      115200bps: BRR = 15000000 / (115200*8) - 1 = 15.27 -> 15
 *      实际波特率 = 15MHz/((15+1)*8) = 117187.5，误差 +1.7%，可用
 *      ★ 电脑端串口助手必须同样选 115200 8-N-1，否则全是乱码/收不到 ★
 *
 * 【3】中断要能进来，必须同时满足下面 4 条（少一条就永远收不到）：
 *      a. PieVectTable.SCIRXINTA 挂上自己的中断函数（漏了就进默认 ISR 死循环）
 *      b. SCIFFRX.RXFFIENA = 1（FIFO 模式下 RX 中断由它使能，不是 SCICTL2.RXBKINTENA）
 *      c. PIEIER9.INTx1 = 1
 *      d. IER |= M_INT9 且 INTM = 0（EINT）
 * ========================================================================================
 */
#include "UART.h"

#define SCI_RX_BUF_LEN  64U   //接收环形缓冲区长度

// 软件环形接收缓冲区
Uint16 sciRxBuf[SCI_RX_BUF_LEN];
volatile Uint16 sciRxWrIdx = 0U;
volatile Uint16 sciRxRdIdx = 0U;

/* 调试用：进入 RX 中断的次数。一直为 0 说明中断根本没进来
 * （向量表没挂 / RXFFIENA 没开 / PIEIER9 没开 / INTM 没清）。 */
volatile Uint32 sciRxIsrCount = 0UL;

/* 调试用：主循环直接轮询 RX FIFO 而拿到数据的次数。
 * 这个大于 0 → 引脚/接线/模块都是好的，问题只在中断路径上。 */
volatile Uint32 sciRxPollCount = 0UL;

/* 把 1 字节写进软件环形缓冲区（中断和轮询两条路共用） */
static void Sci_RingPush(Uint16 ch)
{
    sciRxBuf[sciRxWrIdx] = ch;
    sciRxWrIdx++;
    if(sciRxWrIdx >= SCI_RX_BUF_LEN)
    {
        sciRxWrIdx = 0U;
    }
}

/* 低速外设时钟 LSPCLK：由 LOSPCP=2 决定，LSPCLK = 60MHz/4 = 15MHz
 * （若以后改了 LOSPCP 或 SYSCLKOUT，这个宏要跟着改） */
#define SCI_LSPCLK_HZ   (15000000UL)
#define SCI_BAUD        (115200UL)
#define SCI_BRR         ((Uint16)(SCI_LSPCLK_HZ / (SCI_BAUD * 8UL) - 1UL))   /* 115200bps -> 15 */

// SCI-A初始化 115200 8N1
void InitSciA(void)
{
    EALLOW;
        /* 引脚复用：1 = SCI-A 功能 */
        GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;   /* GPIO28 -> SCIRXDA (接收) */
        GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;   /* GPIO29 -> SCITXDA (发送) */
        /* 方向：RX=输入，TX=输出（复用后方向由外设接管，写清楚更保险） */
        GpioCtrlRegs.GPADIR.bit.GPIO28  = 0;   /* RX 输入 */
        GpioCtrlRegs.GPADIR.bit.GPIO29  = 1;   /* TX 输出 */
        /* RX 脚使能内部上拉（线没插时不会乱收噪声），输入不做采样窗口限定(异步) */
        GpioCtrlRegs.GPAPUD.bit.GPIO28  = 0;   /* 0 = 使能内部上拉 */
        GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3;  /* 3 = 只同步到 SYSCLKOUT，不做 6 采样限定 */
    EDIS;

    SciaRegs.SCICCR.all = 0x0007;   /* 8位数据，1停止位，无校验，异步，不回环 */
    SciaRegs.SCICTL1.all = 0x0003;  /* 使能TX RX；SWRESET=0 先保持复位，配置完再释放 */

    SciaRegs.SCIHBAUD = (Uint16)((SCI_BRR >> 8) & 0x00FF);
    SciaRegs.SCILBAUD = (Uint16)(SCI_BRR & 0x00FF);        /* BRR = 15 -> 115200bps @LSPCLK=15MHz */

    SciaRegs.SCICTL2.bit.TXINTENA  = 0;  /* 关闭发送中断 */
    SciaRegs.SCICTL2.bit.RXBKINTENA= 1;  /* 老式(非FIFO)RXY中断使能，FIFO模式下不生效，留着无害 */

    SciaRegs.SCIFFTX.all = 0xE040;  /* 使能FIFO增强功能(bit14)，复位TX FIFO，清中断标志 */
    /* SCIFFRX: bit4:0 = RXFFIL(中断触发深度) = 1
     *          bit5   = RXFFIENA = 1  <<< 关键！FIFO 模式下 RX 中断靠它使能
     *          bit13  = RXFIFORESET = 1 复位 RX FIFO
     * 0x2021 = RXFIFORESET(0x2000) | RXFFIENA(0x0020) | RXFFIL(0x0001) */
    SciaRegs.SCIFFRX.all = 0x2021;  /* 收到 >=1 字节就产生 RX 中断 */
    SciaRegs.SCIFFCT.all = 0x0000;  /* 发送间隔延时0，不自动波特率 */

    SciaRegs.SCICTL1.bit.SWRESET = 1;  /* 最后释放SCI软件复位 */

    /* 清掉可能残留的 RX FIFO 中断/溢出标志，保证从“干净”状态起步，
     * 否则残留的中断请求会在使能 PIE 的瞬间立刻进来一次“假中断” */
    SciaRegs.SCIFFRX.bit.RXFFINTCLR = 1U;
    SciaRegs.SCIFFRX.bit.RXFFOVRCLR = 1U;

    //==== PIE 中断使能配置 ====
    /* ★ 把 Scia_RxIsr 挂进 PIE 向量表，否则 9.1 号中断还是默认的 SCIRXINTA_ISR
     *   （DSP2803x_DefaultIsr.c 里是 ESTOP0 + for(;;)，中断来了会卡死在那里） */
    EALLOW;
        PieVectTable.SCIRXINTA = &Scia_RxIsr;
    EDIS;

    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;       //使能PIE模块
    PieCtrlRegs.PIEIER9.bit.INTx1 = 1;       //PIE组9 INT1  SCIRXINTA
    IER |= M_INT9;                           //CPU打开INT9中断
    //IFR不用手动设置，中断应答会自动清除


    /* SCI 自检：内部回环（TX 在芯片内部直接接到 RX）发一个字节再收回来。
     * OK 说明 SCI 时钟/波特率/收发代码都没问题，剩下不通就是外部接线/串口助手的问题。 */
    if(Scia_LoopbackTest())
    {
        Scia_SendString("SCI self-test OK\r\n");
    }
    else
    {
        Scia_SendString("SCI self-test FAIL\r\n");
    }
    Scia_SendString("SCI Init OK, 115200 8N1\r\n");

}

//发送单字节：等待FIFO全部发送完成再发下一个，稳定简单
void Scia_SendByte(Uint16 dat)
{
    while(SciaRegs.SCIFFTX.bit.TXFFST != 0); //等待TX FIFO全部为空
    SciaRegs.SCITXBUF = (Uint16)(dat & 0x00FF);
}
Uint32 Serial_Pow(Uint32 X, Uint32 Y)
{
    Uint32 Result = 1;    //设置结果初值为1
    while (Y --)            //执行Y次
    {
        Result *= X;        //将Result乘X的Y次 就是X的Y次方
    }
    return Result;  //将X累乘到结果返回出去
}
void Serial_SendNumber(Uint32 Number, Uint8 Length)
{
    Uint8 i;
    for (i = 0; i < Length; i ++)       //根据数字长度遍历数字的每一位
    {
        Scia_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');    //依次调用Serial_SendByte发送每位数字
    }   //
}

//发送字符串
void Scia_SendString(char *str)
{
    while(*str != '\0')
    {
        Scia_SendByte((Uint16)(*str++));
    }
}

//读取接收字节
Uint16 Scia_ReadByte(void)
{
    return (Uint16)(SciaRegs.SCIRXBUF.all & 0x00FF);
}

//接收FIFO里还有几个字节（0 = 没收到数据）
Uint16 Scia_RxCount(void)
{
    return SciaRegs.SCIFFRX.bit.RXFFST;
}

/* ---------------------------------------------------------------------------
 * SCI 内部回环自检
 *   把 SCI 自己的 TX 在芯片内部接到 RX（不经过引脚），发一个 0x55 再收回来。
 *   返回 1 = SCI 模块/时钟/波特率/收发代码都正常，
 *            此时如果和电脑还是不通，就一定是外部问题：接线(收发是否交叉/共地)、
 *            电平转换(MAX3232/CH340 等)、或电脑端串口参数(波特率/COM口)。
 *   返回 0 = 芯片内部都不通，说明 SCI 配置或外设时钟有问题。
 * ------------------------------------------------------------------------- */
Uint16 Scia_LoopbackTest(void)
{
    Uint16  pass = 0;
    Uint32  to   = 0;
    Uint16  got  = 0;
    Uint16  rxIe = 0;

    /* 测试期间先关掉 RX FIFO 中断：否则中断会把 0x55 抢进软件环形缓冲区，
     * 这里轮询 FIFO 永远是空的，自检会误报 FAIL，而且那个 0x55 会在主循环里被回显出去 */
    rxIe = SciaRegs.SCIFFRX.bit.RXFFIENA;
    SciaRegs.SCIFFRX.bit.RXFFIENA = 0;

    SciaRegs.SCICCR.bit.LOOPBKENA = 1;    /* 打开内部回环 */

    SciaRegs.SCIFFRX.bit.RXFIFORESET = 0; /* 清空接收 FIFO */
    SciaRegs.SCIFFRX.bit.RXFIFORESET = 1;

    Scia_SendByte(0x0055);                /* 等 TX FIFO 空后发出 */

    while((SciaRegs.SCIFFRX.bit.RXFFST == 0) && (to < 3000000UL))
    {
        to++;                             /* 115200bps 一帧约 87us，这里给足约 100ms+ */
    }

    if(SciaRegs.SCIFFRX.bit.RXFFST != 0)
    {
        got = Scia_ReadByte();
        if(got == 0x0055)
        {
            pass = 1;
        }
    }

    while(SciaRegs.SCIFFRX.bit.RXFFST != 0) /* 清干净残余数据 */
    {
        (void)Scia_ReadByte();
    }

    SciaRegs.SCICCR.bit.LOOPBKENA = 0;    /* 关闭回环，恢复正常收发 */

    /* ★ 先把残留的 RX 中断标志清掉，再恢复 RX 中断使能。
     *   否则恢复使能的瞬间会因为残留请求立刻产生一次“假中断”。 */
    SciaRegs.SCIFFRX.bit.RXFFINTCLR = 1U;
    SciaRegs.SCIFFRX.bit.RXFFOVRCLR = 1U;
    SciaRegs.SCIFFRX.bit.RXFFIENA   = rxIe; /* 恢复 RX 中断使能 */

    return pass;
}

/**
 * 中断服务函数：Scia_RxIsr
 * FIFO收到数据进入此中断；把FIFO内全部字节读入软件环形缓冲区
 */
interrupt void Scia_RxIsr(void)
{
    Uint16 n,i;
    sciRxIsrCount++;                 //调试：中断进入次数
    n = SciaRegs.SCIFFRX.bit.RXFFST; //FIFO中有多少个字节

    for(i = 0U; i < n; i++)
    {
        Sci_RingPush(Scia_ReadByte());   //读一个字节并写入环形缓冲区
    }

    /* ★★ 顺序千万不能反：必须先清掉 SCI 自己的中断标志，再去应答 PIE。★★
     *
     * C28x 的 SCI 中断请求是“电平式”的：请求 = RXFFINT & RXFFIENA。
     * 如果 RXFFINT 还挂着就去写 PIEACK，PIE 会立刻认为“还有请求”，
     * 马上再进一次中断；而 ISR 又没清标志… 于是无限循环 —— 中断风暴，
     * CPU 永远出不了中断，主循环（屏幕）就死在那儿不动了。
     * 之前“中断只进 1 次，之后屏幕数字再也不变”就是这个原因。
     */
    SciaRegs.SCIFFRX.bit.RXFFOVRCLR = 1U;  //清FIFO溢出标志
    SciaRegs.SCIFFRX.bit.RXFFINTCLR = 1U;  //★清 RX FIFO 中断标志(写1清)

    //应答PIE第9组中断，必须写，否则不会再次进入中断
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

/* ---------------------------------------------------------------------------
 * 诊断：把中断路径上几个关键寄存器原样读出来，显示到屏幕上用。
 * 用途：分清“中断没进”到底是哪一道门关着。
 * ------------------------------------------------------------------------- */
Uint16 Scia_DiagReg(Uint16 idx)
{
    Uint16 v = 0U;

    switch(idx)
    {
    case SCI_DIAG_PIEACK:  v = (Uint16)(PieCtrlRegs.PIEACK.all);   break; /* 期望 0；0x0100=组9没被应答(卡住) */
    case SCI_DIAG_PIEIFR9: v = (Uint16)(PieCtrlRegs.PIEIFR9.all);  break; /* 9.1 有请求时=1 */
    case SCI_DIAG_PIEIER9: v = (Uint16)(PieCtrlRegs.PIEIER9.all);  break; /* 9.1 使能应为 1 */
    case SCI_DIAG_IER:     v = (Uint16)(IER);                      break; /* 应含 M_INT9(0x0100) */
    case SCI_DIAG_FFRX:    v = (Uint16)(SciaRegs.SCIFFRX.all);     break; /* bit5=RXFFIENA 必须=1 */
    case SCI_DIAG_RXST:    v = (Uint16)(SciaRegs.SCIRXST.all);     break; /* 错误/状态标志 */
    default:               v = 0xFFFFU;                            break;
    }

    return v;
}

/* ---------------------------------------------------------------------------
 * 【诊断/兜底】主循环直接轮询 RX FIFO，不依赖中断
 *   中断正常时这里几乎读不到东西（数据被 ISR 先抢走了），属正常；
 *   如果这个计数在涨，就证明“PC 发过来的数据确实到了 SCI”：
 *       计数涨 + sciRxIsrCount 不涨  -> 数据到了，但中断没进（中断配置问题）
 *       两个计数都不涨              -> 数据根本没进芯片：接线/模块/引脚问题
 * ------------------------------------------------------------------------- */
Uint16 Scia_PollRxFifo(void)
{
    Uint16 got = 0U;

    DINT;   /* 短暂关中断，避免和 ISR 抢同一个 RX FIFO / 环形缓冲区 */
    while(SciaRegs.SCIFFRX.bit.RXFFST != 0U)
    {
        Sci_RingPush(Scia_ReadByte());
        got = 1U;
    }
    if(got != 0U)
    {
        sciRxPollCount++;
    }
    EINT;

    return got;
}

/* 诊断用：直接读 GPIO28(SCIRXDA) 引脚的实时电平。
 * 即使该脚已复用给 SCI，GPADAT 仍反映引脚实际电平。
 * 串口线空闲时是 mark 状态 = 3.3V 高电平 -> 应读回 1；
 * 恒为 0：线被拉死/模块串口未供电；恒为 1：模块 TX 根本没接到 GPIO28（悬空） */
Uint16 Scia_GetRxPinLevel(void)
{
    return (Uint16)(GpioDataRegs.GPADAT.bit.GPIO28);
}

/**
 * 主循环调用，从环形缓冲区取1字节
 * 返回：0xFFFF 代表无数据；否则为收到字节
 */
Uint16 Scia_GetFromRingBuf(void)
{
    Uint16 ret = 0xFFFFU;
    if(sciRxWrIdx != sciRxRdIdx)
    {
        ret = sciRxBuf[sciRxRdIdx];
        sciRxRdIdx++;
        if(sciRxRdIdx >= SCI_RX_BUF_LEN)
        {
            sciRxRdIdx = 0U;
        }
    }
    return ret;
}
