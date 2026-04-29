/*  see copyright notice in squirrel.h */
#ifndef _SQFUNCTION_H_
#define _SQFUNCTION_H_

#include "opcodes.h"

enum SQOuterType {
    otLOCAL = 0,
    otOUTER = 1
};

enum SQLangFeature {
    // parsing stage
    LF_FORBID_ROOT_TABLE = 0x000001,
    LF_FORBID_DELETE_OP = 0x000004,
    LF_FORBID_CLONE_OP = 0x000008,
    LF_FORBID_SWITCH_STMT = 0x000010,

    // code generation stage
    LF_DISABLE_OPTIMIZER = 0x000200,
    LF_FORBID_GLOBAL_CONST_REWRITE = 0x000400,
    LF_FORBID_IMPLICIT_TYPE_METHODS = 0x000800,
    LF_ALLOW_AUTO_FREEZE = 0x001000,
    LF_ALLOW_COMPILER_INTERNALS = 0x002000,

    LF_STRICT = LF_FORBID_ROOT_TABLE |
                LF_FORBID_DELETE_OP
};


enum SQVarFlags {
    VF_ASSIGNABLE = 1 << 0,
    VF_INIT_WITH_CONST = 1 << 1,
    VF_PARAM = 1 << 2,
    VF_DESTRUCTURED = 1 << 3,
    VF_INIT_WITH_FREEZE = 1 << 4,
    VF_INIT_WITH_PURE = 1 << 5,
    VF_FIRST_LEVEL = 1 << 6,
};

struct SQOuterVar
{
    SQOuterVar() : _varFlags(VF_ASSIGNABLE) {}
    SQOuterVar(const SQObjectPtr &name,const SQObjectPtr &src,SQOuterType t,char varFlags)
    {
        _name = name;
        _src=src;
        _type=t;
        _varFlags=varFlags;
    }
    SQOuterVar(const SQOuterVar &ov)
    {
        _type=ov._type;
        _src=ov._src;
        _name=ov._name;
        _varFlags=ov._varFlags;
    }
    SQOuterType _type;
    char _varFlags;
    SQObjectPtr _name;
    SQObjectPtr _src;
};

struct SQLocalVarInfo
{
    SQLocalVarInfo():_start_op(0),_end_op(0),_pos(0),_varFlags(VF_ASSIGNABLE){}
    SQLocalVarInfo(const SQLocalVarInfo &lvi)
    {
        _name=lvi._name;
        _start_op=lvi._start_op;
        _end_op=lvi._end_op;
        _pos=lvi._pos;
        _varFlags=lvi._varFlags;
    }
    SQObjectPtr _name;
    uint32_t _start_op;
    uint32_t _end_op;
    uint32_t _pos;
    char _varFlags;
};

struct SQLineInfosHeader
{
    unsigned _first_line: 31;
    unsigned _is_compressed: 1;
};

struct SQFullLineInfo
{
    uint32_t _op;
    uint32_t _line_offset: 31;
    uint32_t _is_dbg_step_point: 1;
};

struct SQCompressedLineInfo
{
    uint8_t _op;
    uint8_t _line_offset: 7;
    uint8_t _is_dbg_step_point: 1;
};

typedef sqvector<SQOuterVar> SQOuterVarVec;
typedef sqvector<SQLocalVarInfo> SQLocalVarInfoVec;
typedef sqvector<SQFullLineInfo> SQFullLineInfoVec;

#define SQ_ALIGN_TO(n, t) (((n) + alignof(t) - 1) & ~(alignof(t) - 1))

#define _FUNC_SIZE(ni,nl,nparams,nfuncs,nouters,nlineinf,compressed,localinf,defparams,nstaticmemos) (sizeof(SQFunctionProto) \
        +SQ_ALIGN_TO(((ni)-1)*sizeof(SQInstruction), SQObjectPtr)+((nl)*sizeof(SQObjectPtr)) \
        +((nparams)*sizeof(SQObjectPtr))+((nfuncs)*sizeof(SQObjectPtr)) \
        +((nouters)*sizeof(SQOuterVar)) \
        +SQ_ALIGN_TO(sizeof(SQLineInfosHeader)+(nlineinf)*(compressed ? sizeof(SQCompressedLineInfo) : sizeof(SQFullLineInfo)), SQLocalVarInfo) \
        +((localinf)*sizeof(SQLocalVarInfo))+((defparams)*sizeof(SQInt32))+((nparams)*sizeof(SQUnsignedInteger32)) \
        +((nstaticmemos)*sizeof(SQObjectPtr)))


