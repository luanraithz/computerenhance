#include "./dec.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
/**
 * Instructions vary from 1 to 6 bytes in length.
 * The important part of the decoding is in the first 2 bytes.
 *
 * First six bits identifies the basic instruction type: ADD XOR, etc,
 * The following bit (D) specifies the Direction, 1 = the reg field in the
 second byte identifies the destination.
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
 * registers/accumulator ending with x are 16 bits, in other words, it includes
 both the high and low parts
 */

/**
 *
 * SORRY for what I wrote down here, oh boy that's bad
 */

#define DEBUG(pattern, ...)                                                    \
    if (argv[2] != NULL && argv[2][0] == 'D') {                                \
        printf(pattern, __VA_ARGS__);                                          \
    }

char *empty = "";
char *plus = " + ";

char *join(RegisterExpression reg) {
    char *str = malloc(sizeof(char *) * 10);
    for (int i = 0; i < reg.size; i++) {
        if (i > 0) {
            strcat(str, plus);
        }
        strcat(str, reg.registers[i].label);
    }
    return str;
}

Memory valueMemory = {};
char *labels[] = {
    "AX", "BX", "CX", "DX", "SP", "BP", "SI", "DI",
};

void print_state() {
    for (int i = 0; i < 8; i++) {
        u16 *value = mem.registers[i];
        printf("%s: %x (%u)\n", labels[i], *value, *value);
    }
    printf("Flags: ZF %i; SF %i \n", flags.zf, flags.sf);

    printf("\n\nMEMORY\n\n");
    for (int i = 0; i < 1024 * 1024; i++) {
        if (valueMemory.data[i] != 0) {
            printf("%i:\t%i\n", &valueMemory.data[i] - valueMemory.data,
                   valueMemory.data[i]);
        }
    }
}

char *render_reference(Reference *ref) {
    char *str = malloc(sizeof(char) * 30);
    switch (ref->type) {
    case Value:
        if (ref->prefix != NULL) {
            sprintf(str, "%s %d", ref->prefix, ref->value);
        } else {
            sprintf(str, "%d", ref->value);
        }
        break;
    case DirectAccess: {
        if (ref->prefix != NULL) {
            sprintf(str, "%s [%d]", ref->prefix, ref->value);
        } else {
            sprintf(str, "[%d]", ref->value);
        }
    } break;
    case JmpLabel:
        sprintf(str, "%s", ref->label);
        break;
    case Raw:
        sprintf(str, "%s", join(ref->addr));
        break;
    case Expression: {
        if (ref->disp != NULL && ref->disp->value != 0) {
            if (ref->prefix != NULL) {
                sprintf(str, "%s [%s%s]", ref->prefix, join(ref->addr),
                        ref->disp->label);
            } else {
                sprintf(str, "[%s%s]", join(ref->addr), ref->disp->label);
            }
        } else {
            if (ref->prefix != NULL) {
                sprintf(str, "%s [%s]", ref->prefix, join(ref->addr));
            } else {
                sprintf(str, "[%s]", join(ref->addr));
            }
        }
    }
    }
    return str;
}

i16 *get_addr(Reference *ref) {
    switch (ref->type) {
    case DirectAccess: // TODO
        return 0;
        break;
    case Value:
        assert(false);
        return 0;
    case Expression: {
        int reg = ref->addr.size;
        int addr = 0;
        if (reg > 0) {
            for (int i = 0; i < reg; i++) {
                i16 *v = ref->addr.registers[i].addr;
                addr += *v;
            }
        }
        addr += ref->disp->value;
        return (i16 *)&valueMemory.data[addr];
        break;
    }
    case Raw:
        assert(ref->addr.size == 1);
        i16 *v = ref->addr.registers[0].addr;
        return v;
        break;
    }
}

i16 get_value(Reference *ref) {
    switch (ref->type) {
    case DirectAccess: // TODO
    {
        i16 v = 0;
        memcpy(&v, valueMemory.data + ref->value, 2);
        return v;
    } break;
    case Value:
        return (i16)ref->value;
    case Expression: {
        int reg = ref->addr.size;
        int addr = 0;
        if (reg > 0) {
            for (int i = 0; i < reg; i++) {
                i16 *v = ref->addr.registers[i].addr;
                addr += *v;
            }
        }
        addr += ref->disp->value;
        i16 v = 0;
        memcpy(&v, valueMemory.data + addr, 2);
        return v;
        break;
    }
    case Raw:
        // assert(ref->addr.size == 1);
        i16 *v = ref->addr.registers[0].addr;
        return *v;
        break;
    }
}

