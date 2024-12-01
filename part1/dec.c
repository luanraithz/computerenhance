#include "dec.h"
#include <math.h>
#include <sys/types.h>

char* MOV = "mov";
char* SUB = "sub";
char* ADD = "add";
char* CMP = "cmp";
char* JMP_LABEL = "jmp";
char* AX = "ax";
char* AL = "al";
char* word = "word";
char* byte = "byte";

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


i16 AX_V = 0;
i16 BX_V = 0;
i16 CX_V = 0;
i16 DX_V = 0;
i16 SP_V = 0;
i16 BP_V = 0;
i16 SI_V = 0;
i16 DI_V = 0;

const MemArray mem = {
    .registers = {
        &AX_V, // AX - AL/AH
        &BX_V, // BX - AL/AH
        &CX_V, // CX - AL/AH
        &DX_V, // DX - CL/CH
        &SP_V, // SP - DL/DH
        &BP_V, // BP
        &SI_V, // SI
        &DI_V, // DI
    }
};



#define REG_LOW(name, addrV) { .label = name, .addr = addrV, .size = 1 };
#define REG_HIGH(name, addrV) { .label = name, .addr = addrV, .size = 1 };
#define REG_FULL(name, addrV) { .label = name, .addr = addrV, .size = 2 };

const RegisterLocation AL_REG = REG_LOW("AL", mem.registers[0]);
const RegisterLocation AH_REG = REG_HIGH("AH", mem.registers[0]);
const RegisterLocation AX_REG = REG_FULL("AX", mem.registers[0]);
const RegisterLocation BL_REG = REG_LOW("BL", mem.registers[1]);
const RegisterLocation BH_REG = REG_HIGH("BH", mem.registers[1]);
const RegisterLocation BX_REG = REG_FULL("BX", mem.registers[1]);
const RegisterLocation CL_REG = REG_LOW("CL", mem.registers[2]);
const RegisterLocation CH_REG = REG_HIGH("CH", mem.registers[2]);
const RegisterLocation CX_REG = REG_FULL("CX", mem.registers[2]);
const RegisterLocation DL_REG = REG_LOW("DL", mem.registers[3]);
const RegisterLocation DH_REG = REG_HIGH("DH", mem.registers[3]);
const RegisterLocation DX_REG = REG_FULL("DX", mem.registers[3]);
const RegisterLocation SP_REG = REG_FULL("SP", mem.registers[4]);
const RegisterLocation BP_REG = REG_FULL("BP", mem.registers[5]);
const RegisterLocation SI_REG = REG_FULL("SI", mem.registers[6]);
const RegisterLocation DI_REG = REG_FULL("DI", mem.registers[6]);



#define EXPR(name, reg, s) RegisterExpression name = { .registers = reg, .size = s }

#define EXPR_SINGLE(name, reg) const RegisterExpression name = { .registers = reg, .size = 1 }

EXPR_SINGLE(AL_EXP, &AL_REG);
EXPR_SINGLE(AX_EXP, &AX_REG);
EXPR_SINGLE(CL_EXP, &CL_REG);
EXPR_SINGLE(CX_EXP, &CX_REG);
EXPR_SINGLE(DL_EXP, &DL_REG);
EXPR_SINGLE(DX_EXP, &DX_REG);
EXPR_SINGLE(BL_EXP, &BL_REG);
EXPR_SINGLE(BX_EXP, &BX_REG);
EXPR_SINGLE(AH_EXP, &AH_REG);
EXPR_SINGLE(SP_EXP, &SP_REG);
EXPR_SINGLE(CH_EXP, &CH_REG);
EXPR_SINGLE(BP_EXP, &BP_REG);
EXPR_SINGLE(DH_EXP, &DH_REG);
EXPR_SINGLE(SI_EXP, &SI_REG);
EXPR_SINGLE(BH_EXP, &BH_REG);
EXPR_SINGLE(DI_EXP, &DI_REG);

RegisterExpression arr[][2] = {
    { AL_EXP, AX_EXP },
    { CL_EXP, CX_EXP },
    { DL_EXP, DX_EXP },
    { BL_EXP, BX_EXP },
    { AH_EXP, SP_EXP },
    { CH_EXP, BP_EXP },
    { DH_EXP, SI_EXP },
    { BH_EXP, DI_EXP },
};

RegisterLocation BX_SI[] = {BX_REG, SI_REG};
RegisterLocation BX_DI[] = {BX_REG, DI_REG};
RegisterLocation BP_SI[] = {BP_REG, SI_REG};
RegisterLocation BP_DI[] = {BP_REG, DI_REG};
RegisterLocation SI[] = {SI_REG};
RegisterLocation DI[] = {DI_REG};
RegisterLocation BP[] = {BP_REG};
RegisterLocation BX[] = {BX_REG};
RegisterExpression EMPTY = { .registers = {}, .size = 0};

