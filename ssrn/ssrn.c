#include "ssrn.h"
#include "ssrn-config.h"

static uint8_t baud_rate_change = 0;
static uint8_t new_baud_rate = SSRN_DEFAULT_BAUD_RATE;

static ssrn_packet_t rx_queue[SSRN_RX_QUEUE_LEN];
static ssrn_packet_t tx_queue[SSRN_TX_QUEUE_LEN];
static ssrn_packet_t in_queue[SSRN_IN_QUEUE_LEN];

static int8_t rx_queue_head;
static int8_t rx_queue_tail;
static uint8_t rx_queue_items;
static int8_t tx_queue_head;
static int8_t tx_queue_tail;
static uint8_t tx_queue_items;
static int8_t in_queue_head;
static int8_t in_queue_tail;
static uint8_t in_queue_items;

static uint32_t netstack_rx_error_count;
static uint32_t netstack_invalid_packet_count;
static uint32_t netstack_received_count;
static uint32_t netstack_forwarded_count;
static uint32_t netstack_forward_drop_count;
static uint32_t netstack_inbound_count;
static uint32_t netstack_inbound_drop_count;
static uint32_t netstack_transmitted_count;
static uint32_t netstack_tx_error_count;

static ssrn_timer_t ssrn_timers[SSRN_NUM_TIMERS];

static void ssrn_network(void);

static int8_t rx_queue_put_acquire(void)
{
  if (rx_queue_items >= SSRN_RX_QUEUE_LEN){
    return -1;
  }
  return rx_queue_tail;
}

static void rx_queue_put_release(void)
{
  rx_queue_tail++;
  if (rx_queue_tail >= SSRN_RX_QUEUE_LEN){
    rx_queue_tail = 0;
  }
  rx_queue_items++;
}

static int8_t rx_queue_get_acquire(void)
{
  if (!rx_queue_items){
    return -1;
  }
  return rx_queue_head;
}

static void rx_queue_get_release(void)
{
  rx_queue_head++;
  if (rx_queue_head >= SSRN_RX_QUEUE_LEN){
    rx_queue_head = 0;
  }
  rx_queue_items--;
}

static uint8_t rx_queue_full()
{
  return SSRN_RX_QUEUE_LEN == rx_queue_items;
}

static int8_t tx_queue_put(void)
{
  if (tx_queue_items >= SSRN_TX_QUEUE_LEN){
    return -1;
  }
  int8_t idx = tx_queue_tail++;
  if (tx_queue_tail >= SSRN_TX_QUEUE_LEN){
    tx_queue_tail = 0;
  }
  tx_queue_items++;
  return idx;
}

static int8_t tx_queue_get_acquire(void)
{
  if (!tx_queue_items){
    return -1;
  }
  return tx_queue_head;
}

static void tx_queue_get_release(void)
{
  tx_queue_head++;
  if (tx_queue_head >= SSRN_TX_QUEUE_LEN){
    tx_queue_head = 0;
  }
  tx_queue_items--;
}

static int8_t in_queue_put(void)
{
  if (in_queue_items >= SSRN_IN_QUEUE_LEN){
    return -1;
  }
  int8_t idx = in_queue_tail++;
  if (in_queue_tail >= SSRN_IN_QUEUE_LEN){
    in_queue_tail = 0;
  }
  in_queue_items++;
  return idx;
}

static int8_t in_queue_get_acquire(void)
{
  if (!in_queue_items){
    return -1;
  }
  return in_queue_head;
}

static void in_queue_get_release(void)
{
  in_queue_head++;
  if (in_queue_head >= SSRN_IN_QUEUE_LEN){
    in_queue_head = 0;
  }
  in_queue_items--;
}

static void uint32div10(uint32_t n, uint32_t *q, uint32_t *r)
{
  *q = (n >> 1) + (n >> 2);
  *q += (*q >> 4);
  *q += (*q >> 8);
  *q += (*q >> 16);
  *q >>= 3;
  *r = n - (((*q << 2) + *q) << 1);
  if (*r > 9){
    (*q)++;
    *r -= 10;
  }
}

