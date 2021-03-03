#include "ssrn.h"

static const char _ssrn_node_type[] = SSRN_NODE_TYPE;
static const uint16_t _ssrn_node_id = SSRN_NODE_ID;

const char *ssrn_node_type(void)
{
  return _ssrn_node_type;
}

uint16_t ssrn_node_id(void)
{
  return _ssrn_node_id;
}

uint8_t ssrn_uart_rx_available(void)
{
  return uart1RxCount;
}

uint8_t ssrn_uart_tx_available(void)
{
  return uart1TxBufferRemaining;
}

uint8_t ssrn_uart_tx_done(void)
{
  return UART1_is_tx_done();
}

uint8_t ssrn_uart_read(void)
{
  return UART1_Read();
}

void ssrn_uart_write(uint8_t c)
{
  UART1_Write(c);
}

void ssrn_set_baud_rate(ssrn_baud_rate_t rate)
{
  switch(rate){
  case SSRN_BAUD_1200:
    U1BRG = 6666;
    break;
  case SSRN_BAUD_2400:
    U1BRG = 3332;
    break;
  case SSRN_BAUD_4800:
    U1BRG = 1666;
    break;
  case SSRN_BAUD_9600:
    U1BRG = 832;
    break;
  case SSRN_BAUD_19200:
    U1BRG = 416;
    break;
  case SSRN_BAUD_38400:
    U1BRG = 207;
    break;
  case SSRN_BAUD_57600:
    U1BRG = 138;
    break;
  case SSRN_BAUD_115200:
    U1BRG = 68;
    break;
  case SSRN_BAUD_230400:
    U1BRG = 34;
    break;
  case SSRN_BAUD_460800:
    U1BRG = 16;
    break;
  }
}

void ssrn_init(void)
{
  SYSTEM_Initialize();

  ssrn_set_baud_rate(SSRN_DEFAULT_BAUD_RATE);

  INTERRUPT_GlobalInterruptEnable();
}

void ssrn_reset(void)
{
  RESET();
}

void ssrn_idle(void)
{
  if(0 == uart1RxCount){
    IDLEN = 1;
    SLEEP();
  }
}

#ifdef SSRN_USE_TIMERS
volatile uint32_t ssrn_millisecond_counter;

uint32_t ssrn_milliseconds(void)
{
  uint32_t milliseconds;
  di();
  milliseconds = ssrn_millisecond_counter;
  ei();
  return milliseconds;
}
#endif //#ifdef SSRN_USE_TIMERS

