#ifndef SSRN_PLATFORM_H
#define SSRN_PLATFORM_H

// turn off "unused function" warnings in xc8
#pragma warning disable 520

#include <stdint.h>

#include "mcc.h"
#include "ssrn-config.h"
#include "ssrn-core.h"
#include "ssrn-platform.h"

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
uint32_t ssrn_voltage_mv(void);

#ifdef SSRN_USE_TIMERS
extern volatile uint32_t ssrn_millisecond_counter;
uint32_t ssrn_milliseconds(void);
void ssrn_delay_ms(uint32_t duration);
void ssrn_set_timer_event(uint8_t idx,
                          uint32_t duration_milliseconds,
                          uint8_t periodic_flag);
void ssrn_set_timer_callback(uint8_t idx,
                             uint32_t duration_milliseconds,
                             void (*callback)(void),
                             uint8_t periodic_flag);

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

#endif // #ifndef SSRN_PLATFORM_H
