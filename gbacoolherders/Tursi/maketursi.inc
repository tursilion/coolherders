OFILES += tursi/tp1.obj tursi/tp2.obj tursi/thunder.obj tursi/presented.obj tursi/tursipic.o

# Files for the Tursi intro
tursi/tp1.obj : tursi/tp1.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles tursi/tp1.c -o tursi/tp1.obj
tursi/tp2.obj : tursi/tp2.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles tursi/tp2.c -o tursi/tp2.obj
tursi/thunder.obj: tursi/thunder16.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles tursi/thunder16.c -o tursi/thunder.obj
tursi/presented.obj: tursi/presented.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles tursi/presented.c -o tursi/presented.obj