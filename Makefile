CC?=gcc
DEPS=json2c.h

all: check libjson2c.a

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

libjson2c.a: json2c.o
	$(AR) rc $@ $<

test: test.c json2c.c $(DEPS)
	$(CC) -o $@ $< $(CFLAGS)

check: test
	@./test; \
	rc=$$?; \
	if [ $$rc -eq 0 ]; then \
		exit 0; \
	else \
		exit $$rc; \
	fi;

clean:
	rm -f *.o *.a test
