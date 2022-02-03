OFILES += gsm/combined.o

# Buld the GSM player in IWRAM
combined.o : combined.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -I c:/work/ham/gcc-arm/include -I c:/work/ham/gcc-arm/arm-thumb-elf/include -I c:/work/ham/include -I c:/work/ham/system -c -DNDEBUG -O3 -mthumb-interwork -Wall -save-temps -funroll-loops -fverbose-asm -nostartfiles combined.c -o combined.o