static void uint16div10(uint16_t n, uint16_t *q, uint16_t *r)
{
  *q = (n >> 1) + (n >> 2);
  *q += (*q >> 4);
  *q += (*q >> 8);
  *q >>= 3;
  *r = n - (((*q << 2) + *q) << 1);
  if (*r > 9){
    (*q)++;
    *r -= 10;
  }
}

static uint8_t check_checksum(uint8_t *c, uint8_t update)
{
  uint16_t sum = 0;
  uint8_t l = 0;

  while(*c != '\n' &&
        *c != '*' &&
        l++ < (SSRN_MAX_PACKET_LEN-3) ){
    if ('|' != *c){
      sum += (uint8_t)*c;
    }
    c++;
  }
  sum &= 0xff;

  uint8_t cs;
  if ('*' == *c){
    uint8_t d = *(c+1);
    cs = (uint8_t)(((d <= '9') ? (d - '0') :
                    (d <= 'F') ? (d - '7') : (d - 'W')) << 4);
    d = *(c+2);
    cs += (d <= '9') ? (d - '0') : (d <= 'F') ? (d - '7') : (d - 'W');
  } else {
    return 0;
  }

  if (update){
    // write calculated checksum into packet
    uint8_t d = (sum & 0xf0) >> 4;
    *(c+1) = (d > 9) ? (d + '7') : (d + '0');
    d = sum & 0x0f;
    *(c+2) = (d > 9) ? (d + '7') : (d + '0');
    *(c+3) = '\n';
    return 1;
  }

  return cs == sum;
}

static uint8_t validate_packet(ssrn_packet_t *p)
{
  if (p->data[0] != '$' ||
      p->data[1] != 'S' ||
      p->data[2] != 'R' ||
      p->data[3] != 'N' ||      
      !check_checksum(p->data, 0)){
    return 0;
  }

  uint8_t *d = p->data;

  int16_t dst_addr = d[6] - '0';
  dst_addr = ((int16_t)(dst_addr<<3) +
              (int16_t)(dst_addr<<1) +
              (int16_t)(d[7] - '0'));
  dst_addr = ((int16_t)(dst_addr<<3) +
              (int16_t)(dst_addr<<1) +
              (int16_t)(d[8] - '0'));
  if ('-' == d[5]){
    dst_addr = -dst_addr;
  }
  if (dst_addr > -998 && dst_addr < 999){
    dst_addr--;
    uint16_t q, r;
    if (dst_addr < 0){
      d[5] = '-';
      uint16div10((uint16_t)-dst_addr, &q, &r);
    } else {
      uint16div10((uint16_t)dst_addr, &q, &r);
    }
    d[8] = (uint8_t)(r + '0');
    uint16div10(q, &q, &r);
    d[7] = (uint8_t)(r + '0');
    d[6] = (uint8_t)(q + '0');
  }
  switch(dst_addr){
    case SSRN_ADDRESS_LOCALHOST:
      p->dst_addr_code = SSRN_ADDR_CODE_LOCALHOST;
      break;
    case SSRN_ADDRESS_BROADCAST:
      p->dst_addr_code = SSRN_ADDR_CODE_BROADCAST;
      break;
    case SSRN_ADDRESS_CONTROLLER:
      p->dst_addr_code = SSRN_ADDR_CODE_CONTROLLER;
      break;
    default:
      p->dst_addr_code = SSRN_ADDR_CODE_FORWARD;
      break;
  }

  int16_t src_addr = d[11] - '0';
  src_addr = ((int16_t)(src_addr<<3) +
              (int16_t)(src_addr<<1) + 
              (int16_t)(d[12] - '0'));
  src_addr = ((int16_t)(src_addr<<3) +
              (int16_t)(src_addr<<1) +
              (int16_t)(d[13] - '0'));
  if ('-' == d[10]){
    src_addr = -src_addr;
  }
  if (src_addr > -999 && src_addr < 998){
    src_addr++;
    uint16_t q, r;
    if (src_addr >= 0){
      d[10] = '+';
      uint16div10((uint16_t)dst_addr, &q, &r);
    } else {
      uint16div10((uint16_t)-dst_addr, &q, &r);
    }
    d[13] = (uint8_t)(r + '0');
    uint16div10(q, &q, &r);
    d[12] = (uint8_t)(r + '0');
    d[11] = (uint8_t)(q + '0');
  }

  return 1;
}

