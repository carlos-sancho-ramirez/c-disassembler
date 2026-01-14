#include "printu.h"

const char HEX_CHAR[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

#define PRINTER_FLAG_FORMAT_MASK 1
#define PRINTER_FLAG_FORMAT_BIN 0
#define PRINTER_FLAG_FORMAT_DOS 1

void print(struct FilePrinter *printer, const char *str) {
	fprintf(printer->file, "%s", str);
}

void print_literal_hex_byte(struct FilePrinter *printer, int value) {
	char number[] = "0x00";
	number[3] = HEX_CHAR[value & 0x0F];
	number[2] = HEX_CHAR[(value >> 4) & 0x0F];
	print(printer, number);
}

void print_literal_hex_word(struct FilePrinter *printer, int value) {
	char number[] = "0x0000";
	number[5] = HEX_CHAR[value & 0x000F];
	number[4] = HEX_CHAR[(value >> 4) & 0x000F];
	number[3] = HEX_CHAR[(value >> 8) & 0x000F];
	number[2] = HEX_CHAR[(value >> 12) & 0x000F];
	print(printer, number);
}

static void print_literal_hex_word_no_prefix(struct FilePrinter *printer, int value) {
	char number[] = "0000";
	number[3] = HEX_CHAR[value & 0x000F];
	number[2] = HEX_CHAR[(value >> 4) & 0x000F];
	number[1] = HEX_CHAR[(value >> 8) & 0x000F];
	number[0] = HEX_CHAR[(value >> 12) & 0x000F];
	print(printer, number);
}

void print_literal_hex_20bit_value_no_prefix(struct FilePrinter *printer, int value) {
	char number[] = "00000";
	number[4] = HEX_CHAR[value & 0x0000F];
	number[3] = HEX_CHAR[(value >> 4) & 0x0000F];
	number[2] = HEX_CHAR[(value >> 8) & 0x0000F];
	number[1] = HEX_CHAR[(value >> 12) & 0x0000F];
	number[0] = HEX_CHAR[(value >> 16) & 0x0000F];
	print(printer, number);
}

void print_differential_hex_byte(struct FilePrinter *printer, int value) {
	char number[] = "+0x00";
	if (value & 0x80) {
		number[0] = '-';
		value = (value ^ 0xFF) + 1;
	}

	number[4] = HEX_CHAR[value & 0x0F];
	number[3] = HEX_CHAR[(value >> 4) & 0x0F];
	print(printer, number);
}

void print_differential_hex_word(struct FilePrinter *printer, int value) {
	char number[] = "+0x0000";
	if (value & 0x8000) {
		number[0] = '-';
		value = (value ^ 0xFFFF) + 1;
	}

	number[6] = HEX_CHAR[value & 0x000F];
	number[5] = HEX_CHAR[(value >> 4) & 0x000F];
	number[4] = HEX_CHAR[(value >> 8) & 0x000F];
	number[3] = HEX_CHAR[(value >> 12) & 0x000F];
	print(printer, number);
}

static void compose_uint_str(char *buffer, unsigned int *buffer_index_ptr, unsigned int value) {
	if (value >= 10) {
		compose_uint_str(buffer, buffer_index_ptr, value / 10);
	}

	buffer[(*buffer_index_ptr)++] = HEX_CHAR[value % 10];
}

void print_variable_label(struct FilePrinter *printer, unsigned int address) {
	char buffer[9] = "var00000";
	int index;

	if ((printer->flags & PRINTER_FLAG_FORMAT_MASK) == PRINTER_FLAG_FORMAT_BIN) {
		const int value = (address + 0x100) & 0xFFFF;
		buffer[6] = HEX_CHAR[value & 0x000F];
		buffer[5] = HEX_CHAR[(value >> 4) & 0x000F];
		buffer[4] = HEX_CHAR[(value >> 8) & 0x000F];
		buffer[3] = HEX_CHAR[(value >> 12) & 0x000F];
		buffer[7] = '\0';
	}
	else {
		buffer[7] = HEX_CHAR[address & 0x0000F];
		buffer[6] = HEX_CHAR[(address >> 4) & 0x0000F];
		buffer[5] = HEX_CHAR[(address >> 8) & 0x0000F];
		buffer[4] = HEX_CHAR[(address >> 12) & 0x0000F];
		buffer[3] = HEX_CHAR[(address >> 16) & 0x0000F];
	}

	index = index_of_key_in_rename_map(printer->renames, buffer);
	if (index >= 0) {
		print(printer, printer->renames->entries[index].value);
	}
	else {
		print(printer, buffer);
	}
}

#define PRINT_CODE_LABEL_BUFFER_MAX_SIZE 24
#include <assert.h>

void print_code_label(struct FilePrinter *printer, int ip, int cs) {
	char buffer[PRINT_CODE_LABEL_BUFFER_MAX_SIZE];
	unsigned int buffer_index = 0;
	const char *block_start = printer->buffer_start + (cs * 16 + ip & 0xFFFFF);
	int func_index = index_of_func_containing_block_start(printer->func_list, block_start);
	int index;

	if (func_index >= 0) {
		buffer[buffer_index++] = 'f';
		buffer[buffer_index++] = 'u';
		buffer[buffer_index++] = 'n';
		buffer[buffer_index++] = 'c';
		compose_uint_str(buffer, &buffer_index, func_index + 1);
		buffer[buffer_index++] = '_';
	}

	buffer[buffer_index++] = 'a';
	buffer[buffer_index++] = 'd';
	buffer[buffer_index++] = 'd';
	buffer[buffer_index++] = 'r';
	if ((printer->flags & PRINTER_FLAG_FORMAT_MASK) == PRINTER_FLAG_FORMAT_DOS) {
		buffer[buffer_index + 3] = HEX_CHAR[cs & 0x000F];
		buffer[buffer_index + 2] = HEX_CHAR[(cs >> 4) & 0x000F];
		buffer[buffer_index + 1] = HEX_CHAR[(cs >> 8) & 0x000F];
		buffer[buffer_index] = HEX_CHAR[(cs >> 12) & 0x000F];
		buffer[buffer_index + 4] = '_';
		buffer_index += 5;
	}

	buffer[buffer_index + 3] = HEX_CHAR[ip & 0x000F];
	buffer[buffer_index + 2] = HEX_CHAR[(ip >> 4) & 0x000F];
	buffer[buffer_index + 1] = HEX_CHAR[(ip >> 8) & 0x000F];
	buffer[buffer_index] = HEX_CHAR[(ip >> 12) & 0x000F];
	buffer[buffer_index + 4] = '\0';
	assert(buffer_index + 5 <= PRINT_CODE_LABEL_BUFFER_MAX_SIZE);

	index = index_of_key_in_rename_map(printer->renames, buffer);
	if (index >= 0) {
		print(printer, printer->renames->entries[index].value);
	}
	else {
		print(printer, buffer);
	}
}

void print_segment_label(struct FilePrinter *printer, const char *start) {
	int diff = start - printer->buffer_start;

	print(printer, "seg");
	print_literal_hex_word_no_prefix(printer, diff >> 4);

	if (diff & 0xF) {
		char suffix[] = "_X";
		suffix[1] = HEX_CHAR[diff & 0xF];
		print(printer, suffix);
	}
}

void set_printer_bin_format(struct FilePrinter *printer) {
	printer->flags &= ~PRINTER_FLAG_FORMAT_MASK;
	printer->flags |= PRINTER_FLAG_FORMAT_BIN;
}

void set_printer_dos_format(struct FilePrinter *printer) {
	printer->flags &= ~PRINTER_FLAG_FORMAT_MASK;
	printer->flags |= PRINTER_FLAG_FORMAT_DOS;
}
