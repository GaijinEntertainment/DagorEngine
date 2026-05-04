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

static void _set_integer_slot(HSQUIRRELVM v,const char *name,SQInteger val)
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
        return sq_throwerror(v,"crt api failure");
    sq_newtable(v);
    _set_integer_slot(v, "sec", date->tm_sec);
    _set_integer_slot(v, "min", date->tm_min);
    _set_integer_slot(v, "hour", date->tm_hour);
    _set_integer_slot(v, "day", date->tm_mday);
    _set_integer_slot(v, "month", date->tm_mon);
    _set_integer_slot(v, "year", date->tm_year+1900);
    _set_integer_slot(v, "wday", date->tm_wday);
    _set_integer_slot(v, "yday", date->tm_yday);
    return 1;
}



static const SQRegFunctionFromStr datetimelib_funcs[] = {
    { _datetime_clock, "pure clock(): float", "Returns CPU time used by the process in seconds" },
    { _datetime_time,  "time(): int", "Returns the current time as seconds since the Unix epoch" },
    { _datetime_date,  "date([time: int, format: int]): table", "Returns a table with date fields (sec,min,hour,day,month,year,wday,yday); format 'l' for local, 'u' for UTC" },
    { NULL, NULL, NULL }
};


SQRESULT sqstd_register_datetimelib(HSQUIRRELVM v)
{
    SQInteger i = 0;
    while (datetimelib_funcs[i].f) {
        sq_new_closure_slot_from_decl_string(v, datetimelib_funcs[i].f, 0, datetimelib_funcs[i].declstring, datetimelib_funcs[i].docstring);
        i++;
    }
    return SQ_OK;
}
