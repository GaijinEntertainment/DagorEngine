/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "sqarray.h"
#include "sqtable.h"
#include "squserdata.h"
#include "sqfuncproto.h"
#include "sqclass.h"
#include "sqclosure.h"


const char *IdType2Name(SQObjectType type)
{
    switch(_RAW_TYPE(type))
    {
    case OT_NULL: // fallthrough
    case _RT_NULL:return "null";
    case _RT_INTEGER:return "integer";
    case _RT_FLOAT:return "float";
    case _RT_BOOL:return "bool";
    case _RT_STRING:return "string";
    case _RT_TABLE:return "table";
    case _RT_ARRAY:return "array";
    case _RT_GENERATOR:return "generator";
    case _RT_CLOSURE:
    case _RT_NATIVECLOSURE:
        return "function";
    case _RT_USERDATA:
    case _RT_USERPOINTER:
        return "userdata";
    case _RT_THREAD: return "thread";
    case _RT_FUNCPROTO: return "function";
    case _RT_CLASS: return "class";
    case _RT_INSTANCE: return "instance";
    case _RT_WEAKREF: return "weakref";
    case _RT_OUTER: return "outer";
    default:
        return NULL;
    }
}

const char *GetTypeName(const SQObject &obj1)
{
    return IdType2Name(sq_type(obj1));
}

SQObjectPtr::SQObjectPtr(SQVM *vm, const char *str, SQInteger len)
    : SQObjectPtr(SQString::Create(vm->_sharedstate, str, len))
{
}

SQString *SQString::Create(SQSharedState *ss,const char *s,SQInteger len)
{
    SQString *str=ADD_STRING(ss,s,len);
    return str;
}

void SQString::Release()
{
    REMOVE_STRING(_sharedstate,this);
}

SQInteger SQString::Next(const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval)
{
    SQInteger idx = (SQInteger)TranslateIndex(refpos);
    if (idx < _len) {
        outkey = (SQInteger)idx;
        outval = (SQInteger)((SQUnsignedInteger)_val[idx]);
        //return idx for the next iteration
        return ++idx;
    }
    //nothing to iterate anymore
    return -1;
}

SQWeakRef *SQRefCounted::GetWeakRef(SQAllocContext alloc_ctx, SQObjectType type, SQObjectFlags flags)
{
    if(!_weakref) {
        sq_new(alloc_ctx, _weakref, SQWeakRef);
        _weakref->_alloc_ctx = alloc_ctx;
        _weakref->_obj._unVal.raw = 0;
        _weakref->_obj._type = type;
        _weakref->_obj._flags = flags;
        _weakref->_obj._unVal.pRefCounted = this;
    }
    return _weakref;
}

SQRefCounted::~SQRefCounted()
{
    if(_weakref) {
        _weakref->_obj._type = OT_NULL;
        _weakref->_obj._unVal.raw = 0;
    }
}

void SQWeakRef::Release() {
    if(ISREFCOUNTED(_obj._type)) {
        _obj._unVal.pRefCounted->_weakref = NULL;
    }
    sq_delete(_alloc_ctx, this, SQWeakRef);
}

bool SQDelegable::GetMetaMethod(SQVM *v,SQMetaMethod mm,SQObjectPtr &res) {
    if(_delegate) {
        return _delegate->Get((*_ss(v)->_metamethodnames)[mm],res);
    }
    return false;
}

bool SQDelegable::SetDelegate(SQTable *mt)
{
    SQTable *temp = mt;
    if(temp == this) return false;
    while (temp) {
        if (temp->_delegate == this) return false; //cycle detected
        temp = temp->_delegate;
    }
    if (mt) __ObjAddRef(mt);
    __ObjRelease(_delegate);
    _delegate = mt;
    return true;
}

