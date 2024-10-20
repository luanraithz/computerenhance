#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
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


#define RM unsigned int rm: 3
#define MOD unsigned int mod: 2
#define W unsigned int w: 1

#define PAD(n) unsigned int pad: n

#pragma pack(push,1)
typedef struct RegToRegOperation {
    unsigned int w: 1;
    unsigned int d: 1;
    unsigned int op_code: 6;

    unsigned int rm: 3;
    unsigned int reg: 3;
    unsigned int mod: 2;
} RegToRegOperation;


typedef struct ImdToRegOrMemOperation {
    unsigned int w: 1;
    unsigned int d: 1;
    unsigned int op_code: 6;

    unsigned int rm: 3;
    unsigned int pad1: 3;
    unsigned int mod: 2;

    unsigned int data: 8;
    unsigned int dataIfW: 8;

} ImdToRegOrMemOperation;

typedef struct ImdToRegOperation {
    unsigned int reg: 3;
    unsigned int w: 1;
    unsigned int op_code: 4;
} ImdToRegOperation;


typedef struct ImdToRegMemOperation {
    unsigned int w: 1;
    unsigned int s: 1;
    unsigned int op_code: 6; 

    RM;
    PAD(3);
    MOD;
} ImdToRegMemOperation;

typedef struct ImdToAccAddOperation {
    unsigned int w: 1;
    PAD(1);
    unsigned int op_code: 6; 
} ImdToAccAddOperation;

typedef struct ImdToRegMemAddOperation {
    unsigned int w: 1;
    unsigned int s: 1;
    unsigned int op_code: 6; 

    RM;
    unsigned int math_code: 3; 
    MOD;
} ImdToRegMemAddOperation;

typedef struct ByteData {
    int value: 8;
} ByteData;

typedef struct DoubleByteData {
    int value: 16;
} DoubleByteData;

typedef struct OpIdentifier {
    unsigned int pad: 4;
    unsigned int id: 4;
} OpIdentifier;

typedef struct AccumulatorToMemory  {
    W;
    unsigned int order: 1;
    PAD(6);

    unsigned int addr: 16;
} AccumulatorToMemory;

#pragma pack(pop)

char* arr[][2] = {
    { "al", "ax" },
    { "cl", "cx" },
    { "dl", "dx" },
    { "bl", "bx" },
    { "ah", "sp" },
    { "ch", "bp" },
    { "dh", "si" },
    { "bh", "di" },
};


char* arr1[][3] = {
    { "bx + si", "bx + si", "bx + si"},
    { "bx + di", "bx + di", "bx + di"},
    { "bp + si", "bp + si", "bp + si"},
    { "bp + di", "bp + di", "bp + di"},
    { "si", "si", "si"},
    { "di", "di", "di"},
    { "DIRECT", "bp", "bp"},
    { "bx", "bx", "bx"},
};

const int MEM_REG_TO_MEM_REG = 8; // 1000
const int ADD_REG = 0; // 0000
const int SUB_REG = 2; // 1000

const int ACC_TO_MEMORY = 10; // 1100
const int MEMORY_TO_ACC = ACC_TO_MEMORY; // Both are the same code but they differ in a flag before the w
const int MEMORY_TO_ACC_FLAG = 0; 
const int ACC_TO_MEMORY_FLAG = 1; 
const int IMMEDIATE_TO_REG = 11; // 1011
const int IMD_TO_MEM_REG = 12; // 1100
                               //
                               //

#define DEBUG(pattern, ...) if (argv[2] != NULL && argv[2][0] == 'D') { printf(pattern, __VA_ARGS__); }

int read16Bit(FILE* f) {
    DoubleByteData* data = malloc(sizeof(DoubleByteData));
    fread(data, sizeof(DoubleByteData), 1, f);
    int value = data->value;
    free(data);
    return value;
}

int read8Bit(FILE* f) {
    ByteData* data = malloc(sizeof(ByteData));
    fread(data, sizeof(ByteData), 1, f);
    int value = data->value;
    free(data);
    return value;
}

