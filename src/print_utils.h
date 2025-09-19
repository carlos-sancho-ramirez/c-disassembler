#ifndef _PRINTERS_H_
#define _PRINTERS_H_

void print_literal_hex_byte(void (*print)(const char *), int value);
void print_literal_hex_word(void (*print)(const char *), int value);
void print_differential_hex_byte(void (*print)(const char *), int value);
void print_differential_hex_word(void (*print)(const char *), int value);

void print_address_label(void (*print)(const char *), int ip, int cs);
void print_variable_label(void (*print)(const char *), unsigned int address);

#endif // _PRINTERS_H_
