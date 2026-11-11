.RECIPEPREFIX = >
TARGET = slither
TITLE = Slither.io Offline
APPID = SLIT00001
OBJS = main.o

include $(PSL1GHT)/ppu_rules

all: $(TARGET).pkg

$(TARGET).elf: $(OBJS)
> $(LD) $(OBJS) -o $@ $(LDFLAGS)

$(TARGET).self: $(TARGET).elf
> $(SELF) $< $@

$(TARGET).pkg: $(TARGET).self PARAM.SFO
> mkdir -p pkg/USRDIR
> cp $(TARGET).self pkg/USRDIR/EBOOT.BIN
> cp PARAM.SFO pkg/PARAM.SFO
> $(PSL1GHT)/bin/make_package_npdrm -c -x pkg/ $(TARGET).pkg

clean:
> rm -rf pkg $(OBJS) $(TARGET).elf $(TARGET).self $(TARGET).pkg