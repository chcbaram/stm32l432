/*
 * uart.c
 *
 *  Created on: 2019. 12. 10.
 *      Author: HanCheol Cho
 */




#include "uart.h"
#include "qbuffer.h"



/*
 * _DEF_UART1  : UART2
*/


/*
  _DEF_UART1
      USART1
        - RX : DMA1_Stream0
        - TX :
  */



#define UART_MODE_POLLING       0
#define UART_MODE_INTERRUPT     1
#define UART_MODE_DMA           2
#define UART_MODE_VCP           3

#define UART_HW_NONE            0
#define UART_HW_STM32_VCP       1
#define UART_HW_STM32_UART      2
#define UART_HW_MAX14830        3


#define UART_RX_BUF_LENGTH      2048




typedef struct
{
  bool     is_open;
  uint8_t  ch;
  uint32_t baud;
  uint8_t  tx_mode;
  uint8_t  rx_mode;
  uint8_t  hw_driver;

  uint8_t  rx_buf[UART_RX_BUF_LENGTH];

  qbuffer_t qbuffer_rx;

  DMA_HandleTypeDef  *hdma_tx;
  DMA_HandleTypeDef  *hdma_rx;
  UART_HandleTypeDef *handle;

  bool  tx_done;
  void  (*txDoneISR)(void);

  uint32_t err_cnt;
} uart_t;



static uart_t uart_tbl[UART_MAX_CH];


UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;



void uartStartRx(uint8_t channel);
void uartRxHandler(uint8_t channel);



bool uartInit(void)
{
  uint8_t i;


  for (i=0; i<UART_MAX_CH; i++)
  {
    uart_tbl[i].is_open  = false;
    uart_tbl[i].rx_mode  = UART_MODE_POLLING;
    uart_tbl[i].tx_mode  = UART_MODE_POLLING;
    uart_tbl[i].tx_done  = false;
    uart_tbl[i].txDoneISR = NULL;
    uart_tbl[i].err_cnt  = 0;
    uart_tbl[i].hw_driver = UART_HW_NONE;
  }

  return true;
}

bool uartOpen(uint8_t channel, uint32_t baud)
{
  bool ret = false;
  uart_t *p_uart;


  if (channel >= UART_MAX_CH)
  {
    return false;
  }

  switch(channel)
  {
    case _DEF_UART1:
      p_uart = &uart_tbl[channel];

      p_uart->baud      = baud;
      p_uart->hw_driver = UART_HW_STM32_UART;
      p_uart->rx_mode   = UART_MODE_DMA;
      p_uart->tx_mode   = UART_MODE_POLLING;

      p_uart->hdma_rx = &hdma_usart2_rx;
      p_uart->handle =  &huart2;
      p_uart->handle->Instance = USART2;

      p_uart->handle->Init.BaudRate     = baud;
      p_uart->handle->Init.WordLength   = UART_WORDLENGTH_8B;
      p_uart->handle->Init.StopBits     = UART_STOPBITS_1;
      p_uart->handle->Init.Parity       = UART_PARITY_NONE;
      p_uart->handle->Init.Mode         = UART_MODE_TX_RX;
      p_uart->handle->Init.HwFlowCtl    = UART_HWCONTROL_NONE;
      p_uart->handle->Init.OverSampling = UART_OVERSAMPLING_16;
      p_uart->handle->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
      p_uart->handle->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;


      qbufferCreate(&p_uart->qbuffer_rx, p_uart->rx_buf, UART_RX_BUF_LENGTH);

      HAL_UART_DeInit(p_uart->handle);
      HAL_UART_Init(p_uart->handle);

      p_uart->is_open  = true;

      uartStartRx(channel);
      ret = true;
      break;
  }

  return ret;
}

void uartSetTxDoneISR(uint8_t channel, void (*func)(void))
{
  uart_tbl[channel].txDoneISR = func;
}

void uartStartRx(uint8_t channel)
{
  uart_t *p_uart = &uart_tbl[channel];

  if (p_uart->rx_mode == UART_MODE_INTERRUPT)
  {
    HAL_UART_Receive_IT(p_uart->handle, p_uart->rx_buf, 1);
  }
  if (p_uart->rx_mode == UART_MODE_DMA)
  {
    HAL_UART_Receive_DMA(p_uart->handle, (uint8_t *)p_uart->qbuffer_rx.p_buf, p_uart->qbuffer_rx.length);
  }
}

bool uartClose(uint8_t channel)
{
  bool ret = false;


  if (channel >= UART_MAX_CH)
  {
    return false;
  }


  if (uart_tbl[channel].is_open == true && uart_tbl[channel].hw_driver == UART_HW_STM32_UART)
  {
    if(HAL_UART_DeInit(uart_tbl[channel].handle) == HAL_OK)
    {
      ret = true;
    }
  }


  return ret;
}

uint32_t uartAvailable(uint8_t channel)
{
  uint32_t ret;
  uart_t *p_uart = &uart_tbl[channel];


  if (channel >= UART_MAX_CH)
  {
    return 0;
  }

#ifdef _USE_HW_VCP
  if (p_uart->rx_mode == UART_MODE_VCP)
  {
    ret = vcpAvailable();
  }
#endif
  if (p_uart->rx_mode == UART_MODE_INTERRUPT)
  {
    ret = qbufferAvailable(&uart_tbl[channel].qbuffer_rx);
  }
  if (p_uart->rx_mode == UART_MODE_DMA)
  {
    p_uart->qbuffer_rx.ptr_in = p_uart->qbuffer_rx.length - ((DMA_Channel_TypeDef *)p_uart->hdma_rx->Instance)->CNDTR;
    ret = qbufferAvailable(&p_uart->qbuffer_rx);
  }

  return ret;
}