void fill_flags(int value, int previous) {
    flags.zf = value == 0;
    printf("flags:");
    if (flags.zf)
        printf("Z");

    int masked = value & 0x8000;
    flags.sf = masked != 0;
    if (flags.sf)
        printf("S");
}

Operation *find_at_position(Operation *initial, int idx) {
    Operation *itercur = initial;
    while (itercur != NULL) {
        if (itercur->currentIdx == idx) {
            return itercur;
        }
        itercur = itercur->next;
    }
    return NULL;
}

static u16 WidthMaskFor(bool wide) { return wide == 0 ? 0xff : 0xffff; }

int masked_value(Operation *op, u16 value) {
    return value & WidthMaskFor(op->wide);
}

int execute_op(Operation *op, Operation *initial) {
    int ip = op->currentIdx + op->size;

    if (op->name == jz) {
        if (flags.pf == 1) {
            return op->right->value;
        }
    } else if (op->name == jz) {
        if (flags.zf == 1) {
            return op->right->value;
        }
    } else if (op->name == jnz || op->name == loopnz) {
        if (flags.zf == 0) {
            return op->right->value;
        }
    } else if (op->name == MOV) {
        i16 v = get_value(op->right);
        i16 *dest = get_addr(op->left);
        int value = get_value(op->left);
        *dest = v;
        printf("%s:%01x->%01x;", op->left->addr.registers->label, value, v);
    } else if (op->name == ADD) {
        assert(op->left->addr.size == 1);
        i16 vr = get_value(op->right);
        i16 vl = get_value(op->left);
        i16 *addr = op->left->addr.registers[0].addr;
        int value = vr + vl;
        int currentValue = *addr;
        *addr = value;
        printf("%s:%x->%x;", op->left->addr.registers->label, vl, value);
        fill_flags(value, currentValue);
    } else if (op->name == SUB || op->name == CMP) {
        assert(op->left->addr.size == 1);
        u16 vr = get_value(op->right);
        u16 vl = get_value(op->left);
        u16 *addr = op->left->addr.registers[0].addr;
        u16 value = vl - vr;
        printf("%s:%x->%x;", op->left->addr.registers->label, vl, value);
        // CMP is kinda the same as a SUB, it just doesnt save the value
        int currentValue = *addr;
        if (op->name == SUB) {
            *addr = value;
        }
        fill_flags(value, currentValue);
    }
    return ip;
}
void render_op(Operation *op) {
    // memory leak
    if (op->right) {
        printf("%s %s, %s ", op->name, render_reference(op->left),
               render_reference(op->right));
    } else {
        printf("%s %s ", op->name, render_reference(op->left));
    }
}

// Linear search :pepege:
JumpLabel *find_possible_jump_for_location(JumpLabel *initial, int idx) {
    JumpLabel *itercur = initial;
    while (itercur != NULL) {
        if (itercur->idx == idx) {
            return itercur;
        }
        itercur = itercur->next;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("File is required\n");
        return 1;
    }
    Memory *mem = malloc(sizeof(Memory));
    int size = 0;
    {
        FILE *f = fopen(argv[1], "rb");
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
    DecodedOperations *dec = decode(mem, size);
    bool run = argv[2] && argv[2][0] == 'r';
    if (run) {
        int ip = 0;
        while (find_at_position(dec->initial, ip)) {
            int initialip = ip;
            Operation *op = find_at_position(dec->initial, ip);
            JumpLabel *jmp = dec->jmpMap[op->currentIdx];
            if (jmp != NULL) {
                printf("%s:\n", jmp->label);
            }
            render_op(op);
            int nextIp = execute_op(op, dec->initial);
            printf("\tip: 0x%01x->0x%01x ", ip, nextIp);
            printf("\n");
            ip = nextIp;
        }

    } else {

        Operation *itercur = dec->initial;
        while (itercur != NULL) {
            JumpLabel *jmp = dec->jmpMap[itercur->currentIdx];
            if (jmp != NULL) {
                printf("%s:\n", jmp->label);
            }
            render_op(itercur);
            printf("\n");
            itercur = itercur->next;
        }
    }

    if (run) {
        print_state();
    }
    free(mem);
    return 0;
}
