OFILES += binky/thinkbink1.obj binky/thinkbink2.obj binky/binky.o

# Files for the Binky intro
binky/thinkbink1.obj : binky/thinkbink1.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles binky/thinkbink1.c -o binky/thinkbink1.obj
binky/thinkbink2.obj : binky/thinkbink2.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles binky/thinkbink2.c -o binky/thinkbink2.obj

