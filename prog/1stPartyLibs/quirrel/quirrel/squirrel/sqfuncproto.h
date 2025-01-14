/*  see copyright notice in squirrel.h */
#ifndef _SQFUNCTION_H_
#define _SQFUNCTION_H_

#include "sqopcodes.h"

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
    LF_FORBID_IMPLICIT_DEF_DELEGATE = 0x000800,

    LF_STRICT = LF_FORBID_ROOT_TABLE |
                LF_FORBID_DELETE_OP
};

struct SQOuterVar
{
    SQOuterVar() : _assignable(true) {}
    SQOuterVar(const SQObjectPtr &name,const SQObjectPtr &src,SQOuterType t,bool assignable)
    {
        _name = name;
        _src=src;
        _type=t;
        _assignable=assignable;
    }
    SQOuterVar(const SQOuterVar &ov)
    {
        _type=ov._type;
        _src=ov._src;
        _name=ov._name;
        _assignable=ov._assignable;
    }
    SQOuterType _type;
    SQObjectPtr _name;
    SQObjectPtr _src;
    bool _assignable;
};

struct SQLocalVarInfo
{
    SQLocalVarInfo():_start_op(0),_end_op(0),_pos(0),_assignable(true){}
    SQLocalVarInfo(const SQLocalVarInfo &lvi)
    {
        _name=lvi._name;
        _start_op=lvi._start_op;
        _end_op=lvi._end_op;
        _pos=lvi._pos;
        _assignable=lvi._assignable;
    }
    SQObjectPtr _name;
    uint32_t _start_op;
    uint32_t _end_op;
    uint32_t _pos;
    bool _assignable;
};

struct SQLineInfo
{
    int32_t _op;
    unsigned _line: 31;
    unsigned _is_line_op: 1;
};

typedef sqvector<SQOuterVar> SQOuterVarVec;
typedef sqvector<SQLocalVarInfo> SQLocalVarInfoVec;
typedef sqvector<SQLineInfo> SQLineInfoVec;

