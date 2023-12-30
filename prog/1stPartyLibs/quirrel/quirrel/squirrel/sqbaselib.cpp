/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "sqtable.h"
#include "sqarray.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqclass.h"
#include <sqstringlib.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#ifndef SQ_BASELIB_INVOKE_CB_ERR_HANDLER
#define SQ_BASELIB_INVOKE_CB_ERR_HANDLER SQTrue
#endif


static SQInteger delegable_getfuncinfos(HSQUIRRELVM v);
static SQInteger class_getfuncinfos(HSQUIRRELVM v);


#define SQ_CHECK_IMMUTABLE_SELF \
    if (sq_objflags(stack_get(v,1)) & SQOBJ_FLAG_IMMUTABLE) \
        return sq_throwerror(v, _SC("Cannot modify immutable object"));

#define SQ_CHECK_IMMUTABLE_OBJ(o) \
    if (sq_objflags(o) & SQOBJ_FLAG_IMMUTABLE) \
        return sq_throwerror(v, _SC("Cannot modify immutable object"));


static bool str2num(const SQChar *s,SQObjectPtr &res,SQInteger base)
{
    SQChar *end;
    const SQChar *e = s;
    bool iseintbase = base > 13; //to fix error converting hexadecimals with e like 56f0791e
    bool isfloat = false;
    SQChar c;
    while((c = *e) != _SC('\0'))
    {
        if (c == _SC('.') || (!iseintbase && (c == _SC('E') || c == _SC('e')))) { //e and E is for scientific notation
            isfloat = true;
            break;
        }
        e++;
    }
    if(isfloat){
        SQFloat r = SQFloat(strtod(s,&end));
        if(s == end) return false;
        res = r;
    }
    else{
        SQInteger r = SQInteger(scstrtol(s,&end,(int)base));
        if(s == end) return false;
        res = r;
    }
    return true;
}


static SQInteger get_allowed_args_count(SQObject &closure, SQInteger num_supported)
{
    if(sq_type(closure) == OT_CLOSURE) {
        return _closure(closure)->_function->_nparameters;
    }
    else if (sq_type(closure) == OT_NATIVECLOSURE) {
        SQInteger nParamsCheck = _nativeclosure(closure)->_nparamscheck;
        if (nParamsCheck > 0)
            return nParamsCheck;
        else // push all params when there is no check or only minimal count set
            return num_supported;
    }
    assert(false);
    return -1;
}

static SQInteger base_getroottable(HSQUIRRELVM v)
{
    v->Push(v->_roottable);
    return 1;
}

static SQInteger base_getconsttable(HSQUIRRELVM v)
{
    v->Push(_ss(v)->_consts);
    return 1;
}

SQInteger __sq_getcallstackinfos(HSQUIRRELVM v,SQInteger level)
{
    SQStackInfos si;
    SQInteger seq = 0;
    const SQChar *name = NULL;

    if (SQ_SUCCEEDED(sq_stackinfos(v, level, &si)))
    {
        const SQChar *fn = _SC("unknown");
        const SQChar *src = _SC("unknown");
        if(si.funcname)fn = si.funcname;
        if(si.source)src = si.source;
        sq_newtable(v);
        sq_pushstring(v, _SC("func"), -1);
        sq_pushstring(v, fn, -1);
        sq_newslot(v, -3, SQFalse);
        sq_pushstring(v, _SC("src"), -1);
        sq_pushstring(v, src, -1);
        sq_newslot(v, -3, SQFalse);
        sq_pushstring(v, _SC("line"), -1);
        sq_pushinteger(v, si.line);
        sq_newslot(v, -3, SQFalse);
        sq_pushstring(v, _SC("locals"), -1);
        sq_newtable(v);
        seq=0;
        while ((name = sq_getlocal(v, level, seq))) {
            sq_pushstring(v, name, -1);
            sq_push(v, -2);
            sq_newslot(v, -4, SQFalse);
            sq_pop(v, 1);
            seq++;
        }
        sq_newslot(v, -3, SQFalse);
        return 1;
    }

    return 0;
}

static SQInteger base_assert(HSQUIRRELVM v)
{
    if(SQVM::IsFalse(stack_get(v,2))){
        if (sq_gettop(v) > 2) {
            SQObjectPtr &arg = stack_get(v,3);
            SQInteger msgIdx = 0;
            if (sq_type(arg)==OT_CLOSURE || sq_type(arg)==OT_NATIVECLOSURE) {
                sq_pushroottable(v);
                if (SQ_SUCCEEDED(sq_call(v, 1, SQTrue, SQFalse)))
                    msgIdx = -1;
            }
            else
                msgIdx = 3;

            if (msgIdx!=0 && SQ_SUCCEEDED(sq_tostring(v,msgIdx))) {
                const SQChar *str = 0;
                if (SQ_SUCCEEDED(sq_getstring(v,-1,&str)))
                    return sq_throwerror(v, str);
            }
        }

        return sq_throwerror(v, _SC("assertion failed"));
    }
    return 0;
}

static SQInteger get_slice_params(HSQUIRRELVM v,SQInteger &sidx,SQInteger &eidx,SQObjectPtr &o)
{
    SQInteger top = sq_gettop(v);
    sidx=0;
    eidx=0;
    o=stack_get(v,1);
    if(top>1){
        SQObjectPtr &start=stack_get(v,2);
        if(sq_type(start)!=OT_NULL && sq_isnumeric(start)){
            sidx=tointeger(start);
        }
    }
    if(top>2){
        SQObjectPtr &end=stack_get(v,3);
        if(sq_isnumeric(end)){
            eidx=tointeger(end);
        }
    }
    else {
        eidx = sq_getsize(v,1);
    }
    return 1;
}

static SQInteger base_print(HSQUIRRELVM v, SQPRINTFUNCTION pf, bool newline)
{
    if(SQ_SUCCEEDED(sq_tostring(v,2)))
    {
        const SQChar *str;
        if(SQ_SUCCEEDED(sq_getstring(v,-1,&str))) {
            if(pf)
                pf(v, newline ? _SC("%s\n") : _SC("%s"), str);
            return 0;
        }
    }
    return SQ_ERROR;
}

static SQInteger base_print_newline(HSQUIRRELVM v)
{
    return base_print(v, _ss(v)->_printfunc, true);
}

static SQInteger base_print_(HSQUIRRELVM v)
{
    return base_print(v, _ss(v)->_printfunc, false);
}

static SQInteger base_error_newline(HSQUIRRELVM v)
{
    return base_print(v, _ss(v)->_errorfunc, true);
}

static SQInteger base_error_(HSQUIRRELVM v)
{
    return base_print(v, _ss(v)->_errorfunc, false);
}

static SQInteger base_compilestring(HSQUIRRELVM v)
{
    SQInteger nargs=sq_gettop(v);
    const SQChar *src=NULL,*name=_SC("unnamedbuffer");
    SQInteger size;
    sq_getstringandsize(v,2,&src,&size);
    if(nargs>2){
        sq_getstring(v,3,&name);
    }
    HSQOBJECT bindings;
    if (nargs>3)
        sq_getstackobj(v,4,&bindings);
    else
        sq_resetobject(&bindings);

    if(SQ_SUCCEEDED(sq_compilebuffer(v,src,size,name,SQFalse,&bindings)))
        return 1;
    else
        return SQ_ERROR;
}

static SQInteger base_newthread(HSQUIRRELVM v)
{
    SQObjectPtr &func = stack_get(v,2);
    SQInteger stksize = (_closure(func)->_function->_stacksize << 1) +2;
    HSQUIRRELVM newv = sq_newthread(v, (stksize < MIN_STACK_OVERHEAD + 2)? MIN_STACK_OVERHEAD + 2 : stksize);
    sq_move(newv,v,-2);
    return 1;
}

static SQInteger base_suspend(HSQUIRRELVM v)
{
    return sq_suspendvm(v);
}

static SQInteger base_array(HSQUIRRELVM v)
{
    SQInteger size = tointeger(stack_get(v,2));
    if (size < 0)
        return sq_throwerror(v, _SC("array size must be non-negative"));

    SQArray *a;
    if(sq_gettop(v) > 2) {
        a = SQArray::Create(_ss(v),0);
        a->Resize(size,stack_get(v,3));
    }
    else {
        a = SQArray::Create(_ss(v),size);
    }
    v->Push(a);
    return 1;
}

static SQInteger base_type(HSQUIRRELVM v)
{
    SQObjectPtr &o = stack_get(v,2);
    v->Push(SQString::Create(_ss(v),GetTypeName(o),-1));
    return 1;
}

static SQInteger base_callee(HSQUIRRELVM v)
{
    if(v->_callsstacksize > 1)
    {
        v->Push(v->_callsstack[v->_callsstacksize - 2]._closure);
        return 1;
    }
    return sq_throwerror(v,_SC("no closure in the calls stack"));
}

static SQInteger base_freeze(HSQUIRRELVM v)
{
    return SQ_SUCCEEDED(sq_freeze(v, 2)) ? 1 : SQ_ERROR;
}

static SQInteger base_getobjflags(HSQUIRRELVM v)
{
    sq_pushinteger(v, sq_objflags(stack_get(v, 2)));
    return 1;
}

