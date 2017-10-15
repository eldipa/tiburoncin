CODESTD_FLAGS=-std=c99 -pedantic -Wall -Werror

compile:
	gcc ${CODESTD_FLAGS} -O2 -o tiburoncin *.c
	chmod u+x tiburoncin

test: compile
	make _run_test

memcheck: compile
	mv tiburoncin tiburoncin.bin
	@echo '#!/bin/bash \n valgrind --quiet --leak-check=summary --log-file=valgrind-%p.out ./tiburoncin.bin $$@' > tiburoncin
	chmod u+x tiburoncin
	make _run_test
	mv tiburoncin.bin tiburoncin
	cat valgrind*.out

_run_test:
	./test.sh

coverage: clean
	gcc -fprofile-arcs -ftest-coverage ${CODESTD_FLAGS} -o tiburoncin *.c
	make _run_test
	gcov *.c

clean:
	rm -f *.o *.gcov *.gcno *.gcda tiburoncin tiburoncin.bin valgrind*.out dump.dtos dump.stod
