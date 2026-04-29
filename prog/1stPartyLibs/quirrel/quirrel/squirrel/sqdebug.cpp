/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include <stdarg.h>
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"

SQRESULT sq_getfunctioninfo(HSQUIRRELVM v,SQInteger level,SQFunctionInfo *fi)
{
    SQInteger cssize = v->_callsstacksize;
    if (level >= 0 && cssize > level) {
        SQVM::CallInfo &ci = v->_callsstack[cssize-level-1];
        if(sq_isclosure(ci._closure)) {
            SQClosure *c = _closure(ci._closure);
            SQFunctionProto *proto = c->_function;
            fi->funcid = proto;
            fi->name = sq_type(proto->_name) == OT_STRING?_stringval(proto->_name):"unknown";
            fi->source = sq_type(proto->_sourcename) == OT_STRING?_stringval(proto->_sourcename):"unknown";
            fi->line = proto->_lineinfos->_first_line;
            return SQ_OK;
        }
    }
    return sq_throwerror(v,"the object is not a closure");
}

SQRESULT sq_stackinfos(HSQUIRRELVM v, SQInteger level, SQStackInfos *si)
{
    SQInteger cssize = v->_callsstacksize;
    if (level >= 0 && cssize > level) {
        memset(si, 0, sizeof(SQStackInfos));
        SQVM::CallInfo &ci = v->_callsstack[cssize-level-1];
        switch (sq_type(ci._closure)) {
        case OT_CLOSURE:{
            SQFunctionProto *func = _closure(ci._closure)->_function;
            if (sq_type(func->_name) == OT_STRING)
                si->funcname = _stringval(func->_name);
            if (sq_type(func->_sourcename) == OT_STRING)
                si->source = _stringval(func->_sourcename);
            si->line = func->GetLine(ci._ip);
            break;
        }
        case OT_NATIVECLOSURE:
            si->source = "NATIVE";
            si->funcname = "unknown";
            if(sq_type(_nativeclosure(ci._closure)->_name) == OT_STRING)
                si->funcname = _stringval(_nativeclosure(ci._closure)->_name);
            si->line = -1;
            break;
        default: break; //shutup compiler
        }
        return SQ_OK;
    }
    return SQ_ERROR;
}

void SQVM::Raise_Error(const char *s, ...)
{
    va_list vl;
    va_start(vl, s);
    SQInteger buffersize = (SQInteger)strlen(s) + 256;
    vsnprintf(_sp(buffersize),buffersize, s, vl);
    va_end(vl);
    _lasterror = SQString::Create(_ss(this),_spval,-1);
}

void SQVM::Raise_Error(const SQObjectPtr &desc)
{
    _lasterror = desc;
}

SQString *SQVM::PrintObjVal(const SQObject &o)
{
    constexpr SQInteger MAX_STR_DISPLAY = 58;
    // Printable overhead: ' (1) + ...' (4) + ' (type='string')' (16) = 21
    constexpr SQInteger MAX_REPR_LEN = MAX_STR_DISPLAY + 21;
    char buf[MAX_REPR_LEN + 1];

    switch (sq_type(o)) {
        case OT_STRING: {
            SQInteger len = _string(o)->_len;
            if (len > MAX_STR_DISPLAY) {
                scsprintf(buf, sizeof(buf),
                    "'%.*s...' (type='%s')", (int)MAX_STR_DISPLAY, _stringval(o), GetTypeName(o));
            } else {
                scsprintf(buf, sizeof(buf),
                    "'%s' (type='%s')", _stringval(o), GetTypeName(o));
            }
            return SQString::Create(_ss(this), buf);
        }
        case OT_INTEGER:
            scsprintf(buf, sizeof(buf),
                    "'" _PRINT_INT_FMT "' (type='%s')", _integer(o), GetTypeName(o));
            return SQString::Create(_ss(this), buf);
        case OT_FLOAT:
            scsprintf(buf, sizeof(buf),
                    "'%.14g' (type='%s')", _float(o), GetTypeName(o));
            return SQString::Create(_ss(this), buf);
        default:
            return SQString::Create(_ss(this), GetTypeName(o));
    }
}

void SQVM::Raise_IdxError(const SQObjectPtr &o)
{
    SQObjectPtr oval(PrintObjVal(o));
    Raise_Error("the index %s does not exist", _stringval(oval));
}

void SQVM::Raise_MetamethodError(const char *mmname)
{
    if (sq_type(_lasterror) == OT_STRING) {
        Raise_Error("Error in '%s' metamethod: %s", mmname, _stringval(_lasterror));
    } else {
        SQObjectPtr oval(PrintObjVal(_lasterror));
        Raise_Error("Error in '%s' metamethod: %s", mmname, _stringval(oval));
    }
}

void SQVM::Raise_CompareError(const SQObject &o1, const SQObject &o2)
{
    SQObjectPtr oval1(PrintObjVal(o1)), oval2(PrintObjVal(o2));
    Raise_Error("comparison between %s and %s", _stringval(oval1), _stringval(oval2));
}


void SQVM::Raise_ParamTypeError(SQInteger nparam,SQInteger typemask,SQInteger type,const char *funcname)
{
    SQObjectPtr exptypes(SQString::Create(_ss(this), "", -1));
    SQInteger found = 0;
    for(SQInteger i=0; i<16; i++)
    {
        SQInteger mask = ((SQInteger)1) << i;
        if(typemask & (mask)) {
            if(found>0) StringCat(exptypes, SQObjectPtr(SQString::Create(_ss(this), "|", -1)), exptypes);
            found ++;
            StringCat(exptypes, SQObjectPtr(SQString::Create(_ss(this), IdType2Name((SQObjectType)mask), -1)), exptypes);
        }
    }
    Raise_Error("parameter %d of '%s' has an invalid type '%s' ; expected: '%s'", (int)nparam,
                funcname && *funcname ? funcname : "<unknown>", IdType2Name((SQObjectType)type), _stringval(exptypes));
}
