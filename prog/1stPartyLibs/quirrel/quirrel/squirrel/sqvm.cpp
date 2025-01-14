/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include <math.h>
#include <stdlib.h>
#include "sqopcodes.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"
#include "sqtable.h"
#include "squserdata.h"
#include "sqarray.h"
#include "sqclass.h"
#include "vartrace.h"

//#define TOP() (_stack._vals[_top-1])
#define TARGET _stack._vals[_stackbase+arg0]
#define STK(a) _stack._vals[_stackbase+(a)]

#define SLOT_RESOLVE_STATUS_OK         0
#define SLOT_RESOLVE_STATUS_NO_MATCH   1
#define SLOT_RESOLVE_STATUS_ERROR      2

static inline void propagate_immutable(const SQObject &obj, SQObject &slot_val)
{
    if (sq_objflags(obj) & SQOBJ_FLAG_IMMUTABLE)
        slot_val._flags |= SQOBJ_FLAG_IMMUTABLE;
}


bool SQVM::BW_OP(SQUnsignedInteger op,SQObjectPtr &trg,const SQObjectPtr &o1,const SQObjectPtr &o2)
{
    SQInteger res;
    if((sq_type(o1)|sq_type(o2)) == OT_INTEGER)
    {
        SQInteger i1 = _integer(o1), i2 = _integer(o2);
        switch(op) {
            case BW_AND:    res = i1 & i2; break;
            case BW_OR:     res = i1 | i2; break;
            case BW_XOR:    res = i1 ^ i2; break;
            case BW_SHIFTL: res = i1 << i2; break;
            case BW_SHIFTR: res = i1 >> i2; break;
            case BW_USHIFTR:res = (SQInteger)(*((SQUnsignedInteger*)&i1) >> i2); break;
            default: { Raise_Error(_SC("internal vm error bitwise op failed")); return false; }
        }
    }
    else { Raise_Error(_SC("bitwise op between '%s' and '%s'"),GetTypeName(o1),GetTypeName(o2)); return false;}
    trg = res;
    return true;
}