bool SQGenerator::Yield(SQVM *v,SQInteger arg1,SQInteger target)
{
    if(_state==eSuspended) { v->Raise_Error("internal vm error, yielding suspended generator");  return false;}
    if(_state==eDead) { v->Raise_Error("internal vm error, yielding a dead generator"); return false; }
    SQInteger size = v->_top-v->_stackbase;

    _stack.resize(size);
    SQObject _this = v->_stack[v->_stackbase];
    _stack._vals[0] = ISREFCOUNTED(sq_type(_this))
        ? SQObjectPtr(_refcounted(_this)->GetWeakRef(_ss(v)->_alloc_ctx, sq_type(_this), _this._flags))
        : _this;

    for(SQInteger n =1; n<target; n++) {
        _stack._vals[n] = v->_stack[v->_stackbase+n];
    }
    for(SQInteger j =0; j < size; j++)
    {
        v->_stack[v->_stackbase+j].Null();
    }

    _ci = *v->ci;
    _ci._generator=NULL;
    _yield_arg1 = arg1;
    for(SQInteger i=0;i<_ci._etraps;i++) {
        _etraps.push_back(v->_etraps.top());
        v->_etraps.pop_back();
        // store relative stack base and size in case of resume to other _top
        SQExceptionTrap &et = _etraps.back();
        et._stackbase -= v->_stackbase;
        et._stacksize -= v->_stackbase;
    }
    _state=eSuspended;
    return true;
}

bool SQGenerator::Resume(SQVM *v,SQObjectPtr &dest)
{
    if(_state==eDead){ v->Raise_Error("resuming dead generator"); return false; }
    if(_state==eRunning){ v->Raise_Error("resuming active generator"); return false; }
    SQInteger size = _stack.size();
    SQInteger target = &dest - &(v->_stack._vals[v->_stackbase]);
    assert(target>=0 && target<=255);
    SQInteger newbase = v->_top;
    if(!v->EnterFrame(v->_top, v->_top + size, false))
        return false;
    v->ci->_generator   = this;
    v->ci->_target      = (SQInt32)target;
    v->ci->_closure     = _ci._closure;
    v->ci->_ip          = _ci._ip;
    v->ci->_literals    = _ci._literals;
    v->ci->_ncalls      = _ci._ncalls;
    v->ci->_etraps      = _ci._etraps;
    v->ci->_root        = _ci._root;


    for(SQInteger i=0;i<_ci._etraps;i++) {
        v->_etraps.push_back(_etraps.top());
        _etraps.pop_back();
        SQExceptionTrap &et = v->_etraps.back();
        // restore absolute stack base and size
        et._stackbase += newbase;
        et._stacksize += newbase;
    }
    SQObject _this = _stack._vals[0];
    v->_stack[v->_stackbase] = sq_type(_this) == OT_WEAKREF ? _weakref(_this)->_obj : _this;

    // Saved-stack copy. The async runner mutates `_stack[_yield_arg1]` directly
    // for await-style send delivery; the value lands in the VM stack here.
    for(SQInteger n = 1; n<size; n++) {
        v->_stack[v->_stackbase+n] = _stack._vals[n];
        _stack._vals[n].Null();
    }

    _state=eRunning;
    if (v->_debughook)
        v->CallDebugHook('c');

    return true;
}

bool SQGenerator::RunStep(SQVM *v,SQObjectPtr &out,ResumeMode mode,const SQObjectPtr &payload)
{
    SQVM::ExecutionType et = SQVM::ET_RESUME_GENERATOR;
    if (mode == ResumeThrow) {
        v->_lasterror = payload;
        et = SQVM::ET_RESUME_GENERATOR_THROW;
    }
    else if (_yield_arg1 != MAX_FUNC_STACKSIZE) {
        // Still MAX_FUNC_STACKSIZE on the first run: nothing to deliver yet.
        _stack[_yield_arg1] = payload;
    }

    // sq_resume's calling convention: generator + result placeholder on the stack.
    v->Push(SQObjectPtr(this));
    v->PushNull();
    bool execOk = v->_debughook
        ? v->Execute<true>(v->GetUp(-2), 0, v->_top, v->GetUp(-1), SQFalse, et)
        : v->Execute<false>(v->GetUp(-2), 0, v->_top, v->GetUp(-1), SQFalse, et);
    if (execOk)
        out = v->GetUp(-1);
    v->Pop(2);
    return execOk;
}

void SQArray::Extend(const SQArray *a){
    SQInteger xlen;
    if((xlen=a->Size()))
        for(SQInteger i=0;i<xlen;i++)
            Append(a->_values[i]);
}

bool SQArray::IsBinaryEqual(const SQArray *o)
{
    if (this == o)
        return true;
    if (_values.size() != o->_values.size())
        return false;
    if (_values.size() == 0)
        return true;

    return memcmp(_values._vals, o->_values._vals, _values.size() * sizeof(SQObjectPtr)) == 0;
}

