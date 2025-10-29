.PHONY: clean check test

headers = src/code_blocks.h src/dumpers.h src/global_variables.h src/interruption_table.h src/print_utils.h src/reader.h src/refs.h src/registers.h src/relocations.h src/segments.h src/stack.h src/version.h
sources = src/code_blocks.c src/disasm.c src/dumpers.c src/global_variables.c src/interruption_table.c src/print_utils.c src/reader.c src/refs.c src/registers.c src/relocations.c src/segments.c src/stack.c build/src/version.c

build/bin/disasm: build/bin $(sources) $(headers)
	cc -std=c89 -pedantic -o $@ $(sources)

build/src/version.c: build/src .git/HEAD
	echo "#include \"../../src/version.h\"" > $@
	echo "" >> $@
	echo "const char *application_name_and_version = \"Disassembler 1.0.0-SNAPSHOT"`git rev-parse --short HEAD`"\\\\n\";" >> $@

build/bin: build
	mkdir -p $@

build/src: build
	mkdir -p $@

build/test/samples/bin/%.asm: samples/bin/%.com build/bin/disasm build/test/samples/bin
	build/bin/disasm -f bin -i $< -o $@

build/test/samples/bin: build/test/samples
	mkdir -p $@

build/test/samples: build/test
	mkdir -p $@

build/test: build
	mkdir -p $@

build:
	mkdir -p $@

check: $(sources) $(headers)
	editorconfig-checker

test: build/test/samples/bin/hello.asm build/test/samples/bin/timer.asm
	cmp test/samples/bin/hello.asm build/test/samples/bin/hello.asm
	cmp test/samples/bin/timer.asm build/test/samples/bin/timer.asm

clean:
	rm -rf build
