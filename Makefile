CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu11 -I libtslog/include -pthread

.PHONY: all clean test

all: test

lib:
	@echo "No static lib needed for this simple setup."

test: libtslog/src/libtslog.c tests/log_test.c
	$(CC) $(CFLAGS) libtslog/src/libtslog.c tests/log_test.c -o log_test

clean:
	rm -f log_test saida.log
