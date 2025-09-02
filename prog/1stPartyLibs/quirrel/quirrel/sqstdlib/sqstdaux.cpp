/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <sqstdaux.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqobject.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>
#include <squirrel/sqtable.h>
#include <squirrel/sqarray.h>
#include <squirrel/sqvm.h>

#define ARRAY_ELEMENTS_IN_BRIEF_DUMP 4
#define TABLE_ELEMENTS_IN_BRIEF_DUMP 3

#ifdef SQ_STACK_DUMP_SECRET_PREFIX
    #define STRINGIZE(x) #x
    #define SQ_STRING_EXPAND(x) _SC(STRINGIZE(x))
    #define SECRET_PREFIX SQ_STRING_EXPAND(SQ_STACK_DUMP_SECRET_PREFIX)
#endif

template<typename PrintFunc>
static void print_simple_value(HSQUIRRELVM v, PrintFunc pf, SQObjectPtr &val, bool string_quotes = true)
{
    SQObjectPtr valStr;
    switch (sq_type(val))
    {
        case OT_STRING:
            if (v->ToString(val, valStr))
            {
                if (string_quotes)
                    pf(v, _SC("\"%s\""), _stringval(valStr));
                else
                    pf(v, _SC("%s"), _stringval(valStr));
            }
            break;
        case OT_CLOSURE:
            if (v->ToString(_closure(val)->_function->_name, valStr))
                pf(v, _SC("FN:%s"), _stringval(valStr));
            break;
        case OT_NATIVECLOSURE:
            if (v->ToString(_nativeclosure(val)->_name, valStr))
                pf(v, _SC("FN:%s"), _stringval(valStr));
            break;
        case OT_TABLE:
            pf(v, _SC("TABLE"));
            break;
        case OT_ARRAY:
            pf(v, _SC("ARRAY"));
            break;
        case OT_CLASS:
            pf(v, _SC("CLASS"));
            break;
        default:
            if (v->ToString(val, valStr))
                pf(v, _SC("%s"), _stringval(valStr));
        break;
    }
}


template<typename PrintFunc>
static void collect_stack_string(HSQUIRRELVM v, PrintFunc pf)
{
    SQStackInfos si;
    SQInteger i;
    SQFloat f;
    const SQChar *s;
    SQInteger level=0;
    const SQChar *name=0;
    SQInteger seq=0;
    pf(v,_SC("\nCALLSTACK\n"));
    while(SQ_SUCCEEDED(sq_stackinfos(v,level,&si)))
    {
        if (si.line < 0 && level == 0) { // skip top native function
            ++level;
            continue;
        }

        const SQChar *fn=_SC("unknown");
        const SQChar *src=_SC("unknown");
        if(si.funcname)fn=si.funcname;
        if(si.source)src=si.source;
        pf(v,_SC("*FUNCTION [%s()] %s:%d\n"),fn,src,si.line);
        level++;
    }
    level=0;
    pf(v,_SC("\nLOCALS\n"));

    for(level=0;level<10;level++){
        seq=0;
        while((name = sq_getlocal(v,level,seq)))
        {
            seq++;
#ifdef SQ_STACK_DUMP_SECRET_PREFIX
            bool should_keep_secret = (strncmp(name, SECRET_PREFIX, sizeof(SECRET_PREFIX)/sizeof(SQChar) - 1) == 0);
            if (should_keep_secret) {
                sq_pop(v, 1);
                continue;
            }
#endif
            switch(sq_gettype(v,-1))
            {
            case OT_NULL:
                pf(v,_SC("[%s] NULL\n"),name);
                break;
            case OT_INTEGER:
                sq_getinteger(v,-1,&i);
                pf(v,_SC("[%s] ") _SC("%" PRId64) _SC("\n"),name, int64_t(i));
                break;
            case OT_FLOAT:
                sq_getfloat(v,-1,&f);
                pf(v,_SC("[%s] %.14g\n"),name,f);
                break;
            case OT_USERPOINTER:
                pf(v,_SC("[%s] USERPOINTER\n"),name);
                break;
            case OT_STRING:
                sq_getstring(v,-1,&s);
                pf(v,_SC("[%s] \"%s\"\n"),name,s);
                break;
            case OT_TABLE:
                {
                    pf(v,_SC("[%s] TABLE={"),name);
                    SQTable * t = _table(stack_get(v, -1));
                    SQObjectPtr refidx, key, val;
                    SQInteger idx;
                    SQInteger count = 0;
                    while((idx = t->Next(false, refidx, key, val)) != -1) {
                        refidx = idx;
                        print_simple_value(v, pf, key, false);
                        pf(v,_SC("="));
                        print_simple_value(v, pf, val);
                        count++;
                        if (count != t->CountUsed())
                            pf(v,_SC(", "));
                        if (count + 1 > TABLE_ELEMENTS_IN_BRIEF_DUMP && count != t->CountUsed()) {
                            pf(v,_SC("..."), int(t->CountUsed()));
                            break;
                        }
                    }
                    if (t->CountUsed() > TABLE_ELEMENTS_IN_BRIEF_DUMP)
                        pf(v,_SC("} (%d)\n"), int(t->CountUsed()));
                    else
                        pf(v,_SC("}\n"));
                }
                break;
            case OT_ARRAY:
                {
                    pf(v,_SC("[%s] ARRAY=["),name);
                    SQArray * a = _array(stack_get(v, -1));
                    SQObjectPtr val;
                    for (SQInteger i = 0; i < a->Size(); i++) {
                        a->Get(i, val);
                        print_simple_value(v, pf, val);
                        if (i + 1 < a->Size())
                        {
                            pf(v,_SC(", "));
                            if (i > ARRAY_ELEMENTS_IN_BRIEF_DUMP - 2) {
                                pf(v,_SC("..."));
                                break;
                            }
                        }
                    }
                    if (a->Size() > ARRAY_ELEMENTS_IN_BRIEF_DUMP)
                        pf(v,_SC("] (%d)\n"), int(a->Size()));
                    else
                        pf(v,_SC("]\n"),name);
                }
                break;
            case OT_CLOSURE:
                pf(v,_SC("[%s] CLOSURE="),name);
                print_simple_value(v, pf, stack_get(v, -1));
                pf(v,_SC("\n"));
                break;
            case OT_NATIVECLOSURE:
                pf(v,_SC("[%s] NATIVECLOSURE="),name);
                print_simple_value(v, pf, stack_get(v, -1));
                pf(v,_SC("\n"));
                break;
            case OT_GENERATOR:
                pf(v,_SC("[%s] GENERATOR\n"),name);
                break;
            case OT_USERDATA:
                pf(v,_SC("[%s] USERDATA\n"),name);
                break;
            case OT_THREAD:
                pf(v,_SC("[%s] THREAD\n"),name);
                break;
            case OT_CLASS:
                pf(v,_SC("[%s] CLASS\n"),name);
                break;
            case OT_INSTANCE:
                pf(v,_SC("[%s] INSTANCE="),name);
                print_simple_value(v, pf, stack_get(v, -1));
                pf(v,_SC("\n"));
                break;
            case OT_WEAKREF:
                pf(v,_SC("[%s] WEAKREF\n"),name);
                break;
            case OT_BOOL:{
                SQBool bval;
                sq_getbool(v,-1,&bval);
                pf(v,_SC("[%s] %s\n"),name,bval == SQTrue ? _SC("true"):_SC("false"));
                break;
            }
            default:
                assert(0);
                break;
            }
            sq_pop(v,1);
        }
    }
}


