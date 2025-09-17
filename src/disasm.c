#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "code_blocks.h"
#include "dumpers.h"
#include "print_utils.h"
#include "reader.h"

struct SegmentReadResult {
	char *buffer;
	unsigned int size;
	int relative_cs;
	unsigned int ip;
};

static void print_help(const char *executedFile) {
	printf("Syntax: %s <options>\nPossible options:\n  -f or --format    Format of the input file. It can be:\n                        'bin' for plain 16bits executable without header\n                        'dos' for 16bits executable with MZ header.\n  -h or --help      Show this help.\n  -i <filename>     Uses this file as input.\n", executedFile);
}

static void dump_print(const char *str) {
	printf("%s", str);
}

static void print_error(const char *str) {
	fprintf(stderr, "%s", str);
}

struct dos_header {
	uint16_t magic; // Must be "MZ"
	uint16_t bytes_in_last_page;
	uint16_t pages_count;
	uint16_t relocations_count;
	uint16_t header_paragraphs;
	uint16_t required_paragraphs;
	uint16_t desired_paragraphs;
	uint16_t initial_stack_segment;
	uint16_t initial_stack_pointer;
	uint16_t checksum;
	uint16_t initial_ip;
	uint16_t initial_cs;
	uint16_t relocation_table_offset;
};

static int read_file(struct SegmentReadResult *result, const char *filename, const char *format) {
	if (strcmp(format, "bin") && strcmp(format, "dos")) {
		fprintf(stderr, "Undefined format '%s'. It must be 'bin' or 'dos'\n", format);
		return 1;
	}

	FILE *file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Unable to open file\n");
		return 1;
	}

	if (!strcmp(format, "dos")) {
		struct dos_header header;
		// This is assuming that the processor running this is also little endian
		if (fread(&header, 1, sizeof(header), file) < sizeof(header)) {
			fprintf(stderr, "Unexpected end of file\n");
			fclose(file);
			return 1;
		}

		if (header.magic != 0x5A4D ||
				header.bytes_in_last_page >= 0x200 ||
				header.required_paragraphs > header.desired_paragraphs) {
			fprintf(stderr, "Invalid dos file\n");
			fclose(file);
			return 1;
		}

		const unsigned int header_size = header.header_paragraphs * 16;
		unsigned int file_size;
		if (header.bytes_in_last_page) {
			file_size = (header.pages_count - 1) * 512 + header.bytes_in_last_page;
		}
		else {
			file_size = header.pages_count * 512;
		}

		result->size = file_size - header_size;

		printf("Allocating %d bytes of memory\n", result->size);
		result->buffer = malloc(result->size);
		if (!result->buffer) {
			fprintf(stderr, "Unable to allocate memory\n");
			fclose(file);
			return 1;
		}

		if (fseek(file, header_size, SEEK_SET)) {
			fprintf(stderr, "Unable to seek file after header\n");
			free(result->buffer);
			fclose(file);
			return 1;
		}

		if (fread(result->buffer, 1, result->size, file) != result->size) {
			fprintf(stderr, "Unable to read code and data from file\n");
			free(result->buffer);
			fclose(file);
			return 1;
		}

		result->relative_cs = header.initial_cs;
		result->ip = header.initial_ip;
	}
	else {
		if (fseek(file, 0, SEEK_END)) {
			fprintf(stderr, "Unable to seek file to its end.\n");
			fclose(file);
			return 1;
		}

		result->size = ftell(file);
		if (fseek(file, 0, SEEK_SET)) {
			fprintf(stderr, "Unable to seek file to its beginning.\n");
			fclose(file);
			return 1;
		}

		printf("Allocating %d bytes of memory", result->size);
		result->buffer = malloc(result->size);
		if (!result->buffer) {
			fprintf(stderr, "Unable to allocate memory\n");
			fclose(file);
			return 1;
		}

		if (fread(result->buffer, 1, result->size, file) != result->size) {
			fprintf(stderr, "Unable to read code and data from file\n");
			free(result->buffer);
			fclose(file);
			return 1;
		}

		result->ip = 0x100;
		result->relative_cs = -0x10;
	}
	fclose(file);
	return 0;
}

static void read_block_instruction_address(
		struct Reader *reader,
		int value1) {
	if ((value1 & 0xC7) == 0x06 || (value1 & 0xC0) == 0x80) {
		read_next_word(reader);
	}
	else if ((value1 & 0xC0) == 0x40) {
		read_next_byte(reader);
	}
}

