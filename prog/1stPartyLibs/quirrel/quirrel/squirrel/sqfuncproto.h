/*  see copyright notice in squirrel.h */
#ifndef _SQFUNCTION_H_
#define _SQFUNCTION_H_

#include "opcodes.h"
#include "sqstring.h"

enum SQOuterType {
    otLOCAL = 0,
    otOUTER = 1,
    otCOPY = 2   // a copy of a parent local whose value does not change after the capture
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
    LF_ALLOW_COMPILER_INTERNALS = 0x002000,

    LF_STRICT = LF_FORBID_ROOT_TABLE |
                LF_FORBID_DELETE_OP
};


enum SQVarFlags {
    VF_ASSIGNABLE = 1 << 0,
    VF_PARAM = 1 << 2,
    VF_VALUE_PENDING = 1 << 3, // the local exists, but the destructuring has not set its value yet
};

struct SQOuterVar
{
    SQObjectPtr _name;
    uint32_t _src;
    SQOuterType _type;
};

struct alignas(SQObjectPtr) SQFuncOuterVar // keeps the next proto arrays aligned
{
    SQString *_name; // the proto holds a reference
    uint32_t _src;
    SQOuterType _type;
};

struct SQFuncLocalVar
{
    SQString *_name; // the proto holds a reference
    uint32_t _start_op;
    uint32_t _end_op;
};

static_assert(MAX_FUNC_STACKSIZE - 1 <= UINT8_MAX, "a proto keeps the stack slot of a block local in a uint8_t");

struct SQLocalVarInfo
{
    SQObjectPtr _name;
    uint32_t _start_op = 0;
    uint32_t _end_op = 0;
    uint32_t _pos = 0;
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

#define _FUNC_SIZE(ni,nl,nparams,nfuncs,nouters,nlineinf,compressed,localinf,defparams,nmasks) (sizeof(SQFunctionProto) \
        +SQ_ALIGN_TO(((ni)-1)*sizeof(SQInstruction), SQObjectPtr)+((nl)*sizeof(SQObjectPtr)) \
        +((nparams)*sizeof(SQObjectPtr))+((nfuncs)*sizeof(SQObjectPtr)) \
        +((nouters)*sizeof(SQFuncOuterVar)) \
        +SQ_ALIGN_TO(sizeof(SQLineInfosHeader)+(nlineinf)*(compressed ? sizeof(SQCompressedLineInfo) : sizeof(SQFullLineInfo)), SQFuncLocalVar) \
        +SQ_ALIGN_TO((localinf)*(sizeof(SQFuncLocalVar)+1), SQInt32)+((defparams)*sizeof(SQInt32))+((nmasks)*sizeof(SQUnsignedInteger32)))

#define SQ_FUNC_THIS_TYPE_MASK (_RT_INSTANCE | _RT_TABLE | _RT_CLASS | _RT_USERDATA | _RT_NULL)


