#include "finder.h"
#include "register.h"
#include "gvwvmap.h"
#include "reader.h"
#include "stack.h"
#include "itable.h"
#include "printu.h"
#include "relocu.h"

static void read_block_instruction_address(
		struct Reader *reader,
		int value1) {
	if ((value1 & 0xC7) == 0x06 || (value1 & 0xC0) == 0x80) {
		read_next_word(reader);
	}
	else if ((value1 & 0xC0) == 0x40) {
		read_next_byte(reader);
	}
}

static int update_call_return_origin(
		struct CodeBlock *return_block,
		unsigned int call_return_origin_index,
		const struct Registers *regs,
		const struct GlobalVariableWordValueMap *var_values,
		int never_returns) {
	struct CodeBlockOrigin *call_return_origin = return_block->origin_list.sorted_origins[call_return_origin_index];

	if (never_returns) {
		mark_cborigin_as_never_reached(call_return_origin);
	}
	else {
		int error_code;
		struct Registers updated_regs;
		copy_registers(&updated_regs, regs);
		if (is_register_sp_defined_absolute(regs)) {
			set_register_sp(&updated_regs, where_register_sp_defined(regs), get_register_sp(regs) + 2);
		}

		if (is_cborigin_ready_to_be_evaluated(call_return_origin)) {
			if (changes_on_merging_registers(&call_return_origin->regs, &updated_regs) || changes_on_merging_gvwvmap(&call_return_origin->var_values, var_values)) {
				merge_registers(&call_return_origin->regs, &updated_regs);
				if ((error_code = merge_gvwvmap(&call_return_origin->var_values, var_values))) {
					return error_code;
				}

				invalidate_cblock_check(return_block);
			}
		}
		else {
			copy_registers(&call_return_origin->regs, &updated_regs);

			if ((error_code = copy_gvwvmap(&call_return_origin->var_values, var_values))) {
				return error_code;
			}

			set_cborigin_ready_to_be_evaluated(call_return_origin);
			invalidate_cblock_check(return_block);
		}
	}

	return 0;
}

static uint16_t *clone_checked_blocks(uint16_t *checked_blocks, const struct CodeBlockList *cblist) {
	const int checked_blocks_word_count = (cblist->block_count + 15) >> 4;
	int word_index;
	uint16_t *new_checked_blocks = malloc(checked_blocks_word_count * 2);
	if (new_checked_blocks) {
		for (word_index = 0; word_index < checked_blocks_word_count; word_index++) {
			new_checked_blocks[word_index] = checked_blocks[word_index];
		}
	}

	return new_checked_blocks;
}

static int update_call_origins(
		struct CodeBlock *block,
		struct CodeBlockList *code_block_list,
		uint16_t *checked_blocks,
		const struct Registers *regs,
		const struct GlobalVariableWordValueMap *var_values,
		int never_returns) {
	struct CodeBlockOriginList *origin_list = &block->origin_list;
	int index;
	int error_code;
	int cblock_index;
	cblock_index = index_of_code_block_with_start(code_block_list, block->start);
	if (cblock_index < 0 || checked_blocks[cblock_index >> 4] & (1 << (cblock_index & 0xF))) {
		return 0;
	}

	checked_blocks[cblock_index >> 4] |= 1 << (cblock_index & 0xF);

	for (index = 0; index < origin_list->origin_count; index++) {
		struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
		int origin_type = get_cborigin_type(origin);
		if (origin_type == CBORIGIN_TYPE_CONTINUE || origin_type == CBORIGIN_TYPE_CALL_RETURN) {
			struct CodeBlock *previous_block;
			if (cblock_index > 0 && (previous_block = code_block_list->sorted_blocks[cblock_index - 1])->end == block->start) {
				int word_index;
				uint16_t *new_checked_blocks = clone_checked_blocks(checked_blocks, code_block_list);
				if (!new_checked_blocks) {
					return 1;
				}

				error_code = update_call_origins(previous_block, code_block_list, new_checked_blocks, regs, var_values, never_returns);
				free(new_checked_blocks);
				if (error_code) {
					return error_code;
				}
			}
		}
		else if (origin_type == CBORIGIN_TYPE_JUMP) {
			const int jmp_opcode0 = ((int) *origin->instruction) & 0xFF;
			if (jmp_opcode0 == 0xE8) { /* CALL */
				int return_block_index = index_of_code_block_with_start(code_block_list, origin->instruction + 3);
				if (return_block_index >= 0) {
					struct CodeBlock *return_block = code_block_list->sorted_blocks[return_block_index];
					int call_return_origin_index = index_of_cborigin_of_type_call_return(&return_block->origin_list, 3);
					if (call_return_origin_index >= 0 && (error_code = update_call_return_origin(return_block, call_return_origin_index, regs, var_values, never_returns))) {
						return error_code;
					}
				}
			}
			else if (jmp_opcode0 == 0xE9) { /* JMP */
				int jumping_block_index = index_of_code_block_containing_position(code_block_list, origin->instruction);
				if (jumping_block_index >= 0) {
					int word_index;
					uint16_t *new_checked_blocks = clone_checked_blocks(checked_blocks, code_block_list);
					if (!new_checked_blocks) {
						return 1;
					}

					error_code = update_call_origins(code_block_list->sorted_blocks[jumping_block_index], code_block_list, new_checked_blocks, regs, var_values, never_returns);
					free(new_checked_blocks);
					if (error_code) {
						return error_code;
					}
				}
			}
			else if (jmp_opcode0 == 0xFF) {
				const int jmp_opcode1 = ((int) origin->instruction[1]) & 0xFF;
				if ((jmp_opcode1 & 0x38) == 0x10) {
					int return_block_index;
					int jmp_instruction_length = 2;
					if (jmp_opcode1 < 0xC0) {
						if (jmp_opcode1 >= 0x80 || (jmp_opcode1 & 0xC7) == 0x06) {
							jmp_instruction_length = 4;
						}
						else if ((jmp_opcode1 & 0xC0) == 0x40) {
							jmp_instruction_length = 3;
						}
					}

					return_block_index = index_of_code_block_with_start(code_block_list, origin->instruction + jmp_instruction_length);
					if (return_block_index >= 0) {
						struct CodeBlock *return_block = code_block_list->sorted_blocks[return_block_index];
						int call_return_origin_index = index_of_cborigin_of_type_call_return(&return_block->origin_list, jmp_instruction_length);
						if (call_return_origin_index >= 0 && (error_code = update_call_return_origin(return_block, call_return_origin_index, regs, var_values, never_returns))) {
							return error_code;
						}
					}
				}
				else if ((jmp_opcode1 & 0x38) == 0x20) {
					int jumping_block_index = index_of_code_block_containing_position(code_block_list, origin->instruction);
					if (jumping_block_index >= 0) {
						int word_index;
						uint16_t *new_checked_blocks = clone_checked_blocks(checked_blocks, code_block_list);
						if (!new_checked_blocks) {
							return 1;
						}

						error_code = update_call_origins(code_block_list->sorted_blocks[jumping_block_index], code_block_list, new_checked_blocks, regs, var_values, never_returns);
						free(new_checked_blocks);
						if (error_code) {
							return error_code;
						}
					}
				}
			}
		}
	}

	return 0;
}

