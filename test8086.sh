#!/bin/sh


asm_files=$(ls part1/test-cases/*.asm)

for file in $asm_files; do
    t=$(mktemp)
    bi=$(mktemp)
    nasm $file -o $bi
    ./a.out $bi > $t
    echo $file
    diff -y --suppress-common-lines $t $file
done
