#ifndef _SSRN_CONFIG_H
#define _SSRN_CONFIG_H

#include "mcc_generated_files/mcc.h"

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

#endif // #ifndef _SSRN_CONFIG_H
