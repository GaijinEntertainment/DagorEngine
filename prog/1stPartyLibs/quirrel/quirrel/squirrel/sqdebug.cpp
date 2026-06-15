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

// Depth-0 locals capture caps (rare fault path). Per-frame / per-outer counts are
// enforced at capture; the string cap is render-only (the immutable ref is held whole).
static const int       SQ_LOCALS_MAX_PER_FRAME = 16;
static const int       SQ_LOCALS_MAX_OUTERS    = 8;
static const SQInteger SQ_LOCALS_MAX_STRING    = 120;

#ifdef SQ_STACK_DUMP_SECRET_PREFIX
    #define SQ_DBG_STRINGIZE(x) #x
    #define SQ_DBG_STRING_EXPAND(x) SQ_DBG_STRINGIZE(x)
    #define SQ_DBG_SECRET_PREFIX SQ_DBG_STRING_EXPAND(SQ_STACK_DUMP_SECRET_PREFIX)
#endif

// Names with the secret prefix are kept out of captured locals, same contract as the
// sync LOCALS dump (sqstdaux.cpp). No-op unless the build defines the prefix (rel/irel).
static bool isSecretLocalName(const SQObjectPtr &name)
{
#ifdef SQ_STACK_DUMP_SECRET_PREFIX
    if (sq_type(name) == OT_STRING)
        return strncmp(_stringval(name), SQ_DBG_SECRET_PREFIX, sizeof(SQ_DBG_SECRET_PREFIX) - 1) == 0;
#else
    (void)name;
#endif
    return false;
}

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

// Fill a per-local record with its type tag plus a leaf-only depth-0 summary: scalars
// by value, strings by (immutable) ref, aggregates as a ref-free summary. Never stores a
// ref into a live object graph, so the record pins nothing and forms no GC cycle.
static void summarizeLocal(SQVM *v, SQTable *r, const SQObjectPtr &val,
    const SQObjectPtr &typeKey, const SQObjectPtr &valueKey, const SQObjectPtr &countKey,
    const SQObjectPtr &gstateKey, const SQObjectPtr &funcKey, const SQObjectPtr &srcKey,
    const SQObjectPtr &lineKey)
{
    r->NewSlot(typeKey, SQObjectPtr(SQString::Create(_ss(v), IdType2Name(sq_type(val)))));
    switch (sq_type(val)) {
    case OT_NULL:
    case OT_INTEGER:
    case OT_FLOAT:
    case OT_BOOL:
        // by value
        r->NewSlot(valueKey, val); //-V1037
        break;
    case OT_STRING:
        // immutable leaf: reuse the ref, no copy
        r->NewSlot(valueKey, val); //-V1037
        break;
    case OT_ARRAY:
        r->NewSlot(countKey, SQObjectPtr((SQInteger)_array(val)->Size()));
        break;
    case OT_TABLE:
        r->NewSlot(countKey, SQObjectPtr((SQInteger)_table(val)->CountUsed()));
        break;
    case OT_CLOSURE: {
        SQFunctionProto *f = _closure(val)->_function;
        if (sq_type(f->_name) == OT_STRING) r->NewSlot(funcKey, SQObjectPtr(f->_name));
        if (sq_type(f->_sourcename) == OT_STRING) r->NewSlot(srcKey, SQObjectPtr(f->_sourcename));
        r->NewSlot(lineKey, SQObjectPtr((SQInteger)f->_lineinfos->_first_line));
        break;
    }
    case OT_NATIVECLOSURE:
        if (sq_type(_nativeclosure(val)->_name) == OT_STRING)
            r->NewSlot(funcKey, SQObjectPtr(_nativeclosure(val)->_name));
        break;
    case OT_GENERATOR: {
        const char *gs = "running";
        switch (_generator(val)->_state) {
        case SQGenerator::eSuspended: gs = "suspended"; break;
        case SQGenerator::eDead: gs = "dead"; break;
        default: break;
        }
        r->NewSlot(gstateKey, SQObjectPtr(SQString::Create(_ss(v), gs)));
        break;
    }
    default:
        break;   // userpointer/instance/class/userdata/thread/weakref/outer: type tag only
    }
}

