#include "AM2302.h"
#include "ssrn.h"
#include "ssrn-config.h"

void led_on()
{
  LATC0 = 1;
}

void led_off()
{
  LATC0 = 0;
}

void led_blink()
{
  led_on();
  for (uint8_t j=0; j<255; j++){
    ssrn_network();
  }
  led_off();
}

uint8_t ssrn_local_packet(ssrn_packet_t *p)
{
  uint8_t *t = &p->data[20];

  if (ssrn_pkt_type_eq(t, "LED-ON")){
    led_on();
    // reply identical to request
  } else if (ssrn_pkt_type_eq(t, "LED-OFF")){
    led_off();
    // reply identical to request
  } else if (ssrn_pkt_type_eq(t, "LED-BLINK")){
    led_blink();
    // reply identical to request
  } else if (ssrn_pkt_type_eq(t, "READ")){
    AM2302_data_t data;

    AM2302_read_start();
    while (!AM2302_is_read_done()){
      ssrn_network();
    }
    uint8_t result = AM2302_get_data(&data);

    // $SRN|+NNN|+NNN|NNNN|READ|
    // 0000000000111111111122222
    // 0123456789012345678901234

    p->write_ptr = p->data + 25;
        
    ssrn_pkt_ascii_int32(p, result, 0);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_ascii_int32(p, data.error_code, 0);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_ascii_int32(p, data.humidity, 1);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_ascii_int32(p, data.temperature, 1);
    ssrn_pkt_str(p, "|*");
  } else {
    return 0;
  }

  return 1;
}

void main(void)
{
  ssrn_init();

  led_blink();
  
  ssrn_main_loop();
}