static SQInteger base_call_method_impl(HSQUIRRELVM v, bool safe)
{
    SQChar message[1024] = {0};
    int32_t nArgs = sq_gettop(v);

    if (nArgs < 3)
    {
        if (safe)
        {
            v->Pop(nArgs);
            v->PushNull();
            return 1;
        }
        snprintf(message, sizeof message, _SC("'__call_method(...)' recieves at least 2 arguments (%d), object to method on and method's name"), nArgs);
        return sq_throwerror(v, message);
    }

    SQObject obj = stack_get(v, 2);
    SQObject name = stack_get(v, 3);

    if (!sq_isstring(name))
    {
        if (safe)
        {
            v->Pop(nArgs);
            v->PushNull();
            return 1;
        }
        snprintf(message, sizeof message, _SC("'__call_method(...)' expects 2nd argument to be string, actual '%s'"), IdType2Name(sq_type(name)));
        return sq_throwerror(v, message);
    }

    SQObject delegates;
    SQObjectType type = sq_type(obj);
    switch (type)
    {
    case OT_INTEGER:
    case OT_FLOAT:
    case OT_BOOL:
        delegates = _ss(v)->_number_default_delegate;
        break;
    case OT_STRING:
        delegates = _ss(v)->_string_default_delegate;
        break;
    case OT_TABLE:
        delegates = _ss(v)->_table_default_delegate;
        break;
    case OT_ARRAY:
        delegates = _ss(v)->_array_default_delegate;
        break;
    case OT_USERDATA:
        delegates = _ss(v)->_userdata_default_delegate;
        break;
    case OT_CLOSURE:
    case OT_NATIVECLOSURE:
        delegates = _ss(v)->_closure_default_delegate;
        break;
    case OT_GENERATOR:
        delegates = _ss(v)->_generator_default_delegate;
        break;
    case OT_THREAD:
        delegates = _ss(v)->_thread_default_delegate;
        break;
    case OT_CLASS:
        delegates = _ss(v)->_class_default_delegate;
        break;
    case OT_INSTANCE:
        delegates = _ss(v)->_instance_default_delegate;
        break;
    case OT_WEAKREF:
        delegates = _ss(v)->_weakref_default_delegate;
        break;
    default:
        if (safe)
        {
            v->Pop(nArgs);
            v->PushNull();
            return 1;
        }
        snprintf(message, sizeof message, _SC("unsupported object type '%s' as receiver in '__call_method(...)' function"), IdType2Name(type));
        return sq_throwerror(v, message);
    }

    assert(sq_istable(delegates));

    SQObjectPtr callee;
    if (!_table(delegates)->Get(name, callee))
    {
        if (safe)
        {
            v->Pop(nArgs);
            v->PushNull();
            return 1;
        }
        else
        {
            snprintf(message, sizeof message, _SC("function '%s' not found in '%s' delegates"), _stringval(name), IdType2Name(type));
            return sq_throwerror(v, message);
        }
    }

    assert(sq_isnativeclosure(callee));

    sqvector<SQObjectPtr> args(_ss(v)->_alloc_ctx);

    args.reserve(nArgs - 2);
    args.push_back(obj);

    for (int32_t i = 4; i <= nArgs; ++i) // skip reciever + name
    {
        SQObject arg = stack_get(v, i);
        args.push_back(arg);
    }

    for (int32_t i = 0; i < args.size(); ++i)
    {
        v->Push(args[i]);
    }

    bool s, t;
    SQObjectPtr ret;
    if (!v->CallNative(_nativeclosure(callee), nArgs - 2, v->_stackbase + nArgs, ret, -1, s, t))
    {
        return SQ_ERROR;
    }

    v->Pop(nArgs);
    v->Push(ret);
    return 1;
}

static SQInteger base_call_method(HSQUIRRELVM v)
{
    return base_call_method_impl(v, false);
}

static SQInteger base_call_method_safe(HSQUIRRELVM v)
{
  return base_call_method_impl(v, true);
}

static const SQRegFunction base_funcs[]={
    //generic
    {_SC("getroottable"),base_getroottable,1, NULL},
    {_SC("getconsttable"),base_getconsttable,1, NULL},
    {_SC("assert"),base_assert, -2, NULL},
    {_SC("print"),base_print_, 2, NULL},
    {_SC("println"),base_print_newline, 2, NULL},
    {_SC("error"),base_error_, 2, NULL},
    {_SC("errorln"),base_error_newline, 2, NULL},
    {_SC("compilestring"),base_compilestring,-2, _SC(".sst|o")},
    {_SC("newthread"),base_newthread,2, _SC(".c")},
    {_SC("suspend"),base_suspend,-1, NULL},
    {_SC("array"),base_array,-2, _SC(".n")},
    {_SC("type"),base_type,2, NULL},
    {_SC("callee"),base_callee,0,NULL},
    {_SC("freeze"),base_freeze,2,NULL},
    {_SC("getobjflags"), base_getobjflags,2,NULL},
    {_SC("call_type_method"),base_call_method,-1,NULL},
    {_SC("call_type_method_safe"),base_call_method_safe,-1,NULL},
    {NULL,(SQFUNCTION)0,0,NULL}
};

SQRESULT sq_registerbaselib(HSQUIRRELVM v)
{
    SQInteger i=0;
    while(base_funcs[i].name!=0) {
        sq_pushstring(v,base_funcs[i].name,-1);
        sq_newclosure(v,base_funcs[i].f,0);
        sq_setnativeclosurename(v,-1,base_funcs[i].name);
        sq_setparamscheck(v,base_funcs[i].nparamscheck,base_funcs[i].typemask);
        sq_newslot(v,-3, SQFalse);
        i++;
    }
    return SQ_OK;
}

void sq_base_register(HSQUIRRELVM v)
{
    sq_pushroottable(v);
    sq_registerbaselib(v);
    sq_pop(v,1);

    sq_pushconsttable(v);
    sq_pushstring(v, _SC("SQOBJ_FLAG_IMMUTABLE"), -1);
    sq_pushinteger(v, SQOBJ_FLAG_IMMUTABLE);
    sq_newslot(v, -3, SQFalse);
    sq_pop(v,1);
}

static SQInteger default_delegate_len(HSQUIRRELVM v)
{
    v->Push(SQInteger(sq_getsize(v,1)));
    return 1;
}

static SQInteger default_delegate_tofloat(HSQUIRRELVM v)
{
    SQObjectPtr &o=stack_get(v,1);
    switch(sq_type(o)){
    case OT_STRING:{
        SQObjectPtr res;
        if(str2num(_stringval(o),res,10)){
            v->Push(SQObjectPtr(tofloat(res)));
            break;
        }}
        return sq_throwerror(v, _SC("cannot convert the string"));
    case OT_INTEGER:case OT_FLOAT:
        v->Push(SQObjectPtr(tofloat(o)));
        break;
    case OT_BOOL:
        v->Push(SQObjectPtr((SQFloat)(_integer(o)?1:0)));
        break;
    default:
        v->PushNull();
        break;
    }
    return 1;
}

static SQInteger default_delegate_tointeger(HSQUIRRELVM v)
{
    SQObjectPtr &o=stack_get(v,1);
    SQInteger base = 10;
    if(sq_gettop(v) > 1) {
        sq_getinteger(v,2,&base);
    }
    switch(sq_type(o)){
    case OT_STRING:{
        SQObjectPtr res;
        if(str2num(_stringval(o),res,base)){
            v->Push(SQObjectPtr(tointeger(res)));
            break;
        }}
        return sq_throwerror(v, _SC("cannot convert the string"));
        break;
    case OT_INTEGER:case OT_FLOAT:
        v->Push(SQObjectPtr(tointeger(o)));
        break;
    case OT_BOOL:
        v->Push(SQObjectPtr(_integer(o)?(SQInteger)1:(SQInteger)0));
        break;
    default:
        v->PushNull();
        break;
    }
    return 1;
}

static SQInteger default_delegate_tostring(HSQUIRRELVM v)
{
    if(SQ_FAILED(sq_tostring(v,1)))
        return SQ_ERROR;
    return 1;
}

static SQInteger obj_delegate_weakref(HSQUIRRELVM v)
{
    sq_weakref(v,1);
    return 1;
}

static SQInteger obj_clear(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;
    return SQ_SUCCEEDED(sq_clear(v,-1)) ? 1 : SQ_ERROR;
}


static SQInteger number_delegate_tochar(HSQUIRRELVM v)
{
    SQObject &o=stack_get(v,1);
    SQChar c = (SQChar)tointeger(o);
    v->Push(SQString::Create(_ss(v),(const SQChar *)&c,1));
    return 1;
}


static SQInteger container_update(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;

    SQInteger top = sq_gettop(v);
    for (int arg=2; arg<=top; ++arg) {
        SQObjectType argType = sq_gettype(v, arg);
        if (!(argType & (_RT_TABLE | _RT_CLASS | _RT_INSTANCE))) {
            v->Raise_ParamTypeError(arg, _RT_TABLE | _RT_CLASS | _RT_INSTANCE, argType);
            return SQ_ERROR;
        }

        sq_pushnull(v);
        while (SQ_SUCCEEDED(sq_next(v, arg))) {
            sq_newslot(v, 1, false);
        }
        sq_pop(v, 1); // pops the iterator
    }
    sq_push(v, 1);
    return 1;
}


static SQInteger container_merge(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v,1);

    SQInteger top = sq_gettop(v);
    switch (sq_type(o)) {
        case OT_TABLE:
            sq_newtableex(v, sq_getsize(v, 1));
            break;
        case OT_CLASS:
            sq_newclass(v, SQFalse);
            break;
        default:
            return sq_throwerror(v, _SC("merge() only works on tables and classes"));
    }

    for (int arg=1; arg<=top; ++arg) {
        SQObjectType argType = sq_gettype(v, arg);
        if (!(argType & (_RT_TABLE | _RT_CLASS | _RT_INSTANCE))) {
            v->Raise_ParamTypeError(arg, _RT_TABLE | _RT_CLASS | _RT_INSTANCE, argType);
            return SQ_ERROR;
        }

        sq_pushnull(v);
        while (SQ_SUCCEEDED(sq_next(v, arg))) {
            sq_newslot(v, top+1, false);
        }
        sq_pop(v, 1); // pops the iterator
    }
    return 1;
}


static SQInteger container_each(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v,1);
    SQObject &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 4);

    sq_pushnull(v);
    while (SQ_SUCCEEDED(sq_next(v, 1))) {
        SQInteger iterTop = sq_gettop(v);
        v->Push(closure);
        v->Push(o);
        v->Push(stack_get(v, iterTop));
        if (nArgs >= 3)
            v->Push(stack_get(v, iterTop-1));
        if (nArgs >= 4)
            v->Push(o);

        if (SQ_FAILED(sq_call(v,nArgs,SQFalse,SQ_BASELIB_INVOKE_CB_ERR_HANDLER))) {
            return SQ_ERROR;
        }
        v->Pop(3);
    }
    sq_pop(v, 1); // pops the iterator

    return 0;
}


