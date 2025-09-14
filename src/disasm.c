#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

struct Reader {
	const char *buffer;
	unsigned int buffer_size;
	unsigned int buffer_index;
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
				print_differential_hex_byte(reader, read_next_byte(reader));
			}
			else if ((value1 & 0xC0) == 0x40) {
				print_differential_hex_word(reader, read_next_word(reader));
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

static int dump_instruction(struct Reader *reader, void (*print)(const char *), void (*print_error)(const char *)) {
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
			else if ((value0 & 0x07) == 0x05) {
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
		struct Reader *reader,
		void (*print)(const char *),
		void (*print_error)(const char *),
		const char *block_end_map) {
	
	print("\naddr");
	print_literal_hex_word(print, reader->buffer_index);
	print(":\n");

	int error_code;
	do {
		if (error_code = dump_instruction(reader, print, print_error)) {
			return error_code;
		}
	}
	while (!is_current_marked(reader, block_end_map));

	return 0;
}

static int dump(
		struct Reader *reader,
		void (*print)(const char *),
		void (*print_error)(const char *),
		const char *block_start_map,
		const char *block_end_map) {
	const int block_start_map_size = (reader->buffer_size + 7) >> 3;
	int error_code;
	for (int i = 0; i < block_start_map_size; i++) {
		const int block_map_value = block_start_map[i];
		for (int j = 0; j < 8; j++) {
			if (block_map_value & (1 << j)) {
				reader->buffer_index = i * 8 + j;
				if (error_code = dump_block(reader, print, print_error, block_end_map)) {
					return error_code;
				}
			}
		}
	}

	return 0;
}

static void print_help(const char *executedFile) {
	printf("Syntax: %s <options>\nPossible options:\n  -f or --format    Format of the input file. It can be:\n                        'bin' for plain 16bits executable without header\n                        'dos' for 16bits executable with MZ header.\n  -h or --help      Show this help.\n  -i <filename>     Uses this file as input.\n", executedFile);
}

static void dump_print(const char *str) {
	printf(str);
}

static void print_error(const char *str) {
	fprintf(stderr, str);
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

static int prepare_reader(struct Reader *reader, const char *filename, const char *format) {
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

		reader->buffer_size = file_size - header_size;

		printf("Allocating %d bytes of memory\n", reader->buffer_size);
		reader->buffer = malloc(reader->buffer_size);
		if (!reader->buffer) {
			fprintf(stderr, "Unable to allocate memory\n");
			fclose(file);
			return 1;
		}

		if (fseek(file, header_size, SEEK_SET)) {
			fprintf(stderr, "Unable to seek file after header\n");
			free(reader->buffer);
			fclose(file);
			return 1;
		}

		if (fread(reader->buffer, 1, reader->buffer_size, file) != reader->buffer_size) {
			fprintf(stderr, "Unable to read code and data from file\n");
			free(reader->buffer);
			fclose(file);
			return 1;
		}

		reader->buffer_index = header.initial_cs * 16 + header.initial_ip;
	}
	else {
		if (fseek(file, 0, SEEK_END)) {
			fprintf(stderr, "Unable to seek file to its end.\n");
			fclose(file);
			return 1;
		}

		reader->buffer_size = ftell(file);
		if (fseek(file, 0, SEEK_SET)) {
			fprintf(stderr, "Unable to seek file to its beginning.\n");
			fclose(file);
			return 1;
		}

		printf("Allocating %d bytes of memory", reader->buffer_size);
		reader->buffer = malloc(reader->buffer_size);
		if (!reader->buffer) {
			fprintf(stderr, "Unable to allocate memory\n");
			fclose(file);
			return 1;
		}

		if (fread(reader->buffer, 1, reader->buffer_size, file) != reader->buffer_size) {
			fprintf(stderr, "Unable to read code and data from file\n");
			free(reader->buffer);
			fclose(file);
			return 1;
		}

		reader->buffer_index = 0;
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

int is_current_marked(struct Reader *reader, char *block_map) {
	return block_map[reader->buffer_index >> 3] & (1 << (reader->buffer_index & 0x07));
}

static void mark_block_map(struct Reader *reader, char *block_map) {
	block_map[reader->buffer_index >> 3] |= 1 << (reader->buffer_index & 0x07);
}

static int read_block_instruction(
		struct Reader *reader,
		void (*print_error)(const char *),
		char *block_start_map,
		char *block_end_map) {
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
		else if ((value0 & 0x07) == 0x05) {
			read_next_word(reader);
			return 0;
		}
	}
	else if ((value0 & 0xE6) == 0x06 && value0 != 0x0F) {
		return 0;
	}
	else if ((value0 & 0xE7) == 0x26) {
		return read_block_instruction(reader, print_error, block_start_map, block_end_map);
	}
	else if ((value0 & 0xF0) == 0x40) {
		return 0;
	}
	else if ((value0 & 0xF0) == 0x50) {
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
		mark_block_map(reader, block_end_map);
		return 0;
	}
	else if (value0 == 0xCD) {
		const int interruption_number = read_next_byte(reader);
		if (interruption_number == 0x20) {
			mark_block_map(reader, block_end_map);
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
		struct Reader *reader,
		void (*print_error)(const char *),
		char *block_start_map,
		char *block_end_map) {
	int error_code;
	do {
		if (error_code = read_block_instruction(reader, print_error, block_start_map, block_end_map)) {
			return error_code;
		}
	} while (!is_current_marked(reader, block_end_map));

	return 0;
}

int prepare_block_maps(
		struct Reader *reader,
		void (*print_error)(const char *),
		char *block_start_map,
		char *block_end_map) {
	const int block_start_map_size = (reader->buffer_size + 7) >> 3;
	const char *checked_block_map = calloc(block_start_map_size, 1);
	if (!checked_block_map) {
		print_error("Unable to allocate memory for checked block maps\n");
		return 1;
	}

	mark_block_map(reader, block_start_map);
	int error_code;
	int next_block_ready;
	do {
		mark_block_map(reader, checked_block_map);
		if (error_code = read_block(reader, print_error, block_start_map, block_end_map)) {
			free(checked_block_map);
			return error_code;
		}

		next_block_ready = 0;
		for (int i = 0; i < block_start_map_size; i++) {
			if (block_start_map[i] != checked_block_map[i]) {
				for (int j = 0; j < 8; j++) {
					const int flag = 1 << j;
					if ((block_start_map[i] & flag) != (checked_block_map[i] & flag)) {
						reader->buffer_index = i * 8 + j;
						next_block_ready = 1;
					}
				}
			}
		}
	} while (next_block_ready);

	free(checked_block_map);
	return 0;
}

int main(int argc, const char *argv[]) {
	printf("Disassmebler\n");

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

	struct Reader reader;
	int error_code;
	if (error_code = prepare_reader(&reader, filename, format)) {
		return error_code;
	}

	const int block_start_map_size = (reader.buffer_size + 7) >> 3;
	const int block_end_map_size = (reader.buffer_size >> 3) + 1;
	char *block_start_map = calloc(block_start_map_size + block_end_map_size, 1);
	if (!block_start_map) {
		fprintf(stderr, "Unable to allocate memory for block maps\n");
		free(reader.buffer);
		return 1;
	}

	char *block_end_map = block_start_map + block_start_map_size;
	if (error_code = prepare_block_maps(&reader, print_error, block_start_map, block_end_map)) {
		free(block_start_map);
		free(reader.buffer);
		return error_code;
	}

	int result = dump(&reader, dump_print, print_error, block_start_map, block_end_map);
	free(block_start_map);
	free(reader.buffer);
	return result;
}
