# Cool Herders end creditz
# Coding by Rabitguy

OBJS = main.o

TARGET = credits.elf

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) romdisk_boot.*

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS) romdisk_boot.o
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) $(KOS_LIBS) romdisk_boot.o -ltremor -lkallisti -lpng -lz -lm -lgcc

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

reset:
	dc-tool -t 192.168.128.14 -r

romdisk_boot.img:
		$(KOS_GENROMFS) -f $@ -d romdisk_boot -v

romdisk_boot.o: romdisk_boot.img
		$(KOS_BASE)/utils/bin2o/bin2o $< romdisk_boot $@
