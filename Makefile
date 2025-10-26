WARNS:=\
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

CFLAGS+=\
	--std=gnu23\
	-O3

SRC:=$(shell find . -name "*.c")
HEADERS:=$(shell find . -name "*.h")
OBJ:=$(SRC:.c=.o)
OUT:=capsmorse
CC:=cc

all: $(OUT)

$(OUT): $(OBJ)
	clang $(WARNS) $(CFLAGS) -o $(OUT) $(OBJ)
.c.o:
	$(CC) $(WARNS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OUT) $(OBJ)

.PHONY: all clean
.SUFFIXES: .c .o .h
