#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdbool.h"
#include <sys/stat.h>
#include <string.h>
#include <time.h>
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
    int unsigned pad: 1;
    int unsigned size: 1;
    int unsigned op_code: 6;
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


char* jnge = "jnge";
char* jng = "jng";
char* jnae = "jnae";
char* jna = "jna";
char* jpe = "jpe";
char* jo = "jo";
char* jnz = "jnz";
char* js = "js";
char* jge = "jge";
char* jg = "jg";
char* jz = "jz";
char* jae = "jae";
char* jnbe = "jnbe";
char* jnp = "jnp";
char* jno = "jno";
char* jns = "jns";
char* loop = "loop";
char* loopz = "loopz";
char* loopnz = "loopnz";
char* jcxz = "jcxz";


#define DEBUG(pattern, ...) if (argv[2] != NULL && argv[2][0] == 'D') { printf(pattern, __VA_ARGS__); }

typedef struct {
    int value;
    char* label;
    int size;
} Displacement;

char* empty = "";



int read2Bytes(void* mem_start) {
    int16_t value = 0;
    memcpy(&value, mem_start, 2);
    return value;
}

int readByte(void* mem_start) {
    int8_t value = 0;
    memcpy(&value, mem_start, 1);
    return value;
}

Displacement* read_displacement_ptr(int mod, int rm, void* mem_start) {
    int value = 0;
    int size = 0;
    if (mod == 1) {
        value = readByte(mem_start);
        size = 1;
    } else if (mod == 2 || (mod == 0 && rm == 6)) {
        value = read2Bytes(mem_start);
        size = 2 ;
    }
    char* disp = malloc(sizeof(char) * 100);
    Displacement* dispStr = malloc(sizeof(Displacement));
    if (value != 0 && value > 0) {
        sprintf(disp, " + %d", value);
    } else if (value < 0) {
        sprintf(disp, " - %d", abs(value));
    }
    dispStr->value = value;
    dispStr->label = disp;
    dispStr->size = size;
    return dispStr;
}

enum ReferenceType {
    DirectAccess,
    Expression,
    Raw,
    Value
};

char* MOV = "mov";
char* SUB = "sub";
char* ADD = "add";
char* CMP = "cmp";
char* JMP_LABEL = "jmp";
char* AX = "ax";
char* AL = "al";

typedef struct {
    char* addr;
    char* prefix;
    Displacement* disp;
    int value;
    enum ReferenceType type;
} Reference;

typedef struct Operation {
    char* name;
    struct Operation* previous;
    struct Operation* next;
    int currentIdx;
    int size;
    Reference* left;
    Reference* right;
} Operation;

typedef struct JumpLabel {
    int idx;
    int offset;
    char* label;
    struct JumpLabel* next;
} JumpLabel;


Operation* double_raw_operation(char* opName, int idx, int size, char* leftReg, char* rightReg) {
    Operation* op = malloc(sizeof(Operation));
    op->name = opName;
    op->currentIdx = idx;
    op->size = size;
    Reference* left = malloc(sizeof(Reference));
    op->left = left;
    left->addr = leftReg;
    left->type = Raw;
    Reference* right = malloc(sizeof(Reference));
    op->right = right;
    right->addr = rightReg;
    right->type = Raw;
    return op;
}
Operation* multi_reference_operation(char* opName, int idx, int size) {
    Operation* op = malloc(sizeof(Operation));
    op->name = opName;
    op->currentIdx = idx;
    op->size = size;
    Reference* left = malloc(sizeof(Reference));
    op->left = left;
    Reference* right = malloc(sizeof(Reference));
    op->right = right;
    return op;
}

Operation* jmp_operation(char* name, int data, int idx, int size, char* id) {
    Operation* op = malloc(sizeof(Operation));
    op->name = name;
    op->currentIdx = idx;
    op->size = size;
    Reference* left = malloc(sizeof(Reference));
    op->left = left;
    op->left->type = Raw;
    op->left->addr = id;
    return op;
}

char* word = "word";
char* byte = "byte";

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


typedef uint8_t u8;

typedef struct Memory {
    u8 data[1024 * 1024];
} Memory;