void uartFlush(uint8_t channel)
{
#ifdef _USE_HW_VCP
  if (uart_tbl[channel].rx_mode == UART_MODE_VCP)
  {
    vcpFlush();
  }
#endif
  if (uart_tbl[channel].rx_mode == UART_MODE_INTERRUPT)
  {
    qbufferFlush(&uart_tbl[channel].qbuffer_rx);
  }
  if (uart_tbl[channel].rx_mode == UART_MODE_DMA)
  {
    uart_tbl[channel].qbuffer_rx.ptr_in  = uart_tbl[channel].qbuffer_rx.length - ((DMA_Channel_TypeDef *)uart_tbl[channel].hdma_rx->Instance)->CNDTR;
    uart_tbl[channel].qbuffer_rx.ptr_out = uart_tbl[channel].qbuffer_rx.ptr_in;
  }
}

void uartPutch(uint8_t channel, uint8_t ch)
{
  uartWrite(channel, &ch, 1 );
}

uint8_t uartGetch(uint8_t channel)
{
  uint8_t ret = 0;


  while(1)
  {
    if (uartAvailable(channel) > 0)
    {
      ret = uartRead(channel);
      break;
    }
  }

  return ret;
}

int32_t uartWrite(uint8_t channel, uint8_t *p_data, uint32_t length)
{
  int32_t ret = 0;
  uart_t *p_uart = &uart_tbl[channel];

#ifdef _USE_HW_VCP
  if (p_uart->tx_mode == UART_MODE_VCP)
  {
    vcpWrite(p_data, length);
  }
#endif
  if (p_uart->tx_mode == UART_MODE_POLLING)
  {
    if (HAL_UART_Transmit(p_uart->handle, (uint8_t*)p_data, length, 1000) == HAL_OK)
    {
      ret = length;
    }
  }
  if (p_uart->tx_mode == UART_MODE_DMA)
  {
    p_uart->handle->gState = HAL_UART_STATE_READY;

    HAL_DMA_PollForTransfer(p_uart->hdma_tx, HAL_DMA_FULL_TRANSFER, 100);

    if (HAL_UART_Transmit_DMA(p_uart->handle, p_data, length) == HAL_OK)
    {
      ret = length;
    }
  }
  return ret;
}

uint8_t uartRead(uint8_t channel)
{
  uint8_t ret = 0;
  uart_t *p_uart = &uart_tbl[channel];

#ifdef _USE_HW_VCP
  if (p_uart->rx_mode == UART_MODE_VCP)
  {
    ret = vcpRead();
  }
#endif
  if (p_uart->rx_mode == UART_MODE_INTERRUPT)
  {
    qbufferRead(&p_uart->qbuffer_rx, &ret, 1);
  }
  if (p_uart->rx_mode == UART_MODE_DMA)
  {
    qbufferRead(&p_uart->qbuffer_rx, &ret, 1);
  }
  return ret;
}

bool uartSendBreak(uint8_t channel)
{
  HAL_LIN_SendBreak(uart_tbl[channel].handle);

  return true;
}

bool uartSendBreakEnd(uint8_t channel)
{
  return true;
}


int32_t uartPrintf(uint8_t channel, const char *fmt, ...)
{
  int32_t ret = 0;
  va_list arg;
  va_start (arg, fmt);
  int32_t len;
  char print_buffer[256];


  len = vsnprintf(print_buffer, 255, fmt, arg);
  va_end (arg);

  ret = uartWrite(channel, (uint8_t *)print_buffer, len);

  return ret;
}

void uartRxHandler(uint8_t channel)
{
  uart_t *p_uart = &uart_tbl[channel];


  if(p_uart->rx_mode == UART_MODE_INTERRUPT)
  {
    qbufferWrite(&p_uart->qbuffer_rx, &p_uart->rx_buf[0], 1);

    __HAL_UNLOCK(p_uart->handle);
    uartStartRx(channel);
  }
}

void uartErrHandler(uint8_t channel)
{
  uartStartRx(channel);
  uartFlush(channel);

  uart_tbl[channel].err_cnt++;
}

uint32_t uartGetErrCnt(uint8_t channel)
{
  return uart_tbl[channel].err_cnt;
}

void uartResetErrCnt(uint8_t channel)
{
  uartStartRx(channel);
  uartFlush(channel);

  uart_tbl[channel].err_cnt = 0;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
}




void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
  if (UartHandle->Instance == uart_tbl[_DEF_UART1].handle->Instance)
  {
    uartErrHandler(_DEF_UART1);
  }
  if (UartHandle->Instance == uart_tbl[_DEF_UART3].handle->Instance)
  {
    uartErrHandler(_DEF_UART3);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == uart_tbl[_DEF_UART1].handle->Instance)
  {
    uartRxHandler(_DEF_UART1);
  }
}




void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if(uartHandle->Instance==USART2)
  {
    uart_t *p_uart = &uart_tbl[_DEF_UART1];


    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA15 (JTDI)     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


    if (p_uart->rx_mode == UART_MODE_DMA)
    {
      /* USART2 DMA Init */
      /* USART2_RX Init */
      __HAL_RCC_DMA1_CLK_ENABLE();

      hdma_usart2_rx.Instance = DMA1_Channel6;
      hdma_usart2_rx.Init.Request = DMA_REQUEST_2;
      hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
      hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
      hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
      hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
      hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
      hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;
      hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
      if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
      {
        Error_Handler();
      }

      __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);
    }
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART2)
  {
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA15 (JTDI)     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_15);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
  }
}
