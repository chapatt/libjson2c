CC=gcc
RM=rm
CFLAGS=-I.
DEPS=json2c.h

all: check libjson2c.a

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

libjson2c.a: json2c.o
	$(AR) rc $@ $<

tests: tests.c json2c.c $(DEPS)
	$(CC) -o $@ $< $(CFLAGS)

check: tests
	@./tests; \
	rc=$$?; \
	if [ $$rc -eq 0 ]; then \
		exit 0; \
	else \
		exit $$rc; \
	fi;

clean:
	$(RM) -f *.o *.a tests
