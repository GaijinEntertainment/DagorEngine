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
#include "compiler/sqtypeparser.h"
#include <sq_char_class.h>

#ifndef SQ_BASELIB_INVOKE_CB_ERR_HANDLER
#define SQ_BASELIB_INVOKE_CB_ERR_HANDLER SQTrue
#endif


static SQInteger delegable_getfuncinfos(HSQUIRRELVM v);
static SQInteger class_getfuncinfos(HSQUIRRELVM v);
SQInteger sq_deduplicate_object(HSQUIRRELVM vm, int index);


#define SQ_CHECK_IMMUTABLE_SELF \
    if (sq_objflags(stack_get(v,1)) & SQOBJ_FLAG_IMMUTABLE) \
        return sq_throwerror(v, "Cannot modify immutable object");

#define SQ_CHECK_IMMUTABLE_OBJ(o) \
    if (sq_objflags(o) & SQOBJ_FLAG_IMMUTABLE) \
        return sq_throwerror(v, "Cannot modify immutable object");


static bool sq_parse_float(const char* str_begin, const char * str_end, SQObjectPtr& res, SQInteger base)
{
#if SQ_USE_STD_FROM_CHARS
    SQFloat r;
    auto ret = std::from_chars(str_begin, str_end, r);
    for (const char* ptr = ret.ptr; ptr != str_end; ++ptr)
        if (*ptr != '0')
            return false;
    if (ret.ec == std::errc::invalid_argument)
        return false;
    res = r;
#else
    char* end;
    SQFloat r = SQFloat(strtod(str_begin, &end));
    if (str_begin == end)
        return false;
    res = r;
#endif
    return true;
}

static bool sq_parse_int(const char* str_begin, const char* str_end, SQObjectPtr& res, SQInteger base)
{
    char* end;
    const char* e = str_begin;
    bool iseintbase = base > 13; //to fix error converting hexadecimals with e like 56f0791e
    char c;
    while ((c = *e) != '\0')
    {
        if (c == '.' || (!iseintbase && (c == 'E' || c == 'e'))) //e and E is for scientific notation
            return sq_parse_float(str_begin, str_end, res, base);
        e++;
    }

    SQInteger r = SQInteger(scstrtol(str_begin, &end, (int)base));
    if (str_begin == end)
        return false;
    res = r;
    return true;
}


static SQInteger get_allowed_args_count(const SQObject &closure, SQInteger num_supported)
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
    const char *name = NULL;

    if (SQ_SUCCEEDED(sq_stackinfos(v, level, &si)))
    {
        const char *fn = "unknown";
        const char *src = "unknown";
        if(si.funcname)fn = si.funcname;
        if(si.source)src = si.source;
        sq_newtable(v);
        sq_pushstring(v, "func", -1);
        sq_pushstring(v, fn, -1);
        sq_newslot(v, -3, SQFalse);
        sq_pushstring(v, "src", -1);
        sq_pushstring(v, src, -1);
        sq_newslot(v, -3, SQFalse);
        sq_pushstring(v, "line", -1);
        sq_pushinteger(v, si.line);
        sq_newslot(v, -3, SQFalse);
        sq_pushstring(v, "locals", -1);
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
                const char *str = 0;
                if (SQ_SUCCEEDED(sq_getstring(v,-1,&str)))
                    return sq_throwerror(v, str);
            }
        }

        return sq_throwerror(v, "assertion failed");
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
    if (!pf) return 0;

    const SQInteger top = sq_gettop(v);
    SQObjectPtr s;

    for (SQInteger i = 2; i <= top; ++i) {
        if (i > 2)
            pf(v, " ");

        if (v->ToString(stack_get(v, i), s))
            pf(v, "%s", _stringval(s));
        else
            pf(v, "< _tostring() call error >");
    }

    if (newline)
        pf(v, "\n");

    return 0;
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
    const char *src=NULL,*name="unnamedbuffer";
    SQInteger size;
    sq_getstringandsize(v,2,&src,&size);
    if (nargs>2) {
        sq_getstring(v,3,&name);
    }

    HSQOBJECT bindings;
    bool ownBindings = false;
    if (nargs>3) {
        sq_getstackobj(v, 4, &bindings);
    }
    else {
        ownBindings = true;
        sq_newtable(v);
        sq_getstackobj(v, -1, &bindings);
        sq_addref(v, &bindings);
        sq_poptop(v);
    }

    // overwrites existing keys in bindings with ones from base lib
    sq_pushobject(v, bindings);
    sq_registerbaselib(v);
    sq_poptop(v);

    bool ok = SQ_SUCCEEDED(sq_compile(v, src, size, name, SQFalse, &bindings));

    if (ownBindings)
        sq_release(v, &bindings);

    return ok ? 1 : SQ_ERROR;
}

static SQInteger base_newthread(HSQUIRRELVM v)
{
    SQObjectPtr &func = stack_get(v,2);
    SQInteger stksize = (_closure(func)->_function->_stacksize << 1) + 2; // -V629
    HSQUIRRELVM newv = sq_newthread(v, (stksize < MIN_STACK_OVERHEAD + 2)? MIN_STACK_OVERHEAD + 2 : stksize);
    sq_move(newv,v,-2);
    return 1;
}

static SQInteger base_suspend(HSQUIRRELVM v)
{
    return sq_suspendvm(v);
}

static SQInteger builtin_array_ctor(HSQUIRRELVM v)
{
    SQInteger size = tointeger(stack_get(v,2));
    if (size < 0)
        return sq_throwerror(v, "array size must be non-negative");

    SQArray *a;
    if(sq_gettop(v) > 2) {
        a = SQArray::Create(_ss(v),0);
        a->Resize(size,stack_get(v,3));
    }
    else {
        a = SQArray::Create(_ss(v),size);
    }
    v->Push(SQObjectPtr(a));
    return 1;
}

static SQInteger base_type(HSQUIRRELVM v)
{
    SQObjectPtr &o = stack_get(v,2);
    v->Push(SQObjectPtr(SQString::Create(_ss(v),GetTypeName(o),-1)));
    return 1;
}

