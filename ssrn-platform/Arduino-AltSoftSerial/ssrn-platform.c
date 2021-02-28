#include "ssrn.h"

#define ALT_SOFT_SERIAL_TX_BUFFER_SIZE 68

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

static AltSoftSerial *ssrn_port;

uint8_t ssrn_uart_rx_available(void)
{
  return ssrn_port->available() > 0;
}

uint8_t ssrn_uart_tx_available(void)
{
  return ssrn_port->availableForWrite() > 0;
}

uint8_t ssrn_uart_tx_done(void)
{
  // not exactly what's needed
  // add code to check registers for specific board
  return ssrn_port->availableForWrite() == ALT_SOFT_SERIAL_TX_BUFFER_SIZE;
}

uint8_t ssrn_uart_read(void)
{
  return ssrn_port->read();
}

void ssrn_uart_write(uint8_t c)
{
  ssrn_port->write(c);
}

void ssrn_set_baud_rate(ssrn_baud_rate_t rate)
{
  switch(rate){
  case SSRN_BAUD_1200:
    ssrn_port->begin(1200);
    break;
  case SSRN_BAUD_2400:
    ssrn_port->begin(2400);    
    break;
  case SSRN_BAUD_4800:
    ssrn_port->begin(4800);    
    break;
  case SSRN_BAUD_9600:
    ssrn_port->begin(9600);
    break;
  case SSRN_BAUD_19200:
    ssrn_port->begin(19200);
    break;
  case SSRN_BAUD_38400:
    ssrn_port->begin(38400);
    break;
  case SSRN_BAUD_57600:
    ssrn_port->begin(57600);
    break;
  case SSRN_BAUD_115200:
    ssrn_port->begin(115200);
    break;
  }
}

void ssrn_reset(void)
{
  ((void(*)(void))0)(); // should change to WDT timeout instead
}

void ssrn_init(AltSoftSerial *port)
{
  ssrn_port = port;
  ssrn_set_baud_rate(SSRN_DEFAULT_BAUD_RATE);
}

void ssrn_idle(void)
{
  // no idle mode
}

#ifdef SSRN_USE_TIMERS
uint32_t ssrn_milliseconds(void)
{
  return millis();
}
void ssrn_delay_ms(uint32_t duration);
void ssrn_set_timer_event(uint8_t idx,
                          uint32_t duration_milliseconds,
                          uint8_t periodic_flag);
void ssrn_set_timer_callback(uint8_t idx,
                             uint32_t duration_milliseconds,
                             void (*callback)(void),
                             uint8_t periodic_flag);

void ssrn_cancel_timer(uint8_t idx);

#endif //#ifdef SSRN_USE_TIMERS