static SQInteger container_findindex(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v,1);
    SQObject &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 4);

    sq_pushnull(v);
    while (SQ_SUCCEEDED(sq_next(v, 1))) {
        SQInteger iterTop = sq_gettop(v);
        v->Push(closure);
        v->Push(o);
        v->Push(stack_get(v, iterTop));
        if (nArgs >= 3)
            v->Push(stack_get(v, iterTop-1));
        if (nArgs >= 4)
            v->Push(o);

        if (SQ_FAILED(sq_call(v,nArgs,SQTrue,SQ_BASELIB_INVOKE_CB_ERR_HANDLER))) {
            return SQ_ERROR;
        }
        if (!v->IsFalse(stack_get(v, -1))) {
            v->Push(stack_get(v, iterTop-1));
            return 1;
        }
        v->Pop(4);
    }
    sq_pop(v, 1); // pops the iterator

    return 0;
}

static SQInteger container_findvalue(HSQUIRRELVM v)
{
    if (sq_gettop(v) > 3)
        return sq_throwerror(v, _SC("Too many arguments for findvalue()"));

    SQObject &o = stack_get(v,1);
    SQObject &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 4);

    sq_pushnull(v);
    while (SQ_SUCCEEDED(sq_next(v, 1))) {
        SQInteger iterTop = sq_gettop(v);
        v->Push(closure);
        v->Push(o);
        v->Push(stack_get(v, iterTop));
        if (nArgs >= 3)
            v->Push(stack_get(v, iterTop-1));
        if (nArgs >= 4)
            v->Push(o);

        if (SQ_FAILED(sq_call(v,nArgs,SQTrue,SQ_BASELIB_INVOKE_CB_ERR_HANDLER))) {
            return SQ_ERROR;
        }
        if (!v->IsFalse(stack_get(v, -1))) {
            v->Push(stack_get(v, iterTop));
            return 1;
        }
        v->Pop(4);
    }
    sq_pop(v, 1); // pops the iterator

    return sq_gettop(v) > 2 ? 1 : 0;
}

/////////////////////////////////////////////////////////////////
//TABLE DEFAULT DELEGATE

static SQInteger table_rawdelete(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;

    if(SQ_FAILED(sq_rawdeleteslot(v,1,SQTrue)))
        return SQ_ERROR;
    return 1;
}


static SQInteger container_rawexists(HSQUIRRELVM v)
{
    if(SQ_SUCCEEDED(sq_rawget(v,-2))) {
        sq_pushbool(v,SQTrue);
        return 1;
    }
    sq_pushbool(v,SQFalse);
    return 1;
}

static SQInteger container_rawset(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;

    return SQ_SUCCEEDED(sq_rawset(v,-3)) ? 1 : SQ_ERROR;
}


static SQInteger container_rawget(HSQUIRRELVM v)
{
    return SQ_SUCCEEDED(sq_rawget(v,-2))?1:SQ_ERROR;
}


static SQInteger table_filter(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v,1);
    SQTable *tbl = _table(o);
    SQObject &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 4);

    SQObjectPtr ret = SQTable::Create(_ss(v),0);

    SQObjectPtr itr, key, val;
    SQInteger nitr;
    while((nitr = tbl->Next(false, itr, key, val)) != -1) {
        itr = (SQInteger)nitr;

        v->Push(o);
        v->Push(val);
        if (nArgs >= 3)
            v->Push(key);
        if (nArgs >= 4)
            v->Push(o);
        if(SQ_FAILED(sq_call(v,nArgs,SQTrue,SQ_BASELIB_INVOKE_CB_ERR_HANDLER))) {
            return SQ_ERROR;
        }
        if(!SQVM::IsFalse(v->GetUp(-1))) {
            _table(ret)->NewSlot(key, val);
        }
        v->Pop();
    }

    v->Push(ret);
    return 1;
}

#define TABLE_TO_ARRAY_FUNC(_funcname_,_valname_) static SQInteger _funcname_(HSQUIRRELVM v) \
{ \
    SQObject &o = stack_get(v, 1); \
    SQTable *t = _table(o); \
    SQObjectPtr itr, key, val; \
    SQInteger nitr, n = 0; \
    SQInteger nitems = t->CountUsed(); \
    SQArray *a = SQArray::Create(_ss(v), nitems); \
    if (nitems) { \
        while ((nitr = t->Next(false, itr, key, val)) != -1) { \
            itr = (SQInteger)nitr; \
            a->Set(n, _valname_); \
            n++; \
        } \
    } \
    v->Push(a); \
    return 1; \
}

TABLE_TO_ARRAY_FUNC(table_keys, key)
TABLE_TO_ARRAY_FUNC(table_values, val)


static SQInteger table_to_pairs(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v, 1);
    SQTable *t = _table(o);
    SQObjectPtr itr, key, val;
    SQInteger nitr, n = 0;
    SQInteger nitems = t->CountUsed();
    SQArray *result = SQArray::Create(_ss(v), nitems);
    if (nitems) {
        while ((nitr = t->Next(false, itr, key, val)) != -1) {
            itr = (SQInteger)nitr;
            SQArray *item = SQArray::Create(_ss(v), 2);
            item->Set(0, key);
            item->Set(1, val);
            result->Set(n, item);
            n++;
        }
    }
    v->Push(result);
    return 1;
}


static SQInteger pairs_to_table(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v, 1);
    SQArray *arr = _array(o);
    SQInteger count = arr->Size();
    SQTable *result = SQTable::Create(_ss(v), count);
    for (SQInteger i = 0; i < count; i++)
    {
        SQObjectPtr item;
        SQObjectPtr key;
        SQObjectPtr val;
        if (arr->Get(i, item) && sq_isarray(item) &&
            _array(item)->Get(0, key) && _array(item)->Get(1, val))
        {
             result->NewSlot(key, val);
        }
        else
        {
            return sq_throwerror(v, _SC("totable() expected array of pairs [[key, value], ...]"));
        }
    }

    v->Push(result);
    return 1;
}


static SQInteger __map_table(SQTable *dest, SQTable *src, HSQUIRRELVM v) {
    SQObjectPtr temp;
    SQObject &closure = stack_get(v, 2);
    v->Push(closure);

    SQInteger nArgs = get_allowed_args_count(closure, 4);

    SQObjectPtr itr, key, val;
    SQInteger nitr;
    while ((nitr = src->Next(false, itr, key, val)) != -1) {
        itr = (SQInteger)nitr;
        v->Push(src);
        v->Push(val);
        if (nArgs >= 3)
            v->Push(key);
        if (nArgs >= 4)
            v->Push(src);
        if (SQ_FAILED(sq_call(v, nArgs, SQTrue, SQ_BASELIB_INVOKE_CB_ERR_HANDLER))) {
            if (sq_isnull(v->_lasterror))
                continue;
            return SQ_ERROR;
        }

        dest->NewSlot(key, v->GetUp(-1));
        v->Pop();
    }

    v->Pop();
    return 0;
}

static SQInteger table_map(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v, 1);
    SQObjectPtr ret = SQTable::Create(_ss(v), _table(o)->CountUsed());
    if(SQ_FAILED(__map_table(_table(ret), _table(o), v)))
        return SQ_ERROR;
    v->Push(ret);
    return 1;
}


static SQInteger table_reduce(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v,1);
    SQTable *tbl = _table(o);
    SQObject &closure = stack_get(v, 2);

    bool gotAccum = false;
    SQObjectPtr accum;

    SQInteger nArgs = get_allowed_args_count(closure, 5);

    if (sq_gettop(v) > 2) {
        accum = stack_get(v, 3);
        gotAccum = true;
    }

    SQObjectPtr itr, key, val;
    SQInteger nitr;
    while ((nitr = tbl->Next(false, itr, key, val)) != -1) {
        itr = (SQInteger)nitr;

        if (!gotAccum) {
            accum = val;
            gotAccum = true;
        } else {
            v->Push(closure);
            v->Push(o);
            v->Push(accum);
            v->Push(val);
            if (nArgs >= 4)
                v->Push(key);
            if (nArgs >= 5)
                v->Push(tbl);

            if(SQ_FAILED(sq_call(v,nArgs,SQTrue,SQ_BASELIB_INVOKE_CB_ERR_HANDLER))) {
                return SQ_ERROR;
            }

            accum = v->GetUp(-1);
            v->Pop(2);
        }
    }

    if (!gotAccum)
        return 0;

    v->Push(accum);
    return 1;
}


static SQInteger obj_clone(HSQUIRRELVM vm) {
    SQObjectPtr self = vm->PopGet();
    SQObjectPtr target;

    if (!vm->Clone(self, target)) {
      return SQ_ERROR;
    }

    vm->Push(target);
    return 1;
}

static SQInteger obj_is_frozen(HSQUIRRELVM vm) {
  SQObjectPtr self = vm->PopGet();
  SQObjectPtr target;

  sq_pushbool(vm, (self._flags & SQOBJ_FLAG_IMMUTABLE) != 0);

  return 1;
}

const SQRegFunction SQSharedState::_table_default_delegate_funcz[]={
    {_SC("len"),default_delegate_len,1, _SC("t")},
    {_SC("rawget"),container_rawget,2, _SC("t")},
    {_SC("rawset"),container_rawset,3, _SC("t")},
    {_SC("rawdelete"),table_rawdelete,2, _SC("t")},
    {_SC("rawin"),container_rawexists,2, _SC("t")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("clear"),obj_clear,1, _SC(".")},
    {_SC("map"),table_map,2, _SC("tc")},
    {_SC("filter"),table_filter,2, _SC("tc")},
    {_SC("reduce"),table_reduce, -2, _SC("tc")},
    {_SC("getfuncinfos"),delegable_getfuncinfos,1, _SC("t")},
    {_SC("each"),container_each,2, _SC("tc")},
    {_SC("findindex"),container_findindex,2, _SC("tc")},
    {_SC("findvalue"),container_findvalue,-2, _SC("tc.")},
    {_SC("keys"),table_keys,1, _SC("t") },
    {_SC("values"),table_values,1, _SC("t") },
    {_SC("__update"),container_update, -2, _SC("t|yt|y|x") },
    {_SC("__merge"),container_merge, -2, _SC("t|yt|y|x") },
    {_SC("topairs"),table_to_pairs, 1, _SC("t") },
    {_SC("clone"),obj_clone, 1, _SC(".") },
    {_SC("is_frozen"),obj_is_frozen, 1, _SC(".") },
    {NULL,(SQFUNCTION)0,0,NULL}
};

