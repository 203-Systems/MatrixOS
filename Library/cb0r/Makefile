
all: bin

test.c: test/test.c 
	gcc -std=gnu99 -Wall -Iinclude -o bin/test test/test.c src/cb0r.c

test: test.c
	@if ./bin/test ; then \
		echo "TESTS PASSED"; \
	else \
		echo "TESTS FAILED"; exit 1; \
	fi; \

bin: bin/cb0r.c 
	gcc -std=gnu99 -Wall -Iinclude -o bin/cb0r bin/cb0r.c  src/cb0r.c

clean:
	rm -f bin/cb0r bin/test

.PHONY: all test clean bin
