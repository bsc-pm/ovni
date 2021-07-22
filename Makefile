CFLAGS=-fPIC

#CFLAGS+=-fsanitize=address
#LDFLAGS+=-fsanitize=address
#CFLAGS+=-g -O0
CFLAGS+=-O3
CFLAGS+=-fstack-protector-explicit

BIN=dump libovni.a prvth test_speed

all: $(BIN)

libovni.a: ovni.o
	ar -crs $@ $^

dump: ovni.o dump.o

test_speed: test_speed.c ovni.o

clean:
	rm -f *.o $(BIN)
