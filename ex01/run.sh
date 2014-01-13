#!/bin/bash

# compilers
CC=gcc
PYC=python
GOC=gccgo

# number of runs
N=20

echo >results.txt

# C processing
if type $CC >/dev/null 2>&1
then
	echo C >tmp1
	$CC -std=c99 -g -lpthread -o sum_c sum.c
	for I in `seq 1 $N`; do ./sum_c >>tmp1; done
	paste results.txt tmp1 >tmp2
	cp tmp2 results.txt
else
	echo "error: C compiler not available: "$CC 1>&2
fi

# GO processing
if type $GOC >/dev/null 2>&1
then
	echo Go >tmp1
	$GOC -o sum_go sum.go
	for I in `seq 1 $N`; do ./sum_go >>tmp1; done
	paste results.txt tmp1 >tmp2
	cp tmp2 results.txt
else
	echo "error: Go compiler not available: "$GOC 1>&2
fi

# python processing
if type $PYC >/dev/null 2>&1
then
	echo python >tmp1
	for I in `seq 1 $N`; do $PYC sum.py >>tmp1; done
	paste results.txt tmp1 >tmp2
	cp tmp2 results.txt
else
	echo "error: Python compiler not available: "$PYC 1>&2
fi

rm -f tmp1 tmp2
