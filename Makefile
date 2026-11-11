.RECIPEPREFIX = >
CC = gcc
CFLAGS = -Wall -O2
TARGET = slither

all: $(TARGET)
$(TARGET): main.c
> $(CC) $(CFLAGS) main.c -o $(TARGET)

clean:
> rm -f $(TARGET)