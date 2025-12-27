#include "printu.h"

const char HEX_CHAR[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

const char *buffer_start;

void print_literal_hex_byte(void (*print)(const char *), int value) {
	char number[] = "0x00";
	number[3] = HEX_CHAR[value & 0x0F];
	number[2] = HEX_CHAR[(value >> 4) & 0x0F];
	print(number);
}

void print_literal_hex_word(void (*print)(const char *), int value) {
	char number[] = "0x0000";
	number[5] = HEX_CHAR[value & 0x000F];
	number[4] = HEX_CHAR[(value >> 4) & 0x000F];
	number[3] = HEX_CHAR[(value >> 8) & 0x000F];
	number[2] = HEX_CHAR[(value >> 12) & 0x000F];
	print(number);
}

void print_literal_hex_word_no_prefix(void (*print)(const char *), int value) {
	char number[] = "0000";
	number[3] = HEX_CHAR[value & 0x000F];
	number[2] = HEX_CHAR[(value >> 4) & 0x000F];
	number[1] = HEX_CHAR[(value >> 8) & 0x000F];
	number[0] = HEX_CHAR[(value >> 12) & 0x000F];
	print(number);
}

void print_literal_hex_20bit_value_no_prefix(void (*print)(const char *), int value) {
	char number[] = "00000";
	number[4] = HEX_CHAR[value & 0x0000F];
	number[3] = HEX_CHAR[(value >> 4) & 0x0000F];
	number[2] = HEX_CHAR[(value >> 8) & 0x0000F];
	number[1] = HEX_CHAR[(value >> 12) & 0x0000F];
	number[0] = HEX_CHAR[(value >> 16) & 0x0000F];
	print(number);
}

void print_differential_hex_byte(void (*print)(const char *), int value) {
	char number[] = "+0x00";
	if (value & 0x80) {
		number[0] = '-';
		value = (value ^ 0xFF) + 1;
	}

	number[4] = HEX_CHAR[value & 0x0F];
	number[3] = HEX_CHAR[(value >> 4) & 0x0F];
	print(number);
}

void print_differential_hex_word(void (*print)(const char *), int value) {
	char number[] = "+0x0000";
	if (value & 0x8000) {
		number[0] = '-';
		value = (value ^ 0xFFFF) + 1;
	}

	number[6] = HEX_CHAR[value & 0x000F];
	number[5] = HEX_CHAR[(value >> 4) & 0x000F];
	number[4] = HEX_CHAR[(value >> 8) & 0x000F];
	number[3] = HEX_CHAR[(value >> 12) & 0x000F];
	print(number);
}

void print_uint(void (*print)(const char *), unsigned int value) {
	char number[2];

	if (value >= 10) {
		print_uint(print, value / 10);
	}

	number[0] = HEX_CHAR[value % 10];
	number[1] = '\0';
	print(number);
}

void print_bin_address_label(void (*print)(const char *), int ip, int cs) {
	print("addr");
	print_literal_hex_word_no_prefix(print, ip & 0xFFFF);
}

void print_bin_variable_label(void (*print)(const char *), unsigned int address) {
	print("var");
	print_literal_hex_word_no_prefix(print, (address + 0x100) & 0xFFFF);
}

void print_dos_address_label(void (*print)(const char *), int ip, int cs) {
	print("addr");
	print_literal_hex_word_no_prefix(print, cs);
	print("_");
	print_literal_hex_word_no_prefix(print, ip);
}

void print_dos_variable_label(void (*print)(const char *), unsigned int address) {
	print("var");
	print_literal_hex_20bit_value_no_prefix(print, address);
}

void print_segment_start_label(void (*print)(const char *), const char *start) {
	int diff = start - buffer_start;

	print("seg");
	print_literal_hex_word_no_prefix(print, diff >> 4);

	if (diff & 0xF) {
		char suffix[] = "_X";
		suffix[1] = HEX_CHAR[diff & 0xF];
		print(suffix);
	}
}
