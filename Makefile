.PHONY: clean

build/disasm: build src/disasm.c
	cc -o $@ src/disasm.c

build:
	mkdir $@

clean:
	rm -rf build