void sqstd_printcallstack(HSQUIRRELVM v)
{
    SQPRINTFUNCTION pf = sq_geterrorfunc(v);
    if (pf)
        collect_stack_string(v, pf);
}


SQRESULT sqstd_formatcallstackstring(HSQUIRRELVM v)
{
    int memlen = 128;
    SQAllocContext alloc_ctx = sq_getallocctx(v);
    SQChar* mem = (SQChar*)sq_malloc(alloc_ctx, memlen*sizeof(SQChar));
    if (!mem)
      return sq_throwerror(v, _SC("Cannot allocate memory"));

    SQChar* dst = mem;

    collect_stack_string(v, [alloc_ctx, &mem, &dst, &memlen](HSQUIRRELVM, const SQChar *fmt, ...) {
        const int appendBlock = 128;
        va_list args;

        va_start(args, fmt);
        int nappend = vsnprintf(0, 0, fmt, args);
        va_end(args);

        int poffset = int(dst - mem);
        int memleft = memlen - poffset;
        if (memleft < nappend) {
            int nrequire = nappend - memleft;
            int newlen = memlen + ((nrequire / appendBlock) + 1) * appendBlock;
            SQChar *newmem = (SQChar *)sq_realloc(alloc_ctx, mem, memlen*sizeof(SQChar), newlen*sizeof(SQChar));
            if (!newmem)
                return;
            mem = newmem;
            memlen = newlen;
            dst = mem + poffset;
        }

        va_start(args, fmt);
        dst += vsnprintf(dst, memlen - poffset, fmt, args);
        va_end(args);
    });

    sq_pushstring(v, mem, dst-mem);
    sq_free(alloc_ctx, mem, memlen);
    return SQ_OK;
}

static SQInteger _sqstd_aux_printerror(HSQUIRRELVM v)
{
    SQPRINTFUNCTION pf = sq_geterrorfunc(v);
    if(pf) {
        const SQChar *sErr = 0;
        if(sq_gettop(v)>=1) {
            if(SQ_SUCCEEDED(sq_getstring(v,2,&sErr)))   {
                pf(v,_SC("\nAN ERROR HAS OCCURRED [%s]\n"),sErr);
            }
            else{
                pf(v,_SC("\nAN ERROR HAS OCCURRED [unknown]\n"));
            }
            sqstd_printcallstack(v);
        }
    }
    return 0;
}

void _sqstd_compiler_message(HSQUIRRELVM v,SQMessageSeverity severity,const SQChar *sErr,const SQChar *sSource,SQInteger line,SQInteger column, const SQChar *extra)
{
    SQPRINTFUNCTION pf = sq_geterrorfunc(v);
    if(pf) {
        pf(v, _SC("%s\n"), sErr);
        pf(v, _SC("%s:%d:%d\n"),sSource,(int)line,(int)column);
        if (extra)
          pf(v, _SC("%s\n"), extra);
    }
}

void sqstd_seterrorhandlers(HSQUIRRELVM v)
{
    sq_setcompilererrorhandler(v,_sqstd_compiler_message);
    sq_newclosure(v,_sqstd_aux_printerror,0);
    sq_seterrorhandler(v);
}


SQRESULT sqstd_throwerrorf(HSQUIRRELVM v,const SQChar *err,...)
{
    SQInteger n=256;
    va_list args;
begin:
    va_start(args,err);
    SQChar *b=sq_getscratchpad(v,n);
    SQInteger r=vsnprintf(b,n,err,args);
    va_end(args);
    if (r>=n) {
        n=r+1;//required+null
        goto begin;
    } else if (r<0) {
        return sq_throwerror(v,_SC("@failed to generate formatted error message"));
    } else {
        return sq_throwerror(v,b);
    }
}