#define _ARITH_(op,trg,o1,o2) \
{ \
    SQInteger tmask = sq_type(o1)|sq_type(o2); \
    switch(tmask) { \
        case OT_INTEGER: trg = _integer(o1) op _integer(o2);break; \
        case (OT_FLOAT|OT_INTEGER): \
        case (OT_FLOAT): trg = tofloat(o1) op tofloat(o2); break; \
        default: _GUARD(ARITH_OP((#op)[0],trg,o1,o2)); break;\
    } \
}

#define _ARITH_NOZERO(op,trg,o1,o2) \
{ \
    SQInteger tmask = sq_type(o1)|sq_type(o2); \
    switch(tmask) { \
        case OT_INTEGER: { SQInteger i2 = _integer(o2); \
            if (i2 == 0) { Raise_Error(_SC("division by zero")); SQ_THROW(); } \
            else if (i2 == -1 && _integer(o1) == MIN_SQ_INTEGER) { Raise_Error(_SC("integer overflow")); SQ_THROW(); } \
            trg = _integer(o1) op i2; } break; \
        case (OT_FLOAT|OT_INTEGER): \
        case (OT_FLOAT): { SQFloat f2 = tofloat(o2); if(f2 == (SQFloat)0.f) { Raise_Error("float division by zero"); SQ_THROW(); } trg = tofloat(o1) op f2; } break;\
        default: _GUARD(ARITH_OP((#op)[0],trg,o1,o2)); break;\
    } \
}


bool SQVM::ARITH_OP(SQUnsignedInteger op,SQObjectPtr &trg,const SQObjectPtr &o1,const SQObjectPtr &o2)
{
    SQInteger tmask = sq_type(o1)|sq_type(o2);
    switch(tmask) {
        case OT_INTEGER:{
            SQInteger res, i1 = _integer(o1), i2 = _integer(o2);
            switch(op) {
            case '+': res = i1 + i2; break;
            case '-': res = i1 - i2; break;
            case '/': if (i2 == 0) { Raise_Error(_SC("integer division by zero")); return false; }
                    else if (i2 == -1 && i1 == MIN_SQ_INTEGER) { Raise_Error(_SC("integer overflow")); return false; }
                    res = i1 / i2;
                    break;
            case '*': res = i1 * i2; break;
            case '%': if (i2 == 0) { Raise_Error(_SC("integer modulo by zero")); return false; }
                    else if (i2 == -1) { res = 0; break; }
                    res = i1 % i2;
                    break;
            default: res = 0xDEADBEEF;
            }
            trg = res; }
            break;
        case (OT_FLOAT|OT_INTEGER):
        case (OT_FLOAT):{
            SQFloat res, f1 = tofloat(o1), f2 = tofloat(o2);
            switch(op) {
            case '+': res = f1 + f2; break;
            case '-': res = f1 - f2; break;
            case '/':
              if (f2 == 0.f) { Raise_Error(_SC("float division by zero")); return false; }
              res = f1 / f2;
              break;
            case '*': res = f1 * f2; break;
            case '%':
              if (f2 == 0.f) { Raise_Error(_SC("float modulo by zero")); return false; }
              res = SQFloat(fmod((double)f1,(double)f2));
              break;
            default: res = 0x0f;
            }
            trg = res; }
            break;
        default:
            if(op == '+' && (tmask & _RT_STRING)){
                if(!StringCat(o1, o2, trg)) return false;
            }
            else if(!ArithMetaMethod(op,o1,o2,trg)) {
                return false;
            }
    }
    return true;
}

SQVM::SQVM(SQSharedState *ss) :
    _callstackdata(nullptr),
    _stack(ss->_alloc_ctx),
    _etraps(ss->_alloc_ctx)
{
    _sharedstate=ss;
    _suspended = SQFalse;
    _suspended_target = -1;
    _suspended_root = SQFalse;
    _suspended_traps = -1;
    _foreignptr = NULL;
    _nnativecalls = 0;
    _nmetamethodscall = 0;
    _lasterror.Null();
    _errorhandler.Null();
    _debughook = false;
    _debughook_native = NULL;
    _debughook_closure.Null();
    _compile_line_hook = NULL;
    _current_thread = -1;
    _get_current_thread_id_func = NULL;
    _sq_call_hook = NULL;
    _openouters = NULL;
    ci = NULL;
    _releasehook = NULL;
    INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);
}

void SQVM::Finalize()
{
    if (_debughook)
        CallDebugHook(_SC('x'));

    if(_releasehook) { _releasehook(_foreignptr,0); _releasehook = NULL; }
    if(_openouters) CloseOuters(&_stack._vals[0]);
    _roottable.Null();
    _lasterror.Null();
    _errorhandler.Null();
    _debughook = false;
    _debughook_native = NULL;
    _compile_line_hook = NULL;
    _debughook_closure.Null();
    _get_current_thread_id_func = NULL;
    _sq_call_hook = NULL;
    temp_reg.Null();
    _callstackdata.resize(0);
    SQInteger size=_stack.size();
    for(SQInteger i=0;i<size;i++)
        _stack[i].Null();
}

SQVM::~SQVM()
{
    Finalize();
    REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
}

bool SQVM::ArithMetaMethod(SQInteger op,const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &dest)
{
    SQMetaMethod mm;
    switch(op){
        case _SC('+'): mm=MT_ADD; break;
        case _SC('-'): mm=MT_SUB; break;
        case _SC('/'): mm=MT_DIV; break;
        case _SC('*'): mm=MT_MUL; break;
        case _SC('%'): mm=MT_MODULO; break;
        default: mm = MT_ADD; assert(0); break; //shutup compiler
    }
    if(is_delegable(o1) && _delegable(o1)->_delegate) {

        SQObjectPtr closure;
        if(_delegable(o1)->GetMetaMethod(this, mm, closure)) {
            Push(o1);Push(o2);
            return CallMetaMethod(closure,mm,2,dest);
        }
    }
    Raise_Error(_SC("arith op %c on between '%s' and '%s'"),(char)op,GetTypeName(o1),GetTypeName(o2));
    return false;
}

bool SQVM::NEG_OP(SQObjectPtr &trg,const SQObjectPtr &o)
{

    switch(sq_type(o)) {
    case OT_INTEGER:
        trg = -_integer(o);
        return true;
    case OT_FLOAT:
        trg = -_float(o);
        return true;
    case OT_TABLE:
    case OT_USERDATA:
    case OT_INSTANCE:
        if(_delegable(o)->_delegate) {
            SQObjectPtr closure;
            if(_delegable(o)->GetMetaMethod(this, MT_UNM, closure)) {
                Push(o);
                if(!CallMetaMethod(closure, MT_UNM, 1, temp_reg)) return false;
                _Swap(trg,temp_reg);
                return true;

            }
        }
    default:break; //shutup compiler
    }
    Raise_Error(_SC("attempt to negate a %s"), GetTypeName(o));
    return false;
}

#define _RET_SUCCEED(exp) { result = (exp); return true; }
bool SQVM::ObjCmp(const SQObjectPtr &o1,const SQObjectPtr &o2,SQInteger &result)
{
    SQObjectType t1 = sq_type(o1), t2 = sq_type(o2);
    if(t1 == t2) {
        if(_rawval(o1) == _rawval(o2))_RET_SUCCEED(0);
        SQObjectPtr res;
        switch(t1){
        case OT_STRING:
            _RET_SUCCEED(strcmp(_stringval(o1),_stringval(o2)));
        case OT_INTEGER:
        case OT_BOOL:
            _RET_SUCCEED((_integer(o1)<_integer(o2))?-1:1);
        case OT_FLOAT:
            _RET_SUCCEED((_float(o1)<_float(o2))?-1:1);
        case OT_TABLE:
        case OT_USERDATA:
        case OT_INSTANCE:
            if(_delegable(o1)->_delegate) {
                SQObjectPtr closure;
                if(_delegable(o1)->GetMetaMethod(this, MT_CMP, closure)) {
                    Push(o1);Push(o2);
                    if(CallMetaMethod(closure,MT_CMP,2,res)) {
                        if(sq_type(res) != OT_INTEGER) {
                            Raise_Error(_SC("_cmp must return an integer"));
                            return false;
                        }
                        _RET_SUCCEED(_integer(res))
                    }
                    return false;
                }
            }
            //continues through (no break needed)
        default:
            Raise_CompareError(o1, o2);
            return false;
        }
        assert(0);
        //if(sq_type(res)!=OT_INTEGER) { Raise_CompareError(o1,o2); return false; }
        //  _RET_SUCCEED(_integer(res));

    }
    else{
        if(sq_isnumeric(o1) && sq_isnumeric(o2)){
            if((t1==OT_INTEGER) && (t2==OT_FLOAT)) {
                if( _integer(o1)==_float(o2) ) { _RET_SUCCEED(0); }
                else if( _integer(o1)<_float(o2) ) { _RET_SUCCEED(-1); }
                _RET_SUCCEED(1);
            }
            else{
                if( _float(o1)==_integer(o2) ) { _RET_SUCCEED(0); }
                else if( _float(o1)<_integer(o2) ) { _RET_SUCCEED(-1); }
                _RET_SUCCEED(1);
            }
        }
        else if(t1==OT_NULL) {_RET_SUCCEED(-1);}
        else if(t2==OT_NULL) {_RET_SUCCEED(1);}
        else { Raise_CompareError(o1,o2); return false; }

    }
    assert(0);
    _RET_SUCCEED(0); //cannot happen
}

bool SQVM::ObjCmpI(const SQObjectPtr &o1,const SQInteger o2,SQInteger &result)
{
    SQObjectType t1 = sq_type(o1);
    if(t1 == OT_INTEGER) {
        SQInteger i1 = _integer(o1);
        result = (i1 == o2) ? 0 : (i1 < o2 ? -1 : 1);
        return true;
    }
    else if (t1 == OT_FLOAT){
        SQFloat f1 = _float(o1);
        result = (f1 == o2) ? 0 : (f1 < o2 ? -1 : 1);
        return true;
    } else if(t1==OT_NULL) {_RET_SUCCEED(-1);}
    else Raise_CompareError(o1, SQObjectPtr(o2));
    return false;
}

bool SQVM::ObjCmpF(const SQObjectPtr &o1,const SQFloat o2,SQInteger &result)
{
    SQObjectType t1 = sq_type(o1);
    if(t1 == OT_FLOAT) {
        float f1 = _float(o1);
        result = (f1 == o2) ? 0 : (f1 < o2 ? -1 : 1);
        return true;
    }
    else if (t1 == OT_INTEGER){
        SQInteger i1 = _integer(o1);
        result = (i1 == o2) ? 0 : (i1 < o2 ? -1 : 1);
        return true;
    } else if(t1==OT_NULL) {_RET_SUCCEED(-1);}
    else Raise_CompareError(o1, SQObjectPtr(o2));
    return false;
}

bool SQVM::CMP_OP_RESI(CmpOP op, const SQObjectPtr &o1,const SQInteger o2, int &res)
{
    SQInteger r;
    if(ObjCmpI(o1,o2,r)) {
        switch(op) {
            case CMP_G: res = (r > 0); return true;
            case CMP_GE: res = (r >= 0); return true;
            case CMP_L: res = (r < 0); return true;
            case CMP_LE: res = (r <= 0); return true;
            case CMP_3W: res = r; return true;
        }
        assert(0);
    }
    return false;
}

bool SQVM::CMP_OP_RESF(CmpOP op, const SQObjectPtr &o1,const SQFloat o2, int &res)
{
    SQInteger r;
    if(ObjCmpF(o1,o2,r)) {
        switch(op) {
            case CMP_G: res = (r > 0); return true;
            case CMP_GE: res = (r >= 0); return true;
            case CMP_L: res = (r < 0); return true;
            case CMP_LE: res = (r <= 0); return true;
            case CMP_3W: res = r; return true;
        }
        assert(0);
    }
    return false;
}


bool SQVM::CMP_OP_RES(CmpOP op, const SQObjectPtr &o1,const SQObjectPtr &o2, int &res)
{
    SQInteger r;
    if(ObjCmp(o1,o2,r)) {
        switch(op) {
            case CMP_G: res = (r > 0); return true;
            case CMP_GE: res = (r >= 0); return true;
            case CMP_L: res = (r < 0); return true;
            case CMP_LE: res = (r <= 0); return true;
            case CMP_3W: res = r; return true;
        }
        assert(0);
    }
    return false;
}

bool SQVM::CMP_OP(CmpOP op, const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &res)
{
    SQInteger r;
    if(ObjCmp(o1,o2,r)) {
        switch(op) {
            case CMP_G: res = (r > 0); return true;
            case CMP_GE: res = (r >= 0); return true;
            case CMP_L: res = (r < 0); return true;
            case CMP_LE: res = (r <= 0); return true;
            case CMP_3W: res = r; return true;
        }
        assert(0);
    }
    return false;
}

bool SQVM::ToString(const SQObjectPtr &o,SQObjectPtr &res)
{
    switch(sq_type(o)) {
    case OT_STRING:
        res = o;
        return true;
    case OT_FLOAT: {
            char * p = _sp(NUMBER_MAX_CHAR+1);
            int i = scsprintf(p, NUMBER_MAX_CHAR, _SC("%g"), _float(o));
            for (; i >= 0; i--)
                if (p[i] == ',')
                    p[i] = '.';
        }
        break;
    case OT_INTEGER:
        scsprintf(_sp(NUMBER_MAX_CHAR+1),NUMBER_MAX_CHAR,_PRINT_INT_FMT,_integer(o));
        break;
    case OT_BOOL:
        scsprintf(_sp(6),6,_integer(o)?_SC("true"):_SC("false"));
        break;
    case OT_NULL:
        scsprintf(_sp(5),5,_SC("null"));
        break;
    case OT_TABLE:
    case OT_USERDATA:
    case OT_INSTANCE:
        if(_delegable(o)->_delegate) {
            SQObjectPtr closure;
            if(_delegable(o)->GetMetaMethod(this, MT_TOSTRING, closure)) {
                Push(o);
                if(CallMetaMethod(closure,MT_TOSTRING,1,res)) {
                    if(sq_type(res) == OT_STRING)
                        return true;
                }
                else {
                    return false;
                }
            }
        }
    default:
        scsprintf(_sp((sizeof(void*)*2)+NUMBER_MAX_CHAR),(sizeof(void*)*2)+NUMBER_MAX_CHAR,
            _SC("(%s : 0x%p)"),GetTypeName(o),(void*)_rawval(o));
    }
    res = SQString::Create(_ss(this),_spval);
    return true;
}


bool SQVM::StringCat(const SQObjectPtr &str,const SQObjectPtr &obj,SQObjectPtr &dest)
{
    SQObjectPtr a, b;
    if(!ToString(str, a)) return false;
    if(!ToString(obj, b)) return false;
    SQInteger l = _string(a)->_len , ol = _string(b)->_len;
    SQChar *s = _sp(l + ol + 1);
    memcpy(s, _stringval(a), l);
    memcpy(s + l, _stringval(b), ol);
    dest = SQString::Create(_ss(this), _spval, l + ol);
    return true;
}

bool SQVM::TypeOf(const SQObjectPtr &obj1,SQObjectPtr &dest)
{
    if(is_delegable(obj1) && _delegable(obj1)->_delegate) {
        SQObjectPtr closure;
        if(_delegable(obj1)->GetMetaMethod(this, MT_TYPEOF, closure)) {
            Push(obj1);
            return CallMetaMethod(closure,MT_TYPEOF,1,dest);
        }
    }
    dest = SQString::Create(_ss(this),GetTypeName(obj1));
    return true;
}

bool SQVM::Init(SQVM *friendvm, SQInteger stacksize)
{
    _stack.resize(stacksize + STACK_GROW_THRESHOLD);
    _alloccallsstacksize = 4;
    _callstackdata.resize(_alloccallsstacksize);
    _callsstacksize = 0;
    _callsstack = &_callstackdata[0];
    _stackbase = 0;
    _top = 0;
    if(!friendvm) {
        _roottable = SQTable::Create(_ss(this), 0);
        sq_base_register(this);
    }
    else {
        _roottable = friendvm->_roottable;
        _errorhandler = friendvm->_errorhandler;
        _debughook = friendvm->_debughook;
        _debughook_native = friendvm->_debughook_native;
        _debughook_closure = friendvm->_debughook_closure;
        _compile_line_hook = friendvm->_compile_line_hook;
        _current_thread = friendvm->_current_thread;
        _get_current_thread_id_func = friendvm->_get_current_thread_id_func;
        _sq_call_hook = friendvm->_sq_call_hook;
    }


    return true;
}


template <bool debughookPresent>
bool SQVM::StartCall(SQClosure *closure,SQInteger target,SQInteger args,SQInteger stackbase,bool tailcall)
{
    SQFunctionProto *func = closure->_function;

    SQInteger paramssize = func->_nparameters;
    const SQInteger newtop = stackbase + func->_stacksize;
    SQInteger nargs = args;
    if(func->_varparams)
    {
        paramssize--;
        if (nargs < paramssize) {
            Raise_Error(_SC("wrong number of parameters passed to '%s' %s:%d (%d passed, at least %d required)"),
              sq_type(func->_name) == OT_STRING ? _stringval(func->_name) : _SC("unknown"),
              sq_type(func->_sourcename) == OT_STRING ? _stringval(func->_sourcename) : _SC("unknown"),
              int(func->_nlineinfos > 0 ? func->_lineinfos[0]._line : 0),
              (int)nargs, (int)paramssize);
            return false;
        }

        //dumpstack(stackbase);
        SQInteger nvargs = nargs - paramssize;
        SQArray *arr = SQArray::Create(_ss(this),nvargs);
        SQInteger pbase = stackbase+paramssize;
        for(SQInteger n = 0; n < nvargs; n++) {
            arr->_values[n] = _stack._vals[pbase];
            _stack._vals[pbase].Null();
            pbase++;

        }
        _stack._vals[stackbase+paramssize] = arr;
        //dumpstack(stackbase);
    }
    else if (paramssize != nargs) {
        SQInteger ndef = func->_ndefaultparams;
        SQInteger diff;
        if(ndef && nargs < paramssize && (diff = paramssize - nargs) <= ndef) {
            for(SQInteger n = ndef - diff; n < ndef; n++) {
                _stack._vals[stackbase + (nargs++)] = closure->_defaultparams[n];
            }
        }
        else {
            Raise_Error(_SC("wrong number of parameters passed to '%s' %s:%d (%d passed, %d required)"),
              sq_type(func->_name) == OT_STRING ? _stringval(func->_name) : _SC("unknown"),
              sq_type(func->_sourcename) == OT_STRING ? _stringval(func->_sourcename) : _SC("unknown"),
              int(func->_nlineinfos > 0 ? func->_lineinfos[0]._line : 0),
              (int)nargs, (int)paramssize);
            return false;
        }
    }

    if(closure->_env) {
        _stack._vals[stackbase] = closure->_env->_obj;
    }

    if(!EnterFrame(stackbase, newtop, tailcall)) return false;

    ci->_closure  = closure;
    ci->_literals = func->_literals;
    ci->_ip       = func->_instructions;
    ci->_target   = (SQInt32)target;
    if constexpr (debughookPresent)
    {
        if (_debughook) {
            CallDebugHook(_SC('c'));
        }
    }

    if (closure->_function->_bgenerator) {
        SQFunctionProto *f = closure->_function;
        SQGenerator *gen = SQGenerator::Create(_ss(this), closure);
        if(!gen->Yield(this,f->_stacksize))
            return false;
        SQObjectPtr temp;
        Return<debughookPresent>(1, target, temp);
        STK(target) = gen;
    }


    return true;
}

template <bool debughookPresent>
bool SQVM::Return(SQInteger _arg0, SQInteger _arg1, SQObjectPtr &retval)
{
    SQBool    _isroot      = ci->_root;
    SQInteger callerbase   = _stackbase - ci->_prevstkbase;
    if constexpr (debughookPresent)
    {
        if (_debughook) {
            for(SQInteger i=0; i<ci->_ncalls; i++) {
                CallDebugHook(_SC('r'));
            }
        }
    }

    SQObjectPtr *dest;
    if (_isroot) {
        dest = &(retval);
    } else if (ci->_target == -1) {
        dest = NULL;
    } else {
        dest = &_stack._vals[callerbase + ci->_target];
    }
    if (dest) {
        if(_arg0 != 0xFF) {
            *dest = _stack._vals[_stackbase+_arg1];
        }
        else {
            dest->Null();
        }
        //*dest = (_arg0 != 0xFF) ? _stack._vals[_stackbase+_arg1] : _null_;
    }
    LeaveFrame();
    return _isroot ? true : false;
}

#define _RET_ON_FAIL(exp) { if(!exp) return false; }

bool SQVM::PLOCAL_INC(SQInteger op,SQObjectPtr &target, SQObjectPtr &a, SQObjectPtr &incr)
{
    SQObjectPtr trg;
    _RET_ON_FAIL(ARITH_OP( op , trg, a, incr));
    target = a;
    a = trg;
    return true;
}

bool SQVM::DerefInc(SQInteger op,SQObjectPtr &target, SQObjectPtr &self, SQObjectPtr &key, SQObjectPtr &incr, bool postfix,SQInteger selfidx)
{
    if (self._flags & SQOBJ_FLAG_IMMUTABLE) {
        Raise_Error(_SC("trying to modify immutable '%s'"),GetTypeName(self));
        return false;
    }
    SQObjectPtr tmp, tself = self, tkey = key;
    SQObjectPtr *__restrict instanceValue = nullptr;
    if (sq_type(tself) == OT_INSTANCE)
    {
        SQInstance*__restrict instance = _instance(tself);
        const SQClass *__restrict classType = instance->_class;
        const SQTable::_HashNode *n = classType->_members->_Get(tkey);
        if (n && _isfield(n->val))
            instanceValue = instance->_values + _member_idx(n->val);
    } else if (sq_type(tself) == OT_TABLE)
    {
        SQTable::_HashNode *node = _table(tself)->_Get(tkey);
        if (node)
            instanceValue = &node->val;
    } else if (sq_type(tself) == OT_ARRAY){
        if (sq_isnumeric(key)) {
           SQArray * __restrict array = _array(tself);
           uint32_t nidx  = tointeger(tkey);
           if (nidx < (SQInteger)array->_values.size())
              instanceValue = &array->_values[nidx];
        }
    }
    if (instanceValue)
        tmp = _realval(*instanceValue);
    else
    {
        //delegates and OT_USERDATA option. Basically FallbackGet/FallbackSet
        if (!Get(tself, tkey, tmp, 0, selfidx)) { return false; }
    }
    _RET_ON_FAIL(ARITH_OP( op , target, tmp, incr))
    if (instanceValue)
        *instanceValue = target;
    else
        if (!Set(tself, tkey, target,selfidx)) { return false; }
    if (postfix) target = tmp;
    return true;
}

#define arg0 (_i_._arg0)
#define arg1 (_i_._arg1)
#define sarg1 (((SQInt32)_i_._arg1))
#define farg1 (_i_._farg1)
#define arg2 (_i_._arg2)
#define arg3 (_i_._arg3)
#define sarg3 ((SQInteger)((signed char)_i_._arg3))
#define sarg0 ((SQInteger)((signed char)_i_._arg0))

SQRESULT SQVM::Suspend()
{
    if (_suspended)
        return sq_throwerror(this, _SC("cannot suspend an already suspended vm"));
    if (_nnativecalls!=2)
        return sq_throwerror(this, _SC("cannot suspend through native calls/metamethods"));
    return SQ_SUSPEND_FLAG;
}


#define _FINISH(howmuchtojump) {jump = howmuchtojump; return true; }
bool SQVM::FOREACH_OP(SQObjectPtr &o1,SQObjectPtr &o2,SQObjectPtr
&o3,SQObjectPtr &o4,SQInteger SQ_UNUSED_ARG(arg_2),int exitpos,int &jump)
{
    SQInteger nrefidx;
    switch(sq_type(o1)) {
    case OT_TABLE:
        if((nrefidx = _table(o1)->Next(false,o4, o2, o3)) == -1) _FINISH(exitpos);
        o4 = (SQInteger)nrefidx; _FINISH(1);
    case OT_ARRAY:
        if((nrefidx = _array(o1)->Next(o4, o2, o3)) == -1) _FINISH(exitpos);
        o4 = (SQInteger) nrefidx; _FINISH(1);
    case OT_STRING:
        if((nrefidx = _string(o1)->Next(o4, o2, o3)) == -1)_FINISH(exitpos);
        o4 = (SQInteger)nrefidx; _FINISH(1);
    case OT_CLASS:
        if((nrefidx = _class(o1)->Next(o4, o2, o3)) == -1)_FINISH(exitpos);
        o4 = (SQInteger)nrefidx; _FINISH(1);
    case OT_USERDATA:
    case OT_INSTANCE:
        if(_delegable(o1)->_delegate) {
            SQObjectPtr itr;
            SQObjectPtr closure;
            if(_delegable(o1)->GetMetaMethod(this, MT_NEXTI, closure)) {
                Push(o1);
                Push(o4);
                if(CallMetaMethod(closure, MT_NEXTI, 2, itr)) {
                    o4 = o2 = itr;
                    if(sq_type(itr) == OT_NULL) _FINISH(exitpos);
                    if(!Get(o1, itr, o3, 0, DONT_FALL_BACK)) {
                        Raise_Error(_SC("_nexti returned an invalid idx")); // cloud be changed
                        return false;
                    }
                    _FINISH(1);
                }
                else {
                    return false;
                }
            }
            if ((nrefidx = (_delegable(o1)->_delegate)->Next(false, o4, o2, o3)) == -1)
              _FINISH(exitpos);

            _instance(o1)->Get(o2, o3);
            o4 = (SQInteger)nrefidx;
            _FINISH(1);
        }
        break;
    case OT_GENERATOR:
        if(_generator(o1)->_state == SQGenerator::eDead) _FINISH(exitpos);
        if(_generator(o1)->_state == SQGenerator::eSuspended) {
            SQInteger idx = 0;
            if(sq_type(o4) == OT_INTEGER) {
                idx = _integer(o4) + 1;
            }
            o2 = idx;
            o4 = idx;
            _generator(o1)->Resume(this, o3);
            _FINISH(0);
        }
    default:
        Raise_Error(_SC("cannot iterate %s"), GetTypeName(o1));
    }
    return false; //cannot be hit(just to avoid warnings)
}

#define COND_LITERAL (arg3!=0?ci->_literals[arg1]:STK(arg1))

#define SQ_THROW() { goto exception_trap; }

#define _GUARD(exp) { if(!exp) { SQ_THROW();} }

bool SQVM::CLOSURE_OP(SQObjectPtr &target, SQFunctionProto *func)
{
    SQInteger nouters;
    SQClosure *closure = SQClosure::Create(_ss(this), func);
    if((nouters = func->_noutervalues)) {
        for(SQInteger i = 0; i<nouters; i++) {
            SQOuterVar &v = func->_outervalues[i];
            switch(v._type){
            case otLOCAL:
                FindOuter(closure->_outervalues[i], &STK(_integer(v._src)));
                break;
            case otOUTER:
                closure->_outervalues[i] = _closure(ci->_closure)->_outervalues[_integer(v._src)];
                break;
            }
        }
    }
    SQInteger ndefparams;
    if((ndefparams = func->_ndefaultparams)) {
        for(SQInteger i = 0; i < ndefparams; i++) {
            SQInteger spos = func->_defaultparams[i];
            closure->_defaultparams[i] = _stack._vals[_stackbase + spos];
        }
    }
    target = closure;
    return true;

}


bool SQVM::CLASS_OP(SQObjectPtr &target,SQInteger baseclass)
{
    SQClass *base = NULL;
    if (baseclass != -1) {
        if (sq_type(_stack._vals[_stackbase+baseclass]) != OT_CLASS) {
            Raise_Error(_SC("trying to inherit from a %s"),GetTypeName(_stack._vals[_stackbase+baseclass]));
            return false;
        }
        base = _class(_stack._vals[_stackbase + baseclass]);
    }
    SQClass *cls = SQClass::Create(this, base);
    if (!cls)
        return false;
    target = cls;
    return true;
}

bool SQVM::IsEqual(const SQObjectPtr &o1,const SQObjectPtr &o2,bool &res)
{
    SQObjectType t1 = sq_type(o1), t2 = sq_type(o2);
    if(t1 == t2) {
        if (t1 == OT_FLOAT) {
            res = (_float(o1) == _float(o2));
        }
        else {
            res = (_rawval(o1) == _rawval(o2));
        }
    }
    else {
        if(sq_isnumeric(o1) && sq_isnumeric(o2)) {
            res = (tofloat(o1) == tofloat(o2));
        }
        else {
            res = false;
        }
    }
    return true;
}

bool SQVM::IsFalse(const SQObjectPtr &o)
{
    if(((sq_type(o) & SQOBJECT_CANBEFALSE)
        && ( ((sq_type(o) == OT_FLOAT) && (_float(o) == SQFloat(0.0))) ))
#if !defined(SQUSEDOUBLE) || (defined(SQUSEDOUBLE) && defined(_SQ64))
        || (_integer(o) == 0) )  //OT_NULL|OT_INTEGER|OT_BOOL
#else
        || (((sq_type(o) != OT_FLOAT) && (_integer(o) == 0))) )  //OT_NULL|OT_INTEGER|OT_BOOL
#endif
    {
        return true;
    }
    return false;
}

#if defined(SQ_USED_MEM_COUNTER_DECL)
  SQ_USED_MEM_COUNTER_DECL
#endif

extern SQInstructionDesc g_InstrDesc[];

template <bool debughookPresent>
bool SQVM::Execute(SQObjectPtr &closure, SQInteger nargs, SQInteger stackbase,SQObjectPtr &outres, SQBool invoke_err_handler,ExecutionType et)
{
    ValidateThreadAccess();

    #if SQ_CHECK_THREAD >= SQ_CHECK_THREAD_LEVEL_FAST
    if (_get_current_thread_id_func)
    {
      if (!_nnativecalls)
        _current_thread = _get_current_thread_id_func();
      else
        assert(_current_thread == _get_current_thread_id_func());
    }
    #endif

    if ((_nnativecalls + 1) > MAX_NATIVE_CALLS) { Raise_Error(_SC("Native stack overflow")); return false; }
    _nnativecalls++;
    AutoDec ad(&_nnativecalls);
    SQInteger traps = 0;
    CallInfo *prevci = ci;

    switch(et) {
        case ET_CALL: {
            temp_reg = closure;
            if(!StartCall<debughookPresent>(_closure(temp_reg), _top - nargs, nargs, stackbase, false)) {
                //call the handler if there are no calls in the stack, if not relies on the previous node
                if(ci == NULL) CallErrorHandler(_lasterror);
                return false;
            }
            if(ci == prevci) {
                outres = STK(_top-nargs);
                return true;
            }
            ci->_root = SQTrue;
                      }
            break;
        case ET_RESUME_GENERATOR:
            if(!_generator(closure)->Resume(this, outres)) {
                return false;
            }
            ci->_root = SQTrue;
            traps += ci->_etraps;
            break;
        case ET_RESUME_VM:
        case ET_RESUME_THROW_VM:
            traps = _suspended_traps;
            ci->_root = _suspended_root;
            _suspended = SQFalse;
            if(et  == ET_RESUME_THROW_VM) { SQ_THROW(); }
            break;
    }

exception_restore:
    //
    {
        int prevLineNum = -1;
        int lineHint = 0;

        for(;;)
        {
            if constexpr (debughookPresent) {
                if (_debughook) {
                    SQFunctionProto *funcProto = _closure(ci->_closure)->_function;
                    bool isLineOp;
                    int curLineNum = funcProto->GetLine(ci->_ip, &lineHint, &isLineOp);
                    if (curLineNum != prevLineNum) {
                        prevLineNum = curLineNum;
                        if (isLineOp)
                            CallDebugHook(_SC('l'), curLineNum);
                    }
                }
            }

            const SQInstruction &_i_ = *ci->_ip++;
            //dumpstack(_stackbase);
            //printf("\n%s[%d] %s %d %d %d %d\n",_stringval(_closure(ci->_closure)->_function->_name), ci->_ip-1-_closure(ci->_closure)->_function->_instructions,g_InstrDesc[_i_.op].name,arg0,arg1,arg2,arg3);
            switch(_i_.op)
            {
            case _OP_DATA_NOP: continue;
            case _OP_LOAD: TARGET = ci->_literals[arg1]; continue;
            case _OP_LOADINT:
#ifndef _SQ64
                TARGET = (SQInteger)arg1; continue;
#else
                TARGET = (SQInteger)((SQInt32)arg1); continue;
#endif
            case _OP_LOADFLOAT: TARGET = farg1; continue;
            case _OP_DLOAD: TARGET = ci->_literals[arg1]; STK(arg2) = ci->_literals[arg3];continue;
            case _OP_TAILCALL:{
                SQObjectPtr &t = STK(arg1);
                if (sq_type(t) == OT_CLOSURE
                    && (!_closure(t)->_function->_bgenerator)){
                    SQObjectPtr clo = t;
                    SQInteger last_top = _top;
                    if(_openouters) CloseOuters(&(_stack._vals[_stackbase]));
                    for (SQInteger i = 0; i < arg3; i++) STK(i) = STK(arg2 + i);
                    _GUARD(StartCall<debughookPresent>(_closure(clo), ci->_target, arg3, _stackbase, true));
                    if (last_top >= _top) {
                      _top = last_top;
                    }
                    continue;
                }
                              }
            case _OP_CALL:
            case _OP_NULLCALL:
            {
                    SQObjectPtr clo = STK(arg1);
                    int tgt0 = arg0 == 255 ? -1 : arg0;
                    bool nullcall = (_i_.op == _OP_NULLCALL);
                    switch (sq_type(clo)) {
                    case OT_CLOSURE:
                        _GUARD(StartCall<debughookPresent>(_closure(clo), tgt0, arg3, _stackbase+arg2, false));
                        continue;
                    case OT_NATIVECLOSURE: {
                        bool suspend;
                        bool tailcall;
                        _GUARD(CallNative(_nativeclosure(clo), arg3, _stackbase+arg2, clo, tgt0, suspend, tailcall));
                        if(suspend){
                            _suspended = SQTrue;
                            _suspended_target = tgt0;
                            _suspended_root = ci->_root;
                            _suspended_traps = traps;
                            outres = clo;
                            return true;
                        }
                        if(tgt0 != -1 && !tailcall) {
                            STK(tgt0) = clo;
                        }
                                           }
                        continue;
                    case OT_CLASS:{
                        SQObjectPtr inst, ctor;
                        _GUARD(CreateClassInstance(_class(clo),inst,ctor));
                        if(tgt0 != -1) {
                            STK(tgt0) = inst;
                        }
                        SQInteger stkbase;
                        switch(sq_type(ctor)) {
                            case OT_CLOSURE:
                                stkbase = _stackbase+arg2;
                                _stack._vals[stkbase] = inst;
                                _GUARD(StartCall<debughookPresent>(_closure(ctor), -1, arg3, stkbase, false));
                                break;
                            case OT_NATIVECLOSURE:
                                bool dummy;
                                stkbase = _stackbase+arg2;
                                _stack._vals[stkbase] = inst;
                                _GUARD(CallNative(_nativeclosure(ctor), arg3, stkbase, ctor, -1, dummy, dummy));
                                break;
                            case OT_NULL:
                                if (arg3 > 1) {
                                    Raise_Error(_SC("cannot call default constructor with arguments"));
                                    SQ_THROW();
                                }
                                break;
                            default:
                                // cannot happen, but still...
                                assert(!"invalid constructor type");
                                Raise_Error(_SC("invalid constructor type %s"), IdType2Name(sq_type(ctor)));
                                SQ_THROW();
                                break;
                        }
                        }
                        break;
                    case OT_TABLE:
                    case OT_USERDATA:
                    case OT_INSTANCE:{
                        SQObjectPtr mmclosure;
                        if(_delegable(clo)->_delegate && _delegable(clo)->GetMetaMethod(this,MT_CALL,mmclosure)) {
                            Push(clo);
                            for (SQInteger i = 0; i < arg3; i++) Push(STK(arg2 + i));
                            if(!CallMetaMethod(mmclosure, MT_CALL, arg3+1, clo)) SQ_THROW();
                            if(tgt0 != -1) {
                                STK(tgt0) = clo;
                            }
                            break;
                        }

                        //Raise_Error(_SC("attempt to call '%s'"), GetTypeName(clo));
                        //SQ_THROW();
                      }
                    default:
                        if (nullcall && sq_type(clo)==OT_NULL) {
                            if(tgt0 != -1) {
                                STK(tgt0).Null();
                            }
                        } else {
                            Raise_Error(_SC("attempt to call '%s'"), GetTypeName(clo));
                            SQ_THROW();
                        }
                    }
                }
                  continue;
            case _OP_PREPCALL:
            case _OP_PREPCALLK: {
                    SQObjectPtr &key = _i_.op == _OP_PREPCALLK?(ci->_literals)[arg1]:STK(arg1);
                    SQObjectPtr &o = STK(arg2);
                    if (!Get(o, key, temp_reg,0,arg2)) {
                        SQ_THROW();
                    }
                    STK(arg3) = o;
                    _Swap(TARGET,temp_reg);//TARGET = temp_reg;
                }
                continue;
            case _OP_GETK:{
                SQUnsignedInteger getFlagsByOp = (arg3 & OP_GET_FLAG_ALLOW_DEF_DELEGATE) ? 0 : GET_FLAG_NO_DEF_DELEGATE;
                if (arg3 & OP_GET_FLAG_TYPE_METHODS_ONLY)
                    getFlagsByOp |= GET_FLAG_TYPE_METHODS_ONLY;
                if (arg3 & OP_GET_FLAG_NO_ERROR) {
                    SQInteger fb = GetImpl(STK(arg2), ci->_literals[arg1], temp_reg, GET_FLAG_DO_NOT_RAISE_ERROR | getFlagsByOp, DONT_FALL_BACK);
                    if (fb == SLOT_RESOLVE_STATUS_OK) {
                        _Swap(TARGET,temp_reg);//TARGET = temp_reg;
                    } else if (fb == SLOT_RESOLVE_STATUS_ERROR) {
                        SQ_THROW();
                    } else if (!(arg3 & OP_GET_FLAG_KEEP_VAL)) {
                        TARGET.Null();
                    }
                } else {
                    if (!Get(STK(arg2), ci->_literals[arg1], temp_reg, getFlagsByOp, arg2)) {
                        SQ_THROW();
                    }
                    _Swap(TARGET,temp_reg);//TARGET = temp_reg;
                }
                continue;
            }
            case _OP_MOVE: TARGET = STK(arg1); continue;
            case _OP_NEWSLOT:
            case _OP_NEWSLOTK:
                _GUARD(NewSlot(STK(arg2), _i_.op == _OP_NEWSLOTK ? ci->_literals[arg1] : STK(arg1), STK(arg3),false));
                if(arg0 != 0xFF) TARGET = STK(arg3);
                continue;
            case _OP_DELETE: _GUARD(DeleteSlot(STK(arg1), STK(arg2), TARGET)); continue;
            case _OP_SET_LITERAL: {
                uint64_t *__restrict hintP = ((uint64_t*__restrict )(ci->_ip++));
                uint64_t hint = *hintP;
                const SQObjectPtr &from = STK(arg2), &__restrict key = ci->_literals[arg1], &val = STK(arg3);
                auto sqType = sq_type(from);
                if (sqType == OT_INSTANCE && !(from._flags & SQOBJ_FLAG_IMMUTABLE))//for wrong access go to normal Set
                {
                    SQInstance *__restrict instance = _instance(from);
                    const SQClass *__restrict classType = instance->_class;
                    uint32_t memberIdx;
                    //todo:key is string literal, so we better store it's index in literal, or it's hash, rather than use SQObjectPtr from generated previous LOAD command
                    const SQTable *__restrict members = classType->_members;
                    //some class ID. Ideally it is 32bit key, which is correct only when locked
                    //we can achieve that. When unlocked - class has _locked == 0. When _locked != 0, it is class Index+1 in VM (can be just some hash_set, or even just 32 bit hash from pointer)
                    //however, right now we rely on 40 bit hint - which is class pointer. still working good
                    const uint64_t classTypeId = classType->lockedTypeId();
                    if (SQ_LIKELY(classTypeId && SQClass::classTypeFromHint(hint) == classTypeId))
                    {
                        memberIdx = uint32_t(hint>>uintptr_t(SQClass::CLASS_BITS));
                        //todo: validate cache in debug build!
                        //val = hintedMemberIdx ? members->_nodex + hintedMemberIdx-1 : nullptr;
                    } else
                    {
                        //this is optimized version, can be just memberIdx = members->Get(key, tmp_reg) ? _integer(tmp_reg) : 0u; memberIdx = _isfieldi(memberIdx) ? memberIdx : 0u;
                        if (!members->GetStrToInt(key, memberIdx) || !_isfieldi(memberIdx))
                            memberIdx = 0u;
                        //store hint back
                        *hintP = ((uint64_t(memberIdx)<<uintptr_t(SQClass::CLASS_BITS))|classTypeId);
                    }
                    if (SQ_LIKELY(memberIdx != 0u))
                    {
                        instance->SetMemberField(memberIdx, val);
                    } else {
                        if (SQ_UNLIKELY(FallBackSet(from,key,val) != SLOT_RESOLVE_STATUS_OK)) {
                            Raise_IdxError(key);
                            SQ_THROW();
                        }
                    }
                }
                else if (sqType == OT_TABLE &&  !(from._flags & SQOBJ_FLAG_IMMUTABLE))//for wrong access go to normal Set
                {
                    SQTable *__restrict tbl = _table(from);
                    uint64_t cid = tbl->_classTypeId;
                    SQTable::_HashNode *node = nullptr;

                    if (SQ_LIKELY(cid)) {
                        if (SQ_LIKELY((cid & TBL_CLASS_CLASS_MASK) == (hint & TBL_CLASS_CLASS_MASK))) {
                            node = tbl->GetNodeFromTypeHint(hint, key);
                        } else {
                            node = tbl->_GetStr(_rawval(key), _string(key)->_hash & tbl->_numofnodes_minus_one);
                            if (SQ_LIKELY(node)) {
                                size_t nodeIdx = node - tbl->_nodes;
                                assert(nodeIdx <= TBL_CLASS_TYPE_MEMBER_MASK);
                                *hintP = ((cid & TBL_CLASS_CLASS_MASK) | nodeIdx);
                            }
                        }
                    }
                    if (node) {
                        node->val = val;
                    } else {
                        // fallback no unoptimized version
                        if (!Set(from, key, val, arg1)) { SQ_THROW(); }
                    }
                }
                else // default implementation
                {
                    if (!Set(from, key, val, arg1)) { SQ_THROW(); }
                }
                if (arg0 != 0xFF) TARGET = val;
                continue;
            }
            case _OP_SET:
                if (!Set(STK(arg2), STK(arg1), STK(arg3),arg2)) { SQ_THROW(); }
                if (arg0 != 0xFF) TARGET = STK(arg3);
                continue;
            case _OP_SETI: case _OP_SETK:
                if (!Set(STK(arg2), _i_.op == _OP_SETI ? SQInteger(arg1) : ci->_literals[arg1], STK(arg3),arg2)) { SQ_THROW(); }
                if (arg0 != 0xFF) TARGET = STK(arg3);
                continue;
            case _OP_GET_LITERAL:{
                uint64_t *__restrict hintP = ((uint64_t*__restrict )(ci->_ip++));
                uint64_t hint = *hintP;
                const SQUnsignedInteger getFlagsByOp = GET_FLAG_NO_DEF_DELEGATE;
                const SQObjectPtr &__restrict from = STK(arg2), &__restrict key = ci->_literals[arg1];
                auto sqType = sq_type(from);

                //Or we can disallow table access by table.key
                if (sqType == OT_INSTANCE)
                {
                    const SQInstance *__restrict instance = _instance(from);
                    const SQClass *__restrict classType = instance->_class;
                    uint32_t memberIdx;
                    //todo:key is string literal, so we better store it's index in literal, or it's hash, rather than use SQObjectPtr from generated previous LOAD command
                    const SQTable *__restrict members = classType->_members;
                    //some class ID. Ideally it is 32bit key, which is correct only when locked
                    //we can achieve that. When unlocked - class has _locked == 0. When _locked != 0, it is class Index+1 in VM (can be just some hash_set, or even just 32 bit hash from pointer)
                    //however, right now we rely on 40 bit hint - which is class pointer. still working good
                    const uint64_t classTypeId = classType->lockedTypeId();
                    if (SQ_LIKELY(classTypeId && SQClass::classTypeFromHint(hint) == classTypeId))
                    {
                        memberIdx = uint32_t(hint>>uintptr_t(SQClass::CLASS_BITS));
                        //todo: validate cache in debug build!
                        //val = hintedMemberIdx ? members->_nodex + hintedMemberIdx-1 : nullptr;
                    } else
                    {
                        //this is optimized version, can be just memberIdx = members->Get(key, tmp_reg) ? _integer(tmp_reg) : 0u;
                        if (!members->GetStrToInt(key, memberIdx))
                            memberIdx = 0u;
                        //store hint back
                        *hintP = ((uint64_t(memberIdx)<<uintptr_t(SQClass::CLASS_BITS))|classTypeId);
                    }
                    if (SQ_LIKELY(memberIdx != 0u))
                    {
                        instance->GetMember(memberIdx, temp_reg);
                    } else {
                       if (SQ_UNLIKELY(FallBackGet(from,key,temp_reg) != SLOT_RESOLVE_STATUS_OK)) {
                           Raise_IdxError(key);
                           SQ_THROW();
                       }
                    }
                    propagate_immutable(from, temp_reg);
                }
                else if (sqType == OT_TABLE)
                {
                    SQTable *__restrict tbl = _table(from);
                    uint64_t cid = tbl->_classTypeId;
                    SQTable::_HashNode *node = nullptr;

                    if (SQ_LIKELY(cid)) {
                        if (SQ_LIKELY((cid & TBL_CLASS_CLASS_MASK) == (hint & TBL_CLASS_CLASS_MASK))) {
                            node = tbl->GetNodeFromTypeHint(hint, key);
                        } else {
                            node = tbl->_GetStr(_rawval(key), _string(key)->_hash & tbl->_numofnodes_minus_one);
                            if (SQ_LIKELY(node)) {
                                size_t nodeIdx = node - tbl->_nodes;
                                assert(nodeIdx <= TBL_CLASS_TYPE_MEMBER_MASK);
                                *hintP = ((cid & TBL_CLASS_CLASS_MASK) | nodeIdx);
                            }
                        }
                    }
                    if (node) {
                        temp_reg = _realval(node->val);
                        propagate_immutable(from, temp_reg);
                    }
                    else {
                        // fallback no unoptimized version
                        if (!Get(from, key, temp_reg, getFlagsByOp, arg1)) {
                            SQ_THROW();
                        }
                    }
                }
                else
                {
                    if (!Get(from, key, temp_reg, getFlagsByOp, arg1)) {
                        SQ_THROW();
                    }
                }
                _Swap(TARGET,temp_reg);//TARGET = temp_reg;
                continue;
            }
            case _OP_GET:{
                SQUnsignedInteger getFlagsByOp = (arg3 & OP_GET_FLAG_ALLOW_DEF_DELEGATE) ? 0 : GET_FLAG_NO_DEF_DELEGATE;
                if (arg3 & OP_GET_FLAG_TYPE_METHODS_ONLY)
                    getFlagsByOp |= GET_FLAG_TYPE_METHODS_ONLY;
                if (arg3 & OP_GET_FLAG_NO_ERROR) {
                    SQInteger fb = GetImpl(STK(arg2), STK(arg1), temp_reg, GET_FLAG_DO_NOT_RAISE_ERROR | getFlagsByOp, DONT_FALL_BACK);
                    if (fb == SLOT_RESOLVE_STATUS_OK) {
                        _Swap(TARGET,temp_reg);
                    } else if (fb == SLOT_RESOLVE_STATUS_ERROR) {
                        SQ_THROW();
                    } else if (!(arg3 & OP_GET_FLAG_KEEP_VAL)) {
                        TARGET.Null();
                    }
                } else {
                    if (!Get(STK(arg2), STK(arg1), temp_reg, getFlagsByOp, arg2)) {
                        SQ_THROW();
                    }
                    _Swap(TARGET,temp_reg);//TARGET = temp_reg;
                }
                continue;
            }
            case _OP_EQ:{
                bool res;
                if(!IsEqual(STK(arg2),COND_LITERAL,res)) { SQ_THROW(); }
                TARGET = res?true:false;
                }continue;
            case _OP_NE:{
                bool res;
                if(!IsEqual(STK(arg2),COND_LITERAL,res)) { SQ_THROW(); }
                TARGET = (!res)?true:false;
                } continue;
            case _OP_ADD:
              _ARITH_(+,TARGET,STK(arg2),STK(arg1));
              continue;
            case _OP_ADDI: {SQObjectPtr ai = (SQInteger)arg1; _ARITH_(+,TARGET,STK(arg2), ai); continue;}
            case _OP_SUB: _ARITH_(-,TARGET,STK(arg2),STK(arg1)); continue;
            case _OP_MUL: _ARITH_(*,TARGET,STK(arg2),STK(arg1)); continue;
            case _OP_DIV: _ARITH_NOZERO(/,TARGET,STK(arg2),STK(arg1)); continue;
            case _OP_MOD: _GUARD(ARITH_OP('%',TARGET,STK(arg2),STK(arg1))); continue;
            case _OP_BITW:  _GUARD(BW_OP( arg3,TARGET,STK(arg2),STK(arg1))); continue;
            case _OP_RETURN:
                if((ci)->_generator) {
                    (ci)->_generator->Kill();
                }
                if(Return<debughookPresent>(arg0, arg1, temp_reg)){
                    assert(traps==0);
                    //outres = temp_reg;
                    _Swap(outres,temp_reg);
                    return true;
                }
                continue;
            case _OP_LOADNULLS:{ SQInt32 n=arg1-1; assert(n>=0); do { STK(arg0+n).Null(); } while (--n >= 0); }continue;
            case _OP_LOADROOT:
                TARGET = _roottable;
                continue;
            case _OP_LOADBOOL: TARGET = arg1?true:false; continue;
            case _OP_LOADCALLEE:
            TARGET = ci->_closure;
            if (arg2)
            {
                STK(arg2) = STK(arg3);
            }
            continue;
            case _OP_DMOVE: STK(arg0) = STK(arg1); STK(arg2) = STK(arg3); continue;
            case _OP_JMP: ci->_ip += sarg1; continue;
            //case _OP_JNZ: if(!IsFalse(STK(arg0))) ci->_ip+=(sarg1); continue;
            case _OP_JCMP: {
                int r;
                const uint8_t uArg3 = arg3;
                _GUARD(CMP_OP_RES((CmpOP)(uArg3&7),STK(arg2),STK(arg0),r));
                if(uint8_t(bool(r)) == (uArg3>>3)) ci->_ip+=(sarg1);
                }
                continue;
            case _OP_JCMPK: {
                int r;
                const uint8_t uArg3 = arg3;
                _GUARD(CMP_OP_RES((CmpOP)(uArg3&7),STK(arg2),ci->_literals[arg0],r));
                if(uint8_t(bool(r)) == (uArg3>>3)) ci->_ip+=(sarg1);
                }
                continue;
            case _OP_JCMPI: {
                int r;
                //todo: optimize comparison with int
                const uint8_t uArg3 = arg3;
                _GUARD(CMP_OP_RESI((CmpOP)(uArg3&7),STK(arg2), (SQInteger)sarg1, r));
                if(uint8_t(bool(r)) == (uArg3>>3)) ci->_ip+=(sarg0);
                }
                continue;
            case _OP_JCMPF: {
                int r;
                //todo: optimize comparison with float
                const uint8_t uArg3 = arg3;
                _GUARD(CMP_OP_RESF((CmpOP)(uArg3&7),STK(arg2), farg1, r));
                if(uint8_t(bool(r)) == (uArg3>>3)) ci->_ip+=(sarg0);
                }
                continue;
            case _OP_JZ: {
              if(uint8_t(IsFalse(STK(arg0))) != arg2) ci->_ip+=(sarg1);
            } continue;
            case _OP_GETOUTER: {
                SQClosure *cur_cls = _closure(ci->_closure);
                SQOuter *otr = _outer(cur_cls->_outervalues[arg1]);
                TARGET = *(otr->_valptr);
                if (arg2)
                    STK(arg2) = STK(arg3);
                }
            continue;
            case _OP_SETOUTER: {
                SQClosure *cur_cls = _closure(ci->_closure);
                SQOuter   *otr = _outer(cur_cls->_outervalues[arg1]);
                *(otr->_valptr) = STK(arg2);
                if(arg0 != 0xFF) {
                    TARGET = STK(arg2);
                }
                }
            continue;
            case _OP_NEWOBJ:
                switch(arg3) {
                    case NOT_TABLE: TARGET = SQTable::Create(_ss(this), arg1); continue;
                    case NOT_ARRAY: TARGET = SQArray::Create(_ss(this), 0); _array(TARGET)->Reserve(arg1); continue;
                    case NOT_CLASS: _GUARD(CLASS_OP(TARGET,arg1)); continue;
                    default: assert(0); continue;
                }
            case _OP_APPENDARRAY:
                {
                    // No need to check for immutability here since it is only used for array initialization
                    SQObject val;
                    val._unVal.raw = 0;
                    val._flags = 0;
                switch(arg2) {
                case AAT_STACK:
                    val = STK(arg1); break;
                case AAT_LITERAL:
                    val = ci->_literals[arg1]; break;
                case AAT_INT:
                    val._type = OT_INTEGER;
#ifndef _SQ64
                    val._unVal.nInteger = (SQInteger)arg1;
#else
                    val._unVal.nInteger = (SQInteger)((SQInt32)arg1);
#endif
                    break;
                case AAT_FLOAT:
                    val._type = OT_FLOAT;
                    val._unVal.fFloat = *((const SQFloat *)&arg1);
                    break;
                case AAT_BOOL:
                    val._type = OT_BOOL;
                    val._unVal.nInteger = arg1;
                    break;
                default: val._type = OT_INTEGER; assert(0); break;

                }
                _array(STK(arg0))->Append(val); continue;
                }
            case _OP_COMPARITH:
            case _OP_COMPARITH_K: {
                SQInteger selfidx = (((SQUnsignedInteger)arg1&0xFFFF0000)>>16);
                _GUARD(DerefInc(arg3, TARGET, STK(selfidx), _i_.op == _OP_COMPARITH_K ? ci->_literals[arg2] : STK(arg2), STK(arg1&0x0000FFFF), false, selfidx));
                                }
                continue;
            case _OP_INC: {
                SQObjectPtr o(sarg3);
                _GUARD(DerefInc('+',TARGET, STK(arg1), STK(arg2), o, false, arg1));
                } continue;
            case _OP_INCL: {
                SQObjectPtr &a = STK(arg1);
                if(sq_type(a) == OT_INTEGER) {
                    a._unVal.nInteger = _integer(a) + sarg3;
                }
                else {
                    SQObjectPtr o(sarg3); //_GUARD(LOCAL_INC('+',TARGET, STK(arg1), o));
                    _ARITH_(+,a,a,o);
                }
                           } continue;
            case _OP_PINC: {
                SQObjectPtr o(sarg3);
                _GUARD(DerefInc('+',TARGET, STK(arg1), STK(arg2), o, true, arg1));
                } continue;
            case _OP_PINCL: {
                SQObjectPtr &a = STK(arg1);
                if(sq_type(a) == OT_INTEGER) {
                    TARGET = a;
                    a._unVal.nInteger = _integer(a) + sarg3;
                }
                else {
                    SQObjectPtr o(sarg3);
                    _GUARD(PLOCAL_INC('+',TARGET, STK(arg1), o));
                }

                        } continue;
            case _OP_CMP:   _GUARD(CMP_OP((CmpOP)arg3,STK(arg2),STK(arg1),TARGET))  continue;
            case _OP_EXISTS: TARGET = Get(STK(arg1), STK(arg2), temp_reg, GET_FLAG_DO_NOT_RAISE_ERROR | GET_FLAG_NO_DEF_DELEGATE, DONT_FALL_BACK) ? true : false; continue;
            case _OP_INSTANCEOF:
                if(sq_type(STK(arg1)) != OT_CLASS)
                {Raise_Error(_SC("cannot apply instanceof between a %s and a %s"),GetTypeName(STK(arg1)),GetTypeName(STK(arg2))); SQ_THROW();}
                TARGET = (sq_type(STK(arg2)) == OT_INSTANCE) ? (_instance(STK(arg2))->InstanceOf(_class(STK(arg1)))?true:false) : false;
                continue;
            case _OP_AND:{
                if(IsFalse(STK(arg2))) {
                    TARGET = STK(arg2);
                    ci->_ip += (sarg1);
                }

                }
                continue;
            case _OP_OR:{
                if(!IsFalse(STK(arg2))) {
                    TARGET = STK(arg2);
                    ci->_ip += (sarg1);
                }

                }
                continue;
            case _OP_NULLCOALESCE:
                if (!sq_isnull(STK(arg2))) {
                    TARGET = STK(arg2);
                    ci->_ip += (sarg1);
                }
                continue;
            case _OP_NEG: _GUARD(NEG_OP(TARGET,STK(arg1))); continue;
            case _OP_NOT:{
                TARGET = IsFalse(STK(arg1));
            } continue;
            case _OP_BWNOT:
                if(sq_type(STK(arg1)) == OT_INTEGER) {
                    SQInteger t = _integer(STK(arg1));
                    TARGET = SQInteger(~t);
                    continue;
                }
                Raise_Error(_SC("attempt to perform a bitwise op on a %s"), GetTypeName(STK(arg1)));
                SQ_THROW();
            case _OP_CLOSURE: {
                SQClosure *c = ci->_closure._unVal.pClosure;
                SQFunctionProto *fp = c->_function;
                if(!CLOSURE_OP(TARGET,fp->_functions[arg1]._unVal.pFunctionProto)) { SQ_THROW(); }
                continue;
            }
            case _OP_YIELD:{
                if(ci->_generator) {
                    if(sarg1 != MAX_FUNC_STACKSIZE) temp_reg = STK(arg1);
                    if (_openouters) CloseOuters(&_stack._vals[_stackbase]);
                    _GUARD(ci->_generator->Yield(this,arg2));
                    traps -= ci->_etraps;
                    if(sarg1 != MAX_FUNC_STACKSIZE) _Swap(STK(arg1),temp_reg);//STK(arg1) = temp_reg;
                }
                else { Raise_Error(_SC("trying to yield a '%s',only genenerator can be yielded"), GetTypeName(ci->_generator)); SQ_THROW();}
                if(Return<debughookPresent>(arg0, arg1, temp_reg)){
                    assert(traps == 0);
                    outres = temp_reg;
                    return true;
                }

                }
                continue;
            case _OP_RESUME:
                if(sq_type(STK(arg1)) != OT_GENERATOR){ Raise_Error(_SC("trying to resume a '%s',only genenerator can be resumed"), GetTypeName(STK(arg1))); SQ_THROW();}
                _GUARD(_generator(STK(arg1))->Resume(this, TARGET));
                traps += ci->_etraps;
                continue;
            case _OP_PREFOREACH:{
                STK(arg2).Null();STK(arg2+1).Null();STK(arg2+2).Null();
                auto &arg0Stack = STK(arg0);
                const bool isGenerator = sq_type(arg0Stack) == OT_GENERATOR;
                int tojump;
                _GUARD(FOREACH_OP(arg0Stack,STK(arg2),STK(arg2+1),STK(arg2+2),arg2,2,tojump));
                if (tojump == 1)
                    ci->_ip ++;
                else if (tojump == 2) // empty
                    ci->_ip += sarg1;
                }
                continue;
            case _OP_POSTFOREACH:
                assert(sq_type(STK(arg0)) == OT_GENERATOR);
                if(_generator(STK(arg0))->_state == SQGenerator::eDead)
                    ci->_ip += sarg1;
                continue;
            case _OP_FOREACH:{
                const int jumpToBodyOffset = sarg1;
                auto &arg0Stack = STK(arg0);
                const bool isGenerator = sq_type(arg0Stack) == OT_GENERATOR;
                if (isGenerator)
                    ci->_ip += jumpToBodyOffset - 1;  // so generator will resume at postforeach
                int tojump;
                _GUARD(FOREACH_OP(arg0Stack,STK(arg2),STK(arg2+1),STK(arg2+2),arg2,2,tojump));
                assert((tojump == 0 && isGenerator) || (tojump != 0 && !isGenerator));
                if (tojump == 1)
                    ci->_ip += jumpToBodyOffset;
                }continue;
            case _OP_CLONE: _GUARD(Clone(STK(arg1), TARGET)); continue;
            case _OP_TYPEOF: _GUARD(TypeOf(STK(arg1), TARGET)) continue;
            case _OP_PUSHTRAP:{
                SQInstruction *_iv = _closure(ci->_closure)->_function->_instructions;
                _etraps.push_back(SQExceptionTrap(_top,_stackbase, &_iv[(ci->_ip-_iv)+arg1], arg0)); traps++;
                ci->_etraps++;
                              }
                continue;
            case _OP_POPTRAP: {
                for(SQInteger i = 0; i < arg0; i++) {
                    _etraps.pop_back(); traps--;
                    ci->_etraps--;
                }
                              }
                continue;
            case _OP_THROW: Raise_Error(TARGET); SQ_THROW(); continue;
            case _OP_NEWSLOTA:
                _GUARD(NewSlot(STK(arg1),STK(arg2),STK(arg3),(arg0&NEW_SLOT_STATIC_FLAG)?true:false));
                continue;
            case _OP_GETBASE:{
                SQClosure *clo = _closure(ci->_closure);
                if(clo->_base) {
                    TARGET = clo->_base;
                }
                else {
                    TARGET.Null();
                }
                continue;
            }
            case _OP_CLOSE:
                if(_openouters) CloseOuters(&(STK(arg1)));
                continue;
            }

        }
    }
exception_trap:
    {
        SQObjectPtr currerror = _lasterror;
//      dumpstack(_stackbase);
//      SQInteger n = 0;
        SQInteger last_top = _top;

        if ((_ss(this)->_notifyallexceptions || (!traps && invoke_err_handler)) && !sq_isnull(currerror))
            CallErrorHandler(currerror);

        while( ci ) {
            if(ci->_etraps > 0) {
                SQExceptionTrap &et = _etraps.top();
                ci->_ip = et._ip;
                _top = et._stacksize;
                _stackbase = et._stackbase;
                _stack._vals[_stackbase + et._extarget] = currerror;
                _etraps.pop_back(); traps--; ci->_etraps--;
                while(last_top >= _top) _stack._vals[last_top--].Null();
                goto exception_restore;
            }
            else if (_debughook) {
                    //notify debugger of a "return"
                    //even if it really an exception unwinding the stack
                    for(SQInteger i = 0; i < ci->_ncalls; i++) {
                        CallDebugHook(_SC('r'));
                    }
            }
            if(ci->_generator) ci->_generator->Kill();
            bool mustbreak = ci && ci->_root;
            LeaveFrame();
            if(mustbreak) break;
        }

        _lasterror = currerror;
        return false;
    }
    assert(0);
    return false;
}

bool SQVM::CreateClassInstance(SQClass *theclass, SQObjectPtr &__restrict out_inst_res, SQObjectPtr &__restrict constructor)
{
    SQInstance *inst = theclass->CreateInstance(this);
    if (!inst)
        return false;

    out_inst_res = inst;
    if (!theclass->GetConstructor(constructor)) {
        constructor.Null();
    }
    return true;
}

void SQVM::CallErrorHandler(SQObjectPtr &error)
{
  ci->_ip--;
  if (_debughook_native)
  {
    if (ci->_closure._type == OT_NATIVECLOSURE)
    {
      const SQChar *errStr = _stringval(error);
      _debughook_native(this, _SC('e'), _SC(""), 0, errStr);
    }
    else
    {
      SQFunctionProto *func = _closure(ci->_closure)->_function;
      if (func)
      {
        const SQChar *src = sq_type(func->_sourcename) == OT_STRING ? _stringval(func->_sourcename) : NULL;
        const SQChar *errStr = _stringval(error);
        SQInteger line = func->GetLine(ci->_ip);
        _debughook_native(this, _SC('e'), src, line, errStr);
      }
    }
  }

    if(sq_type(_errorhandler) != OT_NULL) {
        SQObjectPtr out;
        assert(_top+2 <= _stack.size());
        Push(_roottable); Push(error);
        Call(_errorhandler, 2, _top-2, out,SQFalse);
        Pop(2);
    }

  ci->_ip++;
}


void SQVM::CallDebugHook(SQInteger type,SQInteger forcedline)
{
    _debughook = false;
    if (!ci) {
        if (_debughook_native)
            _debughook_native(this, type, _SC(""), -1, _SC(""));
        else {
            SQObjectPtr temp_reg; // -V688
            SQInteger nparams = 5;
            Push(_roottable);
            Push(type);
            Push(SQString::Create(_ss(this), _SC("")));
            Push(SQInteger(-1));
            Push(SQString::Create(_ss(this), _SC("")));
            Call(_debughook_closure, nparams, _top - nparams, temp_reg, SQFalse);
            Pop(nparams);
        }
        _debughook = true;
        return;
    }

    SQFunctionProto *func=_closure(ci->_closure)->_function;
    if(_debughook_native) {
        const SQChar *src = sq_type(func->_sourcename) == OT_STRING?_stringval(func->_sourcename):NULL;
        const SQChar *fname = sq_type(func->_name) == OT_STRING?_stringval(func->_name):NULL;
        SQInteger line = forcedline?forcedline:func->GetLine(ci->_ip);
        _debughook_native(this,type,src,line,fname);
    }
    else {
        SQObjectPtr temp_reg;
        SQInteger nparams=5;
        Push(_roottable); Push(type); Push(func->_sourcename); Push(forcedline?forcedline:func->GetLine(ci->_ip)); Push(func->_name);
        Call(_debughook_closure,nparams,_top-nparams,temp_reg,SQFalse);
        Pop(nparams);
    }
    _debughook = true;
}

bool SQVM::CallNative(SQNativeClosure *nclosure, SQInteger nargs, SQInteger newbase, SQObjectPtr &retval, SQInt32 target,bool &suspend, bool &tailcall)
{
    SQInteger nparamscheck = nclosure->_nparamscheck;
    SQInteger newtop = newbase + nargs + nclosure->_noutervalues;

    if (_nnativecalls + 1 > MAX_NATIVE_CALLS) {
        Raise_Error(_SC("Native stack overflow"));
        return false;
    }

    if(nparamscheck && (((nparamscheck > 0) && (nparamscheck != nargs)) ||
        ((nparamscheck < 0) && (nargs < (-nparamscheck)))))
    {
        if (nparamscheck > 0) {
            Raise_Error(_SC("wrong number of parameters passed to native closure '%s' (%d passed, %d required)"),
                sq_type(nclosure->_name) == OT_STRING ? _stringval(nclosure->_name) : _SC("unknown"),
                (int)nargs, (int)nparamscheck);
        } else {
            Raise_Error(_SC("wrong number of parameters passed to native closure '%s' (%d passed, at least %d required)"),
                sq_type(nclosure->_name) == OT_STRING ? _stringval(nclosure->_name) : _SC("unknown"),
                (int)nargs, (int)-nparamscheck);
        }

        return false;
    }

    SQInteger tcs;
    SQIntVec &tc = nclosure->_typecheck;
    if((tcs = tc.size())) {
        for(SQInteger i = 0; i < nargs && i < tcs; i++) {
            if((tc._vals[i] != -1) && !(sq_type(_stack._vals[newbase+i]) & tc._vals[i])) {
                Raise_ParamTypeError(i,tc._vals[i], sq_type(_stack._vals[newbase+i]));
                return false;
            }
        }
    }

    if(!EnterFrame(newbase, newtop, false)) return false;
    ci->_closure  = nclosure;
    ci->_target = target;

    SQInteger outers = nclosure->_noutervalues;
    for (SQInteger i = 0; i < outers; i++) {
        _stack._vals[newbase+nargs+i] = nclosure->_outervalues[i];
    }
    if(nclosure->_env) {
        _stack._vals[newbase] = nclosure->_env->_obj;
    }

    _nnativecalls++;
    SQInteger ret = (nclosure->_function)(this);
    _nnativecalls--;

    suspend = false;
    tailcall = false;
    if (ret == SQ_TAILCALL_FLAG) {
        tailcall = true;
        return true;
    }
    else if (ret == SQ_SUSPEND_FLAG) {
        suspend = true;
    }
    else if (ret < 0) {
        LeaveFrame();
        Raise_Error(_lasterror);
        return false;
    }
    if(ret) {
        retval = _stack._vals[_top-1];
    }
    else {
        retval.Null();
    }
    //retval = ret ? _stack._vals[_top-1] : _null_;
    LeaveFrame();
    return true;
}

bool SQVM::TailCall(SQClosure *closure, SQInteger parambase,SQInteger nparams)
{
    SQInteger last_top = _top;
    SQObjectPtr clo = closure;
    if (ci->_root)
    {
        Raise_Error("root calls cannot invoke tailcalls");
        return false;
    }
    for (SQInteger i = 0; i < nparams; i++) STK(i) = STK(parambase + i);
    bool ret = StartCall<true>(closure, ci->_target, nparams, _stackbase, true);
    if (last_top >= _top) {
        _top = last_top;
    }
    return ret;
}


bool SQVM::GetVarTrace(const SQObjectPtr &self, const SQObjectPtr &key, char * buf, int buf_size)
{
#if SQ_VAR_TRACE_ENABLED == 1

  SQObjectPtr tmp;
  VarTrace * vt = NULL;
  const char * reason = "";

  if (_ss(this)->_varTraceEnabled == false)
  {
    reason = "vartrace is disabled for this VM";
  }
  else
  {
    switch (sq_type(self))
    {
    case OT_TABLE:
      if (_table(self)->Get(key, tmp))
        vt = _table(self)->GetVarTracePtr(key);
      else
        reason = "key not found";
      break;
    case OT_ARRAY:
      if (sq_isnumeric(key))
      {
        if (_array(self)->Get(tointeger(key), tmp))
          vt = _array(self)->GetVarTracePtr(tointeger(key));
        else
          reason = "index not found";
      }
      break;

    default:
      reason = "not a table or array";
      break;
    }
  }

  if (vt)
  {
    vt->printStack(buf, buf_size);
    return true;
  }
  else
  {
    *buf = 0;
    strncat(buf, reason, buf_size);
    buf[buf_size - 1] = 0;
    return false;
  }

#else

  (void)self;
  (void)key;

  strncpy(buf, "vartrace is disabled - use build with SQ_VAR_TRACE_ENABLED=1 option set", buf_size);
  buf[buf_size-1] = 0;

  return false;

#endif
}

bool SQVM::Get(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &dest, SQUnsignedInteger getflags, SQInteger selfidx)
{
    SQInteger fb = GetImpl(self, key, dest, getflags, selfidx);
    return fb == SLOT_RESOLVE_STATUS_OK;
}

SQInteger SQVM::GetImpl(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &dest, SQUnsignedInteger getflags, SQInteger selfidx)
{
    if (getflags & GET_FLAG_TYPE_METHODS_ONLY) {
        if (InvokeDefaultDelegate(self, key, dest)) {
          propagate_immutable(self, dest);
          return SLOT_RESOLVE_STATUS_OK;
        }
        if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0) Raise_IdxError(key);
        return SLOT_RESOLVE_STATUS_NO_MATCH;
    }
    switch(sq_type(self)){
    case OT_TABLE:
        if(_table(self)->Get(key,dest)) {
            propagate_immutable(self, dest);
            return SLOT_RESOLVE_STATUS_OK;
        }
        break;
    case OT_ARRAY:
        if (sq_isnumeric(key)) {
            if (_array(self)->Get(tointeger(key), dest)) {
                propagate_immutable(self, dest);
                return SLOT_RESOLVE_STATUS_OK;
            }
            if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0)
                Raise_IdxError(key);
            return SLOT_RESOLVE_STATUS_NO_MATCH;
        }
        break;
    case OT_INSTANCE:
        if(_instance(self)->Get(key,dest)) {
            propagate_immutable(self, dest);
            return SLOT_RESOLVE_STATUS_OK;
        }
        break;
    case OT_CLASS:
        if(_class(self)->Get(key,dest)) {
            propagate_immutable(self, dest);
            return SLOT_RESOLVE_STATUS_OK;
        }
        break;
    case OT_STRING:
        if(sq_isnumeric(key)){
            SQInteger n = tointeger(key);
            SQInteger len = _string(self)->_len;
            if (n < 0) { n += len; }
            if (n >= 0 && n < len) {
                dest = SQInteger(_stringval(self)[n]);
                return SLOT_RESOLVE_STATUS_OK;
            }
            if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0) Raise_IdxError(key);
            return SLOT_RESOLVE_STATUS_NO_MATCH;
        }
        break;
    default:break; //shut up compiler
    }
    if ((getflags & GET_FLAG_RAW) == 0) {
        switch(FallBackGet(self,key,dest)) {
            case SLOT_RESOLVE_STATUS_OK:
                propagate_immutable(self, dest);
                return SLOT_RESOLVE_STATUS_OK; //okie
            case SLOT_RESOLVE_STATUS_NO_MATCH: break; //keep falling back
            case SLOT_RESOLVE_STATUS_ERROR: return SLOT_RESOLVE_STATUS_ERROR; // the metamethod failed
        }
    }
    if (!(getflags & (GET_FLAG_RAW | GET_FLAG_NO_DEF_DELEGATE)))
    {
        if(InvokeDefaultDelegate(self,key,dest)) {
            propagate_immutable(self, dest);
            return SLOT_RESOLVE_STATUS_OK;
        }
    }
    if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0) Raise_IdxError(key);
    return SLOT_RESOLVE_STATUS_NO_MATCH;
}

