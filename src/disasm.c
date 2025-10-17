#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "code_blocks.h"
#include "dumpers.h"
#include "print_utils.h"
#include "reader.h"
#include "registers.h"
#include "relocations.h"
#include "stack.h"
#include "interruption_table.h"
#include "segments.h"
#include "version.h"

#define SEGMENT_READ_RESULT_FLAG_DS_MATCHES_CS_AT_START 1

struct SegmentReadResult {
	struct FarPointer *relocation_table;
	unsigned int relocation_count;
	const char **sorted_relocations;
	char *buffer;
	unsigned int size;
	int relative_cs;
	unsigned int ip;
	unsigned int flags;

	void (*print_code_label)(void (*print)(const char *), int ip, int cs);
	void (*print_variable_label)(void (*print)(const char *), unsigned int address);
};

static int ds_should_match_cs_at_segment_start(struct SegmentReadResult *result) {
	return result->flags & SEGMENT_READ_RESULT_FLAG_DS_MATCHES_CS_AT_START;
}

static void print_help(const char *executedFile) {
	printf("Syntax: %s <options>\nPossible options:\n  -f or --format    Format of the input file. It can be:\n                        'bin' for plain 16bits executable without header\n                        'dos' for 16bits executable with MZ header.\n  -h or --help      Show this help.\n  -i <filename>     Uses this file as input.\n  -o <filename>     Uses this file as output.\n                    If not defined, the result will be printed in the standard output.\n", executedFile);
}

static FILE *print_output_file;

