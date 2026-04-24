/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#ifndef NO_COMPILER
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqtable.h"
#include "opcodes.h"
#include "sqfuncstate.h"
#include "sqio.h"
#include "sqclosure.h"
#include "sqarray.h"

#include <stdarg.h>


static void streamprintf(OutputStream *stream, const char *fmt, ...) {
    static char buffer[4096] = { 0 };
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buffer, 4096, fmt, vl);
    va_end(vl);
    stream->writeString(buffer);
}

static void DumpLiteral(OutputStream *stream, const SQObjectPtr &o)
{
    switch (sq_type(o)) {
        case OT_NULL: streamprintf(stream, "null"); break;
        case OT_STRING: streamprintf(stream, "string(\"%s\")", _stringval(o)); break;
        case OT_FLOAT: streamprintf(stream, "float(%1.9g)", _float(o)); break;
        case OT_INTEGER: streamprintf(stream, "int(" _PRINT_INT_FMT ")", _integer(o)); break;
        case OT_BOOL: streamprintf(stream, "%s", _integer(o) ? "bool(true)" : "bool(false)"); break;
        case OT_ARRAY: streamprintf(stream, "array(0x%p size=%d)", (void*)_rawval(o), int(_array(o)->Size())); break;
        case OT_TABLE: streamprintf(stream, "table(0x%p size=%d)", (void*)_rawval(o), int(_table(o)->CountUsed())); break;
        case OT_CLOSURE:
        {
            SQFunctionProto *func = _closure(o)->_function;
            const char *funcName = sq_isstring(func->_name) ? _stringval(func->_name) : "<null-name>";
            streamprintf(stream, "%s(0x%p \"%s\")", GetTypeName(o), (void*)func, funcName);
            break;
        }
        case OT_NATIVECLOSURE:
        {
            SQNativeClosure *nc = _nativeclosure(o);
            const char* funcName = sq_isstring(nc->_name) ? _stringval(nc->_name) : "<null-name>";
            streamprintf(stream, "%s(0x%p \"%s\")", GetTypeName(o), (void*)nc, funcName);
            break;
        }
        case OT_FUNCPROTO:
        {
            SQFunctionProto *func = _funcproto(o);
            const char* funcName = sq_isstring(func->_name) ? _stringval(func->_name) : "<null-name>";
            streamprintf(stream, "%s(0x%p \"%s\")", GetTypeName(o), (void*)func, funcName);
            break;
        }
        case OT_GENERATOR:
        {
            SQGenerator *gen = _generator(o);
            SQObjectPtr nameObj = _closure(gen->_closure)->_function->_name;
            const char* funcName = sq_isstring(nameObj) ? _stringval(nameObj) : "<null-name>";
            streamprintf(stream, "%s(0x%p \"%s\")", GetTypeName(o), (void*)gen, funcName);
            break;
        }
        default: streamprintf(stream, "%s(0x%p)", GetTypeName(o), (void*)_rawval(o)); break;
    }
}

SQFuncState::SQFuncState(SQSharedState *ss,SQFuncState *parent,SQCompilationContext &ctx) :
    _vlocals(ss->_alloc_ctx),
    _vlocals_info(ss->_alloc_ctx),
    _targetstack(ss->_alloc_ctx),
    _unresolvedbreaks(ss->_alloc_ctx),
    _unresolvedcontinues(ss->_alloc_ctx),
    _expr_block_results(ss->_alloc_ctx),
    _functions(ss->_alloc_ctx),
    _parameters(ss->_alloc_ctx),
    _param_type_masks(ss->_alloc_ctx),
    _outervalues(ss->_alloc_ctx),
    _outervalues_info(ss->_alloc_ctx),
    _instructions(ss->_alloc_ctx),
    _localvarinfos(ss->_alloc_ctx),
    _full_line_infos(ss->_alloc_ctx),
    _breaktargets(ss->_alloc_ctx),
    _continuetargets(ss->_alloc_ctx),
    _blockstacksizes(ss->_alloc_ctx),
    _defaultparams(ss->_alloc_ctx),
    _childstates(ss->_alloc_ctx),
    _ctx(ctx)
{
        _nliterals = 0;
        _literals = SQTable::Create(ss,0);
        _sharedstate = ss;
        _lastline = 0;
        _optimization = true;
        _parent = parent;
        _stacksize = 0;
        _traps = 0;
        _returnexp = 0;
        _varparams = false;
        _bgenerator = false;
        _purefunction = false;
        _nodiscard = false;
        _outers = 0;
        _hoistLevel = 0;
        _staticmemos_count = 0;
        _ss = ss;
        _result_type_mask = ~0u;
        lang_features = parent ? parent->lang_features : ss->defaultLangFeatures;
}