static int add_call_return_origin_after_interruption(struct Reader *reader, struct Registers *regs, struct GlobalVariableWordValueMap *var_values, struct CodeBlock *block, struct CodeBlockList *code_block_list) {
	struct CodeBlock *return_block;
	struct CodeBlockOrigin *return_origin;
	int index;
	int error_code;
	block->end = block->start + reader->buffer_index;

	index = index_of_code_block_with_start(code_block_list, block->end);
	if (index >= 0) {
		return_block = code_block_list->sorted_blocks[index];
		if ((error_code = add_call_return_type_cborigin_in_block(return_block, 2))) {
			return error_code;
		}
	}
	else {
		return_block = prepare_new_code_block(code_block_list);
		if (!return_block) {
			return 1;
		}

		return_block->relative_cs = block->relative_cs;
		return_block->ip = block->ip + reader->buffer_index;
		return_block->start = block->end;
		return_block->end = block->end;
		return_block->flags = 0;
		initialize_cbolist(&return_block->origin_list);

		if ((error_code = add_call_return_type_cborigin_in_block(return_block, 2))) {
			return error_code;
		}

		if ((error_code = insert_sorted_code_block(code_block_list, return_block))) {
			return error_code;
		}
	}

	index = index_of_cborigin_of_type_call_return(&return_block->origin_list, 2);
	return_origin = return_block->origin_list.sorted_origins[index];
	copy_registers(&return_origin->regs, regs);
	initialize_gvwvmap(&return_origin->var_values);
	copy_gvwvmap(&return_origin->var_values, var_values);
	set_cborigin_ready_to_be_evaluated(return_origin);

	return 0;
}

#define SEGMENT_INDEX_UNDEFINED -1
#define SEGMENT_INDEX_SS 2
#define SEGMENT_INDEX_DS 3

