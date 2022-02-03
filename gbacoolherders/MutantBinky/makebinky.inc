OFILES += binky/binktop.obj binky/mutantbink.obj binky/ergarg.obj binky/binky.o

# Files for the Binky intro
binky/binktop.obj : binky/binktop.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles binky/binktop.c -o binky/binktop.obj
binky/mutantbink.obj : binky/mutantbink.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles binky/mutantbink.c -o binky/mutantbink.obj
binky/ergarg.obj: binky/ergarg.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles binky/ergarg.c -o binky/ergarg.obj