void ResetStaticMemos(SQFunctionProto *func, SQSharedState *ss)
{
    for (int i = 0; i < func->_nfunctions; i++) {
        SQObjectPtr &f = func->_functions[i];
        assert(sq_isfunction(f));
        ResetStaticMemos(_funcproto(f), ss);
    }

    if (func->_nstaticmemos) {
        SQInstruction* instr = func->_instructions;
        int count = func->_ninstructions;

        for (int i = 0; i < count; i += sq_opcode_length(instr[i].op)) {
            if (instr[i].op == _OP_LOAD_STATIC_MEMO) {
                SQInteger staticIdx = instr[i]._arg1;
                assert(unsigned(staticIdx) < func->_nstaticmemos);
                SQObjectPtr& storedStatic = func->_staticmemos[staticIdx];

                if (ISREFCOUNTED(sq_type(storedStatic))) {
                #ifdef NO_GARBAGE_COLLECTOR
                    assert(storedStatic._unVal.pRefCounted->_uiRef > 1);
                    __Release(storedStatic._type, storedStatic._unVal);
                #else
                    ss->_refs_table.Release(storedStatic);
                #endif
                }

                func->_staticmemos[staticIdx].Null();
                instr[i].op = _OP_DATA_NOP;
            }
        }
    }
}


SQInteger SQFuncState::GetNumericConstant(const SQInteger cons)
{
    return GetConstant(SQObjectPtr(cons));
}

SQInteger SQFuncState::GetNumericConstant(const SQFloat cons)
{
    return GetConstant(SQObjectPtr(cons));
}

SQInteger SQFuncState::GetConstant(const SQObjectPtr &cons, int max_const_no)
{
    SQObjectPtr val;
    max_const_no = max_const_no < MAX_LITERALS ? max_const_no : MAX_LITERALS;
    if(!_table(_literals)->Get(cons,val))
    {
        if(_nliterals >= max_const_no) {
            if (max_const_no == MAX_LITERALS)
            {
                val.Null();
                _ctx.reportDiagnostic(DiagnosticsId::DI_TOO_MANY_SYMBOLS, -1, -1, 0, "literals");
            } else
                return -1;
        }
        val = _nliterals;
        _table(_literals)->NewSlot(cons,val);
        _nliterals++;
    }
    int iv = _integer(val);
    return iv <= max_const_no ? iv : -1;
}

void SQFuncState::SetInstructionParams(SQInteger pos,SQInteger arg0,SQInteger arg1,SQInteger arg2,SQInteger arg3)
{
    _instructions[pos]._arg0=(unsigned char)*((SQUnsignedInteger *)&arg0);
    _instructions[pos]._arg1=(SQInt32)*((SQUnsignedInteger *)&arg1);
    _instructions[pos]._arg2=(unsigned char)*((SQUnsignedInteger *)&arg2);
    _instructions[pos]._arg3=(unsigned char)*((SQUnsignedInteger *)&arg3);
}

