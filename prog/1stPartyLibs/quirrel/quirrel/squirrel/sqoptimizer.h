/*  see copyright notice in squirrel.h */
#ifndef _SQOPTIMIZER_H_
#define _SQOPTIMIZER_H_
///////////////////////////////////
#include "sqfuncstate.h"


struct SQOptimizer
{
    SQOptimizer(SQFuncState & func_state);
    void optimize();

private:
    bool isUnsafeRange(int start, int count) const;
    bool isUnsafeJumpRange(int start, int count) const;
    bool isLocalVarInstructions(int start, int count) const;
    void cutRange(int start, int old_count, int new_count);

    void optimizeConstFolding();
    void optimizeJumpFolding();
    void optimizeEmptyJumps();
    enum class JumpArg : uint8_t {JUMP_ARG1, JUMP_ARG0};
    struct Jump {
        int originalInstructionIndex;
        int instructionIndex;
        int originalJumpTo;
        int jumpTo;
        bool modified;
        JumpArg jumpArg;
    };
    sqvector<Jump> jumps;
    SQFuncState * fs;

    bool codeChanged;

#ifdef _DEBUG_DUMP
    void debugPrintInstructionPos(const SQChar * message, int instructionIndex);
#endif
};


#endif // _SQOPTIMIZER_H_