const char* SQFunctionProto::GetLocal(SQVM *vm,SQUnsignedInteger stackbase,SQUnsignedInteger nseq,SQUnsignedInteger nop)
{
    SQUnsignedInteger nvars=_nlocalvarinfos;
    const char *res=NULL;
    if(nvars>=nseq){
        for(SQUnsignedInteger i=0;i<nvars;i++){
            if(_localvarinfos[i]._start_op<=nop && _localvarinfos[i]._end_op>=nop)
            {
                if(nseq==0){
                    vm->Push(vm->_stack[stackbase+_localvarinfos[i]._pos]);
                    res=_stringval(_localvarinfos[i]._name);
                    break;
                }
                nseq--;
            }
        }
    }
    return res;
}


template <typename T>
inline SQInteger get_line_offset_impl(T* lineinfos, int nlineinfos, int instruction_index, int* hint, bool* is_dbg_step_point)
{
    int pos = nlineinfos - 1;
    int low = 0;
    int high = nlineinfos - 1;
    int tryCount = 20;

    if (hint && unsigned(*hint) < unsigned(high)) {
        int h = *hint;
        if (instruction_index >= (int)lineinfos[h]._op && instruction_index < (int)lineinfos[h + 1]._op) {
            if (is_dbg_step_point)
                *is_dbg_step_point = lineinfos[h]._is_dbg_step_point;
            return lineinfos[h]._line_offset;
        }
        else if (instruction_index >= (int)lineinfos[h + 1]._op && instruction_index < (int)lineinfos[h + 2]._op) {
            h++;
            *hint = h;
            if (is_dbg_step_point)
                *is_dbg_step_point = lineinfos[h]._is_dbg_step_point;
            return lineinfos[h]._line_offset;
        }
        else if (instruction_index == 0) {
            for (int i = 0; i < nlineinfos - 1; i++)
                if (instruction_index >= (int)lineinfos[i]._op && instruction_index < (int)lineinfos[i + 1]._op) {
                    *hint = i;
                    if (is_dbg_step_point)
                        *is_dbg_step_point = lineinfos[i]._is_dbg_step_point;
                    return lineinfos[i]._line_offset;
                }
        }
    }

    while (high >= low && --tryCount) {
        int mid = (high + low) / 2;

        if (instruction_index < (int)lineinfos[mid]._op)
            high = mid - 1;
        else if (instruction_index >= (int)lineinfos[mid + 1]._op)
            low = mid + 1;
        else {
            pos = mid;
            break;
        }
    }

    if (tryCount == 0) {
        // TODO: failsafe pass, to be reomved later
        assert(0);
        for (int i = 0; i < nlineinfos - 1; i++)
            if (instruction_index >= (int)lineinfos[i]._op && instruction_index < (int)lineinfos[i + 1]._op) {
                pos = i;
                break;
            }
    }

    if (is_dbg_step_point)
        *is_dbg_step_point = lineinfos[pos]._is_dbg_step_point;

    if (hint)
        *hint = pos;

    return lineinfos[pos]._line_offset;
}


SQInteger SQFunctionProto::GetLine(SQLineInfosHeader* lineinfos, int nlineinfos, int instruction_index, int* hint, bool* is_dbg_step_point)
{
    if (lineinfos->_is_compressed)
        return get_line_offset_impl((SQCompressedLineInfo *)(void *)(lineinfos + 1), nlineinfos, instruction_index, hint, is_dbg_step_point) + lineinfos->_first_line;
    else
        return get_line_offset_impl((SQFullLineInfo *)(void *)(lineinfos + 1), nlineinfos, instruction_index, hint, is_dbg_step_point) + lineinfos->_first_line;
}


SQInteger SQFunctionProto::GetLine(const SQInstruction *curr, int *hint, bool *is_dbg_step_point)
{
    return GetLine(_lineinfos, _nlineinfos, int(curr - _instructions), hint, is_dbg_step_point);
}


SQClosure::~SQClosure()
{
    __ObjRelease(_env);
    __ObjRelease(_base);
    REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
}

SQFunctionProto::SQFunctionProto(SQSharedState *ss)
{
    _stacksize=0;
    _bgenerator=false;
    _purefunction=false;
    _nodiscard=false;
    _isAsync=false;
    _inside_hoisted_scope=false;
    INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);
}

SQFunctionProto::~SQFunctionProto()
{
    REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
}

#ifndef NO_GARBAGE_COLLECTOR

#define START_MARK()    if(!(_uiRef&MARK_FLAG)){ \
        _uiRef|=MARK_FLAG;

#define END_MARK() RemoveFromChain(&_sharedstate->_gc_chain, this); \
        AddToChain(chain, this); }