//ARRAY DEFAULT DELEGATE///////////////////////////////////////

static SQInteger array_append(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;

    SQArray *arr = _array(stack_get(v, 1));
    SQInteger nitems = sq_gettop(v)-1;
    SQInteger offs = arr->Size();
    arr->Resize(offs + nitems);
    for (SQInteger i=0; i<nitems; ++i)
        arr->Set(offs+i, stack_get(v, 2+i));

    v->Push(stack_get(v, 1));
    return 1;
}

static SQInteger array_extend(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;

    SQArray *arr = _array(stack_get(v, 1));
    SQInteger n = sq_gettop(v)-1;
    for (SQInteger i=0; i<n; ++i) {
        SQObject &o=stack_get(v,2+i);
        if (sq_type(o) != OT_ARRAY)
            return sq_throwerror(v, _SC("only arrays can be used to extend array"));
        arr->Extend(_array(o));
    }
    sq_pop(v,n);
    return 1;
}

static SQInteger array_reverse(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;

    return SQ_SUCCEEDED(sq_arrayreverse(v,-1)) ? 1 : SQ_ERROR;
}

static SQInteger array_pop(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;

    return SQ_SUCCEEDED(sq_arraypop(v,1,SQTrue))?1:SQ_ERROR;
}

static SQInteger array_top(HSQUIRRELVM v)
{
    SQObject &o=stack_get(v,1);
    if(_array(o)->Size()>0){
        v->Push(_array(o)->Top());
        return 1;
    }
    else return sq_throwerror(v,_SC("top() on a empty array"));
}

static SQInteger array_insert(HSQUIRRELVM v)
{
    SQObject &o=stack_get(v,1);

    SQ_CHECK_IMMUTABLE_OBJ(o);

    SQObject &idx=stack_get(v,2);
    SQObject &val=stack_get(v,3);
    if(!_array(o)->Insert(tointeger(idx),val))
        return sq_throwerror(v,_SC("index out of range"));
    sq_pop(v,2);
    return 1;
}

static SQInteger array_remove(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v, 1);
    SQ_CHECK_IMMUTABLE_OBJ(o);

    SQObject &idx = stack_get(v, 2);
    if(!sq_isnumeric(idx)) return sq_throwerror(v, _SC("wrong type"));
    SQObjectPtr val;
    if(_array(o)->Get(tointeger(idx), val)) {
        _array(o)->Remove(tointeger(idx));
        v->Push(val);
        return 1;
    }
    return sq_throwerror(v, _SC("idx out of range"));
}

static SQInteger array_resize(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v, 1);
    SQ_CHECK_IMMUTABLE_OBJ(o);

    SQObject &nsize = stack_get(v, 2);
    SQObjectPtr fill;
    if(sq_isnumeric(nsize)) {
        SQInteger sz = tointeger(nsize);
        if (sz<0)
          return sq_throwerror(v, _SC("resizing to negative length"));

        if(sq_gettop(v) > 2)
            fill = stack_get(v, 3);
        _array(o)->Resize(sz,fill);
        sq_settop(v, 1);
        return 1;
    }
    return sq_throwerror(v, _SC("size must be a number"));
}

static SQInteger __map_array(SQArray *dest,SQArray *src,HSQUIRRELVM v, bool append) {
    SQObjectPtr temp;
    SQInteger size = src->Size();
    SQObject &closure = stack_get(v, 2);
    v->Push(closure);

    SQInteger nArgs = get_allowed_args_count(closure, 4);

    for(SQInteger n = 0; n < size; n++) {
        src->Get(n,temp);
        v->Push(src);
        v->Push(temp);
        if (nArgs >= 3)
            v->Push(SQObjectPtr(n));
        if (nArgs >= 4)
            v->Push(src);
        if(SQ_FAILED(sq_call(v,nArgs,SQTrue,SQ_BASELIB_INVOKE_CB_ERR_HANDLER))) {
            if (append && sq_isnull(v->_lasterror))
                continue;
            return SQ_ERROR;
        }
        if (append)
            dest->Append(v->GetUp(-1));
        else
            dest->Set(n,v->GetUp(-1));
        v->Pop();
    }
    v->Pop();
    return 0;
}

static SQInteger array_map(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v,1);
    SQInteger size = _array(o)->Size();
    SQArray *dest = SQArray::Create(_ss(v),0);
    dest->Reserve(size);
    SQObjectPtr ret = dest;
    if(SQ_FAILED(__map_array(dest,_array(o),v,true)))
        return SQ_ERROR;
    dest->ShrinkIfNeeded();
    v->Push(ret);
    return 1;
}

static SQInteger array_apply(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v,1);
    SQ_CHECK_IMMUTABLE_OBJ(o);

    if(SQ_FAILED(__map_array(_array(o),_array(o),v,false)))
        return SQ_ERROR;
    sq_pop(v,1);
    return 1;
}

static SQInteger array_reduce(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v,1);
    SQArray *a = _array(o);
    SQInteger size = a->Size();
    SQObjectPtr res;
    SQInteger iterStart;
    if (sq_gettop(v)>2) {
        res = stack_get(v,3);
        iterStart = 0;
    } else if (size==0) {
        return 0;
    } else {
        a->Get(0,res);
        iterStart = 1;
    }

    SQObject &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 5);

    if (size > iterStart) {
        SQObjectPtr other;
        v->Push(closure);
        for (SQInteger n = iterStart; n < size; n++) {
            a->Get(n,other);
            v->Push(o);
            v->Push(res);
            v->Push(other);
            if (nArgs >= 4)
                v->Push(SQObjectPtr(n));
            if (nArgs >= 5)
                v->Push(o);
            if(SQ_FAILED(sq_call(v,nArgs,SQTrue,SQ_BASELIB_INVOKE_CB_ERR_HANDLER))) {
                return SQ_ERROR;
            }
            res = v->GetUp(-1);
            v->Pop();
        }
        v->Pop();
    }
    v->Push(res);
    return 1;
}

static SQInteger array_filter(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v,1);
    SQArray *a = _array(o);
    SQObject &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 4);

    SQObjectPtr ret = SQArray::Create(_ss(v),0);
    SQInteger size = a->Size();
    SQObjectPtr val;
    for(SQInteger n = 0; n < size; n++) {
        a->Get(n,val);
        v->Push(o);
        v->Push(val);
        if (nArgs >= 3)
            v->Push(n);
        if (nArgs >= 4)
            v->Push(o);
        if(SQ_FAILED(sq_call(v,nArgs,SQTrue,SQ_BASELIB_INVOKE_CB_ERR_HANDLER))) {
            return SQ_ERROR;
        }
        if(!SQVM::IsFalse(v->GetUp(-1))) {
            _array(ret)->Append(val);
        }
        v->Pop();
    }
    v->Push(ret);
    return 1;
}


static SQInteger _push_scan_index(HSQUIRRELVM v, SQInteger index)
{
    if (index >= 0) {
        sq_pushinteger(v, index);
        return 1;
    } else
        return 0;
}


static SQInteger _push_scan_found_flag(HSQUIRRELVM v, SQInteger index)
{
    sq_pushbool(v, index>=0);
    return 1;
}


static SQInteger _array_scan_for_value(HSQUIRRELVM v, SQInteger (*push_result)(HSQUIRRELVM v, SQInteger index))
{
    SQObject &o = stack_get(v,1);
    SQObjectPtr &val = stack_get(v,2);
    SQArray *a = _array(o);
    SQInteger size = a->Size();
    SQObjectPtr temp;
    for(SQInteger n = 0; n < size; n++) {
        bool res = false;
        a->Get(n,temp);
        if(SQVM::IsEqual(temp,val,res) && res)
            return push_result(v, n);
    }
    return push_result(v, -1);
}


static SQInteger array_indexof(HSQUIRRELVM v)
{
    return _array_scan_for_value(v, _push_scan_index);
}


static SQInteger array_contains(HSQUIRRELVM v)
{
    return _array_scan_for_value(v, _push_scan_found_flag);
}


static bool _sort_compare(HSQUIRRELVM v, SQArray *arr, SQObjectPtr &a,SQObjectPtr &b,SQInteger func,SQInteger &ret)
{
    if(func < 0) {
        if(!v->ObjCmp(a,b,ret)) return false;
    }
    else {
        SQInteger top = sq_gettop(v);
        sq_push(v, func);
        sq_pushroottable(v);
        v->Push(a);
        v->Push(b);
        SQObjectPtr *valptr = arr->_values._vals;
        SQUnsignedInteger precallsize = arr->_values.size();
        if(SQ_FAILED(sq_call(v, 3, SQTrue, SQFalse))) {
            if(!sq_isstring( v->_lasterror))
                v->Raise_Error(_SC("compare func failed"));
            return false;
        }
        if(SQ_FAILED(sq_getinteger(v, -1, &ret))) {
            v->Raise_Error(_SC("numeric value expected as return value of the compare function"));
            return false;
        }
        if (precallsize != arr->_values.size() || valptr != arr->_values._vals) {
            v->Raise_Error(_SC("array resized during sort operation"));
            return false;
        }
        sq_settop(v, top);
        return true;
    }
    return true;
}