void SQFuncState::SetInstructionParam(SQInteger pos,SQInteger arg,SQInteger val)
{
    switch(arg){
        case 0:_instructions[pos]._arg0=(unsigned char)*((SQUnsignedInteger *)&val);break;
        case 1:case 4:_instructions[pos]._arg1=(SQInt32)*((SQUnsignedInteger *)&val);break;
        case 2:_instructions[pos]._arg2=(unsigned char)*((SQUnsignedInteger *)&val);break;
        case 3:_instructions[pos]._arg3=(unsigned char)*((SQUnsignedInteger *)&val);break;
    };
}

SQInteger SQFuncState::AllocStackPos()
{
    SQInteger npos=_vlocals.size();
    if(npos >= MAX_FUNC_STACKSIZE)
        _ctx.reportDiagnostic(DiagnosticsId::DI_TOO_MANY_SYMBOLS, -1, -1, 0, "locals");
    _vlocals.push_back(SQLocalVarInfo());
    _vlocals_info.push_back(SQCompiletimeVarInfo{});
    if(_vlocals.size()>((SQUnsignedInteger)_stacksize)) {
        _stacksize=_vlocals.size();
    }
    return npos;
}

SQInteger SQFuncState::PushTarget(SQInteger n)
{
    if(n!=-1){
        _targetstack.push_back(n);
        return n;
    }
    n=AllocStackPos();
    _targetstack.push_back(n);
    return n;
}

SQInteger SQFuncState::GetUpTarget(SQInteger n){
    return _targetstack[((_targetstack.size()-1)-n)];
}

SQInteger SQFuncState::TopTarget(){
    return _targetstack.back();
}
SQInteger SQFuncState::PopTarget()
{
    SQUnsignedInteger npos=_targetstack.back();
    assert(npos < _vlocals.size());
    SQLocalVarInfo &t = _vlocals[npos];
    if(sq_type(t._name)==OT_NULL){
        _vlocals.pop_back();
        _vlocals_info.pop_back();
    }
    _targetstack.pop_back();
    return npos;
}

SQInteger SQFuncState::GetStackSize()
{
    return _vlocals.size();
}

SQInteger SQFuncState::CountOuters(SQInteger stacksize)
{
    SQInteger outers = 0;
    SQInteger k = _vlocals.size() - 1;
    while(k >= stacksize) {
        SQLocalVarInfo &lvi = _vlocals[k];
        k--;
        if(lvi._end_op == UINT32_MINUS_ONE) { //this means is an outer
            outers++;
        }
    }
    return outers;
}

void SQFuncState::SetStackSize(SQInteger n)
{
    SQInteger size=_vlocals.size();
    while(size>n){
        size--;
        SQLocalVarInfo lvi = _vlocals.back();
        if(sq_type(lvi._name)!=OT_NULL){
            if(lvi._end_op == UINT32_MINUS_ONE) { //this means is an outer
                _outers--;
            }
            lvi._end_op = GetCurrentPos();
            _localvarinfos.push_back(lvi);
        }
        _vlocals.pop_back();
        _vlocals_info.pop_back();
    }
}


bool SQFuncState::IsLocal(SQUnsignedInteger stkpos)
{
    if(stkpos>=_vlocals.size())return false;
    else if(sq_type(_vlocals[stkpos]._name)!=OT_NULL)return true;
    return false;
}

SQInteger SQFuncState::PushLocalVariable(const SQObject &name, const SQCompiletimeVarInfo& ct_var_info)
{
    SQInteger pos=_vlocals.size();
    SQLocalVarInfo lvi;
    lvi._name=name;
    lvi._start_op=GetCurrentPos()+1;
    lvi._pos=_vlocals.size();
    lvi._varFlags=ct_var_info.var_flags;
    _vlocals.push_back(lvi);
    _vlocals_info.push_back(ct_var_info);
    if(_vlocals.size()>((SQUnsignedInteger)_stacksize))_stacksize=_vlocals.size();
    return pos;
}



