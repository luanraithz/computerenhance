#!/bin/sh


asm_files=$(ls test-cases/*.asm)
gcc ./8086.c

for file in $asm_files; do
    t=$(mktemp)
    bi=$(mktemp)
    t_bi=$(mktemp)
    nasm $file -o $bi || echo "Bin failed"
    ./a.out $bi > $t
    echo "--- $file ----"
    (nasm $t -o $t_bi && diff $bi $t_bi && echo " - Binary matched" || (echo "Filed to compile script output " && cat $t)) ||  diff -y --suppress-common-lines $t $file
done