// Capture a frame's in-scope locals (and captured outers) as an array of records, read from
// `slots` (size `nslots`): the live stack window for an origin frame, or a parked generator's
// saved _stack. NULL when there is nothing to record. `parked` guards the two parked-frame
// hazards: an open outer's _valptr targets unrelated live frames (skip it), and saved slot 0
// holds the weakref-wrapped self (unwrap it). Block-scoped by ip; `*moreOut` = omitted count.
static SQArray *collectFrameLocals(SQVM *v, const SQVM::CallInfo &ci,
    const SQObjectPtr *slots, SQUnsignedInteger nslots, const SQInstruction *ip,
    bool parked, int *moreOut)
{
    *moreOut = 0;
    if (!slots || sq_type(ci._closure) != OT_CLOSURE)
        return NULL;
    SQClosure *c = _closure(ci._closure);
    SQFunctionProto *func = c->_function;

    const SQSharedState::TraceSchema &sch = _ss(v)->_traceSchema;

    SQArray *arr = NULL;

    int nouter = 0;
    for (SQInteger idx = 0; idx < func->_noutervalues; ++idx) {
        SQOuter *o = _outer(c->_outervalues[idx]);
        if (parked && o->_valptr != &o->_value)
            continue;                              // open outer: unsafe to read on a parked frame
        const SQObjectPtr &oname = func->_outervalues[idx]._name;
        if (isSecretLocalName(oname))
            continue;
        if (nouter >= SQ_LOCALS_MAX_OUTERS) { ++*moreOut; continue; }
        if (!arr) arr = SQArray::Create(_ss(v), 0);
        SQTable *r = SQTable::Create(_ss(v), 4);
        r->NewSlot(sch.name, oname);
        r->NewSlot(sch.captured, SQObjectPtr(true));
        summarizeLocal(v, r, *o->_valptr, sch.type, sch.value, sch.count, sch.gstate, sch.func, sch.source, sch.line);
        arr->Append(SQObjectPtr(r));
        ++nouter;
    }

    SQUnsignedInteger nop = (SQUnsignedInteger)(ip - func->_instructions);
    int nloc = 0;
    for (SQInteger i = 0; i < func->_nlocalvarinfos; ++i) {
        const SQLocalVarInfo &lvi = func->_localvarinfos[i];
        if (!(lvi._start_op <= nop && lvi._end_op >= nop))
            continue;                              // not in scope at this ip (block scoping)
        if ((SQUnsignedInteger)lvi._pos >= nslots)
            continue;                              // bounds guard against a malformed _pos
        if (isSecretLocalName(lvi._name))
            continue;
        if (nloc >= SQ_LOCALS_MAX_PER_FRAME) { ++*moreOut; continue; }
        SQObjectPtr val = slots[lvi._pos];
        if (parked && lvi._pos == 0)
            val = _realval(val);                   // parked slot-0 self is weakref-wrapped
        if (!arr) arr = SQArray::Create(_ss(v), 0);
        SQTable *r = SQTable::Create(_ss(v), 4);
        r->NewSlot(sch.name, lvi._name);
        summarizeLocal(v, r, val, sch.type, sch.value, sch.count, sch.gstate, sch.func, sch.source, sch.line);
        arr->Append(SQObjectPtr(r));
        ++nloc;
    }
    return arr;
}

// One { func, source, line } frame; await-hops add an `awaited` slot so the
// renderer prints "awaited at" rather than "at". When a stack window is supplied the frame
// also carries a `locals` array (and `localsMore` count when capped).
static SQObjectPtr makeFrameTable(SQVM *v, const SQVM::CallInfo &ci, const SQInstruction *ip,
    bool awaited, const SQObjectPtr *slots, SQUnsignedInteger nslots, bool parked)
{
    const SQSharedState::TraceSchema &sch = _ss(v)->_traceSchema;
    SQFrameInfo fi;
    sq_get_frame_info(ci, ip, fi);
    SQTable *t = SQTable::Create(_ss(v), awaited ? 4 : 3);
    t->NewSlot(sch.func, frameNameObject(v, fi.funcnameObj, fi.funcname));
    t->NewSlot(sch.source, frameNameObject(v, fi.sourceObj, fi.source));
    t->NewSlot(sch.line, SQObjectPtr(fi.line));
    if (awaited)
        t->NewSlot(sch.awaited, SQObjectPtr(true));
    int more = 0;
    SQArray *locals = collectFrameLocals(v, ci, slots, nslots, ip, parked, &more);
    if (locals) {
        t->NewSlot(sch.locals, SQObjectPtr(locals));
        if (more > 0)
            t->NewSlot(sch.localsMore, SQObjectPtr((SQInteger)more));
    }
    return SQObjectPtr(t);
}

SQObjectPtr sq_make_diag_frame(SQVM *v, const SQObjectPtr &source, const SQObjectPtr &func, SQInteger line, const char *kind)
{
    const SQSharedState::TraceSchema &sch = _ss(v)->_traceSchema;
    SQTable *t = SQTable::Create(_ss(v), 4);
    t->NewSlot(sch.func, func);
    t->NewSlot(sch.source, source);
    t->NewSlot(sch.line, SQObjectPtr(line));
    t->NewSlot(sch.kind, SQObjectPtr(SQString::Create(_ss(v), kind)));
    return SQObjectPtr(t);
}

