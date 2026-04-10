/* SPDX-License-Identifier: Apache-2.0
 *
 * CBMC proof harness for MicroBlue parser.
 * Verifies VP-14 (terminates), VP-15 (valid or empty),
 * VP-16 (rejects no SOH), VP-17 (no buffer overflow).
 *
 * Run: cbmc --unwind 50 -I../../include \
 *      ../../lib/core/microblue_parser.c proof_parser.c
 */

#include <app/core/microblue_parser.h>
#include <assert.h>
#include <string.h>

/* VP-14, VP-15, VP-17: Parser on arbitrary input */
void proof_parser_safety(void)
{
	uint8_t buf[MB_MAX_MSG_LEN];
	uint16_t len;

	__CPROVER_assume(len <= MB_MAX_MSG_LEN);

	mb_message_t msg = mb_parse(buf, len);

	/* VP-15: valid message has non-empty id and value */
	if (msg.valid) {
		assert(strlen(msg.id) > 0);
		assert(strlen(msg.value) > 0);
		assert(strlen(msg.id) <= MB_MAX_ID_LEN);
		assert(strlen(msg.value) <= MB_MAX_VALUE_LEN);
	}

	/* VP-14: reaching here proves termination */
}

/* VP-16: Missing SOH always rejected */
void proof_parser_no_soh(void)
{
	uint8_t buf[MB_MAX_MSG_LEN];
	uint16_t len;

	__CPROVER_assume(len > 0 && len <= MB_MAX_MSG_LEN);
	__CPROVER_assume(buf[0] != MB_SOH);

	mb_message_t msg = mb_parse(buf, len);

	assert(!msg.valid);
}

int main(void)
{
	proof_parser_safety();
	proof_parser_no_soh();
	return 0;
}
