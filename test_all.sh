#!/bin/bash

make -j4 || exit $?

REGEX_PATH=examples/regex.txt

cat ${REGEX_PATH} | build/regexv -c -fh -o examples/example.html
cat ${REGEX_PATH} | build/regexv -c -fs -o examples/example.svg
cat ${REGEX_PATH} | build/regexv -c -fx -u -o examples/example.xml
cat ${REGEX_PATH} | build/regexv -fg -o examples/example.graph.txt
cat ${REGEX_PATH} | build/regexv -ft -o examples/example.tree.txt

cat ${REGEX_PATH} | build/regexv -fg -c -u
