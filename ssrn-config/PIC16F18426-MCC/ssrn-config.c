#include "ssrn-config.h"

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
  return eusart1RxCount;
}

uint8_t ssrn_uart_tx_available(void)
{
  return eusart1TxBufferRemaining;
}

uint8_t ssrn_uart_tx_done(void)
{
  return EUSART1_is_tx_done();
}

uint8_t ssrn_uart_read(void)
{
  return EUSART1_Read();
}

void ssrn_uart_write(uint8_t c)
{
  EUSART1_Write(c);
}

void ssrn_set_baud_rate(ssrn_baud_rate_t rate)
{
  switch(rate){
  case SSRN_BAUD_1200:
    SP1BRG = 6666;
    break;
  case SSRN_BAUD_2400:
    SP1BRG = 3332;
    break;
  case SSRN_BAUD_4800:
    SP1BRG = 1666;
    break;
  case SSRN_BAUD_9600:
    SP1BRG = 832;
    break;
  case SSRN_BAUD_19200:
    SP1BRG = 416;
    break;
  case SSRN_BAUD_38400:
    SP1BRG = 207;
    break;
  case SSRN_BAUD_57600:
    SP1BRG = 138;
    break;
  case SSRN_BAUD_115200:
    SP1BRG = 68;
    break;
  case SSRN_BAUD_230400:
    SP1BRG = 34;
    break;
  case SSRN_BAUD_460800:
    SP1BRG = 16;
    break;
  }
}

void ssrn_init(void)
{
  SYSTEM_Initialize();
  ssrn_set_baud_rate(SSRN_DEFAULT_BAUD_RATE);
  INTERRUPT_GlobalInterruptEnable();
  INTERRUPT_PeripheralInterruptEnable();
}

void ssrn_reset(void)
{
  RESET();
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

