/*
 * led.h
 *
 *  Created on: 2019. 12. 10.
 *      Author: HanCheol Cho
 */

#ifndef SRC_COMMON_HW_INCLUDE_LED_H_
#define SRC_COMMON_HW_INCLUDE_LED_H_


#ifdef __cplusplus
 extern "C" {
#endif


#include "hw_def.h"

#ifdef _USE_HW_LED

#define LED_MAX_CH       HW_LED_MAX_CH


bool ledInit(void);
bool ledIsInit(void);
void ledOn(uint8_t ch);
void ledOff(uint8_t ch);
void ledToggle(uint8_t ch);
void ledSetTime(uint8_t ch, uint32_t on_time, uint32_t off_time);

#endif


#ifdef __cplusplus
}
#endif


#endif /* SRC_COMMON_HW_INCLUDE_LED_H_ */