bool SQVM::InvokeDefaultDelegate(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest)
{
    SQTable *ddel = NULL;
    switch(sq_type(self)) {
        case OT_CLASS: ddel = _class_ddel; break;
        case OT_TABLE: ddel = _table_ddel; break;
        case OT_ARRAY: ddel = _array_ddel; break;
        case OT_STRING: ddel = _string_ddel; break;
        case OT_INSTANCE: ddel = _instance_ddel; break;
        case OT_INTEGER:case OT_FLOAT:case OT_BOOL: ddel = _number_ddel; break;
        case OT_GENERATOR: ddel = _generator_ddel; break;
        case OT_CLOSURE: case OT_NATIVECLOSURE: ddel = _closure_ddel; break;
        case OT_THREAD: ddel = _thread_ddel; break;
        case OT_WEAKREF: ddel = _weakref_ddel; break;
        case OT_USERDATA: ddel = _userdata_ddel; break;
        default: return false;
    }
    return  ddel->Get(key,dest);
}


SQInteger SQVM::FallBackGet(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest)
{
    switch(sq_type(self)){
    case OT_TABLE:
    case OT_USERDATA:
        //delegation
        if(_delegable(self)->_delegate) {
            if(Get(SQObjectPtr(_delegable(self)->_delegate),key,dest,0,DONT_FALL_BACK)) return SLOT_RESOLVE_STATUS_OK;
        }
        else {
            return SLOT_RESOLVE_STATUS_NO_MATCH;
        }
        //go through
    case OT_INSTANCE: {
        SQObjectPtr closure;
        if(_delegable(self)->GetMetaMethod(this, MT_GET, closure)) {
            Push(self);Push(key);
            _nmetamethodscall++;
            AutoDec ad(&_nmetamethodscall);
            if(Call(closure, 2, _top - 2, dest, SQFalse)) {
                Pop(2);
                return SLOT_RESOLVE_STATUS_OK;
            }
            else {
                Pop(2);
                if(sq_type(_lasterror) != OT_NULL) { //NULL means "clean failure" (not found)
                    return SLOT_RESOLVE_STATUS_ERROR;
                }
            }
        }
        }
        break;
    default: break;//shutup GCC 4.x
    }
    // no metamethod or no fallback type
    return SLOT_RESOLVE_STATUS_NO_MATCH;
}

