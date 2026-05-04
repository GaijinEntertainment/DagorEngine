/*  see copyright notice in squirrel.h */
#ifndef _SQFUNCSTATE_H_
#define _SQFUNCSTATE_H_
///////////////////////////////////
#include "squtils.h"
#include "compilationcontext.h"

namespace SQCompilation { class Expr; }

using namespace SQCompilation;

struct SQCompiletimeVarInfo
{
    char var_flags;
    SQUnsignedInteger32 type_mask;
    Expr *initializer;

    SQCompiletimeVarInfo() { var_flags = 0; type_mask = ~0u; initializer = nullptr; }

    SQCompiletimeVarInfo(char var_flags, SQUnsignedInteger32 type_mask, Expr *initializer) :
        var_flags(var_flags), type_mask(type_mask), initializer(initializer) {}
};

struct SQFuncState
{
    SQFuncState(SQSharedState *ss,SQFuncState *parent,SQCompilationContext &ctx);
    ~SQFuncState();

    SQFuncState *PushChildState(SQSharedState *ss);
    void PopChildState();
    void AddInstruction(SQOpcode _op,SQInteger arg0=0,SQInteger arg1=0,SQInteger arg2=0,SQInteger arg3=0){SQInstruction i(_op,arg0,arg1,arg2,arg3);AddInstruction(i);}
    void AddInstruction(SQInstruction &i);
    void SetInstructionParams(SQInteger pos,SQInteger arg0,SQInteger arg1,SQInteger arg2=0,SQInteger arg3=0);
    void SetInstructionParam(SQInteger pos,SQInteger arg,SQInteger val);
    SQInstruction &GetInstruction(SQInteger pos){return _instructions[pos];}
    void PopInstructions(SQInteger size){for(SQInteger i=0;i<size;i++)_instructions.pop_back();}
    void SetStackSize(SQInteger n);
    SQInteger CountOuters(SQInteger stacksize);
    void SnoozeOpt(){_optimization=false;}
    void RestoreOpt(){_optimization=true;}
    void AddDefaultParam(SQInteger trg) { _defaultparams.push_back(trg); }
    SQInteger GetDefaultParamCount() { return _defaultparams.size(); }
    SQInteger GetCurrentPos(){return _instructions.size()-1;}
    SQInteger GetNumericConstant(const SQInteger cons);
    SQInteger GetNumericConstant(const SQFloat cons);
    SQInteger PushLocalVariable(const SQObject &name, const SQCompiletimeVarInfo &ct_var_info);
    void AddParameter(const SQObject &name, SQUnsignedInteger32 type_mask);
    SQInteger GetLocalVariable(const SQObject &name, SQCompiletimeVarInfo &ct_var_info);
    void MarkLocalAsOuter(SQInteger pos);
    SQInteger GetOuterVariable(const SQObject &name, SQCompiletimeVarInfo &ct_var_info);
    SQInteger GetStackSize();
    void AddLineInfos(SQInteger line, bool is_dbg_step_point, bool force);
    SQFunctionProto *BuildProto();
    SQInteger AllocStackPos();
    SQInteger PushTarget(SQInteger n=-1);
    SQInteger PopTarget();
    SQInteger TopTarget();
    SQInteger GetUpTarget(SQInteger n);
    void DiscardTarget();
    bool IsLocal(SQUnsignedInteger stkpos);
    SQObjectPtr CreateString(const char *s,SQInteger len = -1);
    void CheckForPurity();
    SQUnsignedInteger lang_features;
    SQInteger _returnexp;
    SQLocalVarInfoVec _vlocals;
    sqvector<SQCompiletimeVarInfo> _vlocals_info; // compile time only
    SQIntVec _targetstack;
    SQInteger _stacksize;
    bool _varparams;
    bool _bgenerator;
    bool _purefunction;
    bool _nodiscard;
    SQIntVec _unresolvedbreaks;
    SQIntVec _unresolvedcontinues;
    SQIntVec _expr_block_results;
    SQObjectPtrVec _functions;
    SQObjectPtrVec _parameters;
    sqvector<SQUnsignedInteger32> _param_type_masks;
    SQUnsignedInteger32 _result_type_mask;
    SQOuterVarVec _outervalues;
    sqvector<SQCompiletimeVarInfo> _outervalues_info; // compile time only
    SQInstructionVec _instructions;
    SQLocalVarInfoVec _localvarinfos;
    SQObjectPtr _literals;
    SQObjectPtr _name;
    SQObjectPtr _sourcename;
    SQInteger _nliterals;
    SQFullLineInfoVec _full_line_infos;
    SQFuncState *_parent;
    SQIntVec _breaktargets;
    SQIntVec _continuetargets;
    SQIntVec _blockstacksizes;
    SQIntVec _defaultparams;
    SQInteger _lastline;
    SQInteger _traps; //contains number of nested exception traps
    SQInteger _outers;
    SQInteger _hoistLevel;
    SQInteger _staticmemos_count;
    bool _optimization;
    SQSharedState *_sharedstate;
    sqvector<SQFuncState*> _childstates;
    SQInteger GetConstant(const SQObjectPtr &cons, int max_const_no = 0x7FFFFFFF);//will return value <= max_const_no, or -1
private:
    SQCompilationContext &_ctx;
    SQSharedState *_ss;
};


#endif //_SQFUNCSTATE_H_

