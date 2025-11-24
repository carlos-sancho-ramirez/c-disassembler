.PHONY: clean check testDebug testRelease

headers = src/cblist.h src/cblock.h src/cbolist.h src/cborigin.h src/counter.h src/dumpers.h src/finder.h src/gvar.h src/gvlist.h src/gvwvmap.h src/itable.h src/printd.h src/printu.h src/reader.h src/ref.h src/reflist.h src/register.h src/relocu.h src/slmacros.h src/srresult.h src/sslist.h src/stack.h src/version.h
sources = src/cblist.c src/cblock.c src/cbolist.c src/cborigin.c src/counter.c src/disasm.c src/dumpers.c src/finder.c src/gvlist.c src/gvwvmap.c src/itable.c src/printu.c src/reader.c src/ref.c src/reflist.c src/register.c src/relocu.c src/srresult.c src/sslist.c src/stack.c
sourcesDebug = src/printd.c build/src/debug/version.c
sourcesRelease = build/src/release/version.c

build/bin/debug/disasm: build/bin/debug $(sources) $(sourcesDebug) $(headers)
	cc -DDEBUG=1 -g -O0 -std=c89 -pedantic -o $@ $(sources) $(sourcesDebug)

build/bin/release/disasm: build/bin/release $(sources) $(sourcesRelease) $(headers) $(headersRelease)
	cc -O2 -std=c89 -pedantic -o $@ $(sources) $(sourcesRelease)

build/src/debug/version.c: build/src/debug .git/HEAD
	echo "#include \"../../../src/version.h\"" > $@
	echo "" >> $@
	echo "const char *application_name_and_version = \"Disassembler 1.0.0-SNAPSHOT"`git rev-parse --short HEAD`"-Debug\\\\n\";" >> $@

build/src/release/version.c: build/src/release .git/HEAD
	echo "#include \"../../../src/version.h\"" > $@
	echo "" >> $@
	echo "const char *application_name_and_version = \"Disassembler 1.0.0-SNAPSHOT"`git rev-parse --short HEAD`"\\\\n\";" >> $@

build/bin/debug: build/bin
	mkdir -p $@

build/bin/release: build/bin
	mkdir -p $@

build/bin: build
	mkdir -p $@

build/src/debug: build/src
	mkdir -p $@

build/src/release: build/src
	mkdir -p $@

build/src: build
	mkdir -p $@

build/test/debug/samples/bin/%.asm: samples/bin/%.com build/bin/debug/disasm build/test/debug/samples/bin
	build/bin/debug/disasm -f bin -i $< -o $@

build/test/release/samples/bin/%.asm: samples/bin/%.com build/bin/release/disasm build/test/release/samples/bin
	build/bin/release/disasm -f bin -i $< -o $@

build/test/debug/samples/bin: build/test/debug/samples
	mkdir -p $@

build/test/release/samples/bin: build/test/release/samples
	mkdir -p $@

build/test/debug/samples: build/test/debug
	mkdir -p $@

build/test/release/samples: build/test/release
	mkdir -p $@

build/test/debug: build/test
	mkdir -p $@

build/test/release: build/test
	mkdir -p $@

build/test: build
	mkdir -p $@

build:
	mkdir -p $@

check: $(sources) $(sourcesDebug) $(sourcesRelease) $(headers)
	editorconfig-checker

testDebug: build/test/debug/samples/bin/hello.asm build/test/debug/samples/bin/timer.asm
	cmp test/samples/bin/hello.asm build/test/debug/samples/bin/hello.asm
	cmp test/samples/bin/timer.asm build/test/debug/samples/bin/timer.asm

testRelease: build/test/release/samples/bin/hello.asm build/test/release/samples/bin/timer.asm
	cmp test/samples/bin/hello.asm build/test/release/samples/bin/hello.asm
	cmp test/samples/bin/timer.asm build/test/release/samples/bin/timer.asm

clean:
	rm -rf build
