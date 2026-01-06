#include "funcfind.h"
#include "counter.h"
#include "packed.h"
#include "printd.h"

#include <assert.h>

#define STATE_FLAG_RET_TYPE_MASK 3
#define STATE_FLAG_RET_TYPE_UNKNOWN 0
#define STATE_FLAG_RET_TYPE_NEAR 1
#define STATE_FLAG_RET_TYPE_FAR 2
#define STATE_FLAG_RET_TYPE_INT 3

#define STATE_FLAG_USES_BP 4
#define STATE_FLAG_OWNS_BP 8

struct FuncState {
	int flags;
	int return_size;
	int min_bp_diff;
	int max_bp_diff;
	packed_data_t *included_blocks;
	packed_data_t *starting_blocks;
};

struct FuncStackState {
	/**
	 * Index within the included_blocks for the starting block.
	 */
	unsigned int start_included_block_index;

	/**
	 * This array will be initialised to contain as positions as the number of blocks
	 * included within the companion state, and it reveals the number of bytes
	 * used in the stack at the beginning of each block regarding the beginning
	 * of the function. In other words, it reveals the negative difference
	 * between the original value of SP register at the start of the function
	 * and its value at the beginning of each block.
	 *
	 * This information can be used to detect mistakes or weird behaviours
	 * regarding the stack, and abort the analysis of the function.
	 *
	 * Each position willl be initialised with -1 to denote that the block has
	 * not been evaluated yet. But none of the position must include a negative
	 * number at the end of the analysis.
	 */
	int *stack_size;
};

static int find_block_index(struct CodeBlock **blocks, unsigned int block_count, const char *start) {
	unsigned int first = 0;
	unsigned int last = block_count;

	while (first < last) {
		unsigned int index = (first + last) / 2;
		const char *block_start = blocks[index]->start;
		if (start < block_start) {
			last = index;
		}
		else if (start == block_start) {
			return index;
		}
		else {
			first = index + 1;
		}
	}

	return -1;
}

static int find_block_index_containing_instruction(struct CodeBlock **blocks, unsigned int block_count, const char *instruction) {
	unsigned int first = 0;
	unsigned int last = block_count;

	while (first < last) {
		unsigned int index = (first + last) / 2;
		const char *block_start = blocks[index]->start;
		if (instruction < block_start) {
			last = index;
		}
		else if (instruction == block_start) {
			return index;
		}
		else {
			const char *block_end = blocks[index]->end;
			if (instruction < block_end) {
				return index;
			}
			else {
				first = index + 1;
			}
		}
	}

	return -1;
}

static int find_included_block_index(int block_index, const int *block_map, int included_blocks_count) {
	unsigned int first = 0;
	unsigned int last = included_blocks_count;

	while (first < last) {
		unsigned int index = (first + last) / 2;
		int this_block_index = block_map[index];

		if (block_index < this_block_index) {
			last = index;
		}
		else if (block_index == this_block_index) {
			return index;
		}
		else {
			first = index + 1;
		}
	}

	return -1;
}

static int read_addr_diff(struct Reader *reader, int modRm) {
	if ((modRm & 0xC0) == 0) {
		return ((modRm & 0xC7) == 6)? read_next_word(reader) : 0;
	}
	else if ((modRm & 0xC0) == 0x40) {
		int diff = read_next_byte(reader);
		return (diff >= 0x80)? diff - 0x100 : diff;
	}
	else if ((modRm & 0xC0) == 0x80) {
		return read_next_word(reader);
	}
	else {
		return 0;
	}
}