bool SQVM::Set(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val,SQInteger selfidx)
{
    if (self._flags & SQOBJ_FLAG_IMMUTABLE) {
        Raise_Error(_SC("trying to modify immutable '%s'"),GetTypeName(self));
        return false;
    }

    switch(sq_type(self)){
    case OT_TABLE:
        if(_table(self)->Set(key,val)) return true;
        break;
    case OT_INSTANCE:
        if(_instance(self)->Set(key,val)) return true;
        break;
    case OT_ARRAY:
        if(!sq_isnumeric(key)) { Raise_Error(_SC("indexing %s with %s"),GetTypeName(self),GetTypeName(key)); return false; }
        if(!_array(self)->Set(tointeger(key),val)) {
            Raise_IdxError(key);
            return false;
        }
        return true;
    case OT_USERDATA: break; // must fall back
    default:
        Raise_Error(_SC("trying to set '%s'"),GetTypeName(self));
        return false;
    }

    switch(FallBackSet(self,key,val)) {
        case SLOT_RESOLVE_STATUS_OK: return true; //okie
        case SLOT_RESOLVE_STATUS_NO_MATCH: break; //keep falling back
        case SLOT_RESOLVE_STATUS_ERROR: return false; // the metamethod failed
    }
    if(selfidx == 0) {
        if(_table(_roottable)->Set(key,val))
            return true;
    }
    Raise_IdxError(key);
    return false;
}

