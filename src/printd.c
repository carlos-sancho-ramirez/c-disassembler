#include "printd.h"
#include <stdio.h>

static void print_word_or_byte_register(struct Registers *regs, unsigned int index, const char *word_reg, const char *high_byte_reg, const char *low_byte_reg) {
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

static void print_word_register(struct Registers *regs, unsigned int index, const char *word_reg) {
	if (is_word_register_defined_relative(regs, index)) {
		fprintf(stderr, " %s=+%x;", word_reg, get_word_register(regs, index));
	}
	else if (is_word_register_defined(regs, index)) {
		fprintf(stderr, " %s=%x;", word_reg, get_word_register(regs, index));
	}
	else {
		fprintf(stderr, " %s=?;", word_reg);
	}
}

static void print_segment_register(struct Registers *regs, unsigned int index, const char *word_reg) {
	if (is_segment_register_defined_relative(regs, index)) {
		fprintf(stderr, " %s=+%x;", word_reg, get_segment_register(regs, index));
	}
	else if (is_segment_register_defined(regs, index)) {
		fprintf(stderr, " %s=%x;", word_reg, get_segment_register(regs, index));
	}
	else {
		fprintf(stderr, " %s=?;", word_reg);
	}
}

void print_regs(struct Registers *regs) {
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

void print_stack(struct Stack *stack) {
	int i;
	fprintf(stderr, " Stack(");
	for (i = 0; i < stack->word_count; i++) {
		const unsigned int definition = stack->defined_and_relative[i >> 3] >> ((i & 7) * 2);
		if (i > 0) {
			fprintf(stderr, ", ");
		}

		if ((definition & 1) == 0) {
			fprintf(stderr, "?");
		}
		else if (definition & 2) {
			fprintf(stderr, "+%x", stack->values[i]);
		}
		else {
			fprintf(stderr, "%x", stack->values[i]);
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
		if (is_gvwvalue_relative_at_index(map, i)) {
			fprintf(stderr, "+");
		}

		fprintf(stderr, "%x", map->values[i]);
	}

	fprintf(stderr, ")");
}

void print_itable(struct InterruptionTable *table) {
	int i;
	fprintf(stderr, " IntTable(");
	for (i = 0; i < 256; i++) {
		const char *offset_defined = table->offset_defined[i];
		const char *segment_defined = table->segment_defined[i];
		if (offset_defined && segment_defined) {
			if (table->relative[i >> 3] & (1 << (i & 7))) {
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
			if (table->relative[i >> 3] & (1 << (i & 7))) {
				fprintf(stderr, "%x->+%x:?", i, table->pointers[i].segment);
			}
			else {
				fprintf(stderr, "%x->%x:?", i, table->pointers[i].segment);
			}
		}
	}

	fprintf(stderr, ")\n");
}
