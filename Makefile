.RECIPEPREFIX = >
CC = ppu-gcc
TARGET = slither

all: $(TARGET).pkg

$(TARGET).elf: main.c
> $(CC) main.c -o $(TARGET).elf

$(TARGET).self: $(TARGET).elf
> make_self $(TARGET).elf $(TARGET).self

$(TARGET).pkg: $(TARGET).self PARAM.SFO
> mkdir -p pkg/USRDIR
> cp $(TARGET).self pkg/USRDIR/EBOOT.BIN
> cp PARAM.SFO pkg/PARAM.SFO
> make_package_npdrm -c -x pkg/ $(TARGET).pkg

clean:
> rm -rf pkg $(TARGET).elf $(TARGET).self $(TARGET).pkg