static int read_block_instruction(
		struct Reader *reader,
		void (*print_error)(const char *),
		struct CodeBlock *block,
		struct CodeBlockList *code_block_list,
		struct GlobalVariable *global_variables,
		struct GlobalVariable **sorted_variables,
		unsigned int global_variables_allocation_count,
		unsigned int *global_variable_count) {
	const int value0 = read_next_byte(reader);
	if (value0 >= 0 && value0 < 0x40 && (value0 & 0x06) != 0x06) {
		if ((value0 & 0x04) == 0x00) {
			const int value1 = read_next_byte(reader);
			read_block_instruction_address(reader, value1);
			return 0;
		}
		else if ((value0 & 0x07) == 0x04) {
			read_next_byte(reader);
			return 0;
		}
		else {
			// Assuming (value0 & 0x07) == 0x05
			read_next_word(reader);
			return 0;
		}
	}
	else if ((value0 & 0xE6) == 0x06 && value0 != 0x0F) {
		return 0;
	}
	else if ((value0 & 0xE7) == 0x26) {
		return read_block_instruction(reader, print_error, block, code_block_list, global_variables, sorted_variables, global_variables_allocation_count, global_variable_count);
	}
	else if ((value0 & 0xF0) == 0x40) {
		return 0;
	}
	else if ((value0 & 0xF0) == 0x50) {
		return 0;
	}
	else if ((value0 & 0xF0) == 0x70) {
		const int value1 = read_next_byte(reader);
		block->end = block->start + reader->buffer_index;

		int result;
		if ((result = index_of_code_block_with_start(code_block_list, block->start + reader->buffer_index)) < 0) {
			struct CodeBlock *new_block = prepare_new_code_block(code_block_list);
			if (!new_block) {
				return 1;
			}

			new_block->relative_cs = block->relative_cs;
			new_block->ip = block->ip + reader->buffer_index;
			new_block->start = block->start + reader->buffer_index;
			new_block->end = block->start + reader->buffer_index;
			if ((result = insert_sorted_code_block(code_block_list, new_block))) {
				return result;
			}
		}

		const int diff = (value1 >= 0x80)? -value1 : value1;
		if ((result = index_of_code_block_with_start(code_block_list, block->start + reader->buffer_index + diff)) < 0) {
			struct CodeBlock *new_block = prepare_new_code_block(code_block_list);
			if (!new_block) {
				return 1;
			}

			new_block->relative_cs = block->relative_cs;
			new_block->ip = block->ip + reader->buffer_index + diff;
			new_block->start = block->start + reader->buffer_index + diff;
			new_block->end = block->start + reader->buffer_index + diff;
			if ((result = insert_sorted_code_block(code_block_list, new_block))) {
				return result;
			}
		}

		return 0;
	}
	else if ((value0 & 0xFC) == 0x88) {
		const int value1 = read_next_byte(reader);
		read_block_instruction_address(reader, value1);
		return 0;
	}
	else if ((value0 & 0xFD) == 0x8C) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x20) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			read_block_instruction_address(reader, value1);
			return 0;
		}
	}
	else if ((value0 & 0xFC) == 0xA0) {
		if ((value0 & 0xFE) == 0xA0) {
			read_next_word(reader);
		}
		else if ((value0 & 0xFE) == 0xA2) {
			read_next_word(reader);
		}
		return 0;
	}
	else if ((value0 & 0xFC) == 0xA4) {
		return 0;
	}
	else if ((value0 & 0xF0) == 0xB0) {
		if (value0 & 0x08) {
			read_next_word(reader);
		}
		else {
			read_next_byte(reader);
		}
		return 0;
	}
	else if (value0 == 0xCB) {
		block->end = block->start + reader->buffer_index;
		return 0;
	}
	else if (value0 == 0xCD) {
		const int interruption_number = read_next_byte(reader);
		if (interruption_number == 0x20) {
			block->end = block->start + reader->buffer_index;
		}
		return 0;
	}
	else if (value0 == 0xF2) {
		return 0;
	}
	else if (value0 == 0xF3) {
		return 0;
	}
	else if (value0 == 0xF8) {
		return 0;
	}
	else if (value0 == 0xF9) {
		return 0;
	}
	else if (value0 == 0xFA) {
		return 0;
	}
	else if (value0 == 0xFB) {
		return 0;
	}
	else if (value0 == 0xFC) {
		return 0;
	}
	else if (value0 == 0xFD) {
		return 0;
	}
	else {
		print_error("Unknown opcode ");
		print_literal_hex_byte(print_error, value0);
		print_error("\n");
		return 1;
	}
}