SQInteger SQFuncState::GetLocalVariable(const SQObject &name, SQCompiletimeVarInfo& ct_var_info)
{
    SQInteger locals=_vlocals.size();
    while(locals>=1){
        SQLocalVarInfo &lvi = _vlocals[locals-1];
        if(sq_type(lvi._name)==OT_STRING && _string(lvi._name)==_string(name)){
            ct_var_info = _vlocals_info[locals-1];
            return locals-1;
        }
        locals--;
    }
    return -1;
}

void SQFuncState::MarkLocalAsOuter(SQInteger pos)
{
    SQLocalVarInfo &lvi = _vlocals[pos];
    lvi._end_op = UINT32_MINUS_ONE;
    _outers++;
}


SQInteger SQFuncState::GetOuterVariable(const SQObject &name, SQCompiletimeVarInfo &varInfo)
{
    SQInteger outers = _outervalues.size();
    for(SQInteger i = 0; i<outers; i++) {
        if(_string(_outervalues[i]._name) == _string(name)) {
            varInfo = _outervalues_info[i];
            return i;
        }
    }
    SQInteger pos=-1;
    if(_parent) {
        pos = _parent->GetLocalVariable(name, varInfo);
        if(pos == -1) {
            pos = _parent->GetOuterVariable(name, varInfo);
            if(pos != -1) {
                _outervalues.push_back(SQOuterVar(SQObjectPtr(name), SQObjectPtr(SQInteger(pos)), otOUTER, varInfo.var_flags)); //local
                _outervalues_info.push_back(varInfo);
                return _outervalues.size() - 1;
            }
        }
        else {
            _parent->MarkLocalAsOuter(pos);
            _outervalues.push_back(SQOuterVar(SQObjectPtr(name), SQObjectPtr(SQInteger(pos)), otLOCAL, varInfo.var_flags)); //local
            _outervalues_info.push_back(varInfo);
            return _outervalues.size() - 1;
        }
    }
    return -1;
}

void SQFuncState::AddParameter(const SQObject &name, SQUnsignedInteger32 type_mask) // TODO: initializer node
{
    PushLocalVariable(name, SQCompiletimeVarInfo{VF_PARAM | VF_ASSIGNABLE, type_mask, nullptr});
    _parameters.push_back(SQObjectPtr(name));
    _param_type_masks.push_back(type_mask);
}

void SQFuncState::AddLineInfos(SQInteger line, bool is_dbg_step_point, bool force)
{
    if(_lastline!=line || force){
        if(_lastline!=line) {
            SQFullLineInfo li;
            li._op = (GetCurrentPos()+1);
            li._line_offset = line;
            li._is_dbg_step_point = is_dbg_step_point;
            _full_line_infos.push_back(li);
            if (is_dbg_step_point && _ctx.getVm()->_compile_line_hook)
                _ctx.getVm()->_compile_line_hook(_ctx.getVm(), _ctx.sourceName(), line);
        }
        _lastline=line;
    }
}

void SQFuncState::DiscardTarget()
{
    SQInteger discardedtarget = PopTarget();
    SQInteger size = _instructions.size();
    if(size > 0 && _optimization){
        SQInstruction &pi = _instructions[size-1];//previous instruction
        switch(pi.op) {
        case _OP_SETI:case _OP_SETK:case _OP_SET:case _OP_NEWSLOTK:case _OP_NEWSLOT:case _OP_SETOUTER:case _OP_CALL:case _OP_NULLCALL:
            if(pi._arg0 == discardedtarget) {
                pi._arg0 = 0xFF;
            }
        }
    }
}