void ssrn_pkt_char(ssrn_packet_t *p, char c)
{
  *p->write_ptr++ = c;
}

void ssrn_pkt_str(ssrn_packet_t *p, const char *c)
{
  uint8_t *d = p->write_ptr;
  while (*c != '\0'){
    *d++ = *c++;
  }
  p->write_ptr = d;
}

void ssrn_pkt_ascii_int32(ssrn_packet_t *p,
                          int32_t value,
                          uint8_t decimal_place)
{
  if (value < 0){
    ssrn_pkt_char(p, '-');
    value = -value;
  }
  ssrn_pkt_ascii_uint32(p, (uint32_t)value, decimal_place);
}

void ssrn_pkt_ascii_uint32(ssrn_packet_t *p,
                           uint32_t value,
                           uint8_t decimal_place)
{
  uint32_t quot, rem;
  uint8_t buf[10];
  uint8_t idx = 0;

  do {
    uint32div10(value, &quot, &rem);
    buf[idx++] = (uint8_t)('0' + rem);
    value = quot;
  } while(value);

  do {
    if (idx == decimal_place){
      ssrn_pkt_char(p, '.');
    }
    ssrn_pkt_char(p, buf[--idx]);
  } while (idx);
}

uint8_t ssrn_pkt_type_eq(uint8_t *t, const char *c)
{
  while (*c != '\0' && *t != '\n'){
    if (*t++ != *c++){
      return 0;
    }
  }
  if ('\n' == *t){
    return 0;
  } else {
    return 1;
  }
}

static void process_network_packet(ssrn_event_t *event)
{
  ssrn_packet_t *p = event->packet;

  // $SRN|-999|+000|NNNN|
  // 00000000001111111111
  // 01234567890123456789

  // reply to controller from us
  p->write_ptr = p->data + 5;
  ssrn_pkt_str(p, "-999|+000");

  uint8_t *t = &p->data[20];
  if (ssrn_pkt_type_eq(t, "RESET|")){
    // note: no reply packet from this
    ssrn_reset();
  } else if (ssrn_pkt_type_eq(t, "PING|")){
    event->packet_class = SSRN_PACKET_CLASS_NETWORK;
    // reply format
    //                          type   id
    // $SRN|-999|+000|NNNN|PING|NNNNN|NNNNN|
    // 0000000000111111111122222222223333333
    // 0123456789012345678901234567890123456

    p->write_ptr = p->data + 25;
    ssrn_pkt_str(p, ssrn_node_type());
    ssrn_pkt_char(p, '|');
    ssrn_pkt_ascii_uint32(p, ssrn_node_id(), 0);
    ssrn_pkt_str(p, "|*");
  } else if (ssrn_pkt_type_eq(t, "BAUD|")){
    event->packet_class = SSRN_PACKET_CLASS_NETWORK;
    // $SRN|-999|+000|NNNN|BAUD|NNNNNN|
    // 00000000001111111111222222222233
    // 01234567890123456789012345678901
    baud_rate_change = 1;
    uint8_t *s = &p->data[25];
    if (ssrn_pkt_type_eq(s, "1200")){
      new_baud_rate = SSRN_BAUD_1200;
    } else if (ssrn_pkt_type_eq(s, "2400")){
      new_baud_rate = SSRN_BAUD_2400;
    } else if (ssrn_pkt_type_eq(s, "4800")){
      new_baud_rate = SSRN_BAUD_4800;
    } else if (ssrn_pkt_type_eq(s, "9600")){
      new_baud_rate = SSRN_BAUD_9600;
    } else if (ssrn_pkt_type_eq(s, "19200")){
      new_baud_rate = SSRN_BAUD_19200;
    } else if (ssrn_pkt_type_eq(s, "38400")){
      new_baud_rate = SSRN_BAUD_38400;
    } else if (ssrn_pkt_type_eq(s, "57600")){
      new_baud_rate = SSRN_BAUD_57600;
    } else if (ssrn_pkt_type_eq(s, "115200")){
      new_baud_rate = SSRN_BAUD_115200;
    } else if (ssrn_pkt_type_eq(s, "230400")){
      new_baud_rate = SSRN_BAUD_230400;
    } else if (ssrn_pkt_type_eq(s, "460800")){
      new_baud_rate = SSRN_BAUD_460800;
    } else {
      baud_rate_change = 0;
      while (*p->write_ptr++ != '|');
      ssrn_pkt_str(p, "UNKNOWN-BAUD|*");
    }
    // normal reply identical to request
  } else if (ssrn_pkt_type_eq(t, "PACKET-STATS|")){
    event->packet_class = SSRN_PACKET_CLASS_NETWORK;
    // $SRN|+NNN|+NNN|NNNN|PACKET-STATS|
    // 000000000011111111112222222222333
    // 012345678901234567890123456789012
    p->write_ptr = p->data + 33;
    ssrn_pkt_ascii_uint32(p, netstack_received_count, 0);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_ascii_uint32(p, netstack_forwarded_count, 0);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_ascii_uint32(p, netstack_inbound_count, 0);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_ascii_uint32(p, netstack_transmitted_count, 0);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_char(p, '*');
  } else if (ssrn_pkt_type_eq(t, "ERROR-STATS|")){
    event->packet_class = SSRN_PACKET_CLASS_NETWORK;
    // $SRN|+NNN|+NNN|NNNN|ERROR-STATS|
    // 00000000001111111111222222222233
    // 01234567890123456789012345678901
    p->write_ptr = p->data + 32;
    ssrn_pkt_ascii_uint32(p, netstack_rx_error_count, 0);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_ascii_uint32(p, netstack_invalid_packet_count, 0);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_ascii_uint32(p, netstack_forward_drop_count, 0);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_ascii_uint32(p, netstack_inbound_drop_count, 0);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_ascii_uint32(p, netstack_tx_error_count, 0);
    ssrn_pkt_char(p, '|');
    ssrn_pkt_char(p, '*');
  } else if (ssrn_pkt_type_eq(t, "RESET-STATS|")){
    event->packet_class = SSRN_PACKET_CLASS_NETWORK;
    // zero stat counters
    netstack_rx_error_count = 0;
    netstack_invalid_packet_count = 0;
    netstack_received_count = 0;
    netstack_forwarded_count = 0;
    netstack_forward_drop_count = 0;
    netstack_inbound_count = 0;
    netstack_inbound_drop_count = 0;
    netstack_transmitted_count = 0;
    netstack_tx_error_count = 0;
    // reply identical to request
  }
}

