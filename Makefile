CFLAGS=-fPIC
CFLAGS+=-std=c11 -pedantic -Werror -Wformat
CFLAGS+=-Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition

# Debug flags
#CFLAGS+=-fsanitize=address
#LDFLAGS+=-fsanitize=address
#CFLAGS+=-g -O0
#CFLAGS+=-DENABLE_DEBUG
#CFLAGS+=-fno-omit-frame-pointer

# Performance flags
CFLAGS+=-O3
CFLAGS+=-fstack-protector-explicit
CFLAGS+=-flto

BIN=dump test_speed ovni2prv emu libovni.so ovnisync

all: $(BIN)

libovni.a: ovni.o
	ar -crs $@ $^

dump: ovni.o dump.o parson.o

test_speed: test_speed.c ovni.o parson.o

emu: emu.o emu_ovni.o emu_nosv.o emu_tampi.o emu_openmp.o ovni.o prv.o pcf.o parson.o chan.o

libovni.so: ovni.o parson.o
	$(LINK.c) -shared $^ -o $@

ovni2prv: ovni2prv.c ovni.o parson.o

ovnisync: ovnisync.c
	mpicc $(CFLAGS) $(LDFLAGS) -lm $^ -o $@

clean:
	rm -f *.o $(BIN)
