#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdbool.h"
#include <assert.h>
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
char* plus = " + ";

char* join(RegisterExpression reg) {
    char* str = malloc(sizeof(char*) * 10 );
    for (int i = 0; i < reg.size; i++) {
        if (i > 0) {
             strcat(str, plus);
        }
        strcat(str, reg.registers[i].label);
    }
    return str;
}

char* labels[] =  { "AX", "BX", "CX", "DX", "SP", "BP", "SI", "DI", };

void print_state() {
    for (int i = 0; i < 8; i ++) {
        i16* value = mem.registers[i];
        printf("%s: %i\n", labels[i], *value);
    }
}


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
        case JmpLabel:
            sprintf(str,"%s", ref->label);
            break;
        case Raw:
            sprintf(str,"%s", join(ref->addr));
            break;
        case Expression:
            {
                if (ref->disp != NULL && ref->disp->value != 0) {
                    if (ref->prefix != NULL) {
                        sprintf(str,"%s [%s%s]", ref->prefix, join(ref->addr), ref->disp->label);
                    } else {
                        sprintf(str,"[%s%s]", join(ref->addr), ref->disp->label);
                    }
                } else {
                    if (ref->prefix != NULL) {
                        sprintf(str,"%s [%s]", ref->prefix, join(ref->addr));
                    } else {
                        sprintf(str,"[%s]", join(ref->addr));
                    }

                }
            }
    }
    return str;
}

i16 get_value(Reference *ref) {
    switch(ref->type) {
        case DirectAccess: // TODO 
            break;
        case Value:
            return (i16) ref->value;
        case Raw: 
            assert(ref->addr.size == 1);
            i16* v = ref->addr.registers[0].addr;
            return *v;
            break;
    }

}


void execute_op(Operation* op) {
    if (op->name == MOV) {
        i16 v = get_value(op->right);
        assert(op->left->addr.size == 1);
        i16* dest = op->left->addr.registers[0].addr;
        *dest = v;
    } else if (op->name == ADD) {
        assert(op->left->addr.size == 1);
        i16 vr = get_value(op->right);
        i16 vl = get_value(op->left);
        i16 *addr = op->left->addr.registers[0].addr;
        *addr = vr + vl;
    } else if (op->name == SUB) {
        assert(op->left->addr.size == 1);
        i16 vr = get_value(op->right);
        i16 vl = get_value(op->left);
        i16 *addr = op->left->addr.registers[0].addr;
        *addr = vr - vl;
    }
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
    bool run = argv[2] && argv[2][0] == 'r';
    while (itercur != NULL) {
        JumpLabel* jmp = dec->jmpMap[itercur->currentIdx];
        if (jmp != NULL) {
            printf("%s:\n", jmp->label);
        }
        render_op(itercur);
        if (run) {
            execute_op(itercur, &itercur);
        }
        itercur = itercur->next;
    }
    if (run) {
        print_state();
    }
    free(mem);
    return 0;
}