#ifdef SSRN_USE_TIMERS
void ssrn_delay_ms(uint32_t duration)
{
  uint32_t begin = ssrn_milliseconds();
  while ((ssrn_milliseconds() - begin) <= duration){
    ssrn_network();
  }
}

void ssrn_set_timer_event(uint8_t idx,
                          uint32_t duration_milliseconds,
                          uint8_t periodic_flag)
{
  if (idx < SSRN_NUM_TIMERS){
    ssrn_timers[idx].begin_milliseconds = ssrn_milliseconds();
    ssrn_timers[idx].duration_milliseconds = duration_milliseconds;
    if (periodic_flag){
      ssrn_timers[idx].type = SSRN_TIMER_TYPE_PERIODIC_EVENT;
    } else {
      ssrn_timers[idx].type = SSRN_TIMER_TYPE_EVENT;
    }
  }
}

void ssrn_set_timer_callback(uint8_t idx,
                             uint32_t duration_milliseconds,
                             void (*callback)(void),
                             uint8_t periodic_flag)
{
  if (idx < SSRN_NUM_TIMERS){
    ssrn_timers[idx].begin_milliseconds = ssrn_milliseconds();
    ssrn_timers[idx].duration_milliseconds = duration_milliseconds;
    ssrn_timers[idx].callback = callback;
    if (periodic_flag){
      ssrn_timers[idx].type = SSRN_TIMER_TYPE_PERIODIC_CALLBACK;
    } else {
      ssrn_timers[idx].type = SSRN_TIMER_TYPE_CALLBACK;
    }
  }
}

void ssrn_cancel_timer(uint8_t idx)
{
  if (idx < SSRN_NUM_TIMERS){
    ssrn_timers[idx].type = SSRN_TIMER_TYPE_INACTIVE;
  }
}
#endif //#ifdef SSRN_USE_TIMERS


