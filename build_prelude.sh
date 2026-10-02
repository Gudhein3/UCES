set -xe
./bin/build/uces-asm asm prelude.ubc bin/prelude.bin
./bin/build/uces-asm sym prelude.ubc bin/prelude.sym
