/*
 * Serial.c
 *
 *  Created on: 2026年3月23日
 *      Author: LiangZhaoqi
 */

#include "Serial.h"
#include <stdint.h>
#include <stdlib.h>



uint8_t buffer1[Buffer1_Max];   /* 解析缓存：由 Serial_Data_Resolving 维护成合法的 C 字符串 */

int32_t m = 0;
int32_t x = 0;
int32_t y = 0;
int32_t z = 0;

/* 缓存里当前的有效字节数（取代原来的全局变量 i，避免和中断/其他代码抢同一个变量） */
static uint16_t s_line_len = 0U;
/* 最后一次收到字节时的 1ms 计数值：用来判断“数据是不是已经停下来了” */
static uint32_t s_last_rx_tick = 0UL;

/* 调试计数（可显示到屏幕上，用来判断数据到底有没有进到解析器） */
volatile uint32_t sciParseBytes = 0UL;   /* 已从环形缓冲区取走的字节数 */
volatile uint32_t sciParseOk    = 0UL;   /* 成功解析出的帧数 */

/************************************************
 *  函数名称: 跳过非数字字符（包括正负号）：char* skip_non_numeric_chars_int(const char* str)
 *  功能说明：
 *  参数说明：
 *  参   数：查找的字符串
 *  函数返回：返回第一个数字字符位置
 *  修改时间：2026年3月13日
 *  备   注：注意：已移除对 '.' 的支持，因为现在只解析整数
 ************************************************/
char* skip_non_numeric_chars_int(const char* str)
{
    while(*str != '\0')
    {
        if ((*str >= '0' && *str <= '9') || *str == '-' || *str == '+')
        {
            return (char*)str;
        }
        str++;
    }
    return (char*)str;
}

/************************************************
 *  函数名称: 解析串口接收到的"m整数x整数y整数z整数"格式数据：uint8_t Serial_Data_Resolving(int32_t *m, int32_t *x, int32_t *y, int32_t *z)
 *  功能说明：解析串口接收到的"m整数x整数y整数z整数"格式数据
 *  例如: "m1x68y99z88" -> m=1, x=68, y=99, z=88
 *  如果发送 "m1x68.8...", atoi 会截取为 68
 *  参数说明：
 *  参   数：m 输出参数：模式/ID值（整数）
 *  参   数：x 输出参数：X坐标值（整数）
 *  参   数：y 输出参数：Y坐标值（整数）
 *  参   数：z 输出参数：Z坐标值（整数)
 *  函数返回：成功解析标志位
 *  修改时间：2026年3月13日
 *  备   注：
 ************************************************/
/* ---------------------------------------------------------------------------
 * 在 line 里找一帧“m整数x整数y整数z整数”。
 *   成功：四个数写进输出参数，*next 指向本帧结尾之后，返回 1；
 *   失败（还没凑齐一帧）：返回 0。
 * 注意：只在“这一行已经完整收到”时才调用它，所以最后一串数字到行尾也算完整。
 * ------------------------------------------------------------------------- */
static uint8_t ParseOneFrame(char *line, char **next,
                             int32_t *pm, int32_t *px, int32_t *py, int32_t *pz)
{
    char *p_m;
    char *p_x;
    char *p_y;
    char *p_z;
    char *p_num;
    char *r;

    p_m = strchr(line, 'm');        if(p_m == NULL) { return 0U; }
    p_x = strchr(p_m + 1, 'x');     if(p_x == NULL) { return 0U; }
    p_y = strchr(p_x + 1, 'y');     if(p_y == NULL) { return 0U; }
    p_z = strchr(p_y + 1, 'z');     if(p_z == NULL) { return 0U; }

    p_num = skip_non_numeric_chars_int(p_z + 1);    /* z 后面第一个数字/符号 */
    if(*p_num == '\0') { return 0U; }               /* 数字还没到 */

    r = p_num;
    while(((*r >= '0') && (*r <= '9')) || (*r == '-') || (*r == '+') || (*r == '.'))
    {
        r++;                                        /* 找到数字串的结尾 */
    }

    *pm = (int32_t)atoi(skip_non_numeric_chars_int(p_m + 1));
    *px = (int32_t)atoi(skip_non_numeric_chars_int(p_x + 1));
    *py = (int32_t)atoi(skip_non_numeric_chars_int(p_y + 1));
    *pz = (int32_t)atoi(p_num);

    *next = r;
    return 1U;
}