static int read_block_instruction_internal(
		struct Reader *reader,
		struct Registers *regs,
		struct Stack *stack,
		struct GlobalVariableWordValueMap *var_values,
		struct InterruptionTable *int_table,
		const char *segment_start,
		const char **sorted_relocations,
		unsigned int relocation_count,
		void (*print_error)(const char *),
		struct CodeBlock *block,
		struct CodeBlockList *code_block_list,
		struct GlobalVariableList *global_variable_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list,
		int segment_index,
		const char *opcode_reference) {
	int error_code;
	const int value0 = read_next_byte(reader);
	if (value0 >= 0 && value0 < 0x40 && (value0 & 0x06) != 0x06) {
		if ((value0 & 0x04) == 0x00) {
			const int value1 = read_next_byte(reader);
			if ((value1 & 0xC0) == 0 && (value1 & 0x07) == 6) {
				int result_address = read_next_word(reader);
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_gvar_ref(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x40) {
				const int raw_value = read_next_byte(reader);
			}
			else if ((value1 & 0xC0) == 0x80) {
				const int raw_value = read_next_word(reader);
			}
			else if (value1 >= 0xC0) {
				if ((value0 & 0x38) == 0x30 && (((value1 >> 3) & 0x07) == (value1 & 0x07))) { /* XOR */
					if (value0 & 1) {
						set_word_register(regs, value1 & 0x07, opcode_reference, 0);
					}
					else {
						set_byte_register(regs, value1 & 0x07, opcode_reference, 0);
					}
				}
			}

			return 0;
		}
		else if ((value0 & 0x07) == 0x04) {
			read_next_byte(reader);
			return 0;
		}
		else {
			/* Assuming (value0 & 0x07) == 0x05 */
			read_next_word(reader);
			return 0;
		}
	}
	else if ((value0 & 0xE6) == 0x06 && value0 != 0x0F) {
		const unsigned int sindex = (value0 >> 3) & 3;
		if (value0 & 1) {
			if (stack_is_empty(stack)) {
				mark_segment_register_undefined(regs, sindex);
			}
			else if (top_is_defined_and_relative_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_segment_register_relative(regs, sindex, opcode_reference, stack_value);
			}
			else if (top_is_defined_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_segment_register(regs, sindex, opcode_reference, stack_value);
			}
			else {
				pop_from_stack(stack);
				mark_segment_register_undefined(regs, sindex);
			}
		}
		else {
			if (is_segment_register_defined_relative(regs, sindex)) {
				push_relative_in_stack(stack, get_segment_register(regs, sindex));
			}
			else if (is_segment_register_defined(regs, sindex)) {
				push_in_stack(stack, get_segment_register(regs, sindex));
			}
			else {
				push_undefined_in_stack(stack);
			}
		}

		return 0;
	}
	else if ((value0 & 0xE7) == 0x26) {
		return read_block_instruction_internal(reader, regs, stack, var_values, int_table, segment_start, sorted_relocations, relocation_count, print_error, block, code_block_list, global_variable_list, segment_start_list, reference_list, (value0 >> 3) & 0x03, opcode_reference);
	}
	else if ((value0 & 0xF0) == 0x40) {
		return 0;
	}
	else if ((value0 & 0xF0) == 0x50) {
		const unsigned int rindex = value0 & 7;
		if (value0 & 0x08) {
			if (stack_is_empty(stack)) {
				mark_word_register_undefined(regs, rindex);
			}
			else if (top_is_defined_and_relative_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_word_register_relative(regs, rindex, opcode_reference, stack_value);
			}
			else if (top_is_defined_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_word_register(regs, rindex, opcode_reference, stack_value);
			}
			else {
				pop_from_stack(stack);
				mark_word_register_undefined(regs, rindex);
			}
		}
		else {
			if (is_word_register_defined_relative(regs, rindex)) {
				push_relative_in_stack(stack, get_word_register(regs, rindex));
			}
			else if (is_word_register_defined(regs, rindex)) {
				push_in_stack(stack, get_word_register(regs, rindex));
			}
			else {
				push_undefined_in_stack(stack);
			}
		}

		return 0;
	}
	else if ((value0 & 0xF0) == 0x70 || (value0 & 0xFC) == 0xE0) {
		const int value1 = read_next_byte(reader);
		const int diff = (value1 >= 0x80)? value1 - 0x100 : value1;
		const char *jump_destination = block->start + reader->buffer_index + diff;
		int result = index_of_code_block_containing_position(code_block_list, jump_destination);
		struct CodeBlock *potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
		int potential_container_evaluated_at_least_once = potential_container && potential_container->start != potential_container->end;
		if (potential_container_evaluated_at_least_once && potential_container->end <= jump_destination) {
			potential_container = NULL;
			potential_container_evaluated_at_least_once = 0;
		}

		if (potential_container && potential_container->start == jump_destination) {
			if ((error_code = add_jump_type_cborigin_in_block(potential_container, opcode_reference, regs, var_values))) {
				return error_code;
			}
		}
		else {
			struct CodeBlock *new_block = prepare_new_code_block(code_block_list);
			if (!new_block) {
				return 1;
			}

			new_block->relative_cs = block->relative_cs;
			new_block->ip = block->ip + reader->buffer_index + diff;
			new_block->start = jump_destination;
			new_block->end = jump_destination;
			new_block->flags = 0;
			initialize_cbolist(&new_block->origin_list);
			if ((result = add_jump_type_cborigin_in_block(new_block, opcode_reference, regs, var_values))) {
				return result;
			}

			if (potential_container_evaluated_at_least_once) {
				if (potential_container->end > jump_destination) {
					potential_container->end = jump_destination;
				}

				invalidate_cblock_check(potential_container);
			}

			if ((result = insert_sorted_code_block(code_block_list, new_block))) {
				return result;
			}
		}

		block->end = block->start + reader->buffer_index;
		result = index_of_code_block_with_start(code_block_list, block->end);
		if (result >= 0) {
			if ((result = add_continue_type_cborigin_in_block(code_block_list->sorted_blocks[result], regs, var_values))) {
				return result;
			}
		}
		else {
			struct CodeBlock *new_block = prepare_new_code_block(code_block_list);
			if (!new_block) {
				return 1;
			}

			new_block->relative_cs = block->relative_cs;
			new_block->ip = block->ip + reader->buffer_index;
			new_block->start = block->end;
			new_block->end = block->end;
			new_block->flags = 0;
			initialize_cbolist(&new_block->origin_list);

			if ((result = add_continue_type_cborigin_in_block(new_block, regs, var_values))) {
				return result;
			}

			if ((result = insert_sorted_code_block(code_block_list, new_block))) {
				return result;
			}
		}

		return 0;
	}
	else if ((value0 & 0xFE) == 0x80) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0xC0) == 0 && (value1 & 0x07) == 6) {
			int result_address = read_next_word(reader);
			if (segment_index == SEGMENT_INDEX_UNDEFINED) {
				segment_index = SEGMENT_INDEX_DS;
			}

			if ((error_code = add_gvar_ref(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
				return error_code;
			}
		}
		else if ((value1 & 0xC0) == 0x40) {
			read_next_byte(reader);
		}
		else if ((value1 & 0xC0) == 0x80) {
			read_next_word(reader);
		}

		if (value0 & 1) {
			read_next_word(reader);
		}
		else {
			read_next_byte(reader);
		}

		return 0;
	}
	else if (value0 == 0x83) {
		const int value1 = read_next_byte(reader);
		read_block_instruction_address(reader, value1);
		read_next_byte(reader);
		return 0;
	}
	else if ((value0 & 0xFE) == 0x86 || (value0 & 0xFC) == 0x88) {
		const int value1 = read_next_byte(reader);

		if ((value1 & 0xC7) == 6) {
			int result_address = read_next_word(reader);
			if (segment_index == SEGMENT_INDEX_UNDEFINED) {
				segment_index = SEGMENT_INDEX_DS;
			}

			if ((error_code = add_gvar_ref(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
				return error_code;
			}

			if ((value0 & 0xFD) == 0x89 && is_segment_register_defined_relative(regs, segment_index)) {
				/* All this logic comes from add_global_variable_reference method. We should find a way to centralise this */
				unsigned int segment_value = get_segment_register(regs, segment_index);
				unsigned int relative_address = (segment_value * 16 + result_address) & 0xFFFF;
				const char *target = segment_start + relative_address;
				const int var_index = index_of_global_variable_with_start(global_variable_list, target);
				if (var_index >= 0) {
					const int reg_index = (value1 >> 3) & 7;
					if (value0 == 0x89) {
						if (is_word_register_defined_relative(regs, reg_index)) {
							if ((error_code = put_gvar_in_gvwvmap_relative(var_values, target, get_word_register(regs, reg_index)))) {
								return error_code;
							}
						}
						else if (is_word_register_defined(regs, reg_index)) {
							if ((error_code = put_gvar_in_gvwvmap(var_values, target, get_word_register(regs, reg_index)))) {
								return error_code;
							}
						}
						else {
							if ((error_code = remove_gvwvalue_with_start(var_values, target))) {
								return error_code;
							}
						}
					}
					else { /* value == 0x8B */
						const int var_index_in_map = index_of_gvar_in_gvwvmap_with_start(var_values, target);
						if (var_index_in_map >= 0) {
							uint16_t var_value = get_gvwvalue_at_index(var_values, var_index_in_map);
							if (is_gvwvalue_relative_at_index(var_values, var_index_in_map)) {
								set_word_register_relative(regs, reg_index, opcode_reference, var_value);
							}
							else {
								set_word_register(regs, reg_index, opcode_reference, var_value);
							}
						}
						else {
							mark_word_register_undefined(regs, reg_index);
						}
					}
				}
			}
		}
		else if ((value1 & 0xC0) == 0x80) {
			read_next_word(reader);
		}
		else if ((value1 & 0xC0) == 0x40) {
			read_next_byte(reader);
		}

		return 0;
	}
	else if ((value0 & 0xFD) == 0x8C) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x20) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			if ((value1 & 0xC7) == 6) {
				int result_address = read_next_word(reader);
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_gvar_ref(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, 1, opcode_reference))) {
					return error_code;
				}

				if (value0 == 0x8E) {
					const int reg_index = (value1 >> 3) & 3;
					if (is_segment_register_defined_relative(regs, segment_index)) {
						/* All this logic comes from add_global_variable_reference method. We should find a way to centralise this */
						unsigned int segment_value = get_segment_register(regs, segment_index);
						unsigned int relative_address = (segment_value * 16 + result_address) & 0xFFFF;
						const char *target = segment_start + relative_address;
						const int var_value_index = index_of_gvar_in_gvwvmap_with_start(var_values, target);
						if (var_value_index >= 0) {
							uint16_t var_value = get_gvwvalue_at_index(var_values, var_value_index);
							if (is_gvwvalue_relative_at_index(var_values, var_value_index)) {
								set_segment_register_relative(regs, reg_index, opcode_reference, var_value);
							}
							else {
								set_segment_register(regs, reg_index, opcode_reference, var_value);
							}
						}
						else {
							mark_segment_register_undefined(regs, reg_index);
						}
					}
					else {
						mark_segment_register_undefined(regs, reg_index);
					}
				}
			}
			else if ((value1 & 0xC0) == 0x40) {
				const int raw_value = read_next_byte(reader);
			}
			else if ((value1 & 0xC0) == 0x80) {
				const int raw_value = read_next_word(reader);
			}
			else if (value1 >= 0xC0) {
				const int rm = value1 & 0x07;
				const int index = (value1 >> 3) & 0x03;
				if ((value0 & 2) && is_word_register_defined(regs, rm)) {
					const uint16_t value = get_word_register(regs, rm);
					if (is_word_register_defined_relative(regs, rm)) {
						set_segment_register_relative(regs, index, opcode_reference, value);
					}
					else {
						set_segment_register(regs, index, opcode_reference, value);
					}
				}
				else if ((value0 & 2) == 0 && is_segment_register_defined(regs, index)) {
					const uint16_t value = get_segment_register(regs, index);
					if (is_segment_register_defined_relative(regs, index)) {
						set_word_register_relative(regs, rm, opcode_reference, value);
					}
					else {
						set_word_register(regs, rm, opcode_reference, value);
					}
				}
			}

			return 0;
		}
	}
	else if (value0 == 0x8D) {
		const int value1 = read_next_byte(reader);
		if (value1 >= 0xC0) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			read_block_instruction_address(reader, value1);
			return 0;
		}
	}
	else if (value0 == 0x8F) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x38 || value1 >= 0xC0) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			if (!stack_is_empty(stack)) {
				pop_from_stack(stack);
			}

			if ((value1 & 0xC7) == 0x06) {
				int result_address = read_next_word(reader);
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_gvar_ref(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
			}

			return 0;
		}
	}
	else if ((value0 & 0xF8) == 0x90) { /* xchg */
		return 0;
	}
	else if ((value0 & 0xFC) == 0xA0) {
		int offset;
		unsigned int current_segment_index;
		if ((value0 & 0xFE) == 0xA0) {
			if (value0 & 1) {
				set_register_ax_undefined(regs);
			}
			else {
				set_register_al_undefined(regs);
			}
		}

		offset = read_next_word(reader);
		current_segment_index = (segment_index >= 0)? segment_index : SEGMENT_INDEX_DS;
		if (value0 == 0xA3 && is_register_ax_defined(regs) && is_segment_register_defined_absolute(regs, current_segment_index)) {
			unsigned int addr = get_segment_register(regs, current_segment_index);
			addr = addr * 16 + offset;
			if ((offset & 1) == 0 && addr < 0x400) {
				const uint16_t value = get_register_ax(regs);
				const char *where = where_register_ax_defined(regs);
				if ((addr & 2) == 0) {
					set_interruption_table_offset(int_table, addr >> 2, where, value);
				}
				else if (is_register_ax_defined_relative(regs)) {
					set_interruption_table_segment_relative(int_table, addr >> 2, where, value);
				}
				else {
					set_interruption_table_segment(int_table, addr >> 2, where, value);
				}
			}
		}

		if (segment_index >= 0 && is_segment_register_defined_relative(regs, segment_index) || segment_index == SEGMENT_INDEX_UNDEFINED && is_register_ds_defined_relative(regs)) {
			unsigned int segment_value = (segment_index == SEGMENT_INDEX_UNDEFINED)? get_register_ds(regs) : get_segment_register(regs, segment_index);
			unsigned int relative_address = (segment_value * 16 + offset) & 0xFFFF;
			const char *target = segment_start + relative_address;
			const int var_index = index_of_global_variable_with_start(global_variable_list, target);
			struct GlobalVariable *var;
			if (var_index < 0) {
				var = prepare_new_global_variable(global_variable_list);
				var->start = target;
				var->end = target + 2;
				var->relative_address = relative_address;
				var->var_type = (value0 & 1)? GVAR_TYPE_WORD : GVAR_TYPE_BYTE;

				insert_sorted_global_variable(global_variable_list, var);
			}
			else {
				var = global_variable_list->sorted_variables[var_index];
			}

			if (index_of_reference_with_instruction(reference_list, opcode_reference) < 0) {
				struct Reference *new_ref = prepare_new_reference(reference_list);
				new_ref->instruction = opcode_reference;
				set_gvar_ref_from_instruction_address(new_ref, var);
				if (value0 & 2) {
					set_gvar_ref_write_access(new_ref);
				}
				else {
					set_gvar_ref_read_access(new_ref);
				}

				insert_sorted_reference(reference_list, new_ref);
			}
		}

		return 0;
	}
	else if ((value0 & 0xFC) == 0xA4) {
		return 0;
	}
	else if (value0 == 0xA8) {
		read_next_byte(reader);
		return 0;
	}
	else if (value0 == 0xA9) {
		read_next_word(reader);
		return 0;
	}
	else if ((value0 & 0xFE) == 0xAA) {
		return 0;
	}
	else if ((value0 & 0xFC) == 0xAC) {
		return 0;
	}
	else if ((value0 & 0xF0) == 0xB0) {
		if (value0 & 0x08) {
			const char *relocation_query = reader->buffer + reader->buffer_index;
			int word_value = read_next_word(reader);
			if (is_relocation_present_in_sorted_relocations(sorted_relocations, relocation_count, relocation_query)) {
				set_word_register_relative(regs, value0 & 0x07, opcode_reference, word_value);
			}
			else {
				set_word_register(regs, value0 & 0x07, opcode_reference, word_value);
			}
		}
		else {
			set_byte_register(regs, value0 & 0x07, opcode_reference, read_next_byte(reader));
		}
		return 0;
	}
	else if ((value0 & 0xFE) == 0xC2) {
		const unsigned int checked_blocks_word_count = (code_block_list->block_count + 15) >> 4;
		uint16_t *checked_blocks;
		unsigned int checked_blocks_word_index;

		if (value0 == 0xC2) {
			read_next_word(reader);
		}
		block->end = block->start + reader->buffer_index;

		checked_blocks = malloc(checked_blocks_word_count * 2);
		if (!checked_blocks) {
			return 1;
		}

		for (checked_blocks_word_index = 0; checked_blocks_word_index < checked_blocks_word_count; checked_blocks_word_index++) {
			checked_blocks[checked_blocks_word_index] = 0;
		}

		update_call_origins(block, code_block_list, checked_blocks, regs, var_values, 0);
		free(checked_blocks);
		return 0;
	}
	else if ((value0 & 0xFE) == 0xC4) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0xC0) == 0xC0) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			if ((value1 & 0xC7) == 0x06) {
				int result_address = read_next_word(reader);
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_far_pointer_gvar_ref(global_variable_list, reference_list, regs, segment_index, result_address, segment_start, opcode_reference))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
			}

			mark_word_register_undefined(regs, (value1 >> 3) & 7);
			if (value0 == 0xC4) {
				mark_register_es_undefined(regs);
			}
			else {
				/* Assuming value0 == 0xC5 */
				mark_register_ds_undefined(regs);
			}

			return 0;
		}
	}
	else if ((value0 & 0xFE) == 0xC6) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x38) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			int result_address;
			if (value1 == 0x06) {
				result_address = read_next_word(reader);
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_gvar_ref(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
			}

			if (value0 & 1) {
				int immediate_value = read_next_word(reader);
				if (value1 == 0x06 && is_segment_register_defined_relative(regs, segment_index)) {
					/* All this logic comes from add_global_variable_reference method. We should find a way to centralise this */
					unsigned int segment_value = get_segment_register(regs, segment_index);
					unsigned int relative_address = (segment_value * 16 + result_address) & 0xFFFF;
					const char *target = segment_start + relative_address;
					if (index_of_global_variable_with_start(global_variable_list, target) >= 0 &&
							(error_code = put_gvar_in_gvwvmap(var_values, target, immediate_value))) {
						return error_code;
					}
				}
			}
			else {
				read_next_byte(reader);
			}
			return 0;
		}
	}
	else if (value0 == 0xCD) {
		const int interruption_number = read_next_byte(reader);
		if (interruption_number == 0x20) {
			const unsigned int checked_blocks_word_count = (code_block_list->block_count + 15) >> 4;
			uint16_t *checked_blocks;
			unsigned int checked_blocks_word_index;

			block->end = block->start + reader->buffer_index;

			checked_blocks = malloc(checked_blocks_word_count * 2);
			if (!checked_blocks) {
				return 1;
			}

			for (checked_blocks_word_index = 0; checked_blocks_word_index < checked_blocks_word_count; checked_blocks_word_index++) {
				checked_blocks[checked_blocks_word_index] = 0;
			}

			update_call_origins(block, code_block_list, checked_blocks, regs, var_values, 1);
			free(checked_blocks);
		}
		else if (interruption_number == 0x21 && is_register_ah_defined(regs)) {
			const unsigned int ah_value = get_register_ah(regs);
			if (ah_value == 0x09 && is_register_ds_defined_relative(regs) && is_register_dx_defined(regs)) {
				unsigned int relative_address = (get_register_ds(regs) * 16 + get_register_dx(regs)) & 0xFFFF;
				const char *target = segment_start + relative_address;
				struct GlobalVariable *var;
				const char *instruction;
				int index = index_of_global_variable_with_start(global_variable_list, target);
				if (index < 0) {
					var = prepare_new_global_variable(global_variable_list);
					var->start = target;
					var->end = target;
					var->relative_address = relative_address;
					var->var_type = GVAR_TYPE_DOLLAR_TERMINATED_STRING;
					insert_sorted_global_variable(global_variable_list, var);
				}
				else {
					var = global_variable_list->sorted_variables[index];
				}
				/* What should we do if the variable is present, but its type does not match? Not sure. */

				instruction = where_register_dx_defined(regs);
				if ((((unsigned int) *instruction) & 0xFF) == 0xBA) {
					if (index_of_reference_with_instruction(reference_list, instruction) < 0) {
						struct Reference *new_ref = prepare_new_reference(reference_list);
						new_ref->instruction = instruction;
						set_gvar_ref_from_instruction_immediate_value(new_ref, var);
						insert_sorted_reference(reference_list, new_ref);
					}
				}

				if ((error_code = add_call_return_origin_after_interruption(reader, regs, var_values, block, code_block_list))) {
					return error_code;
				}
			}
			else if (ah_value == 0x25 && is_register_ds_defined_relative(regs) && is_register_dx_defined_absolute(regs)) { /* Set interruption vector. DS:DX point to the handler code */
				uint16_t target_relative_cs = get_register_ds(regs);
				uint16_t target_ip = get_register_dx(regs);
				const char *jump_destination;
				int result;
				struct CodeBlock *potential_container;
				struct CodeBlock *target_block;
				const char *instruction;
				unsigned int addr = target_relative_cs;
				addr = (addr * 16 + target_ip) & 0xFFFFF;

				jump_destination = segment_start + addr;
				result = index_of_code_block_containing_position(code_block_list, jump_destination);
				potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
				if (potential_container && potential_container->start == jump_destination) {
					target_block = potential_container;
				}
				else {
					target_block = prepare_new_code_block(code_block_list);
					if (!target_block) {
						return 1;
					}

					target_block->relative_cs = target_relative_cs;
					target_block->ip = target_ip;
					target_block->start = jump_destination;
					target_block->end = jump_destination;
					target_block->flags = 0;
					initialize_cbolist(&target_block->origin_list);
					if ((result = add_interruption_type_cborigin_in_block(target_block, regs, var_values))) {
						return result;
					}

					if ((result = insert_sorted_code_block(code_block_list, target_block))) {
						return result;
					}

					if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
						potential_container->end = jump_destination;
						invalidate_cblock_check(potential_container);
					}
				}

				instruction = where_register_dx_defined(regs);
				if ((((unsigned int) *instruction) & 0xFF) == 0xBA) {
					if (index_of_reference_with_instruction(reference_list, instruction) < 0) {
						struct Reference *new_ref = prepare_new_reference(reference_list);
						new_ref->instruction = instruction;
						set_cblock_ref_from_instruction_immediate_value(new_ref, target_block);
						insert_sorted_reference(reference_list, new_ref);
					}
				}

				if ((error_code = add_call_return_origin_after_interruption(reader, regs, var_values, block, code_block_list))) {
					return error_code;
				}
			}
			else if (ah_value == 0x30) { /* Get DOS version number */
				mark_register_ax_undefined(regs);
				mark_register_cx_undefined(regs);
				mark_register_bx_undefined(regs);

				if ((error_code = add_call_return_origin_after_interruption(reader, regs, var_values, block, code_block_list))) {
					return error_code;
				}
			}
			else if (ah_value == 0x40) { /* Write to file using handle */
				unsigned int length;
				if (is_register_ds_defined_relative(regs) && is_register_dx_defined_absolute(regs) && is_register_cx_defined_absolute(regs) && (length = get_register_cx(regs)) > 0) {
					int error_code;
					unsigned int segment_value = get_register_ds(regs);
					unsigned int relative_address = (segment_value * 16 + get_register_dx(regs)) & 0xFFFF;
					const char *target = segment_start + relative_address;
					if (index_of_global_variable_with_start(global_variable_list, target) < 0) {
						struct GlobalVariable *var = prepare_new_global_variable(global_variable_list);
						var->start = target;
						var->relative_address = relative_address;
						var->end = target + length;
						var->var_type = GVAR_TYPE_STRING;

						if ((error_code = insert_sorted_global_variable(global_variable_list, var))) {
							return error_code;
						}
					}

					if (segment_value && segment_value != 0xFFF0) {
						const char *target_segment_start = segment_start + segment_value * 16;
						if (!contains_segment_start(segment_start_list, target_segment_start)) {
							if ((error_code = insert_segment_start(segment_start_list, target_segment_start))) {
								return error_code;
							}
						}
					}
				}
				mark_register_ax_undefined(regs);

				if ((error_code = add_call_return_origin_after_interruption(reader, regs, var_values, block, code_block_list))) {
					return error_code;
				}
			}
			else if (ah_value == 0x4C) {
				const unsigned int checked_blocks_word_count = (code_block_list->block_count + 15) >> 4;
				uint16_t *checked_blocks;
				unsigned int checked_blocks_word_index;

				block->end = block->start + reader->buffer_index;

				checked_blocks = malloc(checked_blocks_word_count * 2);
				if (!checked_blocks) {
					return 1;
				}

				for (checked_blocks_word_index = 0; checked_blocks_word_index < checked_blocks_word_count; checked_blocks_word_index++) {
					checked_blocks[checked_blocks_word_index] = 0;
				}

				update_call_origins(block, code_block_list, checked_blocks, regs, var_values, 1);
				free(checked_blocks);
			}
		}
		return 0;
	}
	else if ((value0 & 0xFC) == 0xD0) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0x38) == 0x30) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			read_block_instruction_address(reader, value1);
			return 0;
		}
	}
	else if ((value0 & 0xFE) == 0xE8) {
		const char *jump_destination;
		int result;
		struct CodeBlock *potential_container;
		int diff = read_next_word(reader);
		if (block->ip + reader->buffer_index + diff >= 0x10000) {
			diff -= 0x10000;
		}

		jump_destination = block->start + reader->buffer_index + diff;
		result = index_of_code_block_containing_position(code_block_list, jump_destination);
		potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
		if (!potential_container || potential_container->start != jump_destination) {
			struct CodeBlock *new_block = prepare_new_code_block(code_block_list);
			if (!new_block) {
				return 1;
			}

			new_block->relative_cs = block->relative_cs;
			new_block->ip = block->ip + reader->buffer_index + diff;
			new_block->start = jump_destination;
			new_block->end = jump_destination;
			new_block->flags = 0;
			initialize_cbolist(&new_block->origin_list);
			if ((result = add_jump_type_cborigin_in_block(new_block, opcode_reference, regs, var_values))) {
				return result;
			}

			if ((result = insert_sorted_code_block(code_block_list, new_block))) {
				return result;
			}

			if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
				potential_container->end = jump_destination;
				invalidate_cblock_check(potential_container);
			}
		}

		block->end = block->start + reader->buffer_index;
		if (value0 == 0xE8) {
			result = index_of_code_block_with_start(code_block_list, block->end);
			if (result < 0) {
				struct CodeBlock *next_block = prepare_new_code_block(code_block_list);
				if (!next_block) {
					return 1;
				}

				next_block->relative_cs = block->relative_cs;
				next_block->ip = block->ip + reader->buffer_index;
				next_block->start = block->end;
				next_block->end = block->end;
				next_block->flags = 0;
				initialize_cbolist(&next_block->origin_list);
				if ((result = add_call_return_type_cborigin_in_block(next_block, 3))) {
					return result;
				}

				if ((result = insert_sorted_code_block(code_block_list, next_block))) {
					return result;
				}
			}
			else {
				struct CodeBlock *next_block = code_block_list->sorted_blocks[result];
				struct CodeBlockOriginList *origin_list = &next_block->origin_list;
				if (index_of_cborigin_of_type_call_return(origin_list, 3) < 0 && (result = add_call_return_type_cborigin_in_block(next_block, 3))) {
					return result;
				}
			}
		}

		return 0;
	}
	else if (value0 == 0xEA) {
		read_next_word(reader);
		read_next_word(reader);
		block->end = block->start + reader->buffer_index;
		return 0;
	}
	else if (value0 == 0xEB) {
		const int value1 = read_next_byte(reader);
		const int diff = (value1 >= 0x80)? value1 - 0x100 : value1;
		const char *jump_destination = block->start + reader->buffer_index + diff;
		int result = index_of_code_block_containing_position(code_block_list, jump_destination);
		struct CodeBlock *potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
		if (!potential_container || potential_container->start != jump_destination) {
			struct CodeBlock *new_block = prepare_new_code_block(code_block_list);
			if (!new_block) {
				return 1;
			}

			new_block->relative_cs = block->relative_cs;
			new_block->ip = block->ip + reader->buffer_index + diff;
			new_block->start = jump_destination;
			new_block->end = jump_destination;
			new_block->flags = 0;
			initialize_cbolist(&new_block->origin_list);
			if ((result = add_jump_type_cborigin_in_block(new_block, opcode_reference, regs, var_values))) {
				return result;
			}

			if ((result = insert_sorted_code_block(code_block_list, new_block))) {
				return result;
			}

			if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
				potential_container->end = jump_destination;
				invalidate_cblock_check(potential_container);
			}
		}

		return 0;
	}
	else if (value0 == 0xF2) {
		return 0;
	}
	else if (value0 == 0xF3) {
		return 0;
	}
	else if ((value0 & 0xFE) == 0xF6) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0x38) == 0x08) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			read_block_instruction_address(reader, value1);
			if ((value1 & 0x38) == 0) {
				if (value0 & 1) {
					read_next_word(reader);
				}
				else {
					read_next_byte(reader);
				}
			}
			return 0;
		}
	}
	else if (value0 == 0xFB) { /* sti */
		int i;
		for (i = 0; i < 256; i++) {
			if (is_interruption_defined_and_relative_in_table(int_table, i)) {
				uint16_t target_relative_cs = get_interruption_table_relative_segment(int_table, i);
				uint16_t target_ip = get_interruption_table_offset(int_table, i);
				const char *jump_destination;
				int result;
				struct CodeBlock *potential_container;
				struct CodeBlock *target_block;
				const char *where_offset;
				unsigned int addr = target_relative_cs;
				addr = (addr * 16 + target_ip) & 0xFFFFF;

				jump_destination = segment_start + addr;
				result = index_of_code_block_containing_position(code_block_list, jump_destination);
				potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
				if (potential_container && potential_container->start == jump_destination) {
					target_block = potential_container;
				}
				else {
					struct Registers int_regs;
					if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
						potential_container->end = jump_destination;
					}

					target_block = prepare_new_code_block(code_block_list);
					if (!target_block) {
						return 1;
					}

					target_block->relative_cs = target_relative_cs;
					target_block->ip = target_ip;
					target_block->start = jump_destination;
					target_block->end = jump_destination;
					target_block->flags = 0;
					initialize_cbolist(&target_block->origin_list);

					make_all_registers_undefined(&int_regs);
					set_register_cs_relative(&int_regs, where_interruption_segment_defined_in_table(int_table, i), target_relative_cs);
					if ((result = add_interruption_type_cborigin_in_block(target_block, &int_regs, var_values))) {
						return result;
					}

					if ((result = insert_sorted_code_block(code_block_list, target_block))) {
						return result;
					}
				}

				where_offset = where_interruption_offset_defined_in_table(int_table, i);
				if (where_offset && where_offset != REGISTER_DEFINED_OUTSIDE && (((unsigned int) *where_offset) & 0xF8) == 0xB8) {
					if (index_of_reference_with_instruction(reference_list, where_offset) < 0) {
						struct Reference *new_ref = prepare_new_reference(reference_list);
						new_ref->instruction = where_offset;
						set_cblock_ref_from_instruction_immediate_value(new_ref, target_block);
						insert_sorted_reference(reference_list, new_ref);
					}
				}
			}
		}

		return 0;
	}
	else if ((value0 & 0xFC) == 0xF8 || (value0 & 0xFE) == 0xFC) {
		return 0;
	}
	else if (value0 == 0xFE) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x30) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			read_block_instruction_address(reader, value1);
			return 0;
		}
	}
	else if (value0 == 0xFF) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0x38) == 0x38 || (value1 & 0xF8) == 0xD8 || (value1 & 0xF8) == 0xE8) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			int instruction_length = 2;
			if ((value1 & 0xC7) == 0x06) {
				int result_address = read_next_word(reader);
				instruction_length = 4;
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_gvar_ref(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
				instruction_length = 4;
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
				instruction_length = 3;
			}

			if ((value1 & 0x30) == 0x10) {
				int result;
				block->end = block->start + reader->buffer_index;
				result = index_of_code_block_with_start(code_block_list, block->end);
				if (result < 0) {
					struct CodeBlock *next_block = prepare_new_code_block(code_block_list);
					if (!next_block) {
						return 1;
					}

					next_block->relative_cs = block->relative_cs;
					next_block->ip = block->ip + reader->buffer_index;
					next_block->start = block->end;
					next_block->end = block->end;
					next_block->flags = 0;
					initialize_cbolist(&next_block->origin_list);
					if ((result = add_call_return_type_cborigin_in_block(next_block, instruction_length))) {
						return result;
					}

					if ((result = insert_sorted_code_block(code_block_list, next_block))) {
						return result;
					}
				}
				else {
					struct CodeBlock *next_block = code_block_list->sorted_blocks[result];
					struct CodeBlockOriginList *origin_list = &next_block->origin_list;
					if (index_of_cborigin_of_type_call_return(origin_list, instruction_length) < 0 && (result = add_call_return_type_cborigin_in_block(next_block, instruction_length))) {
						return result;
					}
				}
			}
			else if ((value1 & 0x30) == 0x20) {
				block->end = block->start + reader->buffer_index;
			}
			return 0;
		}
	}
	else {
		const int this_block_index = index_of_code_block_with_start(code_block_list, block->start);
		const char *new_end = reader->buffer + reader->buffer_size;
		if (this_block_index + 1 < code_block_list->block_count) {
			const char *next_start = code_block_list->sorted_blocks[this_block_index + 1]->start;
			if (next_start < new_end) {
				new_end = next_start;
			}
		}

		block->end = new_end;
		return 0;
	}
}

