#include "sqpcheader.h"
#include "sqcompiler.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqtable.h"
#include "sqopcodes.h"
#include "sqfuncstate.h"
#include "sqoptimizer.h"

SQOptimizer::SQOptimizer(SQFuncState & func_state) : fs(&func_state), jumps(func_state._lineinfos._alloc_ctx), codeChanged(false) {}
#undef _DEBUG_DUMP

#ifdef _DEBUG_DUMP
 void SQOptimizer::debugPrintInstructionPos(const SQChar * message, int instructionIndex)
 {
    for (int i = 0; i < fs->_lineinfos.size(); i++)
        if (fs->_lineinfos[i]._op >= instructionIndex) {
            printf(_SC("OPTIMIZER: %s  %s:%d\n"), message, _stringval(fs->_sourcename), fs->_lineinfos[i]._line);
            return;
        }
    if (fs->_lineinfos.size() > 0)
        printf(_SC("OPTIMIZER: %s  %s:%d\n"), message, _stringval(fs->_sourcename), fs->_lineinfos.top()._line);
 }
#endif


bool SQOptimizer::isUnsafeRange(int start, int count)
{
    for (int i = 0; i < jumps.size(); i++) {
        int to = jumps[i].jumpTo;
        if (to > start && to < start + count)
            return true;
    }

    for (int i = 0; i < fs->_localvarinfos.size(); i++) {
        int pos = int(fs->_localvarinfos[i]._start_op);
        if (pos > start && pos < start + count)
            return true;
    }

    return false;
}

void SQOptimizer::cutRange(int start, int old_count, int new_count)
{
    assert(new_count < old_count);
    int tmpStart = start + new_count;
    int tmpCount = old_count - new_count;
    //printf("cut: start=%d count=%d\n", tmpStart, tmpCount);
    for (int i = 0; i < jumps.size(); i++) {
        if (jumps[i].jumpTo >= tmpStart + 1) {
            jumps[i].jumpTo -= tmpCount;
            jumps[i].modified = true;
        }
        if (jumps[i].instructionIndex >= tmpStart) {
            jumps[i].instructionIndex -= tmpCount;
            jumps[i].modified = true;
        }
    }

    memmove(&fs->_instructions[tmpStart], &fs->_instructions[tmpStart + tmpCount],
        sizeof(fs->_instructions[0]) * (fs->_instructions.size() - tmpStart - tmpCount));
    fs->_instructions.resize(fs->_instructions.size() - tmpCount);

    for (int i = 0; i < fs->_localvarinfos.size(); i++) {
        SQLocalVarInfo & varinfo = fs->_localvarinfos[i];
        if (varinfo._start_op > tmpStart) {
            if (varinfo._end_op < varinfo._start_op) {
                varinfo._end_op = varinfo._start_op - tmpCount - 1;
                if (varinfo._end_op > tmpStart)
                    varinfo._end_op = 0;
            }
            varinfo._start_op -= tmpCount;
        }
        if (varinfo._end_op >= tmpStart && varinfo._end_op != UINT_MINUS_ONE) {
            int n = int(varinfo._end_op) - tmpCount;
            if (n < tmpStart - 1)
                n = tmpStart - 1;
            if (n < 0)
                n = 0;
            varinfo._end_op = n;
        }
    }

    for (int i = 0; i < fs->_lineinfos.size(); i++)
        if (fs->_lineinfos[i]._op > tmpStart)
            fs->_lineinfos[i]._op -= tmpCount;

    codeChanged = true;
}


