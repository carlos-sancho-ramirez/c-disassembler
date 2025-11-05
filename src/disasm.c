#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "cblist.h"
#include "gvlist.h"
#include "finder.h"
#include "dumpers.h"
#include "printu.h"
#include "reflist.h"
#include "register.h"
#include "stack.h"
#include "itable.h"
#include "srresult.h"
#include "sslist.h"
#include "version.h"

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
	uint16_t magic; /* Must be "MZ" */
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
	FILE *file;
	if (strcmp(format, "bin") && strcmp(format, "dos")) {
		fprintf(stderr, "Undefined format '%s'. It must be 'bin' or 'dos'\n", format);
		return 1;
	}

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Unable to open file\n");
		return 1;
	}

	if (!strcmp(format, "dos")) {
		unsigned int header_size;
		struct dos_header header;
		unsigned int file_size;

		/* This is assuming that the processor running this is also little endian */
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

		header_size = header.header_paragraphs * 16;
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
			int i;
			result->sorted_relocations = malloc(sizeof(const char *) * header.relocations_count);
			if (!result->sorted_relocations) {
				fprintf(stderr, "Unable to allocate memory for the sorted relocations\n");
				free(result->buffer);
				free(result->relocation_table);
				fclose(file);
				return 1;
			}

			for (i = 0; i < header.relocations_count; i++) {
				struct FarPointer *ptr = result->relocation_table + i;
				const char *relocation = result->buffer + ptr->segment * 16 + ptr->offset;

				int first = 0;
				int last = i;
				unsigned int j;

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

				for (j = header.relocations_count; j > first; j--) {
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
		mark_ds_matches_cs_at_start(result);
	}
	fclose(file);
	return 0;
}

int main(int argc, const char *argv[]) {
	const char *filename = NULL;
	const char *format = NULL;
	const char *out_filename = NULL;
	int i;
	struct SegmentReadResult read_result;
	int error_code;
	struct CodeBlockList code_block_list;
	struct GlobalVariableList global_variable_list;
	struct SegmentStartList segment_start_list;
	struct ReferenceList reference_list;

	printf("%s", application_name_and_version);

	for (i = 1; i < argc; i++) {
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

	if ((error_code = read_file(&read_result, filename, format))) {
		return error_code;
	}

	initialize_code_block_list(&code_block_list);
	initialize_global_variable_list(&global_variable_list);
	initialize_segment_start_list(&segment_start_list);
	initialize_reference_list(&reference_list);

	if ((error_code = find_cblocks_and_gvars(&read_result, print_error, &code_block_list, &global_variable_list, &segment_start_list, &reference_list))) {
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

	for (i = 0; i < code_block_list.block_count; i++) {
		struct CodeBlockOriginList *origin_list = &code_block_list.sorted_blocks[i]->origin_list;
		int j;
		for (j = 0; j < origin_list->origin_count; j++) {
			clear_gvwvmap(&origin_list->sorted_origins[j]->var_values);
		}
		clear_cbolist(origin_list);
	}

	clear_reference_list(&reference_list);
	clear_segment_start_list(&segment_start_list);
	clear_global_variable_list(&global_variable_list);
	clear_code_block_list(&code_block_list);
	free(read_result.buffer);
	return error_code;
}