struct SQFunctionProto : public CHAINABLE_OBJ
{
private:
    SQFunctionProto(SQSharedState *ss);
    ~SQFunctionProto();

public:
    static SQFunctionProto *Create(SQSharedState *ss,
        SQInteger ninstructions,
        SQInteger nliterals,SQInteger nparameters,
        SQInteger nfunctions,SQInteger noutervalues,
        SQInteger nlineinfos, bool compressedLineInfos,
        SQInteger nlocalvarinfos,
        SQInteger ndefaultparams,
        bool typedParams
        )
    {
        SQFunctionProto *f;
        SQInteger nparamtypemasks = typedParams ? nparameters : 0;
        //I compact the whole class and members in a single memory allocation
        size_t fnSize = _FUNC_SIZE(ninstructions,nliterals,nparameters,nfunctions,noutervalues,nlineinfos,compressedLineInfos,nlocalvarinfos,ndefaultparams,nparamtypemasks);
        f = (SQFunctionProto *)sq_vm_malloc(ss->_alloc_ctx, fnSize);

        new (f) SQFunctionProto(ss);
        char *ptr = (char *)f->_instructions;

        f->_result_type_mask = ~0u;

        f->_alloc_ctx = ss->_alloc_ctx;
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

        assert(size_t(ptr) % alignof(SQFuncOuterVar) == 0);
        f->_outervalues = (SQFuncOuterVar*)ptr;
        f->_noutervalues = noutervalues;
        ptr += noutervalues * sizeof(SQFuncOuterVar);

        assert(size_t(ptr) % alignof(SQLineInfosHeader) == 0);
        f->_lineinfos = (SQLineInfosHeader *)ptr;
        f->_nlineinfos = nlineinfos;
        ptr += SQ_ALIGN_TO(sizeof(SQLineInfosHeader) + nlineinfos * (compressedLineInfos ? sizeof(SQCompressedLineInfo) : sizeof(SQFullLineInfo)), SQFuncLocalVar);

        assert(size_t(ptr) % alignof(SQFuncLocalVar) == 0);
        f->_localvarinfos = (SQFuncLocalVar *)ptr;
        f->_nlocalvarinfos = nlocalvarinfos;
        ptr += SQ_ALIGN_TO(nlocalvarinfos * (sizeof(SQFuncLocalVar) + 1), SQInt32);

        assert(size_t(ptr) % alignof(SQInt32) == 0);
        f->_defaultparams = (SQInt32 *)ptr;
        f->_ndefaultparams = ndefaultparams;
        ptr += ndefaultparams * sizeof(SQInt32);

        f->_param_type_masks = nparamtypemasks ? (SQUnsignedInteger32 *)ptr : nullptr;
        ptr += nparamtypemasks * sizeof(SQUnsignedInteger32);

        assert(ptr - (char *)f == fnSize);

        _CONSTRUCT_VECTOR(SQObjectPtr,f->_nliterals,f->_literals);
        _CONSTRUCT_VECTOR(SQObjectPtr,f->_nparameters,f->_parameters);
        _CONSTRUCT_VECTOR(SQObjectPtr,f->_nfunctions,f->_functions);
        memset(f->_outervalues, 0, noutervalues * sizeof(SQFuncOuterVar));
        //_CONSTRUCT_VECTOR(SQLineInfo,f->_nlineinfos,f->_lineinfos); //not required are 2 integers
        memset(f->_localvarinfos, 0, nlocalvarinfos * sizeof(SQFuncLocalVar));
        return f;
    }
    size_t GetAllocSize() const {
        return _FUNC_SIZE(_ninstructions,_nliterals,_nparameters,_nfunctions,_noutervalues,_nlineinfos,_lineinfos->_is_compressed,_nlocalvarinfos,_ndefaultparams,
            _param_type_masks ? _nparameters : 0);
    }
    static SQUnsignedInteger32 DefaultParamTypeMask(SQInteger i) {
        return i == 0 ? SQ_FUNC_THIS_TYPE_MASK : ~0u;
    }
    SQUnsignedInteger32 GetParamTypeMask(SQInteger i) const {
        return _param_type_masks ? _param_type_masks[i] : DefaultParamTypeMask(i);
    }
    uint8_t *GetLocalVarSlots() const {
        return (uint8_t *)(_localvarinfos + _nlocalvarinfos);
    }
    void Release(){
        _DESTRUCT_VECTOR(SQObjectPtr,_nliterals,_literals);
        _DESTRUCT_VECTOR(SQObjectPtr,_nparameters,_parameters);
        _DESTRUCT_VECTOR(SQObjectPtr,_nfunctions,_functions);
        for(SQInteger i = 0; i < _noutervalues; i++) __ObjRelease(_outervalues[i]._name);
        //_DESTRUCT_VECTOR(SQLineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
        for(SQInteger i = 0; i < _nlocalvarinfos; i++) __ObjRelease(_localvarinfos[i]._name);
        SQInteger size = GetAllocSize();
        SQAllocContext ctx = _alloc_ctx;
        this->~SQFunctionProto();
        sq_vm_free(ctx, this, size);
    }

    const char* GetLocal(SQVM *v,SQUnsignedInteger stackbase,SQUnsignedInteger nseq,SQUnsignedInteger nop);
    static void FormatDerivedName(char *buf, size_t size, const char *sourcename, int line);
    const SQObjectPtr &GetName() const;
    static SQInteger GetLine(SQLineInfosHeader *lineinfos, int nlineinfos, int instruction_index, int *hint, bool *is_dbg_step_point = nullptr);
    SQInteger GetLine(const SQInstruction *curr, int *hint = nullptr, bool *is_dbg_step_point = nullptr);
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **chain);
    void Finalize(){
        _NULL_SQOBJECT_VECTOR(_literals,_nliterals);
    }
    SQObjectType GetType() {return OT_FUNCPROTO;}
#endif
    SQAllocContext _alloc_ctx;
    SQObjectPtr _sourcename;
    mutable SQObjectPtr _name; // null: GetName makes "(file:line)"
    SQUnsignedInteger32 _result_type_mask;
    SQUnsignedInteger32 _docstring_id : 24;
    SQUnsignedInteger32 _inside_hoisted_scope : 1;
    SQUnsignedInteger32 _bgenerator : 1;
    SQUnsignedInteger32 _purefunction : 1;
    SQUnsignedInteger32 _nodiscard : 1;
    SQUnsignedInteger32 _isAsync : 1;     // true: function body is a generator wrapped as a Future on call
    SQInt32 _stacksize;
    SQInt32 _varparams;

    // Struct field order optimized for 64-bit platform memory alignment
    // General principle: Group pointers (8-byte) together, then two integers (4-byte),
    // to minimize padding and cache misses

    SQInt32 _nlineinfos;
    SQLineInfosHeader *_lineinfos;

    SQInt32 _nlocalvarinfos;
    SQFuncLocalVar *_localvarinfos;

    SQObjectPtr *_literals;
    SQInt32 _nliterals;

    SQInt32 _nfunctions;
    SQObjectPtr *_functions;

    SQObjectPtr* _parameters;
    SQInt32 _nparameters;

    SQInt32 _noutervalues;
    SQFuncOuterVar *_outervalues;

    SQUnsignedInteger32* _param_type_masks;

    SQInt32* _defaultparams;
    SQInt32 _ndefaultparams;

    SQInt32 _ninstructions;
    alignas(8) SQInstruction _instructions[1];
};

void Dump(SQFunctionProto *func, int instruction_index = -1);
void Dump(SQStreamWriteFunc write, void *ud, SQFunctionProto *func, bool deep = false, int instruction_index = -1);

#endif //_SQFUNCTION_H_
