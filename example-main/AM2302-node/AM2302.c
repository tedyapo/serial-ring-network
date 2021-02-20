#include "mcc_generated_files/mcc.h"
#include "AM2302.h"

uint8_t volatile AM2302_num_bits;
uint16_t AM2302_bit_lengths[AM2302_MAX_BITS];
uint8_t volatile AM2302_rx_error;
uint8_t volatile AM2302_rx_done;
uint8_t volatile AM2302_2s_delay_expired = 0;
static AM2302_data_t cached_data;

void AM2302_read_start(void)
{
  AM2302_rx_error = AM2302_ERROR_NONE;
  AM2302_rx_done = 0;
  AM2302_num_bits = 0;
  
  if (!AM2302_2s_delay_expired){
    AM2302_rx_error = AM2302_ERROR_EARLY_READ;
    AM2302_2s_delay_expired = 0;
    return;
  }

  // reset the 2s bewtween-reads timeout
  TMR2_Initialize();
  AM2302_2s_delay_expired = 0;  
  
  TMR1_Reload();
  TMR1_StartTimer();
    
  // >= 1 ms low pulse on data line
  RA2 = 0;
  TRISA2 = 0;
  __delay_ms(1);
  // Clear any old CCP1 interrupt flag
  PIR1bits.CCP1IF = 0;
  // Enable the CCP1 interrupt  
  PIE1bits.CCP1IE = 1;  
  TRISA2 = 1;
}

uint8_t AM2302_is_read_done(void)
{
  return AM2302_rx_done || AM2302_rx_error;
}

uint8_t AM2302_get_data(AM2302_data_t *data)
{
  data->humidity = 0xffff;
  data->temperature = -1;

  if (AM2302_ERROR_EARLY_READ == AM2302_rx_error){
    *data = cached_data;
  }

  if (AM2302_rx_error){
    data->error_code = AM2302_rx_error;
    return AM2302_rx_error;
  }
  
  for (int8_t i=AM2302_MAX_BITS-1; i>0; i--){
    AM2302_bit_lengths[i] -= AM2302_bit_lengths[i-1];
  }

  uint16_t humidity = 0;
  for (uint8_t i=2; i<18; i++){
    humidity <<= 1;    
    if (AM2302_bit_lengths[i] >= AM2302_BIT_LENGTH_THRESH){
      humidity |= 1;
    }
  }

  uint16_t temperature = 0;
  for (uint8_t i=18; i<34; i++){
    temperature <<= 1;    
    if (AM2302_bit_lengths[i] >= AM2302_BIT_LENGTH_THRESH){
      temperature |= 1;
    }
  }
  
  uint8_t parity = 0;
  for (uint8_t i=34; i<42; i++){
    parity <<= 1;    
    if (AM2302_bit_lengths[i] >= AM2302_BIT_LENGTH_THRESH){
      parity |= 1;
    }
  }

  uint8_t csum = 0;
  csum += (humidity & 0xff00) >> 8;
  csum += humidity & 0xff;
  csum += (temperature & 0xff00) >> 8;
  csum += temperature & 0xff;

  if (csum != parity){
    return AM2302_ERROR_PARITY;
  }
  
  data->humidity = humidity;
  if (temperature & 0x8000){
    data->temperature = -(temperature & 0x7fff);
  } else {
    data->temperature = (int16_t)temperature;    
  }

  data->error_code = AM2302_rx_error;
  cached_data = *data;

  return AM2302_rx_error;
}