static SQInteger base_callee(HSQUIRRELVM v)
{
    if(v->_callsstacksize > 1)
    {
        v->Push(v->_callsstack[v->_callsstacksize - 2]._closure);
        return 1;
    }
    return sq_throwerror(v,"no closure in the calls stack");
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

static SQInteger base_deduplicate_object(HSQUIRRELVM v)
{
    return sq_deduplicate_object(v, 2);
}

static SQInteger base_classof(HSQUIRRELVM v)
{
    SQObjectPtr &obj = stack_get(v, 2);

    if (sq_type(obj) == OT_INSTANCE) {
        v->Push(SQObjectPtr(_instance(obj)->_class));
        return 1;
    }
    else if (SQClass *cls = v->GetBuiltInClassForType(sq_type(obj))) {
        v->Push(SQObjectPtr(cls));
        return 1;
    }

    assert(0);
    return 0;
}

static const SQRegFunctionFromStr base_funcs[] = {
    { base_getroottable, "getroottable(): table" },
    { base_getconsttable, "getconsttable(): table" },
    { base_assert, "assert(condition, [message: function|string])" },
    { base_print_, "print(...)" },
    { base_print_newline, "println(...)" },
    { base_error_, "error(...)" },
    { base_error_newline, "errorln(...)" },
    { base_compilestring, "compilestring(source: string, [name: string, bindings: table|null]): function" },
    { base_newthread, "newthread(func: function): thread" },
    { base_suspend, "suspend(...): any" },
    { builtin_array_ctor, "array(size: int, [default_value]): array" },
    { base_type, "pure type(obj): string" },
    { base_callee, "callee(): function" },
    { base_freeze, "freeze(obj): any" },
    { base_deduplicate_object, "deduplicate_object(obj: table|array|null): table|array|null" },
    { base_getobjflags, "getobjflags(obj): int" },
    { NULL, NULL }
};


SQRESULT sq_registerbaselib(HSQUIRRELVM v)
{
    SQInteger i=0;
    while(base_funcs[i].f) {
        sq_new_closure_slot_from_decl_string(v, base_funcs[i].f, 0, base_funcs[i].declstring, base_funcs[i].docstring);
        i++;
    }

    return SQ_OK;
}


static const SQRegFunctionFromStr types_funcs[] = {
    { base_classof, "pure classof(obj): class" },
    { NULL, NULL }
};

SQRESULT sq_registertypeslib(HSQUIRRELVM v)
{
    SQInteger i=0;
    while(types_funcs[i].f) {
        sq_new_closure_slot_from_decl_string(v, types_funcs[i].f, 0, types_funcs[i].declstring, types_funcs[i].docstring);
        i++;
    }

    #define REG_CLS(name, cls) \
        sq_pushstring(v, #name, -1); \
        v->Push(cls); \
        sq_newslot(v, -3, SQFalse);

    REG_CLS(Integer, _ss(v)->_integer_class)
    REG_CLS(Float, _ss(v)->_float_class);
    REG_CLS(Bool, _ss(v)->_bool_class);
    REG_CLS(String, _ss(v)->_string_class);
    REG_CLS(Array, _ss(v)->_array_class);
    REG_CLS(Table, _ss(v)->_table_class);
    REG_CLS(Function, _ss(v)->_function_class);
    REG_CLS(Generator, _ss(v)->_generator_class);
    REG_CLS(Thread, _ss(v)->_thread_class);
    REG_CLS(Class, _ss(v)->_class_class);
    REG_CLS(Instance, _ss(v)->_instance_class);
    REG_CLS(WeakRef, _ss(v)->_weakref_class);
    REG_CLS(UserData, _ss(v)->_userdata_class);
    REG_CLS(Null, _ss(v)->_null_class);

    #undef REG_CLS

    return SQ_OK;
}


void sq_base_register(HSQUIRRELVM v)
{
    sq_pushroottable(v);
    sq_registerbaselib(v);
    sq_pop(v,1);

    sq_pushconsttable(v);
    sq_pushstring(v, "SQOBJ_FLAG_IMMUTABLE", -1);
    sq_pushinteger(v, SQOBJ_FLAG_IMMUTABLE);
    sq_newslot(v, -3, SQFalse);
    sq_pop(v,1);
}

static SQInteger default_type_method_len(HSQUIRRELVM v)
{
    v->Push(SQObjectPtr(SQInteger(sq_getsize(v,1))));
    return 1;
}

static SQInteger default_type_method_tofloat(HSQUIRRELVM v)
{
    SQObjectPtr &o=stack_get(v,1);
    switch(sq_type(o)){
    case OT_STRING:{
        SQObjectPtr res;
        if(sq_parse_float(_stringval(o), _stringval(o) + _string(o)->_len, res, 10)){
            v->Push(SQObjectPtr(tofloat(res)));
            break;
        }}
        return sq_throwerror(v, "cannot convert the string to float");
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

static SQInteger default_type_method_tointeger(HSQUIRRELVM v)
{
    SQObjectPtr &o=stack_get(v,1);
    SQInteger base = 10;
    if(sq_gettop(v) > 1) {
        sq_getinteger(v,2,&base);
    }
    switch(sq_type(o)){
    case OT_STRING:{
        SQObjectPtr res;
        if(sq_parse_int(_stringval(o), _stringval(o) + _string(o)->_len, res, base)){
            v->Push(SQObjectPtr(tointeger(res)));
            break;
        }}
        return sq_throwerror(v, "cannot convert the string to integer");
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

static SQInteger default_type_method_tostring(HSQUIRRELVM v)
{
    if(SQ_FAILED(sq_tostring(v,1)))
        return SQ_ERROR;
    return 1;
}

static SQInteger obj_type_method_weakref(HSQUIRRELVM v)
{
    sq_weakref(v,1);
    return 1;
}

static SQInteger obj_clear(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;
    return SQ_SUCCEEDED(sq_clear(v,-1)) ? 1 : SQ_ERROR;
}


static SQInteger number_type_method_tochar(HSQUIRRELVM v)
{
    SQObject &o=stack_get(v,1);
    char c = (char)tointeger(o);
    v->Push(SQObjectPtr(SQString::Create(_ss(v),(const char *)&c,1)));
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
            return sq_throwerror(v, "merge() only works on tables and classes");
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
    const SQObjectPtr &o = stack_get(v,1);
    const SQObjectPtr &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 4);
    SQObjectPtr tmp;

    sq_pushnull(v);
    while (SQ_SUCCEEDED(sq_next(v, 1))) {
        SQInteger iterTop = sq_gettop(v);
        v->PushNull();
        v->Push(stack_get(v, iterTop));
        if (nArgs >= 3)
            v->Push(stack_get(v, iterTop-1));
        if (nArgs >= 4)
            v->Push(o);

        bool callRes = v->Call(closure, nArgs, v->_top-nArgs, tmp, SQ_BASELIB_INVOKE_CB_ERR_HANDLER);
        v->Pop(2+nArgs);
        if (!callRes)
            return SQ_ERROR;
    }
    sq_pop(v, 1); // pops the iterator

    return 0;
}


static SQInteger container_findindex(HSQUIRRELVM v)
{
    const SQObjectPtr &o = stack_get(v,1);
    const SQObjectPtr &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 4);
    SQObjectPtr tmp;

    sq_pushnull(v);
    while (SQ_SUCCEEDED(sq_next(v, 1))) {
        SQInteger iterTop = sq_gettop(v);
        v->PushNull();
        v->Push(stack_get(v, iterTop));
        if (nArgs >= 3)
            v->Push(stack_get(v, iterTop-1));
        if (nArgs >= 4)
            v->Push(o);

        bool callRes = v->Call(closure, nArgs, v->_top-nArgs, tmp, SQ_BASELIB_INVOKE_CB_ERR_HANDLER);
        if (!callRes)
            return SQ_ERROR;

        if (!v->IsFalse(tmp)) {
            v->Push(stack_get(v, iterTop-1));
            return 1;
        }
        v->Pop(2+nArgs);
    }
    sq_pop(v, 1); // pops the iterator

    return 0;
}

static SQInteger container_findvalue(HSQUIRRELVM v)
{
    if (sq_gettop(v) > 3)
        return sq_throwerror(v, "Too many arguments for findvalue()");

    const SQObjectPtr &o = stack_get(v,1);
    const SQObjectPtr &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 4);
    SQObjectPtr tmp;

    sq_pushnull(v);
    while (SQ_SUCCEEDED(sq_next(v, 1))) {
        SQInteger iterTop = sq_gettop(v);

        v->PushNull();
        v->Push(stack_get(v, iterTop));
        if (nArgs >= 3)
            v->Push(stack_get(v, iterTop-1));
        if (nArgs >= 4)
            v->Push(o);

        bool callRes = v->Call(closure, nArgs, v->_top-nArgs, tmp, SQ_BASELIB_INVOKE_CB_ERR_HANDLER);
        if (!callRes)
            return SQ_ERROR;

        if (!v->IsFalse(tmp)) {
            v->Push(stack_get(v, iterTop));
            return 1;
        }
        v->Pop(2+nArgs);
    }
    sq_pop(v, 1); // pops the iterator

    return sq_gettop(v) > 2 ? 1 : 0;
}

static SQInteger container_hasindex(HSQUIRRELVM v)
{
    const SQObjectPtr &self = stack_get(v, 1);
    SQObjectType t = sq_type(self);
    if (t != OT_TABLE && t != OT_CLASS && t != OT_INSTANCE)
        return sq_throwerror(v, "hasindex not supported for this type");

    const SQObjectPtr &key = stack_get(v, 2);
    v->Push(self);
    v->Push(key);
    bool exists = SQ_SUCCEEDED(sq_rawget(v, -2));
    sq_pushbool(v, exists);
    return 1;
}

static SQInteger container_hasvalue(HSQUIRRELVM v)
{
    const SQObject &o = stack_get(v, 1);
    SQObjectType t = sq_type(o);
    if (t != OT_TABLE)
        return sq_throwerror(v, "hasvalue not supported for this type");

    const SQObjectPtr &value = stack_get(v, 2);
    bool found = false;

    SQTable *tbl = _table(o);
    SQObjectPtr itr, key, val;
    SQInteger nitr;
    while ((nitr = tbl->Next(false, itr, key, val)) != -1) {
        itr = (SQInteger)nitr;
        if (SQVM::IsEqual(val, value)) {
            found = true;
            break;
         }
    }
    sq_pushbool(v, found);
    return 1;
}


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
    // container type check is done by the typemask
    if (SQ_FAILED(sq_rawget(v, 1)))
        return sq_throwerror(v,"the index doesn't exist");
    return 1;
}


static SQInteger table_filter(HSQUIRRELVM v)
{
    const SQObjectPtr &o = stack_get(v,1);
    SQTable *tbl = _table(o);
    const SQObjectPtr &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 4);

    SQObjectPtr ret(SQTable::Create(_ss(v),0));
    SQObjectPtr temp;
    SQObjectPtr itr, key, val;
    SQInteger nitr;
    while((nitr = tbl->Next(false, itr, key, val)) != -1) {
        itr = (SQInteger)nitr;

        v->PushNull();
        v->Push(val);
        if (nArgs >= 3)
            v->Push(key);
        if (nArgs >= 4)
            v->Push(o);

        bool callRes = v->Call(closure, nArgs, v->_top - nArgs, temp, SQ_BASELIB_INVOKE_CB_ERR_HANDLER);
        v->Pop(nArgs);
        if (!callRes)
            return SQ_ERROR;

        if (!SQVM::IsFalse(temp)) {
            _table(ret)->NewSlot(key, val);
        }
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
    v->Push(SQObjectPtr(a)); \
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
            result->Set(n, SQObjectPtr(item));
            n++;
        }
    }
    v->Push(SQObjectPtr(result));
    return 1;
}

