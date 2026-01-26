#include "dumpers.h"
#include "printu.h"
#include "reader.h"
#include "relocu.h"
#include "counter.h"
#include "funclist.h"

const char *BYTE_REGISTERS[] = {
	"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

const char *WORD_REGISTERS[] = {
	"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

const char *INSTRUCTION[] = {
	"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"
};

const char *MATH_INSTRUCTION[] = {
	"test", NULL, "not", "neg", "mul", "imul", "div", "idiv"
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

const char *LOOP_INSTRUCTIONS[] = {
	"loopne", "loope", "loop", "jcxz"
};

const char *SHIFT_INSTRUCTIONS[] = {
	"rol", "ror", "rcl", "rcr", "shl", "shr", NULL, "sar"
};

const char *FF_INSTRUCTIONS[] = {
	"inc", "dec", "call", "call", "jmp", "jmp", "push" /*, NULL */
};

const char RELOCATION_VALUE[] = "base_segment";

static void dump_address(
		const char *buffer,
		unsigned int buffer_origin,
		struct Reader *reader,
		struct Reference *reference,
		struct FilePrinter *printer,
		int value1,
		const char *segment,
		const char **registers) {
	if (value1 < 0xC0) {
		print(printer, "[");
		if (segment) {
			print(printer, segment);
			print(printer, ":");
		}

		if ((value1 & 0xC7) == 0x06) {
			const int addr = read_next_word(reader);
			struct GlobalVariable *var;
			if (reference && (var = get_gvar_from_ref_target(reference)) && is_ref_in_instruction_address(reference)) {
				const unsigned int reference_address = var->relative_address;
				print_variable_label(printer, reference_address);

				if (addr < reference_address + buffer_origin) {
					print(printer, "-");
					print_segment_label(printer, buffer + (reference_address + buffer_origin - addr));
				}
				else if (addr > reference_address + buffer_origin) {
					print(printer, "+");
					print_segment_label(printer, buffer + (addr - reference_address - buffer_origin));
				}
			}
			else {
				print_literal_hex_word(printer, addr);
			}
		}
		else {
			print(printer, ADDRESS_REGISTERS[value1 & 0x07]);
			if ((value1 & 0xC0) == 0x40) {
				print_differential_hex_byte(printer, read_next_byte(reader));
			}
			else if ((value1 & 0xC0) == 0x80) {
				print_differential_hex_word(printer, read_next_word(reader));
			}
		}
		print(printer, "]");
	}
	else {
		print(printer, registers[value1 & 0x07]);
	}
}

static void dump_address_register_combination(
		const char *buffer,
		unsigned int buffer_origin,
		struct Reader *reader,
		struct Reference *reference,
		struct FilePrinter *printer,
		int value0,
		int value1,
		const char **registers,
		const char *segment,
		const char **addr_replacement_registers) {
	if (value0 & 0x02) {
		print(printer, registers[(value1 >> 3) & 0x07]);
		print(printer, ",");
		dump_address(buffer, buffer_origin, reader, reference, printer, value1, segment, addr_replacement_registers);
	}
	else {
		dump_address(buffer, buffer_origin, reader, reference, printer, value1, segment, addr_replacement_registers);
		print(printer, ",");
		print(printer, registers[(value1 >> 3) & 0x07]);
	}
}

static int dump_instruction(
		const char *buffer,
		unsigned int buffer_origin,
		struct Reader *reader,
		const struct CodeBlock *block,
		struct Reference *reference,
		const char **sorted_relocations,
		unsigned int relocation_count,
		struct FunctionList *func_list,
		struct FilePrinter *printer_out,
		struct FilePrinter *printer_err) {
	const char *segment = NULL;
	while (1) {
		const int value0 = read_next_byte(reader);
		if (value0 >= 0 && value0 < 0x40 && (value0 & 0x06) != 0x06) {
			print(printer_out, INSTRUCTION[value0 >> 3]);
			print(printer_out, " ");
			if ((value0 & 0x04) == 0x00) {
				const char **registers;
				int value1;
				if (value0 & 0x01) {
					registers = WORD_REGISTERS;
				}
				else {
					registers = BYTE_REGISTERS;
				}

				value1 = read_next_byte(reader);
				dump_address_register_combination(buffer, buffer_origin, reader, reference, printer_out, value0, value1, registers, segment, registers);
				print(printer_out, "\n");
				return 0;
			}
			else if ((value0 & 0x07) == 0x04) {
				print(printer_out, BYTE_REGISTERS[0]);
				print(printer_out, ",");
				print_literal_hex_byte(printer_out, read_next_byte(reader));
				print(printer_out, "\n");
				return 0;
			}
			else {
				/* (value0 & 0x07) == 0x05 */
				print(printer_out, WORD_REGISTERS[0]);
				print(printer_out, ",");
				print_literal_hex_word(printer_out, read_next_word(reader));
				print(printer_out, "\n");
				return 0;
			}
		}
		else if ((value0 & 0xE6) == 0x06 && value0 != 0x0F) {
			if (value0 & 0x01) {
				print(printer_out, "pop ");
			}
			else {
				print(printer_out, "push ");
			}
			print(printer_out, SEGMENT_REGISTERS[(value0 >> 3) & 0x03]);
			print(printer_out, "\n");
			return 0;
		}
		else if ((value0 & 0xE7) == 0x26) {
			segment = SEGMENT_REGISTERS[(value0 >> 3) & 0x03];
		}
		else if ((value0 & 0xF0) == 0x40) {
			if (value0 & 0x08) {
				print(printer_out, "dec ");
			}
			else {
				print(printer_out, "inc ");
			}
			print(printer_out, WORD_REGISTERS[value0 & 0x07]);
			print(printer_out, "\n");
			return 0;
		}
		else if ((value0 & 0xF0) == 0x50) {
			if (value0 & 0x08) {
				print(printer_out, "pop ");
			}
			else {
				print(printer_out, "push ");
			}
			print(printer_out, WORD_REGISTERS[value0 & 0x07]);
			print(printer_out, "\n");
			return 0;
		}
		else if ((value0 & 0xF0) == 0x70) {
			const int value1 = read_next_byte(reader);
			const int target_ip = block->ip + reader->buffer_index + ((value1 >= 0x80)? value1 - 256 : value1);

			print(printer_out, JUMP_INSTRUCTIONS[value0 & 0x0F]);
			print(printer_out, " ");
			print_code_label(printer_out, target_ip, block->relative_cs);
			print(printer_out, "\n");
			return 0;
		}
		else if ((value0 & 0xFE) == 0x80) {
			const char **registers;
			const int value1 = read_next_byte(reader);
			print(printer_out, INSTRUCTION[(value1 >> 3) & 0x07]);
			if ((value1 & 0xC0) != 0xC0) {
				if (value0 & 1) {
					print(printer_out, " word ");
				}
				else {
					print(printer_out, " byte ");
				}
			}
			else {
				print(printer_out, " ");
			}

			registers = (value0 & 1)? WORD_REGISTERS : BYTE_REGISTERS;
			dump_address(buffer, buffer_origin, reader, reference, printer_out, value1, segment, registers);
			print(printer_out, ",");
			if (value0 & 1) {
				print_literal_hex_word(printer_out, read_next_word(reader));
			}
			else {
				print_literal_hex_byte(printer_out, read_next_byte(reader));
			}
			print(printer_out, "\n");
			return 0;
		}
		else if (value0 == 0x83) {
			const int value1 = read_next_byte(reader);
			print(printer_out, INSTRUCTION[(value1 >> 3) & 0x07]);
			print(printer_out, (value1 < 0xC0)? " word " : " ");

			dump_address(buffer, buffer_origin, reader, reference, printer_out, value1, segment, WORD_REGISTERS);
			print(printer_out, ",");
			print_differential_hex_byte(printer_out, read_next_byte(reader));
			print(printer_out, "\n");
			return 0;
		}
		else if ((value0 & 0xFE) == 0x86 || (value0 & 0xFC) == 0x88) {
			const char **registers = (value0 & 1)? WORD_REGISTERS : BYTE_REGISTERS;
			const int value1 = read_next_byte(reader);
			print(printer_out, (value0 < 0x88)? "xchg " : "mov ");
			dump_address_register_combination(buffer, buffer_origin, reader, reference, printer_out, value0, value1, registers, segment, registers);
			print(printer_out, "\n");
			return 0;
		}
		else if ((value0 & 0xFD) == 0x8C) {
			const int value1 = read_next_byte(reader);
			if (value1 & 0x20) {
				print(printer_out, "db ");
				print_literal_hex_byte(printer_out, value0);
				print(printer_out, " ");
				print_literal_hex_byte(printer_out, value1);
				print(printer_out, " ; Unknown instruction\n");
				return 1;
			}
			else {
				print(printer_out, "mov ");
				dump_address_register_combination(buffer, buffer_origin, reader, reference, printer_out, value0, value1, SEGMENT_REGISTERS, segment, WORD_REGISTERS);
				print(printer_out, "\n");
				return 0;
			}
		}
		else if (value0 == 0x8D) {
			const int value1 = read_next_byte(reader);
			if (value1 >= 0xC0) {
				print(printer_out, "db ");
				print_literal_hex_byte(printer_out, value0);
				print(printer_out, " ");
				print_literal_hex_byte(printer_out, value1);
				print(printer_out, " ; Unknown instruction\n");
				return 1;
			}
			else {
				print(printer_out, "lea ");
				print(printer_out, WORD_REGISTERS[(value1 >> 3) & 0x07]);
				print(printer_out, ",");
				dump_address(buffer, buffer_origin, reader, reference, printer_out, value1, segment, NULL);
				print(printer_out, "\n");
				return 0;
			}
		}
		else if (value0 == 0x8F) {
			const int value1 = read_next_byte(reader);
			if (value1 & 0x38 || value1 >= 0xC0) {
				print(printer_out, "db ");
				print_literal_hex_byte(printer_out, value0);
				print(printer_out, " ");
				print_literal_hex_byte(printer_out, value1);
				print(printer_out, " ; Unknown instruction\n");
				return 1;
			}
			else {
				print(printer_out, "pop ");
				dump_address(buffer, buffer_origin, reader, reference, printer_out, value1, segment, NULL);
				print(printer_out, "\n");
				return 0;
			}
		}
		else if (value0 == 0x90) {
			print(printer_out, "nop\n");
			return 0;
		}
		else if ((value0 & 0xF8) == 0x90) {
			print(printer_out, "xchg ax,");
			print(printer_out, WORD_REGISTERS[value0 & 0x07]);
			print(printer_out, "\n");
			return 0;
		}
		else if (value0 == 0x98) {
			print(printer_out, "cbw\n");
			return 0;
		}
		else if (value0 == 0x99) {
			print(printer_out, "cwd\n");
			return 0;
		}
		else if ((value0 & 0xFC) == 0xA0) {
			const char **registers;
			int addr_value;
			print(printer_out, "mov ");
			if (value0 & 1) {
				registers = WORD_REGISTERS;
			}
			else {
				registers = BYTE_REGISTERS;
			}

			addr_value = read_next_word(reader);
			if ((value0 & 0xFE) == 0xA0) {
				struct GlobalVariable *var;
				print(printer_out, registers[0]);
				print(printer_out, ",[");
				if (segment) {
					print(printer_out, segment);
					print(printer_out, ":");
				}

				if (reference && (var = get_gvar_from_ref_target(reference)) && is_ref_in_instruction_address(reference)) {
					const unsigned int reference_address = var->relative_address;
					print_variable_label(printer_out, reference_address);

					if (addr_value < reference_address + buffer_origin) {
						print(printer_out, "-");
						print_segment_label(printer_out, buffer + (reference_address + buffer_origin - addr_value));
					}
					else if (addr_value > reference_address + buffer_origin) {
						print(printer_out, "+");
						print_segment_label(printer_out, buffer + (addr_value - reference_address - buffer_origin));
					}
				}
				else {
					print_literal_hex_word(printer_out, addr_value);
				}

				print(printer_out, "]");
			}
			else {
				struct GlobalVariable *var;

				print(printer_out, "[");
				if (segment) {
					print(printer_out, segment);
					print(printer_out, ":");
				}

				if (reference && (var = get_gvar_from_ref_target(reference)) && is_ref_in_instruction_address(reference)) {
					const unsigned int reference_address = var->relative_address;
					print_variable_label(printer_out, reference_address);

					if (addr_value < reference_address + buffer_origin) {
						print(printer_out, "-");
						print_segment_label(printer_out, buffer + (reference_address + buffer_origin - addr_value));
					}
					else if (addr_value > reference_address + buffer_origin) {
						print(printer_out, "+");
						print_segment_label(printer_out, buffer + (addr_value - reference_address - buffer_origin));
					}
				}
				else {
					print_literal_hex_word(printer_out, addr_value);
				}

				print(printer_out, "],");
				print(printer_out, registers[0]);
			}

			print(printer_out, "\n");
			return 0;
		}
		else if (value0 == 0xA4) {
			print(printer_out, "movsb\n");
			return 0;
		}
		else if (value0 == 0xA5) {
			print(printer_out, "movsw\n");
			return 0;
		}
		else if (value0 == 0xA6) {
			print(printer_out, "cmpsb\n");
			return 0;
		}
		else if (value0 == 0xA7) {
			print(printer_out, "cmpsw\n");
			return 0;
		}
		else if (value0 == 0xA8) {
			print(printer_out, "test al,");
			print_literal_hex_byte(printer_out, read_next_byte(reader));
			print(printer_out, "\n");
			return 0;
		}
		else if (value0 == 0xA9) {
			print(printer_out, "test ax,");
			print_literal_hex_word(printer_out, read_next_word(reader));
			print(printer_out, "\n");
			return 0;
		}
		else if (value0 == 0xAA) {
			print(printer_out, "stosb\n");
			return 0;
		}
		else if (value0 == 0xAB) {
			print(printer_out, "stosw\n");
			return 0;
		}
		else if (value0 == 0xAC) {
			print(printer_out, "lodsb\n");
			return 0;
		}
		else if (value0 == 0xAD) {
			print(printer_out, "lodsw\n");
			return 0;
		}
		else if (value0 == 0xAE) {
			print(printer_out, "scasb\n");
			return 0;
		}
		else if (value0 == 0xAF) {
			print(printer_out, "scasw\n");
			return 0;
		}
		else if ((value0 & 0xF0) == 0xB0) {
			print(printer_out, "mov ");
			if (value0 & 0x08) {
				const char *relocation_query;
				int offset_value;
				int relocation_segment_present = 0;
				struct GlobalVariable *var;
				struct CodeBlock *ref_block;

				print(printer_out, WORD_REGISTERS[value0 & 0x07]);
				print(printer_out, ",");
				relocation_query = reader->buffer + reader->buffer_index;
				offset_value = read_next_word(reader);
				if ((relocation_segment_present = is_relocation_present_in_sorted_relocations(sorted_relocations, relocation_count, relocation_query))) {
					print(printer_out, RELOCATION_VALUE);
				}

				if (reference && (var = get_gvar_from_ref_target(reference)) && !is_ref_in_instruction_address(reference)) {
					unsigned int ref_var_value = var->relative_address;
					if (relocation_segment_present) {
						print(printer_out, "+");
					}

					print_variable_label(printer_out, ref_var_value);
				}
				else if (reference && (ref_block = get_cblock_from_ref_target(reference))) {
					if (relocation_segment_present) {
						print(printer_out, "+");
					}

					print_code_label(printer_out, ref_block->ip, ref_block->relative_cs);
				}
				else {
					if (relocation_segment_present) {
						if (offset_value) {
							print(printer_out, "+");
							print_literal_hex_word(printer_out, offset_value);
						}
					}
					else {
						print_literal_hex_word(printer_out, offset_value);
					}
				}
			}
			else {
				print(printer_out, BYTE_REGISTERS[value0 & 0x07]);
				print(printer_out, ",");
				print_literal_hex_byte(printer_out, read_next_byte(reader));
			}
			print(printer_out, "\n");
			return 0;
		}
		else if (value0 == 0xC2) {
			print(printer_out, "ret ");
			print_literal_hex_word(printer_out, read_next_word(reader));
			print(printer_out, "\n");
			return 0;
		}
		else if (value0 == 0xC3) {
			print(printer_out, "ret\n");
			return 0;
		}
		else if ((value0 & 0xFE) == 0xC4) {
			const int value1 = read_next_byte(reader);
			if ((value1 & 0xC0) == 0xC0) {
				print(printer_out, "db ");
				print_literal_hex_byte(printer_out, value0);
				print(printer_out, " ");
				print_literal_hex_byte(printer_out, value1);
				print(printer_out, " ; Unknown instruction\n");
				return 1;
			}
			else {
				if (value0 & 1) {
					print(printer_out, "lds ");
				}
				else {
					print(printer_out, "les ");
				}

				print(printer_out, WORD_REGISTERS[(value1 >> 3) & 0x07]);
				print(printer_out, ",");
				dump_address(buffer, buffer_origin, reader, reference, printer_out, value1, segment, NULL);
				print(printer_out, "\n");
				return 0;
			}
		}
		else if ((value0 & 0xFE) == 0xC6) {
			const int value1 = read_next_byte(reader);
			if (value1 & 0x38) {
				print(printer_out, "db ");
				print_literal_hex_byte(printer_out, value0);
				print(printer_out, " ");
				print_literal_hex_byte(printer_out, value1);
				print(printer_out, " ; Unknown instruction\n");
				return 1;
			}
			else {
				print(printer_out, "mov ");
				if ((value1 & 0xC0) != 0xC0) {
					if (value0 & 1) {
						print(printer_out, "word ");
					}
					else {
						print(printer_out, "byte ");
					}
				}
				dump_address(buffer, buffer_origin, reader, reference, printer_out, value1, segment, BYTE_REGISTERS);
				print(printer_out, ",");
				if (value0 & 1) {
					print_literal_hex_word(printer_out, read_next_word(reader));
				}
				else {
					print_literal_hex_byte(printer_out, read_next_byte(reader));
				}
				print(printer_out, "\n");
				return 0;
			}
		}
		else if (value0 == 0xCB) {
			print(printer_out, "retf\n");
			return 0;
		}
		else if (value0 == 0xCD) {
			print(printer_out, "int ");
			print_literal_hex_byte(printer_out, read_next_byte(reader));
			print(printer_out, "\n");
			return 0;
		}
		else if ((value0 & 0xFC) == 0xD0) {
			const int value1 = read_next_byte(reader);
			if ((value1 & 0x38) == 0x30) {
				print(printer_out, "db ");
				print_literal_hex_byte(printer_out, value0);
				print(printer_out, " ");
				print_literal_hex_byte(printer_out, value1);
				print(printer_out, " ; Unknown instruction\n");
				return 1;
			}
			else {
				const char **registers;
				print(printer_out, SHIFT_INSTRUCTIONS[(value1 >> 3) & 0x07]);
				if ((value0 & 0xC0) == 0xC0) {
					print(printer_out, " ");
				}
				else if (value1 & 1) {
					print(printer_out, " word ");
				}
				else {
					print(printer_out, " byte ");
				}

				registers = (value0 & 1)? WORD_REGISTERS : BYTE_REGISTERS;
				dump_address(buffer, buffer_origin, reader, reference, printer_out, value1, segment, registers);

				if (value0 & 2) {
					print(printer_out, ",cl\n");
				}
				else {
					print(printer_out, ",1\n");
				}
				return 0;
			}
		}
		else if ((value0 & 0xFC) == 0xE0) {
			const int value1 = read_next_byte(reader);
			const int target_ip = block->ip + reader->buffer_index + ((value1 >= 0x80)? value1 - 256 : value1);

			print(printer_out, LOOP_INSTRUCTIONS[value0 & 0x0F]);
			print(printer_out, " ");
			print_code_label(printer_out, target_ip, block->relative_cs);
			print(printer_out, "\n");
			return 0;
		}
		else if ((value0 & 0xFE) == 0xE8) {
			const int diff = read_next_word(reader);
			const uint16_t target_ip = block->ip + reader->buffer_index + diff;

			if (value0 & 1) {
				print(printer_out, "jmp ");
			}
			else {
				print(printer_out, "call ");
			}

			print_code_label(printer_out, target_ip, block->relative_cs);
			print(printer_out, "\n");
			return 0;
		}
		else if (value0 == 0xEA) {
			const int offset = read_next_word(reader);
			print(printer_out, "jmp ");
			print_literal_hex_word(printer_out, read_next_word(reader));
			print(printer_out, ":");
			print_literal_hex_word(printer_out, offset);
			print(printer_out, "\n");
		}
		else if (value0 == 0xEB) {
			const int value1 = read_next_byte(reader);
			const int target_ip = block->ip + reader->buffer_index + ((value1 >= 0x80)? value1 - 256 : value1);
			int index;

			print(printer_out, "jmp ");
			print_code_label(printer_out, target_ip, block->relative_cs);
			print(printer_out, "\n");
			return 0;
		}
		else if (value0 == 0xF2) {
			print(printer_out, "repne\n");
			return 0;
		}
		else if (value0 == 0xF3) {
			print(printer_out, "repe\n");
			return 0;
		}
		else if ((value0 & 0xFE) == 0xF6) {
			const int value1 = read_next_byte(reader);
			if ((value1 & 0x38) == 0x08) {
				print(printer_out, "db ");
				print_literal_hex_byte(printer_out, value0);
				print(printer_out, " ");
				print_literal_hex_byte(printer_out, value1);
				print(printer_out, " ; Unknown instruction\n");
				return 1;
			}
			else {
				const char **registers;
				print(printer_out, MATH_INSTRUCTION[(value1 >> 3) & 0x07]);
				if ((value1 & 0xC0) != 0xC0) {
					if (value0 & 1) {
						print(printer_out, " word ");
					}
					else {
						print(printer_out, " byte ");
					}
				}
				else {
					print(printer_out, " ");
				}

				registers = (value0 & 1)? WORD_REGISTERS : BYTE_REGISTERS;
				dump_address(buffer, buffer_origin, reader, reference, printer_out, value1, segment, registers);
				if ((value1 & 0x38) == 0) {
					print(printer_out, ",");
					if (value0 & 1) {
						print_literal_hex_word(printer_out, read_next_word(reader));
					}
					else {
						print_literal_hex_byte(printer_out, read_next_byte(reader));
					}
				}
				print(printer_out, "\n");
				return 0;
			}
		}
		else if (value0 == 0xF8) {
			print(printer_out, "clc\n");
			return 0;
		}
		else if (value0 == 0xF9) {
			print(printer_out, "stc\n");
			return 0;
		}
		else if (value0 == 0xFA) {
			print(printer_out, "cli\n");
			return 0;
		}
		else if (value0 == 0xFB) {
			print(printer_out, "sti\n");
			return 0;
		}
		else if (value0 == 0xFC) {
			print(printer_out, "cld\n");
			return 0;
		}
		else if (value0 == 0xFD) {
			print(printer_out, "std\n");
			return 0;
		}
		else if (value0 == 0xFE) {
			const int value1 = read_next_byte(reader);
			if (value1 & 0x30) {
				print(printer_err, "Unknown opcode ");
				print_literal_hex_byte(printer_err, value0);
				print(printer_err, " ");
				print_literal_hex_byte(printer_err, value1);
				print(printer_err, "\n");
				return 1;
			}
			else {
				print(printer_out, FF_INSTRUCTIONS[(value1 >> 3) & 0x07]);
				print(printer_out, (value1 < 0xC0)? " byte " : " ");
				dump_address(buffer, buffer_origin, reader, reference, printer_out, value1, segment, BYTE_REGISTERS);
				print(printer_out, "\n");
				return 0;
			}
		}
		else if (value0 == 0xFF) {
			const int value1 = read_next_byte(reader);
			if ((value1 & 0x38) == 0x38 || (value1 & 0xF8) == 0xD8 || (value1 & 0xF8) == 0xE8) {
				print(printer_err, "Unknown opcode ");
				print_literal_hex_byte(printer_err, value0);
				print(printer_err, " ");
				print_literal_hex_byte(printer_err, value1);
				print(printer_err, "\n");
				return 1;
			}
			else {
				print(printer_out, FF_INSTRUCTIONS[(value1 >> 3) & 0x07]);
				if (value1 < 0xC0 && ((value1 & 0x08) == 0x00 || (value1 & 0x38) == 0x08)) {
					print(printer_out, " word ");
				}
				else if (value1 < 0xC0 && ((value1 & 0x38) == 0x18 || (value1 & 0x38) == 0x28)) {
					print(printer_out, " far16 ");
				}
				else {
					print(printer_out, " ");
				}
				dump_address(buffer, buffer_origin, reader, reference, printer_out, value1, segment, WORD_REGISTERS);
				print(printer_out, "\n");
				return 0;
			}
		}
		else {
			print(printer_out, "db ");
			print_literal_hex_byte(printer_out, value0);
			print(printer_out, " ; Unknown instruction\n");
			return 1;
		}
	}
}

static int valid_char_for_string_literal(char ch) {
	/* More can be added when required. Avoid adding quotes, as it may conflict */
	return ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch >= '0' && ch <= '9' || ch == ' ' || ch == '!' || ch == '$';
}

static int should_display_string_literal_for_gvar(const struct GlobalVariable *variable) {
	if (variable->var_type == GVAR_TYPE_BYTE_STRING || variable->var_type == GVAR_TYPE_DOLLAR_TERMINATED_STRING) {
		const char *position;
		for (position = variable->start; position < variable->end; position++) {
			if (!valid_char_for_string_literal(*position)) {
				return 0;
			}
		}

		return 1;
	}

	return 0;
}

static int valid_char_for_string_with_backquotes(char ch) {
	return valid_char_for_string_literal(ch) || ch == '\r' || ch == '\n' || ch == '\0';
}

static int should_display_string_with_backquotes_for_gvar(const struct GlobalVariable *variable) {
	if (variable->var_type == GVAR_TYPE_BYTE_STRING || variable->var_type == GVAR_TYPE_DOLLAR_TERMINATED_STRING) {
		const char *position;
		for (position = variable->start; position < variable->end; position++) {
			if (!valid_char_for_string_with_backquotes(*position)) {
				return 0;
			}
		}

		return 1;
	}

	return 0;
}

static int dump_variable(
		const struct GlobalVariable *variable,
		const unsigned int variable_print_length,
		struct FilePrinter *printer_out,
		struct FilePrinter *printer_err) {
	print(printer_out, "\n");
	print_variable_label(printer_out, variable->relative_address);
	print(printer_out, ":\n");

	if (should_display_string_literal_for_gvar(variable)) {
		const char *position;
		char str[] = "x";

		print(printer_out, "db '");
		for (position = variable->start; position < variable->end; position++) {
			str[0] = *position;
			print(printer_out, str);
		}
		print(printer_out, "'\n");
	}
	else if (should_display_string_with_backquotes_for_gvar(variable)) {
		int start_required = 1;
		char str[] = "x";
		const char *position;

		for (position = variable->start; position < variable->end; position++) {
			if (start_required) {
				print(printer_out, "db `");
				start_required = 0;
			}

			if (*position == '\0') {
				print(printer_out, "\\0");
			}
			else if (*position == '\r') {
				print(printer_out, "\\r");
			}
			else if (*position == '\n') {
				print(printer_out, "\\n`\n");
				start_required = 1;
			}
			else {
				str[0] = *position;
				print(printer_out, str);
			}
		}

		if (!start_required) {
			print(printer_out, "`\n");
		}
	}
	else if (variable->var_type == GVAR_TYPE_WORD && variable->start + 2 == variable->end ||
			variable->var_type == GVAR_TYPE_FAR_POINTER && variable_print_length == 2) {
		print(printer_out, "dw ");
		print_literal_hex_word(printer_out, *((const uint16_t *) variable->start));
		print(printer_out, "\n");
	}
	else if (variable->var_type == GVAR_TYPE_FAR_POINTER && variable->start + 4 == variable->end) {
		print(printer_out, "dw ");
		print_literal_hex_word(printer_out, *((const uint16_t *) variable->start));
		print(printer_out, "\ndw ");
		print_literal_hex_word(printer_out, *((const uint16_t *) variable->start + 2));
		print(printer_out, "\n");
	}
	else {
		const char *position;
		for (position = variable->start; position < (variable->start + variable_print_length); position++) {
			print(printer_out, "db ");
			print_literal_hex_byte(printer_out, *position);
			print(printer_out, "\n");
		}
	}

	return 0;
}

static const char *determine_position(const char *segment_start, struct CodeBlock *block, struct GlobalVariable *variable) {
	if (segment_start) {
		if (block && variable) {
			return (segment_start < block->start && segment_start < variable->start)? segment_start :
					(block->start < variable->start)? block->start : variable->start;
		}
		else if (block) {
			return (segment_start < block->start)? segment_start : block->start;
		}
		else if (variable) {
			return (segment_start < variable->start)? segment_start : variable->start;
		}
		else {
			return NULL;
		}
	}
	else {
		if (block && variable) {
			return (block->start < variable->start)? block->start : variable->start;
		}
		else if (block) {
			return block->start;
		}
		else if (variable) {
			return variable->start;
		}
		else {
			return NULL;
		}
	}
}

#include "printd.h"
#include <stdio.h>

#ifdef DEBUG
#define DEBUG_ASSIGN_LAST_END(x) last_end = x;
#define DEBUG_DUMP_GAP() \
if (position > last_end) { \
	DEBUG_PRINT1("; Gap at %x\n", (int) (last_end - buffer)); \
} \

#else /* DEBUG */
#define DEBUG_ASSIGN_LAST_END(x)
#define DEBUG_DUMP_GAP()
#endif /* DEBUG */

int dump(
		const char *buffer,
		unsigned int buffer_origin,
		struct CodeBlock **sorted_blocks,
		unsigned int code_block_count,
		struct GlobalVariable **sorted_variables,
		unsigned int global_variable_count,
		const char **segment_starts,
		unsigned int segment_start_count,
		struct Reference **gvar_refs,
		unsigned int gvar_ref_count,
		const char **sorted_relocations,
		unsigned int relocation_count,
		struct FunctionList *func_list,
		struct FilePrinter *printer_out,
		struct FilePrinter *printer_err) {
	struct Reader reader;
	int error_code;

	int segment_start_index = 0;
	int code_block_index = 0;
	int global_variable_index = 0;
	const char *segment_start;
	struct CodeBlock *block;
	const char *position;
	struct GlobalVariable *variable;
	int unknown_opcode_found_in_block = 0;

#ifdef DEBUG
	const char *last_end;
#endif

	segment_start = segment_start_count? segment_starts[0] : NULL;
	block = code_block_count? sorted_blocks[code_block_index] : NULL;
	while (block && !should_be_dumped(block)) {
		block = (++code_block_index < code_block_count)? sorted_blocks[code_block_index] : NULL;
	}

	variable = global_variable_count? sorted_variables[global_variable_index] : NULL;
	position = determine_position(segment_start, block, variable);

	while (position) {
		int position_in_block;
		int position_in_variable;

		while (segment_start_index < segment_start_count && segment_starts[segment_start_index] < position) {
			segment_start_index++;
		}

		if (segment_start_index >= segment_start_count) {
			segment_start = NULL;
		}
		else if (segment_starts[segment_start_index] == position) {
			print(printer_out, "\n");
			print_segment_label(printer_out, position);
			print(printer_out, ":\n");
			segment_start = (++segment_start_index < segment_start_count)? segment_starts[segment_start_index] : NULL;
		}
		else {
			segment_start = segment_starts[segment_start_index];
		}

		position_in_block = block && block->start <= position && position < block->end;
		position_in_variable = variable && variable->start <= position && position < variable->end;

		if (!position_in_block && !position_in_variable) {
			DEBUG_PRINT1("; Gap at %x\n", (int) (position - buffer));
			position = determine_position(segment_start, block, variable);
		}
		else if (position_in_variable && !position_in_block && (!block || block->start >= variable->end)) {
			struct GlobalVariable *next_variable = ((global_variable_index + 1) < global_variable_count)? sorted_variables[global_variable_index + 1] : NULL;
			unsigned int variable_print_length = ((next_variable && next_variable->start < variable->end)? next_variable->start : variable->end) - variable->start;
			if ((error_code = dump_variable(variable, variable_print_length, printer_out, printer_err))) {
				return error_code;
			}

			DEBUG_ASSIGN_LAST_END(variable->end)

			++global_variable_index;
			variable = next_variable;
			position = determine_position(segment_start, block, variable);
			DEBUG_DUMP_GAP()
		}
		else if (position_in_block && !position_in_variable) {
			if (block->start == position && should_dump_label_for_block(block)) {
				print(printer_out, "\n");
				print_code_label(printer_out, block->ip, block->relative_cs);
				print(printer_out, ":\n");
			}

			if (unknown_opcode_found_in_block) {
				reader.buffer = block->start;
				reader.buffer_index = position - block->start;
				reader.buffer_size = block->end - block->start;

				print(printer_out, "db ");
				print_literal_hex_byte(printer_out, read_next_byte(&reader));
				print(printer_out, "\n");

				position++;
				if (position >= block->end) {
					unknown_opcode_found_in_block = 0;
					DEBUG_ASSIGN_LAST_END(block->end)

					block = (++code_block_index < code_block_count)? sorted_blocks[code_block_index] : NULL;
					while (block && !should_be_dumped(block)) {
						block = (++code_block_index < code_block_count)? sorted_blocks[code_block_index] : NULL;
					}
					position = determine_position(segment_start, block, variable);
					DEBUG_DUMP_GAP()
				}
			}
			else {
				const char *next_position;
				reader.buffer = position;
				reader.buffer_index = 0;
				reader.buffer_size = block->end - position;
				error_code = read_for_instruction_length(&reader);
				next_position = position + reader.buffer_index;

				if (error_code || variable && next_position > variable->start) {
					unknown_opcode_found_in_block = 1;
					reader.buffer_index = 0;
					print(printer_out, "db ");
					print_literal_hex_byte(printer_out, read_next_byte(&reader));
					print(printer_out, (error_code == READ_ERROR_UNKNOWN_OPCODE)? " ; Unknown opcode\n" : "\n");

					position++;
					if (position >= block->end) {
						unknown_opcode_found_in_block = 0;
						DEBUG_ASSIGN_LAST_END(block->end)
						block = (++code_block_index < code_block_count)? sorted_blocks[code_block_index] : NULL;
						while (block && !should_be_dumped(block)) {
							block = (++code_block_index < code_block_count)? sorted_blocks[code_block_index] : NULL;
						}
						position = determine_position(segment_start, block, variable);
						DEBUG_DUMP_GAP()
					}
				}
				else {
					struct Reference *reference = NULL;

					reader.buffer = block->start;
					reader.buffer_index = position - block->start;
					reader.buffer_size = block->end - block->start;

					while(gvar_ref_count > 0 && gvar_refs[0]->instruction < position) {
						gvar_refs++;
						gvar_ref_count--;
					}

					if (gvar_ref_count > 0 && gvar_refs[0]->instruction == position) {
						reference = gvar_refs[0];

						gvar_refs++;
						gvar_ref_count--;
					}

					unknown_opcode_found_in_block = dump_instruction(buffer, buffer_origin, &reader, block, reference, sorted_relocations, relocation_count, func_list, printer_out, printer_err);
					position = next_position;
					if (position >= block->end) {
						unknown_opcode_found_in_block = 0;
						DEBUG_ASSIGN_LAST_END(block->end)
						block = (++code_block_index < code_block_count)? sorted_blocks[code_block_index] : NULL;
						while (block && !should_be_dumped(block)) {
							block = (++code_block_index < code_block_count)? sorted_blocks[code_block_index] : NULL;
						}
						position = determine_position(segment_start, block, variable);
						DEBUG_DUMP_GAP()
					}
				}
			}
		}
		else {
			struct GlobalVariable *next_variable;
			unsigned int variable_print_length;
			const char *current_variable_end;
			unsigned int current_variable_size;

			if (block->start == position && should_dump_label_for_block(block)) {
				print(printer_out, "\n");
				print_code_label(printer_out, block->ip, block->relative_cs);
				print(printer_out, ":\n");
			}

			next_variable = ((global_variable_index + 1) < global_variable_count)? sorted_variables[global_variable_index + 1] : NULL;
			variable_print_length = ((next_variable && next_variable->start < variable->end)? next_variable->start : variable->end) - variable->start;
			if ((error_code = dump_variable(variable, variable_print_length, printer_out, printer_err))) {
				return error_code;
			}

			current_variable_end = variable->end;
			current_variable_size = variable->end - variable->start;
			++global_variable_index;
			variable = next_variable;

			if (unknown_opcode_found_in_block) {
				position += current_variable_size;

				if (position >= block->end) {
					unknown_opcode_found_in_block = 0;
					DEBUG_ASSIGN_LAST_END(block->end)
					block = (++code_block_index < code_block_count)? sorted_blocks[code_block_index] : NULL;
					while (block && !should_be_dumped(block)) {
						block = (++code_block_index < code_block_count)? sorted_blocks[code_block_index] : NULL;
					}
					position = determine_position(segment_start, block, variable);
					DEBUG_DUMP_GAP()
				}
			}
			else {
				const char *next_position = position;
				do {
					reader.buffer = next_position;
					reader.buffer_index = 0;
					reader.buffer_size = block->end - next_position;
					error_code = read_for_instruction_length(&reader);
					next_position += reader.buffer_index;
				}
				while (error_code == 0 && next_position < current_variable_end);

				for (position = current_variable_end; position < next_position; position++) {
					reader.buffer = position;
					reader.buffer_index = 0;
					reader.buffer_size = 1;
					print(printer_out, "db ");
					print_literal_hex_byte(printer_out, read_next_byte(&reader));
					print(printer_out, "\n");
				}

				if (position >= block->end) {
					unknown_opcode_found_in_block = 0;
					DEBUG_ASSIGN_LAST_END(block->end);
					block = (++code_block_index < code_block_count)? sorted_blocks[code_block_index] : NULL;
					while (block && !should_be_dumped(block)) {
						block = (++code_block_index < code_block_count)? sorted_blocks[code_block_index] : NULL;
					}
					position = determine_position(segment_start, block, variable);
					DEBUG_DUMP_GAP()
				}
			}
		}
	}

	return 0;
}
