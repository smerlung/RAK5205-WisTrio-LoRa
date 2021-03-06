/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Bleeper board UART driver implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include <stdarg.h>
#include "board.h"

#include "uart-board.h"

//uint8_t RxData = 0;

/*!
 * FIFO buffers size
 */

//uint8_t TxBuffer[FIFO_TX_SIZE];
uint8_t UART_RxBuffer[FIFO_RX_SIZE];
uint8_t GPS_RxBuffer[FIFO_RX_SIZE];

typedef struct
{
  UART_HandleTypeDef UartHandle;
  uint8_t RxData;
  uint8_t TxData;
} UartContext_t;

UartContext_t UartContext[UART_COUNT];

void UartMcuInit( Uart_t *obj, uint8_t uartId, PinNames tx, PinNames rx )
{
    obj->UartId = uartId;

    if ( obj->UartId == UART_1 ) 
    {
      __HAL_RCC_USART1_FORCE_RESET( );
      __HAL_RCC_USART1_RELEASE_RESET( );
      __HAL_RCC_USART1_CLK_ENABLE( );
      
      GpioInit( &obj->Tx, tx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART1 );
      GpioInit( &obj->Rx, rx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART1 );
    }
    else if( obj->UartId == UART_3 )
    {
      __HAL_RCC_USART3_FORCE_RESET( );
      __HAL_RCC_USART3_RELEASE_RESET( );
      __HAL_RCC_USART3_CLK_ENABLE( );
      
      GpioInit( &obj->Tx, tx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART3 );
      GpioInit( &obj->Rx, rx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART3 );
    }
}

void UartMcuConfig( Uart_t *obj, UartMode_t mode, uint32_t baudrate, WordLength_t wordLength, StopBits_t stopBits, Parity_t parity, FlowCtrl_t flowCtrl )
{
    UART_HandleTypeDef * handle = &UartContext[obj->UartId].UartHandle;
    IRQn_Type irq;
         
    //FifoInit( &obj->FifoTx, TxBuffer, FIFO_TX_SIZE );
    
    if( obj->UartId == UART_1 )
    {
      handle->Instance = USART1;
      irq = USART1_IRQn;
      FifoInit( &obj->FifoRx, UART_RxBuffer, FIFO_RX_SIZE );
    }
    else if( obj->UartId == UART_3 )
    {
      handle->Instance = USART3;
      irq = USART3_IRQn;
      FifoInit( &obj->FifoRx, GPS_RxBuffer, FIFO_RX_SIZE );
    }
    else
      return;

    handle->Init.BaudRate = baudrate;
    
    if( mode == TX_ONLY )
    {
        if( obj->FifoTx.Data == NULL )
        {
            assert_param( FAIL );
        }
        handle->Init.Mode = UART_MODE_TX;
    }
    else if( mode == RX_ONLY )
    {
        if( obj->FifoRx.Data == NULL )
        {
            assert_param( FAIL );
        }
        handle->Init.Mode = UART_MODE_RX;
    }
    else if( mode == RX_TX )
    {
        if( ( obj->FifoTx.Data == NULL ) || ( obj->FifoRx.Data == NULL ) )
        {
            assert_param( FAIL );
        }
        handle->Init.Mode = UART_MODE_TX_RX;
    }
    else
    {
       assert_param( FAIL );
    }

    if( wordLength == UART_8_BIT )
    {
       handle->Init.WordLength = UART_WORDLENGTH_8B;
    }
    else if( wordLength == UART_9_BIT )
    {
        handle->Init.WordLength = UART_WORDLENGTH_9B;
    }

    switch( stopBits )
    {
    case UART_2_STOP_BIT:
        handle->Init.StopBits = UART_STOPBITS_2;
        break;
    case UART_1_STOP_BIT:
    default:
        handle->Init.StopBits = UART_STOPBITS_1;
        break;
    }

    if( parity == NO_PARITY )
    {
        handle->Init.Parity = UART_PARITY_NONE;
    }
    else if( parity == EVEN_PARITY )
    {
        handle->Init.Parity = UART_PARITY_EVEN;
    }
    else
    {
        handle->Init.Parity = UART_PARITY_ODD;
    }

    if( flowCtrl == NO_FLOW_CTRL )
    {
        handle->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    }
    else if( flowCtrl == RTS_FLOW_CTRL )
    {
        handle->Init.HwFlowCtl = UART_HWCONTROL_RTS;
    }
    else if( flowCtrl == CTS_FLOW_CTRL )
    {
        handle->Init.HwFlowCtl = UART_HWCONTROL_CTS;
    }
    else if( flowCtrl == RTS_CTS_FLOW_CTRL )
    {
        handle->Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
    }

    handle->Init.OverSampling = UART_OVERSAMPLING_16;

    if( HAL_UART_Init( handle ) != HAL_OK )
    {
        while( 1 );
    }

    HAL_NVIC_SetPriority( irq, 8, 0 );
    HAL_NVIC_EnableIRQ( irq );

    /* Enable the UART Data Register not empty Interrupt */
    HAL_UART_Receive_IT( handle, &UartContext[obj->UartId].RxData, 1 );
}

