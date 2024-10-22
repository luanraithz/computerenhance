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

typedef struct CondJMPOP {
    int unsigned op_code: 8;

    int value: 8;
} CondJMPOP;

typedef struct JMPOp {
    int unsigned op_code: 6;
    int unsigned size: 1;
    int unsigned pad: 1;
} JMPOp;


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
const int SUB_REG = 2; // 0010
const int CMP_REG = 3; // 0011

const int ACC_TO_MEMORY = 10; // 1100
const int MEMORY_TO_ACC = ACC_TO_MEMORY; // Both are the same code but they differ in a flag before the w
const int MEMORY_TO_ACC_FLAG = 0; 
const int ACC_TO_MEMORY_FLAG = 1; 
const int IMMEDIATE_TO_REG = 11; // 1011
const int IMD_TO_MEM_REG = 12; // 1100
                               //
const int JMP = 14;
const int COND_JMP = 7;
                               //

#define DEBUG(pattern, ...) if (argv[2] != NULL && argv[2][0] == 'D') { printf(pattern, __VA_ARGS__); }

int read16Bit(FILE* f, int* idx) {
    DoubleByteData* data = malloc(sizeof(DoubleByteData));
    fread(data, sizeof(DoubleByteData), 1, f);
    int value = data->value;
    (*idx) += sizeof(DoubleByteData);
    free(data);
    return value;
}

int read8Bit(FILE* f, int* idx) {
    ByteData* data = malloc(sizeof(ByteData));
    fread(data, sizeof(ByteData), 1, f);
    (*idx) += sizeof(ByteData);
    int value = data->value;
    free(data);
    return value;
}

typedef struct {
    int value;
    char* label;
} Displacement;

char* empty = "";

Displacement read_displacement(int mod, int rm, FILE* f, int* idx) {
    int value = 0;
    if (mod == 1) {
        value = read8Bit(f, idx);
    } else if (mod == 2 || (mod == 0 && rm == 6)) {
        value = read16Bit(f, idx);
    }
    char* disp = malloc(sizeof(char) * 100);
    if (value != 0 && value > 0) {
        sprintf(disp, " + %d", value);
        Displacement d = { .value = value, .label = disp };
        return d;
    } else if (value < 0) {
        sprintf(disp, " - %d", abs(value));
        Displacement d = { .value = value, .label = disp };
        return d;
    }
    free(disp);
    Displacement d = { .value = 0, .label = empty };
    return d;
}

enum ReferenceType {
    DirectAccess,
    Expression,
    Value
};

