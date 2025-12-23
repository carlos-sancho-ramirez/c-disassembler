#ifndef _PRINT_UTILS_H_
#define _PRINT_UTILS_H_

extern const char *buffer_start;

void print_literal_hex_byte(void (*print)(const char *), int value);
void print_literal_hex_word(void (*print)(const char *), int value);
void print_literal_hex_word_no_prefix(void (*print)(const char *), int value);
void print_differential_hex_byte(void (*print)(const char *), int value);
void print_differential_hex_word(void (*print)(const char *), int value);

void print_segment_start_label(void (*print)(const char *), const char *start);
void print_bin_address_label(void (*print)(const char *), int ip, int cs);
void print_dos_address_label(void (*print)(const char *), int ip, int cs);
void print_bin_variable_label(void (*print)(const char *), unsigned int address);
void print_dos_variable_label(void (*print)(const char *), unsigned int address);

#endif /* _PRINT_UTILS_H_ */
