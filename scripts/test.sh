#!/bin/sh

FILE=crimlang_dbg
if [ ! -f "$FILE" ]; then
	make clean 
	make
fi

tests/test.py $@

