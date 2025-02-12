#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <sys/stat.h>
#include <time.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t i16;
typedef int8_t i8;

#define RM unsigned int rm : 3
#define MOD unsigned int mod : 2
#define W unsigned int w : 1

#define PAD(n) unsigned int pad : n

#pragma pack(push, 1)
typedef struct RegToRegOperation {
    unsigned int w : 1;
    unsigned int d : 1;
    unsigned int op_code : 6;

    unsigned int rm : 3;
    unsigned int reg : 3;
    unsigned int mod : 2;
} RegToRegOperation;

typedef struct ImdToRegOperation {
    unsigned int reg : 3;
    unsigned int w : 1;
    unsigned int op_code : 4;
} ImdToRegOperation;

typedef struct ImdToRegMemOperation {
    unsigned int w : 1;
    unsigned int s : 1;
    unsigned int op_code : 6;

    RM;
    PAD(3);
    MOD;
} ImdToRegMemOperation;

typedef struct ImdToAccAddOperation {
    unsigned int w : 1;
    PAD(1);
    unsigned int op_code : 6;
} ImdToAccAddOperation;

typedef struct ImdToRegMemAddOperation {
    unsigned int w : 1;
    unsigned int s : 1;
    unsigned int op_code : 6;

    RM;
    unsigned int math_code : 3;
    MOD;
} ImdToRegMemAddOperation;

typedef struct AccumulatorToMemory {
    W;
    unsigned int order : 1;
    PAD(6);

    unsigned int addr : 16;
} AccumulatorToMemory;

typedef struct CondJMPOP {
    int unsigned op_code : 8;

    int value : 8;
} CondJMPOP;

typedef struct JMPOp {
    int unsigned pad : 1;
    int unsigned size : 1;
    int unsigned op_code : 6;
} JMPOp;

#define bit(name) int unsigned name : 1;

typedef struct Flags {
    bit(zf);
    bit(sf);
    bit(pf);
} Flags;

typedef struct MemArray {
    i16 *registers[8];
} MemArray;

#pragma pack(pop)

typedef struct Memory {
    u8 data[1024 * 1024];
} Memory;

enum ReferenceType { DirectAccess, Expression, JmpLabel, Raw, Value };

typedef struct {
    int value;
    char *label;
    int size;
} Displacement;

typedef struct ReferenceCalc {

} ReferenceCalc;

typedef struct RegisterLocation {
    char *label;
    void *addr;
    u8 size;
} RegisterLocation;

typedef struct RegisterExpression {
    const RegisterLocation *registers;
    u8 size;
} RegisterExpression;

typedef struct {
    RegisterExpression addr;
    char *label;
    char *prefix;
    Displacement *disp;
    int value;
    enum ReferenceType type;
} Reference;

typedef struct Operation {
    char *name;
    struct Operation *previous;
    struct Operation *next;
    int currentIdx;
    int size;
    bool wide;
    Reference *left;
    Reference *right;
} Operation;

typedef struct JumpLabel {
    int idx;
    int offset;
    char *label;
    struct JumpLabel *next;
} JumpLabel;

typedef struct DecodedOperations {
    Operation *initial;
    JumpLabel **jmpMap;
} DecodedOperations;

typedef struct ReferenceExpression ReferenceExpression;

extern const MemArray mem;
extern Flags flags;

extern char *MOV;
extern char *SUB;
extern char *ADD;
extern char *CMP;
extern char *JMP_LABEL;
extern char *AX;
extern char *AL;

DecodedOperations *decode(Memory *mem, int size);

extern char *jnge;
extern char *jng;
extern char *jnae;
extern char *jna;
extern char *jpe;
extern char *jo;
extern char *jnz;
extern char *jne;
extern char *js;
extern char *jge;
extern char *jg;
extern char *jz;
extern char *jae;
extern char *jnbe;
extern char *jnp;
extern char *jno;
extern char *jns;
extern char *loop;
extern char *loopz;
extern char *loopnz;
extern char *jcxz;