static bool _hsort_sift_down(HSQUIRRELVM v,SQArray *arr, SQInteger root, SQInteger bottom, SQInteger func)
{
    SQInteger maxChild;
    SQInteger done = 0;
    SQInteger ret;
    SQInteger root2;
    while (((root2 = root * 2) <= bottom) && (!done))
    {
        if (root2 == bottom) {
            maxChild = root2;
        }
        else {
            if(!_sort_compare(v,arr,arr->_values[root2],arr->_values[root2 + 1],func,ret))
                return false;
            if (ret > 0) {
                maxChild = root2;
            }
            else {
                maxChild = root2 + 1;
            }
        }

        if(!_sort_compare(v,arr,arr->_values[root],arr->_values[maxChild],func,ret))
            return false;
        if (ret < 0) {
            if (root == maxChild) {
                v->Raise_Error(_SC("inconsistent compare function"));
                return false; // We'd be swapping ourselve. The compare function is incorrect
            }

            _Swap(arr->_values[root],arr->_values[maxChild]);
            root = maxChild;
        }
        else {
            done = 1;
        }
    }
    return true;
}

static bool _hsort(HSQUIRRELVM v,SQObjectPtr &arr, SQInteger SQ_UNUSED_ARG(l), SQInteger SQ_UNUSED_ARG(r),SQInteger func)
{
    SQArray *a = _array(arr);
    SQInteger i;
    SQInteger array_size = a->Size();
    for (i = (array_size / 2); i >= 0; i--) {
        if(!_hsort_sift_down(v,a, i, array_size - 1,func)) return false;
    }

    for (i = array_size-1; i >= 1; i--)
    {
        _Swap(a->_values[0],a->_values[i]);
        if(!_hsort_sift_down(v,a, 0, i-1,func)) return false;
    }
    return true;
}

static SQInteger array_sort(HSQUIRRELVM v)
{
    SQInteger func = -1;
    SQObjectPtr &o = stack_get(v,1);
    SQ_CHECK_IMMUTABLE_OBJ(o);

    if(_array(o)->Size() > 1) {
        if(sq_gettop(v) == 2) func = 2;
        if(!_hsort(v, o, 0, _array(o)->Size()-1, func))
            return SQ_ERROR;

    }
    sq_settop(v,1);
    return 1;
}


static SQInteger clamp_int(SQInteger v, SQInteger minv, SQInteger maxv)
{
  return (v < minv) ? minv : (v > maxv) ? maxv : v;
}

static SQInteger array_slice(HSQUIRRELVM v)
{
    SQInteger sidx,eidx;
    SQObjectPtr o;
    if(get_slice_params(v,sidx,eidx,o)==-1)return -1;
    SQInteger alen = _array(o)->Size();

    sidx = clamp_int((sidx < 0) ? (alen + sidx) : sidx, 0, alen);
    eidx = clamp_int((eidx < 0) ? (alen + eidx) : eidx, 0, alen);

    if (alen == 0 || eidx <= sidx)
    {
      SQArray *arr = SQArray::Create(_ss(v), 0);
      v->Push(arr);
      return 1;
    }

    SQArray *arr=SQArray::Create(_ss(v),eidx-sidx);
    SQObjectPtr t;
    SQInteger count=0;
    for(SQInteger i=sidx;i<eidx;i++){
        _array(o)->Get(i,t);
        arr->Set(count++,t);
    }
    v->Push(arr);
    return 1;

}

static SQInteger array_replace(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;

    SQArray *dst = _array(stack_get(v, 1));
    SQArray *src = _array(stack_get(v, 2));
    dst->_values.copy(src->_values);
    dst->ShrinkIfNeeded();
    v->Pop(1);
    return 1;
}

const SQRegFunction SQSharedState::_array_default_delegate_funcz[]={
    {_SC("len"),default_delegate_len,1, _SC("a")},
    {_SC("append"),array_append,-2, _SC("a")},
    {_SC("extend"),array_extend,-2, _SC("aaaaaaaa")},
    {_SC("pop"),array_pop,1, _SC("a")},
    {_SC("top"),array_top,1, _SC("a")},
    {_SC("insert"),array_insert,3, _SC("an")},
    {_SC("remove"),array_remove,2, _SC("an")},
    {_SC("resize"),array_resize,-2, _SC("an")},
    {_SC("reverse"),array_reverse,1, _SC("a")},
    {_SC("sort"),array_sort,-1, _SC("ac")},
    {_SC("slice"),array_slice,-1, _SC("ann")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("clear"),obj_clear,1, _SC(".")},
    {_SC("map"),array_map,2, _SC("ac")},
    {_SC("apply"),array_apply,2, _SC("ac")},
    {_SC("reduce"),array_reduce,-2, _SC("ac.")},
    {_SC("filter"),array_filter,2, _SC("ac")},
    {_SC("indexof"),array_indexof,2, _SC("a.")},
    {_SC("contains"),array_contains,2, _SC("a.")},
    {_SC("each"),container_each,2, _SC("ac")},
    {_SC("findindex"),container_findindex,2, _SC("ac")},
    {_SC("findvalue"),container_findvalue,-2, _SC("ac.")},
    {_SC("totable"),pairs_to_table,1, _SC("a") },
    {_SC("replace"),array_replace,2, _SC("aa")},
    {_SC("clone"),obj_clone, 1, _SC(".") },
    {_SC("is_frozen"),obj_is_frozen, 1, _SC(".") },
    {NULL,(SQFUNCTION)0,0,NULL}
};

//STRING DEFAULT DELEGATE//////////////////////////

static SQInteger string_hash(HSQUIRRELVM v)
{
    union { SQHash hash; SQInteger i; SQUnsignedInteger u; } convert;
    memset(&convert, 0, sizeof(convert));
    convert.hash = _string(stack_get(v, 1))->_hash;
    if (convert.i < 0)
        convert.u = ~convert.u;
    sq_pushinteger(v, convert.i);
    return 1;
}

static SQInteger string_slice(HSQUIRRELVM v)
{
    SQInteger sidx,eidx;
    SQObjectPtr o;
    if(SQ_FAILED(get_slice_params(v,sidx,eidx,o)))return -1;
    SQInteger slen = _string(o)->_len;

    sidx = clamp_int((sidx < 0) ? (slen + sidx) : sidx, 0, slen);
    eidx = clamp_int((eidx < 0) ? (slen + eidx) : eidx, 0, slen);

    if (slen == 0 || eidx <= sidx)
    {
      v->Push(SQString::Create(_ss(v), _SC(""), -1));
      return 1;
    }

    v->Push(SQString::Create(_ss(v),&_stringval(o)[sidx],eidx-sidx));
    return 1;
}

static SQInteger _string_scan_for_substring(HSQUIRRELVM v, SQInteger (*push_result)(HSQUIRRELVM v, SQInteger index))
{
    SQInteger top,start_idx=0;
    const SQChar *str,*substr,*ret;
    if(((top=sq_gettop(v))>1) && SQ_SUCCEEDED(sq_getstring(v,1,&str)) && SQ_SUCCEEDED(sq_getstring(v,2,&substr))){
        if (sq_getsize(v,2)<1)
            return sq_throwerror(v, _SC("empty substring"));
        if(top>2)sq_getinteger(v,3,&start_idx);
        if((sq_getsize(v,1)>start_idx) && (start_idx>=0)){
            ret=strstr(&str[start_idx],substr);
            if(ret)
                return push_result(v, ret-str);
        }
        return push_result(v, -1);
    }
    return sq_throwerror(v,_SC("invalid param"));
}

static SQInteger string_indexof(HSQUIRRELVM v)
{
    return _string_scan_for_substring(v, _push_scan_index);
}

static SQInteger string_contains(HSQUIRRELVM v)
{
    return _string_scan_for_substring(v, _push_scan_found_flag);
}

static SQChar* replace_all(SQAllocContext allocctx, SQChar *s, SQInteger &buf_len, SQInteger &len,
                          const SQChar *from, SQInteger len_from, const SQChar *to, SQInteger len_to)
{
    for (SQInteger pos=0; pos<=len-len_from; ) {
        if (memcmp(s+pos, from, len_from*sizeof(SQChar))==0) {
            SQInteger d_size = len_to - len_from;
            if (d_size > 0) {
                s = (SQChar*)sq_realloc(allocctx, s, buf_len*sizeof(SQChar), (buf_len+d_size)*sizeof(SQChar));
                buf_len += d_size;
            }
            if (d_size!=0)
                memmove(s+pos+len_to, s+pos+len_from, (len-pos-len_from)*sizeof(SQChar));
            memcpy(s+pos, to, len_to*sizeof(SQChar));
            len += d_size;
            pos += len_to;
        }
        else
            ++pos;
    }
    return s;
}

static SQChar* replace_substring_internal(SQAllocContext allocctx, SQChar *s, SQInteger &buf_len, SQInteger &len,
    int pos, SQInteger len_from, const SQChar *to, SQInteger len_to)
{
    SQInteger d_size = len_to - len_from;
    if (d_size > 0) {
        s = (SQChar*)sq_realloc(allocctx, s, buf_len * sizeof(SQChar), (buf_len + d_size) * sizeof(SQChar));
        buf_len += d_size;
    }
    if (d_size != 0)
        memmove(s + pos + len_to, s + pos + len_from, (len - pos - len_from) * sizeof(SQChar));
    memcpy(s + pos, to, len_to * sizeof(SQChar));
    len += d_size;

    return s;
}


