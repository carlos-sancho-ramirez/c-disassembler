#include "printd.h"
#include <stdio.h>

static void print_word_or_byte_register(const struct Registers *regs, unsigned int index, const char *word_reg, const char *high_byte_reg, const char *low_byte_reg) {
	if (is_word_register_defined(regs, index)) {
		if (is_word_register_defined_relative(regs, index)) {
			fprintf(stderr, " %s=+%x;", word_reg, get_word_register(regs, index));
		}
		else {
			fprintf(stderr, " %s=%x;", word_reg, get_word_register(regs, index));
		}
	}
	else if (is_byte_register_defined(regs, index + 4)) {
		fprintf(stderr, " %s=%x;", high_byte_reg, get_byte_register(regs, index + 4));

		if (is_byte_register_defined(regs, index)) {
			fprintf(stderr, " %s=%x;", low_byte_reg, get_byte_register(regs, index));
		}
	}
	else if (is_byte_register_defined(regs, index)) {
		fprintf(stderr, " %s=%x;", low_byte_reg, get_byte_register(regs, index));
	}
	else {
		fprintf(stderr, " %s=?;", word_reg);
	}
}

static void print_word_register(const struct Registers *regs, unsigned int index, const char *word_reg) {
	fprintf(stderr, " %s=", word_reg);

	if (is_word_register_defined_relative(regs, index)) {
		fprintf(stderr, "+%x;", get_word_register(regs, index));
	}
	else if (is_word_register_defined(regs, index)) {
		fprintf(stderr, "%x;", get_word_register(regs, index));
	}
	else if (index == 4 && is_register_sp_relative_from_bp(regs)) {
		fprintf(stderr, "BP+%x;", get_word_register(regs, 4));
	}
	else {
		fprintf(stderr, "?;");
	}
}

static void print_segment_register(const struct Registers *regs, unsigned int index, const char *word_reg) {
	fprintf(stderr, " %s=", word_reg);

	if (is_segment_register_defined_relative(regs, index)) {
		fprintf(stderr, "+%x;", get_segment_register(regs, index));
	}
	else if (is_segment_register_defined(regs, index)) {
		fprintf(stderr, "%x;", get_segment_register(regs, index));
	}
	else {
		fprintf(stderr, "?;");
	}
}

void print_regs(const struct Registers *regs) {
	print_word_or_byte_register(regs, 0, "AX", "AH", "AL");
	print_word_or_byte_register(regs, 1, "CX", "CH", "CL");
	print_word_or_byte_register(regs, 2, "DX", "DH", "DL");
	print_word_or_byte_register(regs, 3, "BX", "BH", "BL");
	print_word_register(regs, 4, "SP");
	print_word_register(regs, 5, "BP");
	print_word_register(regs, 6, "SI");
	print_word_register(regs, 7, "DI");
	print_segment_register(regs, 0, "ES");
	print_segment_register(regs, 1, "CS");
	print_segment_register(regs, 2, "SS");
	print_segment_register(regs, 3, "DS");
}

void print_stack(const struct Stack *stack) {
	int defined_value_printed = 0;
	int i;
	fprintf(stderr, " Stack(");
	for (i = 0; i < stack->word_count; i++) {
		const unsigned int definition = stack->defined_and_relative[i >> 3] >> ((i & 7) * 2);
		if ((definition & 1) == 0) {
			if (defined_value_printed) {
				fprintf(stderr, ", ?");
			}
		}
		else if (definition & 2) {
			if (defined_value_printed) {
				fprintf(stderr, ", +%x", stack->values[i]);
			}
			else {
				fprintf(stderr, "+%x", stack->values[i]);
				defined_value_printed = 1;
			}
		}
		else {
			if (defined_value_printed) {
				fprintf(stderr, ", %x", stack->values[i]);
			}
			else {
				fprintf(stderr, "%x", stack->values[i]);
				defined_value_printed = 1;
			}
		}
	}

	fprintf(stderr, ")");
}

void print_gvwvmap(const struct GlobalVariableWordValueMap *map, const char *buffer) {
	int i;
	fprintf(stderr, " Vars(");
	for (i = 0; i < map->entry_count; i++) {
		if (i > 0) {
			fprintf(stderr, ", ");
		}

		fprintf(stderr, "%lx->", map->keys[i] - buffer);
		if (is_gvwvalue_defined_at_index(map, i)) {
			if (is_gvwvalue_defined_relative_at_index(map, i)) {
				fprintf(stderr, "+");
			}

			fprintf(stderr, "%x", map->values[i]);
		}
		else {
			fprintf(stderr, "?");
		}
	}

	fprintf(stderr, ")");
}

void print_itable(const struct InterruptionTable *table) {
	int i;
	fprintf(stderr, " IntTable(");
	for (i = 0; i < 256; i++) {
		int offset_defined = table->offset_defined[i >> 4] & 1 << (i & 15);
		int segment_defined = table->segment_defined[i >> 4] & 1 << (i & 15);
		if (offset_defined && segment_defined) {
			if (table->relative[i >> 4] & 1 << (i & 15)) {
				fprintf(stderr, "%x->+%x:%x", i, table->pointers[i].segment, table->pointers[i].offset);
			}
			else {
				fprintf(stderr, "%x->%x:%x", i, table->pointers[i].segment, table->pointers[i].offset);
			}
		}
		else if (offset_defined) {
			fprintf(stderr, "%x->?:%x", i, table->pointers[i].offset);
		}
		else if (segment_defined) {
			if (table->relative[i >> 4] & 1 << (i & 15)) {
				fprintf(stderr, "%x->+%x:?", i, table->pointers[i].segment);
			}
			else {
				fprintf(stderr, "%x->%x:?", i, table->pointers[i].segment);
			}
		}
	}

	fprintf(stderr, ")\n");
}

void print_cblist(const struct CodeBlockList *list) {
	int i;
	fprintf(stderr, "CodeBlockList(");
	for (i = 0; i < list->block_count; i++) {
		const struct CodeBlock *block = list->sorted_blocks[i];
		const struct CodeBlockOriginList *origin_list = &block->origin_list;
		int origin_index;
		if (i > 0) {
			fprintf(stderr, ", ");
		}

		fprintf(stderr, "+%X:%X-%X_%d(", block->relative_cs, block->ip, (int) (block->ip + (block->end - block->start)), block->flags);
		for (origin_index = 0; origin_index < origin_list->origin_count; origin_index++) {
			struct CodeBlockOrigin *origin = origin_list->sorted_origins[origin_index];
			const int origin_type = get_cborigin_type(origin);
			if (origin_index > 0) {
				fprintf(stderr, ", ");
			}

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
		fprintf(stderr, ")");
	}

	fprintf(stderr, ")\n");
}
