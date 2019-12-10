/*
 * hw.c
 *
 *  Created on: 2019. 12. 10.
 *      Author: HanCheol Cho
 */




#include "hw.h"





void hwInit(void)
{
  bspInit();

  delayInit();
  millisInit();


  ledInit();
  uartInit();
}
