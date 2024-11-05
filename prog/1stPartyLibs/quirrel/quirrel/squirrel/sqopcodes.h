/*  see copyright notice in squirrel.h */
#ifndef _SQOPCODES_H_
#define _SQOPCODES_H_

#define MAX_FUNC_STACKSIZE 0xFF
#define MAX_LITERALS ((SQInteger)0x7FFFFFFF)

enum BitWiseOP {
    BW_AND = 0,
    BW_OR = 2,
    BW_XOR = 3,
    BW_SHIFTL = 4,
    BW_SHIFTR = 5,
    BW_USHIFTR = 6
};

enum CmpOP {
    CMP_G = 0,
    CMP_GE = 2,
    CMP_L = 3,
    CMP_LE = 4,
    CMP_3W = 5
};

enum NewObjectType {
    NOT_TABLE = 0,
    NOT_ARRAY = 1,
    NOT_CLASS = 2
};

enum AppendArrayType {
    AAT_STACK = 0,
    AAT_LITERAL = 1,
    AAT_INT = 2,
    AAT_FLOAT = 3,
    AAT_BOOL = 4
};


#define SQ_OPCODES_LIST \
    SQ_OPCODE(_OP_DATA_NOP) \
    SQ_OPCODE(_OP_LOAD) \
    SQ_OPCODE(_OP_LOADINT) \
    SQ_OPCODE(_OP_LOADFLOAT) \
    SQ_OPCODE(_OP_DLOAD) \
    SQ_OPCODE(_OP_TAILCALL) \
    SQ_OPCODE(_OP_CALL) \
    SQ_OPCODE(_OP_PREPCALL) \
    SQ_OPCODE(_OP_PREPCALLK) \
    SQ_OPCODE(_OP_GETK) \
    SQ_OPCODE(_OP_MOVE) \
    SQ_OPCODE(_OP_NEWSLOT) \
    SQ_OPCODE(_OP_NEWSLOTK) \
    SQ_OPCODE(_OP_DELETE) \
    SQ_OPCODE(_OP_SET) \
    SQ_OPCODE(_OP_SETK)\
    SQ_OPCODE(_OP_SETI)\
    SQ_OPCODE(_OP_GET) \
    SQ_OPCODE(_OP_SET_LITERAL) \
    SQ_OPCODE(_OP_GET_LITERAL) \
    SQ_OPCODE(_OP_EQ) \
    SQ_OPCODE(_OP_NE) \
    SQ_OPCODE(_OP_ADD) \
    SQ_OPCODE(_OP_ADDI) \
    SQ_OPCODE(_OP_SUB) \
    SQ_OPCODE(_OP_MUL) \
    SQ_OPCODE(_OP_DIV) \
    SQ_OPCODE(_OP_MOD) \
    SQ_OPCODE(_OP_BITW) \
    SQ_OPCODE(_OP_RETURN) \
    SQ_OPCODE(_OP_LOADNULLS) \
    SQ_OPCODE(_OP_LOADROOT) \
    SQ_OPCODE(_OP_LOADBOOL) \
    SQ_OPCODE(_OP_DMOVE) \
    SQ_OPCODE(_OP_JMP) \
    SQ_OPCODE(_OP_JCMP) \
    SQ_OPCODE(_OP_JCMPI) \
    SQ_OPCODE(_OP_JCMPF) \
    SQ_OPCODE(_OP_JCMPK) \
    SQ_OPCODE(_OP_JZ) \
    SQ_OPCODE(_OP_SETOUTER) \
    SQ_OPCODE(_OP_GETOUTER) \
    SQ_OPCODE(_OP_NEWOBJ) \
    SQ_OPCODE(_OP_APPENDARRAY) \
    SQ_OPCODE(_OP_COMPARITH) \
    SQ_OPCODE(_OP_COMPARITH_K) \
    SQ_OPCODE(_OP_INC) \
    SQ_OPCODE(_OP_INCL) \
    SQ_OPCODE(_OP_PINC) \
    SQ_OPCODE(_OP_PINCL) \
    SQ_OPCODE(_OP_CMP) \
    SQ_OPCODE(_OP_EXISTS) \
    SQ_OPCODE(_OP_INSTANCEOF) \
    SQ_OPCODE(_OP_AND) \
    SQ_OPCODE(_OP_OR) \
    SQ_OPCODE(_OP_NEG) \
    SQ_OPCODE(_OP_NOT) \
    SQ_OPCODE(_OP_BWNOT) \
    SQ_OPCODE(_OP_CLOSURE) \
    SQ_OPCODE(_OP_YIELD) \
    SQ_OPCODE(_OP_RESUME) \
    SQ_OPCODE(_OP_PREFOREACH) \
    SQ_OPCODE(_OP_POSTFOREACH) \
    SQ_OPCODE(_OP_FOREACH) \
    SQ_OPCODE(_OP_CLONE) \
    SQ_OPCODE(_OP_TYPEOF) \
    SQ_OPCODE(_OP_PUSHTRAP) \
    SQ_OPCODE(_OP_POPTRAP) \
    SQ_OPCODE(_OP_THROW) \
    SQ_OPCODE(_OP_NEWSLOTA) \
    SQ_OPCODE(_OP_GETBASE) \
    SQ_OPCODE(_OP_CLOSE) \
    SQ_OPCODE(_OP_NULLCOALESCE) \
    SQ_OPCODE(_OP_NULLCALL) \
    SQ_OPCODE(_OP_LOADCALLEE) \


#define SQ_OPCODE(id) id,

enum SQOpcode
{
    SQ_OPCODES_LIST
};

#undef SQ_OPCODE

struct SQInstructionDesc {
    const SQChar *name;
};

struct SQInstruction
{
    SQInstruction(){};
    SQInstruction(SQOpcode _op,SQInteger a0=0,SQInteger a1=0,SQInteger a2=0,SQInteger a3=0)
    {   op = (unsigned char)_op;
        _arg0 = (unsigned char)a0;_arg1 = (SQInt32)a1;
        _arg2 = (unsigned char)a2;_arg3 = (unsigned char)a3;
    }

    SQInstruction(SQOpcode _op,SQInteger a0,float a1=0,SQInteger a2=0,SQInteger a3=0)
    {   op = (unsigned char)_op;
        _arg0 = (unsigned char)a0;_farg1 = a1;
        _arg2 = (unsigned char)a2;_arg3 = (unsigned char)a3;
    }

    union {
        SQInt32 _arg1;
        float _farg1;
    };
    unsigned char op;
    unsigned char _arg0;
    unsigned char _arg2;
    unsigned char _arg3;
    int _sarg0() const {return (signed char)_arg0;}
    int _sarg2() const {return (signed char)_arg2;}
    int _sarg3() const {return (signed char)_arg3;}
};

#include "squtils.h"
typedef sqvector<SQInstruction> SQInstructionVec;

#define NEW_SLOT_STATIC_FLAG        0x02

#define OP_GET_FLAG_ALLOW_DEF_DELEGATE  0x01
#define OP_GET_FLAG_NO_ERROR            0x02
#define OP_GET_FLAG_KEEP_VAL            0x04 //< only used with OP_GET_FLAG_NO_ERROR
#define OP_GET_FLAG_TYPE_METHODS_ONLY   0x08

#endif // _SQOPCODES_H_
