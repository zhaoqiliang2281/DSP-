/*
 * Cla_Adc.h
 *
 *  Created on: 2026Äê10ÔÂ10ÈÕ
 *      Author: liang2281
 */

#ifndef USER_CLA_ADC_H_
#define USER_CLA_ADC_H_

#include "main.h"

#define NUM_DATA_POINTS 20

extern Uint16 ConversionCount;
extern Uint16 VoltageCLA[NUM_DATA_POINTS];

void Cla_adc_test_Init(void);
void Cla_adc_test_Loop(void);



#endif /* USER_CLA_ADC_H_ */
