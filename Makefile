.PHONY: clean

build/disasm: build src/disasm.c src/code_blocks.c src/code_blocks.h
	cc -o $@ src/disasm.c src/code_blocks.c

build:
	mkdir $@

clean:
	rm -rf build

