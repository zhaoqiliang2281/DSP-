/**
 * @file vofa.c
 * @brief VOFA FireWater 协议和 F28035 板级遥测实现。
 */
#include "vofa.h"      // 引入 VOFA 接口声明和工程公共类型。
#include "UART.h"      // 引入 SCI-A 字节发送函数声明。
#include <stdarg.h>    // 引入可变参数列表类型和操作宏。
#include <stdio.h>     // 引入 snprintf 和 vsnprintf 格式化函数。

/* FireWater 文本帧缓冲区容量，包含字符串结束符和结尾换行符。 */
#define VOFA_FIREWATER_TX_BUF_SIZE      256
/* 旧版 VOFA 接口可缓存的最大通道数。 */
#define VOFA_FIREWATER_LEGACY_CH_MAX    16
/* telemetry_process 每调用 5 次发送一帧遥测数据。 */
#define TELEMETRY_SEND_DIV              5
/* 每帧包含主循环计数、三个定时器和一个 ADC 电压，共 5 个通道。 */
#define TELEMETRY_CHANNEL_COUNT         5

static float g_vofa_legacy_samples[VOFA_FIREWATER_LEGACY_CH_MAX]; // 保存旧接口逐通道写入的数据。
static uint8_t g_vofa_legacy_channel_count = 0; // 记录旧接口当前已写入的最高通道数。
static uint8_t g_telemetry_send_div_count = 0; // 累计遥测处理调用次数以实现发送分频。

/* 采集 F28035 当前可用的主循环、定时器和 ADCINA0 数据。 */
static void telemetry_sample_f28035(float *channels) // channels 指向本帧的浮点通道数组。
{
  Uint16 adc_raw; // 保存 ADCINA0 的原始转换结果。

  channels[0] = (float)num; // 通道 0：主循环计数。
  channels[1] = (float)timer0_count; // 通道 1：定时器 0 计数。
  channels[2] = (float)timer1_count; // 通道 2：定时器 1 计数。
  channels[3] = (float)timer2_count; // 通道 3：定时器 2 计数。

  adc_raw = ADC_Read_Channel(0); // 触发并读取 ADCINA0。
  channels[4] = (adc_raw == 0xFFFFU) ? -1.0f : (float)adc_raw * 3.3f / 4095.0f; // 通道 4：转换为电压，超时则写入 -1。
} // 结束 F28035 遥测采样。

/* 将遥测发送分频计数器复位到初始状态。 */
void telemetry_init(void) // 在系统启动或重新初始化时调用。
{
  g_telemetry_send_div_count = 0; // 清除之前累计的调用次数。
} // 结束遥测初始化。

/* 周期调用本函数；每累计 5 次采样并发送一帧。 */
void telemetry_process(void) // 一般在主循环或周期任务中调用。
{
  float channels[TELEMETRY_CHANNEL_COUNT]; // 为本帧的 5 个遥测通道分配临时缓冲区。

  if (g_telemetry_send_div_count < TELEMETRY_SEND_DIV) // 计数未达到发送分频值时继续累计。
  {
    g_telemetry_send_div_count++; // 记录本次调用。
    if (g_telemetry_send_div_count < TELEMETRY_SEND_DIV) // 尚未到发送时机。
    {
      return; // 提前返回，不采样也不发送。
    } // 结束发送时机判断。
  } // 结束分频判断。

  telemetry_sample_f28035(channels); // 读取当前板级数据并填充通道数组。
  if (vofa_send_tagged_samples("F28035", channels, TELEMETRY_CHANNEL_COUNT)) // 以 F28035 标签发送这 5 个通道。
  {
    g_telemetry_send_div_count = 0; // 发送成功后重新开始计数。
  } // 发送失败时保留计数，下一次调用继续尝试。
} // 结束一次遥测处理。

/* 判断标签是否可用于普通 FireWater 数据帧。 */
static bool_t vofa_is_tag_valid(const char *tag) // 检查标签是否为空或属于保留名称。
{
  return (tag != 0) && (tag[0] != '\0') && (strcmp(tag, "image") != 0); // 返回标签有效性判断结果。
} // 结束标签检查。

