#include "finder.h"
#include "register.h"
#include "gvwvmap.h"
#include "reader.h"
#include "stack.h"
#include "itable.h"
#include "printu.h"
#include "relocu.h"
#include "printd.h"

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

static int ensure_call_return_origin(
		struct CodeBlockList *cblock_list,
		struct CodeBlockOrigin *origin,
		const struct Registers *regs,
		const struct Stack *stack,
		const struct GlobalVariableWordValueMap *var_values,
		int is_returning_far,
		unsigned int instruction_length) {
	int jmp_block_index = index_of_cblock_containing_position(cblock_list, origin->instruction);
	struct CodeBlock *jmp_block = cblock_list->sorted_blocks[jmp_block_index];
	uint16_t expected_ip = jmp_block->ip + (origin->instruction + instruction_length - jmp_block->start);
	const int stack_top_matches_expected_ip = top_is_defined_in_stack(stack) && !top_is_defined_relative_in_stack(stack) && get_from_top(stack, 0) == expected_ip;
	const int origin_stack_top_matches_expected_ip = top_is_defined_in_stack(&origin->stack) && !top_is_defined_relative_in_stack(&origin->stack) && get_from_top(&origin->stack, 0) == expected_ip;
	const int stack_matches_expected_cs = is_defined_relative_in_stack_from_top(stack, 1) && get_from_top(stack, 1) == jmp_block->relative_cs;
	const int origin_stack_matches_expected_cs = is_defined_relative_in_stack_from_top(&origin->stack, 1) && get_from_top(&origin->stack, 1) == jmp_block->relative_cs;

	if ((stack_top_matches_expected_ip || !top_is_defined_in_stack(stack) && origin_stack_top_matches_expected_ip) &&
			(!is_returning_far || stack_matches_expected_cs || !is_defined_in_stack_from_top(stack, 1) && origin_stack_matches_expected_cs)) {
		int return_block_index = index_of_cblock_with_start(cblock_list, origin->instruction + instruction_length);
		int error_code;

		if (return_block_index < 0) {
			struct CodeBlock *return_block;
			struct CodeBlockOrigin *return_origin;

			return_block = prepare_new_cblock(cblock_list);
			return_block->start = origin->instruction + instruction_length;
			return_block->end = origin->instruction + instruction_length;
			return_block->flags = 0;
			initialize_cborigin_list(&return_block->origin_list);

			return_origin = prepare_new_cborigin(&return_block->origin_list);
			set_call_return_type_in_cborigin(return_origin, instruction_length);
			copy_registers(&return_origin->regs, regs);
			initialize_stack(&return_origin->stack);
			if ((error_code = copy_stack(&return_origin->stack, stack))) {
				return error_code;
			}

			initialize_gvwvmap(&return_origin->var_values);
			if ((error_code = copy_gvwvmap(&return_origin->var_values, var_values))) {
				return error_code;
			}

			if (is_register_sp_defined_absolute(&return_origin->regs)) {
				set_register_sp(&return_origin->regs, NULL, NULL, get_register_sp(regs) + (is_returning_far? 4 : 2));
			}

			pop_from_stack(&return_origin->stack);
			return_block->ip = expected_ip;

			if (is_returning_far) {
				pop_from_stack(&return_origin->stack);
			}
			return_block->relative_cs = jmp_block->relative_cs;

			set_cborigin_ready_to_be_evaluated(return_origin);
			if ((error_code = insert_cborigin(&return_block->origin_list, return_origin))) {
				return error_code;
			}

			if ((error_code = insert_cblock(cblock_list, return_block))) {
				return error_code;
			}
		}
		else {
			struct CodeBlock *return_block = cblock_list->sorted_blocks[return_block_index];
			int call_return_origin_index = index_of_cborigin_of_type_call_return(&return_block->origin_list, instruction_length);
			if (call_return_origin_index < 0) {
				struct CodeBlockOrigin *return_origin = prepare_new_cborigin(&return_block->origin_list);
				set_call_return_type_in_cborigin(return_origin, instruction_length);
				copy_registers(&return_origin->regs, regs);
				initialize_stack(&return_origin->stack);
				if ((error_code = copy_stack(&return_origin->stack, stack))) {
					return error_code;
				}

				initialize_gvwvmap(&return_origin->var_values);
				if ((error_code = copy_gvwvmap(&return_origin->var_values, var_values))) {
					return error_code;
				}

				if (is_register_sp_defined_absolute(&return_origin->regs)) {
					set_register_sp(&return_origin->regs, NULL, NULL, get_register_sp(regs) + (is_returning_far? 4 : 2));
				}

				return_block->ip = pop_from_stack(&return_origin->stack);
				return_block->relative_cs = (is_returning_far)? pop_from_stack(&return_origin->stack) : jmp_block->relative_cs;

				set_cborigin_ready_to_be_evaluated(return_origin);
				if ((error_code = insert_cborigin(&return_block->origin_list, return_origin))) {
					return error_code;
				}
			}
			else {
				struct CodeBlockOrigin *call_return_origin = return_block->origin_list.sorted_origins[call_return_origin_index];

				int error_code;
				struct Registers updated_regs;
				struct Stack updated_stack;
				copy_registers(&updated_regs, regs);
				if (is_register_sp_defined_absolute(regs)) {
					set_register_sp(&updated_regs, NULL, NULL, get_register_sp(regs) + (is_returning_far? 4 : 2));
				}

				initialize_stack(&updated_stack);
				if ((error_code = copy_stack(&updated_stack, stack))) {
					return error_code;
				}
				pop_from_stack(&updated_stack);
				if (is_returning_far) {
					pop_from_stack(&updated_stack);
				}

				if (is_cborigin_ready_to_be_evaluated(call_return_origin)) {
					if (changes_on_merging_registers(&call_return_origin->regs, &updated_regs) || changes_on_merging_stacks(&call_return_origin->stack, &updated_stack) || changes_on_merging_gvwvmap(&call_return_origin->var_values, var_values)) {
						merge_registers(&call_return_origin->regs, &updated_regs);
						merge_stacks(&call_return_origin->stack, &updated_stack);
						if ((error_code = merge_gvwvmap(&call_return_origin->var_values, var_values))) {
							return error_code;
						}

						invalidate_cblock_check(return_block);
					}
				}
				else {
					copy_registers(&call_return_origin->regs, &updated_regs);
					initialize_stack(&call_return_origin->stack);
					if ((error_code = copy_stack(&call_return_origin->stack, &updated_stack))) {
						return error_code;
					}

					initialize_gvwvmap(&call_return_origin->var_values);
					if ((error_code = copy_gvwvmap(&call_return_origin->var_values, var_values))) {
						return error_code;
					}

					set_cborigin_ready_to_be_evaluated(call_return_origin);
					invalidate_cblock_check(return_block);
				}
			}
		}
	}

	return 0;
}

#define CHECKED_BLOCK_BLOCKS_PER_PAGE 16

struct CheckedBlocks {
	struct CodeBlock **blocks;
	unsigned int count;
	unsigned int allocated_pages;
};