static int read_block_instruction(
		struct Reader *reader,
		struct Registers *regs,
		struct Stack *stack,
		struct GlobalVariableWordValueMap *var_values,
		struct InterruptionTable *int_table,
		const char *segment_start,
		const char **sorted_relocations,
		unsigned int relocation_count,
		void (*print_error)(const char *),
		struct CodeBlock *block,
		struct CodeBlockList *code_block_list,
		struct GlobalVariableList *global_variable_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list) {
	const char *instruction = reader->buffer + reader->buffer_index;
	return read_block_instruction_internal(reader, regs, stack, var_values, int_table, segment_start, sorted_relocations, relocation_count, print_error, block, code_block_list, global_variable_list, segment_start_list, reference_list, SEGMENT_INDEX_UNDEFINED, instruction);
}

static int read_block(
	struct Registers *regs,
	struct GlobalVariableWordValueMap *var_values,
	const char *segment_start,
	const char **sorted_relocations,
	unsigned int relocation_count,
	void (*print_error)(const char *),
	struct CodeBlock *block,
	unsigned int block_max_size,
	struct CodeBlockList *code_block_list,
	struct GlobalVariableList *global_variable_list,
	struct SegmentStartList *segment_start_list,
	struct ReferenceList *reference_list) {
	struct Reader reader;
	struct Stack stack;
	struct InterruptionTable int_table;
	int error_code;

	reader.buffer = block->start;
	reader.buffer_index = 0;
	reader.buffer_size = block_max_size;

	initialize_stack(&stack);
	make_all_interruption_table_undefined(&int_table);

	do {
		int index;
		if ((error_code = read_block_instruction(&reader, regs, &stack, var_values, &int_table, segment_start, sorted_relocations, relocation_count, print_error, block, code_block_list, global_variable_list, segment_start_list, reference_list))) {
			clear_stack(&stack);
			return error_code;
		}

		index = index_of_code_block_with_start(code_block_list, block->start);
		if (index + 1 < code_block_list->block_count) {
			struct CodeBlock *next_block = code_block_list->sorted_blocks[index + 1];
			const char *next_start = next_block->start;
			if (block->start + reader.buffer_index == next_start) {
				struct CodeBlockOriginList *next_origin_list = &next_block->origin_list;
				int next_origin_index;
				block->end = next_start;
				next_origin_index = index_of_cborigin_of_type_continue(next_origin_list);
				if (next_origin_index >= 0) {
					struct CodeBlockOrigin *next_origin = next_origin_list->sorted_origins[next_origin_index];
					if (is_cborigin_ready_to_be_evaluated(next_origin)) {
						if (changes_on_merging_registers(&next_origin->regs, regs) || changes_on_merging_gvwvmap(&next_origin->var_values, var_values)) {
							merge_registers(&next_origin->regs, regs);
							if ((error_code = merge_gvwvmap(&next_origin->var_values, var_values))) {
								return error_code;
							}

							invalidate_cblock_check(next_block);
						}
					}
					else {
						copy_registers(&next_origin->regs, regs);

						if ((error_code = copy_gvwvmap(&next_origin->var_values, var_values))) {
							return error_code;
						}

						set_cborigin_ready_to_be_evaluated(next_origin);
						invalidate_cblock_check(next_block);
					}
				}
			}
			else if (block->start + reader.buffer_index >= next_start) {
				block->end = next_start;
			}
		}
	} while (block->start == block->end || block->start + reader.buffer_index < block->end);

	clear_stack(&stack);
	return 0;
}

