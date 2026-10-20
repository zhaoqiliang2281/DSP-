/*
 * UART.h
 *
 *  Created on: 2026年9月29日
 *      Author: liang2281
 */

#ifndef UART_H_
#define UART_H_

#include "main.h"
#include <string.h>
#include <stdint.h>

void InitSciA(void);
void Scia_SendByte(Uint16 dat);
void Serial_SendNumber(Uint32 Number, Uint8 Length);
void Scia_SendString(char *str);
Uint16 Scia_ReadByte(void);
Uint16 Scia_RxCount(void);
Uint16 Scia_LoopbackTest(void);

interrupt void Scia_RxIsr(void);
Uint16 Scia_GetFromRingBuf(void);

/* 主循环轮询兜底（不依赖中断），返回 1 = 本次轮询收到了数据 */
Uint16 Scia_PollRxFifo(void);
/* 读 GPIO28(SCIRXDA) 引脚实时电平，用于判断模块 TX 到底有没有接上 */
Uint16 Scia_GetRxPinLevel(void);
/* 诊断：读中断路径关键寄存器（索引见下面 SCI_DIAG_*） */
Uint16 Scia_DiagReg(Uint16 idx);

/* ---- 调试开关 ----
 * SCI_RX_POLL_FALLBACK = 1：主循环额外轮询 RX FIFO 兜底（能收数据，但每轮都会 DINT/EINT）
 *                      = 0：只靠中断（默认，接收完全由中断驱动）
 * 只有怀疑“中断没进来”时才改成 1，配合 sciRxIsrCount/sciRxPollCount 对比判断
 */
#define SCI_RX_POLL_FALLBACK   0

/* Scia_DiagReg() 的索引 */
#define SCI_DIAG_PIEACK    0U   /* PIEACK  期望 0；0x0100(256)=第9组没被应答，中断会卡死 */
#define SCI_DIAG_PIEIFR9   1U   /* PIEIFR9 9.1 有请求时=1 */
#define SCI_DIAG_PIEIER9   2U   /* PIEIER9 9.1 使能应为 1 */
#define SCI_DIAG_IER       3U   /* IER     应含 M_INT9 = 0x0100 */
#define SCI_DIAG_FFRX      4U   /* SCIFFRX bit5(RXFFIENA) 必须=1 */
#define SCI_DIAG_RXST      5U   /* SCIRXST 接收状态/错误标志 */

/* 调试用：RX 中断进入次数。一直为 0 说明中断没进来（非接线问题） */
extern volatile Uint32 sciRxIsrCount;
/* 调试用：主循环直接轮询到数据的次数。>0 说明引脚/接线是好的 */
extern volatile Uint32 sciRxPollCount;

extern Uint16 sciRxBuf[];
extern volatile Uint16 sciRxWrIdx;
extern volatile Uint16 sciRxRdIdx;

//#if SCI_RX_POLL_FALLBACK
//        (void)Scia_PollRxFifo();
//#endif
/* 调试用（需要时取消注释）：
 * ips200_show_int(60, 264, sciRxPollCount, 8);
 * ips200_show_int(60, 288, Scia_GetRxPinLevel(), 1);
 * ips200_show_int(140, 240, Scia_DiagReg(SCI_DIAG_PIEACK), 5);
 * ips200_show_int(140, 264, Scia_DiagReg(SCI_DIAG_PIEIFR9), 2);
 * ips200_show_int(140, 288, Scia_DiagReg(SCI_DIAG_IER), 6);
 */
#endif /* UART_H_ */