static int update_call_origins(
		struct CodeBlock *block,
		struct CodeBlockList *cblock_list,
		struct CheckedBlocks *checked_blocks,
		const struct Registers *regs,
		const struct Stack *stack,
		const struct GlobalVariableWordValueMap *var_values,
		int is_returning_far,
		unsigned int depth) {
	struct CodeBlockOriginList *origin_list = &block->origin_list;
	int index;
	int error_code;

	for (index = 0; index < checked_blocks->count; index++) {
		if (checked_blocks->blocks[index] == block) {
			DEBUG_INDENTED_PRINT2(depth, "Block at +%x:%x already checked, or not found.\n", block->relative_cs, block->ip);
			return 0;
		}
	}

	if (checked_blocks->count == checked_blocks->allocated_pages * CHECKED_BLOCK_BLOCKS_PER_PAGE) {
		checked_blocks->blocks = realloc(checked_blocks->blocks, (++checked_blocks->allocated_pages) * CHECKED_BLOCK_BLOCKS_PER_PAGE * sizeof(struct CodeBlock *));
		if (!checked_blocks->blocks) {
			return 1;
		}
	}
	checked_blocks->blocks[checked_blocks->count++] = block;

	DEBUG_INDENTED_PRINT3(depth, "Checking origins of block at +%x:%x. %d origin(s)\n", block->relative_cs, block->ip, origin_list->origin_count);
	for (index = 0; index < origin_list->origin_count; index++) {
		struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
		int origin_type = get_cborigin_type(origin);
		DEBUG_INDENTED_PRINT2(depth + 1, "Index %d -> origin type is %s", index, DEBUG_CBORIGIN_TYPE_NAME(origin_type));
		if (origin_type == CBORIGIN_TYPE_CONTINUE || origin_type == CBORIGIN_TYPE_CALL_RETURN) {
			struct CodeBlock *previous_block;
			int cblock_index = index_of_cblock_with_start(cblock_list, block->start);
			DEBUG_PRINT0(".\n");

			if (cblock_index > 0 && (previous_block = cblock_list->sorted_blocks[cblock_index - 1])->end == block->start) {
				unsigned int current_count = checked_blocks->count;
				if ((error_code = update_call_origins(previous_block, cblock_list, checked_blocks, regs, stack, var_values, is_returning_far, depth + 1))) {
					return error_code;
				}
				checked_blocks->count = current_count;
			}
		}
		else if (origin_type == CBORIGIN_TYPE_JUMP) {
			const int jmp_opcode0 = ((int) *origin->instruction) & 0xFF;
			int jumping_block_index = index_of_cblock_containing_position(cblock_list, origin->instruction);
#ifdef DEBUG
			if (jumping_block_index >= 0) {
				struct CodeBlock *origin_block = cblock_list->sorted_blocks[jumping_block_index];
				DEBUG_PRINT2(" from +%x:%x.\n", origin_block->relative_cs, origin_block->ip + (int) (origin->instruction - origin_block->start));
			}
			else {
				DEBUG_PRINT0(".\n");
			}
#endif /* DEBUG */

			if (jmp_opcode0 == 0xE8) { /* CALL */
				if ((error_code = ensure_call_return_origin(cblock_list, origin, regs, stack, var_values, is_returning_far, 3))) {
					return error_code;
				}
			}
			else if (jmp_opcode0 == 0xE9 || (jmp_opcode0 & 0xF0) == 0x70 || (jmp_opcode0 & 0xFC) == 0xE0 || jmp_opcode0 == 0xEB) { /* JMP and its conditionals */
				if (jumping_block_index >= 0) {
					unsigned int current_count = checked_blocks->count;
					if ((error_code = update_call_origins(cblock_list->sorted_blocks[jumping_block_index], cblock_list, checked_blocks, regs, stack, var_values, is_returning_far, depth + 1))) {
						return error_code;
					}

					checked_blocks->count = current_count;
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

					DEBUG_INDENTED_PRINT1(depth + 2, "Jump from opcode 0xFF 0x%x.\n", jmp_opcode1);
					if ((error_code = ensure_call_return_origin(cblock_list, origin, regs, stack, var_values, is_returning_far, jmp_instruction_length))) {
						return error_code;
					}
				}
				else if ((jmp_opcode1 & 0x38) == 0x20) {
					int jumping_block_index = index_of_cblock_containing_position(cblock_list, origin->instruction);
					if (jumping_block_index >= 0) {
						unsigned int current_count = checked_blocks->count;
						if ((error_code = update_call_origins(cblock_list->sorted_blocks[jumping_block_index], cblock_list, checked_blocks, regs, stack, var_values, is_returning_far, depth + 1))) {
							return error_code;
						}

						checked_blocks->count = current_count;
					}
				}
			}
		}
	}

	return 0;
}