/* 补齐行结束符，并将整行文本通过 SCI-A 逐字节发送。 */
static bool_t vofa_send_text_line(char *buf, int32_t len) // buf 指向待发送文本，len 为有效文本长度。
{
  uint16_t index; // 用于遍历每个待发送字符。

  if ((buf == 0) || (len <= 0)) // 拒绝空指针或空文本。
  {
    return 0; // 返回发送失败。
  } // 结束输入检查。

  if (buf[len - 1] != '\n') // 判断文本是否已经有换行结尾。
  {
    if (len >= (VOFA_FIREWATER_TX_BUF_SIZE - 1)) // 检查追加换行符后是否会越界。
    {
      return 0; // 缓冲区没有空间时拒绝发送。
    } // 结束缓冲区空间检查。

    buf[len]     = '\n'; // 在文本末尾添加换行符。
    buf[len + 1] = '\0'; // 写入字符串结束符。
    len += 1; // 将换行符计入待发送长度。
  } // 结束换行符补齐。

  for (index = 0; index < (uint16_t)len; index++) // 按顺序遍历整行的所有字节。
  {
    Scia_SendByte((Uint16)(uint8_t)buf[index]); // 通过 SCI-A 发送当前字节。
  } // 结束逐字节发送。

  return 1; // 返回发送成功。
} // 结束文本行发送。

/* 清空旧接口缓存和遥测状态；调用前应先初始化 SCI-A。 */
void vofa_init(void) // 在系统启动时调用一次。
{
  g_vofa_legacy_channel_count = 0; // 清除旧接口缓存中的有效通道数。
  telemetry_init(); // 同时重置遥测分频状态。
} // 结束 VOFA 初始化。

/* 发送不带标签的浮点通道数组。 */
bool_t vofa_send_samples(const float *data, uint8_t count) // data 为通道数组，count 为通道数量。
{
  return vofa_send_tagged_samples(0, data, count); // 复用带标签发送函数并传入空标签。
} // 结束无标签通道发送。

/* 将标签和浮点通道格式化为 FireWater 文本帧后发送。 */
bool_t vofa_send_tagged_samples(const char *tag, const float *data, uint8_t count) // tag 可为空，data/count 指定通道内容。
{
  char tx_buf[VOFA_FIREWATER_TX_BUF_SIZE]; // 保存格式化后的文本帧。
  int32_t write_len; // 记录当前帧已写入的字符数。
  uint8_t index; // 遍历输入通道数组的索引。

  if ((data == 0) || (count == 0)) // 检查通道指针和通道数量。
  {
    return 0; // 没有有效通道数据时返回失败。
  } // 结束通道输入检查。

  if (vofa_is_tag_valid(tag)) // 仅为有效标签写入标签前缀。
  {
    write_len = snprintf(tx_buf, sizeof(tx_buf), "%s:", tag); // 将标签和冒号写入帧缓冲区。
    if ((write_len <= 0) || (write_len >= VOFA_FIREWATER_TX_BUF_SIZE)) // 检查标签格式化结果和缓冲区容量。
    {
      return 0; // 标签写入失败或溢出时拒绝发送。
    } // 结束标签写入检查。
  } // 结束有效标签处理。
  else // 标签为空、空字符串或保留的 image 标签。
  {
    write_len = 0; // 从帧缓冲区起始位置直接写入通道数据。
  } // 结束无标签处理。

  /* 按顺序追加浮点通道；发生格式化错误或溢出时放弃整帧。 */
  for (index = 0; index < count; index++) // 遍历所有待发送通道。
  {
    int32_t remain_len = VOFA_FIREWATER_TX_BUF_SIZE - write_len; // 计算当前剩余缓冲区空间。
    int32_t append_len = snprintf(&tx_buf[write_len], (uint32_t)remain_len, // 将当前通道追加到缓冲区末尾。
                                  (index == 0) ? "%.6f" : ",%.6f", data[index]); // 首通道不加逗号，其余通道以逗号分隔并保留 6 位小数。

    if ((append_len <= 0) || (append_len >= remain_len)) // 检查格式化结果是否失败或超出剩余空间。
    {
      return 0; // 任何一个通道写入失败都取消整帧发送。
    } // 结束格式化结果检查。

    write_len += append_len; // 更新已写入的帧长度。
  } // 结束通道数组遍历。

  return vofa_send_text_line(tx_buf, write_len); // 补齐换行符并发送整帧。
} // 结束带标签通道发送。