static SQInteger string_substitute(HSQUIRRELVM v)
{
    SQAllocContext allocctx = _ss(v)->_alloc_ctx;
    const SQChar *fmt;
    SQInteger len;
    sq_getstringandsize(v, 1, &fmt, &len);
    SQInteger buf_len = len;
    SQChar *s = (SQChar *)sq_malloc(allocctx, len*sizeof(SQChar));
    memcpy(s, fmt, len * sizeof(SQChar));

    SQInteger top = sq_gettop(v);

    for (int i = 0; i < int(len) - 2; i++)
        if (s[i] == _SC('{')) {
            int depth = 0;
            for (int j = i + 1; j < len; j++) {
                if (s[j] == _SC('}')) {
                    depth--;
                    if (depth < 0) {
                        if (i + 1 == j)
                          break;

                        int index = 0;
                        for (int k = i + 1; k < j; k++)
                            if (s[k] >= _SC('0') && s[k] <= _SC('9'))
                                index = index * 10 + s[k] - _SC('0');
                            else {
                                index = -1;
                                break;
                            }

                        if (index >= 0) {
                            index += 2;
                            if (index <= top) {
                                SQObjectPtr &val = stack_get(v, index);
                                SQObjectPtr valStr;
                                if (v->ToString(val, valStr)) {
                                    int delta = (int)_string(valStr)->_len - (j + 1 - i);
                                    s = replace_substring_internal(allocctx, s, buf_len, len, i, j + 1 - i,
                                        _stringval(valStr), _string(valStr)->_len);
                                    i = j + delta;
                                    break;
                                }
                                else {
                                    sq_free(allocctx, s, buf_len * sizeof(SQChar));
                                    return sq_throwerror(v, _SC("subst: Failed to convert value to string"));
                                }
                            }
                        }

                        for (int idx = 2; idx <= top; idx++) {
                            SQObjectPtr &arg = stack_get(v, idx);
                            if (sq_type(arg) == OT_TABLE) {
                                SQTable *table = _table(arg);
                                SQObjectPtr val;
                                SQObjectPtr valStr;
                                if (table->GetStr(s + i + 1, j - i - 1, val)) {
                                    if (v->ToString(val, valStr)) {
                                        int delta = (int)_string(valStr)->_len - (j + 1 - i);
                                        s = replace_substring_internal(allocctx, s, buf_len, len, i, j + 1 - i,
                                            _stringval(valStr), _string(valStr)->_len);
                                        i = j + delta;
                                        break;
                                    }
                                    else {
                                        sq_free(allocctx, s, buf_len * sizeof(SQChar));
                                        return sq_throwerror(v, _SC("subst: Failed to convert value to string"));
                                    }
                                }
                            }
                        }

                        break; // depth < 0
                    }
                }
                else if (s[j] == _SC('{'))
                  depth++;
            }
        }

    sq_pushstring(v, s, len);
    sq_free(allocctx, s, buf_len * sizeof(SQChar));
    return 1;
}


static SQInteger string_replace(HSQUIRRELVM v)
{
    const SQChar *s, *from, *to;
    SQInteger len_s, len_from, len_to;
    sq_getstringandsize(v, 1, &s, &len_s);
    sq_getstringandsize(v, 2, &from, &len_from);
    sq_getstringandsize(v, 3, &to, &len_to);

    if (len_from <= 0) {
        sq_push(v, 1);
        return 1;
    }

    // can be optimized by avoiding unnecessary copying
    SQInteger buf_len = len_s;
    SQChar *dest = (SQChar *)sq_malloc(_ss(v)->_alloc_ctx, buf_len*sizeof(SQChar));
    SQInteger len_dest = len_s;
    memcpy(dest, s, len_s*sizeof(SQChar));
    dest = replace_all(_ss(v)->_alloc_ctx, dest, buf_len, len_dest, from, len_from, to, len_to);

    sq_pushstring(v, dest, len_dest);
    sq_free(_ss(v)->_alloc_ctx, dest, buf_len*sizeof(SQChar));
    return 1;
}

static SQInteger buf_concat(sqvector<SQChar> &res, const SQObjectPtrVec &strings, const SQChar *sep, SQInteger sep_len)
{
    SQInteger out_pos = 0;
    for (SQInteger i=0, nitems=strings.size(); i<nitems; ++i) {
        if (i) {
            memcpy(res._vals+out_pos, sep, sep_len*sizeof(SQChar));
            out_pos += sep_len;
        }

        SQString *s = _string(strings[i]);
        memcpy(res._vals+out_pos, s->_val, s->_len*sizeof(SQChar));
        out_pos += s->_len;
    }
    return out_pos;
}

static SQInteger string_join(HSQUIRRELVM v)
{
    const SQChar *sep;
    SQInteger sep_len;
    sq_getstringandsize(v, 1, &sep, &sep_len);
    SQArray *arr = _array(stack_get(v, 2));

    SQObjectPtrVec strings(_ss(v)->_alloc_ctx);
    strings.reserve(arr->Size());
    SQObjectPtr tmp;
    SQObjectPtr flt;

    if (sq_gettop(v) > 3)
        return sq_throwerror(v, _SC("Too many arguments"));

    if (sq_gettop(v) == 3)
        flt = stack_get(v, 3);

    SQInteger res_len = 0;
    for (SQInteger i=0; i<arr->Size(); ++i) {
        SQObjectPtr &item = arr->_values[i];
        if (sq_isbool(flt) && sq_objtobool(&flt)) {
            if (sq_isnull(item) || (sq_isstring(item) && !_string(item)->_len))
                continue;
        } else if (sq_isclosure(flt) || sq_isnativeclosure(flt)) {
            sq_push(v, 1);
            sq_pushobject(v, item);

            if (SQ_FAILED(sq_call(v,2,SQTrue,SQ_BASELIB_INVOKE_CB_ERR_HANDLER)))
                return SQ_ERROR;
            bool use = !SQVM::IsFalse(v->GetUp(-1));
            v->Pop();
            if (!use)
                continue;
        }

        if (!v->ToString(item, tmp))
            return sq_throwerror(v, _SC("Failed to convert array item to string"));

        strings.push_back(tmp);
        res_len += _string(tmp)->_len;
    }

    if (strings.empty()) {
        sq_pushstring(v, _SC(""), 0);
        return 1;
    }

    res_len += sep_len * (strings.size()-1);

    sqvector<SQChar> res(_ss(v)->_alloc_ctx);
    res.resize(res_len);
    SQInteger out_pos = buf_concat(res, strings, sep, sep_len);
    assert(out_pos == res_len);
    sq_pushstring(v, res._vals, res_len);
    return 1;
}


static SQInteger string_concat(HSQUIRRELVM v)
{
    const SQChar *sep;
    SQInteger sep_len;
    sq_getstringandsize(v, 1, &sep, &sep_len);
    SQInteger nitems = sq_gettop(v)-1;
    if (nitems < 1) {
        sq_pushstring(v, _SC(""), 0);
        return 1;
    }

    SQObjectPtrVec strings(_ss(v)->_alloc_ctx);
    strings.resize(nitems);

    SQInteger res_len = sep_len * (nitems-1);
    for (SQInteger i=0; i<nitems; ++i) {
        if (!v->ToString(stack_get(v, i+2), strings[i]))
            return sq_throwerror(v, _SC("Failed to convert array item to string"));
        res_len += _string(strings[i])->_len;
    }

    sqvector<SQChar> res(_ss(v)->_alloc_ctx);
    res.resize(res_len);
    SQInteger out_pos = buf_concat(res, strings, sep, sep_len);
    assert(out_pos == res_len);
    sq_pushstring(v, res._vals, res_len);
    return 1;
}


static SQInteger string_split(HSQUIRRELVM v)
{
    const SQChar *str;
    SQInteger len;
    sq_getstringandsize(v, 1, &str, &len);

    SQArray *res = SQArray::Create(_ss(v),0);

    if (sq_gettop(v) == 1) {
        SQInteger start = 0;
        for (SQInteger pos=0; pos<=len; ++pos) {
            if (pos==len || isspace(str[pos])) {
                if (pos > start)
                    res->Append(SQObjectPtr(SQString::Create(_ss(v), str+start, pos-start)));
                start = pos+1;
            }
        }
    } else {
        const SQChar *sep;
        SQInteger sep_len;
        sq_getstringandsize(v, 2, &sep, &sep_len);
        if (sep_len < 1)
            return sq_throwerror(v, _SC("empty separator"));
        SQInteger start = 0;
        for (SQInteger pos=0; pos<=len;) {
            if (pos>len-sep_len) {
                res->Append(SQObjectPtr(SQString::Create(_ss(v), str+start, len-start)));
                break;
            }
            else if (strncmp(str+pos, sep, sep_len)==0) {
                res->Append(SQObjectPtr(SQString::Create(_ss(v), str+start, pos-start)));
                pos += sep_len;
                start = pos;
            }
            else
                ++pos;
        }
    }
    v->Push(res);
    return 1;
}


#define STRING_TOFUNCZ(func) static SQInteger string_##func(HSQUIRRELVM v) \
{\
    SQInteger sidx,eidx; \
    SQObjectPtr str; \
    if(SQ_FAILED(get_slice_params(v,sidx,eidx,str)))return -1; \
    SQInteger slen = _string(str)->_len; \
    if(sidx < 0)sidx = slen + sidx; \
    if(eidx < 0)eidx = slen + eidx; \
    if(eidx < sidx) return sq_throwerror(v,_SC("wrong indexes")); \
    if(eidx > slen || sidx < 0) return sq_throwerror(v,_SC("slice out of range")); \
    SQInteger len=_string(str)->_len; \
    const SQChar *sthis=_stringval(str); \
    SQChar *snew=(_ss(v)->GetScratchPad(len)); \
    memcpy(snew,sthis,len);\
    for(SQInteger i=sidx;i<eidx;i++) snew[i] = (SQChar)func(sthis[i]); \
    v->Push(SQString::Create(_ss(v),snew,len)); \
    return 1; \
}


STRING_TOFUNCZ(tolower)
STRING_TOFUNCZ(toupper)

#define IMPL_STRING_FUNC(name) static SQInteger _baselib_string_##name(HSQUIRRELVM v) { return _sq_string_ ## name ## _impl(v, 1); }

IMPL_STRING_FUNC(strip)
IMPL_STRING_FUNC(lstrip)
IMPL_STRING_FUNC(rstrip)
IMPL_STRING_FUNC(split_by_chars)
IMPL_STRING_FUNC(escape)
IMPL_STRING_FUNC(startswith)
IMPL_STRING_FUNC(endswith)

#undef IMPL_STRING_FUNC
#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_baselib_string_##name,nparams,pmask}