static void print_output(const char *str) {
	fprintf(print_output_file, "%s", str);
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

		result->relocation_count = header.relocations_count;
		if (header.relocations_count > 0) {
			result->relocation_table = malloc(sizeof(struct FarPointer) * header.relocations_count);
			if (!result->relocation_table) {
				fprintf(stderr, "Unable to allocate memory for relocation table\n");
				fclose(file);
				return 1;
			}

			if (fseek(file, header.relocation_table_offset, SEEK_SET)) {
				fprintf(stderr, "Unable to seek file at %d for relocation table\n", header.relocation_table_offset);
				free(result->relocation_table);
				fclose(file);
				return 1;
			}

			if (fread(result->relocation_table, sizeof(struct FarPointer), header.relocations_count, file) != header.relocations_count) {
				fprintf(stderr, "Unable to read relocation table from file\n");
				free(result->relocation_table);
				fclose(file);
				return 1;
			}
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

		result->buffer = malloc(result->size);
		if (!result->buffer) {
			fprintf(stderr, "Unable to allocate memory\n");
			fclose(file);
			if (result->relocation_count) {
				free(result->relocation_table);
			}
			return 1;
		}

		if (fseek(file, header_size, SEEK_SET)) {
			fprintf(stderr, "Unable to seek file after header\n");
			free(result->buffer);
			fclose(file);
			if (result->relocation_count) {
				free(result->relocation_table);
			}
			return 1;
		}

		if (fread(result->buffer, 1, result->size, file) != result->size) {
			fprintf(stderr, "Unable to read code and data from file\n");
			free(result->buffer);
			fclose(file);
			if (result->relocation_count) {
				free(result->relocation_table);
			}
			return 1;
		}

		result->relative_cs = header.initial_cs;
		result->ip = header.initial_ip;
		result->print_code_label = print_dos_address_label;
		result->print_variable_label = print_dos_variable_label;
		result->flags = 0;

		if (header.relocations_count > 0) {
			result->sorted_relocations = malloc(sizeof(const char *) * header.relocations_count);
			if (!result->sorted_relocations) {
				fprintf(stderr, "Unable to allocate memory for the sorted relocations\n");
				free(result->buffer);
				free(result->relocation_table);
				fclose(file);
				return 1;
			}

			for (int i = 0; i < header.relocations_count; i++) {
				struct FarPointer *ptr = result->relocation_table + i;
				const char *relocation = result->buffer + ptr->segment * 16 + ptr->offset;

				int first = 0;
				int last = i;
				while (last > first) {
					int index = (first + last) / 2;
					const char *this_relocation = result->sorted_relocations[index];
					if (this_relocation < relocation) {
						first = index + 1;
					}
					else if (this_relocation > relocation) {
						last = index;
					}
					else {
						first = index;
						last = index;
					}
				}

				for (unsigned int j = header.relocations_count; j > first; j--) {
					result->sorted_relocations[j] = result->sorted_relocations[j - 1];
				}

				result->sorted_relocations[first] = relocation;
			}
		}
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

		result->relocation_count = 0;
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
		result->print_code_label = print_bin_address_label;
		result->print_variable_label = print_bin_variable_label;
		result->flags = SEGMENT_READ_RESULT_FLAG_DS_MATCHES_CS_AT_START;
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

#define SEGMENT_INDEX_UNDEFINED -1
#define SEGMENT_INDEX_SS 2
#define SEGMENT_INDEX_DS 3

static int read_block_instruction_internal(
		struct Reader *reader,
		struct Registers *regs,
		struct Stack *stack,
		struct InterruptionTable *int_table,
		const char *segment_start,
		const char **sorted_relocations,
		unsigned int relocation_count,
		void (*print_error)(const char *),
		struct CodeBlock *block,
		struct CodeBlockList *code_block_list,
		struct GlobalVariableList *global_variable_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list,
		int segment_index,
		const char *opcode_reference) {
	int error_code;
	const int value0 = read_next_byte(reader);
	if (value0 >= 0 && value0 < 0x40 && (value0 & 0x06) != 0x06) {
		if ((value0 & 0x04) == 0x00) {
			const int value1 = read_next_byte(reader);
			if ((value1 & 0xC0) == 0 && (value1 & 0x07) == 6) {
				int result_address = read_next_word(reader);
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_global_variable_reference(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x40) {
				const int raw_value = read_next_byte(reader);
			}
			else if ((value1 & 0xC0) == 0x80) {
				const int raw_value = read_next_word(reader);
			}
			else if (value1 >= 0xC0) {
				if ((value0 & 0x38) == 0x30 && (((value1 >> 3) & 0x07) == (value1 & 0x07))) { // XOR
					if (value0 & 1) {
						set_word_register(regs, value1 & 0x07, opcode_reference, 0);
					}
					else {
						set_byte_register(regs, value1 & 0x07, opcode_reference, 0);
					}
				}
			}

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
		const unsigned int sindex = (value0 >> 3) & 3;
		if (value0 & 1) {
			if (stack_is_empty(stack)) {
				mark_segment_register_undefined(regs, sindex);
			}
			else if (top_is_defined_and_relative_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_segment_register_relative(regs, sindex, opcode_reference, stack_value);
			}
			else if (top_is_defined_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_segment_register(regs, sindex, opcode_reference, stack_value);
			}
			else {
				pop_from_stack(stack);
				mark_segment_register_undefined(regs, sindex);
			}
		}
		else {
			if (is_segment_register_defined_and_relative(regs, sindex)) {
				push_relative_in_stack(stack, get_segment_register(regs, sindex));
			}
			else if (is_segment_register_defined(regs, sindex)) {
				push_in_stack(stack, get_segment_register(regs, sindex));
			}
			else {
				push_undefined_in_stack(stack);
			}
		}

		return 0;
	}
	else if ((value0 & 0xE7) == 0x26) {
		return read_block_instruction_internal(reader, regs, stack, int_table, segment_start, sorted_relocations, relocation_count, print_error, block, code_block_list, global_variable_list, segment_start_list, reference_list, (value0 >> 3) & 0x03, opcode_reference);
	}
	else if ((value0 & 0xF0) == 0x40) {
		return 0;
	}
	else if ((value0 & 0xF0) == 0x50) {
		const unsigned int rindex = value0 & 7;
		if (value0 & 0x08) {
			if (stack_is_empty(stack)) {
				mark_word_register_undefined(regs, rindex);
			}
			else if (top_is_defined_and_relative_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_word_register_relative(regs, rindex, opcode_reference, stack_value);
			}
			else if (top_is_defined_in_stack(stack)) {
				uint16_t stack_value = pop_from_stack(stack);
				set_word_register(regs, rindex, opcode_reference, stack_value);
			}
			else {
				pop_from_stack(stack);
				mark_word_register_undefined(regs, rindex);
			}
		}
		else {
			if (is_word_register_defined_and_relative(regs, rindex)) {
				push_relative_in_stack(stack, get_word_register(regs, rindex));
			}
			else if (is_word_register_defined(regs, rindex)) {
				push_in_stack(stack, get_word_register(regs, rindex));
			}
			else {
				push_undefined_in_stack(stack);
			}
		}

		return 0;
	}
	else if ((value0 & 0xF0) == 0x70 || (value0 & 0xFC) == 0xE0) {
		const int value1 = read_next_byte(reader);
		const int diff = (value1 >= 0x80)? value1 - 0x100 : value1;
		const char *jump_destination = block->start + reader->buffer_index + diff;
		int result = index_of_code_block_containing_position(code_block_list, jump_destination);
		struct CodeBlock *potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
		if (!potential_container || potential_container->start != jump_destination) {
			struct CodeBlock *new_block = prepare_new_code_block(code_block_list);
			if (!new_block) {
				return 1;
			}

			new_block->relative_cs = block->relative_cs;
			new_block->ip = block->ip + reader->buffer_index + diff;
			new_block->start = jump_destination;
			new_block->end = jump_destination;
			new_block->flags = 0;
			initialize_code_block_origin_list(&new_block->origin_list);
			if ((result = add_jump_type_code_block_origin(new_block, opcode_reference, regs))) {
				return result;
			}

			if ((result = insert_sorted_code_block(code_block_list, new_block))) {
				return result;
			}

			if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
				potential_container->end = jump_destination;
				invalidate_code_block_check(potential_container);
			}
		}

		return 0;
	}
	else if ((value0 & 0xFE) == 0x80) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0xC0) == 0 && (value1 & 0x07) == 6) {
			int result_address = read_next_word(reader);
			if (segment_index == SEGMENT_INDEX_UNDEFINED) {
				segment_index = SEGMENT_INDEX_DS;
			}

			if ((error_code = add_global_variable_reference(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
				return error_code;
			}
		}
		else if ((value1 & 0xC0) == 0x40) {
			read_next_byte(reader);
		}
		else if ((value1 & 0xC0) == 0x80) {
			read_next_word(reader);
		}

		if (value0 & 1) {
			read_next_word(reader);
		}
		else {
			read_next_byte(reader);
		}

		return 0;
	}
	else if (value0 == 0x83) {
		const int value1 = read_next_byte(reader);
		read_block_instruction_address(reader, value1);
		read_next_byte(reader);
		return 0;
	}
	else if ((value0 & 0xFE) == 0x86 || (value0 & 0xFC) == 0x88) {
		const int value1 = read_next_byte(reader);

		if ((value1 & 0xC7) == 6) {
			int result_address = read_next_word(reader);
			if (segment_index == SEGMENT_INDEX_UNDEFINED) {
				segment_index = SEGMENT_INDEX_DS;
			}

			if ((error_code = add_global_variable_reference(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
				return error_code;
			}
		}
		else if ((value1 & 0xC0) == 0x80) {
			read_next_word(reader);
		}
		else if ((value1 & 0xC0) == 0x40) {
			read_next_byte(reader);
		}

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
			if ((value1 & 0xC7) == 6) {
				int result_address = read_next_word(reader);
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_global_variable_reference(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, 1, opcode_reference))) {
					return error_code;
				}

				if (value0 & 2) {
					mark_segment_register_undefined(regs, (value1 >> 3) & 0x03);
				}
			}
			else if ((value1 & 0xC0) == 0x40) {
				const int raw_value = read_next_byte(reader);
			}
			else if ((value1 & 0xC0) == 0x80) {
				const int raw_value = read_next_word(reader);
			}
			else if (value1 >= 0xC0) {
				const int rm = value1 & 0x07;
				const int index = (value1 >> 3) & 0x03;
				if ((value0 & 2) && is_word_register_defined(regs, rm)) {
					const uint16_t value = get_word_register(regs, rm);
					if (is_word_register_defined_and_relative(regs, rm)) {
						set_segment_register_relative(regs, index, opcode_reference, value);
					}
					else {
						set_segment_register(regs, index, opcode_reference, value);
					}
				}
				else if ((value0 & 2) == 0 && is_segment_register_defined(regs, index)) {
					const uint16_t value = get_segment_register(regs, index);
					if (is_segment_register_defined_and_relative(regs, index)) {
						set_word_register_relative(regs, rm, opcode_reference, value);
					}
					else {
						set_word_register(regs, rm, opcode_reference, value);
					}
				}
			}

			return 0;
		}
	}
	else if (value0 == 0x8D) {
		const int value1 = read_next_byte(reader);
		if (value1 >= 0xC0) {
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
	else if (value0 == 0x8F) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x38 || value1 >= 0xC0) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			if (!stack_is_empty(stack)) {
				pop_from_stack(stack);
			}

			if ((value1 & 0xC7) == 0x06) {
				int result_address = read_next_word(reader);
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_global_variable_reference(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
			}

			return 0;
		}
	}
	else if ((value0 & 0xF8) == 0x90) { // xchg
		return 0;
	}
	else if ((value0 & 0xFC) == 0xA0) {
		if ((value0 & 0xFE) == 0xA0) {
			if (value0 & 1) {
				set_register_ax_undefined(regs);
			}
			else {
				set_register_al_undefined(regs);
			}
		}

		const int offset = read_next_word(reader);
		const unsigned int current_segment_index = (segment_index >= 0)? segment_index : SEGMENT_INDEX_DS;
		if (value0 == 0xA3 && is_register_ax_defined(regs) && is_segment_register_defined_and_absolute(regs, current_segment_index)) {
			unsigned int addr = get_segment_register(regs, current_segment_index);
			addr = addr * 16 + offset;
			if ((offset & 1) == 0 && addr < 0x400) {
				const uint16_t value = get_register_ax(regs);
				const char *where = where_register_ax_defined(regs);
				if ((addr & 2) == 0) {
					set_interruption_table_offset(int_table, addr >> 2, where, value);
				}
				else if (is_register_ax_defined_and_relative(regs)) {
					set_interruption_table_segment_relative(int_table, addr >> 2, where, value);
				}
				else {
					set_interruption_table_segment(int_table, addr >> 2, where, value);
				}
			}
		}

		if (segment_index >= 0 && is_segment_register_defined_and_relative(regs, segment_index) || segment_index == SEGMENT_INDEX_UNDEFINED && is_register_ds_defined_and_relative(regs)) {
			unsigned int segment_value = (segment_index == SEGMENT_INDEX_UNDEFINED)? get_register_ds(regs) : get_segment_register(regs, segment_index);
			unsigned int relative_address = (segment_value * 16 + offset) & 0xFFFF;
			const char *target = segment_start + relative_address;
			const int var_index = index_of_global_variable_with_start(global_variable_list, target);
			struct GlobalVariable *var;
			if (var_index < 0) {
				var = prepare_new_global_variable(global_variable_list);
				var->start = target;
				var->end = target + 2;
				var->relative_address = relative_address;
				var->var_type = (value0 & 1)? GLOBAL_VARIABLE_TYPE_WORD : GLOBAL_VARIABLE_TYPE_BYTE;

				insert_sorted_global_variable(global_variable_list, var);
			}
			else {
				var = global_variable_list->sorted_variables[var_index];
			}

			if (index_of_reference_with_instruction(reference_list, opcode_reference) < 0) {
				struct Reference *new_ref = prepare_new_reference(reference_list);
				new_ref->instruction = opcode_reference;
				set_global_variable_reference_from_instruction_address(new_ref, var);
				new_ref->flags |= (value0 & 2)? REFERENCE_FLAG_ACCESS_WRITE : REFERENCE_FLAG_ACCESS_READ;
				insert_sorted_reference(reference_list, new_ref);
			}
		}

		return 0;
	}
	else if ((value0 & 0xFC) == 0xA4) {
		return 0;
	}
	else if (value0 == 0xA8) {
		read_next_byte(reader);
		return 0;
	}
	else if (value0 == 0xA9) {
		read_next_word(reader);
		return 0;
	}
	else if ((value0 & 0xFE) == 0xAA) {
		return 0;
	}
	else if ((value0 & 0xFC) == 0xAC) {
		return 0;
	}
	else if ((value0 & 0xF0) == 0xB0) {
		if (value0 & 0x08) {
			const char *relocation_query = reader->buffer + reader->buffer_index;
			int word_value = read_next_word(reader);
			if (is_relocation_present_in_sorted_relocations(sorted_relocations, relocation_count, relocation_query)) {
				set_word_register_relative(regs, value0 & 0x07, opcode_reference, word_value);
			}
			else {
				set_word_register(regs, value0 & 0x07, opcode_reference, word_value);
			}
		}
		else {
			set_byte_register(regs, value0 & 0x07, opcode_reference, read_next_byte(reader));
		}
		return 0;
	}
	else if (value0 == 0xC2) {
		read_next_word(reader);
		block->end = block->start + reader->buffer_index;
		return 0;
	}
	else if (value0 == 0xC3) {
		block->end = block->start + reader->buffer_index;
		return 0;
	}
	else if ((value0 & 0xFE) == 0xC4) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0xC0) == 0xC0) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			if ((value1 & 0xC7) == 0x06) {
				int result_address = read_next_word(reader);
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_far_pointer_global_variable_reference(global_variable_list, reference_list, regs, segment_index, result_address, segment_start, opcode_reference))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
			}

			return 0;
		}
	}
	else if ((value0 & 0xFE) == 0xC6) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x38) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			if (value1 == 0x06) {
				int result_address = read_next_word(reader);
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_global_variable_reference(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
			}

			if (value0 & 1) {
				read_next_word(reader);
			}
			else {
				read_next_byte(reader);
			}
			return 0;
		}
	}
	else if (value0 == 0xCD) {
		const int interruption_number = read_next_byte(reader);
		if (interruption_number == 0x20) {
			block->end = block->start + reader->buffer_index;
		}
		else if (interruption_number == 0x21 && is_register_ah_defined(regs)) {
			const unsigned int ah_value = get_register_ah(regs);
			if (ah_value == 0x09 && is_register_ds_defined_and_relative(regs) && is_register_dx_defined(regs)) {
				unsigned int relative_address = (get_register_ds(regs) * 16 + get_register_dx(regs)) & 0xFFFF;
				const char *target = segment_start + relative_address;
				struct GlobalVariable *var;
				int index = index_of_global_variable_with_start(global_variable_list, target);
				if (index < 0) {
					var = prepare_new_global_variable(global_variable_list);
					var->start = target;
					var->end = target;
					var->relative_address = relative_address;
					var->var_type = GLOBAL_VARIABLE_TYPE_DOLLAR_TERMINATED_STRING;
					insert_sorted_global_variable(global_variable_list, var);
				}
				else {
					var = global_variable_list->sorted_variables[index];
				}
				// What should we do if the variable is present, but its type does not match? Not sure.

				const char *instruction = where_register_dx_defined(regs);
				if ((((unsigned int) *instruction) & 0xFF) == 0xBA) {
					if (index_of_reference_with_instruction(reference_list, instruction) < 0) {
						struct Reference *new_ref = prepare_new_reference(reference_list);
						new_ref->instruction = instruction;
						set_global_variable_reference_from_instruction_immediate_value(new_ref, var);
						insert_sorted_reference(reference_list, new_ref);
					}
				}
			}
			else if (ah_value == 0x25 && is_register_ds_defined_and_relative(regs) && is_register_dx_defined_and_absolute(regs)) { // Set interruption vector. DS:DX point to the handler code
				uint16_t target_relative_cs = get_register_ds(regs);
				uint16_t target_ip = get_register_dx(regs);
				unsigned int addr = target_relative_cs;
				addr = (addr * 16 + target_ip) & 0xFFFFF;

				const char *jump_destination = segment_start + addr;
				int result = index_of_code_block_containing_position(code_block_list, jump_destination);
				struct CodeBlock *potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
				struct CodeBlock *target_block;
				if (potential_container && potential_container->start == jump_destination) {
					target_block = potential_container;
				}
				else {
					target_block = prepare_new_code_block(code_block_list);
					if (!target_block) {
						return 1;
					}

					target_block->relative_cs = target_relative_cs;
					target_block->ip = target_ip;
					target_block->start = jump_destination;
					target_block->end = jump_destination;
					target_block->flags = 0;
					initialize_code_block_origin_list(&target_block->origin_list);
					if ((result = add_interruption_type_code_block_origin(target_block, regs))) {
						return result;
					}

					if ((result = insert_sorted_code_block(code_block_list, target_block))) {
						return result;
					}

					if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
						potential_container->end = jump_destination;
						invalidate_code_block_check(potential_container);
					}
				}

				const char *instruction = where_register_dx_defined(regs);
				if ((((unsigned int) *instruction) & 0xFF) == 0xBA) {
					if (index_of_reference_with_instruction(reference_list, instruction) < 0) {
						struct Reference *new_ref = prepare_new_reference(reference_list);
						new_ref->instruction = instruction;
						set_code_block_reference_from_instruction_immediate_value(new_ref, target_block);
						insert_sorted_reference(reference_list, new_ref);
					}
				}
			}
			else if (ah_value == 0x30) { // Get DOS version number
				mark_register_ax_undefined(regs);
				mark_register_cx_undefined(regs);
				mark_register_bx_undefined(regs);
			}
			else if (ah_value == 0x40) { // Write to file using handle
				unsigned int length;
				if (is_register_ds_defined_and_relative(regs) && is_register_dx_defined_and_absolute(regs) && is_register_cx_defined_and_absolute(regs) && (length = get_register_cx(regs)) > 0) {
					int error_code;
					unsigned int segment_value = get_register_ds(regs);
					unsigned int relative_address = (segment_value * 16 + get_register_dx(regs)) & 0xFFFF;
					const char *target = segment_start + relative_address;
					if (index_of_global_variable_with_start(global_variable_list, target) < 0) {
						struct GlobalVariable *var = prepare_new_global_variable(global_variable_list);
						var->start = target;
						var->relative_address = relative_address;
						var->end = target + length;
						var->var_type = GLOBAL_VARIABLE_TYPE_STRING;
			
						if ((error_code = insert_sorted_global_variable(global_variable_list, var))) {
							return error_code;
						}
					}
			
					if (segment_value && segment_value != 0xFFF0) {
						const char *target_segment_start = segment_start + segment_value * 16;
						if (!contains_segment_start(segment_start_list, target_segment_start)) {
							if ((error_code = insert_segment_start(segment_start_list, target_segment_start))) {
								return error_code;
							}
						}
					}
				}
				mark_register_ax_undefined(regs);
			}
			else if (ah_value == 0x4C) {
				block->end = block->start + reader->buffer_index;
			}
		}
		return 0;
	}
	else if ((value0 & 0xFC) == 0xD0) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0x38) == 0x30) {
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
	else if ((value0 & 0xFE) == 0xE8) {
		int diff = read_next_word(reader);
		if (block->ip + reader->buffer_index + diff >= 0x10000) {
			diff -= 0x10000;
		}

		const char *jump_destination = block->start + reader->buffer_index + diff;
		int result = index_of_code_block_containing_position(code_block_list, jump_destination);
		struct CodeBlock *potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
		if (!potential_container || potential_container->start != jump_destination) {
			struct CodeBlock *new_block = prepare_new_code_block(code_block_list);
			if (!new_block) {
				return 1;
			}

			new_block->relative_cs = block->relative_cs;
			new_block->ip = block->ip + reader->buffer_index + diff;
			new_block->start = jump_destination;
			new_block->end = jump_destination;
			new_block->flags = 0;
			initialize_code_block_origin_list(&new_block->origin_list);
			if ((result = add_jump_type_code_block_origin(new_block, opcode_reference, regs))) {
				return result;
			}

			if ((result = insert_sorted_code_block(code_block_list, new_block))) {
				return result;
			}

			if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
				potential_container->end = jump_destination;
				invalidate_code_block_check(potential_container);
			}
		}

		if (value0 & 1) {
			block->end = block->start + reader->buffer_index;
		}
		else {
			make_all_registers_undefined_except_cs(regs);
		}

		return 0;
	}
	else if (value0 == 0xEA) {
		read_next_word(reader);
		read_next_word(reader);
		block->end = block->start + reader->buffer_index;
		return 0;
	}
	else if (value0 == 0xEB) {
		const int value1 = read_next_byte(reader);
		const int diff = (value1 >= 0x80)? value1 - 0x100 : value1;
		const char *jump_destination = block->start + reader->buffer_index + diff;
		int result = index_of_code_block_containing_position(code_block_list, jump_destination);
		struct CodeBlock *potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
		if (!potential_container || potential_container->start != jump_destination) {
			struct CodeBlock *new_block = prepare_new_code_block(code_block_list);
			if (!new_block) {
				return 1;
			}

			new_block->relative_cs = block->relative_cs;
			new_block->ip = block->ip + reader->buffer_index + diff;
			new_block->start = jump_destination;
			new_block->end = jump_destination;
			new_block->flags = 0;
			initialize_code_block_origin_list(&new_block->origin_list);
			if ((result = add_jump_type_code_block_origin(new_block, opcode_reference, regs))) {
				return result;
			}

			if ((result = insert_sorted_code_block(code_block_list, new_block))) {
				return result;
			}

			if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
				potential_container->end = jump_destination;
				invalidate_code_block_check(potential_container);
			}
		}

		return 0;
	}
	else if (value0 == 0xF2) {
		return 0;
	}
	else if (value0 == 0xF3) {
		return 0;
	}
	else if ((value0 & 0xFE) == 0xF6) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0x38) == 0x08) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			read_block_instruction_address(reader, value1);
			if ((value1 & 0x38) == 0) {
				if (value0 & 1) {
					read_next_word(reader);
				}
				else {
					read_next_byte(reader);
				}
			}
			return 0;
		}
	}
	else if (value0 == 0xFB) { // sti
		for (int i = 0; i < 256; i++) {
			if (is_interruption_defined_and_relative_in_table(int_table, i)) {
				uint16_t target_relative_cs = get_interruption_table_relative_segment(int_table, i);
				uint16_t target_ip = get_interruption_table_offset(int_table, i);
				unsigned int addr = target_relative_cs;
				addr = (addr * 16 + target_ip) & 0xFFFFF;

				const char *jump_destination = segment_start + addr;
				int result = index_of_code_block_containing_position(code_block_list, jump_destination);
				struct CodeBlock *potential_container = (result < 0)? NULL : code_block_list->sorted_blocks[result];
				struct CodeBlock *target_block;
				if (potential_container && potential_container->start == jump_destination) {
					target_block = potential_container;
				}
				else {
					if (potential_container && potential_container->start != potential_container->end && potential_container->end > jump_destination) {
						potential_container->end = jump_destination;
					}

					target_block = prepare_new_code_block(code_block_list);
					if (!target_block) {
						return 1;
					}

					target_block->relative_cs = target_relative_cs;
					target_block->ip = target_ip;
					target_block->start = jump_destination;
					target_block->end = jump_destination;
					target_block->flags = 0;
					initialize_code_block_origin_list(&target_block->origin_list);

					struct Registers int_regs;
					make_all_registers_undefined(&int_regs);
					set_register_cs_relative(&int_regs, where_interruption_segment_defined_in_table(int_table, i), target_relative_cs);
					if ((result = add_interruption_type_code_block_origin(target_block, &int_regs))) {
						return result;
					}

					if ((result = insert_sorted_code_block(code_block_list, target_block))) {
						return result;
					}
				}

				const char *where_offset = where_interruption_offset_defined_in_table(int_table, i);
				if (where_offset && where_offset != REGISTER_DEFINED_OUTSIDE && (((unsigned int) *where_offset) & 0xF8) == 0xB8) {
					if (index_of_reference_with_instruction(reference_list, where_offset) < 0) {
						struct Reference *new_ref = prepare_new_reference(reference_list);
						new_ref->instruction = where_offset;
						set_code_block_reference_from_instruction_immediate_value(new_ref, target_block);
						insert_sorted_reference(reference_list, new_ref);
					}
				}
			}
		}

		return 0;
	}
	else if ((value0 & 0xFC) == 0xF8 || (value0 & 0xFE) == 0xFC) {
		return 0;
	}
	else if (value0 == 0xFE) {
		const int value1 = read_next_byte(reader);
		if (value1 & 0x30) {
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
	else if (value0 == 0xFF) {
		const int value1 = read_next_byte(reader);
		if ((value1 & 0x38) == 0x38 || (value1 & 0xF8) == 0xD8 || (value1 & 0xF8) == 0xE8) {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error(" ");
			print_literal_hex_byte(print_error, value1);
			print_error("\n");
			return 1;
		}
		else {
			if ((value1 & 0xC7) == 0x06) {
				int result_address = read_next_word(reader);
				if (segment_index == SEGMENT_INDEX_UNDEFINED) {
					segment_index = SEGMENT_INDEX_DS;
				}

				if ((error_code = add_global_variable_reference(global_variable_list, segment_start_list, reference_list, regs, segment_index, result_address, segment_start, value0, opcode_reference))) {
					return error_code;
				}
			}
			else if ((value1 & 0xC0) == 0x80) {
				read_next_word(reader);
			}
			else if ((value1 & 0xC0) == 0x40) {
				read_next_byte(reader);
			}

			if ((value1 & 0x30) == 0x20) {
				block->end = block->start + reader->buffer_index;
			}
			return 0;
		}
	}
	else {
		const int this_block_index = index_of_code_block_with_start(code_block_list, block->start);
		const char *new_end = reader->buffer + reader->buffer_size;
		if (this_block_index + 1 < code_block_list->block_count) {
			const char *next_start = code_block_list->sorted_blocks[this_block_index + 1]->start;
			if (next_start < new_end) {
				new_end = next_start;
			}
		}

		block->end = new_end;
		return 0;
	}
}

static int read_block_instruction(
		struct Reader *reader,
		struct Registers *regs,
		struct Stack *stack,
		struct InterruptionTable *int_table,
		const char *segment_start,
		const char **sorted_relocations,
		unsigned int relocation_count,
		void (*print_error)(const char *),
		struct CodeBlock *block,
		struct CodeBlockList *code_block_list,
		struct GlobalVariableList *global_variable_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list) {
	const char *instruction = reader->buffer + reader->buffer_index;
	return read_block_instruction_internal(reader, regs, stack, int_table, segment_start, sorted_relocations, relocation_count, print_error, block, code_block_list, global_variable_list, segment_start_list, reference_list, SEGMENT_INDEX_UNDEFINED, instruction);
}

void print_word_or_byte_register(struct Registers *regs, unsigned int index, const char *word_reg, const char *high_byte_reg, const char *low_byte_reg) {
	if (is_word_register_defined(regs, index)) {
		if (is_word_register_defined_and_relative(regs, index)) {
			fprintf(stderr, " %s=+%x;", word_reg, get_word_register(regs, index));
		}
		else {
			fprintf(stderr, " %s=%x;", word_reg, get_word_register(regs, index));
		}
	}
	else if (is_byte_register_defined(regs, index + 4)) {
		fprintf(stderr, " %s=%x;", high_byte_reg, get_byte_register(regs, index + 4));

		if (is_byte_register_defined(regs, index)) {
			fprintf(stderr, " %s=%x;", low_byte_reg, get_byte_register(regs, index));
		}
	}
	else if (is_byte_register_defined(regs, index)) {
		fprintf(stderr, " %s=%x;", low_byte_reg, get_byte_register(regs, index));
	}
	else {
		fprintf(stderr, " %s=?;", word_reg);
	}
}

void print_word_register(struct Registers *regs, unsigned int index, const char *word_reg) {
	if (is_word_register_defined_and_relative(regs, index)) {
		fprintf(stderr, " %s=+%x;", word_reg, get_word_register(regs, index));
	}
	else if (is_word_register_defined(regs, index)) {
		fprintf(stderr, " %s=%x;", word_reg, get_word_register(regs, index));
	}
	else {
		fprintf(stderr, " %s=?;", word_reg);
	}
}

void print_segment_register(struct Registers *regs, unsigned int index, const char *word_reg) {
	if (is_segment_register_defined_and_relative(regs, index)) {
		fprintf(stderr, " %s=+%x;", word_reg, get_segment_register(regs, index));
	}
	else if (is_segment_register_defined(regs, index)) {
		fprintf(stderr, " %s=%x;", word_reg, get_segment_register(regs, index));
	}
	else {
		fprintf(stderr, " %s=?;", word_reg);
	}
}

void print_regs(struct Registers *regs) {
	print_word_or_byte_register(regs, 0, "AX", "AH", "AL");
	print_word_or_byte_register(regs, 1, "CX", "CH", "CL");
	print_word_or_byte_register(regs, 2, "DX", "DH", "DL");
	print_word_or_byte_register(regs, 3, "BX", "BH", "BL");
	print_word_register(regs, 4, "SP");
	print_word_register(regs, 5, "BP");
	print_word_register(regs, 6, "SI");
	print_word_register(regs, 7, "DI");
	print_segment_register(regs, 0, "ES");
	print_segment_register(regs, 1, "CS");
	print_segment_register(regs, 2, "SS");
	print_segment_register(regs, 3, "DS");
}

void print_stack(struct Stack *stack) {
	fprintf(stderr, " Stack(");
	for (int i = 0; i < stack->word_count; i++) {
		if (i > 0) {
			fprintf(stderr, ", ");
		}

		const unsigned int definition = stack->defined_and_relative[i >> 3] >> ((i & 7) * 2);
		if ((definition & 1) == 0) {
			fprintf(stderr, "?");
		}
		else if (definition & 2) {
			fprintf(stderr, "+%x", stack->values[i]);
		}
		else {
			fprintf(stderr, "%x", stack->values[i]);
		}
	}

	fprintf(stderr, ")");
}

void print_interruption_table(struct InterruptionTable *table) {
	fprintf(stderr, " IntTable(");
	for (int i = 0; i < 256; i++) {
		const char *offset_defined = table->offset_defined[i];
		const char *segment_defined = table->segment_defined[i];
		if (offset_defined && segment_defined) {
			if (table->relative[i >> 3] & (1 << (i & 7))) {
				fprintf(stderr, "%x->+%x:%x", i, table->pointers[i].segment, table->pointers[i].offset);
			}
			else {
				fprintf(stderr, "%x->%x:%x", i, table->pointers[i].segment, table->pointers[i].offset);
			}
		}
		else if (offset_defined) {
			fprintf(stderr, "%x->?:%x", i, table->pointers[i].offset);
		}
		else if (segment_defined) {
			if (table->relative[i >> 3] & (1 << (i & 7))) {
				fprintf(stderr, "%x->+%x:?", i, table->pointers[i].segment);
			}
			else {
				fprintf(stderr, "%x->%x:?", i, table->pointers[i].segment);
			}
		}
	}

	fprintf(stderr, ")\n");
}

int read_block(
		struct Registers *regs,
		const char *segment_start,
		const char **sorted_relocations,
		unsigned int relocation_count,
		void (*print_error)(const char *),
		struct CodeBlock *block,
		unsigned int block_max_size,
		struct CodeBlockList *code_block_list,
		struct GlobalVariableList *global_variable_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list) {
	struct Reader reader;
	reader.buffer = block->start;
	reader.buffer_index = 0;
	reader.buffer_size = block_max_size;

	struct Stack stack;
	initialize_stack(&stack);

	struct InterruptionTable int_table;
	make_all_interruption_table_undefined(&int_table);

	int error_code;
	do {
		if ((error_code = read_block_instruction(&reader, regs, &stack, &int_table, segment_start, sorted_relocations, relocation_count, print_error, block, code_block_list, global_variable_list, segment_start_list, reference_list))) {
			clear_stack(&stack);
			return error_code;
		}

		const int index = index_of_code_block_with_start(code_block_list, block->start);
		if (index + 1 < code_block_list->block_count) {
			const char *next_start = code_block_list->sorted_blocks[index + 1]->start;
			if (block->start + reader.buffer_index >= next_start) {
				block->end = next_start;
			}
		}
	} while (block->start == block->end);

	clear_stack(&stack);
	return 0;
}

int find_code_blocks_and_variables(
		struct SegmentReadResult *read_result,
		void (*print_error)(const char *),
		struct CodeBlockList *code_block_list,
		struct GlobalVariableList *global_variable_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list) {
	struct CodeBlock *first_block = prepare_new_code_block(code_block_list);
	if (!first_block) {
		return 1;
	}

	first_block->ip = read_result->ip;
	first_block->relative_cs = read_result->relative_cs;
	first_block->start = read_result->buffer + (read_result->relative_cs * 16 + read_result->ip);
	first_block->end = first_block->start;
	first_block->flags = 0;
	initialize_code_block_origin_list(&first_block->origin_list);

	struct CodeBlockOrigin *origin = prepare_new_code_block_origin(&first_block->origin_list);
	set_os_type_in_code_block_origin(origin);
	make_all_registers_undefined(&origin->regs);
	set_register_cs_relative(&origin->regs, REGISTER_DEFINED_OUTSIDE, read_result->relative_cs);
	if (ds_should_match_cs_at_segment_start(read_result)) {
		set_register_ds_relative(&origin->regs, REGISTER_DEFINED_OUTSIDE, read_result->relative_cs);
	}

	int error_code;
	if ((error_code = insert_sorted_code_block_origin(&first_block->origin_list, origin))) {
		return error_code;
	}

	if ((error_code = insert_sorted_code_block(code_block_list, first_block))) {
		return error_code;
	}

	int any_evaluated;
	do {
		any_evaluated = 0;

		for (int block_index = 0; block_index < code_block_list->block_count; block_index++) {
			struct CodeBlock *block = code_block_list->page_array[block_index / code_block_list->blocks_per_page] + (block_index % code_block_list->blocks_per_page);
			if (code_block_requires_evaluation(block)) {
				any_evaluated = 1;
				mark_code_block_as_being_evaluated(block);

				struct Registers regs;
				unsigned int block_max_size = read_result->size - (block->start - read_result->buffer);
	
				accumulate_registers_from_code_block_origin_list(&regs, &block->origin_list);
				if ((error_code = read_block(&regs, read_result->buffer, read_result->sorted_relocations, read_result->relocation_count, print_error, block, block_max_size, code_block_list, global_variable_list, segment_start_list, reference_list))) {
					return error_code;
				}

				mark_code_block_as_evaluated(block);
			}
		}
	}
	while (any_evaluated);

	for (int variable_index = 0; variable_index < global_variable_list->variable_count; variable_index++) {
		struct GlobalVariable *variable = global_variable_list->sorted_variables[variable_index];
		if (variable->start == variable->end && variable->var_type == GLOBAL_VARIABLE_TYPE_DOLLAR_TERMINATED_STRING) {
			const int index = index_of_code_block_containing_position(code_block_list, variable->start);
			if (index < 0 || code_block_list->sorted_blocks[index]->end <= variable->start) {
				const char *potential_end = read_result->buffer + read_result->size;
				if (index + 1 < code_block_list->block_count) {
					potential_end = code_block_list->sorted_blocks[index + 1]->start;
				}

				const char *end;
				for (end = variable->start; end < potential_end; end++) {
					if (*end == '$') {
						end++;
						break;
					}
				}

				variable->end = end;
			}
			else {
				// TODO: Find a better solution
				variable->end = code_block_list->sorted_blocks[index]->end;
			}
		}
	}
	return 0;
}

int main(int argc, const char *argv[]) {
	printf("%s", application_name_and_version);

	const char *filename = NULL;
	const char *format = NULL;
	const char *out_filename = NULL;

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
		else if (!strcmp(argv[i], "-o")) {
			if (++i < argc) {
				out_filename = argv[i];
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

	struct CodeBlockList code_block_list;
	initialize_code_block_list(&code_block_list);

	struct GlobalVariableList global_variable_list;
	initialize_global_variable_list(&global_variable_list);

	struct SegmentStartList segment_start_list;
	initialize_segment_start_list(&segment_start_list);

	struct ReferenceList reference_list;
	initialize_reference_list(&reference_list);

	if ((error_code = find_code_blocks_and_variables(&read_result, print_error, &code_block_list, &global_variable_list, &segment_start_list, &reference_list))) {
		goto end;
	}

	if (out_filename) {
		print_output_file = fopen(out_filename, "w");
		if (!print_output_file) {
			fprintf(stderr, "Unable to open output file\n");
			goto end;
		}
	}
	else {
		print_output_file = stdout;
	}
	buffer_start = read_result.buffer;

	if (!strcmp(format, "bin")) {
		print_output("org 0x100\n");
	}

	error_code = dump(
			read_result.buffer,
			read_result.relative_cs? 0x100 : 0,
			code_block_list.sorted_blocks,
			code_block_list.block_count,
			global_variable_list.sorted_variables,
			global_variable_list.variable_count,
			segment_start_list.start,
			segment_start_list.count,
			reference_list.sorted_references,
			reference_list.reference_count,
			read_result.sorted_relocations,
			read_result.relocation_count,
			print_output,
			print_error,
			print_segment_start_label,
			read_result.print_code_label,
			read_result.print_variable_label);

	if (print_output_file != stdout) {
		fclose(print_output_file);
	}

	end:
	if (read_result.relocation_count) {
		free(read_result.sorted_relocations);
		free(read_result.relocation_table);
	}

	clear_reference_list(&reference_list);
	clear_segment_start_list(&segment_start_list);
	clear_global_variable_list(&global_variable_list);
	clear_code_block_list(&code_block_list);
	free(read_result.buffer);
	return error_code;
}
