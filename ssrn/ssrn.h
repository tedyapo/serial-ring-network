#ifndef _SSRN_H_INCLUDED
#define _SSRN_H_INCLUDED

#include <stdint.h>

#define SSRN_MAX_PACKET_LEN 100

enum {SSRN_ADDRESS_LOCALHOST  = +000,
      SSRN_ADDRESS_BROADCAST  = +999,
      SSRN_ADDRESS_CONTROLLER = -999};

typedef enum {SSRN_ADDR_CODE_LOCALHOST = 1,
              SSRN_ADDR_CODE_BROADCAST = 2,
              SSRN_ADDR_CODE_CONTROLLER = 3,
              SSRN_ADDR_CODE_FORWARD = 4} ssrn_addr_code_t;

typedef struct
{
  ssrn_addr_code_t dst_addr_code;
  uint8_t *write_ptr;
  uint8_t data[SSRN_MAX_PACKET_LEN];
} ssrn_packet_t;

typedef enum {SSRN_EVENT_PACKET,
              SSRN_EVENT_TIMER} ssrn_event_type_t;

typedef enum {SSRN_PACKET_CLASS_UNKNOWN,
              SSRN_PACKET_CLASS_NETWORK,
              SSRN_PACKET_CLASS_USER} ssrn_packet_class_t;

typedef struct
{
  ssrn_event_type_t type;
  uint8_t timer_idx;
  ssrn_packet_t *packet;
  ssrn_packet_class_t packet_class;
  uint8_t packet_has_reply;
} ssrn_event_t;

typedef struct
{
  uint8_t active;
  uint32_t begin_milliseconds;
  uint32_t duration_milliseconds;
} ssrn_timer_t;

typedef enum {SSRN_BAUD_1200,
              SSRN_BAUD_2400,
              SSRN_BAUD_4800,
              SSRN_BAUD_9600,
              SSRN_BAUD_19200,
              SSRN_BAUD_38400,
              SSRN_BAUD_57600,
              SSRN_BAUD_115200,
              SSRN_BAUD_230400,
              SSRN_BAUD_460800} ssrn_baud_rate_t;

uint8_t ssrn_pkt_type_eq(uint8_t *t, const char *c);
void ssrn_pkt_char(ssrn_packet_t *p, char c);
void ssrn_pkt_str(ssrn_packet_t *p, const char *c);
void ssrn_pkt_ascii_int32(ssrn_packet_t *p,
                          int32_t value,
                          uint8_t decimal_place);
void ssrn_pkt_ascii_uint32(ssrn_packet_t *p,
                           uint32_t value,
                           uint8_t decimal_place);
void ssrn_network(void);
void ssrn_processing(void);
void ssrn_main_loop(void);

#ifdef SSRN_USE_TIMERS
void ssrn_set_timer(uint8_t idx, uint32_t duration_milliseconds);
void ssrn_cancel_timer(uint8_t idx);
#endif //#ifdef SSRN_USE_TIMERS

ssrn_event_t *ssrn_next_event(void);

void ssrn_no_reply(ssrn_event_t *event);
void ssrn_unknown_packet(ssrn_event_t *event);

#endif // #ifndef _SSRN_H_INCLUDED
