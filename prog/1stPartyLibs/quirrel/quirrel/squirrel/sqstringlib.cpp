#include <sqstringlib.h>
#include <squirrel.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>


static void __strip_l(const SQChar *str,const SQChar **start)
{
    const SQChar *t = str;
    while(((*t) != '\0') && isspace(*t)){ t++; }
    *start = t;
}

static void __strip_r(const SQChar *str,SQInteger len,const SQChar **end)
{
    if(len == 0) {
        *end = str;
        return;
    }
    const SQChar *t = &str[len-1];
    while(t >= str && isspace(*t)) { t--; }
    *end = t + 1;
}

SQInteger _sq_string_strip_impl(HSQUIRRELVM v, SQInteger arg_stack_start)
{
    const SQChar *str,*start,*end;
    sq_getstring(v,arg_stack_start,&str);
    SQInteger len = sq_getsize(v,arg_stack_start);
    __strip_l(str,&start);
    __strip_r(str,len,&end);
    sq_pushstring(v,start,end - start);
    return 1;
}

SQInteger _sq_string_lstrip_impl(HSQUIRRELVM v, SQInteger arg_stack_start)
{
    const SQChar *str,*start;
    sq_getstring(v,arg_stack_start,&str);
    __strip_l(str,&start);
    sq_pushstring(v,start,-1);
    return 1;
}

SQInteger _sq_string_rstrip_impl(HSQUIRRELVM v, SQInteger arg_stack_start)
{
    const SQChar *str,*end;
    sq_getstring(v,arg_stack_start,&str);
    SQInteger len = sq_getsize(v,arg_stack_start);
    __strip_r(str,len,&end);
    sq_pushstring(v,str,end - str);
    return 1;
}

SQInteger _sq_string_split_by_chars_impl(HSQUIRRELVM v, SQInteger arg_stack_start)
{
    const SQChar *str,*seps;
    SQInteger sepsize;
    SQBool skipempty = SQFalse;
    sq_getstring(v,arg_stack_start,&str);
    sq_getstringandsize(v,arg_stack_start+1,&seps,&sepsize);
    if(sepsize == 0) return sq_throwerror(v,_SC("empty separators string"));
    if(sq_gettop(v)>arg_stack_start+1) {
        sq_getbool(v,arg_stack_start+2,&skipempty);
    }
    const SQChar *start = str;
    const SQChar *end = str;
    sq_newarray(v,0);
    while(*end != '\0')
    {
        SQChar cur = *end;
        for(SQInteger i = 0; i < sepsize; i++)
        {
            if(cur == seps[i])
            {
                if(!skipempty || (end != start)) {
                    sq_pushstring(v,start,end-start);
                    sq_arrayappend(v,-2);
                }
                start = end + 1;
                break;
            }
        }
        end++;
    }
    if(end != start)
    {
        sq_pushstring(v,start,end-start);
        sq_arrayappend(v,-2);
    }
    return 1;
}

SQInteger _sq_string_escape_impl(HSQUIRRELVM v, SQInteger arg_stack_start)
{
    const SQChar *str;
    SQChar *dest,*resstr;
    SQInteger size;
    sq_getstringandsize(v,arg_stack_start,&str,&size);
    if(size == 0) {
        sq_push(v,arg_stack_start);
        return 1;
    }

    const SQChar *escpat = _SC("\\x%02x");
    const SQInteger maxescsize = 4;

    SQInteger destcharsize = (size * maxescsize); //assumes every char could be escaped
    resstr = dest = (SQChar *)sq_getscratchpad(v,destcharsize * sizeof(SQChar));
    SQChar c;
    SQChar escch;
    SQInteger escaped = 0;
    for(int n = 0; n < size; n++){
        c = *str++;
        escch = 0;
        if(isprint(c) || c == 0) {
            switch(c) {
            case '\a': escch = 'a'; break;
            case '\b': escch = 'b'; break;
            case '\t': escch = 't'; break;
            case '\n': escch = 'n'; break;
            case '\v': escch = 'v'; break;
            case '\f': escch = 'f'; break;
            case '\r': escch = 'r'; break;
            case '\\': escch = '\\'; break;
            case '\"': escch = '\"'; break;
            case '\'': escch = '\''; break;
            case 0: escch = '0'; break;
            }
            if(escch) {
                *dest++ = '\\';
                *dest++ = escch;
                escaped++;
            }
            else {
                *dest++ = c;
            }
        }
        else {

            dest += scsprintf(dest, destcharsize, escpat, c);
            escaped++;
        }
    }

    if(escaped) {
        sq_pushstring(v,resstr,dest - resstr);
    }
    else {
        sq_push(v,arg_stack_start); //nothing escaped
    }
    return 1;
}

SQInteger _sq_string_startswith_impl(HSQUIRRELVM v, SQInteger arg_stack_start)
{
    const SQChar *str,*cmp;
    SQInteger len, cmplen;
    sq_getstringandsize(v,arg_stack_start,&str,&len);
    sq_getstringandsize(v,arg_stack_start+1,&cmp,&cmplen);
    SQBool ret = SQFalse;
    if(cmplen <= len) {
        ret = memcmp(str,cmp,cmplen) == 0 ? SQTrue : SQFalse;
    }
    sq_pushbool(v,ret);
    return 1;
}

SQInteger _sq_string_endswith_impl(HSQUIRRELVM v, SQInteger arg_stack_start)
{
    const SQChar *str,*cmp;
    SQInteger len, cmplen;
    sq_getstringandsize(v,arg_stack_start,&str,&len);
    sq_getstringandsize(v,arg_stack_start+1,&cmp,&cmplen);
    SQBool ret = SQFalse;
    if(cmplen <= len) {
        ret = memcmp(&str[len - cmplen],cmp,cmplen) == 0 ? SQTrue : SQFalse;
    }
    sq_pushbool(v,ret);
    return 1;
}