const SQRegFunction SQSharedState::_string_default_delegate_funcz[]={
    {_SC("len"),default_delegate_len,1, _SC("s")},
    {_SC("tointeger"),default_delegate_tointeger,-1, _SC("sn")},
    {_SC("tofloat"),default_delegate_tofloat,1, _SC("s")},
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("hash"),string_hash, 1, _SC("s")},
    {_SC("slice"),string_slice,-1, _SC("s n  n")},
    {_SC("indexof"),string_indexof,-2, _SC("s s n")},
    {_SC("contains"),string_contains,-2, _SC("s s n")},
    {_SC("tolower"),string_tolower,-1, _SC("s n n")},
    {_SC("toupper"),string_toupper,-1, _SC("s n n")},
    {_SC("subst"),string_substitute,-2, _SC("s")},
    {_SC("replace"),string_replace, 3, _SC("s s s")},
    {_SC("join"),string_join, -2, _SC("s a b|c")},
    {_SC("concat"),string_concat, -2, _SC("s")},
    {_SC("split"),string_split, -1, _SC("s s")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("clone"),obj_clone, 1, _SC(".") },
    _DECL_FUNC(strip,1,_SC("s")),
    _DECL_FUNC(lstrip,1,_SC("s")),
    _DECL_FUNC(rstrip,1,_SC("s")),
    _DECL_FUNC(split_by_chars,-2,_SC("ssb")),
    _DECL_FUNC(escape,1,_SC("s")),
    _DECL_FUNC(startswith,2,_SC("ss")),
    _DECL_FUNC(endswith,2,_SC("ss")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

#undef _DECL_FUNC

//INTEGER DEFAULT DELEGATE//////////////////////////
const SQRegFunction SQSharedState::_number_default_delegate_funcz[]={
    {_SC("tointeger"),default_delegate_tointeger,1, _SC("n|b")},
    {_SC("tofloat"),default_delegate_tofloat,1, _SC("n|b")},
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("tochar"),number_delegate_tochar,1, _SC("n|b")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("clone"),obj_clone, 1, _SC(".") },
    {NULL,(SQFUNCTION)0,0,NULL}
};

//CLOSURE DEFAULT DELEGATE//////////////////////////
static SQInteger closure_pcall(HSQUIRRELVM v)
{
    return SQ_SUCCEEDED(sq_call(v,sq_gettop(v)-1,SQTrue,SQFalse))?1:SQ_ERROR;
}

static SQInteger closure_call(HSQUIRRELVM v)
{
    SQObjectPtr &c = stack_get(v, -1);
    if (sq_type(c) == OT_CLOSURE && (_closure(c)->_function->_bgenerator == false))
    {
        return sq_tailcall(v, sq_gettop(v) - 1);
    }
    return SQ_SUCCEEDED(sq_call(v, sq_gettop(v) - 1, SQTrue, SQTrue)) ? 1 : SQ_ERROR;
}

static SQInteger _closure_acall(HSQUIRRELVM v,SQBool invoke_err_handler)
{
    SQArray *aparams=_array(stack_get(v,2));
    SQInteger nparams=aparams->Size();
    v->Push(stack_get(v,1));
    for(SQInteger i=0;i<nparams;i++)v->Push(aparams->_values[i]);
    return SQ_SUCCEEDED(sq_call(v,nparams,SQTrue,invoke_err_handler))?1:SQ_ERROR;
}

static SQInteger closure_acall(HSQUIRRELVM v)
{
    return _closure_acall(v,SQTrue);
}

static SQInteger closure_pacall(HSQUIRRELVM v)
{
    return _closure_acall(v,SQFalse);
}

static SQInteger closure_bindenv(HSQUIRRELVM v)
{
    if(SQ_FAILED(sq_bindenv(v,1)))
        return SQ_ERROR;
    return 1;
}


static SQInteger closure_getfreevar(HSQUIRRELVM v)
{
    SQInteger varidx;
    sq_getinteger(v, 2, &varidx);
    const SQChar *name = sq_getfreevariable(v, 1, SQUnsignedInteger(varidx));
    if (!name)
        return sq_throwerror(v, _SC("Invalid free variable index"));
    SQObjectPtr val = v->PopGet();
    SQTable *res = SQTable::Create(_ss(v),2);
    res->NewSlot(SQString::Create(_ss(v),_SC("name"),-1), SQString::Create(_ss(v),name,-1));
    res->NewSlot(SQString::Create(_ss(v),_SC("value"),-1), val);
    v->Push(res);
    return 1;
}


static SQInteger closure_getfuncinfos_obj(HSQUIRRELVM v, SQObjectPtr & o) {
    SQTable *res = SQTable::Create(_ss(v),4);
    if(sq_type(o) == OT_CLOSURE) {
        SQFunctionProto *f = _closure(o)->_function;
        SQInteger nparams = f->_nparameters + (f->_varparams?1:0);
        SQObjectPtr params = SQArray::Create(_ss(v),nparams);
        SQObjectPtr defparams = SQArray::Create(_ss(v),f->_ndefaultparams);
        for(SQInteger n = 0; n<f->_nparameters; n++) {
            _array(params)->Set((SQInteger)n,f->_parameters[n]);
        }
        for(SQInteger j = 0; j<f->_ndefaultparams; j++) {
            _array(defparams)->Set((SQInteger)j,_closure(o)->_defaultparams[j]);
        }
        if(f->_varparams) {
            _array(params)->Set(nparams-1,SQString::Create(_ss(v),_SC("..."),-1));
        }
        res->NewSlot(SQString::Create(_ss(v),_SC("native"),-1),false);
        res->NewSlot(SQString::Create(_ss(v),_SC("name"),-1),f->_name);
        res->NewSlot(SQString::Create(_ss(v),_SC("src"),-1),f->_sourcename);
        res->NewSlot(SQString::Create(_ss(v),_SC("line"),-1),f->_lineinfos[0]._line);
        res->NewSlot(SQString::Create(_ss(v),_SC("parameters"),-1),params);
        res->NewSlot(SQString::Create(_ss(v),_SC("varargs"),-1),f->_varparams);
        res->NewSlot(SQString::Create(_ss(v),_SC("defparams"),-1),defparams);
        res->NewSlot(SQString::Create(_ss(v),_SC("freevars"),-1),f->_noutervalues);
    }
    else { //OT_NATIVECLOSURE
        SQNativeClosure *nc = _nativeclosure(o);
        res->NewSlot(SQString::Create(_ss(v),_SC("native"),-1),true);
        res->NewSlot(SQString::Create(_ss(v),_SC("name"),-1),nc->_name);
        res->NewSlot(SQString::Create(_ss(v),_SC("docstring"),-1),nc->_docstring);
        res->NewSlot(SQString::Create(_ss(v),_SC("paramscheck"),-1),nc->_nparamscheck);
        res->NewSlot(SQString::Create(_ss(v),_SC("freevars"),-1),SQInteger(nc->_noutervalues));
        SQObjectPtr typecheck;
        if(nc->_typecheck.size() > 0) {
            typecheck =
                SQArray::Create(_ss(v), nc->_typecheck.size());
            for(SQUnsignedInteger n = 0; n<nc->_typecheck.size(); n++) {
                    _array(typecheck)->Set((SQInteger)n,nc->_typecheck[n]);
            }
        }
        res->NewSlot(SQString::Create(_ss(v),_SC("typecheck"),-1),typecheck);
    }
    v->Push(res);
    return 1;
}


static SQInteger closure_getfuncinfos(HSQUIRRELVM v)
{
  SQObjectPtr o = stack_get(v, 1);
  return closure_getfuncinfos_obj(v, o);
}


static SQInteger delegable_getfuncinfos(HSQUIRRELVM v)
{
  SQObjectPtr o = stack_get(v, 1);

  SQDelegable * delegable = _delegable(o);
  SQObjectPtr call;
  if (delegable->GetMetaMethod(v, MT_CALL, call))
    closure_getfuncinfos_obj(v, call);
  else
    sq_pushnull(v);

  return 1;
}

static SQInteger class_getfuncinfos(HSQUIRRELVM v)
{
  SQObjectPtr o = stack_get(v, 1);

  if (!sq_isnull(_class(o)->_metamethods[MT_CALL]))
    closure_getfuncinfos_obj(v, _class(o)->_metamethods[MT_CALL]);
  else
    sq_pushnull(v);

  return 1;
}


const SQRegFunction SQSharedState::_closure_default_delegate_funcz[]={
    {_SC("call"),closure_call,-1, _SC("c")},
    {_SC("pcall"),closure_pcall,-1, _SC("c")},
    {_SC("acall"),closure_acall,2, _SC("ca")},
    {_SC("pacall"),closure_pacall,2, _SC("ca")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("bindenv"),closure_bindenv,2, _SC("c x|y|t")},
    {_SC("getfuncinfos"),closure_getfuncinfos,1, _SC("c")},
    {_SC("getfreevar"),closure_getfreevar,2, _SC("ci")},
    {_SC("clone"),obj_clone, 1, _SC(".") },
    {NULL,(SQFUNCTION)0,0,NULL}
};

//GENERATOR DEFAULT DELEGATE
static SQInteger generator_getstatus(HSQUIRRELVM v)
{
    SQObject &o=stack_get(v,1);
    switch(_generator(o)->_state){
        case SQGenerator::eSuspended:v->Push(SQString::Create(_ss(v),_SC("suspended")));break;
        case SQGenerator::eRunning:v->Push(SQString::Create(_ss(v),_SC("running")));break;
        case SQGenerator::eDead:v->Push(SQString::Create(_ss(v),_SC("dead")));break;
    }
    return 1;
}

const SQRegFunction SQSharedState::_generator_default_delegate_funcz[]={
    {_SC("getstatus"),generator_getstatus,1, _SC("g")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("clone"),obj_clone, 1, _SC(".") },
    {NULL,(SQFUNCTION)0,0,NULL}
};

