/*
 * millis.c
 *
 *  Created on: 2019. 12. 10.
 *      Author: HanCheol Cho
 */




#include "millis.h"



bool millisInit(void)
{
  return true;
}

uint32_t millis(void)
{
  return HAL_GetTick();
}

