.PHONY: clean check test

headers = src/cblist.h src/cblock.h src/cbolist.h src/cborigin.h src/dumpers.h src/finder.h src/gvar.h src/gvlist.h src/gvwvmap.h src/itable.h src/printu.h src/reader.h src/ref.h src/reflist.h src/register.h src/relocu.h src/slmacros.h src/srresult.h src/sslist.h src/stack.h src/version.h
sources = src/cblist.c src/cblock.c src/cbolist.c src/cborigin.c src/disasm.c src/dumpers.c src/finder.c src/gvlist.c src/gvwvmap.c src/itable.c src/printu.c src/reader.c src/ref.c src/reflist.c src/register.c src/relocu.c src/srresult.c src/sslist.c src/stack.c build/src/version.c

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