static int add_call_return_origin_after_interruption(struct Reader *reader, struct Registers *regs, struct Stack *stack, struct GlobalVariableWordValueMap *var_values, struct CodeBlock *block, struct CodeBlockList *code_block_list) {
	struct CodeBlock *return_block;
	struct CodeBlockOrigin *return_origin;
	int index;
	int error_code;
	block->end = block->start + reader->buffer_index;

	index = index_of_cblock_with_start(code_block_list, block->end);
	if (index >= 0) {
		return_block = code_block_list->sorted_blocks[index];
		if ((error_code = add_call_return_type_cborigin_in_block(return_block, regs, stack, 2))) {
			return error_code;
		}
	}
	else {
		return_block = prepare_new_cblock(code_block_list);
		if (!return_block) {
			return 1;
		}

		return_block->relative_cs = block->relative_cs;
		return_block->ip = block->ip + reader->buffer_index;
		return_block->start = block->end;
		return_block->end = block->end;
		return_block->flags = 0;
		initialize_cborigin_list(&return_block->origin_list);

		if ((error_code = add_call_return_type_cborigin_in_block(return_block, regs, stack, 2))) {
			return error_code;
		}

		if ((error_code = insert_cblock(code_block_list, return_block))) {
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

static int register_jump_target_block(
		struct Reader *reader,
		struct Registers *regs,
		struct Stack *stack,
		struct GlobalVariableWordValueMap *var_values,
		struct CodeBlock *block,
		struct CodeBlockList *code_block_list,
		const char *jump_destination,
		const char *opcode_reference,
		int diff) {
	int result = index_of_cblock_containing_position(code_block_list, jump_destination);
	struct CodeBlock *potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
	int potential_container_evaluated_at_least_once = potential_container && potential_container->start != potential_container->end;

	if (potential_container_evaluated_at_least_once && potential_container->end <= jump_destination) {
		potential_container = NULL;
		potential_container_evaluated_at_least_once = 0;
	}

	if (potential_container && potential_container->start == jump_destination) {
		if ((result = add_jump_type_cborigin_in_block(potential_container, opcode_reference, regs, stack, var_values))) {
			return result;
		}
	}
	else {
		struct CodeBlock *new_block = prepare_new_cblock(code_block_list);
		if (!new_block) {
			return 1;
		}

		new_block->relative_cs = block->relative_cs;
		new_block->ip = block->ip + reader->buffer_index + diff;
		new_block->start = jump_destination;
		new_block->end = jump_destination;
		new_block->flags = 0;
		initialize_cborigin_list(&new_block->origin_list);
		if ((result = add_jump_type_cborigin_in_block(new_block, opcode_reference, regs, stack, var_values))) {
			return result;
		}

		if (potential_container_evaluated_at_least_once) {
			if (potential_container->end > jump_destination) {
				potential_container->end = jump_destination;
			}

			invalidate_cblock_check(potential_container);
		}

		if ((result = insert_cblock(code_block_list, new_block))) {
			return result;
		}
	}

	return 0;
}

static int register_next_block(
		struct Reader *reader,
		struct Registers *regs,
		struct Stack *stack,
		struct GlobalVariableWordValueMap *var_values,
		struct CodeBlock *block,
		struct CodeBlockList *code_block_list) {
	int result = index_of_cblock_with_start(code_block_list, block->end);
	if (result >= 0) {
		return add_continue_type_cborigin_in_block(code_block_list->sorted_blocks[result], regs, stack, var_values);
	}
	else {
		struct CodeBlock *new_block = prepare_new_cblock(code_block_list);
		if (!new_block) {
			return 1;
		}

		new_block->relative_cs = block->relative_cs;
		new_block->ip = block->ip + reader->buffer_index;
		new_block->start = block->end;
		new_block->end = block->end;
		new_block->flags = 0;
		initialize_cborigin_list(&new_block->origin_list);

		if ((result = add_continue_type_cborigin_in_block(new_block, regs, stack, var_values))) {
			return result;
		}

		return insert_cblock(code_block_list, new_block);
	}
}

int update_int2140_message_references(
		const struct Registers *regs,
		const char *segment_start,
		struct CodeBlock *block,
		struct CodeBlockList *code_block_list,
		struct GlobalVariableList *gvar_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *ref_list,
		struct CheckedBlocks *checked_blocks,
		const int cx_defined,
		const int cx_relative,
		const uint16_t cx_value,
		const int dx_defined,
		const int dx_relative,
		const uint16_t dx_value,
		const char *dx_value_origin,
		const int ds_defined,
		const int ds_relative,
		const uint16_t ds_value,
		unsigned int depth) {
	uint16_t length;
	int index;

	for (index = 0; index < checked_blocks->count; index++) {
		if (checked_blocks->blocks[index] == block) {
			DEBUG_INDENTED_PRINT2(depth, "Block at +%x:%x already checked, or not found.\n", block->relative_cs, block->ip);
			return 0;
		}
	}

	if (checked_blocks->count == checked_blocks->allocated_pages * CHECKED_BLOCK_BLOCKS_PER_PAGE) {
		checked_blocks->blocks = realloc(checked_blocks->blocks, (++checked_blocks->allocated_pages) * CHECKED_BLOCK_BLOCKS_PER_PAGE * sizeof(struct CodeBlock *));
		if (!checked_blocks->blocks) {
			return 1;
		}
	}
	checked_blocks->blocks[checked_blocks->count++] = block;

	if (ds_defined && dx_defined && cx_defined) {
		if (ds_relative && !dx_relative && !cx_relative && (length = cx_value) > 0) {
			int error_code;
			unsigned int segment_value = get_register_ds(regs);
			unsigned int relative_address = (segment_value * 16 + get_register_dx(regs)) & 0xFFFF;
			const char *target = segment_start + relative_address;
			struct GlobalVariable *var;

			if ((index = index_of_gvar_with_start(gvar_list, target)) < 0) {
				var = prepare_new_gvar(gvar_list);
				var->start = target;
				var->relative_address = relative_address;
				var->end = target + length;
				var->var_type = GVAR_TYPE_STRING;

				if ((error_code = insert_gvar(gvar_list, var))) {
					return error_code;
				}
			}
			else {
				var = gvar_list->sorted_variables[index];
			}

			if (dx_value_origin && index_of_ref_with_instruction(ref_list, dx_value_origin) < 0) {
				struct Reference *new_ref = prepare_new_ref(ref_list);
				DEBUG_INDENTED_PRINT1(depth, "DX value origin at %x. Registering reference.\n", (int) (dx_value_origin - segment_start));

				new_ref->instruction = dx_value_origin;
				set_gvar_ref_from_instruction_immediate_value(new_ref, var);
				if ((error_code = insert_ref(ref_list, new_ref))) {
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
	}
	else if ((ds_defined || is_register_ds_merged(regs)) && (dx_defined || is_register_dx_merged(regs)) && (cx_defined || is_register_cx_merged(regs))) {
		struct CodeBlockOriginList *origin_list = &block->origin_list;
		const unsigned int origin_count = origin_list->origin_count;
		int index;

		for (index = 0; index < origin_count; index++) {
			struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
			const int origin_block_index = index_of_cblock_containing_position(code_block_list, origin->instruction);
			DEBUG_INDENTED_PRINT1(depth, "Checking origin at %d. ", index);

			if (origin_block_index >= 0 && get_cborigin_type(origin) == CBORIGIN_TYPE_JUMP) {
				struct CodeBlock *origin_block = code_block_list->sorted_blocks[origin_block_index];
				const int new_ds_defined = ds_defined || is_register_ds_defined(&origin->regs);
				const int new_ds_relative = ds_defined? ds_relative : is_register_ds_defined_relative(&origin->regs);
				const uint16_t new_ds_value = ds_defined? ds_value : get_register_ds(&origin->regs);

				const int new_dx_defined = dx_defined || is_register_dx_defined(&origin->regs);
				const int new_dx_relative = dx_defined? dx_relative : is_register_dx_defined_relative(&origin->regs);
				const uint16_t new_dx_value = dx_defined? dx_value : get_register_dx(&origin->regs);
				const char *new_dx_value_origin = dx_defined? dx_value_origin : get_register_dx_value_origin(&origin->regs);

				const int new_cx_defined = cx_defined || is_register_cx_defined(&origin->regs);
				const int new_cx_relative = cx_defined? ds_relative : is_register_cx_defined_relative(&origin->regs);
				const uint16_t new_cx_value = cx_defined? ds_value : get_register_cx(&origin->regs);

				int error_code;
				DEBUG_PRINT0("Type is JUMP.\n");

				if ((error_code = update_int2140_message_references(&origin->regs, segment_start, origin_block, code_block_list,
						gvar_list, segment_start_list, ref_list, checked_blocks,
						new_cx_defined, new_cx_relative, new_cx_value,
						new_dx_defined, new_dx_relative, new_dx_value, new_dx_value_origin,
						new_ds_defined, new_ds_relative, new_ds_value,
						depth + 1))) {
					return error_code;
				}
			}
			else {
				DEBUG_PRINT0("Type is not JUMP or origin block not found. Skipping.\n");
			}
		}
	}

	return 0;
}

#define SEGMENT_INDEX_UNDEFINED -1
#define SEGMENT_INDEX_ES 0
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
		struct GlobalVariableList *gvar_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *ref_list,
		int segment_index,
		const char *opcode_reference) {
	int error_code;
	const int value0 = read_next_byte(reader);
	if (value0 >= 0 && value0 < 0x40 && (value0 & 0x06) != 0x06) {
		if ((value0 & 0x04) == 0x00) {
			const int value1 = read_next_byte(reader);
			if ((value1 & 0xC0) == 0) {
				if ((value1 & 0x07) == 6) {
					int result_address = read_next_word(reader);
					DEBUG_PRINT0("\n");

					if (segment_index == SEGMENT_INDEX_UNDEFINED) {
						segment_index = SEGMENT_INDEX_DS;
					}

					if ((error_code = add_gvar_ref(gvar_list, segment_start_list, ref_list, regs, var_values, segment_index, result_address, segment_start, value0, opcode_reference, 0, 0, 0, 0, 0))) {
						return error_code;
					}
				}
				else {
					DEBUG_PRINT0("\n");
				}
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
				DEBUG_PRINT0("\n");
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
				DEBUG_PRINT0("\n");
			}
			else { /* value1 >= 0xC0 */
				DEBUG_PRINT0("\n");
				if ((value0 & 0x38) == 0x30 && (((value1 >> 3) & 0x07) == (value1 & 0x07))) { /* XOR */
					if (value0 & 1) {
						set_word_register(regs, value1 & 0x07, opcode_reference, opcode_reference, 0);
					}
					else {
						set_byte_register(regs, value1 & 0x07, opcode_reference, opcode_reference, 0);
					}
				}
			}

			return 0;
		}
		else if ((value0 & 0x07) == 0x04) {
			read_next_byte(reader);
			DEBUG_PRINT0("\n");
			return 0;
		}
		else {
			/* Assuming (value0 & 0x07) == 0x05 */
			read_next_word(reader);
			DEBUG_PRINT0("\n");
			return 0;
		}
	}
	else if ((value0 & 0xE6) == 0x06 && value0 != 0x0F) {
		const unsigned int sindex = (value0 >> 3) & 3;
		DEBUG_PRINT0("\n");

		if (value0 & 1) {
			if (stack_is_empty(stack)) {
				set_segment_register_undefined(regs, sindex, opcode_reference);
			}
			else if (top_is_defined_relative_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_segment_register_relative(regs, sindex, opcode_reference, opcode_reference, stack_value);
			}
			else if (top_is_defined_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_segment_register(regs, sindex, opcode_reference, opcode_reference, stack_value);
			}
			else {
				pop_from_stack(stack);
				set_segment_register_undefined(regs, sindex, opcode_reference);
			}

			if (is_register_sp_defined_relative(regs)) {
				set_register_sp_relative(regs, opcode_reference, NULL, get_register_sp(regs) + 2);
			}
			else if (is_register_sp_defined(regs)) {
				set_register_sp(regs, opcode_reference, NULL, get_register_sp(regs) + 2);
			}
			else if (is_register_sp_relative_from_bp(regs)) {
				set_register_sp_relative_from_bp(regs, opcode_reference, get_register_sp(regs) + 2);
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

			if (is_register_sp_defined_relative(regs)) {
				set_register_sp_relative(regs, opcode_reference, NULL, get_register_sp(regs) - 2);
			}
			else if (is_register_sp_defined(regs)) {
				set_register_sp(regs, opcode_reference, NULL, get_register_sp(regs) - 2);
			}
			else if (is_register_sp_relative_from_bp(regs)) {
				set_register_sp_relative_from_bp(regs, opcode_reference, get_register_sp(regs) - 2);
			}
		}

		return 0;
	}
	else if ((value0 & 0xE7) == 0x26) {
		return read_block_instruction_internal(reader, regs, stack, var_values, int_table, segment_start, sorted_relocations, relocation_count, print_error, block, code_block_list, gvar_list, segment_start_list, ref_list, (value0 >> 3) & 0x03, opcode_reference);
	}
	else if ((value0 & 0xF0) == 0x40) {
		DEBUG_PRINT0("\n");
		return 0;
	}
	else if ((value0 & 0xF0) == 0x50) {
		const unsigned int rindex = value0 & 7;
		DEBUG_PRINT0("\n");
		if (value0 & 0x08) {
			if (stack_is_empty(stack)) {
				set_word_register_undefined(regs, rindex, opcode_reference);
			}
			else if (top_is_defined_relative_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_word_register_relative(regs, rindex, opcode_reference, opcode_reference, stack_value);
			}
			else if (top_is_defined_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_word_register(regs, rindex, opcode_reference, opcode_reference, stack_value);
			}
			else {
				pop_from_stack(stack);
				set_word_register_undefined(regs, rindex, opcode_reference);
			}

			if (is_register_sp_defined_relative(regs)) {
				set_register_sp_relative(regs, opcode_reference, NULL, get_register_sp(regs) + 2);
			}
			else if (is_register_sp_defined(regs)) {
				set_register_sp(regs, opcode_reference, NULL, get_register_sp(regs) + 2);
			}
			else if (is_register_sp_relative_from_bp(regs)) {
				set_register_sp_relative_from_bp(regs, opcode_reference, get_register_sp(regs) + 2);
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

			if (is_register_sp_defined_relative(regs)) {
				set_register_sp_relative(regs, opcode_reference, NULL, get_register_sp(regs) - 2);
			}
			else if (is_register_sp_defined(regs)) {
				set_register_sp(regs, opcode_reference, NULL, get_register_sp(regs) - 2);
			}
			else if (is_register_sp_relative_from_bp(regs)) {
				set_register_sp_relative_from_bp(regs, opcode_reference, get_register_sp(regs) - 2);
			}
		}

		return 0;
	}
	else if ((value0 & 0xF0) == 0x70 || (value0 & 0xFC) == 0xE0) {
		const int value1 = read_next_byte(reader);
		const int diff = (value1 >= 0x80)? value1 - 0x100 : value1;
		const char *jump_destination = block->start + reader->buffer_index + diff;
		int result;
		struct CodeBlock *potential_container;
		int potential_container_evaluated_at_least_once;
		DEBUG_PRINT0("\n");

		block->end = block->start + reader->buffer_index;
		if (jump_destination <= block->end) {
			if ((result = register_jump_target_block(reader, regs, stack, var_values, block, code_block_list, jump_destination, opcode_reference, diff))) {
				return result;
			}

			return register_next_block(reader, regs, stack, var_values, block, code_block_list);
		}
		else {
			if ((result = register_next_block(reader, regs, stack, var_values, block, code_block_list))) {
				return result;
			}

			return register_jump_target_block(reader, regs, stack, var_values, block, code_block_list, jump_destination, opcode_reference, diff);
		}
	}
	else if ((value0 & 0xFE) == 0x80) {
		const int value1 = read_next_byte(reader);
		int imm_value;

		if ((value1 & 0xC7) == 6) {
			int result_address = read_next_word(reader);
			if (segment_index == SEGMENT_INDEX_UNDEFINED) {
				segment_index = SEGMENT_INDEX_DS;
			}

			if ((error_code = add_gvar_ref(gvar_list, segment_start_list, ref_list, regs, var_values, segment_index, result_address, segment_start, value0, opcode_reference, 0, 0, 0, 0, 0))) {
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
			imm_value = read_next_word(reader);
		}
		else {
			read_next_byte(reader);
		}

		DEBUG_PRINT0("\n");

		if ((value1 & 0xF8) == 0xE8) { /* sub reg,immValue */
			const int reg_index = value1 & 7;
			if (is_word_register_defined_relative(regs, reg_index)) {
				set_word_register_relative(regs, reg_index, opcode_reference, NULL, get_word_register(regs, reg_index) - imm_value);
			}
			else if (is_word_register_defined(regs, reg_index)) {
				set_word_register(regs, reg_index, opcode_reference, NULL, get_word_register(regs, reg_index) - imm_value);
			}
			else if (reg_index == 4 && is_register_sp_relative_from_bp(regs)) {
				set_register_sp_relative_from_bp(regs, opcode_reference, get_register_sp(regs) - imm_value);
			}
			else if (reg_index == 5 && is_register_sp_relative_from_bp(regs)) {
				set_register_sp_relative_from_bp(regs, opcode_reference, get_register_sp(regs) + imm_value);
			}

			if (reg_index == 4 && (imm_value & 1) == 0) {
				const int push_count = imm_value >> 1 & 0x7FFF;
				int push_index;
				for (push_index = 0; push_index < push_count; push_index++) {
					if ((error_code = push_undefined_in_stack(stack))) {
						return error_code;
					}
				}
			}
		}
		return 0;
	}
	else if (value0 == 0x83) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0xC7) == 0x06) {
			int result_address = read_next_word(reader);
			if (segment_index == SEGMENT_INDEX_UNDEFINED) {
				segment_index = SEGMENT_INDEX_DS;
			}

			if ((error_code = add_gvar_ref(gvar_list, segment_start_list, ref_list, regs, var_values, segment_index, result_address, segment_start, value0, opcode_reference, 0, 0, 0, 0, 0))) {
				return error_code;
			}
		}
		else if ((value1 & 0xC0) == 0x80) {
			read_next_word(reader);
		}
		else if ((value1 & 0xC0) == 0x40) {
			read_next_byte(reader);
		}
		read_next_byte(reader);
		DEBUG_PRINT0("\n");
		return 0;
	}
	else if ((value0 & 0xFE) == 0x86 || (value0 & 0xFC) == 0x88) {
		const int value1 = read_next_byte(reader);

		if ((value1 & 0xC0) == 0) {
			if ((value1 & 0xC7) == 6) {
				int result_address = read_next_word(reader);
				const int read_access = value0 == 0x8B;
				const int write_access = value0 == 0x89;
				const int write_value_defined = is_word_register_defined(regs, (value1 >> 3) & 7);
				const int write_value_defined_relative = is_word_register_defined_relative(regs, (value1 >> 3) & 7);
				const int write_value = get_word_register(regs, (value1 >> 3) & 7);
				DEBUG_PRINT0("\n");

				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_gvar_ref(gvar_list, segment_start_list, ref_list, regs, var_values, segment_index, result_address, segment_start, value0, opcode_reference, read_access, write_access, write_value_defined, write_value_defined_relative, write_value))) {
					return error_code;
				}

				if ((value0 & 0xFD) == 0x89 && is_segment_register_defined_relative(regs, segment_index)) {
					/* All this logic comes from add_global_variable_reference method. We should find a way to centralise this */
					unsigned int segment_value = get_segment_register(regs, segment_index);
					unsigned int relative_address = (segment_value * 16 + result_address) & 0xFFFF;
					const char *target = segment_start + relative_address;
					const int var_index = index_of_gvar_with_start(gvar_list, target);
					if (var_index >= 0) {
						const int reg_index = (value1 >> 3) & 7;
						if (value0 == 0x8B) {
							const int var_index_in_map = index_of_gvar_in_gvwvmap_with_start(var_values, target);
							if (var_index_in_map >= 0) {
								uint16_t var_value = get_gvwvalue_at_index(var_values, var_index_in_map);
								if (is_gvwvalue_defined_relative_at_index(var_values, var_index_in_map)) {
									set_word_register_relative(regs, reg_index, opcode_reference, opcode_reference, var_value);
								}
								else {
									set_word_register(regs, reg_index, opcode_reference, opcode_reference, var_value);
								}
							}
							else {
								set_word_register_undefined(regs, reg_index, opcode_reference);
							}
						}
					}
				}
			}
		}
		else if ((value1 & 0xC0) == 0x40) {
			read_next_byte(reader);
			DEBUG_PRINT0("\n");
		}
		else if ((value1 & 0xC0) == 0x80) {
			read_next_word(reader);
			DEBUG_PRINT0("\n");
		}
		else { /* value1 >= 0xC0 */
			DEBUG_PRINT0("\n");

			if ((value0 & 0xFD) == 0x89) {
				int source_register;
				int target_register;

				if (value0 == 0x89) {
					source_register = (value1 >> 3) & 7;
					target_register = value1 & 7;
				}
				else {
					source_register = value1 & 7;
					target_register = (value1 >> 3) & 7;
				}

				if (is_word_register_defined_relative(regs, source_register)) {
					set_word_register_relative(regs, target_register, opcode_reference, get_word_register_value_origin(regs, source_register), get_word_register(regs, source_register));
				}
				else if (is_word_register_defined(regs, source_register)) {
					set_word_register(regs, target_register, opcode_reference, get_word_register_value_origin(regs, source_register), get_word_register(regs, source_register));
				}
				else if (source_register == 4 && target_register == 5 || source_register == 5 && target_register == 4) {
					set_register_sp_relative_from_bp(regs, opcode_reference, 0);
				}
				else {
					set_word_register_undefined(regs, target_register, opcode_reference);
				}
			}
		}

		return 0;
	}
	else if ((value0 & 0xFD) == 0x8C) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x20) {
			DEBUG_PRINT0(" Unknown opcode\n");
			return 1;
		}
		else {
			if ((value1 & 0xC7) == 6) {
				int result_address = read_next_word(reader);
				const int read_access = value0 == 0x8E;
				const int write_access = value0 == 0x8C;
				const int write_value_defined = is_segment_register_defined(regs, (value1 >> 3) & 3);
				const int write_value_defined_relative = is_segment_register_defined_relative(regs, (value1 >> 3) & 3);
				const int write_value = get_segment_register(regs, (value1 >> 3) & 3);
				DEBUG_PRINT0("\n");

				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_gvar_ref(gvar_list, segment_start_list, ref_list, regs, var_values, segment_index, result_address, segment_start, 1, opcode_reference, read_access, write_access, write_value_defined, write_value_defined_relative, write_value))) {
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
							if (is_gvwvalue_defined_relative_at_index(var_values, var_value_index)) {
								set_segment_register_relative(regs, reg_index, opcode_reference, opcode_reference, var_value);
							}
							else {
								set_segment_register(regs, reg_index, opcode_reference, opcode_reference, var_value);
							}
						}
						else {
							set_segment_register_undefined(regs, reg_index, opcode_reference);
						}
					}
					else {
						set_segment_register_undefined(regs, reg_index, opcode_reference);
					}
				}
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
				DEBUG_PRINT0("\n");
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
				DEBUG_PRINT0("\n");
			}
			else if (value1 >= 0xC0) {
				const int rm = value1 & 0x07;
				const int index = (value1 >> 3) & 0x03;
				DEBUG_PRINT0("\n");

				if ((value0 & 2)) {
					if (is_word_register_defined(regs, rm)) {
						const char *value_origin = get_word_register_value_origin(regs, rm);
						const uint16_t value = get_word_register(regs, rm);
						if (is_word_register_defined_relative(regs, rm)) {
							set_segment_register_relative(regs, index, opcode_reference, value_origin, value);
						}
						else {
							set_segment_register(regs, index, opcode_reference, value_origin, value);
						}
					}
					else {
						set_segment_register_undefined(regs, index, opcode_reference);
					}
				}
				else {
					if (is_segment_register_defined(regs, index)) {
						const char *value_origin = get_segment_register_value_origin(regs, index);
						const uint16_t value = get_segment_register(regs, index);
						if (is_segment_register_defined_relative(regs, index)) {
							set_word_register_relative(regs, rm, opcode_reference, value_origin, value);
						}
						else {
							set_word_register(regs, rm, opcode_reference, value_origin, value);
						}
					}
					else {
						set_word_register_undefined(regs, rm, opcode_reference);
					}
				}
			}

			return 0;
		}
	}
	else if (value0 == 0x8D) {
		const int value1 = read_next_byte(reader);
		if (value1 >= 0xC0) {
			DEBUG_PRINT0(" Unknown opcode\n");
			return 1;
		}
		else {
			read_block_instruction_address(reader, value1);
			DEBUG_PRINT0("\n");

			set_word_register_undefined(regs, (value1 >> 3) & 7, opcode_reference);
			return 0;
		}
	}
	else if (value0 == 0x8F) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x38 || value1 >= 0xC0) {
			DEBUG_PRINT0(" Unknown opcode\n");
			return 1;
		}
		else {
			int top_defined = top_is_defined_in_stack(stack);
			int top_defined_relative = top_is_defined_relative_in_stack(stack);
			uint16_t stack_top = pop_from_stack(stack);

			if ((value1 & 0xC7) == 0x06) {
				int result_address = read_next_word(reader);
				DEBUG_PRINT0("\n");

				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_gvar_ref(gvar_list, segment_start_list, ref_list, regs, var_values, segment_index, result_address, segment_start, value0, opcode_reference, 0, 1, top_defined, top_defined_relative, stack_top))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
				DEBUG_PRINT0("\n");
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
				DEBUG_PRINT0("\n");
			}
			else {
				DEBUG_PRINT0("\n");
			}

			return 0;
		}
	}
	else if ((value0 & 0xF8) == 0x90) { /* xchg */
		DEBUG_PRINT0("\n");
		return 0;
	}
	else if ((value0 & 0xFC) == 0xA0) {
		int offset;
		unsigned int current_segment_index;
		if (value0 == 0xA0) {
			set_register_al_undefined(regs, opcode_reference);
		}
		else if (value0 == 0xA1) {
			set_register_ax_undefined(regs, opcode_reference);
		}

		offset = read_next_word(reader);
		DEBUG_PRINT0("\n");

		current_segment_index = (segment_index >= 0)? segment_index : SEGMENT_INDEX_DS;
		if (value0 == 0xA3 && is_register_ax_defined(regs) && is_segment_register_defined_absolute(regs, current_segment_index)) {
			unsigned int addr = get_segment_register(regs, current_segment_index);
			addr = addr * 16 + offset;
			if ((offset & 1) == 0 && addr < 0x400) {
				const uint16_t value = get_register_ax(regs);
				const char *value_origin = get_register_ax_value_origin(regs);
				if ((addr & 2) == 0) {
					set_interruption_table_offset(int_table, addr >> 2, value_origin, value);
				}
				else if (is_register_ax_defined_relative(regs)) {
					set_interruption_table_segment_relative(int_table, addr >> 2, value_origin, value);
				}
				else {
					set_interruption_table_segment(int_table, addr >> 2, value_origin, value);
				}
			}
		}

		if (segment_index >= 0 && is_segment_register_defined_relative(regs, segment_index) || segment_index == SEGMENT_INDEX_UNDEFINED && is_register_ds_defined_relative(regs)) {
			unsigned int segment_value = (segment_index == SEGMENT_INDEX_UNDEFINED)? get_register_ds(regs) : get_segment_register(regs, segment_index);
			unsigned int relative_address = (segment_value * 16 + offset) & 0xFFFF;
			const char *target = segment_start + relative_address;
			const int var_index = index_of_gvar_with_start(gvar_list, target);
			struct GlobalVariable *var;
			if (var_index < 0) {
				var = prepare_new_gvar(gvar_list);
				var->start = target;
				var->end = target + 2;
				var->relative_address = relative_address;
				var->var_type = (value0 & 1)? GVAR_TYPE_WORD : GVAR_TYPE_BYTE;

				insert_gvar(gvar_list, var);
			}
			else {
				var = gvar_list->sorted_variables[var_index];
			}

			if (index_of_ref_with_instruction(ref_list, opcode_reference) < 0) {
				struct Reference *new_ref = prepare_new_ref(ref_list);
				new_ref->instruction = opcode_reference;
				set_gvar_ref_from_instruction_address(new_ref, var);
				if (value0 & 2) {
					set_gvar_ref_write_access(new_ref);
				}
				else {
					set_gvar_ref_read_access(new_ref);
				}

				insert_ref(ref_list, new_ref);
			}

			if (value0 == 0xA3) {
				const uint16_t original_value = *((uint16_t *) target);
				if (is_register_ax_defined_relative(regs)) {
					if ((error_code = put_gvar_in_gvwvmap_relative(var_values, target, get_register_ax(regs)))) {
						return error_code;
					}
				}
				else if (is_register_ax_defined(regs)) {
					const uint16_t reg_value = get_register_ax(regs);
					if (reg_value != original_value) {
						if ((error_code = put_gvar_in_gvwvmap(var_values, target, reg_value))) {
							return error_code;
						}
					}
					else if ((error_code = remove_gvwvalue_with_start(var_values, target))) {
						return error_code;
					}
				}
				else if ((error_code = put_gvar_in_gvwvmap_undefined(var_values, target))) {
					return error_code;
				}
			}
		}

		return 0;
	}
	else if ((value0 & 0xFC) == 0xA4) {
		DEBUG_PRINT0("\n");
		return 0;
	}
	else if (value0 == 0xA8) {
		read_next_byte(reader);
		DEBUG_PRINT0("\n");
		return 0;
	}
	else if (value0 == 0xA9) {
		read_next_word(reader);
		DEBUG_PRINT0("\n");
		return 0;
	}
	else if ((value0 & 0xFE) == 0xAA) {
		DEBUG_PRINT0("\n");
		return 0;
	}
	else if ((value0 & 0xFC) == 0xAC) {
		DEBUG_PRINT0("\n");
		return 0;
	}
	else if ((value0 & 0xF0) == 0xB0) {
		const int target_register = value0 & 7;
		if (value0 & 0x08) {
			const char *relocation_query = reader->buffer + reader->buffer_index;
			int word_value = read_next_word(reader);
			DEBUG_PRINT0("\n");

			if (is_relocation_present_in_sorted_relocations(sorted_relocations, relocation_count, relocation_query)) {
				set_word_register_relative(regs, target_register, opcode_reference, opcode_reference, word_value);
			}
			else {
				set_word_register(regs, target_register, opcode_reference, opcode_reference, word_value);
			}
		}
		else {
			int byte_value = read_next_byte(reader);
			DEBUG_PRINT0("\n");
			set_byte_register(regs, target_register, opcode_reference, opcode_reference, byte_value);
		}

		return 0;
	}
	else if ((value0 & 0xFE) == 0xC2) {
		struct CheckedBlocks checked_blocks;

		if (value0 == 0xC2) {
			read_next_word(reader);
		}
		DEBUG_PRINT0("\n  Finding origins of this function.\n");
		block->end = block->start + reader->buffer_index;

		checked_blocks.blocks = NULL;
		checked_blocks.count = 0;
		checked_blocks.allocated_pages = 0;

		update_call_origins(block, code_block_list, &checked_blocks, regs, stack, var_values, 0, 3);
		if (checked_blocks.blocks != NULL) {
			free(checked_blocks.blocks);
		}

		return 0;
	}
	else if ((value0 & 0xFE) == 0xC4) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0xC0) == 0xC0) {
			DEBUG_PRINT0(" Unknown opcode\n");
			return 1;
		}
		else {
			const int target_register_index = (value1 >> 3) & 7;
			const int target_segment_index = (value0 == 0xC4)? SEGMENT_INDEX_ES : SEGMENT_INDEX_DS;
			int result_address;
			if ((value1 & 0xC7) == 0x06) {
				result_address = read_next_word(reader);
				DEBUG_PRINT0("\n");

				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_far_pointer_gvar_ref(gvar_list, ref_list, regs, segment_index, result_address, segment_start, opcode_reference, 1))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
				DEBUG_PRINT0("\n");
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
				DEBUG_PRINT0("\n");
			}
			else {
				DEBUG_PRINT0("\n");
			}

			if ((value1 & 0xC7) == 0x06 && is_segment_register_defined_relative(regs, segment_index)) {
				unsigned int relative_address = (get_segment_register(regs, segment_index) * 16 + result_address) & 0xFFFF;
				const char *target = segment_start + relative_address;
				int index = index_of_gvar_in_gvwvmap_with_start(var_values, target);
				uint16_t offset_value;
				uint16_t segment_value;

				if (index < 0) {
					set_word_register(regs, target_register_index, opcode_reference, opcode_reference, *((uint16_t *) target));
				}
				else if (is_gvwvalue_defined_relative_at_index(var_values, index)) {
					set_word_register_relative(regs, target_register_index, opcode_reference, opcode_reference, get_gvwvalue_at_index(var_values, index));
				}
				else if (is_gvwvalue_defined_at_index(var_values, index)) {
					set_word_register(regs, target_register_index, opcode_reference, opcode_reference, get_gvwvalue_at_index(var_values, index));
				}
				else {
					set_word_register_undefined(regs, target_register_index, opcode_reference);
				}

				index = index_of_gvar_in_gvwvmap_with_start(var_values, target + 2);
				if (index < 0) {
					set_segment_register(regs, target_segment_index, opcode_reference, opcode_reference, ((uint16_t *) target)[1]);
				}
				else if (is_gvwvalue_defined_relative_at_index(var_values, index)) {
					set_segment_register_relative(regs, target_segment_index, opcode_reference, opcode_reference, get_gvwvalue_at_index(var_values, index));
				}
				else if (is_gvwvalue_defined_at_index(var_values, index)) {
					set_segment_register(regs, target_segment_index, opcode_reference, opcode_reference, get_gvwvalue_at_index(var_values, index));
				}
				else {
					set_segment_register_undefined(regs, target_segment_index, opcode_reference);
				}
			}
			else {
				set_word_register_undefined(regs, target_register_index, opcode_reference);
				if (value0 == 0xC4) {
					set_register_es_undefined(regs, opcode_reference);
				}
				else {
					/* Assuming value0 == 0xC5 */
					set_register_ds_undefined(regs, opcode_reference);
				}
			}

			return 0;
		}
	}
	else if ((value0 & 0xFE) == 0xC6) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x38) {
			DEBUG_PRINT0(" Unknown opcode\n");
			return 1;
		}
		else {
			int diff_address;
			int immediate_value;
			if ((value1 & 0xC0) == 0x40) {
				diff_address = read_next_byte(reader);
				if (diff_address >= 0x80) {
					diff_address -= 0x100;
				}
			}
			else if (value1 == 0x06 || (value1 & 0xC0) == 0x80) {
				diff_address = read_next_word(reader);
			}

			if (value0 & 1) {
				immediate_value = read_next_word(reader);
			}
			else {
				read_next_byte(reader);
			}
			DEBUG_PRINT0("\n");

			if (value1 == 0x06) {
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_gvar_ref(gvar_list, segment_start_list, ref_list, regs, var_values, segment_index, diff_address, segment_start, value0, opcode_reference, 0, 0, 0, 0, 0))) {
					return error_code;
				}

				if (value0 & 1 && is_segment_register_defined_relative(regs, segment_index)) {
					/* All this logic comes from add_global_variable_reference method. We should find a way to centralise this */
					unsigned int segment_value = get_segment_register(regs, segment_index);
					unsigned int relative_address = (segment_value * 16 + diff_address) & 0xFFFF;
					const char *target = segment_start + relative_address;
					if (index_of_gvar_with_start(gvar_list, target) >= 0) {
						const uint16_t original_value = *((uint16_t *) target);
						if (immediate_value != original_value) {
							if ((error_code = put_gvar_in_gvwvmap(var_values, target, immediate_value))) {
								return error_code;
							}
						}
						else if ((error_code = remove_gvwvalue_with_start(var_values, target))) {
							return error_code;
						}
					}
				}
			}
			else if (value0 == 0xC7 && (value1 == 0x46 || value1 == 0x86) && (segment_index == SEGMENT_INDEX_UNDEFINED || segment_index == SEGMENT_INDEX_SS)) {
				if (is_register_bp_defined_absolute(regs) && is_register_sp_defined_absolute(regs)) {
					const int from_top = (diff_address + get_register_bp(regs) - get_register_sp(regs)) >> 1;
					DEBUG_PRINT1("  From top resolved to: %d\n", from_top);
					set_in_stack_from_top(stack, from_top, immediate_value);
				}
				else if (is_register_sp_relative_from_bp(regs)) {
					set_in_stack_from_top(stack, (diff_address - get_register_sp(regs)) >> 1, immediate_value);
				}
			}

			return 0;
		}
	}
	else if (value0 == 0xCB) {
		struct CheckedBlocks checked_blocks;
		DEBUG_PRINT0("\n");

		block->end = block->start + reader->buffer_index;

		checked_blocks.blocks = NULL;
		checked_blocks.count = 0;
		checked_blocks.allocated_pages = 0;

		error_code = update_call_origins(block, code_block_list, &checked_blocks, regs, stack, var_values, 0, 0);
		if (checked_blocks.blocks != NULL) {
			free(checked_blocks.blocks);
		}

		return error_code;
	}
	else if (value0 == 0xCD) {
		const int interruption_number = read_next_byte(reader);
		DEBUG_PRINT0("\n");

		if (interruption_number == 0x1A && is_register_ah_defined(regs)) {
			const unsigned int ah_value = get_register_ah(regs);
			if (ah_value == 0x00) { /* Read System Clock Counter */
				set_register_ax_undefined(regs, opcode_reference);
				set_register_cx_undefined(regs, opcode_reference);
				set_register_dx_undefined(regs, opcode_reference);

				if ((error_code = add_call_return_origin_after_interruption(reader, regs, stack, var_values, block, code_block_list))) {
					return error_code;
				}
			}
		}
		else if (interruption_number == 0x20) {
			block->end = block->start + reader->buffer_index;
		}
		else if (interruption_number == 0x21 && is_register_ah_defined(regs)) {
			const unsigned int ah_value = get_register_ah(regs);
			if (ah_value == 0x09 && is_register_ds_defined_relative(regs) && is_register_dx_defined(regs)) {
				unsigned int relative_address = (get_register_ds(regs) * 16 + get_register_dx(regs)) & 0xFFFF;
				const char *target = segment_start + relative_address;
				struct GlobalVariable *var;
				const char *instruction;
				int index = index_of_gvar_with_start(gvar_list, target);
				if (index < 0) {
					var = prepare_new_gvar(gvar_list);
					var->start = target;
					var->end = target;
					var->relative_address = relative_address;
					var->var_type = GVAR_TYPE_DOLLAR_TERMINATED_STRING;
					insert_gvar(gvar_list, var);
				}
				else {
					var = gvar_list->sorted_variables[index];
				}
				/* What should we do if the variable is present, but its type does not match? Not sure. */

				instruction = get_register_dx_value_origin(regs);
				if (instruction && (((unsigned int) *instruction) & 0xFF) == 0xBA && index_of_ref_with_instruction(ref_list, instruction) < 0) {
					struct Reference *new_ref = prepare_new_ref(ref_list);
					new_ref->instruction = instruction;
					set_gvar_ref_from_instruction_immediate_value(new_ref, var);
					insert_ref(ref_list, new_ref);
				}

				if ((error_code = add_call_return_origin_after_interruption(reader, regs, stack, var_values, block, code_block_list))) {
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
				result = index_of_cblock_containing_position(code_block_list, jump_destination);
				potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
				if (potential_container && potential_container->start == jump_destination) {
					target_block = potential_container;
				}
				else {
					target_block = prepare_new_cblock(code_block_list);
					if (!target_block) {
						return 1;
					}

					target_block->relative_cs = target_relative_cs;
					target_block->ip = target_ip;
					target_block->start = jump_destination;
					target_block->end = jump_destination;
					target_block->flags = 0;
					initialize_cborigin_list(&target_block->origin_list);
					if ((result = add_interruption_type_cborigin_in_block(target_block, regs, var_values))) {
						return result;
					}

					if ((result = insert_cblock(code_block_list, target_block))) {
						return result;
					}

					if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
						potential_container->end = jump_destination;
						invalidate_cblock_check(potential_container);
					}
				}

				instruction = get_register_dx_value_origin(regs);
				if (instruction && (((unsigned int) *instruction) & 0xFF) == 0xBA && index_of_ref_with_instruction(ref_list, instruction) < 0) {
					struct Reference *new_ref = prepare_new_ref(ref_list);
					new_ref->instruction = instruction;
					set_cblock_ref_from_instruction_immediate_value(new_ref, target_block);
					insert_ref(ref_list, new_ref);
				}

				if ((error_code = add_call_return_origin_after_interruption(reader, regs, stack, var_values, block, code_block_list))) {
					return error_code;
				}
			}
			else if (ah_value == 0x30) { /* Get DOS version number */
				set_register_ax_undefined(regs, opcode_reference);
				set_register_cx_undefined(regs, opcode_reference);
				set_register_bx_undefined(regs, opcode_reference);

				if ((error_code = add_call_return_origin_after_interruption(reader, regs, stack, var_values, block, code_block_list))) {
					return error_code;
				}
			}
			else if (ah_value == 0x35) { /* Get interruption vector */
				set_register_bx_undefined(regs, opcode_reference);
				set_register_es_undefined(regs, opcode_reference);

				if ((error_code = add_call_return_origin_after_interruption(reader, regs, stack, var_values, block, code_block_list))) {
					return error_code;
				}
			}
			else if (ah_value == 0x40) { /* Write to file using handle */
				const int cx_defined = is_register_cx_defined(regs);
				const int cx_relative = is_register_cx_defined(regs);
				const uint16_t cx_value = is_register_cx_defined(regs);

				const int dx_defined = is_register_dx_defined(regs);
				const int dx_relative = is_register_dx_defined(regs);
				const uint16_t dx_value = is_register_dx_defined(regs);
				const char *dx_value_origin = get_register_dx_value_origin(regs);

				const int ds_defined = is_register_ds_defined(regs);
				const int ds_relative = is_register_ds_defined(regs);
				const uint16_t ds_value = is_register_ds_defined(regs);

				struct CheckedBlocks checked_blocks;
				checked_blocks.blocks = NULL;
				checked_blocks.count = 0;
				checked_blocks.allocated_pages = 0;

				DEBUG_PRINT0("  Checking for message references.\n");
				error_code = update_int2140_message_references(regs, segment_start, block, code_block_list, gvar_list, segment_start_list, ref_list, &checked_blocks, cx_defined, cx_relative, cx_value, dx_defined, dx_relative, dx_value, dx_value_origin, ds_defined, ds_relative, ds_value, 2);
				if (checked_blocks.blocks != NULL) {
					free(checked_blocks.blocks);
				}

				if (error_code) {
					return error_code;
				}

				set_register_ax_undefined(regs, opcode_reference);

				if ((error_code = add_call_return_origin_after_interruption(reader, regs, stack, var_values, block, code_block_list))) {
					return error_code;
				}
			}
			else if (ah_value == 0x4A) { /* Modify allocated memory block */
				set_register_ax_undefined(regs, opcode_reference);
				set_register_bx_undefined(regs, opcode_reference);

				if ((error_code = add_call_return_origin_after_interruption(reader, regs, stack, var_values, block, code_block_list))) {
					return error_code;
				}
			}
			else if (ah_value == 0x4C) {
				block->end = block->start + reader->buffer_index;
			}
		}
		return 0;
	}
	else if ((value0 & 0xFC) == 0xD0) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0x38) == 0x30) {
			DEBUG_PRINT0(" Unknown opcode\n");
			return 1;
		}
		else {
			read_block_instruction_address(reader, value1);
			DEBUG_PRINT0("\n");
			return 0;
		}
	}
	else if ((value0 & 0xFE) == 0xE8) {
		const char *jump_destination;
		int result;
		struct CodeBlock *potential_container;
		int potential_container_evaluated_at_least_once;
		int diff = read_next_word(reader);
		DEBUG_PRINT0("\n");

		if (block->ip + reader->buffer_index + diff >= 0x10000) {
			diff -= 0x10000;
		}

		jump_destination = block->start + reader->buffer_index + diff;
		result = index_of_cblock_containing_position(code_block_list, jump_destination);
		potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
		potential_container_evaluated_at_least_once = potential_container && potential_container->start != potential_container->end;
		if (potential_container_evaluated_at_least_once && potential_container->end <= jump_destination) {
			potential_container = NULL;
			potential_container_evaluated_at_least_once = 0;
		}

		if (value0 == 0xE8) {
			push_in_stack(stack, block->ip + reader->buffer_index);
			if (is_register_sp_defined_absolute(regs)) {
				set_register_sp(regs, opcode_reference, opcode_reference, get_register_sp(regs) - 2);
			}
		}

		if (potential_container && potential_container->start == jump_destination) {
			if ((error_code = add_jump_type_cborigin_in_block(potential_container, opcode_reference, regs, stack, var_values))) {
				return error_code;
			}
		}
		else {
			struct CodeBlock *new_block = prepare_new_cblock(code_block_list);
			if (!new_block) {
				return 1;
			}

			new_block->relative_cs = block->relative_cs;
			new_block->ip = block->ip + reader->buffer_index + diff;
			new_block->start = jump_destination;
			new_block->end = jump_destination;
			new_block->flags = 0;
			initialize_cborigin_list(&new_block->origin_list);

			if ((result = add_jump_type_cborigin_in_block(new_block, opcode_reference, regs, stack, var_values))) {
				return result;
			}

			if ((result = insert_cblock(code_block_list, new_block))) {
				return result;
			}

			if (potential_container_evaluated_at_least_once && potential_container->end > jump_destination) {
				potential_container->end = jump_destination;
				invalidate_cblock_check(potential_container);
			}
		}

		block->end = block->start + reader->buffer_index;
		return 0;
	}
	else if (value0 == 0xEA) {
		read_next_word(reader);
		read_next_word(reader);
		DEBUG_PRINT0("\n");
		block->end = block->start + reader->buffer_index;
		return 0;
	}
	else if (value0 == 0xEB) {
		const int value1 = read_next_byte(reader);
		const int diff = (value1 >= 0x80)? value1 - 0x100 : value1;
		const char *jump_destination = block->start + reader->buffer_index + diff;
		DEBUG_PRINT0("\n");

		block->end = block->start + reader->buffer_index;
		return register_jump_target_block(reader, regs, stack, var_values, block, code_block_list, jump_destination, opcode_reference, diff);
	}
	else if (value0 == 0xF2) {
		DEBUG_PRINT0("\n");
		return 0;
	}
	else if (value0 == 0xF3) {
		DEBUG_PRINT0("\n");
		return 0;
	}
	else if ((value0 & 0xFE) == 0xF6) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0x38) == 0x08) {
			DEBUG_PRINT0(" Unknown opcode\n");
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

			DEBUG_PRINT0("\n");
			return 0;
		}
	}
	else if (value0 == 0xFB) { /* sti */
		int i;
		DEBUG_PRINT0("\n");

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
				result = index_of_cblock_containing_position(code_block_list, jump_destination);
				potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
				if (potential_container && potential_container->start == jump_destination) {
					target_block = potential_container;
				}
				else {
					struct Registers int_regs;
					if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
						potential_container->end = jump_destination;
					}

					target_block = prepare_new_cblock(code_block_list);
					if (!target_block) {
						return 1;
					}

					target_block->relative_cs = target_relative_cs;
					target_block->ip = target_ip;
					target_block->start = jump_destination;
					target_block->end = jump_destination;
					target_block->flags = 0;
					initialize_cborigin_list(&target_block->origin_list);

					set_all_registers_undefined(&int_regs);
					set_register_cs_relative(&int_regs, NULL, where_interruption_segment_defined_in_table(int_table, i), target_relative_cs);
					if ((result = add_interruption_type_cborigin_in_block(target_block, &int_regs, var_values))) {
						return result;
					}

					if ((result = insert_cblock(code_block_list, target_block))) {
						return result;
					}
				}

				where_offset = where_interruption_offset_defined_in_table(int_table, i);
				if (where_offset && (((unsigned int) *where_offset) & 0xF8) == 0xB8) {
					if (index_of_ref_with_instruction(ref_list, where_offset) < 0) {
						struct Reference *new_ref = prepare_new_ref(ref_list);
						new_ref->instruction = where_offset;
						set_cblock_ref_from_instruction_immediate_value(new_ref, target_block);
						insert_ref(ref_list, new_ref);
					}
				}
			}
		}

		return 0;
	}
	else if ((value0 & 0xFC) == 0xF8 || (value0 & 0xFE) == 0xFC) {
		DEBUG_PRINT0("\n");
		return 0;
	}
	else if (value0 == 0xFE) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x30) {
			DEBUG_PRINT0(" Unknown opcode\n");
			return 1;
		}
		else {
			read_block_instruction_address(reader, value1);
			DEBUG_PRINT0("\n");
			return 0;
		}
	}
	else if (value0 == 0xFF) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0x38) == 0x38 || (value1 & 0xF8) == 0xD8 || (value1 & 0xF8) == 0xE8) {
			DEBUG_PRINT0(" Unknown opcode\n");
			return 1;
		}
		else {
			if ((value1 & 0xC7) == 0x06) {
				int result_address = read_next_word(reader);
				DEBUG_PRINT0("\n");

				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_gvar_ref(gvar_list, segment_start_list, ref_list, regs, var_values, segment_index, result_address, segment_start, value0, opcode_reference, 1, 0, 0, 0, 0))) {
					return error_code;
				}

				if ((value1 == 0x16 || value1 == 0x26) && is_segment_register_defined_relative(regs, segment_index)) {
					unsigned int segment_value = get_segment_register(regs, segment_index);
					unsigned int relative_address = (segment_value * 16 + result_address) & 0xFFFF;
					const char *var_target = segment_start + relative_address;
					int index = index_of_gvar_in_gvwvmap_with_start(var_values, var_target);
					unsigned int code_relative_target = (index < 0)? *((uint16_t *) var_target) : get_gvwvalue_at_index(var_values, index);
					const char *jump_destination;
					struct CodeBlock *potential_container;
					int result;
					int potential_container_evaluated_at_least_once;

					code_relative_target += ((unsigned int) get_register_cs(regs)) << 4;
					jump_destination = segment_start + code_relative_target;

					result = index_of_cblock_containing_position(code_block_list, jump_destination);
					potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
					potential_container_evaluated_at_least_once = potential_container && potential_container->start != potential_container->end;
					if (potential_container_evaluated_at_least_once && potential_container->end <= jump_destination) {
						potential_container = NULL;
						potential_container_evaluated_at_least_once = 0;
					}

					if (value1 == 0x16) {
						push_in_stack(stack, block->ip + reader->buffer_index);
						if (is_register_sp_defined_absolute(regs)) {
							set_register_sp(regs, opcode_reference, opcode_reference, get_register_sp(regs) - 2);
						}
					}

					if (potential_container && potential_container->start == jump_destination) {
						if ((error_code = add_jump_type_cborigin_in_block(potential_container, opcode_reference, regs, stack, var_values))) {
							return error_code;
						}
					}
					else {
						struct CodeBlock *new_block = prepare_new_cblock(code_block_list);
						if (!new_block) {
							return 1;
						}

						new_block->relative_cs = block->relative_cs;
						new_block->ip = code_relative_target;
						new_block->start = jump_destination;
						new_block->end = jump_destination;
						new_block->flags = 0;
						initialize_cborigin_list(&new_block->origin_list);

						if ((result = add_jump_type_cborigin_in_block(new_block, opcode_reference, regs, stack, var_values))) {
							return result;
						}

						if ((result = insert_cblock(code_block_list, new_block))) {
							return result;
						}

						if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
							potential_container->end = jump_destination;
							invalidate_cblock_check(potential_container);
						}
					}
				}
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
				DEBUG_PRINT0("\n");
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
				DEBUG_PRINT0("\n");
			}
			else{
				DEBUG_PRINT0("\n");
			}

			if ((value1 & 0x30) == 0x10 || (value1 & 0x30) == 0x20) {
				block->end = block->start + reader->buffer_index;
			}
			return 0;
		}
	}
	else {
		const int this_block_index = index_of_cblock_with_start(code_block_list, block->start);
		const char *new_end = reader->buffer + reader->buffer_size;
		DEBUG_PRINT0("\n");

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
	int result;
#ifdef DEBUG
	reader_debug_print_enabled = 1;
#endif

	result = read_block_instruction_internal(reader, regs, stack, var_values, int_table, segment_start, sorted_relocations, relocation_count, print_error, block, code_block_list, global_variable_list, segment_start_list, reference_list, SEGMENT_INDEX_UNDEFINED, instruction);
#ifdef DEBUG
	reader_debug_print_enabled = 0;
#endif

	return result;
}

