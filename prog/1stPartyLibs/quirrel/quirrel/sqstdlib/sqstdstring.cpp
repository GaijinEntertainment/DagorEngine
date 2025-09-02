/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <sqstdstring.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include <sqstringlib.h>

#define MAX_FORMAT_LEN  20
#define MAX_WFORMAT_LEN 3
#define ADDITIONAL_FORMAT_SPACE (100*sizeof(SQChar))

static SQUserPointer rex_typetag = NULL;

static SQBool isfmtchr(SQChar ch)
{
    switch(ch) {
    case '-': case '+': case ' ': case '#': case '0': return SQTrue;
    }
    return SQFalse;
}

static SQInteger validate_format(HSQUIRRELVM v, SQChar *fmt, const SQChar *src, SQInteger n,SQInteger &width)
{
    SQChar *dummy;
    SQChar swidth[MAX_WFORMAT_LEN];
    SQInteger wc = 0;
    SQInteger start = n;
    fmt[0] = '%';
    while (isfmtchr(src[n])) n++;
    while (isdigit(src[n])) {
        swidth[wc] = src[n];
        n++;
        wc++;
        if(wc>=MAX_WFORMAT_LEN)
            return sq_throwerror(v,_SC("width format too long"));
    }
    swidth[wc] = '\0';
    if(wc > 0) {
        width = scstrtol(swidth,&dummy,10);
    }
    else
        width = 0;
    if (src[n] == '.') {
        n++;

        wc = 0;
        while (isdigit(src[n])) {
            swidth[wc] = src[n];
            n++;
            wc++;
            if(wc>=MAX_WFORMAT_LEN)
                return sq_throwerror(v,_SC("precision format too long"));
        }
        swidth[wc] = '\0';
        if(wc > 0) {
            width += scstrtol(swidth,&dummy,10);

        }
    }
    if (n-start > MAX_FORMAT_LEN )
        return sq_throwerror(v,_SC("format too long"));
    memcpy(&fmt[1],&src[start],((n-start)+1)*sizeof(SQChar));
    fmt[(n-start)+2] = '\0';
    return n;
}

SQRESULT sqstd_format(HSQUIRRELVM v,SQInteger nformatstringidx,SQInteger *outlen,SQChar **output)
{
    const SQChar *format;
    SQChar *dest;
    SQChar fmt[MAX_FORMAT_LEN];
    const SQRESULT res = sq_getstring(v,nformatstringidx,&format);
    if (SQ_FAILED(res)) {
        return res; // propagate the error
    }
    SQInteger format_size = sq_getsize(v,nformatstringidx);
    SQInteger allocated = (format_size+2)*sizeof(SQChar);
    dest = sq_getscratchpad(v,allocated);
    SQInteger n = 0,i = 0, nparam = nformatstringidx+1, w = 0;
    //while(format[n] != '\0')
    while(n < format_size)
    {
        if(format[n] != '%') {
            assert(i < allocated);
            dest[i++] = format[n];
            n++;
        }
        else if(format[n+1] == '%') { //handles %%
                dest[i++] = '%';
                n += 2;
        }
        else {
            n++;
            if( nparam > sq_gettop(v) )
                return sq_throwerror(v,_SC("not enough parameters for the given format string"));
            n = validate_format(v,fmt,format,n,w);
            if(n < 0) return -1;
            SQInteger addlen = 0;
            SQInteger valtype = 0;
            const SQChar *ts = NULL;
            SQInteger ti = 0;
            SQFloat tf = 0;
            switch(format[n]) {
            case 's':
                if(SQ_FAILED(sq_getstring(v,nparam,&ts)))
                    return sq_throwerror(v,_SC("string expected for the specified format"));
                addlen = (sq_getsize(v,nparam)*sizeof(SQChar))+((w+1)*sizeof(SQChar));
                valtype = 's';
                break;
            case 'i': case 'd': case 'o': case 'u':  case 'x':  case 'X':
#ifdef _SQ64
                {
                size_t flen = strlen(fmt);
                SQInteger fpos = flen - 1;
                SQChar f = fmt[fpos];
                const SQChar *prec = (const SQChar *)_PRINT_INT_PREC;
                while(*prec != _SC('\0')) {
                    fmt[fpos++] = *prec++;
                }
                fmt[fpos++] = f;
                fmt[fpos++] = _SC('\0');
                }
#endif
            case 'c':
                if(SQ_FAILED(sq_getinteger(v,nparam,&ti)))
                    return sq_throwerror(v,_SC("integer expected for the specified format"));
                addlen = (ADDITIONAL_FORMAT_SPACE)+((w+1)*sizeof(SQChar));
                valtype = 'i';
                break;
            case 'f': case 'g': case 'G': case 'e':  case 'E':
                if(SQ_FAILED(sq_getfloat(v,nparam,&tf)))
                    return sq_throwerror(v,_SC("float expected for the specified format"));
                addlen = (ADDITIONAL_FORMAT_SPACE)+((w+1)*sizeof(SQChar));
                valtype = 'f';
                break;
            default:
                return sq_throwerror(v,_SC("invalid format"));
            }
            n++;
            allocated += addlen + sizeof(SQChar);
            dest = sq_getscratchpad(v,allocated);
            switch(valtype) {
            case 's': i += scsprintf(&dest[i],allocated,fmt,ts); break;
            case 'i': i += scsprintf(&dest[i],allocated,fmt,ti); break;
            case 'f': {
                int len = scsprintf(&dest[i], allocated, fmt, tf);
                for (; len > 0; len--, i++)
                    if (dest[i] == ',')
                        dest[i] = '.';
                break;
                }
            };
            nparam ++;
        }
    }
    *outlen = i;
    dest[i] = '\0';
    *output = dest;
    return SQ_OK;
}