#define parse_displacement(mod, rm) {\
    if (mod == 1) { \
        value = read8Bit(f); \
        *idx = idx + sizeof(ByteData); \
    } else if (mod == 2 || (rm != NULL && *rm == 6)) { \
        value = read16Bit(f); \
        idx+= sizeof(DoubleByteData); \
    } \
    char disp[100] = ""; \
    if (value != 0 && value > 0) { \
        sprintf(disp, " + %d", value); \
    } else if (value < 0) { \
        sprintf(disp, " - %d", abs(value)); \
    } \
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("File is required\n");
        return 1; 
    }
    FILE* f = fopen(argv[1], "rb");
    if (f == NULL) {
        printf("File not found\n");
        return 1;
    }
    RegToRegOperation *op = malloc(sizeof(RegToRegOperation));
    OpIdentifier *opId = malloc(sizeof(OpIdentifier));
    int idx = 0;
    while (fread(opId, sizeof(OpIdentifier), 1, f)) {

        DEBUG("opId: %d\n", opId->id);
        if (opId->id == 9) {
            fseek(f, idx, SEEK_SET);
            idx += sizeof(ImdToRegOrMemOperation);
            printf("Memory to accumulator exp\n");
        } else if (opId->id == IMMEDIATE_TO_REG) {
            fseek(f, idx, SEEK_SET);
            int size = sizeof(ImdToRegOperation);
            ImdToRegOperation* regOp = malloc(size);
            fread(regOp, size, 1, f);
            idx += sizeof(ImdToRegOperation);

            char* destination = arr[regOp->reg][regOp->w];
            if (regOp->w) {
                fseek(f, idx, SEEK_SET);
                idx += sizeof(DoubleByteData);
                printf("mov %s, %d\n", destination, read16Bit(f));
            } else {
                fseek(f, idx, SEEK_SET);
                idx += sizeof(ByteData);
                printf("mov %s, %d\n", destination, read8Bit(f));
            }

        } else if (opId->id == MEM_REG_TO_MEM_REG || opId->id == ADD_REG || opId->id == SUB_REG) {
            fseek(f, idx, SEEK_SET);
            RegToRegOperation* op = malloc(sizeof(RegToRegOperation));
            fread(op, sizeof(RegToRegOperation), 1, f);

            DEBUG("code: %d\n", op->op_code);
            switch (op->op_code) {
                case 1:
                    {

                        fseek(f, idx, SEEK_SET);
                        ImdToAccAddOperation* op = malloc(sizeof(ImdToAccAddOperation));
                        fread(op, sizeof(ImdToAccAddOperation), 1, f);
                        idx += sizeof(ImdToAccAddOperation);

                        int data = 0;
                        if (op->w == 1) {
                            data = read16Bit(f);
                            idx+= sizeof(DoubleByteData);
                            printf("add ax, %d\n",  data);
                        } else {
                            data = read8Bit(f);
                            idx+= sizeof(ByteData);
                            printf("add al, %d\n",  data);
                        }

                    }
                    break;
                case 11:
                case 32:
                    {
                        fseek(f, idx, SEEK_SET);
                        ImdToRegMemAddOperation* op = malloc(sizeof(ImdToRegMemAddOperation));
                        fread(op, sizeof(ImdToRegMemAddOperation), 1, f);
                        idx += sizeof(ImdToRegMemAddOperation);

                        int value = 0;
                        if (op->mod == 1) {
                            // 8 bit displacement
                            value = read8Bit(f);
                            idx+= sizeof(ByteData);
                        } else if (op->mod == 2) {
                            // 16 bit displacement
                            value = read16Bit(f);
                            idx+= sizeof(DoubleByteData);
                        }
                        char disp[100] = "";
                        if (value != 0 && value > 0) {
                            sprintf(disp, " + %d", value);
                        } else if (value < 0) {
                            sprintf(disp, " - %d", abs(value));
                        }
                        int data = 0;
                        if (op->s == 1 && op->w != 1) {
                            data = read16Bit(f);
                            idx+= sizeof(DoubleByteData);
                        } else {
                            data = read8Bit(f);
                            idx+= sizeof(ByteData);
                        }

                        char* prefix = "";
                        if (op->mod != 3) {
                            if (op->s == op->w && op->w) {
                                prefix = "word";
                            } else {
                                prefix = "byte ";
                            }
                        }

                        DEBUG("mod: %d math_code:%d s:%d rm: %d w: %d\n", op->mod, op->math_code, op->s, op->rm, op->w);
                        // printf("add %s, %d\n", dest, data);
                        char* opName = op->math_code == 7 ? "cmp": op->math_code == 5 ? "sub": "add";
                        if (op->mod == 1) {
                                char* dest = arr1[op->rm][op->w];
                                printf("%s [%s%s], %d\n", opName, dest, disp, data);
                            } else {
                                bool isRawReg = op->mod == 3;
                                if (isRawReg)  {
                                    char* dest = arr[op->rm][op->w];
                                    printf("%s %s%s, %d\n",  opName, dest, disp, data);
                                } else {
                                    char* dest = arr1[op->rm][op->w];
                                    printf("%s %s [%s%s], %d\n", opName, prefix, dest, disp, data);
                                }
                            }
                    }
                    break;
                case 34:
                case 10:
                case 0:
                case 7:
                    {
                        idx += sizeof(RegToRegOperation);
                        char* opName = op->op_code == 34 ? "mov": op->op_code == 10? "sub": op->op_code == 7 ? "cmp": "add";
                        DEBUG("cmd: %s mod: %d rm: %d reg: %d w: %d d: %d\n", opName, op->mod, op->rm, op->reg, op->w, op->d);
                        if (op->mod == 3) {
                            char* destination = arr[op->d == 1 ? op->reg : op->rm][op->w];
                            char* source = arr[op->d == 1 ? op->rm : op->reg][op->w];
                            printf("%s %s, %s\n", opName, destination, source);
                        } else  {
                            int value = 0;
                            bool isTargetMemoryWrite = op->d == 0 ;
                            if (op->mod == 1) {
                                value = read8Bit(f);
                                idx+= sizeof(ByteData);
                            } else if (op->mod == 2 || op->rm == 6) {
                                value = read16Bit(f);
                                idx+= sizeof(DoubleByteData);
                            }
                            char disp[100] = "";
                            if (value != 0 && value > 0) {
                                sprintf(disp, " + %d", value);
                            } else if (value < 0) {
                                sprintf(disp, " - %d", abs(value));
                            }

                            if (op->mod == 1) {
                                char* src = arr[op->reg][op->w];
                                char* dest = arr1[op->rm][op->w];
                                if (op->rm == 6 && op->w == 0) {
                                    dest = arr[op->reg][1];
                                }
                                if (op->d != 1)  {
                                    printf("%s [%s%s], %s\n", opName, dest, disp, src);
                                } else {
                                    printf("%s %s, [%s%s]\n", opName, src, dest, disp);
                                }
                            } else {
                                char* str = arr1[op->rm][op->mod];
                                char* dest = arr[(op->d == 0 || (op->mod == 0)) ? op->reg: op->rm][op->w];
                                if (isTargetMemoryWrite)  {
                                    printf("%s [%s%s], %s\n", opName, str, disp, dest);
                                } else {
                                    if (op->rm == 6 && op->mod == 0) {
                                        printf("%s %s, [%d]\n", opName, dest, value);
                                    } else {
                                        printf("%s %s, [%s%s]\n", opName, dest, str, disp);
                                    }
                                }
                            }
                        }
                    }
                    break;
                default:
                      idx += sizeof(RegToRegOperation);
                      DEBUG("Operation not handled %d \n", op->op_code);
            } 
        } else if (opId->id == IMD_TO_MEM_REG) {
            fseek(f, idx, SEEK_SET);
            ImdToRegMemOperation* op = malloc(sizeof(ImdToRegMemOperation));
            fread(op, sizeof(ImdToRegMemOperation), 1, f);
            idx += sizeof(ImdToRegMemOperation);
            int dispValue = 0;
            if (op->mod == 1) {
                dispValue = read8Bit(f);
                idx+= sizeof(ByteData);
            } else if (op->mod == 2 || op->rm == 6) {
                dispValue = read16Bit(f);
                idx+= sizeof(DoubleByteData);
            }
            char disp[100] = "";
            if (dispValue != 0 && dispValue > 0) {
                sprintf(disp, " + %d", dispValue);
            } else if (dispValue < 0) {
                sprintf(disp, " - %d", abs(dispValue));
            }

            int data = 0;
            char* prefix = NULL;
            if (op->w == 0) {
                // 8 bit data
                data = read8Bit(f);
                idx+= sizeof(ByteData);
                prefix = "byte";
            } else {
                // 16 bit data
                data = read16Bit(f);
                idx+= sizeof(DoubleByteData);
                prefix = "word";
            }

            DEBUG("mod: %d rm: %d w: %d\n", op->mod, op->rm, op->w);
            if (op->mod == 3)
            {
                char* dest = arr[op->rm][op->w];
                printf("mov %s, %s %d\n", dest, prefix, data);
            }
            else
            {
                char* dest = arr1[op->rm][op->w];
                printf("mov [%s%s], %s %d\n", dest, disp, prefix, data);
            }
        } else if (opId->id == ACC_TO_MEMORY) {
            fseek(f, idx, SEEK_SET);
            AccumulatorToMemory* op = malloc(sizeof(AccumulatorToMemory));
            fread(op, sizeof(AccumulatorToMemory), 1, f);
            idx += sizeof(AccumulatorToMemory);
            if (op->order == MEMORY_TO_ACC_FLAG)
            {
                printf("mov ax, [%d]\n", op->addr);
            }
            else
            {
                printf("mov [%d], ax\n", op->addr);
            }

        } else if (opId->id == 0) {
            printf("Add \n");
        } else {
            idx += sizeof(RegToRegOperation);
        }
    }
    free(op);
    free(opId);
    return 0;
}
