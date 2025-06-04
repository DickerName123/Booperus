CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -g

all: libfileutils.a

test: test_program
	./test_program

libfileutils.a: src/file_utils.o
	ar rcs $@ $^

src/file_utils.o: src/file_utils.c src/file_utils.h
	$(CC) $(CFLAGS) -c -o $@ src/file_utils.c

test_program: test_program.c libfileutils.a
	$(CC) $(CFLAGS) test_program.c -L. -lfileutils -o $@

clean:
	rm -f src/*.o libfileutils.a test_program input.txt output.txt output2.txt

.PHONY: all test clean
