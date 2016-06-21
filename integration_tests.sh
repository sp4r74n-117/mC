#!/bin/bash
BASE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
MCC="$BASE/mC.sh --libs $BASE/lib/ --no-unit-tests"
GCC="gcc -std=gnu99 -O0 -Wall -m32 -mfpmath=sse -march=pentium4 -x c"

function runtime_test()
{
	if [ ! -e "$1" ]
	then
		printf "\033[0;31mFAILED:\033[0m input file $1 is missing!\n"
		return
	fi
	$2 < $1 > "$1.mcc"
	$3 < $1 > "$1.gcc"
	diff -w -B -E -i "$1.mcc" "$1.gcc"
	if [ $? -ne 0 ]
	then
		rm -f "$1.mcc"
		rm -f "$1.gcc"
		printf "\033[0;31mFAILED:\033[0m expected and actual output differs!\n"
		return
	fi
	printf "\033[0;32mPASSED!\033[0m\n"
	rm -f "$1.mcc"
	rm -f "$1.gcc"
}

function integration_test()
{
	printf "\033[0;33mTEST:\033[0m $1\n"
	local input_file="$1.in"
	local output_file_mcc="$1.mcc.out"
	local output_file_gcc="$1.gcc.out"

	$MCC "--backend-$2" --output $output_file_mcc "$1"
	if [ $? -ne 0 ]
	then
		printf "\033[0;31mFAILED:\033[0m mcc compilation error!\n"
		return
	fi

	$GCC -o $output_file_gcc $BASE/lib/lib.c "$1"
	if [ $? -ne 0 ]
	then
		rm -f $output_file_mcc
		printf "\033[0;31mFAILED:\033[0m gcc compilation error!\n"
		return
	fi

	runtime_test $input_file $output_file_mcc $output_file_gcc
	rm -f $output_file_mcc
	rm -f $output_file_gcc
}

for backend in "simple" "regalloc"
do
	printf "\n\033[0;33mexecuting integration tests using backend: $backend\033[0m\n"
	for file in "$BASE/tests/snippets"/*.mC
	do
		integration_test "$file" "$backend"
	done
done
