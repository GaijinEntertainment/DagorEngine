/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#ifndef NO_COMPILER
#include "sqcompiler.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqtable.h"
#include "sqopcodes.h"
#include "sqfuncstate.h"
#include "sqastio.h"
#include "sqclosure.h"

#include <stdarg.h>

#define SQ_OPCODE(id) {_SC(#id)},

SQInstructionDesc g_InstrDesc[]={
    SQ_OPCODES_LIST
};

#undef SQ_OPCODE


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
        case OT_NULL: streamprintf(stream, _SC("null")); break;
        case OT_STRING: streamprintf(stream, _SC("string(\"%s\")"), _stringval(o)); break;
        case OT_FLOAT: streamprintf(stream, _SC("float(%f)"), _float(o)); break;
        case OT_INTEGER: streamprintf(stream, _SC("int(") _PRINT_INT_FMT _SC(")"), _integer(o)); break;
        case OT_BOOL: streamprintf(stream, _SC("%s"), _integer(o) ? _SC("bool(true)") : _SC("bool(false)")); break;
        case OT_CLOSURE:
        {
            SQFunctionProto *func = _closure(o)->_function;
            const SQChar *funcName = sq_isstring(func->_name) ? _stringval(func->_name) : _SC("<null-name>");
            streamprintf(stream, _SC("%s(0x%p \"%s\")"), GetTypeName(o), (void*)func, funcName);
            break;
        }
        case OT_NATIVECLOSURE:
        {
            SQNativeClosure *nc = _nativeclosure(o);
            const SQChar* funcName = sq_isstring(nc->_name) ? _stringval(nc->_name) : _SC("<null-name>");
            streamprintf(stream, _SC("%s(0x%p \"%s\")"), GetTypeName(o), (void*)nc, funcName);
            break;
        }
        case OT_FUNCPROTO:
        {
            SQFunctionProto *func = _funcproto(o);
            const SQChar* funcName = sq_isstring(func->_name) ? _stringval(func->_name) : _SC("<null-name>");
            streamprintf(stream, _SC("%s(0x%p \"%s\")"), GetTypeName(o), (void*)func, funcName);
            break;
        }
        case OT_GENERATOR:
        {
            SQGenerator *gen = _generator(o);
            SQObjectPtr nameObj = _closure(gen->_closure)->_function->_name;
            const SQChar* funcName = sq_isstring(nameObj) ? _stringval(nameObj) : _SC("<null-name>");
            streamprintf(stream, _SC("%s(0x%p \"%s\")"), GetTypeName(o), (void*)gen, funcName);
            break;
        }
        default: streamprintf(stream, _SC("%s(0x%p)"), GetTypeName(o), (void*)_rawval(o)); break; //shut up compiler
    }
}

SQFuncState::SQFuncState(SQSharedState *ss,SQFuncState *parent,SQCompilationContext &ctx) :
    _vlocals(ss->_alloc_ctx),
    _vlocals_nodes(ss->_alloc_ctx),
    _targetstack(ss->_alloc_ctx),
    _unresolvedbreaks(ss->_alloc_ctx),
    _unresolvedcontinues(ss->_alloc_ctx),
    _functions(ss->_alloc_ctx),
    _parameters(ss->_alloc_ctx),
    _outervalues(ss->_alloc_ctx),
    _outervalues_nodes(ss->_alloc_ctx),
    _instructions(ss->_alloc_ctx),
    _localvarinfos(ss->_alloc_ctx),
    _lineinfos(ss->_alloc_ctx),
    _breaktargets(ss->_alloc_ctx),
    _continuetargets(ss->_alloc_ctx),
    _blockstacksizes(ss->_alloc_ctx),
    _defaultparams(ss->_alloc_ctx),
    _childstates(ss->_alloc_ctx),
    _ctx(ctx)
{
        _nliterals = 0;
        _literals = SQTable::Create(ss,0);
        _ref_holder =  SQTable::Create(ss,0);
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
        _outers = 0;
        _hoistLevel = 0;
        _staticmemos_count = 0;
        _ss = ss;
        lang_features = parent ? parent->lang_features : ss->defaultLangFeatures;
}

void Dump(SQFunctionProto *func) {
    FileOutputStream fos(stdout);
    Dump(&fos, func, false);
}

