#include "itable.h"
#include <stdlib.h>

int is_interruption_defined_and_relative_in_table(struct InterruptionTable *table, uint8_t index) {
	const int word_index = index >> 4;
	const int mask = 1 << (index & 15);
	return table->offset_defined[word_index] & mask && table->segment_defined[word_index] & mask && table->relative[word_index] & mask;
}

const char *where_interruption_offset_defined_in_table(struct InterruptionTable *table, uint8_t index) {
	return table->offset_origin[index];
}

const char *where_interruption_segment_defined_in_table(struct InterruptionTable *table, uint8_t index) {
	return table->segment_origin[index];
}

uint16_t get_interruption_table_offset(struct InterruptionTable *table, uint8_t index) {
	return table->pointers[index].offset;
}

uint16_t get_interruption_table_relative_segment(struct InterruptionTable *table, uint8_t index) {
	return table->pointers[index].segment;
}

void set_interruption_table_offset(struct InterruptionTable *table, uint8_t index, const char *where, uint16_t value) {
	table->pointers[index].offset = value;
	table->offset_origin[index] = where;
	table->offset_defined[index >> 4] |= 1 << (index & 15);
}

void set_interruption_table_segment(struct InterruptionTable *table, uint8_t index, const char *where, uint16_t value) {
	table->pointers[index].segment = value;
	table->segment_origin[index] = where;
	table->segment_defined[index >> 4] |= 1 << (index & 15);
	table->relative[index >> 4] &= ~(1 << (index & 15));
}

void set_interruption_table_segment_relative(struct InterruptionTable *table, uint8_t index, const char *where, uint16_t value) {
	table->pointers[index].segment = value;
	table->segment_origin[index] = where;
	table->segment_defined[index >> 4] |= 1 << (index & 15);
	table->relative[index >> 4] |= 1 << (index & 15);
}

void set_all_interruption_table_undefined(struct InterruptionTable *table) {
	int i;
	for (i = 0; i < 16; i++) {
		table->offset_defined[i] = 0;
		table->segment_defined[i] = 0;
	}
}
