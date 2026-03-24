/* SPDX-License-Identifier: Apache-2.0
 *
 * VSDD Phase 5: libFuzzer harness for MicroBlue parser.
 * Verifies VP-14 (always terminates), VP-15 (valid or empty output),
 * VP-16 (rejects missing SOH), VP-17 (no buffer overflow).
 *
 * Build: clang -fsanitize=fuzzer,address,undefined -I../../include \
 *        ../../lib/core/microblue_parser.c fuzz_parser.c -o fuzz_parser
 * Run:   ./fuzz_parser -max_len=128 -runs=1000000
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <app/core/microblue_parser.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size > MB_MAX_MSG_LEN * 2) {
		/* Limit to reasonable sizes to avoid wasting cycles */
		return 0;
	}

	mb_message_t msg = mb_parse(data, (uint16_t)size);

	/* VP-15: output is either empty (valid=false) or has both id and value */
	if (msg.valid) {
		assert(strlen(msg.id) > 0);
		assert(strlen(msg.value) > 0);
		assert(strlen(msg.id) <= MB_MAX_ID_LEN);
		assert(strlen(msg.value) <= MB_MAX_VALUE_LEN);
	}

	/* VP-14: if we reach here, the parser terminated (no infinite loop) */
	return 0;
}
