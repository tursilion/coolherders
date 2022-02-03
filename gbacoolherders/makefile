
include ./master.mak

#
# Set the name of your desired GBA image name here
#
PROGNAME=CoolHerders
GAMETITLE=COOL_HERDERS

#
# Compile using Krawall software (set to yes or no) ?
# Also specify if Krawall is registered (yes or no)
#
USE_KRAWALL=no
KRAWALL_IS_REGISTERED=no
KRAWALL_FILES=

# Specify a custom lnkscript - need both of these lines
USE_CUSTOM_LNKSCRIPT=yes
CUSTOM_LNKSCRIPT=lnkscript

#
# Set a list of files you want to compile
# Data files are set as obj so they aren't cleaned out by the standard makefile
#
OFILES += main.o main_ram.o ingame.obj win.obj title.obj titlepic.obj notice.obj
#ADD_LIBS = -lz -lpng

##########################################################################################
# Standard Makefile targets start here
##########################################################################################
all : $(PROGNAME).$(EXT) clean

##########################################################################################
# Custom  Makefile targets start here
##########################################################################################

include gsm/makegsm.inc
include tursi/maketursi.inc
include binky/makebinky.inc

# IWRAM file
main_ram.o : main_ram.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -I c:/work/ham/gcc-arm/include -I c:/work/ham/gcc-arm/arm-thumb-elf/include -I c:/work/ham/include -I c:/work/ham/system -c -DNDEBUG -O3 -marm -mthumb-interwork -Wall -save-temps -funroll-loops -fverbose-asm -nostartfiles main_ram.c -o main_ram.o

# Music
title.obj : coolherders_16.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles coolherders_16.c -o title.obj

win.obj : jakoshadows_8.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles jakoshadows_8.c -o win.obj

ingame.obj : sheep_16.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles sheep_16.c -o ingame.obj

# GFX
titlepic.obj : splash.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles splash.c -o titlepic.obj
	
notice.obj : notice.c
	c:/work/ham/gcc-arm/bin/arm-thumb-elf-gcc.exe -c -I c:/work/ham/include -DNDEBUG -mthumb-interwork -Wall -nostartfiles notice.c -o notice.obj

#
# Most Makefile targets are predefined for you, suchas
# vba, clean ... in the following file
#
include $(HAMDIR)/system/standard-targets.mak

