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
  case SSRN_BAUD_1000:
    SP1BRG = 7999;
    break;
  case SSRN_BAUD_1200:
    SP1BRG = 6666;
    break;
  case SSRN_BAUD_2000:
    SP1BRG = 3999;
    break;
  case SSRN_BAUD_2400:
    SP1BRG = 3332;
    break;
  case SSRN_BAUD_3200:
    SP1BRG = 2499;
    break;
  case SSRN_BAUD_4800:
    SP1BRG = 1666;
    break;
  case SSRN_BAUD_5000:
    SP1BRG = 1599;
    break;
  case SSRN_BAUD_8000:
    SP1BRG = 999;
    break;
  case SSRN_BAUD_9600:
    SP1BRG = 832;
    break;
  case SSRN_BAUD_10000:
    SP1BRG = 799;
    break;
  case SSRN_BAUD_19200:
    SP1BRG = 416;
    break;
  case SSRN_BAUD_20000:
    SP1BRG = 399;
    break;
  case SSRN_BAUD_32000:
    SP1BRG = 249;
    break;
  case SSRN_BAUD_38400:
    SP1BRG = 207;
    break;
  case SSRN_BAUD_50000:
    SP1BRG = 159;
    break;
  case SSRN_BAUD_57600:
    SP1BRG = 138;
    break;
  case SSRN_BAUD_80000:
    SP1BRG = 99;
    break;
  case SSRN_BAUD_100000:
    SP1BRG = 79;
    break;
  case SSRN_BAUD_115200:
    SP1BRG = 68;
    break;
  case SSRN_BAUD_125000:
    SP1BRG = 63;
    break;
  case SSRN_BAUD_160000:
    SP1BRG = 49;
    break;
  case SSRN_BAUD_200000:
    SP1BRG = 39;
    break;
  case SSRN_BAUD_230400:
    SP1BRG = 34;
    break;
  case SSRN_BAUD_320000:
    SP1BRG = 24;
    break;
  case SSRN_BAUD_460800:
    SP1BRG = 16;
    break;
  case SSRN_BAUD_500000:
    SP1BRG = 15;
    break;
  case SSRN_BAUD_800000:
    SP1BRG = 9;
    break;
  case SSRN_BAUD_1000000:
    SP1BRG = 7;
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

void ssrn_idle(void)
{
  if(0 == eusart1RxCount){
    IDLEN = 1;
    SLEEP();
  }
}

/*
  measure vdd on PIC18F16Q41
  2.048 V measured w/ 12-bit ADC from Vdd reference

  n = ADC counts
  n = 4095 * 2.048 / Vdd
  Vdd = 4095 * 2.048 / n
  Vdd (mV) = 8386560 / n
*/
uint32_t ssrn_voltage_mv(void)
{
  // save previous FVR setup
  uint8_t FVRCON_old = FVRCON;

  // setup FVR
  FVRCONbits.ADFVR = 0b10; // 2.048 V
  FVRCONbits.FVREN = 1;
  while(!FVRCONbits.FVRRDY){
    ssrn_yield();
  }

  // save previous ADC setup
  uint8_t ADCON0_old = ADCON0;
  uint8_t ADCLK_old = ADCLK;
  uint8_t ADREF_old = ADREF;
  uint8_t ADPCH_old = ADPCH;
  uint16_t ADACQ_old = ADACQ;

  // setup ADC
  ADCON0bits.FM = 1;
  ADCON0bits.CS = 0;
  ADCLK = 7;
  ADREFbits.NREF = 0;
  ADREFbits.PREF = 0;
  ADPCH = 0b111110; // FVR1
  ADACQ = 1024;
  ADCON0bits.ON = 1;
  ADCON0bits.GO = 1;
  while(ADCON0bits.GO){
    ssrn_yield();
  }
  uint16_t n = (((uint16_t)ADRESH) << 8) | ADRESL;
  uint32_t Vdd_mV = 8386560UL / n;

  // restore previous FVR setup
  FVRCON = FVRCON_old;

  // restore previous ADC setup
  uint8_t ADCON0 = ADCON0_old;
  uint8_t ADCLK = ADCLK_old;
  uint8_t ADREF = ADREF_old;
  uint8_t ADPCH = ADPCH_old;
  uint16_t ADACQ = ADACQ_old;
  return Vdd_mV;
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

