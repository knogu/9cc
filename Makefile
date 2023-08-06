CFLAGS=-std=c11 -g -static

9cc: 9cc.c

test: 9cc
	./test.sh

clean:
	rm -f 9cc *.o *~ tmp*

dtest:
	docker run --rm --platform linux/amd64 -v ~/9cc:/9cc -w /9cc 9cc make test

.PHONY: test clean dtest
