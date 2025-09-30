#include "interruption_table.h"
#include <stdlib.h>

int is_interruption_defined_and_relative_in_table(struct InterruptionTable *table, uint8_t index) {
    return table->offset_defined[index] && table->segment_defined[index] && (table->relative[index >> 3] & (1 << (index & 0x07)));
}

const char *where_interruption_offset_defined_in_table(struct InterruptionTable *table, uint8_t index) {
    return table->offset_defined[index];
}

uint16_t get_interruption_table_offset(struct InterruptionTable *table, uint8_t index) {
    return table->pointers[index].offset;
}

uint16_t get_interruption_table_relative_segment(struct InterruptionTable *table, uint8_t index) {
    return table->pointers[index].segment;
}

void set_interruption_table_offset(struct InterruptionTable *table, uint8_t index, const char *where, uint16_t value) {
    table->pointers[index].offset = value;
    table->offset_defined[index] = where;
}

void set_interruption_table_segment(struct InterruptionTable *table, uint8_t index, const char *where, uint16_t value) {
    table->pointers[index].segment = value;
    table->segment_defined[index] = where;
    table->relative[index >> 3] &= ~(1 << (index & 0x07));
}

void set_interruption_table_segment_relative(struct InterruptionTable *table, uint8_t index, const char *where, uint16_t value) {
    table->pointers[index].segment = value;
    table->segment_defined[index] = where;
    table->relative[index >> 3] |= 1 << (index & 0x07);
}

void make_all_interruption_table_undefined(struct InterruptionTable *table) {
    for (int i = 0; i < 256; i++) {
        table->offset_defined[i] = NULL;
        table->segment_defined[i] = NULL;
    }
}
