/* 
 * File:   AM2302.h
 * Author: Name
 *
 * Created on December 25, 2020, 1:56 PM
 */

#ifndef AM2302_H
#define	AM2302_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define AM2302_MAX_BITS 42
extern volatile uint8_t AM2302_num_bits;
extern uint16_t AM2302_bit_lengths[AM2302_MAX_BITS];
extern volatile uint8_t AM2302_rx_error;
extern volatile uint8_t AM2302_rx_done;
extern uint8_t volatile AM2302_2s_delay_expired;
  
#define AM2302_BIT_LENGTH_THRESH 402

typedef struct
{
    uint8_t error_code;
    uint16_t humidity;
    int16_t temperature;
} AM2302_data_t;

enum {AM2302_ERROR_NONE = 0,
      AM2302_ERROR_TIMEOUT,
      AM2302_ERROR_PARITY,
      AM2302_ERROR_EARLY_READ};

void AM2302_read_start(void);
uint8_t AM2302_is_read_done(void);
uint8_t AM2302_get_data(AM2302_data_t *data);

#ifdef	__cplusplus
}
#endif

#endif	/* AM2302_H */

