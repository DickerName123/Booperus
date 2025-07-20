CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -g

all: libfileutils.a liblogging.a

test: test_program
	./test_program

libfileutils.a: src/file_utils.o
	ar rcs $@ $^

liblogging.a: src/logging.o
	ar rcs $@ $^

src/file_utils.o: src/file_utils.c src/file_utils.h
	$(CC) $(CFLAGS) -c -o $@ src/file_utils.c

src/logging.o: src/logging.c src/logging.h
	$(CC) $(CFLAGS) -c -o $@ src/logging.c

test_program: test_program.c libfileutils.a liblogging.a
	$(CC) $(CFLAGS) test_program.c -L. -lfileutils -llogging -o $@

clean:
	rm -f src/*.o libfileutils.a liblogging.a test_program input.txt output.txt output2.txt

.PHONY: all test clean
