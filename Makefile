.PHONY: clean

headers = src/code_blocks.h src/dumpers.h src/global_variables.h src/interruption_table.h src/print_utils.h src/reader.h src/registers.h src/version.h
sources = src/code_blocks.c src/disasm.c src/dumpers.c src/global_variables.c src/interruption_table.c src/print_utils.c src/reader.c src/registers.c build/src/version.c

build/bin/disasm: build/bin $(sources) $(headers)
	cc -o $@ $(sources)

build/src/version.c: build/src .git/HEAD
	echo "#include \"../../src/version.h\"" > $@
	echo "" >> $@
	echo "const char *application_name_and_version = \"Disassembler 1.0.0-SNAPSHOT"`git rev-parse --short HEAD`"\\\\n\";" >> $@

build/bin: build
	mkdir $@

build/src: build
	mkdir $@

build:
	mkdir $@

clean:
	rm -rf build