int find_cblocks_and_gvars(
		struct SegmentReadResult *read_result,
		void (*print_error)(const char *),
		struct CodeBlockList *code_block_list,
		struct GlobalVariableList *global_variable_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list) {
	struct CodeBlockOrigin *origin;
	struct CodeBlock *first_block = prepare_new_code_block(code_block_list);
	int error_code;
	int any_evaluated;
	int any_not_ready;
	int evaluate_all;
	int variable_index;

	if (!first_block) {
		return 1;
	}

	first_block->ip = read_result->ip;
	first_block->relative_cs = read_result->relative_cs;
	first_block->start = read_result->buffer + (read_result->relative_cs * 16 + read_result->ip);
	first_block->end = first_block->start;
	first_block->flags = 0;
	initialize_cbolist(&first_block->origin_list);

	origin = prepare_new_cborigin(&first_block->origin_list);
	set_os_type_in_cborigin(origin);
	make_all_registers_undefined(&origin->regs);
	set_register_cs_relative(&origin->regs, REGISTER_DEFINED_OUTSIDE, read_result->relative_cs);
	if (ds_should_match_cs_at_segment_start(read_result)) {
		set_register_ds_relative(&origin->regs, REGISTER_DEFINED_OUTSIDE, read_result->relative_cs);
	}
	initialize_gvwvmap(&origin->var_values);

	if ((error_code = insert_cborigin(&first_block->origin_list, origin))) {
		return error_code;
	}

	if ((error_code = insert_sorted_code_block(code_block_list, first_block))) {
		return error_code;
	}

	evaluate_all = 0;
	do {
		int block_index;
		any_evaluated = 0;
		any_not_ready = 0;

		for (block_index = 0; block_index < code_block_list->block_count; block_index++) {
			struct CodeBlock *block = code_block_list->page_array[block_index / code_block_list->blocks_per_page] + (block_index % code_block_list->blocks_per_page);
			if (cblock_requires_evaluation(block)) {
				if (evaluate_all || cblock_ready_to_be_evaluated(block)) {
					struct Registers regs;
					unsigned int block_max_size;
					struct GlobalVariableWordValueMap var_values;

					any_evaluated = 1;
					mark_cblock_as_being_evaluated(block);

					block_max_size = read_result->size - (block->start - read_result->buffer);

					accumulate_registers_from_cbolist(&regs, &block->origin_list);

					initialize_gvwvmap(&var_values);
					accumulate_gvwvmap_from_cbolist(&var_values, &block->origin_list);

					if ((error_code = read_block(&regs, &var_values, read_result->buffer, read_result->sorted_relocations, read_result->relocation_count, print_error, block, block_max_size, code_block_list, global_variable_list, segment_start_list, reference_list))) {
						return error_code;
					}

					clear_gvwvmap(&var_values);
					mark_cblock_as_evaluated(block);
				}
				else {
					any_not_ready = 1;
				}
			}
		}

		if (!any_evaluated && any_not_ready) {
			evaluate_all = 1;
		}
	}
	while (any_evaluated || any_not_ready);

	for (variable_index = 0; variable_index < global_variable_list->variable_count; variable_index++) {
		struct GlobalVariable *variable = global_variable_list->sorted_variables[variable_index];
		if (variable->start == variable->end && variable->var_type == GVAR_TYPE_DOLLAR_TERMINATED_STRING) {
			const int index = index_of_code_block_containing_position(code_block_list, variable->start);
			if (index < 0 || code_block_list->sorted_blocks[index]->end <= variable->start) {
				const char *potential_end = read_result->buffer + read_result->size;
				const char *end;

				if (index + 1 < code_block_list->block_count) {
					potential_end = code_block_list->sorted_blocks[index + 1]->start;
				}

				for (end = variable->start; end < potential_end; end++) {
					if (*end == '$') {
						end++;
						break;
					}
				}

				variable->end = end;
			}
			else {
				/* TODO: Find a better solution */
				variable->end = code_block_list->sorted_blocks[index]->end;
			}
		}
	}
	return 0;
}