SQInteger SQVM::FallBackSet(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val)
{
    switch(sq_type(self)) {
        case OT_TABLE:
            if(_table(self)->_delegate) {
                if(Set(_table(self)->_delegate,key,val,DONT_FALL_BACK)) return SLOT_RESOLVE_STATUS_OK;
            }
            //keps on going
        case OT_INSTANCE:
        case OT_USERDATA:{
            SQObjectPtr closure;
            SQObjectPtr t;
            if(_delegable(self)->GetMetaMethod(this, MT_SET, closure)) {
                Push(self);Push(key);Push(val);
                _nmetamethodscall++;
                AutoDec ad(&_nmetamethodscall);
                if(Call(closure, 3, _top - 3, t, SQFalse)) {
                    Pop(3);
                    return SLOT_RESOLVE_STATUS_OK;
                }
                else {
                    Pop(3);
                    if(sq_type(_lasterror) != OT_NULL) { //NULL means "clean failure" (not found)
                        return SLOT_RESOLVE_STATUS_ERROR;
                    }
                }
            }
        }
        break;
        default: break;//shutup GCC 4.x
    }
    // no metamethod or no fallback type
    return SLOT_RESOLVE_STATUS_NO_MATCH;
}

bool SQVM::Clone(const SQObjectPtr &self,SQObjectPtr &target)
{
    SQObjectPtr temp_reg;
    SQObjectPtr newobj;
    switch(sq_type(self)){
    case OT_TABLE:
        newobj = _table(self)->Clone();
        goto cloned_mt;
    case OT_INSTANCE: {
        newobj = _instance(self)->Clone(_ss(this));
cloned_mt:
        SQObjectPtr closure;
        if(_delegable(newobj)->_delegate && _delegable(newobj)->GetMetaMethod(this,MT_CLONED,closure)) {
            Push(newobj);
            Push(self);
            if(!CallMetaMethod(closure,MT_CLONED,2,temp_reg))
                return false;
        }
        }
        target = newobj;
        return true;
    case OT_ARRAY:
        target = _array(self)->Clone();
        return true;
    case OT_USERDATA:
    case OT_CLASS:
        Raise_Error(_SC("cloning a %s"), GetTypeName(self));
        return false;
    default:
        target = self;
        return true;
    }
}