void SQOptimizer::optimizeConstFolding()
{
    SQInstructionVec & instr = fs->_instructions;
    bool changed = false;
    for (int i = 0; i + 3 < instr.size(); i++) {
        do {
            changed = false;
            if (i + 3 < instr.size()) {
                SQInstruction & operation = instr[i + 2];
                SQInstruction & loadA = instr[i];
                SQInstruction & loadB = instr[i + 1];
                int s = operation.op;

                if ((s == _OP_ADD || s == _OP_SUB || s == _OP_MUL || s == _OP_DIV || s == _OP_MOD || s == _OP_BITW) &&
                        (loadA.op == _OP_LOADINT || loadA.op == _OP_LOADFLOAT) &&
                        (loadB.op == _OP_LOADINT || loadB.op == _OP_LOADFLOAT) &&
                        loadB._arg0 == operation._arg1 && loadA._arg0 == operation._arg2) {

                    if (!isUnsafeRange(i, 3)) {

                        if (loadA.op == _OP_LOADINT && loadB.op == _OP_LOADINT) {
                            SQInteger res = 0;
                            SQInt32 lv = loadA._arg1;
                            SQInt32 rv = loadB._arg1;
                            bool applyOpt = true;
                            switch (s) {
                                case _OP_ADD: res = SQInteger(lv) + SQInteger(rv); break;
                                case _OP_SUB: res = SQInteger(lv) - SQInteger(rv); break;
                                case _OP_MUL: res = SQInteger(lv) * SQInteger(rv); break;
                                case _OP_DIV:
                                    if (rv < -1 || rv > 0)
                                        res = lv / rv;
                                    else
                                        applyOpt = false;
                                    break;
                                case _OP_MOD:
                                    if (rv < -1 || rv > 0)
                                        res = lv % rv;
                                    else
                                        applyOpt = false;
                                    break;
                                case _OP_BITW:
                                    switch (operation._arg3) {
                                        case BW_AND: res = lv & rv; break;
                                        case BW_OR: res = lv | rv; break;
                                        case BW_XOR: res = lv ^ rv; break;
                                        case BW_SHIFTL: res = SQInteger(lv) << rv; break;
                                        default: applyOpt = false; break;
                                    }
                                    break;

                                default: applyOpt = false; break;
                            }

                            if (applyOpt && res >= SQInteger(INT_MIN) && res <= SQInteger(INT_MAX)) {
                                instr[i]._arg1 = (SQInt32)res;
                                instr[i]._arg0 = operation._arg0;
                                cutRange(i, 3, 1);
                                changed = true;
                                codeChanged = true;
                                #ifdef _DEBUG_DUMP
                                    debugPrintInstructionPos(_SC("Const folding"), i);
                                #endif
                            }
                        } else { // float
                            assert(sizeof(SQFloat) == sizeof(SQInt32));
                            SQFloat res = 0;
                            SQFloat lv = (loadA.op == _OP_LOADFLOAT) ? *((SQFloat *) &loadA._arg1) : SQFloat(loadA._arg1);
                            SQFloat rv = (loadB.op == _OP_LOADFLOAT) ? *((SQFloat *) &loadB._arg1) : SQFloat(loadB._arg1);
                            bool applyOpt = true;
                            switch (s) {
                                case _OP_ADD: res = lv + rv; break;
                                case _OP_SUB: res = lv - rv; break;
                                case _OP_MUL: res = lv * rv; break;
                                case _OP_DIV:
                                    if (rv != 0)
                                        res = lv / rv;
                                    else
                                        applyOpt = false;
                                    break;
                                default: applyOpt = false; break;
                            }

                            if (applyOpt) {
                                instr[i].op = _OP_LOADFLOAT;
                                instr[i]._arg1 = *((SQInt32*) &res);
                                instr[i]._arg0 = operation._arg0;
                                cutRange(i, 3, 1);
                                changed = true;
                                codeChanged = true;
                                #ifdef _DEBUG_DUMP
                                    debugPrintInstructionPos(_SC("Const folding"), i);
                                #endif
                            }
                        }
                    }
                }
            }
        } while (changed && i + 3 < instr.size());
    }
}


void SQOptimizer::optimizeJumpFolding()
{
    SQInstructionVec & instr = fs->_instructions;
    bool changed = true;
    for (int pass = 0; pass < 4 && changed; pass++) {
        changed = false;
        for (int i = 0; i < instr.size(); i++) {
            int op = instr[i].op;
            if (op == _OP_JMP || op == _OP_JCMP || op == _OP_JZ || op == _OP_AND || op == _OP_OR || op == _OP_PUSHTRAP) {
                int to = (i + instr[i]._arg1 + 1);
                if (instr[to].op == _OP_JMP) {
                    changed = true;
                    codeChanged = true;
                    instr[i]._arg1 += instr[to]._arg1 + 1;
                    #ifdef _DEBUG_DUMP
                        debugPrintInstructionPos(_SC("Jump folding"), i);
                    #endif
                }
            }
        }
    }
}

void SQOptimizer::optimizeEmptyJumps()
{
    SQInstructionVec & instr = fs->_instructions;
    if (!instr.size())
        return;

    for (int i = instr.size() - 1; i > 1; i--) {
        int op = instr[i].op;
        if (op == _OP_JMP && instr[i]._arg1 == 0) {
            codeChanged = true;
            cutRange(i, 1, 0);
            #ifdef _DEBUG_DUMP
                debugPrintInstructionPos(_SC("Empty jump"), i);
            #endif
        }
    }
}

void SQOptimizer::optimize()
{
    codeChanged = true;
    for (int pass = 0; pass < 2 && codeChanged; pass++) {
        jumps.resize(0);
        SQInstructionVec & instr = fs->_instructions;
        for (int i = 0; i < instr.size(); i++)
            switch (instr[i].op) {
                case _OP_JMP:
                case _OP_JCMP:
                case _OP_JZ:
                case _OP_AND:
                case _OP_OR:
                case _OP_PUSHTRAP:
                case _OP_FOREACH:
                    jumps.push_back({i, i, i + instr[i]._arg1 + 1, i + instr[i]._arg1 + 1, false});
                    break;
                case _OP_POSTFOREACH:
                case _OP_NULLCOALESCE:
                    jumps.push_back({i, i, i + instr[i]._arg1, i + instr[i]._arg1, false});
                    break;
                default:
                    break;
            }

        codeChanged = false;
        optimizeConstFolding();
        optimizeEmptyJumps();

        if (codeChanged)
            for (int i = 0; i < jumps.size(); i++)
                if (jumps[i].modified) {
                    instr[jumps[i].instructionIndex]._arg1 +=
                        (jumps[i].jumpTo - jumps[i].originalJumpTo) - (jumps[i].instructionIndex - jumps[i].originalInstructionIndex);
                }
    }

    optimizeJumpFolding();

#ifdef _DEBUG_DUMP
    for (int i = 0; i < jumps.size(); i++)
        printf(_SC("JUMPS:  instruction: %d  to: %d\n"), jumps[i].instructionIndex, jumps[i].jumpTo);
#endif

}