void SQVM::Mark(SQCollectable **chain)
{
    START_MARK()
        SQSharedState::MarkObject(_lasterror,chain);
        SQSharedState::MarkObject(_errorhandler,chain);
        SQSharedState::MarkObject(_debughook_closure,chain);
        SQSharedState::MarkObject(_roottable, chain);
        SQSharedState::MarkObject(temp_reg, chain);
        for(SQUnsignedInteger i = 0; i < _stack.size(); i++) SQSharedState::MarkObject(_stack[i], chain);
        for(SQInteger k = 0; k < _callsstacksize; k++) SQSharedState::MarkObject(_callsstack[k]._closure, chain);
    END_MARK()
}

void SQArray::Mark(SQCollectable **chain)
{
    START_MARK()
        SQInteger len = _values.size();
        for(SQInteger i = 0;i < len; i++) SQSharedState::MarkObject(_values[i], chain);
    END_MARK()
}
void SQTable::Mark(SQCollectable **chain)
{
    START_MARK()
        if(_delegate) _delegate->Mark(chain);
        SQInteger len = _numofnodes_minus_one;
        for(SQInteger i = 0; i <= len; i++){
            SQSharedState::MarkObject(_nodes[i].key, chain);
            SQSharedState::MarkObject(_nodes[i].val, chain);
        }
    END_MARK()
}

void SQClass::Mark(SQCollectable **chain)
{
    START_MARK()
        _members->Mark(chain);
        if(_base) _base->Mark(chain);
        for(SQUnsignedInteger i =0; i< _defaultvalues.size(); i++) {
            SQSharedState::MarkObject(_defaultvalues[i].val, chain);
        }
        for(SQUnsignedInteger j =0; j< _methods.size(); j++) {
            SQSharedState::MarkObject(_methods[j].val, chain);
        }
        for(SQUnsignedInteger k =0; k< MT_NUM_METHODS; k++) {
            SQSharedState::MarkObject(_metamethods[k], chain);
        }
    END_MARK()
}

void SQInstance::Mark(SQCollectable **chain)
{
    START_MARK()
        _class->Mark(chain);
        SQUnsignedInteger nvalues = _class->_defaultvalues.size();
        for(SQUnsignedInteger i =0; i< nvalues; i++) {
            SQSharedState::MarkObject(_values[i], chain);
        }
    END_MARK()
}

void SQGenerator::Mark(SQCollectable **chain)
{
    START_MARK()
        for(SQUnsignedInteger i = 0; i < _stack.size(); i++) SQSharedState::MarkObject(_stack[i], chain);
        SQSharedState::MarkObject(_closure, chain);
    END_MARK()
}

void SQFunctionProto::Mark(SQCollectable **chain)
{
    START_MARK()
        for(SQInteger i = 0; i < _nliterals; i++) SQSharedState::MarkObject(_literals[i], chain);
        for(SQInteger k = 0; k < _nfunctions; k++) SQSharedState::MarkObject(_functions[k], chain);
    END_MARK()
}

void SQClosure::Mark(SQCollectable **chain)
{
    START_MARK()
        if(_base) _base->Mark(chain);
        SQFunctionProto *fp = _function;
        fp->Mark(chain);
        for(SQInteger i = 0; i < fp->_noutervalues; i++) SQSharedState::MarkObject(_outervalues[i], chain);
        for(SQInteger k = 0; k < fp->_ndefaultparams; k++) SQSharedState::MarkObject(_defaultparams[k], chain);
        for(SQInteger j = 0; j < fp->_nstaticmemos; j++) SQSharedState::MarkObject(fp->_staticmemos[j], chain);
    END_MARK()
}

void SQNativeClosure::Mark(SQCollectable **chain)
{
    START_MARK()
        for(SQUnsignedInteger i = 0; i < _noutervalues; i++) SQSharedState::MarkObject(_outervalues[i], chain);
    END_MARK()
}

void SQOuter::Mark(SQCollectable **chain)
{
    START_MARK()
    /* If the valptr points to a closed value, that value is alive */
    if(_valptr == &_value) {
      SQSharedState::MarkObject(_value, chain);
    }
    END_MARK()
}

void SQUserData::Mark(SQCollectable **chain){
    START_MARK()
        if(_delegate) _delegate->Mark(chain);
    END_MARK()
}

void SQCollectable::UnMark() { _uiRef&=~MARK_FLAG; }

#endif