void UartMcuDeInit( Uart_t *obj )
{
  if ( obj->UartId == UART_1 )
  {
    __HAL_RCC_USART1_FORCE_RESET( );
    __HAL_RCC_USART1_RELEASE_RESET( );
    __HAL_RCC_USART1_CLK_DISABLE( );
  } 
  else if( obj->UartId == UART_3 )
  {
    __HAL_RCC_USART3_FORCE_RESET( );
    __HAL_RCC_USART3_RELEASE_RESET( );
    __HAL_RCC_USART3_CLK_DISABLE( );
  }
  
    GpioInit( &obj->Tx, obj->Tx.pin, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &obj->Rx, obj->Rx.pin, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
}

uint8_t UartMcuPutChar( Uart_t *obj, uint8_t data )
{
    
    if( IsFifoFull( &obj->FifoTx ) == false )
    {
        __disable_irq( );
        FifoPush( &obj->FifoTx, data );
        __enable_irq( );
        // Enable the USART Transmit interrupt
        __HAL_UART_ENABLE_IT( &UartContext[UART_1].UartHandle, USART_IT_TXE );
        return 0; // OK
    }
    return 1; // Busy
}

uint8_t UartMcuGetChar( Uart_t *obj, uint8_t *data )
{
    if( IsFifoEmpty( &obj->FifoRx ) == false )
    {
        __disable_irq( );
        *data = FifoPop( &obj->FifoRx );
        __enable_irq( );
        return 0;
    }
    return 1;
}

void UartFlush( Uart_t *obj)
{
    __disable_irq( );
    FifoFlush( &obj->FifoRx );
    __enable_irq( );   
}

void HAL_UART_TxCpltCallback( UART_HandleTypeDef *UartHandle )
{
    uint8_t data;

    if( IsFifoEmpty( &Uart1.FifoTx ) == false )
    {
        data = FifoPop( &Uart1.FifoTx );
        //  Write one byte to the transmit data register
        HAL_UART_Transmit_IT( UartHandle, &data, 1 );
    }
    else
    {
        // Disable the USART Transmit interrupt
        HAL_NVIC_DisableIRQ( USART1_IRQn );
    }
    if( Uart1.IrqNotify != NULL )
    {
        Uart1.IrqNotify( UART_NOTIFY_TX );
    }
}

void HAL_UART_RxCpltCallback( UART_HandleTypeDef *handle )
{
    Uart_t *uart = &Uart1;
    UartId_t uartId = UART_1;
    
    if( handle == &UartContext[UART_1].UartHandle )
    {
      uart = &Uart1;
      uartId = UART_1;
    }
    else if( handle == &UartContext[UART_3].UartHandle )
    {
      uart = &GpsUart;
      uartId = UART_3;
    }
    else // Unknown UART peripheral skip processing
      return;
    
    if( IsFifoFull( &uart->FifoRx ) == false ) {
      // Read one byte from the receive data register
      FifoPush( &uart->FifoRx, UartContext[uartId].RxData );
    }
    if( uart->IrqNotify != NULL )
      uart->IrqNotify( UART_NOTIFY_RX );
    
    //__HAL_UART_FLUSH_DRREGISTER( handle );
    HAL_UART_Receive_IT( &UartContext[uartId].UartHandle, &UartContext[uartId].RxData, 1 );
}

void HAL_UART_ErrorCallback( UART_HandleTypeDef *handle )
{
  UartId_t uartId = UART_1;
  
  if( handle == &UartContext[UART_1].UartHandle )
    uartId = UART_1;
  else if( handle == &UartContext[UART_3].UartHandle )
    uartId = UART_3;
  else // Unknown UART peripheral skip processing
    return;
  
  HAL_UART_Receive_IT( &UartContext[uartId].UartHandle, &UartContext[uartId].RxData, 1 );
}

void USART1_IRQHandler( void )
{
    HAL_UART_IRQHandler( &UartContext[UART_1].UartHandle );
}

void USART3_IRQHandler( void )
{
  // [BEGIN] Workaround to solve an issue with the HAL drivers not managin the uart state correctly.
  uint32_t tmpFlag = 0, tmpItSource = 0;
  
  tmpFlag = __HAL_UART_GET_FLAG( &UartContext[UART_3].UartHandle, UART_FLAG_TC );
  tmpItSource = __HAL_UART_GET_IT_SOURCE( &UartContext[UART_3].UartHandle, UART_IT_TC );
  // UART in mode Transmitter end
  if( ( tmpFlag != RESET ) && ( tmpItSource != RESET ) )
  {
    if( ( UartContext[UART_3].UartHandle.State == HAL_UART_STATE_BUSY_RX ) || UartContext[UART_3].UartHandle.State == HAL_UART_STATE_BUSY_TX_RX )
    {
      UartContext[UART_3].UartHandle.State = HAL_UART_STATE_BUSY_TX_RX;
    }
  }
  // [END] Workaround to solve an issue with the HAL drivers not managin the uart state correctly.
  
  HAL_UART_IRQHandler( &UartContext[UART_3].UartHandle );
}

#define MAX_MSG_LEN 127

static char ll_msg_buf_[MAX_MSG_LEN];

void e_printf(const char *format, ...)
{
    int i;
    va_list args;
    size_t len;
    
    /* Format the string */
    va_start(args, format);
    len = vsnprintf(ll_msg_buf_, MAX_MSG_LEN, &format[0], args);
    va_end(args);
    HAL_UART_Transmit(&UartContext[UART_1].UartHandle, (uint8_t *)&ll_msg_buf_, len, 0xFFFF);
}

int e_printf_raw(const char *buffer, int size)
{
    HAL_UART_Transmit(&UartContext[UART_1].UartHandle, (uint8_t *)buffer, size, 0xFFFF);
}

int e_getchar(void)
{
    int c = -1;
    uint8_t data;

    if (UartMcuGetChar(&Uart1, &data) == 0) {
        c = data;			
    } 
    return c;
}