static int read_block(
		struct Registers *regs,
		struct Stack *stack,
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
	struct InterruptionTable int_table;
	int error_code;

	reader.buffer = block->start;
	reader.buffer_index = 0;
	reader.buffer_size = block_max_size;

	set_all_interruption_table_undefined(&int_table);

	DEBUG_PRINT2("Reading block at +%x:%x\n", block->relative_cs, block->ip);
	do {
		int index;
		if ((error_code = read_block_instruction(&reader, regs, stack, var_values, &int_table, segment_start, sorted_relocations, relocation_count, print_error, block, code_block_list, global_variable_list, segment_start_list, reference_list))) {
			return error_code;
		}

		DEBUG_PRINT_STATE(block->ip + reader.buffer_index, regs, stack, var_values, segment_start, &int_table);
		index = index_of_cblock_with_start(code_block_list, block->start);
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

	return 0;
}

int find_cblocks_and_gvars(
		struct SegmentReadResult *read_result,
		void (*print_error)(const char *),
		struct CodeBlockList *cblock_list,
		struct GlobalVariableList *global_variable_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list) {
	struct CodeBlockOrigin *origin;
	struct CodeBlock *first_block = prepare_new_cblock(cblock_list);
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
	initialize_cborigin_list(&first_block->origin_list);

	origin = prepare_new_cborigin(&first_block->origin_list);
	set_os_type_in_cborigin(origin);
	set_all_registers_undefined(&origin->regs);
	set_register_cs_relative(&origin->regs, NULL, NULL, read_result->relative_cs);
	if (ds_should_match_cs_at_segment_start(read_result)) {
		set_register_ds_relative(&origin->regs, NULL, NULL, read_result->relative_cs);
	}
	initialize_stack(&origin->stack);
	initialize_gvwvmap(&origin->var_values);

	if ((error_code = insert_cborigin(&first_block->origin_list, origin))) {
		return error_code;
	}

	if ((error_code = insert_cblock(cblock_list, first_block))) {
		return error_code;
	}

	evaluate_all = 0;
	do {
		int block_index;
		any_evaluated = 0;
		any_not_ready = 0;

		for (block_index = 0; block_index < cblock_list->block_count; block_index++) {
			struct CodeBlock *block = get_unsorted_cblock(cblock_list, block_index);
			if (cblock_requires_evaluation(block)) {
				if (evaluate_all || cblock_ready_to_be_evaluated(block)) {
					struct Registers regs;
					struct Stack stack;
					unsigned int block_max_size;
					struct GlobalVariableWordValueMap var_values;

					any_evaluated = 1;
					mark_cblock_as_being_evaluated(block);

					block_max_size = read_result->size - (block->start - read_result->buffer);

					accumulate_registers_from_cbolist(&regs, &block->origin_list);
					initialize_stack(&stack);
					if ((error_code = accumulate_stack_from_cbolist(&stack, &block->origin_list))) {
						return error_code;
					}

					initialize_gvwvmap(&var_values);
					accumulate_gvwvmap_from_cbolist(&var_values, &block->origin_list);

					if ((error_code = read_block(&regs, &stack, &var_values, read_result->buffer, read_result->sorted_relocations, read_result->relocation_count, print_error, block, block_max_size, cblock_list, global_variable_list, segment_start_list, reference_list))) {
						return error_code;
					}

					clear_stack(&stack);
					clear_gvwvmap(&var_values);
					mark_cblock_as_evaluated(block);
					DEBUG_CBLIST(cblock_list);
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
			const int index = index_of_cblock_containing_position(cblock_list, variable->start);
			if (index < 0 || cblock_list->sorted_blocks[index]->end <= variable->start) {
				const char *potential_end = read_result->buffer + read_result->size;
				const char *end;

				if (index + 1 < cblock_list->block_count) {
					potential_end = cblock_list->sorted_blocks[index + 1]->start;
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
				variable->end = cblock_list->sorted_blocks[index]->end;
			}
		}
	}
	return 0;
}
