#ifndef _PRINT_UTILS_H_
#define _PRINT_UTILS_H_

#include <stdio.h>
#include "funclist.h"

struct FilePrinter {
	unsigned int flags;
	const char *buffer_start;
	FILE *file;
	struct FunctionList *func_list;
};

void print(struct FilePrinter *printer, const char *str);
void print_literal_hex_byte(struct FilePrinter *printer, int value);
void print_literal_hex_word(struct FilePrinter *printer, int value);
void print_differential_hex_byte(struct FilePrinter *printer, int value);
void print_differential_hex_word(struct FilePrinter *printer, int value);

void print_variable_label(struct FilePrinter *printer, unsigned int address);
void print_code_label(struct FilePrinter *printer, int ip, int cs);
void print_segment_label(struct FilePrinter *printer, const char *start);

void set_printer_bin_format(struct FilePrinter *printer);
void set_printer_dos_format(struct FilePrinter *printer);

#endif /* _PRINT_UTILS_H_ */
