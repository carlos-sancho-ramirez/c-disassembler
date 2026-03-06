#include "cborigin.h"
#include <assert.h>

#define CBORIGIN_TYPE_MASK 7

/**
 * Only in case type is either CONTINUE or CALL RETURN
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

#ifdef DEBUG
const char *CBORIGIN_TYPE_NAME[] = {
	"OS", "INTERRUPTION", "CONTINUE", "CALL_RETURN", "JUMP"
};

#endif

#include <stdlib.h>

void initialize_cborigin_as_os(struct CodeBlockOrigin *origin, uint16_t relative_cs, int ds_defined_like_cs) {
	origin->flags = CBORIGIN_TYPE_OS;
	set_all_registers_undefined(&origin->regs);
	set_register_cs_relative(&origin->regs, NULL, NULL, relative_cs);
	if (ds_defined_like_cs) {
		set_register_ds_relative(&origin->regs, NULL, NULL, relative_cs);
	}
	initialize_stack(&origin->stack);
	initialize_gvwvmap(&origin->var_values);
}

int initialize_cborigin_as_interruption(struct CodeBlockOrigin *origin, const struct Registers *regs, const struct GlobalVariableWordValueMap *var_values) {
	origin->flags = CBORIGIN_TYPE_INTERRUPTION;
	copy_registers(&origin->regs, regs);
	initialize_stack(&origin->stack);
	initialize_gvwvmap(&origin->var_values);
	return copy_gvwvmap(&origin->var_values, var_values);
}

static int initialize_cborigin_structs(struct CodeBlockOrigin *origin, const struct Registers *regs, const struct Stack *stack, const struct GlobalVariableWordValueMap *var_values) {
	int error_code;
	copy_registers(&origin->regs, regs);
	initialize_stack(&origin->stack);
	if ((error_code = copy_stack(&origin->stack, stack))) {
		return error_code;
	}

	initialize_gvwvmap(&origin->var_values);
	return copy_gvwvmap(&origin->var_values, var_values);
}

int initialize_cborigin_as_continue(struct CodeBlockOrigin *origin, const struct Registers *regs, const struct Stack *stack, const struct GlobalVariableWordValueMap *var_values) {
	origin->flags = CBORIGIN_TYPE_CONTINUE;
	return initialize_cborigin_structs(origin, regs, stack, var_values);
}

int initialize_cborigin_as_call_return(struct CodeBlockOrigin *origin, unsigned int behind_count, const struct Registers *regs, const struct Stack *stack, const struct GlobalVariableWordValueMap *var_values) {
	unsigned int shifted = behind_count << CBORIGIN_BEHIND_COUNT_SHIFT;
	assert((shifted & CBORIGIN_BEHIND_COUNT_MASK) == shifted);
	origin->flags = CBORIGIN_TYPE_CALL_RETURN | (behind_count << CBORIGIN_BEHIND_COUNT_SHIFT);
	return initialize_cborigin_structs(origin, regs, stack, var_values);
}

int initialize_cborigin_as_jump(struct CodeBlockOrigin *origin, const char *instruction, const struct Registers *regs, const struct Stack *stack, const struct GlobalVariableWordValueMap *var_values) {
	int error_code;
	origin->flags = CBORIGIN_TYPE_JUMP;
	origin->instruction = instruction;
	return initialize_cborigin_structs(origin, regs, stack, var_values);
}

int get_cborigin_type(const struct CodeBlockOrigin *origin) {
	return origin->flags & CBORIGIN_TYPE_MASK;
}

const char *get_cborigin_instruction(const struct CodeBlockOrigin *origin) {
	assert(get_cborigin_type(origin) == CBORIGIN_TYPE_JUMP);
	return origin->instruction;
}

const struct Registers *get_cborigin_registers_const(const struct CodeBlockOrigin *origin) {
	return &origin->regs;
}

struct Registers *get_cborigin_registers(struct CodeBlockOrigin *origin) {
	return &origin->regs;
}

struct Stack *get_cborigin_stack(struct CodeBlockOrigin *origin) {
	return &origin->stack;
}

struct GlobalVariableWordValueMap *get_cborigin_var_values(struct CodeBlockOrigin *origin) {
	return &origin->var_values;
}

int is_cborigin_ready_to_be_evaluated(const struct CodeBlockOrigin *origin) {
	return origin->flags & CBORIGIN_FLAG_READY_TO_BE_EVALUATED;
}

int get_cborigin_behind_count(const struct CodeBlockOrigin *origin) {
	return (origin->flags & CBORIGIN_BEHIND_COUNT_MASK) >> CBORIGIN_BEHIND_COUNT_SHIFT;
}

void set_cborigin_ready_to_be_evaluated(struct CodeBlockOrigin *origin) {
	origin->flags |= CBORIGIN_FLAG_READY_TO_BE_EVALUATED;
}

#ifdef DEBUG
#include <stdio.h>

void print_cborigin(const struct CodeBlockOrigin *origin) {
	const int origin_type = get_cborigin_type(origin);
	if (origin_type == CBORIGIN_TYPE_OS) {
		fprintf(stderr, "OS");
	}
	else if (origin_type == CBORIGIN_TYPE_INTERRUPTION) {
		fprintf(stderr, "INT");
	}
	else if (origin_type == CBORIGIN_TYPE_CONTINUE) {
		fprintf(stderr, "CONT(");
		if (is_cborigin_ready_to_be_evaluated(origin)) {
			fprintf(stderr, "V)");
		}
		else {
			fprintf(stderr, "x)");
		}
	}
	else if (origin_type == CBORIGIN_TYPE_CALL_RETURN) {
		fprintf(stderr, "CR(%d,", get_cborigin_behind_count(origin));
		if (is_cborigin_ready_to_be_evaluated(origin)) {
			fprintf(stderr, "V)");
		}
		else {
			fprintf(stderr, "x)");
		}
	}
	else if (origin_type == CBORIGIN_TYPE_JUMP) {
		fprintf(stderr, "JMP");
	}
}

#endif /* DEBUG */
