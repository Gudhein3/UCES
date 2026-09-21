.PHONY: test all clear

all: bin/build/uces-emu bin/build/uces-asm

test: all
	./bin/build/uces-asm asm example.ubc bin/prog.bin
	./bin/build/uces-emu bin/prog.bin

CC=gcc
CFLAGS=-ggdb -std=c99 -Wswitch-enum

bin/build/cpu.o: bin/.gitignore src/cpu.c src/cpu.h src/mmu.h src/uces.h src/libasm.h
	$(CC) $(CFLAGS) -c -o bin/build/cpu.o src/cpu.c

bin/build/dev.o: bin/.gitignore src/cpu.c src/cpu.h src/mmu.h src/dev.h src/uces.h
	$(CC) $(CFLAGS) -c -o bin/build/dev.o src/dev.c

bin/build/libasm.o: bin/.gitignore src/libasm.c src/libasm.h src/uces.h src/da.h src/cpu.h src/mmu.h src/strings.h
	$(CC) $(CFLAGS) -c -o bin/build/libasm.o src/libasm.c

bin/build/mmu.o: bin/.gitignore src/mmu.c src/mmu.h src/uces.h src/dev.h src/cpu.h
	$(CC) $(CFLAGS) -c -o bin/build/mmu.o src/mmu.c

bin/build/strings.o: bin/.gitignore src/strings.c src/strings.h src/da.h
	$(CC) $(CFLAGS) -c -o bin/build/strings.o src/strings.c

bin/build/uces.o: bin/.gitignore src/uces.c src/uces.h
	$(CC) $(CFLAGS) -c -o bin/build/uces.o src/uces.c

bin/build/uces-emu: bin/.gitignore src/main.c bin/build/cpu.o bin/build/dev.o bin/build/libasm.o bin/build/mmu.o bin/build/strings.o bin/build/uces.o src/uces.h src/libasm.h src/cpu.h src/dev.h
	$(CC) $(CFLAGS) -o bin/build/uces-emu src/main.c bin/build/cpu.o bin/build/dev.o bin/build/libasm.o bin/build/mmu.o bin/build/strings.o bin/build/uces.o -lSDL3

bin/build/uces-asm: bin/.gitignore src/asm.c bin/build/libasm.o bin/build/libasm.o bin/build/uces.o bin/build/strings.o
	$(CC) $(CFLAGS) -o bin/build/uces-asm src/asm.c bin/build/libasm.o bin/build/uces.o bin/build/strings.o

bin/.gitignore:
	mkdir -p bin/build/
	echo "*" > bin/.gitignore

clear:
	rm -rf bin/