struct SQFunctionProto : public CHAINABLE_OBJ
{
private:
    SQFunctionProto(SQSharedState *ss);
    ~SQFunctionProto();

public:
    static SQFunctionProto *Create(SQSharedState *ss,
        SQUnsignedInteger lang_features,
        SQInteger ninstructions,
        SQInteger nliterals,SQInteger nparameters,
        SQInteger nfunctions,SQInteger noutervalues,
        SQInteger nlineinfos, bool compressedLineInfos,
        SQInteger nlocalvarinfos,
        SQInteger ndefaultparams,SQInteger nstaticmemos
        )
    {
        SQFunctionProto *f;
        //I compact the whole class and members in a single memory allocation
        size_t fnSize = _FUNC_SIZE(ninstructions,nliterals,nparameters,nfunctions,noutervalues,nlineinfos,compressedLineInfos,nlocalvarinfos,ndefaultparams,nstaticmemos);
        f = (SQFunctionProto *)sq_vm_malloc(ss->_alloc_ctx, fnSize);

        new (f) SQFunctionProto(ss);
        char *ptr = (char *)f->_instructions;

        f->_result_type_mask = ~0u;

        f->_alloc_ctx = ss->_alloc_ctx;
        f->lang_features = lang_features;
        f->_ninstructions = ninstructions;
        ptr += SQ_ALIGN_TO(ninstructions * sizeof(SQInstruction), SQLocalVarInfo);

        assert(size_t(ptr) % alignof(SQObjectPtr) == 0);
        f->_literals = (SQObjectPtr*)ptr;
        f->_nliterals = nliterals;
        ptr += nliterals * sizeof(SQObjectPtr);

        f->_parameters = (SQObjectPtr*)ptr;
        f->_nparameters = nparameters;
        ptr += nparameters * sizeof(SQObjectPtr);

        f->_functions = (SQObjectPtr*)ptr;
        f->_nfunctions = nfunctions;
        ptr += nfunctions * sizeof(SQObjectPtr);

        f->_staticmemos = (SQObjectPtr*)ptr;
        f->_nstaticmemos = nstaticmemos;
        ptr += nstaticmemos * sizeof(SQObjectPtr);

        assert(size_t(ptr) % alignof(SQOuterVar) == 0);
        f->_outervalues = (SQOuterVar*)ptr;
        f->_noutervalues = noutervalues;
        ptr += noutervalues * sizeof(SQOuterVar);

        assert(size_t(ptr) % alignof(SQLineInfosHeader) == 0);
        f->_lineinfos = (SQLineInfosHeader *)ptr;
        f->_nlineinfos = nlineinfos;
        ptr += SQ_ALIGN_TO(sizeof(SQLineInfosHeader) + nlineinfos * (compressedLineInfos ? sizeof(SQCompressedLineInfo) : sizeof(SQFullLineInfo)), SQLocalVarInfo);

        assert(size_t(ptr) % alignof(SQLocalVarInfo) == 0);
        f->_localvarinfos = (SQLocalVarInfo *)ptr;
        f->_nlocalvarinfos = nlocalvarinfos;
        ptr += nlocalvarinfos * sizeof(SQLocalVarInfo);

        assert(size_t(ptr) % alignof(SQInt32) == 0);
        f->_defaultparams = (SQInt32 *)ptr;
        f->_ndefaultparams = ndefaultparams;
        ptr += ndefaultparams * sizeof(SQInt32);

        f->_param_type_masks = (SQUnsignedInteger32 *)ptr;
        ptr += nparameters * sizeof(SQUnsignedInteger32);

        assert(ptr - (char *)f == fnSize);

        _CONSTRUCT_VECTOR(SQObjectPtr,f->_nliterals,f->_literals);
        _CONSTRUCT_VECTOR(SQObjectPtr,f->_nparameters,f->_parameters);
        _CONSTRUCT_VECTOR(SQObjectPtr,f->_nfunctions,f->_functions);
        _CONSTRUCT_VECTOR(SQObjectPtr,f->_nstaticmemos,f->_staticmemos);
        _CONSTRUCT_VECTOR(SQOuterVar,f->_noutervalues,f->_outervalues);
        //_CONSTRUCT_VECTOR(SQLineInfo,f->_nlineinfos,f->_lineinfos); //not required are 2 integers
        _CONSTRUCT_VECTOR(SQLocalVarInfo,f->_nlocalvarinfos,f->_localvarinfos);
        return f;
    }
    void Release(){
        _DESTRUCT_VECTOR(SQObjectPtr,_nliterals,_literals);
        _DESTRUCT_VECTOR(SQObjectPtr,_nparameters,_parameters);
        _DESTRUCT_VECTOR(SQObjectPtr,_nfunctions,_functions);
        _DESTRUCT_VECTOR(SQObjectPtr,_nstaticmemos,_staticmemos);
        _DESTRUCT_VECTOR(SQOuterVar,_noutervalues,_outervalues);
        //_DESTRUCT_VECTOR(SQLineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
        _DESTRUCT_VECTOR(SQLocalVarInfo,_nlocalvarinfos,_localvarinfos);
        SQInteger size = _FUNC_SIZE(_ninstructions,_nliterals,_nparameters,_nfunctions,_noutervalues,_nlineinfos,_lineinfos->_is_compressed,_nlocalvarinfos,_ndefaultparams,_nstaticmemos);
        SQAllocContext ctx = _alloc_ctx;
        this->~SQFunctionProto();
        sq_vm_free(ctx, this, size);
    }

