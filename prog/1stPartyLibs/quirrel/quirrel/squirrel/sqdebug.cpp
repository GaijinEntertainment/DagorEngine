/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include <stdarg.h>
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"
#include "sqarray.h"
#include "sqtable.h"
#include "sqclass.h"

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

void sq_get_frame_info(const SQVM::CallInfo &ci, const SQInstruction *ip, SQFrameInfo &out)
{
    out.funcname = "unknown";
    out.source = "unknown";
    out.line = 0;
    sq_resetobject(&out.funcnameObj);
    sq_resetobject(&out.sourceObj);
    switch (sq_type(ci._closure)) {
    case OT_CLOSURE:{
        SQFunctionProto *func = _closure(ci._closure)->_function;
        if (sq_type(func->_name) == OT_STRING) {
            out.funcname = _stringval(func->_name);
            out.funcnameObj = func->_name;
        }
        if (sq_type(func->_sourcename) == OT_STRING) {
            out.source = _stringval(func->_sourcename);
            out.sourceObj = func->_sourcename;
        }
        out.line = func->GetLine(ip);
        break;
    }
    case OT_NATIVECLOSURE:
        out.source = "NATIVE";
        if(sq_type(_nativeclosure(ci._closure)->_name) == OT_STRING) {
            out.funcname = _stringval(_nativeclosure(ci._closure)->_name);
            out.funcnameObj = _nativeclosure(ci._closure)->_name;
        }
        out.line = -1;
        break;
    default:
        break; //shutup compiler
    }
}

// Reuse the live SQString for a frame name when one exists; only the rare
// literal fallbacks ("unknown"/"NATIVE") have no backing object and get interned.
static SQObjectPtr frameNameObject(SQVM *v, const SQObject &nameObj, const char *fallback)
{
    if (sq_type(nameObj) == OT_STRING)
        return SQObjectPtr(nameObj);
    return SQObjectPtr(SQString::Create(_ss(v), fallback));
}

// One { func, source, line } frame; await-hops add an `awaited` slot so the
// renderer prints "awaited at" rather than "at".
static SQObjectPtr makeFrameTable(SQVM *v, const SQVM::CallInfo &ci, const SQInstruction *ip, bool awaited)
{
    SQFrameInfo fi;
    sq_get_frame_info(ci, ip, fi);
    SQTable *t = SQTable::Create(_ss(v), awaited ? 4 : 3);
    t->NewSlot(SQObjectPtr(SQString::Create(_ss(v), "func")), frameNameObject(v, fi.funcnameObj, fi.funcname));
    t->NewSlot(SQObjectPtr(SQString::Create(_ss(v), "source")), frameNameObject(v, fi.sourceObj, fi.source));
    t->NewSlot(SQObjectPtr(SQString::Create(_ss(v), "line")), SQObjectPtr(fi.line));
    if (awaited)
        t->NewSlot(SQObjectPtr(SQString::Create(_ss(v), "awaited")), SQObjectPtr(true));
    return SQObjectPtr(t);
}

SQObjectPtr sq_capture_error_trace(SQVM *v)
{
    SQArray *arr = SQArray::Create(_ss(v), 0);
    for (SQVM::CallInfo *p = v->ci; p && p >= v->_callsstack; --p) {
        arr->Append(makeFrameTable(v, *p, p->_ip - 1, /*awaited*/false));
        if (p->_root) break;
    }
    return SQObjectPtr(arr);
}

// Error's trace array, created if absent. NULL for non-Error values and for a
// caller-set non-array trace, which we preserve rather than convert.
static SQArray *resolveTraceArray(SQVM *v, const SQObjectPtr &errVal)
{
    if (sq_type(errVal) != OT_INSTANCE)
        return NULL;
    const SQObjectPtr &ecls = _ss(v)->_error_class;
    if (sq_type(ecls) != OT_CLASS || !_instance(errVal)->InstanceOf(_class(ecls)))
        return NULL;
    SQObjectPtr traceKey(SQString::Create(_ss(v), "trace")), cur;
    if (_instance(errVal)->Get(traceKey, cur) && sq_type(cur) != OT_NULL)
        return sq_type(cur) == OT_ARRAY ? _array(cur) : NULL;
    SQArray *arr = SQArray::Create(_ss(v), 0);
    _instance(errVal)->Set(traceKey, SQObjectPtr(arr));
    return arr;
}

