/**
 * @file vofa.h
 * @brief VOFA FireWater protocol interface.
 * @date 2026-05-22
 */
#ifndef __VOFA_H
#define __VOFA_H

#include "main.h"

typedef unsigned char bool_t;

void vofa_init(void);
bool_t vofa_send_samples(const float *data, uint8_t count);
bool_t vofa_send_tagged_samples(const char *tag, const float *data, uint8_t count);
bool_t vofa_printf(const char *fmt, ...);
void telemetry_init(void);
void telemetry_process(void);

/* Legacy-compatible wrappers */
void vofa_start(void);
void vofa_send_data(uint8_t num, float data);
void vofa_sendframetail(void);
void vofa_demo(void);

#endif /* __VOFA_H */
