#ifndef _INTERRUPTION_TABLE_H_
#define _INTERRUPTION_TABLE_H_
#include <stdint.h>

struct FarPointer {
    uint16_t offset;
    uint16_t segment;
};

struct InterruptionTable {
    struct FarPointer pointers[256];
    char defined[64];
    char relative[32];
};

int is_interruption_defined_and_relative_in_table(struct InterruptionTable *table, uint8_t index);
uint16_t get_interruption_table_offset(struct InterruptionTable *table, uint8_t index);
uint16_t get_interruption_table_relative_segment(struct InterruptionTable *table, uint8_t index);

void set_interruption_table_offset(struct InterruptionTable *table, uint8_t index, uint16_t value);
void set_interruption_table_segment(struct InterruptionTable *table, uint8_t index, uint16_t value);
void set_interruption_table_segment_relative(struct InterruptionTable *table, uint8_t index, uint16_t value);
void make_all_interruption_table_undefined(struct InterruptionTable *table);

#endif // _INTERRUPTION_TABLE_H_