int read_block(
		void (*print_error)(const char *),
		struct CodeBlock *block,
		struct CodeBlockList *code_block_list,
		struct GlobalVariable *global_variables,
		struct GlobalVariable **sorted_variables,
		unsigned int global_variables_allocation_count,
		unsigned int *global_variable_count) {
	struct Reader reader;
	reader.buffer = block->start;
	reader.buffer_index = 0;
	int error_code;
	do {
		if ((error_code = read_block_instruction(&reader, print_error, block, code_block_list, global_variables, sorted_variables, global_variables_allocation_count, global_variable_count))) {
			return error_code;
		}
	} while (block->end != (block->start + reader.buffer_index));

	return 0;
}

int find_code_blocks_and_variables(
		struct SegmentReadResult *read_result,
		void (*print_error)(const char *),
		struct CodeBlockList *code_block_list,
		struct GlobalVariable *global_variables,
		struct GlobalVariable **sorted_variables,
		unsigned int global_variables_allocation_count,
		unsigned int *global_variable_count) {
	struct CodeBlock *first_block = prepare_new_code_block(code_block_list);
	if (!first_block) {
		return 1;
	}

	first_block->ip = read_result->ip;
	first_block->relative_cs = read_result->relative_cs;
	first_block->start = read_result->buffer + (read_result->relative_cs * 16 + read_result->ip);
	first_block->end = first_block->start;

	int error_code;
	if (insert_sorted_code_block(code_block_list, first_block)) {
		return error_code;
	}

	*global_variable_count = 0;
	for (int block_index = 0; block_index < code_block_list->block_count; block_index++) {
		struct CodeBlock *block = code_block_list->page_array[block_index / code_block_list->blocks_per_page] + (block_index % code_block_list->blocks_per_page);
		if ((error_code = read_block(print_error, block, code_block_list, global_variables, sorted_variables, global_variables_allocation_count, global_variable_count))) {
			return error_code;
		}
	}

	return 0;
}

int main(int argc, const char *argv[]) {
	printf("Disassembler\n");

	const char *filename = NULL;
	const char *format = NULL;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--format")) {
			if (++i < argc) {
				format = argv[i];
			}
			else {
				fprintf(stderr, "Missing format after %s argument\n", argv[i - 1]);
				print_help(argv[0]);
				return 1;
			}
		}
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_help(argv[0]);
			return 0;
		}
		else if (!strcmp(argv[i], "-i")) {
			if (++i < argc) {
				filename = argv[i];
			}
			else {
				fprintf(stderr, "Missing file name after %s argument\n", argv[i - 1]);
				print_help(argv[0]);
				return 1;
			}
		}
		else {
			fprintf(stderr, "Unexpected argument %s\n", argv[i]);
			print_help(argv[0]);
			return 1;
		}
	}

	if (!filename) {
		fprintf(stderr, "Argument -i is required\n");
		print_help(argv[0]);
		return 1;
	}

	if (!format) {
		fprintf(stderr, "Argument -f or --format is required\n");
		print_help(argv[0]);
		return 1;
	}

	struct SegmentReadResult read_result;
	int error_code;
	if ((error_code = read_file(&read_result, filename, format))) {
		return error_code;
	}

	unsigned int global_variables_allocation_count = read_result.size >> 3;
	struct GlobalVariable *global_variables = malloc(global_variables_allocation_count * sizeof(struct GlobalVariable));
	if (!global_variables) {
		fprintf(stderr, "Unable to allocate memory for global variables\n");
		error_code = 1;
		goto end0;
	}

	struct GlobalVariable **sorted_variables = malloc(global_variables_allocation_count * sizeof(struct GlobalVariable *));
	if (!sorted_variables) {
		fprintf(stderr, "Unable to allocate memory for global variable indexes\n");
		error_code = 1;
		goto end1;
	}

	struct CodeBlockList code_block_list;
	initialize_code_block_list(&code_block_list);

	unsigned int global_variable_count;
	if ((error_code = find_code_blocks_and_variables(&read_result, print_error, &code_block_list, global_variables, sorted_variables, global_variables_allocation_count, &global_variable_count))) {
		goto end;
	}

	error_code = dump(code_block_list.sorted_blocks, code_block_list.block_count, sorted_variables, global_variable_count, dump_print, print_error);

	end:
	free(sorted_variables);

	end1:
	free(global_variables);

	end0:
	clear_code_block_list(&code_block_list);
	free(read_result.buffer);
	return error_code;
}