/* 使用 printf 风格格式化字符串并发送一行文本。 */
bool_t vofa_printf(const char *fmt, ...) // fmt 为格式字符串，后续参数按格式传入。
{
  char tx_buf[VOFA_FIREWATER_TX_BUF_SIZE]; // 暂存格式化后的输出文本。
  va_list args; // 保存可变参数列表。
  int32_t write_len = 0; // 保存格式化结果长度。

  if (fmt == 0) // 检查格式字符串指针。
  {
    return 0; // 格式字符串为空时返回失败。
  } // 结束格式字符串检查。

  va_start(args, fmt); // 初始化可变参数读取。
  write_len = vsnprintf(tx_buf, VOFA_FIREWATER_TX_BUF_SIZE, fmt, args); // 将格式化结果写入有限长度缓冲区。
  va_end(args); // 释放可变参数列表状态。

  if ((write_len <= 0) || (write_len >= VOFA_FIREWATER_TX_BUF_SIZE)) // 检查格式化失败、空结果或缓冲区溢出。
  {
    return 0; // 结果无效时不发送。
  } // 结束格式化结果检查。

  return vofa_send_text_line(tx_buf, write_len); // 补齐换行符并发送格式化文本。
} // 结束格式化文本发送。

/* 保留旧版启动接口，并转调统一初始化函数。 */
void vofa_start(void) // 兼容旧代码中的启动函数调用。
{
  vofa_init(); // 使用统一初始化逻辑。
} // 结束旧版启动接口。

/* 暂存旧版接口的一个通道值，稍后由帧尾函数统一发送。 */
void vofa_send_data(uint8_t num, float data) // num 为通道下标，data 为该通道数据。
{
  if (num >= VOFA_FIREWATER_LEGACY_CH_MAX) // 检查通道下标是否超出缓存容量。
  {
    return; // 超出容量时忽略该通道数据。
  } // 结束通道下标检查。

  g_vofa_legacy_samples[num] = data; // 保存指定通道的数据。
  if (num >= g_vofa_legacy_channel_count) // 判断是否需要扩展当前有效通道数。
  {
    g_vofa_legacy_channel_count = (uint8_t)(num + 1); // 更新为最高下标加一。
  } // 结束有效通道数更新。
} // 结束旧版通道数据暂存。

/* 发送旧版接口已暂存的通道数据，并清空本帧通道计数。 */
void vofa_sendframetail(void) // 在旧版接口写完一帧数据后调用。
{
  if (g_vofa_legacy_channel_count == 0) // 检查是否有通道数据待发送。
  {
    return; // 没有数据时直接返回。
  } // 结束空帧检查。

  vofa_send_samples(g_vofa_legacy_samples, g_vofa_legacy_channel_count); // 发送从通道 0 到最高有效通道的数据。
  g_vofa_legacy_channel_count = 0; // 清空本帧有效通道数，准备下一帧。
} // 结束旧版帧发送。

/* 生成递增测试数据，并以 demo 标签发送四个通道。 */
void vofa_demo(void) // 调用一次便生成并发送一帧测试数据。
{
  static float demo_t = 0.0f; // 保存跨调用递增的测试基准值。
  float demo_data[4]; // 保存本次生成的四个测试通道。

  demo_t += 0.1f; // 每次调用将测试基准值增加 0.1。
  demo_data[0] = demo_t; // 通道 0：基准值。
  demo_data[1] = demo_t * 0.5f; // 通道 1：基准值的一半。
  demo_data[2] = demo_t * -0.25f; // 通道 2：基准值的负四分之一。
  demo_data[3] = demo_t * -0.5f; // 通道 3：基准值的负一半。

  vofa_send_tagged_samples("demo", demo_data, 4); // 发送带 demo 标签的四通道测试帧。
} // 结束测试帧生成和发送。