void ssrn_no_reply(ssrn_event_t *event)
{
  event->packet_has_reply = 0;
}

void ssrn_unknown_packet(ssrn_event_t *event)
{
  event->packet_class = SSRN_PACKET_CLASS_UNKNOWN;
}

ssrn_event_t *ssrn_next_event(void)
{
  enum {PROCESSING_STATE_IDLE,
        PROCESSING_STATE_ACTIVE,
        PROCESSING_STATE_USER,
        PROCESSING_STATE_OUTPUT_WAIT};
  static uint8_t processing_state = PROCESSING_STATE_IDLE;
  static int8_t in_queue_idx;
  static ssrn_event_t timer_event;
  static ssrn_event_t packet_event;

  while(1){
    ssrn_network();

#ifdef SSRN_USE_TIMERS
    // process timer events first
    for (uint8_t i=0; i<SSRN_NUM_TIMERS; i++){
      if (SSRN_TIMER_TYPE_INACTIVE != ssrn_timers[i].type &&
          (ssrn_milliseconds() - ssrn_timers[i].begin_milliseconds) >
          ssrn_timers[i].duration_milliseconds){

        if (SSRN_TIMER_TYPE_EVENT == ssrn_timers[i].type ||
            SSRN_TIMER_TYPE_CALLBACK == ssrn_timers[i].type){
          // one-shot timers
          ssrn_timers[i].type = SSRN_TIMER_TYPE_INACTIVE;
        } else {
          // periodic timers
          ssrn_timers[i].begin_milliseconds += 
            ssrn_timers[i].duration_milliseconds;
        }

        if (SSRN_TIMER_TYPE_EVENT == ssrn_timers[i].type ||
            SSRN_TIMER_TYPE_PERIODIC_EVENT == ssrn_timers[i].type){
          // event timers
          timer_event.type = SSRN_EVENT_TIMER;
          timer_event.timer_idx = i;
          return &timer_event;
        } else {
          // callback timers
          ssrn_timers[i].callback();
        }
      }
    }
#endif //#ifdef SSRN_USE_TIMERS

    if (PROCESSING_STATE_IDLE == processing_state){
      in_queue_idx = in_queue_get_acquire();
      if (in_queue_idx >= 0){
        packet_event.type = SSRN_EVENT_PACKET;
        packet_event.packet = &in_queue[in_queue_idx];
        packet_event.packet_class = SSRN_PACKET_CLASS_UNKNOWN;
        if (SSRN_ADDR_CODE_BROADCAST == packet_event.packet->dst_addr_code){
          packet_event.packet_has_reply = 0;
        } else {
          packet_event.packet_has_reply = 1;
        }
        process_network_packet(&packet_event);
        if (SSRN_PACKET_CLASS_NETWORK == packet_event.packet_class){
          if (packet_event.packet_has_reply){
            processing_state = PROCESSING_STATE_OUTPUT_WAIT;
          } else {
            in_queue_get_release();
            processing_state = PROCESSING_STATE_IDLE;
          }
        } else {
          processing_state = PROCESSING_STATE_USER;
          packet_event.packet_class = SSRN_PACKET_CLASS_USER;
          return &packet_event;
        }
      }
    }
    
    if (PROCESSING_STATE_USER == processing_state){
      if (SSRN_PACKET_CLASS_UNKNOWN == packet_event.packet_class){
        // repond to unknown packet type/request
        packet_event.packet->write_ptr = packet_event.packet->data + 20;
        ssrn_pkt_str(packet_event.packet, "UNKNOWN|*");
        processing_state = PROCESSING_STATE_OUTPUT_WAIT;
      } else if (packet_event.packet_has_reply){
        processing_state = PROCESSING_STATE_OUTPUT_WAIT;
      } else {
        in_queue_get_release();
        processing_state = PROCESSING_STATE_IDLE;
      }
    }
    
    if (PROCESSING_STATE_OUTPUT_WAIT == processing_state){
      int tx_queue_idx = tx_queue_put();
      if (tx_queue_idx >= 0){
        // calculate checksum for packet
        check_checksum(in_queue[in_queue_idx].data, 1);
        // copy packet to output queue
        uint8_t idx = 0;
        uint8_t *s = in_queue[in_queue_idx].data; 
        uint8_t *d = tx_queue[tx_queue_idx].data;
        do {
          *d++ = *s;
        } while (*s++ != '\n' && ++idx < SSRN_MAX_PACKET_LEN);
        in_queue_get_release();
        processing_state = PROCESSING_STATE_IDLE;
      }
    }
  }
}

