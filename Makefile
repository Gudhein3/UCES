.PHONY: test all clear

source_files := $(shell find src/ -iname "*.c" ! -iname "emu.c" ! -iname "asm.c")
dependency_files := $(source_files:src/%.c=bin/build/%.d)
all: bin/build/uces-emu bin/build/uces-asm

test: all
	./bin/build/uces-asm asm test.ubc bin/test.bin
	./bin/build/uces-emu bin/test.bin

CC=gcc
CFLAGS=-ggdb -std=c99 -Wswitch-enum
OBJECTS4EMU=bin/build/cpu.o bin/build/dev.o bin/build/libasm.o bin/build/mmu.o bin/build/strings.o bin/build/uces.o
OBJECTS4ASM=bin/build/libasm.o bin/build/uces.o bin/build/strings.o

bin/build/%.o: src/%.c bin/.gitignore
	$(CC) $(CFLAGS) -c -o $@ $<

bin/build/%.d: src/%.c bin/.gitignore
	$(CC) $(CFLAGS) -c -MD -MF $@ -o bin/build/$*.o $<

bin/build/uces-emu: bin/.gitignore src/emu.c $(OBJECTS4EMU)
	$(CC) $(CFLAGS) -o bin/build/uces-emu src/emu.c $(OBJECTS4EMU) -lSDL3

bin/build/uces-asm: bin/.gitignore src/asm.c $(OBJECTS4ASM)
	$(CC) $(CFLAGS) -o bin/build/uces-asm src/asm.c $(OBJECTS4ASM)

bin/.gitignore:
	mkdir -p bin/build/
	echo "*" > bin/.gitignore

clear:
	rm -rf bin/

include $(dependency_files)
