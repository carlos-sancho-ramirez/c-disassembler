#include "interruption_table.h"

int is_interruption_defined_and_relative_in_table(struct InterruptionTable *table, uint8_t index) {
    const int value = 0x03 << ((index & 0x03) * 2);
    return (table->defined[index >> 2] & value) == value && (table->relative[index >> 3] & (1 << (index & 0x07)));
}

uint16_t get_interruption_table_offset(struct InterruptionTable *table, uint8_t index) {
    return table->pointers[index].offset;
}

uint16_t get_interruption_table_relative_segment(struct InterruptionTable *table, uint8_t index) {
    return table->pointers[index].segment;
}

void set_interruption_table_offset(struct InterruptionTable *table, uint8_t index, uint16_t value) {
    table->pointers[index].offset = value;
    table->defined[index >> 2] |= 1 << ((index & 0x03) * 2);
}

void set_interruption_table_segment(struct InterruptionTable *table, uint8_t index, uint16_t value) {
    table->pointers[index].segment = value;
    table->defined[index >> 2] |= 1 << ((index & 0x03) * 2 + 1);
    table->relative[index >> 3] &= ~(1 << (index & 0x07));
}

void set_interruption_table_segment_relative(struct InterruptionTable *table, uint8_t index, uint16_t value) {
    table->pointers[index].segment = value;
    table->defined[index >> 2] |= 1 << ((index & 0x03) * 2 + 1);
    table->relative[index >> 3] |= 1 << (index & 0x07);
}

void make_all_interruption_table_undefined(struct InterruptionTable *table) {
    for (int i = 0; i < 64; i++) {
        table->defined[i] = 0;
    }
}