static void ssrn_network(void)
{
  enum {RX_STATE_IDLE,
        RX_STATE_SCANNING,
        RX_STATE_ACTIVE};
  enum {TX_STATE_IDLE,
        TX_STATE_ACTIVE};
  enum {NETSTACK_STATE_IDLE,
        NETSTACK_STATE_PROCESSING,
        NETSTACK_STATE_FORWARDING};
  static uint8_t rx_state = RX_STATE_IDLE;
  static uint8_t tx_state = TX_STATE_IDLE;
  static uint8_t netstack_state = NETSTACK_STATE_IDLE;
  static int8_t input_rx_queue_idx;
  static uint8_t input_rx_buffer_idx;
  static int8_t output_tx_queue_idx;
  static uint8_t output_tx_buffer_idx;
  static int8_t netstack_rx_queue_idx;
  static int8_t netstack_tx_queue_idx;

  // service EUSART input character queue
  while (ssrn_uart_rx_available()){
    if (RX_STATE_IDLE == rx_state){
      input_rx_queue_idx = rx_queue_put_acquire();
      if (input_rx_queue_idx < 0){
        break;
      }
      input_rx_buffer_idx = 0;
      rx_state = RX_STATE_SCANNING;
    }
    if (RX_STATE_SCANNING == rx_state){
      uint8_t c = ssrn_uart_read();
      if ('$' == c){
        rx_queue[input_rx_queue_idx].data[input_rx_buffer_idx++] = c;
        rx_state = RX_STATE_ACTIVE;
        continue;
      }
    }
    if (RX_STATE_ACTIVE == rx_state){
      uint8_t c = ssrn_uart_read();
      rx_queue[input_rx_queue_idx].data[input_rx_buffer_idx++] = c;
      if ('\n' == c){
        if (validate_packet(&rx_queue[input_rx_queue_idx])){
          rx_queue_put_release();
          netstack_received_count++;
        } else {
          netstack_invalid_packet_count++;
        }
        rx_state = RX_STATE_IDLE;
        continue;
      }

      if (input_rx_buffer_idx >= SSRN_MAX_PACKET_LEN){
        netstack_rx_error_count++;
        rx_state = RX_STATE_IDLE;        
      }
    }
  }

  // network stack processing
  if (NETSTACK_STATE_IDLE == netstack_state &&
      !baud_rate_change){
    netstack_rx_queue_idx = rx_queue_get_acquire();
    if (netstack_rx_queue_idx >= 0){
      if (SSRN_ADDR_CODE_FORWARD == 
          rx_queue[netstack_rx_queue_idx].dst_addr_code){
        netstack_state = NETSTACK_STATE_FORWARDING;
      } else {
        netstack_state = NETSTACK_STATE_PROCESSING;
      }
    }
  }

  // rx queue --> input queue (rename as processing queue?)
  if (NETSTACK_STATE_PROCESSING == netstack_state){
    int8_t in_queue_idx = in_queue_put();
    if (in_queue_idx >= 0){
      // this packet is for us; enqueue for processing
      uint8_t idx = 0;
      uint8_t *s = rx_queue[netstack_rx_queue_idx].data;
      uint8_t *d = in_queue[in_queue_idx].data;
      do {
        *d++ = *s;
      } while (*s++ != '\n' && ++idx < SSRN_MAX_PACKET_LEN);
      netstack_inbound_count++;
      if (SSRN_ADDR_CODE_BROADCAST == 
          rx_queue[netstack_rx_queue_idx].dst_addr_code){
        netstack_state = NETSTACK_STATE_FORWARDING;
      } else {
        rx_queue_get_release();
        netstack_state = NETSTACK_STATE_IDLE;
      }
    } else {
      // can't get a spot in the input queue; drop if rx_queue is full
      //   or try to forward bcast packets

//!!!!!!!!!!!!! this is problematic; must drop broadcasts uniformly
// since forwarded bcast packet acts as response
      if (rx_queue_full()){
        if (SSRN_ADDR_CODE_BROADCAST == 
            rx_queue[netstack_rx_queue_idx].dst_addr_code){
          netstack_state = NETSTACK_STATE_FORWARDING;
        } else {
          rx_queue_get_release();
          netstack_inbound_drop_count++;
          netstack_state = NETSTACK_STATE_IDLE;
        }
      }
    }
  }

  // rx queue -> tx queue (forwarding)
  if (NETSTACK_STATE_FORWARDING == netstack_state){
    netstack_tx_queue_idx = tx_queue_put();
    if (netstack_tx_queue_idx >= 0){
      // forward packet
      uint8_t idx = 0;
      uint8_t *s = rx_queue[netstack_rx_queue_idx].data;
      uint8_t *d = tx_queue[netstack_tx_queue_idx].data;
      do {
        *d++ = *s;
      } while (*s++ != '\n' && ++idx < SSRN_MAX_PACKET_LEN);
      check_checksum(tx_queue[netstack_tx_queue_idx].data, 1);
      netstack_forwarded_count++;
      rx_queue_get_release();
      netstack_state = NETSTACK_STATE_IDLE;
    } else {
      // can't get a spot in the transmit queue; drop if rx_queue is full
      if (rx_queue_full()){
        rx_queue_get_release();
        netstack_forward_drop_count++;
        netstack_state = NETSTACK_STATE_IDLE;
      }
    }
  }

  if (baud_rate_change && !tx_queue_items && ssrn_uart_tx_done()){
    baud_rate_change = 0;
    ssrn_set_baud_rate(new_baud_rate);
  }
    
  // service output TX queue
  while (ssrn_uart_tx_available()){
    if (TX_STATE_IDLE == tx_state){
      output_tx_queue_idx = tx_queue_get_acquire();
      if (output_tx_queue_idx < 0){
        break;
      }
      output_tx_buffer_idx = 0;
      tx_state = TX_STATE_ACTIVE;
    }
    if (TX_STATE_ACTIVE == tx_state){
      uint8_t c = tx_queue[output_tx_queue_idx].data[output_tx_buffer_idx++];
      ssrn_uart_write(c);
      if ('\n' == c){
        tx_queue_get_release();
        tx_state = TX_STATE_IDLE;
        netstack_transmitted_count++;
        continue;
      }
      if (output_tx_buffer_idx >= SSRN_MAX_PACKET_LEN){
        netstack_tx_error_count++;
        tx_queue_get_release();
        tx_state = TX_STATE_IDLE;
      }
    }
  }

  if (RX_STATE_IDLE == rx_state &&
      NETSTACK_STATE_IDLE == netstack_state &&
      TX_STATE_IDLE == tx_state &&
      0 == rx_queue_items &&
      0 == in_queue_items &&
      0 == tx_queue_items){
      ssrn_idle();
  }
}

void ssrn_yield(void)
{
  static uint8_t already_yielding = 0;

  // prevent recursive yield calls from callbacks
  if (already_yielding){
    return;
  }
  already_yielding = 1;

  ssrn_network();
  
#ifdef SSRN_USE_TIMERS
  // process timer callback events
  for (uint8_t i=0; i<SSRN_NUM_TIMERS; i++){
    if ((SSRN_TIMER_TYPE_CALLBACK == ssrn_timers[i].type ||
         SSRN_TIMER_TYPE_PERIODIC_CALLBACK == ssrn_timers[i].type) &&
        (ssrn_milliseconds() - ssrn_timers[i].begin_milliseconds) >
        ssrn_timers[i].duration_milliseconds){
      if (SSRN_TIMER_TYPE_CALLBACK == ssrn_timers[i].type){
        ssrn_timers[i].type = SSRN_TIMER_TYPE_INACTIVE;
      } else {
        ssrn_timers[i].begin_milliseconds +=
          ssrn_timers[i].duration_milliseconds;
      }
      ssrn_timers[i].callback();
    }
  }
#endif //#ifdef SSRN_USE_TIMERS
  already_yielding = 0;  
}