    const char* GetLocal(SQVM *v,SQUnsignedInteger stackbase,SQUnsignedInteger nseq,SQUnsignedInteger nop);
    static SQInteger GetLine(SQLineInfosHeader *lineinfos, int nlineinfos, int instruction_index, int *hint, bool *is_dbg_step_point = nullptr);
    SQInteger GetLine(const SQInstruction *curr, int *hint = nullptr, bool *is_dbg_step_point = nullptr);
    bool Save(SQVM *v,SQUserPointer up,SQWRITEFUNC write);
    static bool Load(SQVM *v,SQUserPointer up,SQREADFUNC read,SQObjectPtr &ret);
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **chain);
    void Finalize(){
        _NULL_SQOBJECT_VECTOR(_literals,_nliterals);
        _NULL_SQOBJECT_VECTOR(_staticmemos,_nstaticmemos);
    }
    SQObjectType GetType() {return OT_FUNCPROTO;}
#endif
    SQAllocContext _alloc_ctx;
    SQObjectPtr _sourcename;
    SQObjectPtr _name;
    SQUnsignedInteger32 lang_features;
    SQUnsignedInteger32 _result_type_mask;
    bool _inside_hoisted_scope;
    bool _bgenerator;
    bool _purefunction;
    bool _nodiscard;
    SQInt32 _stacksize;
    SQInt32 _varparams;

    // Struct field order optimized for 64-bit platform memory alignment
    // General principle: Group pointers (8-byte) together, then two integers (4-byte),
    // to minimize padding and cache misses

    SQInt32 _nlineinfos;
    SQLineInfosHeader *_lineinfos;

    SQObjectPtr* _staticmemos;
    SQInt32 _nstaticmemos;

    SQInt32 _nlocalvarinfos;
    SQLocalVarInfo *_localvarinfos;

    SQObjectPtr *_literals;
    SQInt32 _nliterals;

    SQInt32 _nfunctions;
    SQObjectPtr *_functions;

    SQObjectPtr* _parameters;
    SQInt32 _nparameters;

    SQInt32 _noutervalues;
    SQOuterVar *_outervalues;

    SQUnsignedInteger32* _param_type_masks;

    SQInt32* _defaultparams;
    SQInt32 _ndefaultparams;

    SQInt32 _ninstructions;
    alignas(8) SQInstruction _instructions[1];
};

void Dump(SQFunctionProto *func, int instruction_index = -1);
void Dump(OutputStream *stream, SQFunctionProto *func, bool deep = false, int instruction_index = -1);

void ResetStaticMemos(SQFunctionProto *func, SQSharedState *ss);

#endif //_SQFUNCTION_H_
