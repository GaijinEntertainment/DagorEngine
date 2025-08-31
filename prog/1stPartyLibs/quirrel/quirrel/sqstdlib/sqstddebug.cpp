#include <squirrel.h>
#include <sqstddebug.h>
#include <string.h>
#include <assert.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqobject.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>
#include <squirrel/sqtable.h>
#include <squirrel/sqclass.h>


static SQInteger debug_seterrorhandler(HSQUIRRELVM v)
{
  sq_seterrorhandler(v);
  return 0;
}

static SQInteger debug_setdebughook(HSQUIRRELVM v)
{
  sq_setdebughook(v);
  return 0;
}

extern SQInteger __sq_getcallstackinfos(HSQUIRRELVM v, SQInteger level);

static SQInteger debug_getstackinfos(HSQUIRRELVM v)
{
  SQInteger level;
  sq_getinteger(v, -1, &level);
  return __sq_getcallstackinfos(v, level);
}

static SQInteger debug_getlocals(HSQUIRRELVM v)
{
    SQInteger level = 1;
    SQBool includeInternal = false;

    if (sq_gettop(v) >= 2)
        sq_getinteger(v, 2, &level);
    if (sq_gettop(v) >= 3)
        sq_getbool(v, 3, &includeInternal);

    sq_newtable(v);
    const SQChar *name = NULL;

    SQInteger seq=0;
    SQInteger prevTop = sq_gettop(v);
    while ((name = sq_getlocal(v, level, seq))) {
        ++seq;
        if (!includeInternal && (name[0] == '@' || strcmp(name, "this")==0 || strcmp(name, "vargv")==0)) {
            sq_pop(v, 1);
            continue;
        }

        sq_pushstring(v, name, -1);
        sq_push(v, -2);
        sq_newslot(v, -4, SQFalse);
        sq_pop(v, 1);
    }
    return 1;
}


static SQInteger debug_get_stack_top(HSQUIRRELVM v)
{
    sq_pushinteger(v, sq_gettop(v));
    return 1;
}

#ifndef NO_GARBAGE_COLLECTOR
static SQInteger debug_collectgarbage(HSQUIRRELVM v)
{
    sq_pushinteger(v, sq_collectgarbage(v));
    return 1;
}
static SQInteger debug_resurrectunreachable(HSQUIRRELVM v)
{
    sq_resurrectunreachable(v);
    return 1;
}
#endif

static SQInteger debug_getbuildinfo(HSQUIRRELVM v)
{
  sq_newtable(v);
  sq_pushstring(v, _SC("version"), -1);
  sq_pushstring(v, SQUIRREL_VERSION, -1);
  sq_newslot(v, -3, SQFalse);
  sq_pushstring(v, _SC("charsize"), -1);
  sq_pushinteger(v, sizeof(SQChar));
  sq_newslot(v, -3, SQFalse);
  sq_pushstring(v, _SC("intsize"), -1);
  sq_pushinteger(v, sizeof(SQInteger));
  sq_newslot(v, -3, SQFalse);
  sq_pushstring(v, _SC("floatsize"), -1);
  sq_pushinteger(v, sizeof(SQFloat));
  sq_newslot(v, -3, SQFalse);
  sq_pushstring(v, _SC("gc"), -1);
#ifndef NO_GARBAGE_COLLECTOR
  sq_pushstring(v, _SC("enabled"), -1);
#else
  sq_pushstring(v, _SC("disabled"), -1);
#endif // NO_GARBAGE_COLLECTOR
  sq_newslot(v, -3, SQFalse);
  return 1;
}

static SQInteger debug_doc(HSQUIRRELVM v)
{
    HSQOBJECT subject;
    sq_getstackobj(v, 2, &subject);

    SQObjectPtr value;
    SQObjectPtr key;
    key._type = OT_USERPOINTER;
    switch(sq_type(subject))
    {
        case OT_CLOSURE:
            key._unVal.pUserPointer = (void *)_closure(subject)->_function;
            break;
        case OT_NATIVECLOSURE:
            key._unVal.pUserPointer = (void *)_nativeclosure(subject)->_function;
            break;
        case OT_INSTANCE:
            key._unVal.pUserPointer = (void *)_instance(subject)->_class;
            break;
        default:
            key._unVal.pUserPointer = subject._unVal.pUserPointer;
            break;
    }

    if (!_table(_ss(v)->doc_objects)->Get(key, value))
    {
        sq_pushnull(v);
        return 1;
    }

    sq_pushobject(v, value);
    return 1;
}

static const SQRegFunction debuglib_funcs[] = {
    {_SC("seterrorhandler"),debug_seterrorhandler,2, NULL},
    {_SC("setdebughook"),debug_setdebughook,2, NULL},
    {_SC("getstackinfos"),debug_getstackinfos,2, _SC(".n")},
    {_SC("getlocals"),debug_getlocals,-1, _SC(".nb")},
    {_SC("get_stack_top"),debug_get_stack_top,0, NULL},
    {_SC("doc"),debug_doc,2, _SC(".t|c|x|y"), _SC("Returns a documentation string for a function, class, or table")},
#ifndef NO_GARBAGE_COLLECTOR
    {_SC("collectgarbage"),debug_collectgarbage,0, NULL},
    {_SC("resurrectunreachable"),debug_resurrectunreachable,0, NULL},
#endif
    {_SC("getbuildinfo"), debug_getbuildinfo,1,NULL},
    {NULL,(SQFUNCTION)0,0,NULL}
};


SQRESULT sqstd_register_debuglib(HSQUIRRELVM v)
{
  SQInteger i = 0;
  while (debuglib_funcs[i].name != 0)
  {
    sq_pushstring(v, debuglib_funcs[i].name, -1);
    sq_newclosure(v, debuglib_funcs[i].f, 0);
    sq_setparamscheck(v, debuglib_funcs[i].nparamscheck, debuglib_funcs[i].typemask);
    sq_setnativeclosurename(v, -1, debuglib_funcs[i].name);
    if (debuglib_funcs[i].docstring)
      sq_setnativeclosuredocstring(v, -1, debuglib_funcs[i].docstring);
    sq_newslot(v, -3, SQFalse);
    i++;
  }
  return SQ_OK;
}
