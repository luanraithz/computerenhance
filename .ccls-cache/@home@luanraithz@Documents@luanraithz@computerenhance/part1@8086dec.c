#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdbool.h"
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include "./dec.h"
/**
 * Instructions vary from 1 to 6 bytes in length.
 * The important part of the decoding is in the first 2 bytes.
 *
 * First six bits identifies the basic instruction type: ADD XOR, etc,
 * The following bit (D) specifies the Direction, 1 = the reg field in the second byte identifies the destination.
 *  0 = the reg field identifies the source.
 * The next bit (W) is for differentiate between word(1) and byte(0) operation.
 * W = 1 -> 16 bit data
 * W = 0 -> 8 bit data
 

    Load from memory into bx
    MOV BX, [75]

    write bx to memory 
    MOV [75], BX

    Where 75 is an arbitrary memory address
 *
 * registers/accumulator ending with x are 16 bits, in other words, it includes both the high and low parts 
 */

/**
 *
 * SORRY for what I wrote down here, oh boy that's bad
 */

#define DEBUG(pattern, ...) if (argv[2] != NULL && argv[2][0] == 'D') { printf(pattern, __VA_ARGS__); }

char* empty = "";


char* render_reference(Reference* ref) {
    char* str = malloc(sizeof(char) * 30);
    switch (ref->type) {
        case Value:
            if (ref->prefix != NULL) {
                sprintf(str,"%s %d", ref->prefix, ref->value);
            } else {
                sprintf(str,"%d", ref->value);
            }
            break;
        case DirectAccess:
            {
                if (ref->prefix != NULL) {
                    sprintf(str,"%s [%d]", ref->prefix, ref->value);
                } else {
                    sprintf(str,"[%d]", ref->value);
                }
            }
            break;
        case Raw:
            sprintf(str,"%s", ref->addr);
            break;
        case Expression:
            {
                if (ref->disp != NULL && ref->disp->value != 0) {
                    if (ref->prefix != NULL) {
                        sprintf(str,"%s [%s%s]", ref->prefix, ref->addr, ref->disp->label);
                    } else {
                        sprintf(str,"[%s%s]", ref->addr, ref->disp->label);
                    }
                } else {
                    if (ref->prefix != NULL) {
                        sprintf(str,"%s [%s]", ref->prefix, ref->addr);
                    } else {
                        sprintf(str,"[%s]", ref->addr);
                    }
                }
            }
    }
    return str;
}

void render_op(Operation* op) {
    // memory leak
    if (op->right) {
        printf("%s %s, %s\n", op->name, render_reference(op->left), render_reference(op->right));
    } else {
        printf("%s %s\n", op->name, render_reference(op->left));
    }
}

// Linear search :pepege:
JumpLabel* find_possible_jump_for_location(JumpLabel* initial, int idx) {
    JumpLabel* itercur = initial;
    while (itercur != NULL) {
        if (itercur->idx == idx) {
            return itercur;
        }
        itercur = itercur->next;
    }
    return NULL;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("File is required\n");
        return 1; 
    }
    Memory* mem = malloc(sizeof(Memory));
    int size = 0;
    {
        FILE* f = fopen(argv[1], "rb");
        if (f == NULL) {
            printf("File not found\n");
            free(mem);
            return 1;
        }
        fseek(f, 0l, SEEK_END);
        size = ftell(f);
        rewind(f);
        fread(mem, size, 1, f);
        fclose(f);
    }
    DecodedOperations* dec = decode(mem, size);
    Operation* itercur = dec->initial;
    while (itercur != NULL) {
        JumpLabel* jmp = dec->jmpMap[itercur->currentIdx];
        if (jmp != NULL) {
            printf("%s:\n", jmp->label);
        }
        render_op(itercur);
        itercur = itercur->next;
    }
    free(mem);
    return 0;
}
