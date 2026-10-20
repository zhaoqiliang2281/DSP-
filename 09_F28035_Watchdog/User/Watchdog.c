/*
 * Watchdog.c
 *
 *  Created on: 2026年10月6日
 *      Author: liang2281
 */

#include "Watchdog.h"

Uint32 WakeCount;
Uint32 LoopCount;

__interrupt void wakeint_isr(void);

void Watchdog_Init(void)
{
    EALLOW; // This is needed to write to EALLOW protected registers
    PieVectTable.WAKEINT = &wakeint_isr;
    EDIS;   // This is needed to disable write to EALLOW protected registers
    //
    // Connect the watchdog to the WAKEINT interrupt of the PIE
    // Write to the whole SCSR register to avoid clearing WDOVERRIDE bit
    //
    EALLOW;
    // WDENINT = 0x0000 -> a watchdog timeout drives a device reset.
    // (WDENINT = BIT1 would generate the WAKEINT interrupt instead.)
    SysCtrlRegs.SCSR = 0x0000;
    EDIS;

    //
    // Enable WAKEINT in the PIE: Group 1 interrupt 8
    // Enable INT1 which is connected to WAKEINT:
    // (WDENINT = 0 means WAKEINT is never generated; kept so that
    //  switching back to interrupt mode is a one-line change.)
    //
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;   // Enable the PIE block
//    - WDENINT = 1：溢出 → 触发 WAKEINT 中断（本例模式）
//    - WDENINT = 0：溢出 → **直接硬件复位 DSP（工程常用模式）
    PieCtrlRegs.PIEIER1.bit.INTx8 = 1;   // Enable PIE Group 1 INT8
    IER |= M_INT1;                       // Enable CPU INT1
    EINT;                                // Enable Global Interrupts

    //
    // Reset the watchdog counter
    //
    ServiceDog();

    //
    // Enable the watchdog
    //
    EALLOW;

    // 0x0028：二进制00101000，WDPS=000，分频 1，超时≈13.1ms
    // 0x0029：二进制00101001，WDPS=001，分频 1，超时≈13.1ms
    // 0x002A：二进制00101010，WDPS=010，分频 2，超时≈26.2ms
    // 0x002B：二进制00101011，WDPS=011，分频 4，超时≈52.4ms
    // 0x002C：二进制00101100，WDPS=100，分频 8，超时≈104.9ms
    // 0x002D：二进制00101101，WDPS=101，分频 16，超时≈209.7ms
    // 0x002E：二进制00101110，WDPS=110，分频 32，超时≈419.4ms
    // 0x002F：二进制00101111，WDPS=111，分频 64，超时≈838.9ms
    //
    // 说明：WDCR bit5~bit3必须固定为101（WDCHK=101），看门狗才能正常使能；bit6=0代表WDDIS=0，开启看门狗。
    // 公式：T_timeout = 256 * 512 / INTOSC1 * WDPS分频，INTOSC1典型10MHz，为理论值，内部振荡器存在偏差
    // 0x0028 超时仅13.1ms，小于主循环单次执行时间（2个ips200_show_int软件SPI刷屏，一轮约30ms），来不及喂狗直接溢出
    SysCtrlRegs.WDCR = 0x002B;

//    1. WDCHK 位（bit5~3）必须写`101`，这是硬件强制要求，不满足该值看门狗无法开启。
//       - `0x002X` 代表 WDCHK=101，WDDIS=0，看门狗开启；
//       - `0x000X / 0x001X`  WDCHK 不是 101，**无法启用看门狗，不要使用**。
//    2. `0x0026` 和 `0x002E` 两者 WDPS 都是 110，只是高位保留位不同，**功能完全一样**；同理`0x0027`和`0x002F`功能一致。工程一般习惯用`0x002E`、`0x0027`。
//    3. 计算公式：\(T_{timeout}=256\times\frac{512}{10000000}\times WDPS_{div}\)，基于 INTOSC1 典型 10MHz，为理论计算值，内部振荡器存在少量误差。
//    bit7: 0 (保留/WDFLAG，写0)
//    bit6: WDDIS = 0    → 0=使能看门狗
//    bit5~3: WDCHK = 101 → 固定校验位，必须写101才能打开看门狗
//    bit2~0: WDPS = 110 → 分频系数 = 32
    EDIS;



    //
    // Clear the counters
    //
    WakeCount = 0;          // Count interrupts
    LoopCount = 0;          // Count times through idle loop

}

//
// wakeint_isr
//
__interrupt void wakeint_isr(void)
{
    WakeCount++;

    //
    // Acknowledge this interrupt to get more from group 1
    //
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

