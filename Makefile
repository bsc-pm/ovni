CFLAGS=-g -O0 -fPIC

CFLAGS+=-fsanitize=address
LDFLAGS+=-fsanitize=address

BIN=dump libovni.a

all: $(BIN)

libovni.a: ovni.o
	ar -crs $@ $^

dump: ovni.o dump.o

clean:
	rm -f *.o $(BIN)