static int evaluate_block(struct CodeBlock **blocks, unsigned int block_count, unsigned int block_index, packed_data_t *available_blocks, struct FuncState *state, const struct FunctionList *func_list) {
	const int is_first_block = count_set_bits_in_bitset(state->included_blocks, block_count) == 1;
	const struct CodeBlock *block = blocks[block_index];
	struct Reader reader;
	int error_code;
	int starts_with_push_bp = 0;

	struct CodeBlockOriginList *origin_list = &block->origin_list;
	const unsigned int origin_count = origin_list->origin_count;
	unsigned int origin_index;
	for (origin_index = 0; origin_index < origin_count; origin_index++) {
		struct CodeBlockOrigin *origin = origin_list->sorted_origins[origin_index];
		const int origin_type = get_cborigin_type(origin);
		if (origin_type == CBORIGIN_TYPE_CONTINUE || origin_type == CBORIGIN_TYPE_CALL_RETURN) {
			if (block_index == 0) {
				WARN_PRINT0("'Continue' or 'call return' origin type found in the first block.\n");
				return 1;
			}

			set_bitset_value(state->included_blocks, block_index - 1, 1);
		}
		else if (origin_type == CBORIGIN_TYPE_JUMP) {
			const int jmp_opcode0 = ((int) *origin->instruction) & 0xFF;
			if (jmp_opcode0 == 0xE8) { /* CALL */
				set_bitset_value(state->starting_blocks, block_index, 1);
			}
			else if (jmp_opcode0 == 0xE9 || (jmp_opcode0 & 0xF0) == 0x70 || (jmp_opcode0 & 0xFC) == 0xE0 || jmp_opcode0 == 0xEB) { /* JMP and its conditionals */
				find_block_index_containing_instruction(adad
			}
		}
	}

	reader.buffer = block->start;
	reader.buffer_index = 0;
	reader.buffer_size = block->end - block->start;

	while (reader.buffer_index < reader.buffer_size) {
		unsigned int buffer_index = reader.buffer_index;
		unsigned int next_instruction_index;
		int value0;

		if ((error_code = read_for_instruction_length(&reader))) {
			return error_code;
		}

		next_instruction_index = reader.buffer_index;
		assert(next_instruction_index > buffer_index);
		reader.buffer_index = buffer_index;

		value0 = read_next_byte(&reader);
		if ((value0 & 0xC4) == 0) {
			const int value1 = read_next_byte(&reader);
			if ((value1 & 0xC7) == 0x46 || (value1 & 0xC7) == 0x86) {
				const int diff = read_addr_diff(&reader, value1);
				state->flags |= STATE_FLAG_USES_BP;
				if (diff > 0 && diff > state->max_bp_diff) {
					state->max_bp_diff = diff;
				}
				else if (diff < 0 && diff < state->min_bp_diff) {
					state->min_bp_diff = diff;
				}
			}
		}
		else if (value0 == 0x55) {
			if (reader.buffer_index == 1) {
				starts_with_push_bp = 1;
			}
		}
		else if ((value0 & 0xF0) == 0x70 || (value0 & 0xFC) == 0xE0 || value0 == 0xEB) {
			const int value1 = read_next_byte(&reader);
			const int diff = (value1 >= 0x80)? value1 - 0x100 : value1;
			const char *jump_destination = block->start + reader.buffer_index + diff;
			const int target_block_index = find_block_index(blocks, block_count, block->start + reader.buffer_index + diff);
			if (target_block_index >= 0) {
				if (get_bitset_value(available_blocks, target_block_index)) {
					set_bitset_value(state->included_blocks, target_block_index, 1);
				}
				else {
					WARN_PRINT0("7X or EX. Block found, but already in use.\n");
					return 1;
				}
			}
			else {
				WARN_PRINT0("7X or EX on evaluating block. Unable to find a block matching the expected start position.\n");
				return 1;
			}
		}
		else if ((value0 & 0xF0) == 0x80 || (value0 & 0xFE) == 0xC0 || (value0 & 0xFC) == 0xC4 || (value0 & 0xFC) == 0xD0 || (value0 & 0xFE) == 0xF6 || value0 == 0xFF) {
			const int value1 = read_next_byte(&reader);
			const int bp_relative = (value1 & 0xC7) == 0x46 || (value1 & 0xC7) == 0x86;

			if (bp_relative) {
				const int diff = read_addr_diff(&reader, value1);
				state->flags |= STATE_FLAG_USES_BP;
				if (diff > 0 && diff > state->max_bp_diff) {
					state->max_bp_diff = diff;
				}
				else if (diff < 0 && diff < state->min_bp_diff) {
					state->min_bp_diff = diff;
				}
			}

			if (value0 == 0x8B && value1 == 0xEC && is_first_block && starts_with_push_bp && reader.buffer_index == 3) {
				state->flags |= STATE_FLAG_OWNS_BP;
			}

			if (value0 == 0xFF && (value1 & 0x38) == 0x10 && bp_relative) {
				if (block_index + 1 < block_count && blocks[block_index + 1]->start == reader.buffer + reader.buffer_index) {
					struct CodeBlock *next_block = blocks[block_index + 1];
					const int instruction_length = (value1 == 0x96)? 4 : 3;
					if (index_of_cborigin_of_type_call_return(&next_block->origin_list, instruction_length) >= 0) {
						if (get_bitset_value(available_blocks, block_index + 1)) {
							set_bitset_value(state->included_blocks, block_index + 1, 1);
						}
						else {
							WARN_PRINT0("Next block has origin of type 'call return', but it is already in use.\n");
							return 1;
						}
					}
					else {
						WARN_PRINT0("Block found after 'call' instruction, but does not have the expected origin of type 'call return'.\n");
						return 1;
					}
				}
				else {
					WARN_PRINT0("call instruction found, but no return block found.\n");
					return 1;
				}
			}
			else if (value0 == 0xFF && ((value1 & 0x30) == 0x10 || (value1 & 0x30) == 0x20)) {
				DEBUG_PRINT0("Opcode FF for call or jmp found. For now, we do not allow this. Skipping this function evaluation.\n");
				return 1;
			}
		}
		else if (value0 == 0xC2) {
			const int ret_size = read_next_word(&reader);
			if ((state->flags & STATE_FLAG_RET_TYPE_MASK) == STATE_FLAG_RET_TYPE_UNKNOWN) {
				state->flags |= STATE_FLAG_RET_TYPE_NEAR;
				state->return_size = ret_size;
			}
			else if ((state->flags & STATE_FLAG_RET_TYPE_MASK) == STATE_FLAG_RET_TYPE_NEAR) {
				if (ret_size != state->return_size) {
					WARN_PRINT0("Mismatch between return sizes.\n");
					return 1;
				}
			}
			else {
				WARN_PRINT0("Mismatch between return types.\n");
				return 1;
			}
		}
		else if (value0 == 0xC3) {
			if ((state->flags & STATE_FLAG_RET_TYPE_MASK) == STATE_FLAG_RET_TYPE_UNKNOWN) {
				state->flags |= STATE_FLAG_RET_TYPE_NEAR;
				state->return_size = 0;
			}
			else if ((state->flags & STATE_FLAG_RET_TYPE_MASK) == STATE_FLAG_RET_TYPE_NEAR) {
				if (state->return_size != 0) {
					WARN_PRINT0("Mismatch between return sizes.\n");
					return 1;
				}
			}
			else {
				WARN_PRINT0("Mismatch between return types.\n");
				return 1;
			}
		}
		else if (value0 == 0xCB) {
			if ((state->flags & STATE_FLAG_RET_TYPE_MASK) == STATE_FLAG_RET_TYPE_UNKNOWN) {
				state->flags |= STATE_FLAG_RET_TYPE_FAR;
				state->return_size = 0;
			}
			else if ((state->flags & STATE_FLAG_RET_TYPE_MASK) == STATE_FLAG_RET_TYPE_FAR) {
				if (state->return_size != 0) {
					WARN_PRINT0("Mismatch between return sizes.\n");
					return 1;
				}
			}
			else {
				WARN_PRINT0("Mismatch between return types.\n");
				return 1;
			}
		}
		else if (value0 == 0xE8) {
			const int diff = read_next_word(&reader);
			const char *target = block->start + reader.buffer_index + diff;
			const int target_block_index = find_block_index(blocks, block_count, target);
			if (target_block_index >= 0) {
				const int target_func_index = index_of_func_with_block_start(func_list, target);
				if (target_block_index >= 0) {
					if (block_index + 1 < block_count && blocks[block_index + 1]->start == reader.buffer + reader.buffer_index) {
						struct CodeBlock *next_block = blocks[block_index + 1];
						if (index_of_cborigin_of_type_call_return(&next_block->origin_list, 3) >= 0) {
							if (get_bitset_value(available_blocks, block_index + 1)) {
								set_bitset_value(state->included_blocks, block_index + 1, 1);
							}
							else {
								WARN_PRINT0("Next block has origin of type 'call return', but it is already in use.\n");
								return 1;
							}
						}
						else {
							WARN_PRINT0("Block found after 'call' instruction, but does not have the expected origin of type 'call return'.\n");
							return 1;
						}
					}
					else {
						WARN_PRINT0("call instruction found, but no return block found.\n");
						return 1;
					}
				}
				else {
					WARN_PRINT2("Trying to call to +%x:%x, but it is not yet considered a valid function.\n", blocks[target_block_index]->relative_cs, blocks[target_block_index]->ip);
					return 1;
				}
			}
			else {
				WARN_PRINT0("Target block not found.\n");
				return 1;
			}
		}
		else if (value0 == 0xE9) {
			const int diff = read_next_word(&reader);
			const int target_block_index = find_block_index(blocks, block_count, block->start + reader.buffer_index + diff);
			if (target_block_index >= 0) {
				if (get_bitset_value(available_blocks, target_block_index)) {
					set_bitset_value(state->included_blocks, target_block_index, 1);
				}
				else {
					WARN_PRINT0("E9. Block found, but already in use.\n");
					return 1;
				}
			}
			else {
				WARN_PRINT0("E9. Unable to find a block matching the expected start position.\n");
				return 1;
			}
		}
		else if (value0 == 0xCD) {
			const int value1 = read_next_byte(&reader);
			const int target_block_index = find_block_index(blocks, block_count, block->start + reader.buffer_index);
			if (target_block_index >= 0) {
				if (get_bitset_value(available_blocks, target_block_index)) {
					set_bitset_value(state->included_blocks, target_block_index, 1);
				}
				else {
					WARN_PRINT0("Block found after the interruption call, but already in use.\n");
					return 1;
				}
			}
			else {
				WARN_PRINT0("Unable to find a block after the interruption call.\n");
				return 1;
			}
		}
		else if (value0 == 0xEA) {
			DEBUG_PRINT0("Opcode EA found. For now, we do not allow it. Skipping this function evaluation.\n");
			return 1;
		}

		reader.buffer_index = next_instruction_index;
	}

	if (block_index + 1 < block_count && blocks[block_index + 1]->start == reader.buffer + reader.buffer_index) {
		struct CodeBlock *next_block = blocks[block_index + 1];
		if (next_block->start == reader.buffer + reader.buffer_index && index_of_cborigin_of_type_continue(&next_block->origin_list) >= 0) {
			if (get_bitset_value(available_blocks, block_index + 1)) {
				set_bitset_value(state->included_blocks, block_index + 1, 1);
			}
			else {
				WARN_PRINT0("Next block has origin of type 'continue', but it is already in use.\n");
				return 1;
			}
		}
	}

	return 0;
}

static int find_all_blocks_in_function(struct CodeBlock **blocks, unsigned int block_count, packed_data_t *available_blocks, struct FuncState *state, const struct FunctionList *func_list) {
	packed_data_t *evaluated_blocks = allocate_bitset(block_count);
	int block_index;

	if (!evaluated_blocks) {
		return 1;
	}

	while(!are_bitsets_equal(state->included_blocks, evaluated_blocks, block_count)) {
		for (block_index = 0; block_index < block_count; block_index++) {
			if (get_bitset_value(state->included_blocks, block_index) && !get_bitset_value(evaluated_blocks, block_index)) {
				int error_code;
				if ((error_code = evaluate_block(blocks, block_count, block_index, available_blocks, state, func_list))) {
					free(evaluated_blocks);
					return error_code;
				}

				set_bitset_value(evaluated_blocks, block_index, 1);
			}
		}
	}

	free(evaluated_blocks);
	return 0;
}

static int check_block_stack(struct CodeBlock **blocks, unsigned int block_count, unsigned int block_index, unsigned int included_block_index, unsigned int included_blocks_count, const struct FuncState *state, struct FuncStackState *stack_state, const int *block_map, const struct FunctionList *func_list) {
	const struct CodeBlock *block = blocks[block_index];
	struct Reader reader;
	int error_code;
	unsigned int stack_word_count = stack_state->stack_size[included_block_index];

	assert(stack_word_count >= 0);
	reader.buffer = block->start;
	reader.buffer_index = 0;
	reader.buffer_size = block->end - block->start;

	while (reader.buffer_index < reader.buffer_size) {
		unsigned int buffer_index = reader.buffer_index;
		unsigned int next_instruction_index;
		int value0;

		if ((error_code = read_for_instruction_length(&reader))) {
			return error_code;
		}

		next_instruction_index = reader.buffer_index;
		assert(next_instruction_index > buffer_index);
		reader.buffer_index = buffer_index;

		value0 = read_next_byte(&reader);
		/* TODO: ADD and SUB of sp is missing */
		if ((value0 & 0xE7) == 0x06) {
			stack_word_count++;
		}
		else if ((value0 & 0xE7) == 0x07 && value0 != 0x0F) {
			if (stack_word_count == 0) {
				WARN_PRINT0("Pop instruction found, but stack is empty within the function.\n");
				return 1;
			}
			--stack_word_count;
		}
		else if ((value0 & 0xF8) == 0x50) {
			stack_word_count++;
		}
		else if ((value0 & 0xF8) == 0x58) {
			if (stack_word_count == 0) {
				WARN_PRINT0("Pop instruction found, but stack is empty within the function.\n");
				return 1;
			}
			--stack_word_count;
		}
		else if ((value0 & 0xF0) == 0x70 || (value0 & 0xFC) == 0xE0 || value0 == 0xEB) {
			const int value1 = read_next_byte(&reader);
			const int diff = (value1 >= 0x80)? value1 - 0x100 : value1;
			const char *jump_destination = block->start + reader.buffer_index + diff;
			const int target_block_index = find_block_index(blocks, block_count, block->start + reader.buffer_index + diff);
			const int target_included_block_index = find_included_block_index(target_block_index, block_map, included_blocks_count);

			if (target_included_block_index >= 0) {
				const int target_stack_size = stack_state->stack_size[target_included_block_index];
				if (target_stack_size < 0) {
					stack_state->stack_size[target_included_block_index] = stack_word_count;
				}
				else if (target_stack_size != stack_word_count) {
					WARN_PRINT0("Mismatch among expected stack sizes on this block.\n");
					return 1;
				}
			}
			else {
				WARN_PRINT0("7X or EX on checking block stack. Unable to find a block matching the expected start position.\n");
				return 1;
			}
		}
		else if ((value0 & 0xFE) == 0xC2) {
			if (stack_word_count != 0) {
				WARN_PRINT0("ret instruction reached without popping the rest of the stack.\n");
				return 1;
			}
		}
		else if (value0 == 0xCB) {
			if (stack_word_count != 0) {
				WARN_PRINT0("retf instruction reached without popping the rest of the stack.\n");
				return 1;
			}
		}
		else if (value0 == 0xE8) {
			const int diff = read_next_word(&reader);
			const char *target = block->start + reader.buffer_index + diff;
			const int target_block_index = find_block_index(blocks, block_count, target);
			if (target_block_index >= 0) {
				const int target_func_index = index_of_func_with_block_start(func_list, target);
				if (target_func_index >= 0) {
					if (block_index + 1 < block_count && blocks[block_index + 1]->start == reader.buffer + reader.buffer_index) {
						struct CodeBlock *next_block = blocks[block_index + 1];
						if (index_of_cborigin_of_type_call_return(&next_block->origin_list, 3) >= 0) {
							const struct Function *target_func = func_list->sorted_funcs[target_func_index];
							if (target_func->return_size & 1) {
								WARN_PRINT0("Target function has an odd value in its return size.\n");
								return 1;
							}
							else if (target_func->return_size > stack_word_count * 2) {
								WARN_PRINT0("Target function has a return value bigger than the current stack count.\n");
								return 1;
							}
							else {
								int next_block_stack_size = stack_state->stack_size[included_block_index + 1];
								stack_word_count -= target_func->return_size / 2;
								if (next_block_stack_size < 0) {
									stack_state->stack_size[included_block_index + 1] = stack_word_count;
								}
								else if (next_block_stack_size != stack_word_count) {
									WARN_PRINT0("Stack word count mismatch for the next block");
									return 1;
								}
							}
						}
						else {
							WARN_PRINT0("Block found after 'call' instruction, but does not have the expected origin of type 'call return'.\n");
							return 1;
						}
					}
					else {
						WARN_PRINT0("call instruction found, but no return block found.\n");
						return 1;
					}
				}
				else {
					WARN_PRINT2("Trying to call to +%x:%x, but it is not yet considered a valid function.\n", blocks[target_block_index]->relative_cs, blocks[target_block_index]->ip);
					return 1;
				}
			}
			else {
				WARN_PRINT0("Target block not found.\n");
				return 1;
			}
		}
		else if (value0 == 0xE9) {
			const int diff = read_next_word(&reader);
			const int target_block_index = find_block_index(blocks, block_count, block->start + reader.buffer_index + diff);
			const int target_included_block_index = find_included_block_index(target_block_index, block_map, included_blocks_count);

			if (target_included_block_index >= 0) {
				const int target_stack_size = stack_state->stack_size[target_included_block_index];
				if (target_stack_size < 0) {
					stack_state->stack_size[target_included_block_index] = stack_word_count;
				}
				else if (target_stack_size != stack_word_count) {
					WARN_PRINT0("Mismatch among expected stack sizes on this block.\n");
					return 1;
				}
			}
			else {
				WARN_PRINT0("Opcode E9. Unable to find a block matching the expected start position.\n");
				return 1;
			}
		}
		else if (value0 == 0xEA) {
			DEBUG_PRINT0("Opcode EA found. For now, we do not allow it. Skipping this function evaluation.\n");
			return 1;
		}
		else if (value0 == 0xFF) {
			const int value1 = read_next_byte(&reader);
			if (value1 == 0x56 || value1 == 0x96) {
				/* Let's assume for now that the argument is a pointer to a function finishing with "ret" (without return size)... C-style, so nothing to substract from stack_word_count */
			}
			else if ((value1 & 0x30) == 0x10 || (value1 & 0x30) == 0x20) {
				DEBUG_PRINT0("Opcode FF for call or jmp found. For now, we do not allow this. Skipping this function evaluation.\n");
				return 1;
			}
		}

		reader.buffer_index = next_instruction_index;
	}

	return 0;
}

static int check_stack_in_all_blocks(struct CodeBlock **blocks, unsigned int block_count, const struct FuncState *state, struct FuncStackState *stack_state, const int *block_map, const struct FunctionList *func_list) {
	const packed_data_t *included_blocks = state->included_blocks;
	const unsigned int included_blocks_count = count_set_bits_in_bitset(included_blocks, block_count);
	packed_data_t *evaluated_included_blocks = allocate_bitset(included_blocks_count);
	int block_index = 0;
	int included_block_index = 0;

	for (block_index = 0; block_index < block_count; block_index++) {
		if (get_bitset_value(included_blocks, block_index)) {
			if (!get_bitset_value(evaluated_included_blocks, included_block_index) && stack_state->stack_size[included_block_index] >= 0) {
				int error_code;
				if ((error_code = check_block_stack(blocks, block_count, block_index, included_block_index, included_blocks_count, state, stack_state, block_map, func_list))) {
					free(evaluated_included_blocks);
					return error_code;
				}
			}

			++included_block_index;
		}
	}

	free(evaluated_included_blocks);
	return 0;
}

int find_functions(struct CodeBlock **blocks, unsigned int block_count, struct FunctionList *func_list) {
	packed_data_t *available_blocks = allocate_bitset(block_count);
	int block_index;
	int new_function_added;

	DEBUG_PRINT0("Finding functions\n");
	if (!available_blocks) {
		return 1;
	}

	for (block_index = 0; block_index < block_count; block_index++) {
		set_bitset_value(available_blocks, block_index, 1);
	}

	do {
		new_function_added = 0;

		for (block_index = 0; block_index < block_count; block_index++) {
			if (get_bitset_value(available_blocks, block_index)) {
				const struct CodeBlock *block = blocks[block_index];
				const struct CodeBlockOriginList *origin_list = &block->origin_list;
				int origin_index;
				int valid_origins = origin_list->origin_count > 0;

				for (origin_index = 0; origin_index < origin_list->origin_count; origin_index++) {
					const struct CodeBlockOrigin *origin = origin_list->sorted_origins[origin_index];

					if (get_cborigin_type(origin) == CBORIGIN_TYPE_JUMP) {
						const int opcode = *(origin->instruction) & 0xFF;
						if (opcode == 0xFF) {
							const int opcode1 = (opcode == 0xFF)? *(origin->instruction + 1) & 0xFF : 0;
							if ((opcode1 & 0x38) != 0x10 && (opcode1 & 0x38) != 0x20) {
								valid_origins = 0;
								break;
							}
						}
						else if (opcode != 0xE8) {
							valid_origins = 0;
							break;
						}
					}
					else {
						valid_origins = 0;
						break;
					}
				}

				if (valid_origins) {
					struct FuncState state;
					state.flags = 0;
					state.min_bp_diff = 0;
					state.max_bp_diff = 0;
					state.included_blocks = allocate_bitset(block_count);
					set_bitset_value(state.included_blocks, block_index, 1);

					DEBUG_PRINT2(" Finding all blocks in function starting at +%x:%x\n", block->relative_cs, block->ip);
					if (!find_all_blocks_in_function(blocks, block_count, available_blocks, &state, func_list) && (state.flags & STATE_FLAG_RET_TYPE_MASK) != STATE_FLAG_RET_TYPE_UNKNOWN) {
						struct FuncStackState stack_state;
						const unsigned int included_blocks_count = count_set_bits_in_bitset(state.included_blocks, block_count);
						int included_block_index;
						int block_index2;
						int *block_map;

						stack_state.stack_size = malloc(sizeof(int) * included_blocks_count);
						if (!stack_state.stack_size) {
							free(state.included_blocks);
							free(available_blocks);
							return 1;
						}

						block_map = malloc(sizeof(int) * included_blocks_count);
						if (!block_map) {
							free(stack_state.stack_size);
							free(state.included_blocks);
							free(available_blocks);
							return 1;
						}

						for (included_block_index = 0; included_block_index < included_blocks_count; included_block_index++) {
							stack_state.stack_size[included_block_index] = -1;
						}

						included_block_index = -1;
						for (block_index2 = 0; block_index2 < block_count; block_index2++) {
							if (get_bitset_value(state.included_blocks, block_index2)) {
								++included_block_index;
								if (block_index2 == block_index) {
									stack_state.start_included_block_index = included_block_index;
									stack_state.stack_size[included_block_index] = 0;
									break;
								}
							}
						}

						included_block_index = -1;
						for (block_index2 = 0; block_index2 < block_count; block_index2++) {
							if (get_bitset_value(state.included_blocks, block_index2)) {
								block_map[++included_block_index] = block_index2;
							}
						}

						DEBUG_PRINT0("  Checking if stack is properly balanced.\n");
						if (!check_stack_in_all_blocks(blocks, block_count, &state, &stack_state, block_map, func_list)) {
							struct Function *new_func = prepare_new_func(func_list);
							int all_block_index;
							int new_blocks_index = 0;
							int error_code;

							initialize_func(new_func);
							set_function_return_type(new_func, state.flags & STATE_FLAG_RET_TYPE_MASK);

							if (state.flags & STATE_FLAG_USES_BP) {
								if ((state.flags & STATE_FLAG_RET_TYPE_MASK) == STATE_FLAG_RET_TYPE_NEAR) {
									if (state.max_bp_diff >= 2) {
										set_function_uses_bp(new_func, (state.max_bp_diff / 2) - 1);
									}
								}
								else if ((state.flags & STATE_FLAG_RET_TYPE_MASK) == STATE_FLAG_RET_TYPE_FAR) {
									if (state.max_bp_diff >= 4) {
										set_function_uses_bp(new_func, (state.max_bp_diff / 2) - 2);
									}
								}
							}

							if (state.flags & STATE_FLAG_OWNS_BP) {
								set_function_owns_bp(new_func);
							}

							new_func->return_size = state.return_size;
							new_func->start = block->start;
							new_func->block_count = count_set_bits_in_bitset(state.included_blocks, block_count);
							new_func->blocks = malloc(sizeof(struct CodeBlock *) * new_func->block_count);
							if (!new_func->blocks) {
								free(stack_state.stack_size);
								free(state.included_blocks);
								free(available_blocks);
								return 1;
							}

							for (all_block_index = 0; all_block_index < block_count; all_block_index++) {
								if (get_bitset_value(state.included_blocks, all_block_index)) {
									new_func->blocks[new_blocks_index++] = blocks[all_block_index];
									set_bitset_value(available_blocks, all_block_index, 0);
								}
							}

							if ((error_code = insert_func(func_list, new_func))) {
								free(stack_state.stack_size);
								free(state.included_blocks);
								free(available_blocks);
								return error_code;
							}

							new_function_added = 1;
						}

						free(block_map);
						free(stack_state.stack_size);
					}

					free(state.included_blocks);
				}
			}
		}
	}
	while (new_function_added);

	free(available_blocks);
	return 0;
}
