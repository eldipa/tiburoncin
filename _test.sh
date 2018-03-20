#!/bin/bash

for baseline_file in regress/$1*.py.baseline; do
	test_file="regress/$(basename $baseline_file .baseline)"
	if [ ! -f "$baseline_file" ]; then
		continue
	fi

        _name=$(echo -n "$baseline_file" | cut -c 1-64)
        _off=$((${#_name} - 64 + 1))

        printf "%s %*s" "${_name}" ${_off} ""
	python "$test_file" > regress/stage
	sed 's/127.0.0.1:[0-9]\+/<addr>/g' regress/stage | \
		sed 's/.\[[0-9]\{1,2\}m//g' > regress/stage.san

	diff -d regress/stage.san "$baseline_file" > regress/diff
	if [ $? -eq 0 ]; then
		echo -e "\033[32mPASS\033[0m"
	else
		echo -e "\033[31mFAIL\033[0m"
		cat regress/diff
	fi

	rm regress/diff
	rm regress/stage
	rm regress/stage.san
done