static SQInteger swap(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;
    SQObjectPtr &o = stack_get(v, 1);
    SQObjectPtr &key1 = stack_get(v, 2);
    SQObjectPtr &key2 = stack_get(v, 3);
    switch(sq_type(o)) {
        case OT_ARRAY: {
            SQArray *arr = _array(o);
            if (!sq_isnumeric(key1) || !sq_isnumeric(key2))
                return sq_throwerror(v,"invalid index type for an array");

            const int asize = arr->Size();
            int k1 = tointeger(key1);
            k1 = k1 >= 0 ? k1 : asize + k1;
            int k2 = tointeger(key2);
            k2 = k2 >= 0 ? k2 : asize + k2 ;
            if( k1 >= asize || k2 >= asize || k1 < 0 || k2 < 0)
                return sq_throwerror(v,"index is out of range");

            _Swap(_array(o)->_values[k1], _array(o)->_values[k2]);
            break;
        }
        case OT_TABLE: {
            SQObjectPtr val1, val2;
            if (!_table(o)->Get(key1, val1) || !_table(o)->Get(key2, val2))
                sq_throwerror(v,"the index doesn't exist");

            _table(o)->Set(key1, val2);
            _table(o)->Set(key2, val1);
            break;
        }
        case OT_INSTANCE: {
            SQObjectPtr val1, val2;
            if (!_instance(o)->Get(key1, val1) || !_instance(o)->Get(key2, val2))
                return sq_throwerror(v,"the index doesn't exist");

            _instance(o)->Set(key1, val2);
            _instance(o)->Set(key2, val1);
            break;
        }
        default:
            return sq_throwerror(v,"swap works only on array/table/instance");
    }
    v->Push(o);
    return 1;
}

static SQInteger array_to_table(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v, 1);
    SQArray *arr = _array(o);
    SQInteger count = arr->Size();
    SQTable *result = SQTable::Create(_ss(v), count);

    if (count == 0)
    {
        v->Push(SQObjectPtr(result));
        return 1;
    }

    SQObjectPtr item;
    bool expectArrays = arr->Get(0, item) && sq_isarray(item);

    for (SQInteger i = 0; i < count; i++)
    {
        SQObjectPtr key;
        SQObjectPtr val;

        if (!arr->Get(i, item))
            return sq_throwerror(v, "totable() failed to get element from array");

        bool isSimpleType = sq_isstring(item) || sq_isnumeric(item) || sq_isbool(item) || sq_isnull(item);

        if (expectArrays && sq_isarray(item) && _array(item)->Get(0, key) && _array(item)->Get(1, val) && _array(item)->Size() == 2)
            result->NewSlot(key, val);
        else if (!expectArrays && isSimpleType)
            result->NewSlot(item, item);
        else
        {
            if (expectArrays && sq_isarray(item) && _array(item)->Size() != 2)
                return sq_throwerror(v, "totable() expected array of pairs [[key, value], ...], size of the each pair array must be exactly 2 elements");
            else
                return sq_throwerror(v, "totable() expected array of pairs [[key, value], ...] or array of simple types [\"key1\", \"key2\", ...]");
        }
    }

    v->Push(SQObjectPtr(result));
    return 1;
}


static SQInteger __map_table(SQTable *dest, SQTable *src, HSQUIRRELVM v) {
    const SQObjectPtr &closure = stack_get(v, 2);

    SQInteger nArgs = get_allowed_args_count(closure, 4);
    SQObjectPtr itr, key, val;
    SQInteger nitr;
    SQObjectPtr temp;
    SQObjectPtr srcObj(src);
    while ((nitr = src->Next(false, itr, key, val)) != -1) {
        itr = (SQInteger)nitr;

        v->PushNull();
        v->Push(val);
        if (nArgs >= 3)
            v->Push(key);
        if (nArgs >= 4)
            v->Push(srcObj);

        bool callRes = v->Call(closure, nArgs, v->_top - nArgs, temp, SQ_BASELIB_INVOKE_CB_ERR_HANDLER);
        v->Pop(nArgs);
        if (!callRes) {
            if (sq_isnull(v->_lasterror))
                continue;
            else
                return SQ_ERROR;
        }

        dest->NewSlot(key, temp);
    }

    return 0;
}

static SQInteger table_map(HSQUIRRELVM v)
{
    SQObject &o = stack_get(v, 1);
    SQObjectPtr ret(SQTable::Create(_ss(v), _table(o)->CountUsed()));
    if(SQ_FAILED(__map_table(_table(ret), _table(o), v)))
        return SQ_ERROR;
    v->Push(ret);
    return 1;
}


static SQInteger table_reduce(HSQUIRRELVM v)
{
    const SQObjectPtr &o = stack_get(v,1);
    SQTable *tbl = _table(o);
    const SQObjectPtr &closure = stack_get(v, 2);

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
            v->PushNull();
            v->Push(accum);
            v->Push(val);
            if (nArgs >= 4)
                v->Push(key);
            if (nArgs >= 5)
                v->Push(o);

            bool callRes = v->Call(closure, nArgs, v->_top - nArgs, accum, SQ_BASELIB_INVOKE_CB_ERR_HANDLER);
            v->Pop(nArgs);
            if (!callRes)
                return SQ_ERROR;
        }
    }

    if (!gotAccum)
        return 0;

    v->Push(accum);
    return 1;
}

static SQInteger table_replace_with(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;

    SQTable *dst = _table(stack_get(v, 1));
    dst->Clear(false);

    // Alternatively, consider copying hash nodes directly instead of iterating slots
    sq_pushnull(v);
    while (SQ_SUCCEEDED(sq_next(v, 2)))
        sq_newslot(v, 1, false);
    sq_pop(v, 1); // pop iterator

    v->Pop(1); // pop src, leave dst on stack as the return value
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

static SQInteger builtin_null_ctor(HSQUIRRELVM v);
static SQInteger builtin_integer_ctor(HSQUIRRELVM v);
static SQInteger builtin_float_ctor(HSQUIRRELVM v);
static SQInteger builtin_bool_ctor(HSQUIRRELVM v);
static SQInteger builtin_string_ctor(HSQUIRRELVM v);
static SQInteger builtin_table_ctor(HSQUIRRELVM v);
static SQInteger builtin_weakref_ctor(HSQUIRRELVM v);

const SQRegFunction SQSharedState::_table_default_type_methods_funcz[]={
    {"constructor",builtin_table_ctor,1, NULL},
    {"len",default_type_method_len,1, "t", NULL, true},
    {"rawget",container_rawget,2, "t", NULL, true},
    {"rawset",container_rawset,3, "t"},
    {"rawdelete",table_rawdelete,2, "t"},
    {"rawin",container_rawexists,2, "t", NULL, true},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"tostring",default_type_method_tostring,1, "."},
    {"clear",obj_clear,1, "."},
    {"map",table_map,2, "tc"},
    {"filter",table_filter,2, "tc"},
    {"reduce",table_reduce, -2, "tc"},
    {"getfuncinfos",delegable_getfuncinfos,1, "t"},
    {"each",container_each,2, "tc"},
    {"findindex",container_findindex,2, "tc", NULL, true},
    {"findvalue",container_findvalue,-2, "tc.", NULL, true},
    {"keys",table_keys,1, "t", NULL, true},
    {"values",table_values,1, "t", NULL, true},
    {"__update",container_update, -2, "t|yt|y|x", NULL, false},
    {"__merge",container_merge, -2, "t|yt|y|x", NULL, true},
    {"topairs",table_to_pairs, 1, "t" },
    {"replace_with",table_replace_with, 2, "tt" },
    {"hasindex",container_hasindex,2, "t.", NULL, true},
    {"hasvalue",container_hasvalue,2, "t.", NULL, true},
    {"clone",obj_clone, 1, "." },
    {"is_frozen",obj_is_frozen, 1, "." },
    {"swap",swap, 3, "t.." },
    {NULL,(SQFUNCTION)0,0,NULL}
};