void sq_append_awaited_frame(SQVM *v, const SQObjectPtr &errVal, const SQVM::CallInfo &ci)
{
    SQArray *trace = resolveTraceArray(v, errVal);
    if (!trace)
        return;
    trace->Append(makeFrameTable(v, ci, ci._ip - 1, /*awaited*/true));
}

SQObjectPtr sq_clone_error_for_branch(SQVM *v, const SQObjectPtr &errVal)
{
    if (sq_type(errVal) != OT_INSTANCE)
        return errVal;
    const SQObjectPtr &ecls = _ss(v)->_error_class;
    if (sq_type(ecls) != OT_CLASS || !_instance(errVal)->InstanceOf(_class(ecls)))
        return errVal;
    // Clone copies member slots without running _cloned -- allocation-only, safe
    // mid-cascade. Then re-seat the trace to its own array (frame tables stay
    // shared, only the container is private).
    SQObjectPtr clone(_instance(errVal)->Clone(_ss(v)));
    SQObjectPtr traceKey(SQString::Create(_ss(v), "trace")), cur;
    if (_instance(clone)->Get(traceKey, cur) && sq_type(cur) == OT_ARRAY)
        _instance(clone)->Set(traceKey, SQObjectPtr(_array(cur)->Clone()));
    return clone;
}

static void appendCStr(sqvector<char> &buf, const char *s)
{
    while (*s)
        buf.push_back(*s++);
}

bool sq_append_error_trace(SQVM *v, const SQObjectPtr &errVal, sqvector<char> &buf)
{
    if (sq_type(errVal) != OT_INSTANCE)
        return false;
    const SQObjectPtr &ecls = _ss(v)->_error_class;
    if (sq_type(ecls) != OT_CLASS || !_instance(errVal)->InstanceOf(_class(ecls)))
        return false;
    SQObjectPtr traceKey(SQString::Create(_ss(v), "trace")), trace;
    if (!_instance(errVal)->Get(traceKey, trace) || sq_type(trace) != OT_ARRAY)
        return false;

    SQObjectPtr funcKey(SQString::Create(_ss(v), "func"));
    SQObjectPtr srcKey(SQString::Create(_ss(v), "source"));
    SQObjectPtr lineKey(SQString::Create(_ss(v), "line"));
    SQObjectPtr awaitedKey(SQString::Create(_ss(v), "awaited"));
    SQArray *frames = _array(trace);
    const SQInteger startSize = buf.size();
    buf.reserve(startSize + frames->Size() * 64);
    for (SQInteger i = 0; i < frames->Size(); ++i) {
        SQObjectPtr frame;
        if (!frames->Get(i, frame) || sq_type(frame) != OT_TABLE)
            continue;
        SQTable *t = _table(frame);
        SQObjectPtr fn, src, line, awaited;
        t->Get(funcKey, fn);
        t->Get(srcKey, src);
        t->Get(lineKey, line);
        bool isAwaited = t->Get(awaitedKey, awaited) && !SQVM::IsFalse(awaited);
        char lbuf[320];
        scsprintf(lbuf, sizeof(lbuf), "    %s%s (%s:%d)\n",
            isAwaited ? "awaited at " : "at ",
            sq_type(fn) == OT_STRING ? _stringval(fn) : "?",
            sq_type(src) == OT_STRING ? _stringval(src) : "?",
            sq_type(line) == OT_INTEGER ? (int)_integer(line) : 0);
        appendCStr(buf, lbuf);
    }
    return buf.size() != startSize;
}

void sq_print_error_trace(SQVM *v, const SQObjectPtr &errVal, SQPRINTFUNCTION errfn)
{
    if (!errfn)
        return;
    sqvector<char> buf(_ss(v)->_alloc_ctx);
    if (!sq_append_error_trace(v, errVal, buf))
        return;
    buf.push_back('\0');
    errfn(v, "\nERROR TRACE\n");
    errfn(v, "%s", buf._vals);
}

SQRESULT sq_stackinfos(HSQUIRRELVM v, SQInteger level, SQStackInfos *si)
{
    SQInteger cssize = v->_callsstacksize;
    if (level >= 0 && cssize > level) {
        memset(si, 0, sizeof(SQStackInfos));
        SQVM::CallInfo &ci = v->_callsstack[cssize-level-1];
        SQFrameInfo fi;
        sq_get_frame_info(ci, ci._ip, fi);
        si->funcname = fi.funcname;
        si->source = fi.source;
        si->line = fi.line;
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
