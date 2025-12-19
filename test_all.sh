#!/bin/bash

make -j4 || exit $?

REGEX_PATH=examples/regex.txt

cat ${REGEX_PATH} | build/regexparser -c -fh -o examples/example.html
cat ${REGEX_PATH} | build/regexparser -c -fs -o examples/example.svg
cat ${REGEX_PATH} | build/regexparser -fg -o examples/example.graph.txt
cat ${REGEX_PATH} | build/regexparser -ft -o examples/example.tree.txt

cat ${REGEX_PATH} | build/regexparser -fg -c -u