static uint64_t GetUInt64FromPtr(const void *ptr)
{
    uint64_t res = 0;
    for (int i = 0; i < 8; i++)
        res += uint64_t(((const uint8_t *)ptr)[i]) << (i * 8);
    return res;
}

void DumpInstructions(OutputStream *stream, SQLineInfo * lineinfos, int nlineinfos, const SQInstruction *ii, SQInt32 i_count,
    const SQObjectPtr *_literals, SQInt32 _nliterals, int instruction_index)
{
    SQInt32 n = 0;
    int lineHint = 0;
    for (int i = 0; i < i_count; i++) {
        const SQInstruction &inst = ii[i];
        bool isLineOp = false;
        char curInstr = instruction_index == i ? '>' : ' ';
        SQInteger line = SQFunctionProto::GetLine(lineinfos, nlineinfos, i, &lineHint, &isLineOp);
        if (inst.op == _OP_LOAD || inst.op == _OP_DLOAD || inst.op == _OP_PREPCALLK || inst.op == _OP_GETK) {
            SQInteger lidx = inst._arg1;
            streamprintf(stream, _SC("[line%c%03d]%c[op %03d] %15s %d "), isLineOp ? '-' : ' ', (SQInt32)line, curInstr, (SQInt32)n,
                g_InstrDesc[inst.op].name, inst._arg0);
            if (lidx >= 0xFFFFFFFF) //-V547
                streamprintf(stream, _SC("null"));
            else {
                assert(lidx < _nliterals);
                SQObjectPtr key = _literals[lidx];
                DumpLiteral(stream, key);
            }
            if (inst.op != _OP_DLOAD) {
                streamprintf(stream, _SC(" %d %d"), inst._arg2, inst._arg3);
            }
            else {
                streamprintf(stream, _SC(" %d "), inst._arg2);
                lidx = inst._arg3;
                if (lidx >= 0xFFFFFFFF) //-V547
                    streamprintf(stream, _SC("null"));
                else {
                    assert(lidx < _nliterals);
                    SQObjectPtr key = _literals[lidx];
                    DumpLiteral(stream, key);
                }
            }
        }
        /*  else if(inst.op==_OP_ARITH){
                printf(_SC("[%03d] %15s %d %d %d %c\n"),n,g_InstrDesc[inst.op].name,inst._arg0,inst._arg1,inst._arg2,inst._arg3);
            }*/
        else {
            int jumpLabel = 0x7FFFFFFF;
            switch (inst.op) {
            case _OP_JMP: case _OP_JCMP: case _OP_JZ: case _OP_AND: case _OP_OR: case _OP_PUSHTRAP: case _OP_FOREACH: case _OP_POSTFOREACH: case _OP_PREFOREACH:
                jumpLabel = i + inst._arg1 + 1;
                break;
            case _OP_JCMPI: case _OP_JCMPF:
                jumpLabel = i + inst._sarg0() + 1;
                break;
            case _OP_NULLCOALESCE:
                jumpLabel = i + inst._arg1;
                break;
            default:
                break;
            }
            bool arg1F = inst.op == _OP_JCMPF || inst.op == _OP_LOADFLOAT;
            if (arg1F)
                streamprintf(stream, _SC("[line%c%03d]%c[op %03d] %15s %d %f %d %d"), isLineOp ? '-' : ' ', (SQInt32)line, curInstr, (SQInt32)n,
                    g_InstrDesc[inst.op].name, inst._arg0, inst._farg1, inst._arg2, inst._arg3);
            else
            {
                if (i > 0 && (ii[i - 1].op == _OP_SET_LITERAL || ii[i - 1].op == _OP_GET_LITERAL))
                    streamprintf(stream, _SC("[line%c%03d]%c[op %03d] HINT (0x%") _SC(PRIx64) _SC(")"), isLineOp ? '-' : ' ', (SQInt32)line, curInstr, (SQInt32)n,
                        GetUInt64FromPtr(ii + i));
                else if (inst.op < sizeof(g_InstrDesc) / sizeof(g_InstrDesc[0]))
                    streamprintf(stream, _SC("[line%c%03d]%c[op %03d] %15s %d %d %d %d"), isLineOp ? '-' : ' ', (SQInt32)line, curInstr, (SQInt32)n,
                        g_InstrDesc[inst.op].name, inst._arg0, inst._arg1, inst._arg2, inst._arg3);
                else
                    streamprintf(stream, _SC("[line%c%03d]%c[op %03d] INVALID INSTRUNCTION (%d)"), isLineOp ? '-' : ' ', (SQInt32)line, curInstr, (SQInt32)n,
                        int(inst.op));
            }
            if (jumpLabel != 0x7FFFFFFF)
                streamprintf(stream, _SC("  // jump to %d"), jumpLabel);
        }

        // comments
        switch (inst.op) {
            case _OP_LOAD:
            case _OP_LOADFLOAT:
            case _OP_LOADINT:
            case _OP_LOADBOOL:
            case _OP_LOADROOT:
            case _OP_LOADCALLEE:
                streamprintf(stream, _SC("  // -> [%d]"), int(inst._arg0));
                break;

            case _OP_LOADNULLS:
                streamprintf(stream, _SC("  // null -> [%d .. %d]"), int(inst._arg0), int(inst._arg0 + inst._arg1 - 1));
                break;

            case _OP_DLOAD: {
                streamprintf(stream, _SC("  //"));
                if (unsigned(inst._arg1) < unsigned(_nliterals)) {
                    const SQObjectPtr &key = _literals[inst._arg1];
                    DumpLiteral(stream, key);
                }
                else {
                    streamprintf(stream, _SC("?"));
                }
                streamprintf(stream, _SC(" -> [%d], "), int(inst._arg0));
                if (unsigned(inst._arg3) < unsigned(_nliterals)) {
                    const SQObjectPtr &key = _literals[inst._arg3];
                    DumpLiteral(stream, key);
                }
                else {
                    streamprintf(stream, _SC("?"));
                }
                streamprintf(stream, _SC(" -> [%d]"), int(inst._arg2));
                break;
            }

            case _OP_DATA_NOP: {
                    int saveInstr = i + ((inst._arg2 << 8) + inst._arg3);
                    if ((inst._arg0 || inst._arg1 || inst._arg2 || inst._arg3) && saveInstr > 0 && saveInstr < i_count &&
                        ii[saveInstr].op == _OP_SAVE_STATIC_MEMO)
                    {
                        int loadInstr = saveInstr + 1 - ((ii[saveInstr]._arg2 << 8) + ii[saveInstr]._arg3);
                        if (loadInstr == i) {
                            streamprintf(stream, _SC("  // LOAD_STATIC_MEMO: staticmemo[%d] -> [%d], jump to %d"),
                                int(inst._arg1), int(inst._arg0), saveInstr + 1);
                        }
                    }
                }
                break;

            case _OP_SAVE_STATIC_MEMO: {
                    int loadInstr = i + 1 - ((inst._arg2 << 8) + inst._arg3);
                    streamprintf(stream, _SC("  // [%d] -> staticmemo[%d], [%d] -> [%d], modify instr at %d"),
                        int(inst._arg0), int(inst._arg1), int(inst._arg0), int(ii[loadInstr]._arg0), loadInstr);
                }
                break;

            case _OP_CALL:
                streamprintf(stream, _SC("  // (call [%d]) -> [%d]"), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_NULLCALL:
                streamprintf(stream, _SC("  // (if [%d] then call [%d]) -> [%d]"), int(inst._arg1), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_MOVE:
                streamprintf(stream, _SC("  // [%d] -> [%d]"), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_GET:
                streamprintf(stream, _SC("  // [%d].[%d] -> [%d]"), int(inst._arg2), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_NEWOBJ: {
                const SQChar *objType = inst._arg3 == NEWOBJ_TABLE ? _SC("table") : (inst._arg3 == NEWOBJ_ARRAY ? _SC("array") :
                    (inst._arg3 == NEWOBJ_CLASS ? _SC("class") : _SC("unknown")));
                streamprintf(stream, _SC("  // new %s -> [%d]"), objType, int(inst._arg0));
                break;
            }

            case _OP_CLOSURE:
                streamprintf(stream, _SC("  // closure -> [%d]"), int(inst._arg0));
                break;

            case _OP_APPENDARRAY: {
                streamprintf(stream, _SC("  // [%d].append("), int(inst._arg0));
                switch (inst._arg2) {
                    case AAT_STACK:
                        streamprintf(stream, _SC("[%d]"), int(inst._arg1));
                        break;
                    case AAT_LITERAL:
                        if (unsigned(inst._arg1) < unsigned(_nliterals)) {
                            const SQObjectPtr &key = _literals[inst._arg1];
                            DumpLiteral(stream, key);
                        }
                        else {
                            streamprintf(stream, _SC("?"));
                        }
                        break;
                    case AAT_INT:
                        streamprintf(stream, _SC("%d"), int(inst._arg1));
                        break;
                    case AAT_FLOAT:
                        streamprintf(stream, _SC("%f"), *((const SQFloat *)&inst._arg1));
                        break;
                    case AAT_BOOL:
                        streamprintf(stream, _SC("%s"), inst._arg1 ? _SC("true") : _SC("false"));
                        break;
                    default:
                        streamprintf(stream, _SC("unknown"));
                        break;
                }

                streamprintf(stream, _SC(")"));
                break;
            }

            case _OP_GETK:
            case _OP_GET_LITERAL: {
                streamprintf(stream, _SC("  // [%d].["), int(inst._arg2));
                if (unsigned(inst._arg1) < unsigned(_nliterals)) {
                    const SQObjectPtr &key = _literals[inst._arg1];
                    DumpLiteral(stream, key);
                }
                else {
                    streamprintf(stream, _SC("?"));
                }
                streamprintf(stream, _SC("] -> [%d]"), int(inst._arg0));
                break;
            }

            default:
                break;
        }
        streamprintf(stream, _SC("\n"));
        n++;
    }
}

static void DumpLiterals(OutputStream *stream, const SQObjectPtr *_literals, SQInt32 _nliterals)
{
    streamprintf(stream, _SC("-----LITERALS\n"));
    for (SQInt32 i = 0; i < _nliterals; ++i) {
        streamprintf(stream, _SC("[%d] "), (SQInt32)i);
        DumpLiteral(stream, _literals[i]);
        streamprintf(stream, _SC("\n"));
    }
}

static void DumpLocals(OutputStream *stream, const SQLocalVarInfo *_localvarinfos, SQInt32 _nlocalvarinfos)
{
    streamprintf(stream, _SC("-----LOCALS\n"));
    for (SQInt32 si = 0; si < _nlocalvarinfos; si++) {
        SQLocalVarInfo lvi = _localvarinfos[si];
        streamprintf(stream, _SC("[%d] %s \t%d %d\n"), (SQInt32)lvi._pos, _stringval(lvi._name), (SQInt32)lvi._start_op, (SQInt32)lvi._end_op);
    }
}

static void DumpLineInfo(OutputStream *stream, const SQLineInfo *_lineinfos, SQInt32 _nlineinfos)
{
    streamprintf(stream, _SC("-----LINE INFO\n"));
    for (SQInt32 i = 0; i < _nlineinfos; i++) {
        SQLineInfo li = _lineinfos[i];
        streamprintf(stream, _SC("op [%d] line [%d]%s\n"), (SQInt32)li._op, (SQInt32)li._line, li._is_line_op ? " line_op" : "");
    }
}

void Dump(OutputStream *stream, SQFunctionProto *func, bool deep, int instruction_index)
{

    //if (!dump_enable) return ;
    SQUnsignedInteger n = 0, i;
    SQInteger si;
    if (deep) {
        for (i = 0; i < func->_nfunctions; ++i) {
            SQObjectPtr &f = func->_functions[i];
            assert(sq_isfunction(f));
            Dump(stream, _funcproto(f), deep, -1);
        }
    }
    streamprintf(stream, _SC("SQInstruction sizeof %d\n"), (SQInt32)sizeof(SQInstruction));
    streamprintf(stream, _SC("SQObject sizeof %d\n"), (SQInt32)sizeof(SQObject));
    streamprintf(stream, _SC("--------------------------------------------------------------------\n"));
    streamprintf(stream, _SC("*****FUNCTION [%s]\n"), sq_type(func->_name) == OT_STRING ? _stringval(func->_name) : _SC("unknown"));
    DumpLiterals(stream, func->_literals, func->_nliterals);
    streamprintf(stream, _SC("-----PARAMS\n"));
    if (func->_varparams)
        streamprintf(stream, _SC("<<VARPARAMS>>\n"));
    n = 0;
    for (i = 0; i < func->_nparameters; i++) {
        streamprintf(stream, _SC("[%d] "), (SQInt32)n);
        DumpLiteral(stream, func->_parameters[i]);
        streamprintf(stream, _SC("\n"));
        n++;
    }
    DumpLocals(stream, func->_localvarinfos, func->_nlocalvarinfos);
    DumpLineInfo(stream, func->_lineinfos, func->_nlineinfos);
    streamprintf(stream, _SC("-----dump\n"));
    DumpInstructions(stream, func->_lineinfos, func->_nlineinfos,
        func->_instructions, func->_ninstructions, func->_literals, func->_nliterals, instruction_index);
    streamprintf(stream, _SC("-----\n"));
    streamprintf(stream, _SC("stack size[%d]\n"), (SQInt32)func->_stacksize);
    streamprintf(stream, _SC("--------------------------------------------------------------------\n\n"));
}

void Dump(OutputStream *stream, const SQFuncState *fState, int instruction_index)
{
    streamprintf(stream, _SC("*****FUNCTION [%s]\n"), sq_type(fState->_name) == OT_STRING ? _stringval(fState->_name) : _SC("unknown"));
    SQObjectPtrVec literals(fState->_sharedstate->_alloc_ctx);
    literals.resize(_table(fState->_literals)->CountUsed());
    {
        SQObjectPtr refidx,key,val;
        SQInteger idx;
        while((idx=_table(fState->_literals)->Next(false,refidx,key,val))!=-1) {
            literals[_integer(val)]=key;
            refidx=idx;
        }
    }
    DumpLiterals(stream, literals.begin(), literals.size());
    DumpLocals(stream, fState->_localvarinfos.begin(), fState->_localvarinfos.size());
    DumpLineInfo(stream, fState->_lineinfos.begin(), fState->_lineinfos.size());
    streamprintf(stream, _SC("-----dump\n"));
    DumpInstructions(stream, &fState->_lineinfos[0], fState->_lineinfos.size(),
        fState->_instructions.begin(), fState->_instructions.size(), literals.begin(), literals.size(), instruction_index);
}

void Dump(const SQFuncState *fState)
{
    FileOutputStream fos(stdout);
    Dump(&fos, fState, -1);
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
                assert(unsigned(instr[i]._arg1) < func->_nstaticmemos);
                func->_staticmemos[instr[i]._arg1].Null();
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

SQInteger SQFuncState::GetConstant(const SQObject &cons, int max_const_no)
{
    SQObjectPtr val;
    max_const_no = max_const_no < MAX_LITERALS ? max_const_no : MAX_LITERALS;
    if(!_table(_literals)->Get(SQObjectPtr(cons),val))
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
        _table(_literals)->NewSlot(SQObjectPtr(cons),val);
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
    _vlocals.push_back(SQLocalVarInfo());
    _vlocals_nodes.push_back(nullptr);
    if(_vlocals.size()>((SQUnsignedInteger)_stacksize)) {
        if(_stacksize>MAX_FUNC_STACKSIZE)
            _ctx.reportDiagnostic(DiagnosticsId::DI_TOO_MANY_SYMBOLS, -1, -1, 0, "locals");
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
        _vlocals_nodes.pop_back();
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
        _vlocals_nodes.pop_back();
    }
}


bool SQFuncState::IsLocal(SQUnsignedInteger stkpos)
{
    if(stkpos>=_vlocals.size())return false;
    else if(sq_type(_vlocals[stkpos]._name)!=OT_NULL)return true;
    return false;
}

SQInteger SQFuncState::PushLocalVariable(const SQObject &name, char varFlags, Expr *node)
{
    SQInteger pos=_vlocals.size();
    SQLocalVarInfo lvi;
    lvi._name=name;
    lvi._start_op=GetCurrentPos()+1;
    lvi._pos=_vlocals.size();
    lvi._varFlags=varFlags;
    _vlocals.push_back(lvi);
    _vlocals_nodes.push_back(node);
    if(_vlocals.size()>((SQUnsignedInteger)_stacksize))_stacksize=_vlocals.size();
    return pos;
}



SQInteger SQFuncState::GetLocalVariable(const SQObject &name, char &varFlags, Expr **node)
{
    SQInteger locals=_vlocals.size();
    while(locals>=1){
        SQLocalVarInfo &lvi = _vlocals[locals-1];
        if(sq_type(lvi._name)==OT_STRING && _string(lvi._name)==_string(name)){
            varFlags = lvi._varFlags;
            if (node)
                *node = _vlocals_nodes[locals - 1];
            return locals-1;
        }
        locals--;
    }
    varFlags = 0;
    if (node)
        *node = nullptr;
    return -1;
}

void SQFuncState::MarkLocalAsOuter(SQInteger pos)
{
    SQLocalVarInfo &lvi = _vlocals[pos];
    lvi._end_op = UINT32_MINUS_ONE;
    _outers++;
}


SQInteger SQFuncState::GetOuterVariable(const SQObject &name, char &varFlags, Expr **node)
{
    SQInteger outers = _outervalues.size();
    for(SQInteger i = 0; i<outers; i++) {
        if(_string(_outervalues[i]._name) == _string(name)) {
            varFlags = _outervalues[i]._varFlags;
            if (node)
                *node = _outervalues_nodes[i];
            return i;
        }
    }
    SQInteger pos=-1;
    if(_parent) {
        Expr *n = nullptr;
        pos = _parent->GetLocalVariable(name, varFlags, &n);
        if(pos == -1) {
            pos = _parent->GetOuterVariable(name, varFlags, &n);
            if(pos != -1) {
                _outervalues.push_back(SQOuterVar(SQObjectPtr(name),SQObjectPtr(SQInteger(pos)),otOUTER,varFlags)); //local
                _outervalues_nodes.push_back(n);
                if (node)
                    *node = n;
                return _outervalues.size() - 1;
            }
        }
        else {
            _parent->MarkLocalAsOuter(pos);
            _outervalues.push_back(SQOuterVar(SQObjectPtr(name),SQObjectPtr(SQInteger(pos)),otLOCAL,varFlags)); //local
            _outervalues_nodes.push_back(n);
            if (node)
                *node = n;
            return _outervalues.size() - 1;
        }
    }
    if (node)
        *node = nullptr;
    return -1;
}

void SQFuncState::AddParameter(const SQObject &name)
{
    PushLocalVariable(name, VF_PARAM | VF_ASSIGNABLE, nullptr);
    _parameters.push_back(SQObjectPtr(name));
}

void SQFuncState::AddLineInfos(SQInteger line, bool lineop, bool force)
{
    if(_lastline!=line || force){
        if(_lastline!=line) {
            SQLineInfo li;
            li._op = (GetCurrentPos()+1);
            li._line = line;
            li._is_line_op = lineop;
            _lineinfos.push_back(li);
            if (lineop && _ctx.getVm()->_compile_line_hook)
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

                if(pi._arg0 == i._arg1)
                {
                    pi._arg0 = i._arg0;
                    _optimization = false;
                    //_result_elimination = false;
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

SQObject SQFuncState::CreateString(const SQChar *s,SQInteger len)
{
    SQObjectPtr ns(SQString::Create(_sharedstate,s,len));
    _table(_ref_holder)->NewSlot(ns, SQObjectPtr((SQInteger)1));
    return ns;
}

SQObject SQFuncState::CreateTable()
{
    SQObjectPtr nt(SQTable::Create(_sharedstate,0));
    _table(_ref_holder)->NewSlot(nt, SQObjectPtr((SQInteger)1));
    return nt;
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
    SQFunctionProto *f=SQFunctionProto::Create(_ss,lang_features,_instructions.size(),
        _nliterals,_parameters.size(),_functions.size(),_outervalues.size(),
        _lineinfos.size(),_localvarinfos.size(),_defaultparams.size(),
        _staticmemos_count);

    SQObjectPtr refidx,key,val;
    SQInteger idx;

    f->_stacksize = _stacksize;
    f->_sourcename = _sourcename;
    f->_bgenerator = _bgenerator;
    f->_purefunction = _purefunction;
    f->_name = _name;
    f->_hoistingLevel = _hoistLevel;

    while((idx=_table(_literals)->Next(false,refidx,key,val))!=-1) {
        f->_literals[_integer(val)]=key;
        refidx=idx;
    }

    for(SQUnsignedInteger nf = 0; nf < _functions.size(); nf++) f->_functions[nf] = _functions[nf];
    for(SQUnsignedInteger np = 0; np < _parameters.size(); np++) f->_parameters[np] = _parameters[np];
    for(SQUnsignedInteger no = 0; no < _outervalues.size(); no++) f->_outervalues[no] = _outervalues[no];
    for(SQUnsignedInteger nl = 0; nl < _localvarinfos.size(); nl++) f->_localvarinfos[nl] = _localvarinfos[nl];
    for(SQUnsignedInteger ni = 0; ni < _lineinfos.size(); ni++) f->_lineinfos[ni] = _lineinfos[ni];
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
