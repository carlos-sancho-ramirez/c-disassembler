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

static void print_uint(struct FilePrinter *printer, unsigned int value) {
	char number[2];

	if (value >= 10) {
		print_uint(printer, value / 10);
	}

	number[0] = HEX_CHAR[value % 10];
	number[1] = '\0';
	print(printer, number);
}

void print_variable_label(struct FilePrinter *printer, unsigned int address) {
	print(printer, "var");
	if ((printer->flags & PRINTER_FLAG_FORMAT_MASK) == PRINTER_FLAG_FORMAT_BIN) {
		print_literal_hex_word_no_prefix(printer, (address + 0x100) & 0xFFFF);
	}
	else {
		print_literal_hex_20bit_value_no_prefix(printer, address);
	}
}

void print_code_label(struct FilePrinter *printer, int ip, int cs) {
	const char *block_start = printer->buffer_start + (cs * 16 + ip & 0xFFFFF);
	int index = index_of_func_containing_block_start(printer->func_list, block_start);
	if (index >= 0) {
		print(printer, "func");
		print_uint(printer, index + 1);
		print(printer, "_");
	}

	print(printer, "addr");
	if ((printer->flags & PRINTER_FLAG_FORMAT_MASK) == PRINTER_FLAG_FORMAT_BIN) {
		print_literal_hex_word_no_prefix(printer, ip & 0xFFFF);
	}
	else { /* (printer->flags & PRINTER_FLAG_FORMAT_MASK) == PRINTER_FLAG_FORMAT_DOS */
		print_literal_hex_word_no_prefix(printer, cs);
		print(printer, "_");
		print_literal_hex_word_no_prefix(printer, ip);
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