SQObjectPtr sq_capture_error_trace(SQVM *v)
{
    SQArray *arr = SQArray::Create(_ss(v), 0);
    // Track each frame's stackbase down the live chain (running -= _prevstkbase, mirroring
    // sq_getlocal) so each origin frame reads its own locals window.
    SQInteger base = v->_stackbase;
    for (SQVM::CallInfo *p = v->ci; p && p >= v->_callsstack; --p) {
        const SQObjectPtr *slots = NULL;
        SQUnsignedInteger nslots = 0;
        if (base >= 0 && base < (SQInteger)v->_stack.size()) {
            slots = &v->_stack._vals[base];
            nslots = (SQUnsignedInteger)(v->_stack.size() - base);
        }
        arr->Append(makeFrameTable(v, *p, p->_ip - 1, /*awaited*/false, slots, nslots, /*parked*/false));
        if (p->_root) break;
        base -= p->_prevstkbase;
    }
    return SQObjectPtr(arr);
}

void sq_describe_fault_value(const SQObject &value, char *buf, size_t buf_size)
{
    switch (sq_type(value)) {
        case OT_INTEGER: scsprintf(buf, buf_size, "integer " _PRINT_INT_FMT, _integer(value)); break;
        case OT_FLOAT:   scsprintf(buf, buf_size, "float %.14g", (double)_float(value)); break;
        case OT_BOOL:    scsprintf(buf, buf_size, "bool %s", _integer(value) ? "true" : "false"); break;
        case OT_NULL:    scsprintf(buf, buf_size, "null"); break;
        default:         scsprintf(buf, buf_size, "<%s>", GetTypeName(value)); break;
    }
}

void sq_append_awaited_frame_to(SQVM *v, SQArray *trace, const SQVM::CallInfo &ci, const SQGenerator *gen)
{
    // Read the parked generator's saved stack for this hop's locals. Must run while the
    // generator is still suspended (settleTerminal calls this before gen->Kill()).
    const SQObjectPtr *slots = NULL;
    SQUnsignedInteger nslots = 0;
    if (gen && gen->_state == SQGenerator::eSuspended && gen->_stack.size() > 0) {
        slots = gen->_stack._vals;
        nslots = (SQUnsignedInteger)gen->_stack.size();
    }
    trace->Append(makeFrameTable(v, ci, ci._ip - 1, /*awaited*/true, slots, nslots, /*parked*/true));
}

static void appendCStr(sqvector<char> &buf, const char *s)
{
    while (*s)
        buf.push_back(*s++);
}

// Render one captured-local record as `      name[ (captured)] = <summary>`. Composes the
// depth-0 text lazily from the stored pieces; the only string truncation lives here.
static void appendLocalLine(SQVM *v, SQTable *r, sqvector<char> &buf)
{
    const SQSharedState::TraceSchema &sch = _ss(v)->_traceSchema;

    SQObjectPtr nm, ty, cap, value;
    r->Get(sch.name, nm);
    r->Get(sch.type, ty);
    const char *nms = sq_type(nm) == OT_STRING ? _stringval(nm) : "?";
    const char *tys = sq_type(ty) == OT_STRING ? _stringval(ty) : "?";
    bool captured = r->Get(sch.captured, cap) && !SQVM::IsFalse(cap);

    char valbuf[160];
    if (r->Get(sch.value, value)) {
        switch (sq_type(value)) {
        case OT_INTEGER: scsprintf(valbuf, sizeof(valbuf), _PRINT_INT_FMT, _integer(value)); break;
        case OT_FLOAT:   scsprintf(valbuf, sizeof(valbuf), "%.14g", (double)_float(value)); break;
        case OT_BOOL:    scsprintf(valbuf, sizeof(valbuf), "%s", _integer(value) ? "true" : "false"); break;
        case OT_NULL:    scsprintf(valbuf, sizeof(valbuf), "null"); break;
        case OT_STRING: {
            const char *s = _stringval(value);
            if (_string(value)->_len > SQ_LOCALS_MAX_STRING)
                scsprintf(valbuf, sizeof(valbuf), "'%.*s...'", (int)SQ_LOCALS_MAX_STRING, s);
            else
                scsprintf(valbuf, sizeof(valbuf), "'%s'", s);
            break;
        }
        default: scsprintf(valbuf, sizeof(valbuf), "%s", GetTypeName(value)); break;
        }
    } else if (strcmp(tys, "array") == 0) {
        SQObjectPtr cnt; r->Get(sch.count, cnt);
        scsprintf(valbuf, sizeof(valbuf), "array(%d)", sq_type(cnt) == OT_INTEGER ? (int)_integer(cnt) : 0);
    } else if (strcmp(tys, "table") == 0) {
        SQObjectPtr cnt; r->Get(sch.count, cnt);
        scsprintf(valbuf, sizeof(valbuf), "table(%d keys)", sq_type(cnt) == OT_INTEGER ? (int)_integer(cnt) : 0);
    } else if (strcmp(tys, "function") == 0) {
        // IdType2Name maps both closure kinds to "function"; script ones add source/line
        SQObjectPtr fnm, fsrc, fline;
        r->Get(sch.func, fnm); r->Get(sch.source, fsrc); r->Get(sch.line, fline);
        const char *fn = sq_type(fnm) == OT_STRING ? _stringval(fnm) : "?";
        if (sq_type(fsrc) == OT_STRING)
            scsprintf(valbuf, sizeof(valbuf), "function %s (%s:%d)", fn, _stringval(fsrc),
                sq_type(fline) == OT_INTEGER ? (int)_integer(fline) : 0);
        else
            scsprintf(valbuf, sizeof(valbuf), "function %s", fn);
    } else if (strcmp(tys, "generator") == 0) {
        SQObjectPtr gs; r->Get(sch.gstate, gs);
        scsprintf(valbuf, sizeof(valbuf), "generator(%s)", sq_type(gs) == OT_STRING ? _stringval(gs) : "?");
    } else {
        scsprintf(valbuf, sizeof(valbuf), "%s", tys);
    }

    char lbuf[320];
    scsprintf(lbuf, sizeof(lbuf), "      %s%s = %s\n", nms, captured ? " (captured)" : "", valbuf);
    appendCStr(buf, lbuf);
}

