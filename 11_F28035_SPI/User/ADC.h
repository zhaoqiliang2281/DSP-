/*
 * ADC.H
 *
 *  Created on: 2026Äê9ÔÂ28ÈÕ
 *      Author: liang2281
 */

#ifndef USER_ADC_H_
#define USER_ADC_H_

#include "main.h"

extern Uint16 adc_result0;
extern float adc_voltage;

void ADC_Init(void);
Uint16 ADC_Read_Channel(Uint16 ch);
void ADC_Get_value(void);

#endif /* USER_ADC_H_ */
