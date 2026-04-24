/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#ifndef NO_COMPILER
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqtable.h"
#include "opcodes.h"
#include "sqvm.h"
#include "sqio.h"
#include "sqclosure.h"
#include "sqarray.h"

#include <stdarg.h>

#define SQ_OPCODE(id) {#id},

SQInstructionDesc g_InstrDesc[]={
    SQ_OPCODES_LIST
};

#undef SQ_OPCODE


static void streamprintf(OutputStream *stream, const char *fmt, ...) {
    static char buffer[4096] = { 0 };
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buffer, 4096, fmt, vl);
    va_end(vl);
    stream->writeString(buffer);
}

static void DumpLiteral(OutputStream *stream, const SQObjectPtr &o)
{
    switch (sq_type(o)) {
        case OT_NULL: streamprintf(stream, "null"); break;
        case OT_STRING: streamprintf(stream, "string(\"%s\")", _stringval(o)); break;
        case OT_FLOAT: streamprintf(stream, "float(%1.9g)", _float(o)); break;
        case OT_INTEGER: streamprintf(stream, "int(" _PRINT_INT_FMT ")", _integer(o)); break;
        case OT_BOOL: streamprintf(stream, "%s", _integer(o) ? "bool(true)" : "bool(false)"); break;
        case OT_ARRAY: streamprintf(stream, "array(0x%p size=%d)", (void*)_rawval(o), int(_array(o)->Size())); break;
        case OT_TABLE: streamprintf(stream, "table(0x%p size=%d)", (void*)_rawval(o), int(_table(o)->CountUsed())); break;
        case OT_CLOSURE:
        {
            SQFunctionProto *func = _closure(o)->_function;
            const char *funcName = sq_isstring(func->_name) ? _stringval(func->_name) : "<null-name>";
            streamprintf(stream, "%s(0x%p \"%s\")", GetTypeName(o), (void*)func, funcName);
            break;
        }
        case OT_NATIVECLOSURE:
        {
            SQNativeClosure *nc = _nativeclosure(o);
            const char* funcName = sq_isstring(nc->_name) ? _stringval(nc->_name) : "<null-name>";
            streamprintf(stream, "%s(0x%p \"%s\")", GetTypeName(o), (void*)nc, funcName);
            break;
        }
        case OT_FUNCPROTO:
        {
            SQFunctionProto *func = _funcproto(o);
            const char* funcName = sq_isstring(func->_name) ? _stringval(func->_name) : "<null-name>";
            streamprintf(stream, "%s(0x%p \"%s\")", GetTypeName(o), (void*)func, funcName);
            break;
        }
        case OT_GENERATOR:
        {
            SQGenerator *gen = _generator(o);
            SQObjectPtr nameObj = _closure(gen->_closure)->_function->_name;
            const char* funcName = sq_isstring(nameObj) ? _stringval(nameObj) : "<null-name>";
            streamprintf(stream, "%s(0x%p \"%s\")", GetTypeName(o), (void*)gen, funcName);
            break;
        }
        default: streamprintf(stream, "%s(0x%p)", GetTypeName(o), (void*)_rawval(o)); break;
    }
}

static uint64_t GetUInt64FromPtr(const void *ptr)
{
    uint64_t res = 0;
    for (int i = 0; i < 8; i++)
        res += uint64_t(((const uint8_t *)ptr)[i]) << (i * 8);
    return res;
}

