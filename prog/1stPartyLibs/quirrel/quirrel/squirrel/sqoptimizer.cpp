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


bool SQOptimizer::isUnsafeJumpRange(int start, int count) const
{
    for (int i = 0, ie = jumps.size(); i < ie; i++) {
        int to = jumps[i].jumpTo;
        if (to > start && to < start + count)
            return true;
    }
    return false;
}

bool SQOptimizer::isLocalVarInstructions(int start, int count) const
{
    for (int i = 0, ie = fs->_localvarinfos.size(); i < ie; i++) {
        int pos = int(fs->_localvarinfos[i]._start_op);
        if (pos > start && pos < start + count)
            return true;
    }

    return false;
}

bool SQOptimizer::isUnsafeRange(int start, int count) const
{
    return isUnsafeJumpRange(start, count) || isLocalVarInstructions(start, count);
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
        if (varinfo._end_op >= tmpStart && varinfo._end_op != UINT32_MINUS_ONE) {
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
    for (int i = 0; i + 2 < instr.size(); i++) {
        do {
            changed = false;
            if (i + 3 < instr.size()) {
                const SQInstruction & operation = instr[i + 2];
                const SQInstruction & loadA = instr[i];
                const SQInstruction & loadB = instr[i + 1];
                int s = operation.op;

                if ((s == _OP_ADD || s == _OP_SUB || s == _OP_MUL || s == _OP_DIV || s == _OP_MOD || s == _OP_BITW) &&
                        (loadA.op == _OP_LOADINT || loadA.op == _OP_LOADFLOAT) &&
                        (loadB.op == _OP_LOADINT || loadB.op == _OP_LOADFLOAT) &&
                        ((loadB._arg0 == operation._arg2 && loadA._arg0 == operation._arg1) ||
                        (loadB._arg0 == operation._arg1 && loadA._arg0 == operation._arg2))
                        && !isUnsafeJumpRange(i, 3)) {

                    bool applyOpt = true;
                    const bool reversed = (loadB._arg0 == operation._arg2 && loadA._arg0 == operation._arg1);
                    bool removeLoadAVar = false, removeLoadBVar = false;
                    if (!isLocalVarInstructions(i, 3))
                        removeLoadAVar = removeLoadBVar = true;
                    else if (!isLocalVarInstructions(i, 2))
                        removeLoadAVar = true;
                    else if (!isLocalVarInstructions(i + 1, 2))
                        removeLoadBVar = true;
                    const int targetInst = removeLoadAVar ? i : removeLoadBVar ? i + 1 : i + 2;

                    if (loadA.op == _OP_LOADINT && loadB.op == _OP_LOADINT) {
                        SQInteger res = 0;
                        SQInt32 lv = loadA._arg1;
                        SQInt32 rv = loadB._arg1;
                        if (reversed)
                        {
                            SQInt32 t = lv; lv = rv; rv = t;
                        }
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

                        if (applyOpt) {
                            instr[targetInst]._arg0 = operation._arg0;
                            if (res < SQInteger(INT_MIN) || res > SQInteger(INT_MAX))
                            {
                                instr[targetInst].op = _OP_LOAD;
                                instr[targetInst]._arg1 = fs->GetNumericConstant(res);
                            } else {
                                instr[targetInst].op = _OP_LOADINT;
                                instr[targetInst]._arg1 = (SQInt32)res;
                            }
                            changed = true;
                            codeChanged = true;
                            #ifdef _DEBUG_DUMP
                                debugPrintInstructionPos(_SC("Const folding"), i);
                            #endif
                        }
                    } else { // float
                        assert(sizeof(SQFloat) == sizeof(SQInt32));
                        SQFloat res = 0;
                        SQFloat lv = (loadA.op == _OP_LOADFLOAT) ? loadA._farg1 : SQFloat(loadA._arg1);
                        SQFloat rv = (loadB.op == _OP_LOADFLOAT) ? loadB._farg1 : SQFloat(loadB._arg1);
                        if (reversed)
                        {
                            SQFloat t = lv; lv = rv; rv = t;
                        }
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
                            instr[targetInst].op = _OP_LOADFLOAT;
                            instr[targetInst]._farg1 = res;
                            instr[targetInst]._arg0 = operation._arg0;
                            changed = true;
                            codeChanged = true;
                            #ifdef _DEBUG_DUMP
                                debugPrintInstructionPos(_SC("Const folding"), i);
                            #endif
                        }
                    }
                    if (applyOpt && (removeLoadAVar || removeLoadBVar))
                    {
                        if (removeLoadAVar && removeLoadBVar)
                            cutRange(i, 3, 1);
                        else
                        {
                            if (removeLoadAVar)
                                cutRange(i, 1, 0);
                            cutRange(i + 1, 2, 1);
                        }
                    }
                }
            }
            if (i + 2 < instr.size()) {
                const SQInstruction operation = instr[i + 1];
                const SQInstruction loadA = instr[i];
                int s = operation.op;

                if (s == _OP_ADDI && (loadA.op == _OP_LOADINT || loadA.op == _OP_LOADFLOAT) && loadA._arg0 == operation._arg2 && !isUnsafeJumpRange(i, 2)){
                    bool applyOpt = true;
                    const bool removeLoadAVar = !isLocalVarInstructions(i, 2);
                    const int targetInst = removeLoadAVar ? i : i + 1;

                    if (loadA.op == _OP_LOADINT) {
                        SQInteger res = 0;
                        SQInt32 lv = loadA._arg1;
                        SQInt32 rv = operation._arg1;
                        switch (s) { // -V785
                            case _OP_ADDI: res = SQInteger(lv) + SQInteger(rv); break;
                            default: applyOpt = false; break;
                        }

                        if (applyOpt) { // -V547
                            instr[targetInst]._arg0 = operation._arg0;
                            if (res < SQInteger(INT_MIN) || res > SQInteger(INT_MAX))
                            {
                                instr[targetInst].op = _OP_LOAD;
                                instr[targetInst]._arg1 = fs->GetNumericConstant(res);
                            } else
                            {
                                instr[targetInst].op = _OP_LOADINT;
                                instr[targetInst]._arg1 = (SQInt32)res;
                            }
                            changed = true;
                            codeChanged = true;
                            #ifdef _DEBUG_DUMP
                                debugPrintInstructionPos(_SC("Const folding"), i);
                            #endif
                        }
                    } else { // float
                        assert(sizeof(SQFloat) == sizeof(SQInt32));
                        SQFloat res = 0;
                        SQFloat lv = loadA._farg1;
                        SQFloat rv = SQFloat(operation._arg1);
                        switch (s) { // -V785
                            case _OP_ADDI: res = lv + rv; break;
                            default: applyOpt = false; break;
                        }

                        if (applyOpt) { // -V547
                            instr[targetInst].op = _OP_LOADFLOAT;
                            instr[targetInst]._farg1 = res;
                            instr[targetInst]._arg0 = operation._arg0;
                            changed = true;
                            codeChanged = true;
                            #ifdef _DEBUG_DUMP
                                debugPrintInstructionPos(_SC("Const folding"), i);
                            #endif
                        }
                    }
                    if (applyOpt && removeLoadAVar) // -V547
                        cutRange(i, 2, 1);
                }
                if (s == _OP_JCMP && (loadA.op == _OP_LOADINT || loadA.op == _OP_LOADFLOAT || loadA.op == _OP_LOAD) &&
                    loadA._arg0 == operation._arg0 &&
                    ( loadA._arg1 <= 255 || (loadA.op != _OP_LOAD && operation._arg1 >= -128 && operation._arg1 <= 127) ) &&
                    !isUnsafeJumpRange(i, 2))
                {
                    const bool removeLoadAVar = !isLocalVarInstructions(i, 2);
                    const int targetInst = removeLoadAVar ? i : i + 1;
                    bool applyOpt = true;
                    if (loadA.op == _OP_LOAD)
                    {
                        instr[targetInst]._arg1 = operation._arg1;
                        instr[targetInst]._arg2 = operation._arg2;
                        instr[targetInst]._arg3 = operation._arg3;
                        instr[targetInst]._arg0 = loadA._arg1;
                        instr[targetInst].op = _OP_JCMPK;
                    } else if (operation._arg1 < -128 || operation._arg1 > 127) // we can't fit into JCMP(I|F) due to big jump distance
                    {
                        const uint32_t constI = fs->GetConstant(SQObjectPtr(loadA.op == _OP_LOADFLOAT ? loadA._farg1 : (SQInteger)loadA._arg1), 255);
                        if (constI <= 255u)
                        {
                            instr[targetInst]._arg1 = operation._arg1;
                            instr[targetInst]._arg2 = operation._arg2;
                            instr[targetInst]._arg3 = operation._arg3;
                            instr[targetInst]._arg0 = constI;
                            instr[targetInst].op = _OP_JCMPK;
                        } else
                            applyOpt = false;
                    } else {
                        instr[targetInst]._arg3 = operation._arg3; // cmp type
                        instr[targetInst]._arg1 = loadA._arg1; // compare value
                        instr[targetInst]._arg2 = operation._arg2;
                        instr[targetInst]._arg0 = operation._arg1;  // jump target
                        instr[targetInst].op = loadA.op == _OP_LOADINT ? _OP_JCMPI : _OP_JCMPF;
                    }
                    if (applyOpt)
                    {
                        if (removeLoadAVar)
                            cutRange(i, 2, 1);
                        const bool convertJumpTarget = instr[targetInst].op == _OP_JCMPI || instr[targetInst].op == _OP_JCMPF;
                        if (convertJumpTarget)
                            for (int ji = 0, jie = jumps.size(); ji < jie; ji++)
                            {
                                if (jumps[ji].instructionIndex == targetInst)
                                {
                                    assert(jumps[ji].jumpArg == JumpArg::JUMP_ARG1);
                                    jumps[ji].jumpArg = JumpArg::JUMP_ARG0;
                                    break;
                                }
                            }
                        changed = true;
                        codeChanged = true;
                        #ifdef _DEBUG_DUMP
                            debugPrintInstructionPos(_SC("Jump Const folding"), i);
                        #endif
                    }

                }
            }

        } while (changed && i + 2 < instr.size());
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
            if (op == _OP_JMP || op == _OP_JCMP || op == _OP_JCMPK || op == _OP_JZ || op == _OP_AND || op == _OP_OR || op == _OP_PUSHTRAP) {
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
            if (op == _OP_JCMPI || op == _OP_JCMPF) {
                int to = (i + instr[i]._sarg0() + 1);
                if (instr[to].op == _OP_JMP) {
                    const int nextJumpOfs = instr[i]._sarg0() + instr[to]._arg1 + 1;
                    if (nextJumpOfs >= -128 && nextJumpOfs <= 127)
                    {
                        changed = true;
                        codeChanged = true;
                        instr[i]._arg0 = nextJumpOfs;
                        #ifdef _DEBUG_DUMP
                            debugPrintInstructionPos(_SC("Jump folding"), i);
                        #endif
                    }
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
                case _OP_JCMPK:
                case _OP_JZ:
                case _OP_AND:
                case _OP_OR:
                case _OP_PUSHTRAP:
                case _OP_FOREACH:
                case _OP_PREFOREACH:
                case _OP_POSTFOREACH:
                    jumps.push_back({i, i, i + instr[i]._arg1 + 1, i + instr[i]._arg1 + 1, false, JumpArg::JUMP_ARG1});
                    break;
                case _OP_JCMPI:
                case _OP_JCMPF:
                    jumps.push_back({i, i, i + instr[i]._sarg0() + 1, i + instr[i]._sarg0() + 1, false, JumpArg::JUMP_ARG0});
                    break;
                case _OP_NULLCOALESCE:
                    jumps.push_back({i, i, i + instr[i]._arg1, i + instr[i]._arg1, false, JumpArg::JUMP_ARG1});
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
                    const int change = (jumps[i].jumpTo - jumps[i].originalJumpTo) - (jumps[i].instructionIndex - jumps[i].originalInstructionIndex);
                    if (jumps[i].jumpArg == JumpArg::JUMP_ARG1)
                        instr[jumps[i].instructionIndex]._arg1 += change;
                    else {
                        const SQInstruction originalJump = instr[jumps[i].instructionIndex];
                        const int nextJumpVal = originalJump._sarg0() + change;
                        if (nextJumpVal >= -128 && nextJumpVal <= 127)
                        {
                            instr[jumps[i].instructionIndex]._arg0 = nextJumpVal; // still fit
                        } else if (instr[jumps[i].instructionIndex].op == _OP_JCMPF || instr[jumps[i].instructionIndex].op == _OP_JCMPI) {
                            const uint32_t constI = fs->GetConstant(SQObjectPtr(instr[jumps[i].instructionIndex].op == _OP_JCMPF ? originalJump._farg1 : (SQInteger)originalJump._arg1), 255);
                            if (constI <= 255u) // we still fit in const table
                            {
                              jumps[i].jumpArg = JumpArg::JUMP_ARG1;
                              instr[jumps[i].instructionIndex]._arg1 = nextJumpVal;
                              instr[jumps[i].instructionIndex]._arg0 = constI;
                              instr[jumps[i].instructionIndex].op = _OP_JCMPK;
                            } else
                              assert(0);//todo: we need to convert back to OP_JCMP, and we need to generate load instruction for that
                        } else
                          assert(0);//todo: we need to convert back to OP_JCMP, and we need to generate load instruction for that
                    }
                }
    }

    optimizeJumpFolding();

#ifdef _DEBUG_DUMP
    for (int i = 0; i < jumps.size(); i++)
        printf(_SC("JUMPS:  instruction: %d  to: %d\n"), jumps[i].instructionIndex, jumps[i].jumpTo);
#endif

}


