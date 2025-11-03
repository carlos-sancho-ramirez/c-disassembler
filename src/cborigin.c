#include "cborigin.h"
#include <assert.h>

#define CBORIGIN_TYPE_MASK 7

/**
 * Only in case tyoe is CALL RETURN
 *
 * If set, it means that the registers and variables in this origin has been
 * set up as the result of evaluating the block where the previous instruction
 * is calling, and then, this block can now be evaluated.
 *
 * If clear, it means that the block where the call is
 */
#define CBORIGIN_FLAG_READY_TO_BE_EVALUATED 8

/**
 * Mask for the amount of bytes that you have to substract to the start of this
 * block to find the call instruction.
 */
#define CBORIGIN_BEHIND_COUNT_MASK 0x70

/**
 * Shift of bits for the amount of bytes that you have to substract to the start
 * of this block to find the call instruction.
 */
#define CBORIGIN_BEHIND_COUNT_SHIFT 4

/**
 * Only in case type is CALL RETURN
 *
 * If set, it means that even if there is a call instruction before the start
 * of this block, we can ensure that this block is never executed after the
 * return of that call. So, if it is the only origin, it may not be valuable to
 * disassemble this block as it is never executed.
 */
#define CBORIGIN_FLAG_NEVER_REACHED 0x80

int get_cborigin_type(const struct CodeBlockOrigin *origin) {
	return origin->flags & CBORIGIN_TYPE_MASK;
}

int is_cborigin_ready_to_be_evaluated(const struct CodeBlockOrigin *origin) {
	return origin->flags & CBORIGIN_FLAG_READY_TO_BE_EVALUATED;
}

int is_marked_as_never_reached(const struct CodeBlockOrigin *origin) {
	return (origin->flags & (CBORIGIN_TYPE_MASK | CBORIGIN_FLAG_NEVER_REACHED)) == (CBORIGIN_TYPE_CALL_RETURN | CBORIGIN_FLAG_NEVER_REACHED);
}

int get_cborigin_behind_count(const struct CodeBlockOrigin *origin) {
	return (origin->flags & CBORIGIN_BEHIND_COUNT_MASK) >> CBORIGIN_BEHIND_COUNT_SHIFT;
}

void set_os_type_in_cborigin(struct CodeBlockOrigin *origin) {
	origin->flags = CBORIGIN_TYPE_OS;
}

void set_interruption_type_in_cborigin(struct CodeBlockOrigin *origin) {
	origin->flags = CBORIGIN_TYPE_INTERRUPTION;
}

void set_call_return_type_in_cborigin(struct CodeBlockOrigin *origin, unsigned int behind_count) {
	unsigned int shifted = behind_count << CBORIGIN_BEHIND_COUNT_SHIFT;
	assert((shifted & CBORIGIN_BEHIND_COUNT_MASK) == shifted);
	origin->flags = CBORIGIN_TYPE_CALL_RETURN | (behind_count << CBORIGIN_BEHIND_COUNT_SHIFT);
}

void set_jump_type_in_cborigin(struct CodeBlockOrigin *origin, const char *instruction) {
	origin->flags = CBORIGIN_TYPE_JUMP;
	origin->instruction = instruction;
}

void set_cborigin_ready_to_be_evaluated(struct CodeBlockOrigin *origin) {
	origin->flags |= CBORIGIN_FLAG_READY_TO_BE_EVALUATED;
}