#define _FUNC_SIZE(ni,nl,nparams,nfuncs,nouters,nlineinf,localinf,defparams) (sizeof(SQFunctionProto) \
        +(((ni)-1)*sizeof(SQInstruction))+((nl)*sizeof(SQObjectPtr)) \
        +((nparams)*sizeof(SQObjectPtr))+((nfuncs)*sizeof(SQObjectPtr)) \
        +((nouters)*sizeof(SQOuterVar))+((nlineinf)*sizeof(SQLineInfo)) \
        +((localinf)*sizeof(SQLocalVarInfo))+((defparams)*sizeof(SQInteger)))


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
        SQInteger nlineinfos,SQInteger nlocalvarinfos,SQInteger ndefaultparams)
    {
        SQFunctionProto *f;
        //I compact the whole class and members in a single memory allocation
        f = (SQFunctionProto *)sq_vm_malloc(ss->_alloc_ctx,
            _FUNC_SIZE(ninstructions,nliterals,nparameters,nfunctions,noutervalues,nlineinfos,nlocalvarinfos,ndefaultparams));

        new (f) SQFunctionProto(ss);
        f->_alloc_ctx = ss->_alloc_ctx;
        f->lang_features = lang_features;
        f->_ninstructions = ninstructions;
        f->_literals = (SQObjectPtr*)&f->_instructions[ninstructions];
        f->_nliterals = nliterals;
        f->_parameters = (SQObjectPtr*)&f->_literals[nliterals];
        f->_nparameters = nparameters;
        f->_functions = (SQObjectPtr*)&f->_parameters[nparameters];
        f->_nfunctions = nfunctions;
        f->_outervalues = (SQOuterVar*)&f->_functions[nfunctions];
        f->_noutervalues = noutervalues;
        f->_lineinfos = (SQLineInfo *)&f->_outervalues[noutervalues];
        f->_nlineinfos = nlineinfos;
        f->_localvarinfos = (SQLocalVarInfo *)&f->_lineinfos[nlineinfos];
        f->_nlocalvarinfos = nlocalvarinfos;
        f->_defaultparams = (SQInteger *)&f->_localvarinfos[nlocalvarinfos];
        f->_ndefaultparams = ndefaultparams;

        _CONSTRUCT_VECTOR(SQObjectPtr,f->_nliterals,f->_literals);
        _CONSTRUCT_VECTOR(SQObjectPtr,f->_nparameters,f->_parameters);
        _CONSTRUCT_VECTOR(SQObjectPtr,f->_nfunctions,f->_functions);
        _CONSTRUCT_VECTOR(SQOuterVar,f->_noutervalues,f->_outervalues);
        //_CONSTRUCT_VECTOR(SQLineInfo,f->_nlineinfos,f->_lineinfos); //not required are 2 integers
        _CONSTRUCT_VECTOR(SQLocalVarInfo,f->_nlocalvarinfos,f->_localvarinfos);
        return f;
    }
    void Release(){
        _DESTRUCT_VECTOR(SQObjectPtr,_nliterals,_literals);
        _DESTRUCT_VECTOR(SQObjectPtr,_nparameters,_parameters);
        _DESTRUCT_VECTOR(SQObjectPtr,_nfunctions,_functions);
        _DESTRUCT_VECTOR(SQOuterVar,_noutervalues,_outervalues);
        //_DESTRUCT_VECTOR(SQLineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
        _DESTRUCT_VECTOR(SQLocalVarInfo,_nlocalvarinfos,_localvarinfos);
        SQInteger size = _FUNC_SIZE(_ninstructions,_nliterals,_nparameters,_nfunctions,_noutervalues,_nlineinfos,_nlocalvarinfos,_ndefaultparams);
        SQAllocContext ctx = _alloc_ctx;
        this->~SQFunctionProto();
        sq_vm_free(ctx, this, size);
    }

    const SQChar* GetLocal(SQVM *v,SQUnsignedInteger stackbase,SQUnsignedInteger nseq,SQUnsignedInteger nop);
    static SQInteger GetLine(SQLineInfo * lineinfos, int nlineinfos, int instruction_index, int *hint, bool *is_line_op = nullptr);
    SQInteger GetLine(const SQInstruction *curr, int *hint = nullptr, bool *is_line_op = nullptr);
    bool Save(SQVM *v,SQUserPointer up,SQWRITEFUNC write);
    static bool Load(SQVM *v,SQUserPointer up,SQREADFUNC read,SQObjectPtr &ret);
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **chain);
    void Finalize(){ _NULL_SQOBJECT_VECTOR(_literals,_nliterals); }
    SQObjectType GetType() {return OT_FUNCPROTO;}
#endif
    SQObjectPtr _sourcename;
    SQObjectPtr _name;
    SQUnsignedInteger   lang_features;
    SQInteger _stacksize;
    SQInteger _hoistingLevel;
    bool _bgenerator;
    SQInteger _varparams;

    SQAllocContext _alloc_ctx;

    SQInteger _nlocalvarinfos;
    SQLocalVarInfo *_localvarinfos;

    SQInteger _nlineinfos;
    SQLineInfo *_lineinfos;

    SQInteger _nliterals;
    SQObjectPtr *_literals;

    SQInteger _nparameters;
    SQObjectPtr *_parameters;

    SQInteger _nfunctions;
    SQObjectPtr *_functions;

    SQInteger _noutervalues;
    SQOuterVar *_outervalues;

    SQInteger _ndefaultparams;
    SQInteger *_defaultparams;

    SQInteger _ninstructions;
    SQInstruction _instructions[1];
};

void Dump(SQFunctionProto *func, int instruction_index = -1);
void Dump(OutputStream *stream, SQFunctionProto *func, bool deep = false, int instruction_index = -1);

#endif //_SQFUNCTION_H_
