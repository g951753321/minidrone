/* SPDX-License-Identifier: Apache-2.0 */

#ifndef APP_CORE_MICROBLUE_PARSER_H
#define APP_CORE_MICROBLUE_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#define MB_SOH 0x01
#define MB_STX 0x02
#define MB_ETX 0x03

#define MB_MAX_ID_LEN    8
#define MB_MAX_VALUE_LEN 32
#define MB_MAX_MSG_LEN   (1 + MB_MAX_ID_LEN + 1 + MB_MAX_VALUE_LEN + 1)

typedef struct {
	char id[MB_MAX_ID_LEN + 1];
	char value[MB_MAX_VALUE_LEN + 1];
	bool valid;
} mb_message_t;

/**
 * Parse a MicroBlue message from a byte buffer.
 * Pure function: no I/O, no side effects.
 *
 * Expected format: [SOH] id [STX] value [ETX]
 * If format is invalid, returns msg with valid=false.
 *
 * @param buf   Input byte buffer (from UART shell)
 * @param len   Number of bytes in buffer
 * @return      Parsed message (valid=true if successful)
 */
mb_message_t mb_parse(const uint8_t *buf, uint16_t len);

#endif
