.POSIX:
CC = c99
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2 -g
LDFLAGS =
LDLIBS =
PREFIX = /usr/local

all: libsha3.a sha3sum sha3test

.SUFFIXES: .c .o
.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

libsha3.a: sha3.o
	ar rsv $@ sha3.o

sha3sum: sha3.o sha3sum.o
	$(CC) $(LDFLAGS) -o $@ sha3.o sha3sum.o $(LDLIBS)

sha3fuzz: sha3.c fuzz/sha3fuzz.c
	clang -g --std=c99 -fsanitize=fuzzer,address -O0 -I . -o $@ $^

sha3test: sha3.o sha3test.o
	$(CC) $(LDFLAGS) -o $@ sha3.o sha3test.o $(LDLIBS)

check: sha3test
	./sha3test

clean:
	rm -f *.o libsha3.a sha3sum sha3test sha3fuzz

install:
	mkdir -p $(DESTDIR)$(PREFIX)/include \
		$(DESTDIR)$(PREFIX)/lib \
		$(DESTDIR)$(PREFIX)/bin
	install sha3.h $(DESTDIR)$(PREFIX)/include
	install -m 755 libsha3.a $(DESTDIR)$(PREFIX)/lib
	install -m 755 sha3sum $(DESTDIR)$(PREFIX)/bin

uninstall:
	rm -f \
		$(DESTDIR)$(PREFIX)/include/sha3.h \
		$(DESTDIR)$(PREFIX)/lib/libsha3.a \
		$(DESTDIR)$(PREFIX)/bin/sha3sum

.PHONY: all check clean install uninstall
