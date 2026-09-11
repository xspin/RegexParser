#!/bin/bash

PARSER='build/regexv -d -c'

run() {
    return
    echo "$PARSER '$@'"
    $PARSER $@ || exit 1
    echo
}

run "abc123"

run ".a?b*c+|.?a??b*?c+?"

run "a{0}b{1,}c{2,3}|a{0}?b{1,}?c{2,3}?"

run "[0-9a-z]+([^abc7-7]??)"

run "(?:(no capture))(capture(group))+"

run '^(?=look ahead)(?!neg look ahead)(?<=look behind)(?<!neg look behind)$'

run '\\\.\+\?\*\||\[\]\(\)\{\}|\b\B\s\S\w\W\d|[\b\B\s\S\w\W\d]'

run '((((a)))(b))((c)(d))(((((e)[0-9]))))'

run '||| a|bcd|ef|'

run '^(?:\\.|[^\^$\\.|?*+()\[\]{}]+|(?:\(\?[:=!<>]?)?\(.*?\)|\[.*?\]|\{[0-9,]*\}|[|^$*+?{}\[\]().\\])+$'

run '^(?:\\w|\\d|\\s|\\b|[^\x00-\x7F]|[\u4e00-\u9fa5]|[a-zA-Z0-9_]|[^abc]|.){1,5}?(?:(ab)+|(?:cd)*|(\d{4}))(?=ef)(?!gh)(?<=ij)(?<!kl)\b\Z$'

run '[\xaa-\xFa\u1234-\uaBf0](\xaa-\xFa*\u1234?-\uaBf0+)'

run '^$^$|^$^$'

BIN='build/regexv -fd -c '

$BIN '(a[ab]c|b[bc]c|c[ac]c)'
$BIN '(aab|aac|aba|abc|aca|acb|baa|bac|bba|bbc|bca|bcb|caa|cab|cba|cbc)'
$BIN 'a(bc|cb)+a|b(ac|ca)+b|c(ab|ba)+c'
$BIN '(ab|ac|ba|bc|ca|cb){2,4}'
$BIN 'a+bc*|b+ca*|c+ab*'
$BIN '中+文*|文+c中*|c+中文*'
$BIN '中+文*|文+c中*|c+中文*' -u