bool SQVM::NewSlot(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val,bool bstatic)
{
    if (self._flags & SQOBJ_FLAG_IMMUTABLE) {
        Raise_Error(_SC("trying to modify immutable '%s'"),GetTypeName(self));
        return false;
    }

    if(sq_type(key) == OT_NULL) { Raise_Error(_SC("null cannot be used as index")); return false; }
    switch(sq_type(self)) {
    case OT_TABLE: {
        bool rawcall = true;
        if(_table(self)->_delegate) {
            SQObjectPtr res;
            if(!_table(self)->Get(key,res)) {
                SQObjectPtr closure;
                if(_delegable(self)->_delegate && _delegable(self)->GetMetaMethod(this,MT_NEWSLOT,closure)) {
                    Push(self);Push(key);Push(val);
                    if(!CallMetaMethod(closure,MT_NEWSLOT,3,res)) {
                        return false;
                    }
                    rawcall = false;
                }
                else {
                    rawcall = true;
                }
            }
        }
        if(rawcall) _table(self)->NewSlot(key,val); //cannot fail

        break;}
    case OT_INSTANCE: {
        SQObjectPtr res;
        SQObjectPtr closure;
        if(_delegable(self)->_delegate && _delegable(self)->GetMetaMethod(this,MT_NEWSLOT,closure)) {
            Push(self);Push(key);Push(val);
            if(!CallMetaMethod(closure,MT_NEWSLOT,3,res)) {
                return false;
            }
            break;
        }
        Raise_Error(_SC("class instances do not support the new slot operator"));
        return false;
        break;}
    case OT_CLASS:
        if(!_class(self)->NewSlot(_ss(this),key,val,bstatic)) {
            if(_class(self)->isLocked()) {
                Raise_Error(_SC("trying to modify a class that has already been instantiated, inherited or is locked manually"));
                return false;
            }
            else {
                SQObjectPtr oval = PrintObjVal(key);
                Raise_Error(_SC("the property '%s' already exists"),_stringval(oval));
                return false;
            }
        }
        break;
    default:
        Raise_Error(_SC("indexing %s with %s"),GetTypeName(self),GetTypeName(key));
        return false;
        break;
    }
    return true;
}



