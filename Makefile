CFLAGS=-fPIC

# Debug CFLAGS
#CFLAGS+=-fsanitize=address
#LDFLAGS+=-fsanitize=address
CFLAGS+=-g -O0

# Performance CFLAGS
#CFLAGS+=-O3
#CFLAGS+=-fstack-protector-explicit
#CFLAGS+=-flto

BIN=dump libovni.a test_speed ovni2prv emu

all: $(BIN)

libovni.a: ovni.o
	ar -crs $@ $^

dump: ovni.o dump.o

test_speed: test_speed.c ovni.o

emu: emu.o emu_ovni.o emu_nosv.o ovni.o

ovni2prv: ovni2prv.c ovni.o

clean:
	rm -f *.o $(BIN)
