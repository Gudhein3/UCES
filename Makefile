.PHONY: test all

all: bin/build/uces-emu bin/build/uces-asm

test: all
	./bin/build/uces-asm asm example.ubc bin/prog.bin
	./bin/build/uces-emu bin/prog.bin

CFLAGS=-ggdb -std=c99 -Wswitch-enum

bin/build/uces-emu: bin/.gitignore src/uces.c src/dev.c src/uces.h bin/build/libasm.o bin/build/sv.o
	$(CC) $(CFLAGS) -o bin/build/uces-emu src/uces.c src/dev.c bin/build/libasm.o bin/build/sv.o

bin/build/libasm.o: bin/.gitignore src/libasm.c src/libasm.h src/sv.h src/uces.h
	$(CC) $(CFLAGS) -c -o bin/build/libasm.o src/libasm.c

bin/build/sv.o: bin/.gitignore src/sv.c src/sv.h
	$(CC) $(CFLAGS) -c -o bin/build/sv.o src/sv.c

bin/build/uces-asm: bin/.gitignore src/asm.c src/uces.h bin/build/libasm.o bin/build/sv.o
	$(CC) $(CFLAGS) -o bin/build/uces-asm src/asm.c bin/build/libasm.o bin/build/sv.o

bin/.gitignore:
	mkdir -p bin/build/
	echo "*" > bin/.gitignore
