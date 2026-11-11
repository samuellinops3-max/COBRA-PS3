TARGET		:= slither
TITLE		:= Slither.io Offline
APPID		:= SLIT00001
CONTENTID	:= UP0001-SLIT00001_00-SLITHER0000000001

OBJS		:= main.o

include $(PSL1GHT)/ppu_rules

all: $(TARGET).elf $(TARGET).self $(TARGET).pkg

$(TARGET).elf: $(OBJS)
	$(LD) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).self $(TARGET).pkg