void DumpInstructions(OutputStream *stream, SQLineInfosHeader *lineinfos, int nlineinfos, const SQInstruction *ii, SQInt32 i_count,
    const SQObjectPtr *_literals, SQInt32 _nliterals, const SQObjectPtr *_functions, SQInt32 _nfunctions, int instruction_index)
{
    SQInt32 n = 0;
    int lineHint = 0;
    for (int i = 0; i < i_count; i++) {
        const SQInstruction &inst = ii[i];
        bool isStepPoint = false;
        char curInstr = instruction_index == i ? '>' : ' ';
        SQInteger line = SQFunctionProto::GetLine(lineinfos, nlineinfos, i, &lineHint, &isStepPoint);
        if (inst.op == _OP_LOAD || inst.op == _OP_DLOAD || inst.op == _OP_PREPCALLK || inst.op == _OP_GETK) {
            SQInteger lidx = inst._arg1;
            streamprintf(stream, "[line%c%03d]%c[op %03d] %15s %d ", isStepPoint ? '-' : ' ', (SQInt32)line, curInstr, (SQInt32)n,
                g_InstrDesc[inst.op].name, inst._arg0);
            if (lidx >= 0xFFFFFFFF) //-V547
                streamprintf(stream, "null");
            else {
                assert(lidx < _nliterals);
                SQObjectPtr key = _literals[lidx];
                DumpLiteral(stream, key);
            }
            if (inst.op != _OP_DLOAD) {
                streamprintf(stream, " %d %d", inst._arg2, inst._arg3);
            }
            else {
                streamprintf(stream, " %d ", inst._arg2);
                lidx = inst._arg3;
                if (lidx >= 0xFFFFFFFF) //-V547
                    streamprintf(stream, "null");
                else {
                    assert(lidx < _nliterals);
                    SQObjectPtr key = _literals[lidx];
                    DumpLiteral(stream, key);
                }
            }
        }
        /*  else if(inst.op==_OP_ARITH){
                printf("[%03d] %15s %d %d %d %c\n",n,g_InstrDesc[inst.op].name,inst._arg0,inst._arg1,inst._arg2,inst._arg3);
            }*/
        else {
            int jumpLabel = 0x7FFFFFFF;
            switch (inst.op) {
            case _OP_JMP:
                jumpLabel = i + inst._arg1 + 1;
                break;
            default:
                break;
            }
            bool arg1F = inst.op == _OP_JCMPF || inst.op == _OP_LOADFLOAT;
            if (arg1F)
                streamprintf(stream, "[line%c%03d]%c[op %03d] %15s %d %1.9g %d %d", isStepPoint ? '-' : ' ', (SQInt32)line, curInstr, (SQInt32)n,
                    g_InstrDesc[inst.op].name, inst._arg0, inst._farg1, inst._arg2, inst._arg3);
            else
            {
                if (i > 0 && (ii[i - 1].op == _OP_SET_LITERAL || ii[i - 1].op == _OP_GET_LITERAL))
                    streamprintf(stream, "[line%c%03d]%c[op %03d] HINT (0x%" PRIx64 ")", isStepPoint ? '-' : ' ', (SQInt32)line, curInstr, (SQInt32)n,
                        GetUInt64FromPtr(ii + i));
                else if (inst.op < sizeof(g_InstrDesc) / sizeof(g_InstrDesc[0]))
                    streamprintf(stream, "[line%c%03d]%c[op %03d] %15s %d %d %d %d", isStepPoint ? '-' : ' ', (SQInt32)line, curInstr, (SQInt32)n,
                        g_InstrDesc[inst.op].name, inst._arg0, inst._arg1, inst._arg2, inst._arg3);
                else
                    streamprintf(stream, "[line%c%03d]%c[op %03d] INVALID INSTRUNCTION (%d)", isStepPoint ? '-' : ' ', (SQInt32)line, curInstr, (SQInt32)n,
                        int(inst.op));
            }
            if (jumpLabel != 0x7FFFFFFF)
                streamprintf(stream, "  // jump to %d", jumpLabel);
        }

        // comments
        switch (inst.op) {
            case _OP_LOAD:
            case _OP_LOADFLOAT:
            case _OP_LOADINT:
            case _OP_LOADBOOL:
            case _OP_LOADROOT:
            case _OP_LOADCALLEE:
                streamprintf(stream, "  // -> r%d", int(inst._arg0));
                break;

            case _OP_LOADNULLS:
                streamprintf(stream, "  // null -> [r%d .. r%d]", int(inst._arg0), int(inst._arg0 + inst._arg1 - 1));
                break;

            case _OP_DLOAD: {
                streamprintf(stream, "  //");
                if (unsigned(inst._arg1) < unsigned(_nliterals)) {
                    const SQObjectPtr &key = _literals[inst._arg1];
                    DumpLiteral(stream, key);
                }
                else {
                    streamprintf(stream, "?");
                }
                streamprintf(stream, " -> r%d, ", int(inst._arg0));
                if (unsigned(inst._arg3) < unsigned(_nliterals)) {
                    const SQObjectPtr &key = _literals[inst._arg3];
                    DumpLiteral(stream, key);
                }
                else {
                    streamprintf(stream, "?");
                }
                streamprintf(stream, " -> r%d", int(inst._arg2));
                break;
            }

            case _OP_DATA_NOP: {
                    int saveInstr = i + ((inst._arg2 << 8) + inst._arg3);
                    if ((inst._arg0 || inst._arg1 || inst._arg2 || inst._arg3) && saveInstr > 0 && saveInstr < i_count &&
                        ii[saveInstr].op == _OP_SAVE_STATIC_MEMO)
                    {
                        int loadInstr = saveInstr + 1 - ((ii[saveInstr]._arg2 << 8) + ii[saveInstr]._arg3);
                        if (loadInstr == i) {
                            streamprintf(stream, "  // LOAD_STATIC_MEMO: staticmemo[%d] -> r%d, jump to %d%s",
                                int(inst._arg1 & STATIC_MEMO_IDX_MASK), int(inst._arg0), saveInstr + 1,
                                (inst._arg1 & STATIC_MEMO_AUTO_FLAG) ? " (auto)" : "");
                        }
                    }
                }
                break;

            case _OP_SAVE_STATIC_MEMO: {
                    int loadInstr = i + 1 - ((inst._arg2 << 8) + inst._arg3);
                    streamprintf(stream, "  // r%d -> staticmemo[%d], r%d -> r%d, modify instr at %d%s",
                        int(inst._arg0), int(inst._arg1 & STATIC_MEMO_IDX_MASK), int(inst._arg0), int(ii[loadInstr]._arg0), loadInstr,
                        (inst._arg1 & STATIC_MEMO_AUTO_FLAG) ? " (auto)" : "");
                }
                break;

            case _OP_CALL:
                streamprintf(stream, "  // (call r%d) -> r%d", int(inst._arg1), int(inst._arg0));
                break;

            case _OP_NULLCALL:
                streamprintf(stream, "  // (if r%d then call r%d) -> r%d", int(inst._arg1), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_MOVE:
                streamprintf(stream, "  // r%d -> r%d", int(inst._arg1), int(inst._arg0));
                break;

            case _OP_GET:
                streamprintf(stream, "  // r%d[r%d] -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_NEWOBJ: {
                const char *objType = inst._arg3 == NEWOBJ_TABLE ? "table" : (inst._arg3 == NEWOBJ_ARRAY ? "array" :
                    (inst._arg3 == NEWOBJ_CLASS ? "class" : "unknown"));
                streamprintf(stream, "  // new %s -> r%d", objType, int(inst._arg0));
                break;
            }

            case _OP_CLOSURE: {
                streamprintf(stream, "  // closure ");
                if (unsigned(inst._arg1) < unsigned(_nfunctions) && sq_isfunction(_functions[inst._arg1])) {
                    SQFunctionProto *fp = _funcproto(_functions[inst._arg1]);
                    if (sq_type(fp->_name) == OT_STRING)
                        streamprintf(stream, "%s", _stringval(fp->_name));
                }
                streamprintf(stream, " -> r%d", int(inst._arg0));
                break;
            }

            case _OP_APPENDARRAY: {
                streamprintf(stream, "  // r%d.append(", int(inst._arg0));
                switch (inst._arg2) {
                    case AAT_STACK:
                        streamprintf(stream, "r%d", int(inst._arg1));
                        break;
                    case AAT_LITERAL:
                        if (unsigned(inst._arg1) < unsigned(_nliterals)) {
                            const SQObjectPtr &key = _literals[inst._arg1];
                            DumpLiteral(stream, key);
                        }
                        else {
                            streamprintf(stream, "?");
                        }
                        break;
                    case AAT_INT:
                        streamprintf(stream, "%d", int(inst._arg1));
                        break;
                    case AAT_FLOAT:
                        streamprintf(stream, "%1.9g", *((const SQFloat *)&inst._arg1));
                        break;
                    case AAT_BOOL:
                        streamprintf(stream, "%s", inst._arg1 ? "true" : "false");
                        break;
                    default:
                        streamprintf(stream, "unknown");
                        break;
                }

                streamprintf(stream, ")");
                break;
            }

            case _OP_GETK:
            case _OP_GET_LITERAL: {
                streamprintf(stream, "  // r%d.[", int(inst._arg2));
                if (unsigned(inst._arg1) < unsigned(_nliterals)) {
                    const SQObjectPtr &key = _literals[inst._arg1];
                    DumpLiteral(stream, key);
                }
                else {
                    streamprintf(stream, "?");
                }
                streamprintf(stream, "] -> r%d", int(inst._arg0));
                break;
            }

            case _OP_SET:
                if (inst._arg0 != 0xFF)
                    streamprintf(stream, "  // r%d[r%d] = r%d -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg3), int(inst._arg0));
                else
                    streamprintf(stream, "  // r%d[r%d] = r%d", int(inst._arg2), int(inst._arg1), int(inst._arg3));
                break;

            case _OP_SETK:
            case _OP_SET_LITERAL: {
                streamprintf(stream, "  // r%d[", int(inst._arg2));
                if (unsigned(inst._arg1) < unsigned(_nliterals)) {
                    const SQObjectPtr &key = _literals[inst._arg1];
                    DumpLiteral(stream, key);
                }
                else {
                    streamprintf(stream, "?");
                }
                if (inst._arg0 != 0xFF)
                    streamprintf(stream, "] = r%d -> r%d", int(inst._arg3), int(inst._arg0));
                else
                    streamprintf(stream, "] = r%d", int(inst._arg3));
                break;
            }

            case _OP_SETI:
                if (inst._arg0 != 0xFF)
                    streamprintf(stream, "  // r%d[%d] = r%d -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg3), int(inst._arg0));
                else
                    streamprintf(stream, "  // r%d[%d] = r%d", int(inst._arg2), int(inst._arg1), int(inst._arg3));
                break;

            case _OP_NEWSLOT:
                streamprintf(stream, "  // r%d[r%d] <- r%d", int(inst._arg2), int(inst._arg1), int(inst._arg3));
                break;

            case _OP_NEWSLOTK: {
                streamprintf(stream, "  // r%d[", int(inst._arg2));
                if (unsigned(inst._arg1) < unsigned(_nliterals)) {
                    const SQObjectPtr &key = _literals[inst._arg1];
                    DumpLiteral(stream, key);
                }
                else {
                    streamprintf(stream, "?");
                }
                streamprintf(stream, "] <- r%d", int(inst._arg3));
                break;
            }

            case _OP_NEWSLOTA:
                streamprintf(stream, "  // r%d[r%d] <- r%d %s", int(inst._arg1), int(inst._arg2), int(inst._arg3),
                    (inst._arg0 & NEW_SLOT_STATIC_FLAG) ? " static" : "");
                break;

            case _OP_DELETE:
                streamprintf(stream, "  // delete r%d[r%d] -> r%d", int(inst._arg1), int(inst._arg2), int(inst._arg0));
                break;

            case _OP_PREPCALL:
                streamprintf(stream, "  // r%d[r%d] -> func=r%d, this=r%d", int(inst._arg2), int(inst._arg1), int(inst._arg0), int(inst._arg3));
                break;

            case _OP_PREPCALLK: {
                streamprintf(stream, "  // r%d[", int(inst._arg2));
                if (unsigned(inst._arg1) < unsigned(_nliterals)) {
                    const SQObjectPtr &key = _literals[inst._arg1];
                    DumpLiteral(stream, key);
                }
                else {
                    streamprintf(stream, "?");
                }
                streamprintf(stream, "] -> func=r%d, this=r%d", int(inst._arg0), int(inst._arg3));
                break;
            }

            case _OP_TAILCALL:
                streamprintf(stream, "  // tailcall r%d(%d args) -> r%d", int(inst._arg1), int(inst._arg3), int(inst._arg0));
                break;

            case _OP_DMOVE:
                streamprintf(stream, "  // r%d -> r%d, r%d -> r%d", int(inst._arg1), int(inst._arg0), int(inst._arg3), int(inst._arg2));
                break;

            case _OP_RETURN:
                if (inst._arg0 != 0xFF)
                    streamprintf(stream, "  // return r%d", int(inst._arg1));
                else
                    streamprintf(stream, "  // return null");
                break;

            case _OP_EQ:
            case _OP_NE: {
                const char *op = inst.op == _OP_EQ ? "==" : "!=";
                if (inst._arg3) {
                    streamprintf(stream, "  // r%d %s ", int(inst._arg2), op);
                    if (unsigned(inst._arg1) < unsigned(_nliterals)) {
                        const SQObjectPtr &key = _literals[inst._arg1];
                        DumpLiteral(stream, key);
                    }
                    else {
                        streamprintf(stream, "?");
                    }
                    streamprintf(stream, " -> r%d", int(inst._arg0));
                }
                else
                    streamprintf(stream, "  // r%d %s r%d -> r%d", int(inst._arg2), op, int(inst._arg1), int(inst._arg0));
                break;
            }

            case _OP_ADD:
                streamprintf(stream, "  // r%d + r%d -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_SUB:
                streamprintf(stream, "  // r%d - r%d -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_MUL:
                streamprintf(stream, "  // r%d * r%d -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_DIV:
                streamprintf(stream, "  // r%d / r%d -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_MOD:
                streamprintf(stream, "  // r%d %% r%d -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_ADDI:
                streamprintf(stream, "  // r%d + %d -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_BITW: {
                const char *bwop = "?";
                switch (inst._arg3) {
                    case BW_AND: bwop = "&"; break;
                    case BW_OR: bwop = "|"; break;
                    case BW_XOR: bwop = "^"; break;
                    case BW_SHIFTL: bwop = "<<"; break;
                    case BW_SHIFTR: bwop = ">>"; break;
                    case BW_USHIFTR: bwop = ">>>"; break;
                }
                streamprintf(stream, "  // r%d %s r%d -> r%d", int(inst._arg2), bwop, int(inst._arg1), int(inst._arg0));
                break;
            }

            case _OP_NEG:
                streamprintf(stream, "  // -r%d -> r%d", int(inst._arg1), int(inst._arg0));
                break;

            case _OP_NOT:
                streamprintf(stream, "  // !r%d -> r%d", int(inst._arg1), int(inst._arg0));
                break;

            case _OP_BWNOT:
                streamprintf(stream, "  // ~r%d -> r%d", int(inst._arg1), int(inst._arg0));
                break;

            case _OP_CMP: {
                const char *cmpop = "?";
                switch (inst._arg3) {
                    case CMP_G: cmpop = ">"; break;
                    case CMP_GE: cmpop = ">="; break;
                    case CMP_L: cmpop = "<"; break;
                    case CMP_LE: cmpop = "<="; break;
                    case CMP_3W: cmpop = "<=>"; break;
                }
                streamprintf(stream, "  // r%d %s r%d -> r%d", int(inst._arg2), cmpop, int(inst._arg1), int(inst._arg0));
                break;
            }

            case _OP_EXISTS:
                streamprintf(stream, "  // r%d in r%d -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_INSTANCEOF:
                streamprintf(stream, "  // r%d instanceof r%d -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg0));
                break;

            case _OP_AND:
                streamprintf(stream, "  // if r%d is false: r%d -> r%d, jump to %d", int(inst._arg2), int(inst._arg2), int(inst._arg0), i + inst._arg1 + 1);
                break;

            case _OP_OR:
                streamprintf(stream, "  // if r%d is true: r%d -> r%d, jump to %d", int(inst._arg2), int(inst._arg2), int(inst._arg0), i + inst._arg1 + 1);
                break;

            case _OP_NULLCOALESCE:
                streamprintf(stream, "  // if r%d != null: r%d -> r%d, jump to %d", int(inst._arg2), int(inst._arg2), int(inst._arg0), i + inst._arg1);
                break;

            case _OP_GETOUTER:
                streamprintf(stream, "  // outer[%d] -> r%d", int(inst._arg1), int(inst._arg0));
                break;

            case _OP_SETOUTER:
                if (inst._arg0 != 0xFF)
                    streamprintf(stream, "  // r%d -> outer[%d] -> r%d", int(inst._arg2), int(inst._arg1), int(inst._arg0));
                else
                    streamprintf(stream, "  // r%d -> outer[%d]", int(inst._arg2), int(inst._arg1));
                break;

            case _OP_INC:
                streamprintf(stream, "  // r%d[r%d] += %d -> r%d", int(inst._arg1), int(inst._arg2), int(inst._sarg3()), int(inst._arg0));
                break;

            case _OP_INCL:
                streamprintf(stream, "  // r%d += %d", int(inst._arg1), int(inst._sarg3()));
                break;

            case _OP_PINC:
                streamprintf(stream, "  // r%d[r%d] (old -> r%d) += %d", int(inst._arg1), int(inst._arg2), int(inst._arg0), int(inst._sarg3()));
                break;

            case _OP_PINCL:
                streamprintf(stream, "  // r%d (old -> r%d) += %d", int(inst._arg1), int(inst._arg0), int(inst._sarg3()));
                break;

            case _OP_COMPARITH: {
                int selfidx = (unsigned(inst._arg1) >> 16) & 0xFFFF;
                int validx = unsigned(inst._arg1) & 0xFFFF;
                streamprintf(stream, "  // r%d[r%d] %c= r%d -> r%d", selfidx, int(inst._arg2),
                    char(inst._arg3), validx, int(inst._arg0));
                break;
            }

            case _OP_COMPARITH_K: {
                int selfidx = (unsigned(inst._arg1) >> 16) & 0xFFFF;
                int validx = unsigned(inst._arg1) & 0xFFFF;
                streamprintf(stream, "  // r%d.[", selfidx);
                if (unsigned(inst._arg2) < unsigned(_nliterals)) {
                    const SQObjectPtr &key = _literals[inst._arg2];
                    DumpLiteral(stream, key);
                }
                else {
                    streamprintf(stream, "?");
                }
                streamprintf(stream, "] %c= r%d -> r%d", char(inst._arg3), validx, int(inst._arg0));
                break;
            }

            case _OP_YIELD:
                if (inst._arg1 != 0xFF)
                    streamprintf(stream, "  // yield r%d", int(inst._arg1));
                else
                    streamprintf(stream, "  // yield null");
                break;

            case _OP_RESUME:
                streamprintf(stream, "  // resume r%d -> r%d", int(inst._arg1), int(inst._arg0));
                break;

            case _OP_CLONE:
                streamprintf(stream, "  // clone r%d -> r%d", int(inst._arg1), int(inst._arg0));
                break;

            case _OP_TYPEOF:
                streamprintf(stream, "  // typeof r%d -> r%d", int(inst._arg1), int(inst._arg0));
                break;

            case _OP_THROW:
                streamprintf(stream, "  // throw r%d", int(inst._arg0));
                break;

            case _OP_GETBASE:
                streamprintf(stream, "  // base -> r%d", int(inst._arg0));
                break;

            case _OP_CLOSE:
                streamprintf(stream, "  // close outers from r%d", int(inst._arg1));
                break;

            case _OP_PREFOREACH:
                streamprintf(stream, "  // init iter r%d -> [r%d..r%d], if empty jump to %d", int(inst._arg0), int(inst._arg2), int(inst._arg2 + 2), i + inst._arg1 + 1);
                break;

            case _OP_FOREACH:
                streamprintf(stream, "  // iter r%d next -> key=r%d, val=r%d, jump to %d", int(inst._arg0), int(inst._arg2), int(inst._arg2 + 1), i + inst._arg1 + 1);
                break;

            case _OP_POSTFOREACH:
                streamprintf(stream, "  // if iter r%d dead jump to %d", int(inst._arg0), i + inst._arg1 + 1);
                break;

            case _OP_FREEZE:
                streamprintf(stream, "  // freeze r%d -> r%d", int(inst._arg1), int(inst._arg0));
                break;

            case _OP_CHECK_TYPE:
                streamprintf(stream, "  // check r%d type mask=0x%X", int(inst._arg0), int(inst._arg1));
                break;

            case _OP_JZ:
                if (inst._arg2)
                    streamprintf(stream, "  // if r%d jump to %d", int(inst._arg0), i + inst._arg1 + 1);
                else
                    streamprintf(stream, "  // if not r%d jump to %d", int(inst._arg0), i + inst._arg1 + 1);
                break;

            case _OP_JCMP: {
                const char *cmpop = "?";
                bool negate = !(inst._arg3 >> 3);
                switch (inst._arg3 & 0x7) {
                    case CMP_G: cmpop = negate ? "<=" : ">"; break;
                    case CMP_GE: cmpop = negate ? "<" : ">="; break;
                    case CMP_L: cmpop = negate ? ">=" : "<"; break;
                    case CMP_LE: cmpop = negate ? ">" : "<="; break;
                    case CMP_3W: cmpop = "<=>"; break;
                }
                streamprintf(stream, "  // if r%d %s r%d jump to %d", int(inst._arg2), cmpop, int(inst._arg0), i + inst._arg1 + 1);
                break;
            }

            case _OP_JCMPK: {
                const char *cmpop = "?";
                bool negate = !(inst._arg3 >> 3);
                switch (inst._arg3 & 0x7) {
                    case CMP_G: cmpop = negate ? "<=" : ">"; break;
                    case CMP_GE: cmpop = negate ? "<" : ">="; break;
                    case CMP_L: cmpop = negate ? ">=" : "<"; break;
                    case CMP_LE: cmpop = negate ? ">" : "<="; break;
                    case CMP_3W: cmpop = "<=>"; break;
                }
                streamprintf(stream, "  // if r%d %s ", int(inst._arg2), cmpop);
                if (unsigned(inst._arg0) < unsigned(_nliterals)) {
                    const SQObjectPtr &key = _literals[inst._arg0];
                    DumpLiteral(stream, key);
                }
                else {
                    streamprintf(stream, "?");
                }
                streamprintf(stream, " jump to %d", i + inst._arg1 + 1);
                break;
            }

            case _OP_JCMPI: {
                const char *cmpop = "?";
                bool negate = !(inst._arg3 >> 3);
                switch (inst._arg3 & 0x7) {
                    case CMP_G: cmpop = negate ? "<=" : ">"; break;
                    case CMP_GE: cmpop = negate ? "<" : ">="; break;
                    case CMP_L: cmpop = negate ? ">=" : "<"; break;
                    case CMP_LE: cmpop = negate ? ">" : "<="; break;
                    case CMP_3W: cmpop = "<=>"; break;
                }
                streamprintf(stream, "  // if r%d %s %d jump to %d", int(inst._arg2), cmpop, int(inst._arg1), i + inst._sarg0() + 1);
                break;
            }

            case _OP_JCMPF: {
                const char *cmpop = "?";
                bool negate = !(inst._arg3 >> 3);
                switch (inst._arg3 & 0x7) {
                    case CMP_G: cmpop = negate ? "<=" : ">"; break;
                    case CMP_GE: cmpop = negate ? "<" : ">="; break;
                    case CMP_L: cmpop = negate ? ">=" : "<"; break;
                    case CMP_LE: cmpop = negate ? ">" : "<="; break;
                    case CMP_3W: cmpop = "<=>"; break;
                }
                streamprintf(stream, "  // if r%d %s %g jump to %d", int(inst._arg2), cmpop, inst._farg1, i + inst._sarg0() + 1);
                break;
            }

            case _OP_LOAD_STATIC_MEMO:
                streamprintf(stream, "  // staticmemo[%d] -> r%d, jump to %d", int(inst._arg1), int(inst._arg0), i + (inst._arg2 << 8) + inst._arg3 + 1);
                break;

            case _OP_PUSHTRAP:
                streamprintf(stream, "  // catch -> r%d, jump to %d", int(inst._arg0), i + inst._arg1 + 1);
                break;

            case _OP_POPTRAP:
                streamprintf(stream, "  // pop %d trap(s)", int(inst._arg0));
                break;

            case _OP_PATCH_DOCOBJ:
                streamprintf(stream, "  // patch docobj r%d", int(inst._arg0));
                break;

            default:
                break;
        }
        streamprintf(stream, "\n");
        n++;
    }
}

static void DumpLiterals(OutputStream *stream, const SQObjectPtr *_literals, SQInt32 _nliterals)
{
    streamprintf(stream, "-----LITERALS\n");
    for (SQInt32 i = 0; i < _nliterals; ++i) {
        streamprintf(stream, "[%d] ", (SQInt32)i);
        DumpLiteral(stream, _literals[i]);
        streamprintf(stream, "\n");
    }
}

static void DumpStaticMemos(OutputStream *stream, const SQObjectPtr *_staticmemos, SQInt32 _nstaticmemos)
{
    streamprintf(stream, "-----STATIC MEMOS\n");
    for (SQInt32 i = 0; i < _nstaticmemos; ++i) {
        streamprintf(stream, "[%d] ", (SQInt32)i);
        DumpLiteral(stream, _staticmemos[i]);
        streamprintf(stream, "\n");
    }
}

static void DumpLocals(OutputStream *stream, const SQLocalVarInfo *_localvarinfos, SQInt32 _nlocalvarinfos)
{
    streamprintf(stream, "-----LOCALS\n");
    for (SQInt32 si = 0; si < _nlocalvarinfos; si++) {
        SQLocalVarInfo lvi = _localvarinfos[si];
        streamprintf(stream, "[%d] %s \t%d %d\n", (SQInt32)lvi._pos, _stringval(lvi._name), (SQInt32)lvi._start_op, (SQInt32)lvi._end_op);
    }
}

static void DumpLineInfo(OutputStream *stream, const SQLineInfosHeader *_lineinfos, SQInt32 _nlineinfos)
{
    streamprintf(stream, "-----LINE INFO\n");
    for (SQInt32 i = 0; i < _nlineinfos; i++) {
        int op = 0;
        int line = 0;
        bool isDbgStepPoint = false;
        if (_lineinfos->_is_compressed) {
            SQCompressedLineInfo li = ((SQCompressedLineInfo *)(void *)(_lineinfos + 1))[i];
            op = li._op;
            line = _lineinfos->_first_line + li._line_offset;
            isDbgStepPoint = li._is_dbg_step_point;
        } else {
            SQFullLineInfo li = ((SQFullLineInfo *)(void *)(_lineinfos + 1))[i];
            op = li._op;
            line = _lineinfos->_first_line + li._line_offset;
            isDbgStepPoint = li._is_dbg_step_point;
        }
        streamprintf(stream, "op [%d] line [%d]%s\n", op, line, isDbgStepPoint ? " dbgstep" : "");
    }
}

void Dump(OutputStream *stream, SQFunctionProto *func, bool deep, int instruction_index)
{

    //if (!dump_enable) return ;
    SQUnsignedInteger n = 0, i;
    if (deep) {
        for (i = 0; i < func->_nfunctions; ++i) {
            SQObjectPtr &f = func->_functions[i];
            assert(sq_isfunction(f));
            Dump(stream, _funcproto(f), deep, -1);
        }
    }
    streamprintf(stream, "SQInstruction sizeof %d\n", (SQInt32)sizeof(SQInstruction));
    streamprintf(stream, "SQObject sizeof %d\n", (SQInt32)sizeof(SQObject));
    streamprintf(stream, "--------------------------------------------------------------------\n");
    streamprintf(stream, "*****FUNCTION [%s]\n", sq_type(func->_name) == OT_STRING ? _stringval(func->_name) : "unknown");
    DumpLiterals(stream, func->_literals, func->_nliterals);
    DumpStaticMemos(stream, func->_staticmemos, func->_nstaticmemos);
    streamprintf(stream, "-----RESULT TYPE MASK = 0x%X\n", func->_result_type_mask);
    streamprintf(stream, "-----PARAMS\n");
    if (func->_varparams)
        streamprintf(stream, "<<VARPARAMS>>\n");
    n = 0;
    for (i = 0; i < func->_nparameters; i++) {
        streamprintf(stream, "[%d] ", (SQInt32)n);
        DumpLiteral(stream, func->_parameters[i]);
        streamprintf(stream, ", type mask = 0x%X\n", func->_param_type_masks[i]);
        n++;
    }
    DumpLocals(stream, func->_localvarinfos, func->_nlocalvarinfos);
    DumpLineInfo(stream, func->_lineinfos, func->_nlineinfos);
    streamprintf(stream, "-----dump\n");
    DumpInstructions(stream, func->_lineinfos, func->_nlineinfos,
        func->_instructions, func->_ninstructions, func->_literals, func->_nliterals,
        func->_functions, func->_nfunctions, instruction_index);
    streamprintf(stream, "-----\n");
    streamprintf(stream, "stack size[%d]\n", (SQInt32)func->_stacksize);
    streamprintf(stream, "--------------------------------------------------------------------\n\n");
}

void Dump(SQFunctionProto *func, int instruction_index) {
    FileOutputStream fos(stdout);
    Dump(&fos, func, false, instruction_index);
}

#endif
