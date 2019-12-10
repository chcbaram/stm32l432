/*
 * hw_def.h
 *
 *  Created on: 2019. 12. 10.
 *      Author: HanCheol Cho
 */

#ifndef SRC_HW_HW_DEF_H_
#define SRC_HW_HW_DEF_H_


#include "def.h"
#include "bsp.h"


#define _USE_LOG_PRINTF     1

#if _USE_LOG_PRINTF == 1
#define logPrintf(fmt,...)  printf(fmt,##__VA_ARGS__)
#else
#define logPrintf(fmt,...)
#endif




#define _USE_HW_DELAY
#define _USE_HW_MILLIS



#define _USE_HW_LED
#define      HW_LED_MAX_CH        1

#define _USE_HW_UART
#define      HW_UART_MAX_CH       1



#endif /* SRC_HW_HW_DEF_H_ */
