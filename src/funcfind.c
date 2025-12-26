#include "funcfind.h"
#include "counter.h"
#include "packed.h"
#include "printd.h"

#include <assert.h>

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

static int evaluate_block(struct CodeBlock **blocks, unsigned int block_count, unsigned int block_index, packed_data_t *available_blocks, packed_data_t *included_blocks) {
	const struct CodeBlock *block = blocks[block_index];
	struct Reader reader;
	int error_code;

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
		if ((value0 & 0xF0) == 0x70 || (value0 & 0xFC) == 0xE0 || value0 == 0xEB) {
			const int value1 = read_next_byte(&reader);
			const int diff = (value1 >= 0x80)? value1 - 0x100 : value1;
			const char *jump_destination = block->start + reader.buffer_index + diff;
			const int target_block_index = find_block_index(blocks, block_count, block->start + reader.buffer_index + diff);
			if (target_block_index >= 0) {
				if (get_bitset_value(available_blocks, target_block_index)) {
					set_bitset_value(included_blocks, target_block_index, 1);
				}
				else {
					WARN_PRINT0("7X or EX. Block found, but already in use.\n");
					return 1;
				}
			}
			else {
				WARN_PRINT0("7X or EX. Unable to find a block matching the expected start position.\n");
				return 1;
			}
		}
		else if (value0 == 0xE8) {
			DEBUG_PRINT0("Opcode E8 found. For now, we do not allow nested calls. Skipping this function evaluation.\n");
			return 1;
		}
		else if (value0 == 0xE9) {
			const int diff = read_next_word(&reader);
			const int target_block_index = find_block_index(blocks, block_count, block->start + reader.buffer_index + diff);
			if (target_block_index >= 0) {
				if (get_bitset_value(available_blocks, target_block_index)) {
					set_bitset_value(included_blocks, target_block_index, 1);
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
					set_bitset_value(included_blocks, target_block_index, 1);
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
		else if (value0 == 0xFF) {
			const int value1 = read_next_byte(&reader);
			if ((value1 & 0x30) == 0x10 || (value1 & 0x30) == 0x20) {
				DEBUG_PRINT0("Opcode FF for call or jmp found. For now, we do not allow this. Skipping this function evaluation.\n");
				return 1;
			}
		}

		reader.buffer_index = next_instruction_index;
	}

	return 0;
}

static int find_all_blocks_in_function(struct CodeBlock **blocks, unsigned int block_count, packed_data_t *available_blocks, packed_data_t *included_blocks) {
	packed_data_t *evaluated_blocks = allocate_bitset(block_count);
	int block_index;

	if (!evaluated_blocks) {
		return 1;
	}

	while(!are_bitsets_equal(included_blocks, evaluated_blocks, block_count)) {
		for (block_index = 0; block_index < block_count; block_index++) {
			if (get_bitset_value(included_blocks, block_index) && !get_bitset_value(evaluated_blocks, block_index)) {
				int error_code;
				if ((error_code = evaluate_block(blocks, block_count, block_index, available_blocks, included_blocks))) {
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

int find_functions(struct CodeBlock **blocks, unsigned int block_count, struct FunctionList *func_list) {
	packed_data_t *available_blocks = allocate_bitset(block_count);
	int block_index;

	DEBUG_PRINT0("Finding functions\n");
	if (!available_blocks) {
		return 1;
	}

	for (block_index = 0; block_index < block_count; block_index++) {
		set_bitset_value(available_blocks, block_index, 1);
	}

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
				packed_data_t *included_blocks = allocate_bitset(block_count);
				set_bitset_value(included_blocks, block_index, 1);

				DEBUG_PRINT2(" Finding all blocks in function starting at +%x:%x\n", block->relative_cs, block->ip);
				if (find_all_blocks_in_function(blocks, block_count, available_blocks, included_blocks)) {
					set_bitset_value(available_blocks, block_index, 0);
				}
				else {
					struct Function *new_func = prepare_new_func(func_list);
					int all_block_index;
					int new_blocks_index = 0;
					int error_code;

					new_func->flags = 0;
					new_func->start = block->start;
					new_func->block_count = count_set_bits_in_bitset(included_blocks, block_count);
					new_func->blocks = malloc(sizeof(struct CodeBlock *) * new_func->block_count);

					for (all_block_index = 0; all_block_index < block_count; all_block_index++) {
						if (get_bitset_value(included_blocks, all_block_index)) {
							new_func->blocks[new_blocks_index++] = blocks[all_block_index];
							set_bitset_value(available_blocks, all_block_index, 0);
						}
					}

					if ((error_code = insert_func(func_list, new_func))) {
						return error_code;
					}
				}
			}
		}
	}

	free(available_blocks);
	return 0;
}