/* ---------------------------------------------------------------------------
 * 把 buffer1 这一行里【所有】完整的帧都解析出来（一次发多帧也不会漏），
 * 解析不完整的尾巴留在 buffer1 里，下一批数据拼上再解析。
 * ------------------------------------------------------------------------- */
static uint8_t ProcessLine(int32_t *pm, int32_t *px, int32_t *py, int32_t *pz)
{
    char    *p = (char *)buffer1;
    char    *next;
    uint8_t  got = 0U;
    int32_t  v_m, v_x, v_y, v_z;
    uint16_t i;

    while(ParseOneFrame(p, &next, &v_m, &v_x, &v_y, &v_z))
    {
        *pm = v_m;
        *px = v_x;
        *py = v_y;
        *pz = v_z;
        sciParseOk++;
        got = 1U;
        p = next;
    }

    /* 没解析完的残留挪到缓冲区开头 */
    i = 0U;
    while(*p != '\0')
    {
        buffer1[i] = (uint8_t)(*p);
        i++;
        p++;
    }
    s_line_len = i;
    buffer1[s_line_len] = (uint8_t)'\0';

    return got;
}

uint8_t Serial_Data_Resolving(int32_t *m, int32_t *x, int32_t *y, int32_t *z)
{
    uint16_t ch;
    uint8_t  got = 0U;

    /* -----------------------------------------------------------------
     * 1) 收数据；换行不再作为“必须有”的条件，只是最可靠的分隔符：
     *      勾了“发送新行” -> 收到一行就解析一次，连续发送也不会漏；
     *      没勾           -> 交给下面第 2) 步的兜底逻辑。
     *    这里必须把一行里所有帧都解析掉，不能 break 一次只解析一帧，
     *    否则连续发送时永远追不上（之前“只能解析一遍”就是这个原因）。
     * ------------------------------------------------------------------ */
    while((ch = Scia_GetFromRingBuf()) != 0xFFFFU)
    {
        sciParseBytes++;
        s_last_rx_tick = (uint32_t)timer0_count;    /* 记下最后一次收到字节的时刻(ms) */

        if((ch == '\r') || (ch == '\n'))
        {
            if(s_line_len > 0U)
            {
                if(ProcessLine(m, x, y, z)) { got = 1U; }
                s_line_len = 0U;
            }
            continue;
        }

        if(s_line_len >= (uint16_t)(Buffer1_Max - 1U))
        {
            s_line_len = 0U;                /* 一行太长：乱数据，丢掉重来 */
        }

        buffer1[s_line_len] = (uint8_t)ch;
        s_line_len++;
        buffer1[s_line_len] = (uint8_t)'\0';    /* ★ 每写一个字节都保证有结束符 */
    }

    /* -----------------------------------------------------------------
     * 2) 兜底：串口助手没发换行时，靠“距离最后一个字节已经超过 1ms”
     *    来判断这一行收完了。
     *    必须用时间判断，不能用“这一轮长度没变”：主循环一旦跑得比
     *    字节间隔还快（刷屏节流之后就是这种情况），“长度没变”会在数据
     *    还在传的中途就成立，从而解析出半帧。
     * ------------------------------------------------------------------ */
    if((s_line_len > 0U) && ((uint32_t)(timer0_count - s_last_rx_tick) >= 1UL))
    {
        if(ProcessLine(m, x, y, z)) { got = 1U; }
        s_line_len = 0U;
    }

    return got;
}
