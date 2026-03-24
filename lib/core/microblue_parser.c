/* SPDX-License-Identifier: Apache-2.0 */

#include <app/core/microblue_parser.h>
#include <string.h>

mb_message_t mb_parse(const uint8_t *buf, uint16_t len)
{
	mb_message_t msg;
	memset(&msg, 0, sizeof(msg));
	msg.valid = false;

	/* Minimum: SOH + 1 id char + STX + 1 value char + ETX = 5 bytes */
	if (buf == NULL || len < 5) {
		return msg;
	}

	/* Must start with SOH */
	if (buf[0] != MB_SOH) {
		return msg;
	}

	/* Find STX */
	uint16_t stx_pos = 0;
	for (uint16_t i = 1; i < len; i++) {
		if (buf[i] == MB_STX) {
			stx_pos = i;
			break;
		}
	}
	if (stx_pos == 0) {
		return msg;
	}

	/* Find ETX after STX */
	uint16_t etx_pos = 0;
	for (uint16_t i = stx_pos + 1; i < len; i++) {
		if (buf[i] == MB_ETX) {
			etx_pos = i;
			break;
		}
	}
	if (etx_pos == 0) {
		return msg;
	}

	/* Extract ID (between SOH and STX) */
	uint16_t id_len = stx_pos - 1;
	if (id_len == 0 || id_len > MB_MAX_ID_LEN) {
		return msg;
	}
	memcpy(msg.id, &buf[1], id_len);
	msg.id[id_len] = '\0';

	/* Extract VALUE (between STX and ETX) */
	uint16_t val_len = etx_pos - stx_pos - 1;
	if (val_len == 0) {
		return msg; /* Empty value → invalid */
	}
	if (val_len > MB_MAX_VALUE_LEN) {
		val_len = MB_MAX_VALUE_LEN;
	}
	memcpy(msg.value, &buf[stx_pos + 1], val_len);
	msg.value[val_len] = '\0';

	msg.valid = true;
	return msg;
}
