CC := gcc
CFLAGS := -O2 -g -march=native -ftree-vectorize -pipe -fomit-frame-pointer
CXXFLAGS := $(CFLAGS)

.PHONY: all clean
all: monorale_encoder

monorale_encoder: src/monorale_encoder.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f monorale_encoder