typedef struct {
    char* adrr;
    Displacement disp;
    int value;
    enum ReferenceType type;
} Reference;

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
                printf("mov %s, %d\n", destination, read16Bit(f, &idx));
            } else {
                fseek(f, idx, SEEK_SET);
                printf("mov %s, %d\n", destination, read8Bit(f, &idx));
            }

        } else if (opId->id == MEM_REG_TO_MEM_REG || opId->id == ADD_REG || opId->id == SUB_REG || opId->id == CMP_REG) {
            fseek(f, idx, SEEK_SET);
            RegToRegOperation* op = malloc(sizeof(RegToRegOperation));
            fread(op, sizeof(RegToRegOperation), 1, f);

            DEBUG("code: %d\n", op->op_code);
            switch (op->op_code) {
                case 11:
                case 15:
                case 1:
                    {

                        fseek(f, idx, SEEK_SET);
                        ImdToAccAddOperation* op = malloc(sizeof(ImdToAccAddOperation));
                        fread(op, sizeof(ImdToAccAddOperation), 1, f);
                        idx += sizeof(ImdToAccAddOperation);

                        int data = 0;
                        char* opName = op->op_code == 11 ? "sub": op->op_code == 15 ? "cmp" : "add";
                        if (op->w == 1) {
                            data = read16Bit(f, &idx);
                            printf("%s ax, %d\n", opName,  data);
                        } else {
                            data = read8Bit(f, &idx);
                            printf("%s al, %d\n", opName, data);
                        }

                    }
                    break;
                case 32:
                    {
                        fseek(f, idx, SEEK_SET);
                        ImdToRegMemAddOperation* op = malloc(sizeof(ImdToRegMemAddOperation));
                        fread(op, sizeof(ImdToRegMemAddOperation), 1, f);
                        idx += sizeof(ImdToRegMemAddOperation);

                        Displacement disp = read_displacement(op->mod, op->rm, f, &idx);

                        char* prefix = "";
                        if (op->mod != 3) {
                            if (op->s == op->w && op->w) {
                                prefix = "word";
                            } else {
                                prefix = "byte ";
                            }
                        }

                        int data = 0;
                        if (op->s == 1 && op->w != 1) {
                            data = read16Bit(f, &idx);
                        } else {
                            data = read8Bit(f, &idx);
                        }

                        DEBUG("mod: %d math_code:%d s:%d rm: %d w: %d\n", op->mod, op->math_code, op->s, op->rm, op->w);
                        char* opName = op->math_code == 7 ? "cmp": op->math_code == 5 ? "sub": "add";
                        if (op->mod == 1) {
                            char* dest = arr1[op->rm][op->w];
                            if (disp.value != 0) {
                                printf("%s [%s%s], %d\n", opName, dest, disp.label, data);
                            } else {
                                printf("%s %s, %d\n", opName, dest, data);
                            }
                        } else {
                            bool isRawReg = op->mod == 3;
                            if (isRawReg)  {
                                char* dest = arr[op->rm][op->w];
                                if (disp.value != 0) {
                                    printf("%s [%s%s], %d\n", opName, dest, disp.label, data);
                                } else {
                                    printf("%s %s, %d\n", opName, dest, data);
                                }
                            } else {
                                char* dest = arr1[op->rm][op->w];
                                if (op->rm == 6) {
                                    printf("%s %s [%d], %d\n", opName, prefix, disp.value, data);
                                } else {
                                    printf("%s %s [%s%s], %d\n", opName, prefix, dest, disp.label , data);
                                }
                            }
                        }
                        if (disp.value) free(disp.label);
                    }
                    break;
                case 34:
                case 10:
                case 0:
                case 14:
                    {
                        idx += sizeof(RegToRegOperation);
                        char* opName = op->op_code == 34 ? "mov": op->op_code == 10? "sub": op->op_code == 14 ? "cmp": "add";
                        DEBUG("cmd: %s mod: %d rm: %d reg: %d w: %d d: %d\n", opName, op->mod, op->rm, op->reg, op->w, op->d);
                        if (op->mod == 3) {
                            char* destination = arr[op->d == 1 ? op->reg : op->rm][op->w];
                            char* source = arr[op->d == 1 ? op->rm : op->reg][op->w];
                            printf("%s %s, %s\n", opName, destination, source);
                        } else  {
                            Displacement disp = read_displacement(op->mod, op->rm, f, &idx);
                            bool isTargetMemoryWrite = op->d == 0 ;
                            bool isDirectAccess = op->rm == 6 && op->mod == 0;

                            if (op->mod == 1) {
                                char* src = arr[op->reg][op->w];
                                char* dest = arr1[op->rm][op->w];
                                if (op->rm == 6 && op->w == 0) {
                                    dest = arr[op->reg][1];
                                }
                                if (op->d != 1)  {
                                    printf("%s [%s%s], %s\n", opName, dest, disp.label, src);
                                } else {
                                    printf("%s %s, [%s%s]\n", opName, src, dest, disp.label);
                                }
                            } else {
                                char* str = arr1[op->rm][op->mod];
                                char* dest = arr[(op->d == 0 || (op->mod == 0)) ? op->reg: op->rm][op->w];
                                if (isTargetMemoryWrite)  {
                                    if (isDirectAccess) {
                                        printf("%s [%d], %s\n", opName, disp.value, dest);
                                    } else {
                                        printf("%s [%s%s], %s\n", opName, str, disp.label, dest);
                                    }
                                } else {
                                    if (op->rm == 6 && op->mod == 0) {
                                        printf("%s %s, [%d]\n", opName, dest, disp.value);
                                    } else {
                                        printf("%s %s, [%s%s]\n", opName, dest, str, disp.label);
                                    }
                                }
                            }
                            if (disp.value) free(disp.label);
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
            Displacement disp = read_displacement(op->mod, op->rm, f, &idx); 

            int data = 0;
            char* prefix = NULL;
            if (op->w == 0) {
                // 8 bit data
                data = read8Bit(f, &idx);
                prefix = "byte";
            } else {
                // 16 bit data
                data = read16Bit(f, &idx);
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
                printf("mov [%s%s], %s %d\n", dest, disp.label, prefix, data);
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

        } else if (opId->id == JMP) {
            fseek(f, idx, SEEK_SET);
            JMPOp* jumpOp = malloc(sizeof(JMPOp));
            fread(jumpOp, sizeof(JMPOp), 1, f);
            idx += sizeof(JMPOp);

            int ipinc = 0;
            if (jumpOp->size == 1) {
                ipinc = read8Bit(f, &idx);
            } else {
                ipinc = read16Bit(f, &idx);
            }
            printf("JMP %d \n", ipinc);

        } else if (opId->id == COND_JMP) {
            char* opName = "";
            fseek(f, idx, SEEK_SET);
            CondJMPOP* jumpOp = malloc(sizeof(CondJMPOP));
            fread(jumpOp, sizeof(CondJMPOP), 1, f);
            idx += sizeof(CondJMPOP);
            DEBUG("code: %d \n", jumpOp->op_code);
            switch (jumpOp->op_code) {
                case 125: opName = "jnge"; break;
                case 128: opName = "jng"; break;
                case 114: opName = "jnae"; break;
                case 118: opName = "jna"; break;
                case 122: opName = "jpe"; break;
                case 112: opName = "jo"; break;
                case 117: opName = "jnz"; break;
                case 120: opName = "js"; break;
                case 126: opName = "jge"; break;
                case 129: opName = "jg"; break;
                case 115: opName = "jae"; break;
                case 119: opName = "jnp"; break;
                case 113: opName = "jno"; break;
            }
            printf("%s %d \n", opName, jumpOp->value);
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