void SQFuncState::AddInstruction(SQInstruction &i)
{
    SQInteger size = _instructions.size();
    if (size > 0 && _optimization && !(lang_features & LF_DISABLE_OPTIMIZER)){ //simple optimizer
        SQInstruction &pi = _instructions[size-1];//previous instruction
        switch(i.op) {
        case _OP_JZ:
            if( pi.op == _OP_CMP && pi._arg1 < 0xFF) {
                pi.op = _OP_JCMP;
                pi._arg0 = (unsigned char)pi._arg1;
                pi._arg3 |= (uint8_t(i._arg2 ? 1 : 0)<<3);
                pi._arg1 = i._arg1;
                return;
            }
            break;
        case _OP_SET:
            if(i._arg0 == i._arg3) {
                i._arg0 = 0xFF;
            }
            if( pi.op == _OP_LOAD && pi._arg0 == i._arg1 && (!IsLocal(pi._arg0))){
                // arg1 is size of int
                pi._arg2 = i._arg2;
                pi.op = _OP_SETK;
                pi._arg0 = i._arg0;
                pi._arg3 = i._arg3;
                return;
            }
            if( pi.op == _OP_LOADINT && pi._arg0 == i._arg1 && (!IsLocal(pi._arg0))){
                pi._arg2 = i._arg2;
                pi.op = _OP_SETI;
                // arg1 is size of int
                pi._arg0 = i._arg0;
                pi._arg3 = i._arg3;
                return;
            }
            break;
        case _OP_NEWSLOT:
            if(i._arg0 == i._arg3) {
                i._arg0 = 0xFF;
            }
            if( (pi.op == _OP_LOADINT || pi.op == _OP_LOAD) && pi._arg0 == i._arg1 && (!IsLocal(pi._arg0))){
                // arg1 is size of int
                pi._arg1 = pi.op == _OP_LOADINT ? GetNumericConstant((SQInteger)pi._arg1) : pi._arg1;
                pi._arg0 = i._arg0;
                pi._arg3 = i._arg3;
                pi._arg2 = i._arg2;
                pi.op = _OP_NEWSLOTK;
                return;
            }
            break;
        case _OP_SETI:
        case _OP_SETK:
        case _OP_NEWSLOTK:
            if(i._arg0 == i._arg3) {
                i._arg0 = 0xFF;
            }
            break;
        case _OP_SETOUTER:
            if(i._arg0 == i._arg2) {
                i._arg0 = 0xFF;
            }
            break;
        case _OP_RETURN:
            if( _parent && i._arg0 != MAX_FUNC_STACKSIZE && pi.op == _OP_CALL && _returnexp < size-1) {
                pi.op = _OP_TAILCALL;
            } else if(pi.op == _OP_CLOSE){
                pi = i;
                return;
            }
        break;
        case _OP_GET:
            if( pi.op == _OP_LOAD && pi._arg0 == i._arg1 && (!IsLocal(pi._arg0))){
                // arg1 is size of int
                pi._arg2 = i._arg2;
                pi.op = _OP_GETK;
                pi._arg0 = i._arg0;
                pi._arg3 = i._arg3;
                return;
            }
            if( pi.op == _OP_LOADINT && pi._arg0 == i._arg1 && (!IsLocal(pi._arg0))){
                // arg1 is size of int
                // if (GetNumericConstant((SQInteger)pi._arg1) < 256)
                pi._arg2 = i._arg2;
                pi._arg1 = GetNumericConstant((SQInteger)pi._arg1);
                pi.op = _OP_GETK;
                pi._arg0 = i._arg0;
                pi._arg3 = i._arg3;
                return;
            }
        break;
        case _OP_PREPCALL:
            if( pi.op == _OP_LOAD  && pi._arg0 == i._arg1 && (!IsLocal(pi._arg0))){
                pi.op = _OP_PREPCALLK;
                pi._arg0 = i._arg0;
                pi._arg2 = i._arg2;
                pi._arg3 = i._arg3;
                return;
            }
            break;
        case _OP_APPENDARRAY: {
            SQInteger aat = -1;
            switch(pi.op) {
            case _OP_LOAD: aat = AAT_LITERAL; break;
            case _OP_LOADINT: aat = AAT_INT; break;
            case _OP_LOADBOOL: aat = AAT_BOOL; break;
            case _OP_LOADFLOAT: aat = AAT_FLOAT; break;
            default: break;
            }
            if(aat != -1 && pi._arg0 == i._arg1 && (!IsLocal(pi._arg0))){
                pi.op = _OP_APPENDARRAY;
                pi._arg0 = i._arg0;
                pi._arg2 = (unsigned char)aat;
                pi._arg3 = MAX_FUNC_STACKSIZE;
                return;
            }
                              }
            break;
        case _OP_MOVE:
            switch(pi.op) {
            case _OP_GET: case _OP_GETK:
            case _OP_ADD: case _OP_SUB: case _OP_MUL: case _OP_DIV: case _OP_MOD: case _OP_BITW:
            case _OP_LOADINT: case _OP_LOADFLOAT: case _OP_LOADBOOL: case _OP_LOAD:
            case _OP_NEG: case _OP_NOT: case _OP_BWNOT:
            case _OP_ADDI:
            case _OP_CMP:
            case _OP_TYPEOF:
            case _OP_CLONE:

                if(pi._arg0 == i._arg1 && !IsLocal(pi._arg0))
                {
                    pi._arg0 = i._arg0;
                    _optimization = false;
                    return;
                }
            }

            if(pi.op == _OP_GETOUTER && i._arg1 < 256 && i._arg0 && !pi._arg2)
            {
                pi._arg2 = i._arg0;
                pi._arg3 = (unsigned char)i._arg1;
                return;
            }

            if(pi.op == _OP_LOADCALLEE && i._arg1 < 256 && i._arg0 && !pi._arg2)
            {
                pi._arg2 = i._arg0;
                pi._arg3 = (unsigned char)i._arg1;
                return;
            }

            if(pi.op == _OP_MOVE && i._arg1 < 256)
            {
                pi.op = _OP_DMOVE;
                pi._arg2 = i._arg0;
                pi._arg3 = (unsigned char)i._arg1;
                return;
            }
            break;
        case _OP_LOAD:
            if(pi.op == _OP_LOAD && i._arg1 < 256) {
                pi.op = _OP_DLOAD;
                pi._arg2 = i._arg0;
                pi._arg3 = (unsigned char)i._arg1;
                return;
            }
            break;
        case _OP_EQ:case _OP_NE:
            if((pi.op == _OP_LOAD || pi.op == _OP_LOADINT || pi.op == _OP_LOADFLOAT || pi.op == _OP_LOADBOOL) && pi._arg0 == i._arg1 && (!IsLocal(pi._arg0) ))
            {
                if (pi.op != _OP_LOAD)
                {
                    pi._arg1 = pi.op == _OP_LOADINT ? GetNumericConstant((SQInteger)pi._arg1) : pi.op == _OP_LOADFLOAT ? GetNumericConstant(pi._farg1) : GetConstant(SQObjectPtr(bool(pi._arg1)));
                }
                pi.op = i.op;
                pi._arg0 = i._arg0;
                pi._arg2 = i._arg2;
                pi._arg3 = 1;
                return;
            }
            break;
        case _OP_LOADNULLS:
            if((pi.op == _OP_LOADNULLS && pi._arg0+pi._arg1 == i._arg0)) {

                pi._arg1 = pi._arg1 + 1;
                pi.op = _OP_LOADNULLS; //-V1048
                return;
            }
            break;
        }
    }
    _optimization = true;
    _instructions.push_back(i);
}

