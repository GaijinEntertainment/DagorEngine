/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sqstdsystem.h>

#define scgetenv getenv
#define scsystem system
#define scremove remove
#define screname rename

#if _TARGET_PC
static SQInteger _system_getenv(HSQUIRRELVM v)
{
    const SQChar *s;
    if(SQ_SUCCEEDED(sq_getstring(v,2,&s))){
        sq_pushstring(v,scgetenv(s),-1);
        return 1;
    }
    return 0;
}


static SQInteger _system_system(HSQUIRRELVM v)
{
    const SQChar *s;
    if(SQ_SUCCEEDED(sq_getstring(v,2,&s))){
        sq_pushinteger(v,scsystem(s));
        return 1;
    }
    return sq_throwerror(v,_SC("wrong param"));
}
#endif


static SQInteger _system_remove(HSQUIRRELVM v)
{
    const SQChar *s;
    sq_getstring(v,2,&s);
    if(scremove(s)==-1)
        return sq_throwerror(v,_SC("remove() failed"));
    return 0;
}

static SQInteger _system_rename(HSQUIRRELVM v)
{
    const SQChar *oldn,*newn;
    sq_getstring(v,2,&oldn);
    sq_getstring(v,3,&newn);
    if(screname(oldn,newn)==-1)
        return sq_throwerror(v,_SC("rename() failed"));
    return 0;
}



#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_system_##name,nparams,pmask}
static const SQRegFunction systemlib_funcs[]={
#if _TARGET_PC
    _DECL_FUNC(getenv,2,_SC(".s")),
    _DECL_FUNC(system,2,_SC(".s")),
#endif
    _DECL_FUNC(remove,2,_SC(".s")),
    _DECL_FUNC(rename,3,_SC(".ss")),
    {NULL,(SQFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC


SQRESULT sqstd_register_command_line_args(HSQUIRRELVM v, int argc, char ** argv)
{
    sq_pushroottable(v);
    sq_pushstring(v, _SC("__argv"), -1);
    sq_newarray(v, 0);
    for (int idx = 0; idx < argc; idx++)
    {
        sq_pushstring(v, argv[idx], -1);
        sq_arrayappend(v, -2);
    }
    sq_newslot(v, -3, SQFalse);
    sq_pop(v,1);

    return SQ_OK;
}


SQRESULT sqstd_register_systemlib(HSQUIRRELVM v)
{
    SQInteger i=0;
    while(systemlib_funcs[i].name!=0)
    {
        sq_pushstring(v,systemlib_funcs[i].name,-1);
        sq_newclosure(v,systemlib_funcs[i].f,0);
        sq_setparamscheck(v,systemlib_funcs[i].nparamscheck,systemlib_funcs[i].typemask);
        sq_setnativeclosurename(v,-1,systemlib_funcs[i].name);
        sq_newslot(v,-3,SQFalse);
        i++;
    }
    return SQ_OK;
}
