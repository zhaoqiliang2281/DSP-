/*
 * Watchdog.h
 *
 *  Created on: 2026Äê10ÔÂ6ÈÕ
 *      Author: liang2281
 */

#ifndef USER_WATCHDOG_H_
#define USER_WATCHDOG_H_


#include "main.h"

extern Uint32 Watchdog_WakeCount;
extern Uint32 Watchdog_LoopCount;

void Watchdog_Init(void);


#endif /* USER_WATCHDOG_H_ */