// Array type methods

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
        if (sq_type(stack_get(v,2+i)) != OT_ARRAY)
            return sq_throwerror(v, "only arrays can be used to extend array");
    }

    for (SQInteger i=0; i<n; ++i) {
        arr->Extend(_array(stack_get(v,2+i)));
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
    const SQObject &o=stack_get(v,1);
    if(_array(o)->Size()>0){
        v->Push(_array(o)->Top());
        return 1;
    }
    else return sq_throwerror(v,"top() on a empty array");
}

static SQInteger array_insert(HSQUIRRELVM v)
{
    const SQObject &o=stack_get(v,1);

    SQ_CHECK_IMMUTABLE_OBJ(o);

    SQObject &idx=stack_get(v,2);
    SQObject &val=stack_get(v,3);
    if(!_array(o)->Insert(tointeger(idx),val))
        return sq_throwerror(v,"index out of range");
    sq_pop(v,2);
    return 1;
}

static SQInteger array_remove(HSQUIRRELVM v)
{
    const SQObject &o = stack_get(v, 1);
    SQ_CHECK_IMMUTABLE_OBJ(o);

    SQObject &idx = stack_get(v, 2);
    if(!sq_isnumeric(idx)) return sq_throwerror(v, "wrong type");
    SQObjectPtr val;
    if(_array(o)->Get(tointeger(idx), val)) {
        _array(o)->Remove(tointeger(idx));
        v->Push(val);
        return 1;
    }
    return sq_throwerror(v, "idx out of range");
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
          return sq_throwerror(v, "resizing to negative length");

        if(sq_gettop(v) > 2)
            fill = stack_get(v, 3);
        _array(o)->Resize(sz,fill);
        sq_settop(v, 1);
        return 1;
    }
    return sq_throwerror(v, "size must be a number");
}

static SQInteger __map_array(SQArray *dest,SQArray *src,HSQUIRRELVM v, bool append) {
    SQObjectPtr temp;
    SQObjectPtr srcObj(src);
    SQInteger size = src->Size();
    const SQObjectPtr &closure = stack_get(v, 2);

    SQInteger nArgs = get_allowed_args_count(closure, 4);
    for(SQInteger n = 0; n < size; n++) {
        src->Get(n,temp);
        v->PushNull();
        v->Push(temp);
        if (nArgs >= 3)
            v->Push(SQObjectPtr(n));
        if (nArgs >= 4)
            v->Push(srcObj);

        bool callRes = v->Call(closure, nArgs, v->_top - nArgs, temp, SQ_BASELIB_INVOKE_CB_ERR_HANDLER);
        v->Pop(nArgs);
        if (!callRes) {
            if (append && sq_isnull(v->_lasterror)) {
                continue;
            }
            return SQ_ERROR;
        }

        if (append)
            dest->Append(temp);
        else
            dest->Set(n, temp);
    }
    return 0;
}

static SQInteger array_map(HSQUIRRELVM v)
{
    const SQObjectPtr &o = stack_get(v,1);
    SQInteger size = _array(o)->Size();
    SQArray *dest = SQArray::Create(_ss(v),0);
    dest->Reserve(size);
    SQObjectPtr ret(dest);
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
    const SQObjectPtr &o = stack_get(v,1);
    SQArray *a = _array(o);
    SQInteger size = a->Size();
    SQObjectPtr accum;
    SQInteger iterStart;
    if (sq_gettop(v)>2) {
        accum = stack_get(v,3);
        iterStart = 0;
    } else if (size==0) {
        return 0;
    } else {
        a->Get(0, accum);
        iterStart = 1;
    }

    const SQObjectPtr &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 5);

    if (size > iterStart) {
        SQObjectPtr other;
        for (SQInteger n = iterStart; n < size; n++) {
            a->Get(n,other);
            v->PushNull();
            v->Push(accum);
            v->Push(other);
            if (nArgs >= 4)
                v->Push(SQObjectPtr(n));
            if (nArgs >= 5)
                v->Push(o);

            bool callRes = v->Call(closure, nArgs, v->_top - nArgs, accum, SQ_BASELIB_INVOKE_CB_ERR_HANDLER);
            v->Pop(nArgs);
            if (!callRes)
                return SQ_ERROR;
        }
    }
    v->Push(accum);
    return 1;
}

static SQInteger array_filter(HSQUIRRELVM v)
{
    const SQObjectPtr &o = stack_get(v,1);
    SQArray *a = _array(o);
    const SQObjectPtr &closure = stack_get(v, 2);
    SQInteger nArgs = get_allowed_args_count(closure, 4);

    SQObjectPtr ret(SQArray::Create(_ss(v),0));
    SQInteger size = a->Size();
    SQObjectPtr val, temp;

    for(SQInteger n = 0; n < size; n++) {
        a->Get(n,val);
        v->PushNull();
        v->Push(val);
        if (nArgs >= 3)
            v->Push(SQObjectPtr(n));
        if (nArgs >= 4)
            v->Push(o);

        bool callRes = v->Call(closure, nArgs, v->_top - nArgs, temp, SQ_BASELIB_INVOKE_CB_ERR_HANDLER);
        v->Pop(nArgs);
        if (!callRes)
            return SQ_ERROR;

        if(!SQVM::IsFalse(temp)) {
            _array(ret)->Append(val);
        }
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
        a->Get(n,temp);
        if(SQVM::IsEqual(temp,val))
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


static bool _sort_compare(HSQUIRRELVM v, SQArray *arr, SQObjectPtr &a, SQObjectPtr &b,
                          const SQObjectPtr &func, SQInteger &ret)
{
    if (sq_isnull(func)) {
        if (!v->ObjCmp(a,b,ret))
            return false;
    }
    else {
        SQInteger top = sq_gettop(v);
        v->PushNull();
        v->Push(a);
        v->Push(b);
        SQObjectPtr *valptr = arr->_values._vals;
        SQUnsignedInteger precallsize = arr->_values.size();
        SQObjectPtr out;
        bool callSucceeded = v->Call(func, 3, v->_top-3, out, false);
        if (!callSucceeded) {
            if (!sq_isstring( v->_lasterror))
                v->Raise_Error("compare func failed");
            return false;
        }
        if (!sq_isnumeric(out)) {
            v->Raise_Error("numeric value expected as return value of the compare function");
            return false;
        }
        if (precallsize != arr->_values.size() || valptr != arr->_values._vals) {
            v->Raise_Error("array resized during sort operation");
            return false;
        }
        ret = tointeger(out);
        sq_settop(v, top);
        return true;
    }
    return true;
}

static bool _hsort_sift_down(HSQUIRRELVM v,SQArray *arr, SQInteger root, SQInteger bottom, const SQObjectPtr &func)
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
                v->Raise_Error("inconsistent compare function");
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

static bool _hsort(HSQUIRRELVM v,SQObjectPtr &arr, SQInteger SQ_UNUSED_ARG(l), SQInteger SQ_UNUSED_ARG(r),const SQObjectPtr &func)
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
    SQObjectPtr func;
    SQObjectPtr &o = stack_get(v,1);
    SQ_CHECK_IMMUTABLE_OBJ(o);

    if (_array(o)->Size() > 1) {
        if(sq_gettop(v) == 2)
            func = stack_get(v, 2);
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
      v->Push(SQObjectPtr(arr));
      return 1;
    }

    SQArray *arr=SQArray::Create(_ss(v),eidx-sidx);
    SQObjectPtr t;
    SQInteger count=0;
    for(SQInteger i=sidx;i<eidx;i++){
        _array(o)->Get(i,t);
        arr->Set(count++,t);
    }
    v->Push(SQObjectPtr(arr));
    return 1;

}

static SQInteger array_hasindex(HSQUIRRELVM v)
{
    const SQObjectPtr &o = stack_get(v, 1);
    SQInteger idx = tointeger(stack_get(v, 2));
    sq_pushbool(v, idx>=0 && idx < _array(o)->Size());
    return 1;
}

static SQInteger array_replace_with(HSQUIRRELVM v)
{
    SQ_CHECK_IMMUTABLE_SELF;

    SQArray *dst = _array(stack_get(v, 1));
    SQArray *src = _array(stack_get(v, 2));
    dst->_values.copy(src->_values);
    dst->ShrinkIfNeeded();
    VT_CLONE_FROM_TO(src, dst);
    v->Pop(1);
    return 1;
}

const SQRegFunction SQSharedState::_array_default_type_methods_funcz[]={
    {"constructor",builtin_array_ctor,-2, ".n."},
    {"len",default_type_method_len,1, "a", NULL, true},
    {"append",array_append,-2, "a."},
    {"extend",array_extend,-2, "aa"},
    {"pop",array_pop,1, "a"},
    {"top",array_top,1, "a"},
    {"insert",array_insert,3, "an."},
    {"remove",array_remove,2, "an"},
    {"resize",array_resize,-2, "an"},
    {"reverse",array_reverse,1, "a"},
    {"sort",array_sort,-1, "ac"},
    {"slice",array_slice,-1, "ann"},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"tostring",default_type_method_tostring,1, "."},
    {"clear",obj_clear,1, "."},
    {"map",array_map,2, "ac"},
    {"apply",array_apply,2, "ac"},
    {"reduce",array_reduce,-2, "ac."},
    {"filter",array_filter,2, "ac"},
    {"indexof",array_indexof,2, "a.", NULL, true},
    {"contains",array_contains,2, "a."},
    {"each",container_each,2, "ac"},
    {"findindex",container_findindex,2, "ac", NULL, true},
    {"findvalue",container_findvalue,-2, "ac.", NULL, true},
    {"totable",array_to_table,1, "a", NULL, true},
    {"replace",array_replace_with,2, "aa"}, // deprecated, use replace_with
    {"replace_with",array_replace_with,2, "aa"},
    {"hasindex",array_hasindex,2, "an", NULL, true},
    {"hasvalue",array_contains,2, "a.", NULL, true},
    {"clone",obj_clone, 1, "." },
    {"is_frozen",obj_is_frozen, 1, "." },
    {"swap",swap, 3, "ann" },
    {NULL,(SQFUNCTION)0,0,NULL}
};

