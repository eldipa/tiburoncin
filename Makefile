CODESTD_FLAGS=-std=c99 -pedantic -Wall -Werror

compile:
	gcc ${CODESTD_FLAGS} -O2 -o tiburoncin *.c
	chmod u+x tiburoncin

test: compile
	@make _run_test

memcheck: compile
	@echo "****************************************"
	@echo "Not supported yet: when valgrind is stopped (^Z), it seems that interrupt (^C) tiburoncin which breaks the tests."
	@echo "****************************************"
	exit 1
	mv tiburoncin tiburoncin.bin
	@echo '#!/bin/bash \n exec valgrind --quiet --leak-check=summary --log-file=valgrind-%p.out ../tiburoncin.bin $$@' > tiburoncin 2>&1
	chmod u+x tiburoncin
	make _run_test
	mv tiburoncin.bin tiburoncin
	cat valgrind*.out

_run_test: test-doc test-circular-buffer

test-doc:
	@hash byexample || if true; then echo "byexample is not installed, install it with 'pip install byexample', see https://byexamples.github.io/byexample/" ; exit 1; fi
	( cd docs ; byexample -l python,shell *.md )
	byexample -l shell README.md

test-circular-buffer:
	@hash byexample || if true; then echo "byexample is not installed, install it with 'pip install byexample', see https://byexamples.github.io/byexample/" ; exit 1; fi
	@hash cling || if true; then echo "cling is not installed, see https://github.com/root-project/cling" ; exit 1; fi
	byexample -l cpp circular_buffer.h

coverage: clean
	gcc -fprofile-arcs -ftest-coverage ${CODESTD_FLAGS} -o tiburoncin *.c
	make _run_test
	gcov *.c

clean:
	rm -f *.o *.gcov *.gcno *.gcda tiburoncin tiburoncin.bin valgrind*.out AtoB.dump BtoA.dump