void sqstd_pushstringf(HSQUIRRELVM v,const SQChar *s,...)
{
    SQInteger n=256;
    va_list args;
begin:
    va_start(args,s);
    SQChar *b=sq_getscratchpad(v,n);
    SQInteger r=vsnprintf(b,n,s,args);
    va_end(args);
    if (r>=n) {
        n=r+1;//required+null
        goto begin;
    } else if (r<0) {
        sq_pushnull(v);
    } else {
        sq_pushstring(v,b,r);
    }
}

static SQInteger _string_printf(HSQUIRRELVM v)
{
    SQChar *dest = NULL;
    SQInteger length = 0;
    if(SQ_FAILED(sqstd_format(v,2,&length,&dest)))
        return -1;

    SQPRINTFUNCTION printfunc = sq_getprintfunc(v);
    if(printfunc) printfunc(v,_SC("%s"),dest);

    return 0;
}

static SQInteger _string_format(HSQUIRRELVM v)
{
    SQChar *dest = NULL;
    SQInteger length = 0;
    if(SQ_FAILED(sqstd_format(v,2,&length,&dest)))
        return -1;
    sq_pushstring(v,dest,length);
    return 1;
}

#define IMPL_STRING_FUNC(name) static SQInteger _string_##name(HSQUIRRELVM v) { return _sq_string_ ## name ## _impl(v, 2); }

IMPL_STRING_FUNC(strip)
IMPL_STRING_FUNC(lstrip)
IMPL_STRING_FUNC(rstrip)
IMPL_STRING_FUNC(split_by_chars)
IMPL_STRING_FUNC(escape)
IMPL_STRING_FUNC(startswith)
IMPL_STRING_FUNC(endswith)

#undef IMPL_STRING_FUNC

#define SETUP_REX(v) \
    SQRex *self = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer *)&self,rex_typetag))) { \
        return sq_throwerror(v,_SC("invalid type tag")); \
    }

static SQInteger _rexobj_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    SQRex *self = ((SQRex *)p);
    sqstd_rex_free(self);
    return 1;
}

static SQInteger _regexp_match(HSQUIRRELVM v)
{
    SETUP_REX(v);
    const SQChar *str;
    sq_getstring(v,2,&str);
    if(sqstd_rex_match(self,str) == SQTrue)
    {
        sq_pushbool(v,SQTrue);
        return 1;
    }
    sq_pushbool(v,SQFalse);
    return 1;
}

static void _addrexmatch(HSQUIRRELVM v,const SQChar *str,const SQChar *begin,const SQChar *end)
{
    sq_newtable(v);
    sq_pushstring(v,_SC("begin"),-1);
    sq_pushinteger(v,begin - str);
    sq_rawset(v,-3);
    sq_pushstring(v,_SC("end"),-1);
    sq_pushinteger(v,end - str);
    sq_rawset(v,-3);
}

