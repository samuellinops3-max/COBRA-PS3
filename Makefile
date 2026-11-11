TARGET		:= slither
TITLE		:= Slither.io Offline
APPID		:= SLIT00001
CONTENTID	:= UP0001-SLIT00001_00-0000000000000000

SOURCES		:= .
INCLUDES	:= .
DATA		:= .

CFILES		:= $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CXXFILES	:= $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
SFILES		:= $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.S))
BINFILES	:= $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))

OBJS		:= $(CFILES:.c=.o) $(CXXFILES:.cpp=.o) $(SFILES:.S=.o) $(BINFILES:.bin=.o)

include $(PSL1GHT)/ppu_rules