bool SQVM::DeleteSlot(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &res)
{
    if (self._flags & SQOBJ_FLAG_IMMUTABLE) {
        Raise_Error(_SC("trying to modify immutable '%s'"),GetTypeName(self));
        return false;
    }

    switch(sq_type(self)) {
    case OT_TABLE:
    case OT_INSTANCE:
    case OT_USERDATA: {
        SQObjectPtr t;
        //bool handled = false;
        SQObjectPtr closure;
        if(_delegable(self)->_delegate && _delegable(self)->GetMetaMethod(this,MT_DELSLOT,closure)) {
            Push(self);Push(key);
            return CallMetaMethod(closure,MT_DELSLOT,2,res);
        }
        else {
            if(sq_type(self) == OT_TABLE) {
                if(_table(self)->Get(key,t)) {
                    _table(self)->Remove(key);
                }
                else {
                    Raise_IdxError((const SQObject &)key);
                    return false;
                }
            }
            else {
                Raise_Error(_SC("cannot delete a slot from %s"),GetTypeName(self));
                return false;
            }
        }
        res = t;
                }
        break;
    default:
        Raise_Error(_SC("attempt to delete a slot from a %s"),GetTypeName(self));
        return false;
    }
    return true;
}

bool SQVM::Call(SQObjectPtr &closure,SQInteger nparams,SQInteger stackbase,SQObjectPtr &outres,SQBool invoke_err_handler)
{
    switch(sq_type(closure)) {
    case OT_CLOSURE:
        return _debughook ?
            Execute<true>(closure, nparams, stackbase, outres, invoke_err_handler) :
            Execute<false>(closure, nparams, stackbase, outres, invoke_err_handler);
    case OT_NATIVECLOSURE:{
        bool dummy;
        return CallNative(_nativeclosure(closure), nparams, stackbase, outres, -1, dummy, dummy);
    }
    case OT_CLASS: {
        SQObjectPtr constr;
        SQObjectPtr temp;
        if (!CreateClassInstance(_class(closure),outres,constr))
            return false;
        SQObjectType ctype = sq_type(constr);
        if (ctype == OT_NATIVECLOSURE || ctype == OT_CLOSURE) {
            _stack[stackbase] = outres;
            return Call(constr,nparams,stackbase,temp,invoke_err_handler);
        }
        return true;
    }
    default:
        Raise_Error(_SC("attempt to call '%s'"), GetTypeName(closure));
        return false;
    }
}

