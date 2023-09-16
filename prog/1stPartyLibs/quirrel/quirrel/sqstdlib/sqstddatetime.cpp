/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sqstddatetime.h>


static SQInteger _datetime_clock(HSQUIRRELVM v)
{
    sq_pushfloat(v,((SQFloat)clock())/(SQFloat)CLOCKS_PER_SEC);
    return 1;
}

static SQInteger _datetime_time(HSQUIRRELVM v)
{
    SQInteger t = (SQInteger)time(NULL);
    sq_pushinteger(v,t);
    return 1;
}

static void _set_integer_slot(HSQUIRRELVM v,const SQChar *name,SQInteger val)
{
    sq_pushstring(v,name,-1);
    sq_pushinteger(v,val);
    sq_rawset(v,-3);
}

static SQInteger _datetime_date(HSQUIRRELVM v)
{
    time_t t;
    SQInteger it;
    SQInteger format = 'l';
    if(sq_gettop(v) > 1) {
        sq_getinteger(v,2,&it);
        t = it;
        if(sq_gettop(v) > 2) {
            sq_getinteger(v,3,(SQInteger*)&format);
        }
    }
    else {
        time(&t);
    }
    tm *date;
    if(format == 'u')
        date = gmtime(&t);
    else
        date = localtime(&t);
    if(!date)
        return sq_throwerror(v,_SC("crt api failure"));
    sq_newtable(v);
    _set_integer_slot(v, _SC("sec"), date->tm_sec);
    _set_integer_slot(v, _SC("min"), date->tm_min);
    _set_integer_slot(v, _SC("hour"), date->tm_hour);
    _set_integer_slot(v, _SC("day"), date->tm_mday);
    _set_integer_slot(v, _SC("month"), date->tm_mon);
    _set_integer_slot(v, _SC("year"), date->tm_year+1900);
    _set_integer_slot(v, _SC("wday"), date->tm_wday);
    _set_integer_slot(v, _SC("yday"), date->tm_yday);
    return 1;
}



#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_datetime_##name,nparams,pmask}
static const SQRegFunction datetimelib_funcs[]={
    _DECL_FUNC(clock,0,NULL),
    _DECL_FUNC(time,1,NULL),
    _DECL_FUNC(date,-1,_SC(".nn")),
    {NULL,(SQFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC


SQRESULT sqstd_register_datetimelib(HSQUIRRELVM v)
{
    SQInteger i=0;
    while(datetimelib_funcs[i].name!=0)
    {
        const SQRegFunction &rf = datetimelib_funcs[i];
        sq_pushstring(v, rf.name, -1);
        sq_newclosure(v, rf.f, 0);
        sq_setparamscheck(v, rf.nparamscheck, rf.typemask);
        sq_setnativeclosurename(v, -1, rf.name);
        sq_newslot(v, -3, SQFalse);
        i++;
    }
    return SQ_OK;
}