// String type methods

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
      v->Push(SQObjectPtr(SQString::Create(_ss(v), "", -1)));
      return 1;
    }

    v->Push(SQObjectPtr(SQString::Create(_ss(v),&_stringval(o)[sidx],eidx-sidx)));
    return 1;
}

static SQInteger string_hasindex(HSQUIRRELVM v)
{
    const SQObjectPtr &o = stack_get(v, 1);
    SQInteger idx = tointeger(stack_get(v, 2));
    sq_pushbool(v, idx >= 0 && idx < _string(o)->_len);
    return 1;
}

static SQInteger _string_scan_for_substring(HSQUIRRELVM v, SQInteger (*push_result)(HSQUIRRELVM v, SQInteger index))
{
    SQInteger top,start_idx=0;
    const char *str,*substr,*ret;
    if(((top=sq_gettop(v))>1) && SQ_SUCCEEDED(sq_getstring(v,1,&str)) && SQ_SUCCEEDED(sq_getstring(v,2,&substr))){
        if (sq_getsize(v,2)<1)
            return sq_throwerror(v, "empty substring");
        if(top>2)sq_getinteger(v,3,&start_idx);
        if((sq_getsize(v,1)>start_idx) && (start_idx>=0)){
            ret=strstr(&str[start_idx],substr);
            if(ret)
                return push_result(v, ret-str);
        }
        return push_result(v, -1);
    }
    return sq_throwerror(v,"invalid param");
}

static SQInteger string_indexof(HSQUIRRELVM v)
{
    return _string_scan_for_substring(v, _push_scan_index);
}

static SQInteger string_contains(HSQUIRRELVM v)
{
    return _string_scan_for_substring(v, _push_scan_found_flag);
}

static char* replace_all(SQAllocContext allocctx, char *s, SQInteger &buf_len, SQInteger &len,
                          const char *from, SQInteger len_from, const char *to, SQInteger len_to)
{
    for (SQInteger pos=0; pos<=len-len_from; ) {
        if (memcmp(s+pos, from, len_from*sizeof(char))==0) {
            SQInteger d_size = len_to - len_from;
            if (d_size > 0) {
                s = (char*)sq_realloc(allocctx, s, buf_len*sizeof(char), (buf_len+d_size)*sizeof(char));
                buf_len += d_size;
            }
            if (d_size!=0)
                memmove(s+pos+len_to, s+pos+len_from, (len-pos-len_from)*sizeof(char));
            memcpy(s+pos, to, len_to*sizeof(char));
            len += d_size;
            pos += len_to;
        }
        else
            ++pos;
    }
    return s;
}

static char* replace_substring_internal(SQAllocContext allocctx, char *s, SQInteger &buf_len, SQInteger &len,
    int pos, SQInteger len_from, const char *to, SQInteger len_to)
{
    SQInteger d_size = len_to - len_from;
    if (d_size > 0) {
        s = (char*)sq_realloc(allocctx, s, buf_len * sizeof(char), (buf_len + d_size) * sizeof(char));
        buf_len += d_size;
    }
    if (d_size != 0)
        memmove(s + pos + len_to, s + pos + len_from, (len - pos - len_from) * sizeof(char));
    memcpy(s + pos, to, len_to * sizeof(char));
    len += d_size;

    return s;
}


static SQInteger string_substitute(HSQUIRRELVM v)
{
    SQAllocContext allocctx = _ss(v)->_alloc_ctx;
    const char *fmt;
    SQInteger len;
    sq_getstringandsize(v, 1, &fmt, &len);
    SQInteger buf_len = len;
    char *s = (char *)sq_malloc(allocctx, len*sizeof(char));
    memcpy(s, fmt, len * sizeof(char));

    SQInteger top = sq_gettop(v);

    for (int i = 0; i < int(len) - 2; i++)
        if (s[i] == '{') {
            int depth = 0;
            for (int j = i + 1; j < len; j++) {
                if (s[j] == '}') {
                    depth--;
                    if (depth < 0) {
                        if (i + 1 == j)
                          break;

                        int index = 0;
                        for (int k = i + 1; k < j; k++)
                            if (s[k] >= '0' && s[k] <= '9')
                                index = index * 10 + s[k] - '0';
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
                                    sq_free(allocctx, s, buf_len * sizeof(char));
                                    return sq_throwerror(v, "subst: Failed to convert value to string");
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
                                        sq_free(allocctx, s, buf_len * sizeof(char));
                                        return sq_throwerror(v, "subst: Failed to convert value to string");
                                    }
                                }
                            }
                        }

                        break; // depth < 0
                    }
                }
                else if (s[j] == '{')
                  depth++;
            }
        }

    sq_pushstring(v, s, len);
    sq_free(allocctx, s, buf_len * sizeof(char));
    return 1;
}


static SQInteger string_replace(HSQUIRRELVM v)
{
    const char *s, *from, *to;
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
    char *dest = (char *)sq_malloc(_ss(v)->_alloc_ctx, buf_len*sizeof(char));
    SQInteger len_dest = len_s;
    memcpy(dest, s, len_s*sizeof(char));
    dest = replace_all(_ss(v)->_alloc_ctx, dest, buf_len, len_dest, from, len_from, to, len_to);

    sq_pushstring(v, dest, len_dest);
    sq_free(_ss(v)->_alloc_ctx, dest, buf_len*sizeof(char));
    return 1;
}

static SQInteger buf_concat(sqvector<char> &res, const SQObjectPtrVec &strings, const char *sep, SQInteger sep_len)
{
    SQInteger out_pos = 0;
    for (SQInteger i=0, nitems=strings.size(); i<nitems; ++i) {
        if (i) {
            memcpy(res._vals+out_pos, sep, sep_len*sizeof(char));
            out_pos += sep_len;
        }

        SQString *s = _string(strings[i]);
        memcpy(res._vals+out_pos, s->_val, s->_len*sizeof(char));
        out_pos += s->_len;
    }
    return out_pos;
}