bool SQVM::CallMetaMethod(SQObjectPtr &closure,SQMetaMethod SQ_UNUSED_ARG(mm),SQInteger nparams,SQObjectPtr &outres)
{
    //SQObjectPtr closure;

    _nmetamethodscall++;
    if(Call(closure, nparams, _top - nparams, outres, SQFalse)) {
        _nmetamethodscall--;
        Pop(nparams);
        return true;
    }
    _nmetamethodscall--;
    //}
    Pop(nparams);
    return false;
}

void SQVM::FindOuter(SQObjectPtr &target, SQObjectPtr *stackindex)
{
    SQOuter **pp = &_openouters;
    SQOuter *p;
    SQOuter *otr;

    while ((p = *pp) != NULL && p->_valptr >= stackindex) {
        if (p->_valptr == stackindex) {
            target = SQObjectPtr(p);
            return;
        }
        pp = &p->_next;
    }
    otr = SQOuter::Create(_ss(this), stackindex);
    otr->_next = *pp;
    otr->_idx  = (stackindex - _stack._vals);
    __ObjAddRef(otr);
    *pp = otr;
    target = SQObjectPtr(otr);
}

bool SQVM::EnterFrame(SQInteger newbase, SQInteger newtop, bool tailcall)
{
    if( !tailcall ) {
        if( _callsstacksize == _alloccallsstacksize ) {
            GrowCallStack();
        }
        ci = &_callsstack[_callsstacksize++];
        ci->_prevstkbase = (SQInt32)(newbase - _stackbase);
        ci->_prevtop = (SQInt32)(_top - _stackbase);
        ci->_etraps = 0;
        ci->_ncalls = 1;
        ci->_generator = NULL;
        ci->_root = SQFalse;
    }
    else {
        ci->_ncalls++;
    }

    _stackbase = newbase;
    _top = newtop;

    if(newtop + STACK_GROW_THRESHOLD > (SQInteger)_stack.size()) {
        if(!_nmetamethodscall) {
            _stack.resize(newtop + (STACK_GROW_THRESHOLD << 1));
            RelocateOuters();
        }
        // TODO: this exception should never occur and will be removed soon
        else if(newtop + MIN_STACK_OVERHEAD > (SQInteger)_stack.size()) {
            Raise_Error(_SC("stack overflow, cannot resize stack while in a metamethod, stack.size = %d, required = %d"),
                int(_stack.size()), int(newtop + MIN_STACK_OVERHEAD));
            return false;
        }
    }
    return true;
}


void SQVM::Release()
{
  sq_delete(_sharedstate->_alloc_ctx, this, SQVM);
}


void SQVM::LeaveFrame() {
    SQInteger last_top = _top;
    SQInteger last_stackbase = _stackbase;
    SQInteger css = --_callsstacksize;

    /* First clean out the call stack frame */
    ci->_closure.Null();
    _stackbase -= ci->_prevstkbase;
    _top = _stackbase + ci->_prevtop;
    ci = (css) ? &_callsstack[css-1] : NULL;

    if(_openouters) CloseOuters(&(_stack._vals[last_stackbase]));
    while (last_top >= _top) {
        _stack._vals[last_top--].Null();
    }
}

void SQVM::RelocateOuters()
{
    SQOuter *p = _openouters;
    while (p) {
        p->_valptr = _stack._vals + p->_idx;
        p = p->_next;
    }
}

void SQVM::CloseOuters(SQObjectPtr *stackindex) {
  SQOuter *p;
  while ((p = _openouters) != NULL && p->_valptr >= stackindex) {
    p->_value = *(p->_valptr);
    p->_valptr = &p->_value;
    _openouters = p->_next;
    __ObjRelease(p);
  }
}

void SQVM::Remove(SQInteger n) {
    n = (n >= 0)?n + _stackbase - 1:_top + n;
    for(SQInteger i = n; i < _top; i++){
        _stack[i] = _stack[i+1];
    }
    _stack[_top].Null();
    _top--;
}

void SQVM::Pop() {
    ValidateThreadAccess();

    _stack[--_top].Null();
}

void SQVM::Pop(SQInteger n) {
    ValidateThreadAccess();

    for(SQInteger i = 0; i < n; i++){
        _stack[--_top].Null();
    }
}

void SQVM::PushNull() {
    ValidateThreadAccess();

    _stack[_top++].Null();
}
void SQVM::Push(const SQObjectPtr &o) {
    ValidateThreadAccess();

    _stack[_top++] = o;
}
SQObjectPtr &SQVM::Top() { return _stack[_top-1]; }
SQObjectPtr &SQVM::PopGet() {
    ValidateThreadAccess();
    return _stack[--_top];
}
SQObjectPtr &SQVM::GetUp(SQInteger n) {
    ValidateThreadAccess();
    return _stack[_top+n];
}
SQObjectPtr &SQVM::GetAt(SQInteger n) {
    ValidateThreadAccess();
    return _stack[n];
}

#ifdef _DEBUG_DUMP
void SQVM::dumpstack(SQInteger stackbase,bool dumpall)
{
    SQInteger size=dumpall?_stack.size():_top;
    SQInteger n=0;
    printf(_SC("\n>>>>stack dump<<<<\n"));
    CallInfo &ci=_callsstack[_callsstacksize-1];
    printf(_SC("IP: %p\n"),ci._ip);
    printf(_SC("prev stack base: %d\n"),ci._prevstkbase);
    printf(_SC("prev top: %d\n"),ci._prevtop);
    for(SQInteger i=0;i<size;i++){
        SQObjectPtr &obj=_stack[i];
        if(stackbase==i)printf(_SC(">"));else printf(_SC(" "));
        printf(_SC("[" _PRINT_INT_FMT "]:"),n);
        switch(sq_type(obj)){
        case OT_FLOAT:          printf(_SC("FLOAT %.3f"),_float(obj));break;
        case OT_INTEGER:        printf(_SC("INTEGER " _PRINT_INT_FMT),_integer(obj));break;
        case OT_BOOL:           printf(_SC("BOOL %s"),_integer(obj)?"true":"false");break;
        case OT_STRING:         printf(_SC("STRING %s"),_stringval(obj));break;
        case OT_NULL:           printf(_SC("NULL"));  break;
        case OT_TABLE:          printf(_SC("TABLE %p[%p]"),_table(obj),_table(obj)->_delegate);break;
        case OT_ARRAY:          printf(_SC("ARRAY %p"),_array(obj));break;
        case OT_CLOSURE:        printf(_SC("CLOSURE [%p]"),_closure(obj));break;
        case OT_NATIVECLOSURE:  printf(_SC("NATIVECLOSURE"));break;
        case OT_USERDATA:       printf(_SC("USERDATA %p[%p]"),_userdataval(obj),_userdata(obj)->_delegate);break;
        case OT_GENERATOR:      printf(_SC("GENERATOR %p"),_generator(obj));break;
        case OT_THREAD:         printf(_SC("THREAD [%p]"),_thread(obj));break;
        case OT_USERPOINTER:    printf(_SC("USERPOINTER %p"),_userpointer(obj));break;
        case OT_CLASS:          printf(_SC("CLASS %p"),_class(obj));break;
        case OT_INSTANCE:       printf(_SC("INSTANCE %p"),_instance(obj));break;
        case OT_WEAKREF:        printf(_SC("WEAKREF %p"),_weakref(obj));break;
        default:
            assert(0);
            break;
        };
        printf(_SC("\n"));
        ++n;
    }
}



#endif
