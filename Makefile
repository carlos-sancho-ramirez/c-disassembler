.PHONY: clean

build/disasm: build src/code_blocks.c src/disasm.c src/dumpers.c src/print_utils.c src/reader.c src/code_blocks.h src/dumpers.h src/print_utils.h src/reader.h
	cc -o $@ src/code_blocks.c src/disasm.c src/dumpers.c src/print_utils.c src/reader.c

build:
	mkdir $@

clean:
	rm -rf build