static SQInteger string_join(HSQUIRRELVM v)
{
    const char *sep;
    SQInteger sep_len;
    sq_getstringandsize(v, 1, &sep, &sep_len);
    SQArray *arr = _array(stack_get(v, 2));

    SQObjectPtrVec strings(_ss(v)->_alloc_ctx);
    strings.reserve(arr->Size());
    SQObjectPtr tmp;
    SQObjectPtr flt;

    if (sq_gettop(v) > 3)
        return sq_throwerror(v, "Too many arguments");

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
            return sq_throwerror(v, "Failed to convert array item to string");

        strings.push_back(tmp);
        res_len += _string(tmp)->_len;
    }

    if (strings.empty()) {
        sq_pushstring(v, "", 0);
        return 1;
    }

    res_len += sep_len * (strings.size()-1);

    sqvector<char> res(_ss(v)->_alloc_ctx);
    res.resize(res_len);
    SQInteger out_pos = buf_concat(res, strings, sep, sep_len);
    (void)out_pos;
    assert(out_pos == res_len);
    sq_pushstring(v, res._vals, res_len);
    return 1;
}


static SQInteger string_concat(HSQUIRRELVM v)
{
    const char *sep;
    SQInteger sep_len;
    sq_getstringandsize(v, 1, &sep, &sep_len);
    SQInteger nitems = sq_gettop(v)-1;
    if (nitems < 1) {
        sq_pushstring(v, "", 0);
        return 1;
    }

    SQObjectPtrVec strings(_ss(v)->_alloc_ctx);
    strings.resize(nitems);

    SQInteger res_len = sep_len * (nitems-1);
    for (SQInteger i=0; i<nitems; ++i) {
        if (!v->ToString(stack_get(v, i+2), strings[i]))
            return sq_throwerror(v, "Failed to convert array item to string");
        res_len += _string(strings[i])->_len;
    }

    sqvector<char> res(_ss(v)->_alloc_ctx);
    res.resize(res_len);
    SQInteger out_pos = buf_concat(res, strings, sep, sep_len);
    (void)out_pos;
    assert(out_pos == res_len);
    sq_pushstring(v, res._vals, res_len);
    return 1;
}


static SQInteger string_split(HSQUIRRELVM v)
{
    const char *str;
    SQInteger len;
    sq_getstringandsize(v, 1, &str, &len);

    SQArray *res = SQArray::Create(_ss(v),0);

    if (sq_gettop(v) == 1) {
        SQInteger start = 0;
        for (SQInteger pos=0; pos<=len; ++pos) {
            if (pos==len || sq_isspace(str[pos])) {
                if (pos > start)
                    res->Append(SQObjectPtr(SQString::Create(_ss(v), str+start, pos-start)));
                start = pos+1;
            }
        }
    } else {
        const char *sep;
        SQInteger sep_len;
        sq_getstringandsize(v, 2, &sep, &sep_len);
        if (sep_len < 1)
            return sq_throwerror(v, "empty separator");
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
    v->Push(SQObjectPtr(res));
    return 1;
}


#define STRING_TOFUNCZ(func, call_func) static SQInteger string_##func(HSQUIRRELVM v) \
{\
    SQInteger sidx,eidx; \
    SQObjectPtr str; \
    if(SQ_FAILED(get_slice_params(v,sidx,eidx,str)))return -1; \
    SQInteger slen = _string(str)->_len; \
    if(sidx < 0)sidx = slen + sidx; \
    if(eidx < 0)eidx = slen + eidx; \
    if(eidx < sidx) return sq_throwerror(v,"wrong indexes"); \
    if(eidx > slen || sidx < 0) return sq_throwerror(v,"slice out of range"); \
    SQInteger len=_string(str)->_len; \
    const char *sthis=_stringval(str); \
    char *snew=(_ss(v)->GetScratchPad(len)); \
    memcpy(snew,sthis,len);\
    for(SQInteger i=sidx;i<eidx;i++) snew[i] = (char)call_func(sthis[i]); \
    v->Push(SQObjectPtr(SQString::Create(_ss(v),snew,len))); \
    return 1; \
}


STRING_TOFUNCZ(tolower, sq_tolower)
STRING_TOFUNCZ(toupper, sq_toupper)

#define IMPL_STRING_FUNC(name) static SQInteger _baselib_string_##name(HSQUIRRELVM v) { return _sq_string_ ## name ## _impl(v, 1); }

IMPL_STRING_FUNC(strip)
IMPL_STRING_FUNC(lstrip)
IMPL_STRING_FUNC(rstrip)
IMPL_STRING_FUNC(split_by_chars)
IMPL_STRING_FUNC(escape)
IMPL_STRING_FUNC(startswith)
IMPL_STRING_FUNC(endswith)

#undef IMPL_STRING_FUNC
#define _DECL_FUNC(name,nparams,pmask) {#name,_baselib_string_##name,nparams,pmask}

const SQRegFunction SQSharedState::_string_default_type_methods_funcz[]={
    {"constructor",builtin_string_ctor,2, NULL},
    {"len",default_type_method_len,1, "s", NULL, true},
    {"tointeger",default_type_method_tointeger,-1, "sn", NULL, true},
    {"tofloat",default_type_method_tofloat,1, "s", NULL, true},
    {"tostring",default_type_method_tostring,1, ".", NULL, true},
    {"hash",string_hash, 1, "s", NULL, true},
    {"slice",string_slice,-1, "s n  n", NULL, true},
    {"indexof",string_indexof,-2, "s s n", NULL, true},
    {"contains",string_contains,-2, "s s n", NULL, true},
    {"hasindex",string_hasindex,-2, "s n", NULL, true},
    {"tolower",string_tolower,-1, "s n n", NULL, true},
    {"toupper",string_toupper,-1, "s n n", NULL, true},
    {"subst",string_substitute,-2, "s", NULL, true},
    {"replace",string_replace, 3, "s s s", NULL, true},
    {"join",string_join, -2, "s a b|c", NULL, true},
    {"concat",string_concat, -2, "s.", NULL, true},
    {"split",string_split, -1, "s s", NULL, true},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"clone",obj_clone, 1, "." },
    _DECL_FUNC(strip,1,"s"),
    _DECL_FUNC(lstrip,1,"s"),
    _DECL_FUNC(rstrip,1,"s"),
    _DECL_FUNC(split_by_chars,-2,"ssb"),
    _DECL_FUNC(escape,1,"s"),
    _DECL_FUNC(startswith,2,"ss"),
    _DECL_FUNC(endswith,2,"ss"),
    {NULL,(SQFUNCTION)0,0,NULL}
};

#undef _DECL_FUNC

static SQInteger builtin_integer_ctor(HSQUIRRELVM v)
{
    SQInteger nparams = sq_gettop(v);
    if (nparams == 1) {
        v->Push(SQObjectPtr((SQInteger)0));
        return 1;
    }

    SQObjectPtr &o = stack_get(v, 2);
    SQInteger base = 10;
    if (nparams > 2) {
        sq_getinteger(v, 3, &base);
    }

    switch (sq_type(o)) {
        case OT_INTEGER:
            v->Push(o);
            return 1;
        case OT_FLOAT:
            v->Push(SQObjectPtr(tointeger(o)));
            return 1;
        case OT_STRING: {
            SQObjectPtr res;
            if (sq_parse_int(_stringval(o), _stringval(o) + _string(o)->_len, res, base)) {
                v->Push(SQObjectPtr(tointeger(res)));
                return 1;
            }
            return sq_throwerror(v, "cannot convert string to Integer");
        }
        case OT_BOOL:
            v->Push(SQObjectPtr(_integer(o) ? (SQInteger)1 : (SQInteger)0));
            return 1;
        default:
            return sq_throwerror(v, "cannot convert to Integer");
    }
}

static SQInteger builtin_float_ctor(HSQUIRRELVM v)
{
    SQInteger nparams = sq_gettop(v);
    if (nparams == 1) {
        v->Push(SQObjectPtr((SQFloat)0.0));
        return 1;
    }

    SQObjectPtr &o = stack_get(v, 2);
    switch (sq_type(o)) {
        case OT_FLOAT:
            v->Push(o);
            return 1;
        case OT_INTEGER:
            v->Push(SQObjectPtr(tofloat(o)));
            return 1;
        case OT_STRING: {
            SQObjectPtr res;
            if (sq_parse_float(_stringval(o), _stringval(o) + _string(o)->_len, res, 10)) {
                v->Push(SQObjectPtr(tofloat(res)));
                return 1;
            }
            return sq_throwerror(v, "cannot convert string to Float");
        }
        case OT_BOOL:
            v->Push(SQObjectPtr((SQFloat)(_integer(o) ? 1.0 : 0.0)));
            return 1;
        default:
            return sq_throwerror(v, "cannot convert to Float");
    }
}

static SQInteger builtin_bool_ctor(HSQUIRRELVM v)
{
    SQInteger nparams = sq_gettop(v);
    if (nparams == 1) {
        v->Push(SQObjectPtr(false));
        return 1;
    }

    v->Push(SQObjectPtr(!SQVM::IsFalse(stack_get(v, 2))));
    return 1;
}

