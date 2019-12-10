/*
 * ap.cpp
 *
 *  Created on: 2019. 12. 10.
 *      Author: HanCheol Cho
 */




#include "ap.h"





void apInit(void)
{
  uartOpen(_DEF_UART1, 57600);
}

void apMain(void)
{
  uint32_t pre_time;


  while(1)
  {
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }

    if (uartAvailable(_DEF_UART1) > 0)
    {
      logPrintf("rx : 0x%X\n", uartRead(_DEF_UART1));
    }
  }
}

