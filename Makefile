.PHONY: clean

headers = src/code_blocks.h src/dumpers.h src/global_variables.h src/print_utils.h src/reader.h src/registers.h
sources = src/code_blocks.c src/disasm.c src/dumpers.c src/global_variables.c src/print_utils.c src/reader.c src/registers.c

build/disasm: build $(sources) $(headers)
	cc -o $@ $(sources)

build:
	mkdir $@

clean:
	rm -rf build

