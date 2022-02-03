# Cool Herders
#
# Makefile
# (c)2001-2005 Mike Brent
#

TARGET = herders.elf
OBJS = cool.o sprite.o rand.o collide.o sheep.o levels.o disclaimer.o font.o sound.o vmu_logo.o \
score.o menu.o control.o audioramdisk.o pathfind.o musicgallery.o imagegallery.o creditssrc/creditsmain.o \
storymode.o cheat.o

all: rm-elf $(TARGET)

fuckingdemo: rm-elf herdersdemo.elf
demo: rm-elf herdersdemo.elf

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) romdisk.*

rm-elf:
	-rm -f $(TARGET) romdisk.*

romdisk_ver: $(OBJS) romdisk.o
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) romdisk.o $(OBJEXTRA) -loggvorbisplay -lpng -lz -lm -lkosutils $(KOS_LIBS)
	# Padding to save us from the dcload up to last 1023 bytes drop bug
	dd if=/dev/zero bs=1024 count=1 >>$(TARGET)

$(TARGET): $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) -Wl,-Map,herders.map $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) -loggvorbisplay -lpng -lz -lm -lkosutils $(KOS_LIBS)
	# Padding to save us from the dcload up to last 1023 bytes drop bug
	dd if=/dev/zero bs=1024 count=1 >>$(TARGET)

herdersdemo.elf: KOS_CFLAGS+=-DDEMO_BUILD
herdersdemo.elf: $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) -Wl,-Map,herders.map $(KOS_LDFLAGS) -L/home/tursi/Dreamcast/libmenu -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) -loggvorbisplay -lpng -lz -lm -lkosutils -lmenu -lbz2 $(KOS_LIBS)
	# Padding to save us from the dcload up to last 1023 bytes drop bug
	dd if=/dev/zero bs=1024 count=1 >>$(TARGET)

romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS) romdisk.o romdisk.img
	$(KOS_STRIP) $(TARGET)

elf: $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -e _start -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) -loggvorbisplay -lpng -lz -lm -lkosutils $(KOS_LIBS)
	$(KOS_STRIP) $(TARGET)
	# Without removing these sections (at least stack) the output file is too large!! :)
	$(KOS_OBJCOPY) -R .stack -R .comment -R .bss -R .sbss -O binary $(TARGET) $(TARGET).bin
	/home/tursi/Dreamcast/scramble $(TARGET).bin 1ST_READ.BIN
