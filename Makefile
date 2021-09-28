CFLAGS=-fPIC

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

emu: emu.o emu_ovni.o emu_nosv.o ovni.o prv.o pcf.o parson.o

libovni.so: ovni.o parson.o
	$(LINK.c) -shared $^ -o $@

ovni2prv: ovni2prv.c ovni.o parson.o

ovnisync: ovnisync.c
	mpicc $(CFLAGS) $(LDFLAGS) -lm $^ -o $@

clean:
	rm -f *.o $(BIN)