SQObjectPtr SQFuncState::CreateString(const char *s,SQInteger len)
{
    return SQObjectPtr(SQString::Create(_sharedstate,s,len));
}

void SQFuncState::CheckForPurity()
{
    // Only set pure flag to true.
    // Don't reset it to false, don't assume that non-pure operations make the function impure.
    // Allow calls, arithmetics and other operations.
    int count = _instructions.size();
    for (int i = 0; i < count; i += sq_opcode_length(_instructions[i].op)) {
        if (!sq_is_pure_op(_instructions[i].op)) {
            return;
        }
    }

    _purefunction = true;
}

SQFunctionProto *SQFuncState::BuildProto()
{
    bool useCompressedLineInfos = true;
    int firstLine = INT_MAX;
    int lastLine = 0;

    for (SQUnsignedInteger ni = 0; ni < _full_line_infos.size(); ni++) {
        int line = _full_line_infos[ni]._line_offset;
        if (line < firstLine)
            firstLine = line;
        if (line > lastLine)
            lastLine = line;
        if (_full_line_infos[ni]._op > 255)
            useCompressedLineInfos = false;
    }

    if (lastLine - firstLine > 127)
        useCompressedLineInfos = false;

    SQFunctionProto *f=SQFunctionProto::Create(_ss,lang_features,_instructions.size(),
        _nliterals,_parameters.size(),_functions.size(),_outervalues.size(),
        _full_line_infos.size(),useCompressedLineInfos,_localvarinfos.size(),_defaultparams.size(),
        _staticmemos_count);

    SQObjectPtr refidx,key,val;
    SQInteger idx;

    f->_stacksize = _stacksize;
    f->_sourcename = _sourcename;
    f->_bgenerator = _bgenerator;
    f->_purefunction = _purefunction;
    f->_nodiscard = _nodiscard;
    f->_name = _name;
    f->_inside_hoisted_scope = _hoistLevel > 0;
    f->_result_type_mask = _result_type_mask;

    while((idx=_table(_literals)->Next(false,refidx,key,val))!=-1) {
        f->_literals[_integer(val)]=key;
        refidx=idx;
    }

    for(SQUnsignedInteger nf = 0; nf < _functions.size(); nf++) f->_functions[nf] = _functions[nf];
    for(SQUnsignedInteger no = 0; no < _outervalues.size(); no++) f->_outervalues[no] = _outervalues[no];
    for(SQUnsignedInteger nl = 0; nl < _localvarinfos.size(); nl++) f->_localvarinfos[nl] = _localvarinfos[nl];

    for(SQUnsignedInteger np = 0; np < _parameters.size(); np++) {
        f->_parameters[np] = _parameters[np];
        f->_param_type_masks[np] = _param_type_masks[np];
    }

    f->_lineinfos->_is_compressed = useCompressedLineInfos;
    f->_lineinfos->_first_line = firstLine;
    if (useCompressedLineInfos) {
        for(SQUnsignedInteger ni = 0; ni < _full_line_infos.size(); ni++) {
            SQCompressedLineInfo *li = (SQCompressedLineInfo *)(void *)(f->_lineinfos + 1) + ni;
            li->_op = _full_line_infos[ni]._op;
            li->_line_offset = _full_line_infos[ni]._line_offset - (unsigned)firstLine;
            li->_is_dbg_step_point = _full_line_infos[ni]._is_dbg_step_point;
        }
    } else {
        for(SQUnsignedInteger ni = 0; ni < _full_line_infos.size(); ni++) {
            SQFullLineInfo *li = (SQFullLineInfo *)(void *)(f->_lineinfos + 1) + ni;
            *li = _full_line_infos[ni];
            li->_line_offset -= firstLine;
        }
    }

    for(SQUnsignedInteger nd = 0; nd < _defaultparams.size(); nd++) f->_defaultparams[nd] = _defaultparams[nd];

    memcpy(f->_instructions,&_instructions[0],_instructions.size()*sizeof(SQInstruction));

    f->_varparams = _varparams;

    return f;
}

SQFuncState *SQFuncState::PushChildState(SQSharedState *ss)
{
    SQFuncState *child = (SQFuncState *)sq_malloc(ss->_alloc_ctx, sizeof(SQFuncState));
    new (child) SQFuncState(ss,this,_ctx);
    _childstates.push_back(child);
    return child;
}

void SQFuncState::PopChildState()
{
    SQFuncState *child = _childstates.back();
    SQAllocContext ctx = _ss->_alloc_ctx;
    sq_delete(ctx, child,SQFuncState);
    _childstates.pop_back();
}

SQFuncState::~SQFuncState()
{
    while(_childstates.size() > 0)
    {
        PopChildState();
    }
}

#endif