static SQInteger _regexp_search(HSQUIRRELVM v)
{
    SETUP_REX(v);
    const SQChar *str,*begin,*end;
    SQInteger start = 0;
    sq_getstring(v,2,&str);
    if(sq_gettop(v) > 2) sq_getinteger(v,3,&start);
    if(sqstd_rex_search(self,str+start,&begin,&end) == SQTrue) {
        _addrexmatch(v,str,begin,end);
        return 1;
    }
    return 0;
}

static SQInteger _regexp_capture(HSQUIRRELVM v)
{
    SETUP_REX(v);
    const SQChar *str,*begin,*end;
    SQInteger start = 0;
    sq_getstring(v,2,&str);
    if(sq_gettop(v) > 2) sq_getinteger(v,3,&start);
    if(sqstd_rex_search(self,str+start,&begin,&end) == SQTrue) {
        SQInteger n = sqstd_rex_getsubexpcount(self);
        SQRexMatch match;
        sq_newarray(v,0);
        for(SQInteger i = 0;i < n; i++) {
            sqstd_rex_getsubexp(self,i,&match);
            if(match.len > 0)
                _addrexmatch(v,str,match.begin,match.begin+match.len);
            else
                _addrexmatch(v,str,str,str); //empty match
            sq_arrayappend(v,-2);
        }
        return 1;
    }
    return 0;
}

static SQInteger _regexp_subexpcount(HSQUIRRELVM v)
{
    SETUP_REX(v);
    sq_pushinteger(v,sqstd_rex_getsubexpcount(self));
    return 1;
}

static SQInteger _regexp_constructor(HSQUIRRELVM v)
{
    SQRex *self = NULL;
    if (SQ_FAILED(sq_getinstanceup(v, 1, (SQUserPointer *)&self, rex_typetag))) {
        return sq_throwerror(v, _SC("invalid type tag"));
    }
    if (self != NULL) {
        return sq_throwerror(v, _SC("invalid regexp object"));
    }
    const SQChar *error,*pattern;
    sq_getstring(v,2,&pattern);
    SQRex *rex = sqstd_rex_compile(sq_getallocctx(v),pattern,&error);
    if(!rex) return sq_throwerror(v,error);
    sq_setinstanceup(v,1,rex);
    sq_setreleasehook(v,1,_rexobj_releasehook);
    return 0;
}

static SQInteger _regexp__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,_SC("regexp"),-1);
    return 1;
}

static const SQRegFunctionFromStr rexobj_funcs[] = {
    { _regexp_constructor, "constructor(pattern: string): instance" },
    { _regexp_match, "instance.match(str: string): bool" },
    { _regexp_search, "instance.search(str: string, [start: int]): table" },
    { _regexp_capture, "instance.capture(str: string, [start: int]): array" },
    { _regexp_subexpcount, "instance.subexpcount(): int" },
    { _regexp__typeof, "instance._typeof(): string" },
    { NULL, NULL }
};

static const SQRegFunctionFromStr stringlib_funcs[] = {
    { _string_format, "pure format(fmt: string, ...): string" },
    { _string_printf, "printf(fmt: string, ...)" },
    { _string_strip, "pure strip(str: string): string" },
    { _string_lstrip, "pure lstrip(str: string): string" },
    { _string_rstrip, "pure rstrip(str: string): string" },
    { _string_split_by_chars, "split_by_chars(str: string, separators: string, [skip_empty: bool]): array" },
    { _string_escape, "pure escape(str: string): string" },
    { _string_startswith, "pure startswith(str: string, prefix: string): bool" },
    { _string_endswith, "pure endswith(str: string, suffix: string): bool" },
    { NULL, NULL },
};
#undef _DECL_FUNC


SQRESULT sqstd_register_stringlib(HSQUIRRELVM v)
{
    sq_pushstring(v, _SC("regexp"), -1);
    sq_newclass(v, SQFalse);
    rex_typetag = (SQUserPointer)rexobj_funcs;
    sq_settypetag(v, -1, rex_typetag);
    SQInteger i = 0;
    while (rexobj_funcs[i].f) {
        sq_new_closure_slot_from_decl_string(v, rexobj_funcs[i].f, 0, rexobj_funcs[i].declstring, rexobj_funcs[i].docstring);
        i++;
    }
    sq_newslot(v, -3, SQFalse);

    i = 0;
    while (stringlib_funcs[i].f) {
        sq_new_closure_slot_from_decl_string(v, stringlib_funcs[i].f, 0, stringlib_funcs[i].declstring, stringlib_funcs[i].docstring);
        i++;
    }
    return SQ_OK;
}
