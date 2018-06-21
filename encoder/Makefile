CC := gcc
CFLAGS := -Wall -O2 -g -march=native -ftree-vectorise -pipe -fomit-frame-pointer
CXXFLAGS := $(CFLAGS)

.PHONY: all clean
all: monorale_encoder

monorale_encoder: monorale_encoder.c
	$(CC) $(CFLAG) -o $@ $^

clean:
	rm -f monorale_encoder