bool sq_append_error_trace(SQVM *v, const SQObject &trace, sqvector<char> &buf)
{
    if (sq_type(trace) != OT_ARRAY)
        return false;

    const SQSharedState::TraceSchema &sch = _ss(v)->_traceSchema;
    SQArray *frames = _array(trace);
    const SQInteger startSize = buf.size();
    buf.reserve(startSize + frames->Size() * 64);
    for (SQInteger i = 0; i < frames->Size(); ++i) {
        SQObjectPtr frame;
        if (!frames->Get(i, frame) || sq_type(frame) != OT_TABLE)
            continue;
        SQTable *t = _table(frame);
        SQObjectPtr fn, src, line, awaited, kind;
        t->Get(sch.func, fn);
        t->Get(sch.source, src);
        t->Get(sch.line, line);
        const char *fns = sq_type(fn) == OT_STRING ? _stringval(fn) : "?";
        const char *srcs = sq_type(src) == OT_STRING ? _stringval(src) : "?";
        int ln = sq_type(line) == OT_INTEGER ? (int)_integer(line) : 0;
        const char *kinds = t->Get(sch.kind, kind) && sq_type(kind) == OT_STRING ? _stringval(kind) : "";
        char lbuf[320];
        if (strcmp(kinds, "launched") == 0) {
            scsprintf(lbuf, sizeof(lbuf), "    launched at %s:%d %s\n",
                srcs, ln, sq_type(fn) == OT_STRING && _string(fn)->_len > 0 ? fns : "<root>");
        } else if (strcmp(kinds, "taskroot") == 0) {
            scsprintf(lbuf, sizeof(lbuf), "    task root: %s (%s:%d)\n", fns, srcs, ln);
        } else {
            bool isAwaited = t->Get(sch.awaited, awaited) && !SQVM::IsFalse(awaited);
            scsprintf(lbuf, sizeof(lbuf), "    %s%s (%s:%d)\n",
                isAwaited ? "awaited at " : "at ", fns, srcs, ln);
        }
        appendCStr(buf, lbuf);

        SQObjectPtr localsObj;
        if (t->Get(sch.locals, localsObj) && sq_type(localsObj) == OT_ARRAY) {
            SQArray *locals = _array(localsObj);
            for (SQInteger li = 0; li < locals->Size(); ++li) {
                SQObjectPtr rec;
                if (locals->Get(li, rec) && sq_type(rec) == OT_TABLE)
                    appendLocalLine(v, _table(rec), buf);
            }
            SQObjectPtr moreObj;
            if (t->Get(sch.localsMore, moreObj) && sq_type(moreObj) == OT_INTEGER && _integer(moreObj) > 0) {
                char mbuf[64];
                scsprintf(mbuf, sizeof(mbuf), "      ... (%d more)\n", (int)_integer(moreObj));
                appendCStr(buf, mbuf);
            }
        }
    }
    return buf.size() != startSize;
}

void sq_print_error_trace(SQVM *v, const SQObject &trace, SQPRINTFUNCTION errfn)
{
    if (!errfn)
        return;
    sqvector<char> buf(_ss(v)->_alloc_ctx);
    if (!sq_append_error_trace(v, trace, buf))
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
