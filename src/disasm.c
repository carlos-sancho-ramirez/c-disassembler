#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

struct Reader {
	const char *buffer;
	unsigned int buffer_size;
	unsigned int buffer_index;
};

struct SegmentReadResult {
	char *buffer;
	unsigned int size;
	int relative_cs;
	unsigned int ip;
};

struct CodeBlock {
	unsigned int relative_cs;
	unsigned int ip;
	const char *start;
	const char *end;
};

struct GlobalVariable {
	unsigned int *position;
	unsigned int byte_size;
};

static int read_next_byte(struct Reader *reader) {
	return reader->buffer[(reader->buffer_index)++] & 0xFF;
}

static int read_next_word(struct Reader *reader) {
	return read_next_byte(reader) + (read_next_byte(reader) << 8);
}

const char HEX_CHAR[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static void print_literal_hex_byte(void (*print)(const char *), int value) {
	char number[] = "0x00";
	number[3] = HEX_CHAR[value & 0x0F];
	number[2] = HEX_CHAR[(value >> 4) & 0x0F];
	print(number);
}

static void print_literal_hex_word(void (*print)(const char *), int value) {
	char number[] = "0x0000";
	number[5] = HEX_CHAR[value & 0x000F];
	number[4] = HEX_CHAR[(value >> 4) & 0x000F];
	number[3] = HEX_CHAR[(value >> 8) & 0x000F];
	number[2] = HEX_CHAR[(value >> 12) & 0x000F];
	print(number);
}

static void print_differential_hex_byte(void (*print)(const char *), int value) {
	char number[] = "+0x00";
	if (value & 0x80) {
		number[0] = '-';
		value = (value ^ 0xFF) + 1;
	}

	number[4] = HEX_CHAR[value & 0x0F];
	number[3] = HEX_CHAR[(value >> 4) & 0x0F];
	print(number);
}

static void print_differential_hex_word(void (*print)(const char *), int value) {
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

const char *BYTE_REGISTERS[] = {
	"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

const char *WORD_REGISTERS[] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

const char *INSTRUCTION[] = {
	"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"
};

const char *ADDRESS_REGISTERS[] = {
	"bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx"
};

const char *SEGMENT_REGISTERS[] = {
	"es", "cs", "ss", "ds"
};

const char *JUMP_INSTRUCTIONS[] = {
	"jo", "jno", "jb", "jae", "jz", "jnz", "jbe", "ja",
	"js", "jns", "jpe", "jpo", "jl", "jge", "jle", "jg"
};

static void dump_address(
		struct Reader *reader,
		void (*print)(const char *),
		int value1,
		const char *segment,
		const char **registers) {
	if ((value1 & 0xC0) != 0xC0) {
		print("[");
		if (segment) {
			print(segment);
			print(":");
		}

		if ((value1 & 0xC7) == 0x06) {
			print_literal_hex_word(print, read_next_word(reader));
		}
		else {
			print(ADDRESS_REGISTERS[value1 & 0x07]);
			if ((value1 & 0xC0) == 0x40) {
				print_differential_hex_byte(print, read_next_byte(reader));
			}
			else if ((value1 & 0xC0) == 0x40) {
				print_differential_hex_word(print, read_next_word(reader));
			}
		}
		print("]");
	}
	else {
		print(registers[value1 & 0x07]);
	}
}

static void dump_address_register_combination(
		struct Reader *reader,
		void (*print)(const char *),
		int value0,
		int value1,
		const char **registers,
		const char *segment,
		const char **addr_replacement_registers) {
	if (value0 & 0x02) {
		print(registers[(value1 >> 3) & 0x07]);
		print(",");
		dump_address(reader, print, value1, segment, addr_replacement_registers);
	}
	else {
		dump_address(reader, print, value1, segment, addr_replacement_registers);
		print(",");
		print(registers[(value1 >> 3) & 0x07]);
	}
}

static void print_address_label(void (*print)(const char *), int ip, int cs) {
	print("\naddr");
	print_literal_hex_word(print, cs);
	print("_");
	print_literal_hex_word(print, ip);
}

static int dump_instruction(
		struct Reader *reader,
		const struct CodeBlock *block,
		void (*print)(const char *),
		void (*print_error)(const char *)) {
    const char *segment = NULL;
	while (1) {
		const int value0 = read_next_byte(reader);
		if (value0 >= 0 && value0 < 0x40 && (value0 & 0x06) != 0x06) {
			print(INSTRUCTION[value0 >> 3]);
			print(" ");
			if ((value0 & 0x04) == 0x00) {
				const char **registers;
				if (value0 & 0x01) {
					registers = WORD_REGISTERS;
				}
				else {
					registers = BYTE_REGISTERS;
				}

				const int value1 = read_next_byte(reader);
				dump_address_register_combination(reader, print, value0, value1, registers, segment, registers);
				print("\n");
				return 0;
			}
			else if ((value0 & 0x07) == 0x04) {
				print(BYTE_REGISTERS[0]);
				print(",");
				print_literal_hex_byte(print, read_next_byte(reader));
				print("\n");
				return 0;
			}
			else {
				// Assuming (value0 & 0x07) == 0x05
				print(WORD_REGISTERS[0]);
				print(",");
				print_literal_hex_word(print, read_next_word(reader));
				print("\n");
				return 0;
			}
		}
		else if ((value0 & 0xE6) == 0x06 && value0 != 0x0F) {
			if (value0 & 0x01) {
				print("pop ");
			}
			else {
				print("push ");
			}
			print(SEGMENT_REGISTERS[(value0 >> 3) & 0x03]);
			print("\n");
			return 0;
		}
		else if ((value0 & 0xE7) == 0x26) {
			segment = SEGMENT_REGISTERS[(value0 >> 3) & 0x03];
		}
		else if ((value0 & 0xF0) == 0x40) {
			if (value0 & 0x08) {
				print("dec ");
			}
			else {
				print("inc ");
			}
			print(WORD_REGISTERS[value0 & 0x07]);
			print("\n");
			return 0;
		}
		else if ((value0 & 0xF0) == 0x50) {
			if (value0 & 0x08) {
				print("pop ");
			}
			else {
				print("push ");
			}
			print(WORD_REGISTERS[value0 & 0x07]);
			print("\n");
			return 0;
		}
		else if ((value0 & 0xF0) == 0x70) {
			const int value1 = read_next_byte(reader);
			print(JUMP_INSTRUCTIONS[value0 & 0x0F]);
			print(" ");
			print_address_label(print, block->ip + reader->buffer_index, block->relative_cs);
			print("\n");
			return 0;
		}
		else if ((value0 & 0xFC) == 0x88) {
			const char **registers;
			if (value0 & 1) {
				registers = WORD_REGISTERS;
			}
			else {
				registers = BYTE_REGISTERS;
			}

			const int value1 = read_next_byte(reader);
			print("mov ");
			dump_address_register_combination(reader, print, value0, value1, registers, segment, registers);
			print("\n");
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
				print("mov ");
				dump_address_register_combination(reader, print, value0, value1, SEGMENT_REGISTERS, segment, WORD_REGISTERS);
				print("\n");
				return 0;
			}
		}
		else if ((value0 & 0xFC) == 0xA0) {
			print("mov ");
			const char **registers;
			if (value0 & 1) {
				registers = WORD_REGISTERS;
			}
			else {
				registers = BYTE_REGISTERS;
			}

			if ((value0 & 0xFE) == 0xA0) {
				print(registers[0]);
				print(",[");
				if (segment) {
					print(segment);
					print(":");
				}
				print_literal_hex_word(print, read_next_word(reader));
				print("]");
			}
			else if ((value0 & 0xFE) == 0xA2) {
				print("[");
				if (segment) {
					print(segment);
					print(":");
				}
				print_literal_hex_word(print, read_next_word(reader));
				print("],");
				print(registers[0]);
			}
			print("\n");
			return 0;
		}
		else if (value0 == 0xA4) {
			print("movsb\n");
			return 0;
		}
		else if (value0 == 0xA5) {
			print("movsw\n");
			return 0;
		}
		else if (value0 == 0xA6) {
			print("cmpsb\n");
			return 0;
		}
		else if (value0 == 0xA7) {
			print("cmpsw\n");
			return 0;
		}
		else if ((value0 & 0xF0) == 0xB0) {
			print("mov ");
			if (value0 & 0x08) {
				print(WORD_REGISTERS[value0 & 0x07]);
				print(",");
				print_literal_hex_word(print, read_next_word(reader));
			}
			else {
				print(BYTE_REGISTERS[value0 & 0x07]);
				print(",");
				print_literal_hex_byte(print, read_next_byte(reader));
			}
			print("\n");
			return 0;
		}
		else if (value0 == 0xCB) {
			print("retf\n");
			return 0;
		}
		else if (value0 == 0xCD) {
			print("int ");
			print_literal_hex_byte(print, read_next_byte(reader));
			print("\n");
			return 0;
		}
		else if (value0 == 0xF2) {
			print("repne\n");
			return 0;
		}
		else if (value0 == 0xF3) {
			print("repe\n");
			return 0;
		}
		else if (value0 == 0xF8) {
			print("clc\n");
			return 0;
		}
		else if (value0 == 0xF9) {
			print("stc\n");
			return 0;
		}
		else if (value0 == 0xFA) {
			print("cli\n");
			return 0;
		}
		else if (value0 == 0xFB) {
			print("sti\n");
			return 0;
		}
		else if (value0 == 0xFC) {
			print("cld\n");
			return 0;
		}
		else if (value0 == 0xFD) {
			print("std\n");
			return 0;
		}
		else {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error("\n");
			return 1;
		}
	}
}

static int dump_block(
		const struct CodeBlock *block,
		void (*print)(const char *),
		void (*print_error)(const char *)) {
	struct Reader reader;
	reader.buffer = block->start;
	reader.buffer_index = 0;
	reader.buffer_size = block->end - block->start;

	print_address_label(print, block->ip, block->relative_cs);
	print(":\n");

	int error_code;
	do {
		if ((error_code = dump_instruction(&reader, block, print, print_error))) {
			return error_code;
		}
	}
	while (block->start + reader.buffer_index != block->end);

	return 0;
}

static int dump(
		struct CodeBlock **sorted_blocks,
		unsigned int code_block_count,
		struct GlobalVariable **global_variables,
		unsigned int global_variable_count,
		void (*print)(const char *),
		void (*print_error)(const char *)) {
	struct Reader reader;
	int error_code;

	// TODO: Global variables not printed yet
	for (int code_block_index = 0; code_block_index < code_block_count; code_block_index++) {
		if ((error_code = dump_block(sorted_blocks[code_block_index], print, print_error))) {
			return error_code;
		}
	}

	return 0;
}

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

struct CodeBlockList {
	unsigned int page_array_granularity;
	unsigned int blocks_per_page;
	struct CodeBlock **page_array;
	struct CodeBlock **sorted_blocks;
	unsigned int block_count;
};

static void initialize_code_block_list(struct CodeBlockList *list) {
	list->page_array_granularity = 8;
	list->blocks_per_page = 64;
	list->block_count = 0;
	list->page_array = NULL;
	list->sorted_blocks = NULL;
}

static int index_of_code_block_with_start(const struct CodeBlockList *list, const char *start) {
	int first = 0;
	int last = list->block_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = list->sorted_blocks[index]->start;
		if (this_start < start) {
			first = index + 1;
		}
		else if (this_start > start) {
			last = index;
		}
		else {
			return index;
		}
	}

	return -1;
}

/**
 * Returns a proper pointer to store a new block.
 * The returned pointer should be filled and call insert_sorted_code_block method in order to sort it properly.
 * This method may require allocating a new page of memory.
 * This method will return NULL in case of failure.
 */
static struct CodeBlock *prepare_new_code_block(struct CodeBlockList *list) {
	if ((list->block_count % list->blocks_per_page) == 0) {
		if ((list->block_count % (list->blocks_per_page * list->page_array_granularity)) == 0) {
			const int new_page_array_length = (list->block_count / (list->blocks_per_page * list->page_array_granularity)) + list->page_array_granularity;
			list->page_array = realloc(list->page_array, new_page_array_length * sizeof(struct CodeBlock *));
			if (!(list->page_array)) {
				return NULL;
			}

			list->sorted_blocks = realloc(list->sorted_blocks, new_page_array_length * list->blocks_per_page * sizeof(struct CodeBlock *));
			if (!(list->sorted_blocks)) {
				return NULL;
			}
		}

		struct CodeBlock *new_page = malloc(list->blocks_per_page * sizeof(struct CodeBlock));
		if (!new_page) {
			return NULL;
		}

		list->page_array[list->block_count / list->blocks_per_page] = new_page;
	}

	return list->page_array[list->block_count / list->blocks_per_page] + (list->block_count % list->blocks_per_page);
}

static int insert_sorted_code_block(struct CodeBlockList *list, struct CodeBlock *new_block) {
	int first = 0;
	int last = list->block_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = list->sorted_blocks[index]->start;
		if (this_start < new_block->start) {
			first = index + 1;
		}
		else if (this_start > new_block->start) {
			last = index;
		}
		else {
			return -1;
		}
	}

	for (int i = list->block_count; i > last; i--) {
		list->sorted_blocks[i] = list->sorted_blocks[i - 1];
	}

	list->sorted_blocks[last] = new_block;
	list->block_count++;

	return 0;
}

static void clear_code_block_list(struct CodeBlockList *list) {
	if (list->block_count > 0) {
		const int allocated_pages = (list->block_count + list->blocks_per_page - 1) / list->blocks_per_page;
		for (int i = allocated_pages - 1; i >= 0; i--) {
			free(list->page_array[i]);
		}

		free(list->page_array);
		free(list->sorted_blocks);
		list->block_count = 0;
	}
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
