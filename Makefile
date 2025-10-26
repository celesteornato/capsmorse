WARNS=\
    -Wall\
    -Wextra\
    -Wpedantic\
    -Wunused\
    -Wfloat-equal\
    -Wundef\
    -Wshadow\
    -Wpointer-arith\
    -Wcast-align\
    -Wstrict-prototypes\
    -Wstrict-overflow=5\
    -Wwrite-strings\
    -Wcast-qual\
    -Wswitch-default\
    -Wswitch-enum\
    -Wconversion\
    -Wvla\
    -Wunreachable-code

CFLAGS+= \
	--std=gnu23\
	-O3

SRC=main.c
OUT=capsmorse.out

all: $(OUT)

$(OUT): $(SRC)
	clang $(WARNS) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f main.out 

.PHONY: all clean
.SUFFIXES: .c .o