//THREAD DEFAULT DELEGATE
static SQInteger thread_call(HSQUIRRELVM v)
{
    SQObjectPtr o = stack_get(v,1);
    if(sq_type(o) == OT_THREAD) {
        SQInteger nparams = sq_gettop(v);
        sq_reservestack(_thread(o), nparams + 3);
        _thread(o)->Push(_thread(o)->_roottable);
        for(SQInteger i = 2; i<(nparams+1); i++)
            sq_move(_thread(o),v,i);
        if(SQ_SUCCEEDED(sq_call(_thread(o),nparams,SQTrue,SQTrue))) {
            sq_move(v,_thread(o),-1);
            sq_pop(_thread(o),1);
            return 1;
        }
        v->_lasterror = _thread(o)->_lasterror;
        return SQ_ERROR;
    }
    return sq_throwerror(v,_SC("wrong parameter"));
}

static SQInteger thread_wakeup(HSQUIRRELVM v)
{
    SQObjectPtr o = stack_get(v,1);
    if(sq_type(o) == OT_THREAD) {
        SQVM *thread = _thread(o);
        SQInteger state = sq_getvmstate(thread);
        if(state != SQ_VMSTATE_SUSPENDED) {
            switch(state) {
                case SQ_VMSTATE_IDLE:
                    return sq_throwerror(v,_SC("cannot wakeup a idle thread"));
                break;
                case SQ_VMSTATE_RUNNING:
                    return sq_throwerror(v,_SC("cannot wakeup a running thread"));
                break;
            }
        }

        SQInteger wakeupret = sq_gettop(v)>1?SQTrue:SQFalse;
        if(wakeupret) {
            sq_move(thread,v,2);
        }
        if(SQ_SUCCEEDED(sq_wakeupvm(thread,wakeupret,SQTrue,SQTrue,SQFalse))) {
            sq_move(v,thread,-1);
            sq_pop(thread,1); //pop retval
            if(sq_getvmstate(thread) == SQ_VMSTATE_IDLE) {
                sq_settop(thread,1); //pop roottable
            }
            return 1;
        }
        sq_settop(thread,1);
        v->_lasterror = thread->_lasterror;
        return SQ_ERROR;
    }
    return sq_throwerror(v,_SC("wrong parameter"));
}

static SQInteger thread_wakeupthrow(HSQUIRRELVM v)
{
    SQObjectPtr o = stack_get(v,1);
    if(sq_type(o) == OT_THREAD) {
        SQVM *thread = _thread(o);
        SQInteger state = sq_getvmstate(thread);
        if(state != SQ_VMSTATE_SUSPENDED) {
            switch(state) {
                case SQ_VMSTATE_IDLE:
                    return sq_throwerror(v,_SC("cannot wakeup a idle thread"));
                break;
                case SQ_VMSTATE_RUNNING:
                    return sq_throwerror(v,_SC("cannot wakeup a running thread"));
                break;
            }
        }

        sq_move(thread,v,2);
        sq_throwobject(thread);
        SQBool rethrow_error = SQTrue;
        if(sq_gettop(v) > 2) {
            sq_getbool(v,3,&rethrow_error);
        }
        if(SQ_SUCCEEDED(sq_wakeupvm(thread,SQFalse,SQTrue,SQTrue,SQTrue))) {
            sq_move(v,thread,-1);
            sq_pop(thread,1); //pop retval
            if(sq_getvmstate(thread) == SQ_VMSTATE_IDLE) {
                sq_settop(thread,1); //pop roottable
            }
            return 1;
        }
        sq_settop(thread,1);
        if(rethrow_error) {
            v->_lasterror = thread->_lasterror;
            return SQ_ERROR;
        }
        return SQ_OK;
    }
    return sq_throwerror(v,_SC("wrong parameter"));
}

static SQInteger thread_getstatus(HSQUIRRELVM v)
{
    SQObjectPtr &o = stack_get(v,1);
    switch(sq_getvmstate(_thread(o))) {
        case SQ_VMSTATE_IDLE:
            sq_pushstring(v,_SC("idle"),-1);
        break;
        case SQ_VMSTATE_RUNNING:
            sq_pushstring(v,_SC("running"),-1);
        break;
        case SQ_VMSTATE_SUSPENDED:
            sq_pushstring(v,_SC("suspended"),-1);
        break;
        default:
            return sq_throwerror(v,_SC("internal VM error"));
    }
    return 1;
}

static SQInteger thread_getstackinfos(HSQUIRRELVM v)
{
    SQObjectPtr o = stack_get(v,1);
    if(sq_type(o) == OT_THREAD) {
        SQVM *thread = _thread(o);
        SQInteger threadtop = sq_gettop(thread);
        SQInteger level;
        sq_getinteger(v,-1,&level);
        SQRESULT res = __sq_getcallstackinfos(thread,level);
        if(SQ_FAILED(res))
        {
            sq_settop(thread,threadtop);
            if(sq_type(thread->_lasterror) == OT_STRING) {
                sq_throwerror(v,_stringval(thread->_lasterror));
            }
            else {
                sq_throwerror(v,_SC("unknown error"));
            }
        }
        if(res > 0) {
            //some result
            sq_move(v,thread,-1);
            sq_settop(thread,threadtop);
            return 1;
        }
        //no result
        sq_settop(thread,threadtop);
        return 0;

    }
    return sq_throwerror(v,_SC("wrong parameter"));
}

const SQRegFunction SQSharedState::_thread_default_delegate_funcz[] = {
    {_SC("call"), thread_call, -1, _SC("v")},
    {_SC("wakeup"), thread_wakeup, -1, _SC("v")},
    {_SC("wakeupthrow"), thread_wakeupthrow, -2, _SC("v.b")},
    {_SC("getstatus"), thread_getstatus, 1, _SC("v")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("getstackinfos"),thread_getstackinfos,2, _SC("vn")},
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("clone"),obj_clone, 1, _SC(".") },
    {NULL,(SQFUNCTION)0,0,NULL}
};


static SQInteger class_instance(HSQUIRRELVM v)
{
    return SQ_SUCCEEDED(sq_createinstance(v,-1))?1:SQ_ERROR;
}

static SQInteger class_getbase(HSQUIRRELVM v)
{
    return SQ_SUCCEEDED(sq_getbase(v,-1))?1:SQ_ERROR;
}

static SQInteger class_newmember(HSQUIRRELVM v)
{
    SQInteger top = sq_gettop(v);
    SQBool bstatic = SQFalse;
    if(top == 5)
    {
        sq_tobool(v,-1,&bstatic);
        sq_pop(v,1);
    }

    if(top < 4) {
        sq_pushnull(v);
    }
    return SQ_SUCCEEDED(sq_newmember(v,-4,bstatic))?1:SQ_ERROR;
}


static SQInteger get_class_metamethod(HSQUIRRELVM v)
{
    SQInteger mmidx = _ss(v)->GetMetaMethodIdxByName(stack_get(v, 2));
    if (mmidx < 0)
        return sq_throwerror(v, _SC("Unknown metamethod"));

    SQClass *cls = nullptr;
    if (sq_gettype(v, 1) == OT_CLASS)
        cls = _class(stack_get(v, 1));
    else // instance
        cls = _instance(stack_get(v, 1))->_class;
    v->Push(cls->_metamethods[mmidx]);
    return 1;
}


const SQRegFunction SQSharedState::_class_default_delegate_funcz[] = {
    {_SC("rawget"),container_rawget,2, _SC("y")},
    {_SC("rawset"),container_rawset,3, _SC("y")},
    {_SC("rawin"),container_rawexists,2, _SC("y")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("instance"),class_instance,1, _SC("y")},
    {_SC("getbase"),class_getbase,1, _SC("y")},
    {_SC("newmember"),class_newmember,-3, _SC("y")},
    {_SC("getfuncinfos"),class_getfuncinfos,1, _SC("y")},
    {_SC("call"),closure_call,-1, _SC("y")},
    {_SC("pcall"),closure_pcall,-1, _SC("y")},
    {_SC("acall"),closure_acall,2, _SC("ya")},
    {_SC("pacall"),closure_pacall,2, _SC("ya")},
    {_SC("__update"),container_update, -2, _SC("t|yt|y|x") },
    {_SC("__merge"),container_merge, -2, _SC("t|yt|y|x") },
    {_SC("getmetamethod"),get_class_metamethod, 2, _SC("ys")},
    {_SC("clone"),obj_clone, 1, _SC(".") },
    {_SC("is_frozen"),obj_is_frozen, 1, _SC(".") },
    {NULL,(SQFUNCTION)0,0,NULL}
};


static SQInteger instance_getclass(HSQUIRRELVM v)
{
    if(SQ_SUCCEEDED(sq_getclass(v,1)))
        return 1;
    return SQ_ERROR;
}

const SQRegFunction SQSharedState::_instance_default_delegate_funcz[] = {
    {_SC("getclass"), instance_getclass, 1, _SC("x")},
    {_SC("rawget"),container_rawget,2, _SC("x")},
    {_SC("rawset"),container_rawset,3, _SC("x")},
    {_SC("rawin"),container_rawexists,2, _SC("x")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("getfuncinfos"),delegable_getfuncinfos,1, _SC("x")},
    {_SC("getmetamethod"),get_class_metamethod, 2, _SC("xs")},
    {_SC("clone"),obj_clone, 1, _SC(".") },
    {_SC("is_frozen"),obj_is_frozen, 1, _SC(".") },
    {NULL,(SQFUNCTION)0,0,NULL}
};

static SQInteger weakref_ref(HSQUIRRELVM v)
{
    if(SQ_FAILED(sq_getweakrefval(v,1)))
        return SQ_ERROR;
    return 1;
}

const SQRegFunction SQSharedState::_weakref_default_delegate_funcz[] = {
    {_SC("ref"),weakref_ref,1, _SC("r")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("clone"),obj_clone, 1, _SC(".") },
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQRegFunction SQSharedState::_userdata_default_delegate_funcz[] = {
    {_SC("getfuncinfos"),delegable_getfuncinfos,1, _SC("u")},
    {_SC("clone"),obj_clone, 1, _SC(".") },
    {_SC("is_frozen"),obj_is_frozen, 1, _SC(".") },
    {NULL,(SQFUNCTION)0,0,NULL}
};
