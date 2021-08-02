CFLAGS=-fPIC

# Debug CFLAGS
CFLAGS+=-fsanitize=address
LDFLAGS+=-fsanitize=address
CFLAGS+=-g -O0

# Performance CFLAGS
#CFLAGS+=-O3
#CFLAGS+=-fstack-protector-explicit
#CFLAGS+=-flto

BIN=dump test_speed ovni2prv emu libovni.so ovnisync

all: $(BIN)

libovni.a: ovni.o
	ar -crs $@ $^

dump: ovni.o dump.o

test_speed: test_speed.c ovni.o

emu: emu.o emu_ovni.o emu_nosv.o ovni.o prv.o

libovni.so: ovni.o
	$(LINK.c) -shared $^ -o $@

ovni2prv: ovni2prv.c ovni.o

ovnisync: ovnisync.c
	mpicc -lm $^ -o $@

clean:
	rm -f *.o $(BIN)