typedef int16_t i16;

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
    Operation* initial = NULL;
    Operation* current = NULL;
    JumpLabel** jmpMap = calloc(1000, sizeof(JumpLabel*));
    int jmpCount = 0;
    int bytesRead = 0; 
    for (int idx = 0; idx < size; idx+=bytesRead) {
        u8 opName = mem->data[idx] >> 4;
        bytesRead = 0;
        Operation* operation = NULL;
        if (opName == IMMEDIATE_TO_REG) {
            bytesRead = sizeof(ImdToRegOperation);
            ImdToRegOperation* regOp = malloc(bytesRead);
            memcpy(regOp, mem->data + idx, sizeof(ImdToRegOperation));

            operation = multi_reference_operation(MOV, idx, sizeof(ImdToRegOperation) + (regOp->w ? 2 : 1));

            char* destination = arr[regOp->reg][regOp->w];
            operation->left->addr = destination;
            operation->left->type = Raw;
            operation->right->type = Value;
            operation->right->value = regOp->w ? read2Bytes(mem->data+bytesRead+idx): readByte(mem->data+bytesRead+idx);
            bytesRead+= regOp->w?2: 1;
        } else if (opName == MEM_REG_TO_MEM_REG || opName == ADD_REG || opName == SUB_REG || opName == CMP_REG) {
            RegToRegOperation* op = malloc(sizeof(RegToRegOperation));
            DEBUG("idx: %d %d\n", idx, sizeof(RegToRegOperation));
            memcpy(op, mem->data+idx, sizeof(RegToRegOperation));
            bytesRead = 0;
            DEBUG("code: %d\n", op->op_code);
            switch (op->op_code) {
                case 11:
                case 15:
                case 1:
                    {
                        ImdToAccAddOperation* op = malloc(sizeof(ImdToAccAddOperation));
                        memcpy(op, mem->data + idx, sizeof(ImdToAccAddOperation));
                        char* opName = op->op_code == 11 ? SUB: op->op_code == 15 ? CMP : ADD;
                        bytesRead += sizeof(ImdToAccAddOperation);
                        operation = multi_reference_operation(opName, idx, sizeof(ImdToRegOperation) + (op->w == 1 ? 2: 1));
                        operation->left->addr = op->w == 1 ? AX: AL; 
                        operation->left->type = Raw;
                        int value = 0;
                        if (op->w == 1)  {
                            value = read2Bytes(mem->data+bytesRead+idx);
                        } else {
                            value = readByte(mem->data+bytesRead+idx);
                        }
                        operation->right->value = value;

                        operation->right->type = Value;
                        bytesRead+= op->w == 1 ? 2: 1;
                    }
                    break;
                case 32:
                    {
                        ImdToRegMemAddOperation* op = malloc(sizeof(ImdToRegMemAddOperation));
                        memcpy(op, mem->data+ idx, sizeof(ImdToRegMemAddOperation));
                        bytesRead+=sizeof(ImdToRegMemAddOperation);

                        Displacement* disp = read_displacement_ptr(op->mod, op->rm, mem->data + idx + bytesRead );
                        bytesRead += disp->size;

                        char* prefix = NULL;
                        if (op->mod != 3) {
                            prefix =  op->s == op->w && op->w ? word: byte;
                        }

                        int data = 0;
                        if (op->s == 1 && op->w != 1) {
                            data = read2Bytes(mem->data+idx+bytesRead);
                            bytesRead++;
                        } else {
                            data = readByte(mem->data+idx+bytesRead);
                        }
                        bytesRead++;

                        DEBUG("mod: %d math_code:%d s:%d rm: %d w: %d\n", op->mod, op->math_code, op->s, op->rm, op->w);
                        char* opName = op->math_code == 7 ? CMP: op->math_code == 5 ? SUB: ADD;
                        operation = multi_reference_operation(opName, idx, sizeof(ImdToRegMemAddOperation));
                        operation->right->type = Value;
                        operation->right->value = data;
                        bool isRawReg = op->mod == 3;
                        if (isRawReg) {
                            operation->left->addr = arr[op->rm][op->w];;
                            operation->left->type = Raw;
                            operation->left->disp = disp;
                        } else {
                            bool isDirectAccess = op->rm == 6;
                            operation->left->prefix = prefix;
                            operation->left->addr = isDirectAccess ? NULL : arr1[op->rm][op->w];;
                            operation->left->value = disp->value;
                            operation->left->type = isDirectAccess ? DirectAccess : Expression;
                            operation->left->disp = disp;
                        }
                    }
                    break;
                case 34:
                case 10:
                case 0:
                case 14:
                    {
                        bytesRead += sizeof(RegToRegOperation);
                        char* opName = op->op_code == 34 ? MOV: op->op_code == 10? SUB: op->op_code == 14 ? CMP: ADD;
                        DEBUG("cmd: %s mod: %d rm: %d reg: %d w: %d d: %d\n", opName, op->mod, op->rm, op->reg, op->w, op->d);
                        if (op->mod == 3) {
                            char* destination = arr[op->d == 1 ? op->reg : op->rm][op->w];
                            char* source = arr[op->d == 1 ? op->rm : op->reg][op->w];
                            operation = double_raw_operation(opName, idx, sizeof(RegToRegOperation), destination, source);
                        } else  {
                            Displacement* disp = read_displacement_ptr(op->mod, op->rm, mem->data + idx + bytesRead);
                            DEBUG("disp: %d\n", disp->value);
                            bytesRead+= disp->size;
                            bool isTargetMemoryWrite = op->d == 0 ;
                            operation = multi_reference_operation(opName, idx, sizeof(RegToRegOperation) + disp->size);

                            if (op->mod == 1) {
                                char* dest = arr[op->reg][op->w];
                                char* src = arr1[op->rm][op->w];
                                if (op->rm == 6 && op->w == 0) {
                                    src = arr[op->reg][1];
                                }
                                operation->left->type = Raw;
                                operation->left->addr = dest;
                                operation->right->type = Expression;
                                operation->right->addr = src;
                                operation->right->disp = disp;
                            } else {
                                char* str = arr1[op->rm][op->mod];
                                char* dest = arr[(op->d == 0 || (op->mod == 0)) ? op->reg: op->rm][op->w];
                                operation->left->addr = dest;
                                operation->left->type = Raw;
                                if (op->rm == 6 && op->mod == 0) {
                                    operation->right->type = DirectAccess;
                                    operation->right->value = disp->value;
                                } else {
                                    operation->right->disp = disp;
                                    operation->right->type = Expression;
                                    operation->right->addr = str;
                                }
                            }
                            if (isTargetMemoryWrite)  {
                                Reference* suposedLeft = operation->left;
                                Reference* suposedRight = operation->right;
                                operation->left = suposedRight;
                                operation->right = suposedLeft;
                            }
                        }
                    }
                    break;
                default:
                      bytesRead += sizeof(RegToRegOperation);
                      DEBUG("Operation not handled %d \n", op->op_code);
            } 
        } else if (opName == IMD_TO_MEM_REG) {
            ImdToRegMemOperation* op = malloc(sizeof(ImdToRegMemOperation));
            memcpy(op, mem->data+idx, sizeof(ImdToRegMemOperation));
            bytesRead+=sizeof(ImdToRegMemOperation);
            Displacement* disp = read_displacement_ptr(op->mod, op->rm, mem->data+idx+bytesRead); 
            bytesRead+= disp->size;
            DEBUG("displ %d %u\n", disp->value, disp->value);

            int data = 0;
            char* prefix = NULL;
            if (op->w == 0) {
                data= readByte(mem->data + idx + bytesRead);
                prefix = "byte";
            } else {
                data = read2Bytes(mem->data + idx + bytesRead);
                bytesRead++;
                prefix = "word";
            }
            bytesRead++;

            DEBUG("mod: %d rm: %d w: %d\n", op->mod, op->rm, op->w);
            operation = multi_reference_operation(MOV, idx, sizeof(ImdToRegMemOperation));
            operation->right->type = Value;
            operation->right->prefix = prefix;
            operation->right->value = data;
            if (op->mod == 3)
            {
                operation->left->type = Raw;
                operation->left->addr = arr[op->rm][op->w];
            }
            else
            {
                operation->left->type = Expression;
                operation->left->addr = arr1[op->rm][op->w];;
                operation->left->disp = disp;
            }
        } else if (opName == ACC_TO_MEMORY) {
            AccumulatorToMemory* op = malloc(sizeof(AccumulatorToMemory));
            memcpy(op, mem->data+idx, sizeof(AccumulatorToMemory));

            bytesRead+=sizeof(AccumulatorToMemory);
            operation = multi_reference_operation(MOV, idx, sizeof(AccumulatorToMemory));
            if (op->order == MEMORY_TO_ACC_FLAG)
            {
                operation->left->type = Raw;
                operation->left->addr = AX;
                operation->right->type = DirectAccess;
                operation->right->value = op->addr;
            }
            else
            {
                operation->right->type = Raw;
                operation->right->addr = AX;
                operation->left->type = DirectAccess;
                operation->left->value = op->addr;
            }
        } else if (opName == JMP) {
            JMPOp* jumpOp = malloc(sizeof(JMPOp));
            memcpy(jumpOp, mem->data+idx, sizeof(JMPOp));
            bytesRead += sizeof(JMPOp);
            int ipinc = 0;
            bool isConditional = jumpOp->op_code != 43;
            // 16 bit?
            ipinc = readByte(mem->data+idx+bytesRead);
            bytesRead++;

            char* opName = JMP_LABEL;
            if (isConditional) {
                if (jumpOp->size == 1) {
                    opName = jumpOp->pad == 1 ? jcxz: loop;
                } else {
                    opName = jumpOp->pad == 1 ? loopz: loopnz;
                }
            }

            int i = idx + ipinc + bytesRead;
            DEBUG("%d %d \n", i, ipinc);
            JumpLabel* jmp = jmpMap[i];
            if (jmp == NULL) {
                jmp = malloc(sizeof(JumpLabel));
                jmp->offset = ipinc;
                jmp->idx = i;
                char* label = malloc(sizeof(char) * 10);
                sprintf(label, "label%d", jmpCount);
                jmp->label = label;
                jmpCount++;
                jmpMap[i] = jmp;

            }
            operation = jmp_operation(opName, ipinc, idx, sizeof(JMPOp) + (jumpOp->size == 1 ? 1: 2), jmp->label);

        } else if (opName == COND_JMP) {
            char* jmpName = "";
            CondJMPOP* jumpOp = malloc(sizeof(CondJMPOP));
            memcpy(jumpOp, mem->data+idx, sizeof(CondJMPOP));
            bytesRead += sizeof(CondJMPOP);
            DEBUG("code: %d \n", jumpOp->op_code);
            switch (jumpOp->op_code) {
                case 124: jmpName = jnge; break;
                case 126: jmpName = jng; break;
                case 114: jmpName = jnae; break;
                case 118: jmpName = jna; break;
                case 122: jmpName = jpe; break;
                case 112: jmpName = jo; break;
                case 117: jmpName = jnz; break;
                case 120: jmpName = js; break;
                case 125: jmpName = jge; break;
                case 127: jmpName = jg; break;
                case 116: jmpName = jz; break;
                case 115: jmpName = jae; break;
                case 119: jmpName = jnbe; break;
                case 123: jmpName = jnp; break;
                case 113: jmpName = jno; break;
                case 121: jmpName = jns; break;
                case 225: jmpName = loop; break;
                case 226: jmpName = loopz; break;
                case 224: jmpName = loopnz; break;
                case 227: jmpName = jcxz; break;
            }
            int i = idx + jumpOp->value + bytesRead;
            JumpLabel* jmp = jmpMap[i];
            if (jmp == NULL) {
                jmp = malloc(sizeof(JumpLabel));
                jmp->offset = jumpOp->value;
                jmp->idx = idx;
                char* label = malloc(sizeof(char) * 10);
                sprintf(label, "label%d", jmpCount);
                jmp->label = label;
                jmpCount++;
                jmpMap[i] = jmp;

            }

            operation = jmp_operation(jmpName, jumpOp->value, idx, sizeof(CondJMPOP), jmp->label);
            DEBUG("idx: %d\n", jmp->idx);
        }

        if (operation != NULL) {
            if (initial == NULL) {
                initial = operation;
                current = operation;
            } else {
                Operation* last = current;
                last->next = operation;
                operation->previous = last;
                current = operation;
            }
        }
    }
    Operation* itercur = initial;
    while (itercur != NULL) {
        JumpLabel* jmp = jmpMap[itercur->currentIdx];
        if (jmp != NULL) {
            printf("%s:\n", jmp->label);
        }
        render_op(itercur);
        itercur = itercur->next;
    }
    free(jmpMap);
    free(mem);
    return 0;
}