const RegisterExpression arr1[] = {
     { .registers = BX_SI, 2 },
     { .registers = BX_DI, 2 },
     { .registers = BP_SI, 2 },
     { .registers = BP_DI, 2 },
     { .registers = SI, 1 },
     { .registers = DI, 1 },
     { .registers = BP, 1 },
     { .registers = BX, 1 },
};


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


Operation* double_raw_operation(char* opName, int idx, int size, RegisterExpression leftReg, RegisterExpression rightReg) {
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
    op->left->type = JmpLabel;
    op->left->label = id;
    return op;
}
DecodedOperations* decode(Memory* mem, int size) {
    JumpLabel** jmpMap = calloc(1000, sizeof(JumpLabel*));
    int jmpCount = 0;
    Operation* initial = NULL;
    Operation* current = NULL;
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

            operation->left->addr = arr[regOp->reg][regOp->w];;
            operation->left->type = Raw;
            operation->right->type = Value;
            operation->right->value = regOp->w ? read2Bytes(mem->data+bytesRead+idx): readByte(mem->data+bytesRead+idx);
            bytesRead+= regOp->w?2: 1;
        } else if (opName == MEM_REG_TO_MEM_REG || opName == ADD_REG || opName == SUB_REG || opName == CMP_REG) {
            RegToRegOperation* op = malloc(sizeof(RegToRegOperation));
            memcpy(op, mem->data+idx, sizeof(RegToRegOperation));
            bytesRead = 0;
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
                        RegisterExpression exp = op->w == 1 ? AX_EXP: AL_EXP;
                        operation->left->addr = exp;
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


                        if ((op->s == 1 && op->w != 1) || (op->s == 0 && op->w == 1)) {
                            data = read2Bytes(mem->data+idx+bytesRead);
                            bytesRead++;
                        } else {
                            data = readByte(mem->data+idx+bytesRead);
                        }
                        bytesRead++;

                        char* opName = op->math_code == 7 ? CMP: op->math_code == 5 ? SUB: ADD;
                        operation = multi_reference_operation(opName, idx, sizeof(ImdToRegMemAddOperation));
                        operation->right->type = Value;
                        operation->right->value = data;
                        bool isRawReg = op->mod == 3;
                        if (isRawReg) {
                            operation->left->addr = arr[op->rm][op->w];
                            operation->left->type = Raw;
                            operation->left->disp = disp;
                        } else {
                            bool isDirectAccess = op->rm == 6;
                            operation->left->prefix = prefix;
                            operation->left->addr = isDirectAccess ? EMPTY : arr1[op->rm];;
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
                        if (op->mod == 3) {
                            RegisterExpression dest = arr[op->d == 1 ? op->reg : op->rm][op->w];
                            RegisterExpression src = arr[op->d == 1 ? op->rm : op->reg][op->w];
                            operation = double_raw_operation(opName, idx, sizeof(RegToRegOperation), dest, src);
                        } else  {
                            Displacement* disp = read_displacement_ptr(op->mod, op->rm, mem->data + idx + bytesRead);
                            bytesRead+= disp->size;
                            bool isTargetMemoryWrite = op->d == 0 ;
                            operation = multi_reference_operation(opName, idx, sizeof(RegToRegOperation) + disp->size);

                            if (op->mod == 1) {
                                RegisterExpression dest = arr[op->reg][op->w];
                                RegisterExpression src = arr1[op->rm];
                                if (op->rm == 6 && op->w == 0) {
                                    src = arr[op->reg][1];;
                                }
                                operation->left->type = Raw;
                                operation->left->addr = dest;
                                operation->right->type = Expression;
                                operation->right->addr = src;
                                operation->right->disp = disp;
                            } else {
                                RegisterExpression str = arr1[op->rm];
                                RegisterExpression dest = arr[(op->d == 0 || (op->mod == 0)) ? op->reg: op->rm][op->w];
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
            } 
        } else if (opName == IMD_TO_MEM_REG) {
            ImdToRegMemOperation* op = malloc(sizeof(ImdToRegMemOperation));
            memcpy(op, mem->data+idx, sizeof(ImdToRegMemOperation));
            bytesRead+=sizeof(ImdToRegMemOperation);
            Displacement* disp = read_displacement_ptr(op->mod, op->rm, mem->data+idx+bytesRead); 
            bytesRead+= disp->size;

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
                operation->left->addr = arr1[op->rm];;
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
                operation->left->addr = AX_EXP;
                operation->right->type = DirectAccess;
                operation->right->value = op->addr;
            }
            else
            {
                operation->right->type = Raw;
                operation->right->addr = AX_EXP;
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

    DecodedOperations* res = malloc(sizeof(DecodedOperations));
    res->initial = initial;
    res->jmpMap = jmpMap;
    return res;
}