static SQInteger builtin_string_ctor(HSQUIRRELVM v)
{
    SQObjectPtr res;
    if (v->ToString(stack_get(v, 2), res)) {
        v->Push(res);
        return 1;
    }
    return sq_throwerror(v, "cannot convert to String");
}

static SQInteger builtin_table_ctor(HSQUIRRELVM v)
{
    SQTable *tbl = SQTable::Create(_ss(v), 0);
    v->Push(SQObjectPtr(tbl));
    return 1;
}

static SQInteger builtin_null_ctor(HSQUIRRELVM v)
{
    return 0;
}

static SQInteger builtin_weakref_ctor(HSQUIRRELVM v)
{
    sq_weakref(v, 2);
    return 1;
}

// Null type methods
const SQRegFunction SQSharedState::_null_default_type_methods_funcz[]={
    {"constructor",builtin_null_ctor,-1, NULL},
    {NULL,(SQFUNCTION)0,0,NULL}
};


// Integer type methods
const SQRegFunction SQSharedState::_integer_default_type_methods_funcz[]={
    {"constructor",builtin_integer_ctor,-1, NULL},
    {"tointeger",default_type_method_tointeger,1, "n|b", NULL, true},
    {"tofloat",default_type_method_tofloat,1, "n|b", NULL, true},
    {"tostring",default_type_method_tostring,1, ".", NULL, true},
    {"tochar",number_type_method_tochar,1, "n|b", NULL, true},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"clone",obj_clone, 1, "." },
    {NULL,(SQFUNCTION)0,0,NULL}
};

// Float type methods
const SQRegFunction SQSharedState::_float_default_type_methods_funcz[]={
    {"constructor",builtin_float_ctor,-1, NULL},
    {"tointeger",default_type_method_tointeger,1, "n|b", NULL, true},
    {"tofloat",default_type_method_tofloat,1, "n|b", NULL, true},
    {"tostring",default_type_method_tostring,1, ".", NULL, true},
    {"tochar",number_type_method_tochar,1, "n|b", NULL, true},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"clone",obj_clone, 1, "." },
    {NULL,(SQFUNCTION)0,0,NULL}
};

// Bool type methods
const SQRegFunction SQSharedState::_bool_default_type_methods_funcz[]={
    {"constructor",builtin_bool_ctor,-1, NULL},
    {"tointeger",default_type_method_tointeger,1, "n|b", NULL, true},
    {"tofloat",default_type_method_tofloat,1, "n|b", NULL, true},
    {"tostring",default_type_method_tostring,1, ".", NULL, true},
    {"tochar",number_type_method_tochar,1, "n|b", NULL, true},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"clone",obj_clone, 1, "." },
    {NULL,(SQFUNCTION)0,0,NULL}
};

// Closure type methods
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
    const char *name = sq_getfreevariable(v, 1, SQUnsignedInteger(varidx));
    if (!name)
        return sq_throwerror(v, "Invalid free variable index");
    SQObjectPtr val = v->PopGet();
    SQTable *res = SQTable::Create(_ss(v),2);
    res->NewSlot(SQObjectPtr(SQString::Create(_ss(v),"name",-1)), SQObjectPtr(SQString::Create(_ss(v),name,-1)));
    res->NewSlot(SQObjectPtr(SQString::Create(_ss(v),"value",-1)), val);
    v->Push(SQObjectPtr(res));
    return 1;
}


static SQInteger closure_getfuncinfos_obj(HSQUIRRELVM v, SQObjectPtr & o) {
    SQTable *res = SQTable::Create(_ss(v), 20);

    #define SET_SLOT(name, value) \
        res->NewSlot(SQObjectPtr(SQString::Create(_ss(v), name, -1)), SQObjectPtr(value))

    if(sq_type(o) == OT_CLOSURE) {
        SQFunctionProto *f = _closure(o)->_function;
        SQInteger nparams = f->_nparameters - (f->_varparams ? 1 : 0);
        SQObjectPtr params(SQArray::Create(_ss(v),nparams));
        SQObjectPtr typecheck(SQArray::Create(_ss(v),nparams));
        SQObjectPtr defparams(SQArray::Create(_ss(v),f->_ndefaultparams));

        for(SQInteger n = 0; n < nparams; n++) {
            _array(params)->Set(n, f->_parameters[n]);
            _array(typecheck)->Set(n, SQObjectPtr(int(f->_param_type_masks[n])));
        }
        for(SQInteger j = 0; j < f->_ndefaultparams; j++) {
            _array(defparams)->Set((SQInteger)j,_closure(o)->_defaultparams[j]);
        }
        SET_SLOT("native", false);
        SET_SLOT("pure", f->_purefunction);
        SET_SLOT("nodiscard", f->_nodiscard);
        SET_SLOT("name", f->_name);
        SET_SLOT("freevars", f->_noutervalues);
        SET_SLOT("src", f->_sourcename);
        SET_SLOT("line", SQInteger(f->_lineinfos->_first_line));
        SET_SLOT("parameters", params);
        SET_SLOT("defparams", defparams);
        SET_SLOT("required_params", nparams - f->_ndefaultparams);
        SET_SLOT("varargs", bool(f->_varparams));
        SET_SLOT("typecheck", typecheck);
        SET_SLOT("return_type_mask", SQObjectPtr(int(f->_result_type_mask)));
        SET_SLOT("varargs_type_mask", SQObjectPtr(int(f->_varparams ? f->_param_type_masks[f->_nparameters - 1] : 0)));

        SQObjectPtr key;
        SQObjectPtr docObject;
        key._type = OT_USERPOINTER;
        key._unVal.pUserPointer = (void *)_closure(o)->_function;
        _table(_ss(v)->doc_objects)->Get(key, docObject);
        SET_SLOT("doc", docObject);
    }
    else { //OT_NATIVECLOSURE
        SQNativeClosure *nc = _nativeclosure(o);

        int requiredParams = nc->_nparamscheck == 0 ? 1 : abs(nc->_nparamscheck);
        bool varargs = nc->_nparamscheck <= 0;
        SQInteger varargsTypeMask = varargs ? -1 : 0;


        SQObjectPtr parameters;
        SQObjectPtr defparams;
        SQObjectPtr typecheck;
        SQObjectPtr key;
        SQObjectPtr value;
        key._type = OT_USERPOINTER;
        key._unVal.pUserPointer = (void *)((size_t)(void *)nc->_function ^ ~size_t(0));
        if (_table(_ss(v)->doc_objects)->Get(key, value)) {
            SQFunctionType ft(_ss(v));
            SQInteger errorPos = -1;
            SQObjectPtr errorString;
            if (sq_isstring(value)) {
               if (sq_parse_function_type_string(v, _stringval(value), ft, errorPos, errorString)) {

                   parameters = SQObjectPtr(SQArray::Create(_ss(v), ft.argNames.size() + 1));
                   _array(parameters)->Set(SQInteger(0), SQObjectPtr(SQString::Create(_ss(v), "this", -1)));
                   for (SQUnsignedInteger n = 0; n < ft.argNames.size(); n++) {
                       _array(parameters)->Set((SQInteger)n + 1, SQObjectPtr(ft.argNames[n]));
                   }

                   int defParamsCount = ft.argNames.size() - ft.requiredArgs;
                   defparams = SQObjectPtr(SQArray::Create(_ss(v), defParamsCount));
                   for (SQUnsignedInteger n = 0; n < defParamsCount; n++) {
                       _array(parameters)->Set((SQInteger)n, SQObjectPtr());
                   }

                   varargs = ft.ellipsisArgTypeMask != 0;
                   varargsTypeMask = int(ft.ellipsisArgTypeMask);
               }
               else {
                   // TODO: raise errorString
               }
            }
        }
        else {
            parameters = SQObjectPtr(SQArray::Create(_ss(v), requiredParams));

            _array(parameters)->Set(SQInteger(0), SQObjectPtr(SQString::Create(_ss(v), "this", -1)));
            for (SQInteger n = 1; n < requiredParams; n++) {
                char buf[16] = { 0 };
                snprintf(buf, sizeof(buf), "arg%d", int(n));
                _array(parameters)->Set((SQInteger)n, SQObjectPtr(SQString::Create(_ss(v), buf, -1)));
            }

            defparams = SQObjectPtr(SQArray::Create(_ss(v), 0));
        }

        typecheck = SQObjectPtr(SQArray::Create(_ss(v), nc->_typecheck.size()));
        for (SQUnsignedInteger n = 0; n < nc->_typecheck.size(); n++) {
            _array(typecheck)->Set((SQInteger)n, SQObjectPtr(int(nc->_typecheck[n])));
        }

        SET_SLOT("native", true);
        SET_SLOT("pure", nc->_purefunction);
        SET_SLOT("nodiscard", nc->_nodiscard);
        SET_SLOT("name", nc->_name);
        SET_SLOT("freevars", SQInteger(nc->_noutervalues));
        SET_SLOT("src", SQObjectPtr());
        SET_SLOT("line", SQObjectPtr());
        SET_SLOT("parameters", parameters);
        SET_SLOT("defparams", defparams);
        SET_SLOT("required_params", requiredParams);
        SET_SLOT("paramscheck", nc->_nparamscheck);
        SET_SLOT("varargs", varargs);
        SET_SLOT("typecheck", typecheck);
        SET_SLOT("return_type_mask", SQObjectPtr(int(nc->_result_type_mask)));
        SET_SLOT("varargs_type_mask", varargsTypeMask);

        SQObjectPtr docObject;
        key._unVal.pUserPointer = (void *)nc->_function;
        _table(_ss(v)->doc_objects)->Get(key, docObject);
        SET_SLOT("doc", docObject);
    }

    #undef SET_SLOT

    v->Push(SQObjectPtr(res));
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


const SQRegFunction SQSharedState::_closure_default_type_methods_funcz[]={
    {"call",closure_call,-1, "c"},
    {"pcall",closure_pcall,-1, "c"},
    {"acall",closure_acall,2, "ca"},
    {"pacall",closure_pacall,2, "ca"},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"tostring",default_type_method_tostring,1, "."},
    {"bindenv",closure_bindenv,2, "c x|y|t"},
    {"getfuncinfos",closure_getfuncinfos,1, "c"},
    {"getfreevar",closure_getfreevar,2, "ci"},
    {"clone",obj_clone, 1, "." },
    {NULL,(SQFUNCTION)0,0,NULL}
};

