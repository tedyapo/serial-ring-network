#ifndef _SSRN_CONFIG_H
#define _SSRN_CONFIG_H

#define SSRN_USE_TIMERS
#define SSRN_NUM_TIMERS 5

#include "mcc.h"

// turn off "unused function" warnings in xc8
#pragma warning disable 520

#include "ssrn.h"

#include <stdint.h>

#define SSRN_NODE_TYPE "AM2302-V1"
#define SSRN_NODE_ID   0x01

#define SSRN_DEFAULT_BAUD_RATE SSRN_BAUD_1200

#define SSRN_RX_QUEUE_LEN 2
#define SSRN_IN_QUEUE_LEN 2
#define SSRN_TX_QUEUE_LEN 2

const char *ssrn_node_type(void);
uint16_t ssrn_node_id(void);
uint8_t ssrn_uart_rx_available(void);
uint8_t ssrn_uart_tx_available(void);
uint8_t ssrn_uart_tx_done(void);
uint8_t ssrn_uart_read(void);
void ssrn_uart_write(uint8_t c);
void ssrn_set_baud_rate(ssrn_baud_rate_t rate);
uint8_t ssrn_local_packet(ssrn_packet_t *p);
void ssrn_init(void);
void ssrn_reset(void);
void ssrn_idle(void);

#ifdef SSRN_USE_TIMERS
extern volatile uint32_t ssrn_millisecond_counter;
uint32_t ssrn_milliseconds(void);
void ssrn_set_timer_event(uint8_t idx, uint32_t duration_milliseconds);
void ssrn_set_timer_callback(uint8_t idx,
                             uint32_t duration_milliseconds,
                             void (*callback)(void));

void ssrn_cancel_timer(uint8_t idx);

#if SSRN_NUM_TIMERS > 0
#define SSRN_TIMER0 0
#endif
#if SSRN_NUM_TIMERS > 1
#define SSRN_TIMER1 1
#endif
#if SSRN_NUM_TIMERS > 2
#define SSRN_TIMER2 2
#endif
#if SSRN_NUM_TIMERS > 3
#define SSRN_TIMER3 3
#endif
#if SSRN_NUM_TIMERS > 4
#define SSRN_TIMER4 4
#endif
#if SSRN_NUM_TIMERS > 5
#define SSRN_TIMER5 5
#endif
#if SSRN_NUM_TIMERS > 6
#define SSRN_TIMER6 6
#endif
#if SSRN_NUM_TIMERS > 7
#define SSRN_TIMER7 7
#endif
#if SSRN_NUM_TIMERS > 8
#define SSRN_TIMER8 8
#endif
#if SSRN_NUM_TIMERS > 9
#define SSRN_TIMER9 9
#endif
#endif //#ifdef SSRN_USE_TIMERS

#endif // #ifndef _SSRN_CONFIG_H