// Generator type methods
static SQInteger generator_getstatus(HSQUIRRELVM v)
{
    SQObject &o=stack_get(v,1);
    switch(_generator(o)->_state){
        case SQGenerator::eSuspended:v->Push(SQObjectPtr(SQString::Create(_ss(v),"suspended")));break;
        case SQGenerator::eRunning:v->Push(SQObjectPtr(SQString::Create(_ss(v),"running")));break;
        case SQGenerator::eDead:v->Push(SQObjectPtr(SQString::Create(_ss(v),"dead")));break;
    }
    return 1;
}

const SQRegFunction SQSharedState::_generator_default_type_methods_funcz[]={
    {"getstatus",generator_getstatus,1, "g"},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"tostring",default_type_method_tostring,1, "."},
    {"clone",obj_clone, 1, "." },
    {NULL,(SQFUNCTION)0,0,NULL}
};

// Thead type methods
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
    return sq_throwerror(v,"wrong parameter");
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
                    return sq_throwerror(v,"cannot wakeup a idle thread");
                break;
                case SQ_VMSTATE_RUNNING:
                    return sq_throwerror(v,"cannot wakeup a running thread");
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
    return sq_throwerror(v,"wrong parameter");
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
                    return sq_throwerror(v,"cannot wakeup a idle thread");
                break;
                case SQ_VMSTATE_RUNNING:
                    return sq_throwerror(v,"cannot wakeup a running thread");
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
    return sq_throwerror(v,"wrong parameter");
}

static SQInteger thread_getstatus(HSQUIRRELVM v)
{
    SQObjectPtr &o = stack_get(v,1);
    switch(sq_getvmstate(_thread(o))) {
        case SQ_VMSTATE_IDLE:
            sq_pushstring(v,"idle",-1);
        break;
        case SQ_VMSTATE_RUNNING:
            sq_pushstring(v,"running",-1);
        break;
        case SQ_VMSTATE_SUSPENDED:
            sq_pushstring(v,"suspended",-1);
        break;
        default:
            return sq_throwerror(v,"internal VM error");
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
                sq_throwerror(v,"unknown error");
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
    return sq_throwerror(v,"wrong parameter");
}

const SQRegFunction SQSharedState::_thread_default_type_methods_funcz[] = {
    {"call", thread_call, -1, "v"},
    {"wakeup", thread_wakeup, -1, "v"},
    {"wakeupthrow", thread_wakeupthrow, -2, "v.b"},
    {"getstatus", thread_getstatus, 1, "v"},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"getstackinfos",thread_getstackinfos,2, "vn"},
    {"tostring",default_type_method_tostring,1, "."},
    {"clone",obj_clone, 1, "." },
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
        return sq_throwerror(v, "Unknown metamethod");

    SQClass *cls = nullptr;
    if (sq_gettype(v, 1) == OT_CLASS)
        cls = _class(stack_get(v, 1));
    else // instance
        cls = _instance(stack_get(v, 1))->_class;
    v->Push(cls->_metamethods[mmidx]);
    return 1;
}


static SQInteger class_lock(HSQUIRRELVM v)
{
    SQClass *cls = _class(stack_get(v, 1));
    if (!cls->isLocked()) {
        if (!cls->Lock(v))
            return SQ_ERROR; // propagate raised error
    }
    return 1;
}


const SQRegFunction SQSharedState::_class_default_type_methods_funcz[] = {
    {"rawget",container_rawget,2, "y", NULL, true},
    {"rawset",container_rawset,3, "y"},
    {"rawin",container_rawexists,2, "y", NULL, true},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"tostring",default_type_method_tostring,1, "."},
    {"instance",class_instance,1, "y"},
    {"getbase",class_getbase,1, "y", NULL, true},
    {"newmember",class_newmember,-3, "y"},
    {"getfuncinfos",class_getfuncinfos,1, "y"},
    {"call",closure_call,-1, "y"},
    {"pcall",closure_pcall,-1, "y"},
    {"acall",closure_acall,2, "ya"},
    {"pacall",closure_pacall,2, "ya"},
    {"__update",container_update, -2, "t|yt|y|x", NULL, false},
    {"__merge",container_merge, -2, "t|yt|y|x", NULL, true},
    {"getmetamethod",get_class_metamethod, 2, "ys"},
    {"hasindex",container_hasindex,2, "y.", NULL, true},
    {"clone",obj_clone, 1, "." },
    {"is_frozen",obj_is_frozen, 1, "." },
    {"lock",class_lock, 1, "y" },
    {NULL,(SQFUNCTION)0,0,NULL}
};


static SQInteger instance_getclass(HSQUIRRELVM v)
{
    if(SQ_SUCCEEDED(sq_getclass(v,1)))
        return 1;
    return SQ_ERROR;
}

const SQRegFunction SQSharedState::_instance_default_type_methods_funcz[] = {
    {"getclass", instance_getclass, 1, "x", NULL, true},
    {"rawget",container_rawget,2, "x", NULL, true},
    {"rawset",container_rawset,3, "x"},
    {"rawin",container_rawexists,2, "x", NULL, true},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"tostring",default_type_method_tostring,1, "."},
    {"getfuncinfos",delegable_getfuncinfos,1, "x"},
    {"getmetamethod",get_class_metamethod, 2, "xs"},
    {"hasindex",container_hasindex,2, "x.", NULL, true},
    {"clone",obj_clone, 1, "." },
    {"is_frozen",obj_is_frozen, 1, "." },
    {"swap",swap, 3, "x.." },
    {NULL,(SQFUNCTION)0,0,NULL}
};

static SQInteger weakref_ref(HSQUIRRELVM v)
{
    if(SQ_FAILED(sq_getweakrefval(v,1)))
        return SQ_ERROR;
    return 1;
}

const SQRegFunction SQSharedState::_weakref_default_type_methods_funcz[] = {
    {"constructor",builtin_weakref_ctor, 2, NULL},
    {"ref",weakref_ref,1, "r"},
    {"weakref",obj_type_method_weakref,1, NULL },
    {"tostring",default_type_method_tostring,1, "."},
    {"clone",obj_clone, 1, "." },
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQRegFunction SQSharedState::_userdata_default_type_methods_funcz[] = {
    {"getfuncinfos",delegable_getfuncinfos,1, "u"},
    {"clone",obj_clone, 1, "." },
    {"is_frozen",obj_is_frozen, 1, "." },
    {NULL,(SQFUNCTION)0,0,NULL}
};
