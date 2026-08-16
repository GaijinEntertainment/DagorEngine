#pragma once

#include "daScript/ast/ast.h"

namespace das {

    struct ExprFor;

    // ===== Control-flow graph over a function body =====
    // A statement-level CFG: each block holds a straight-line run of expressions; control transfers
    // (if / while / for / return / break / continue) become edges. Built for one Function.
    //
    // This is plain analysis infrastructure: it does not mutate the AST.

    struct CfgBlock {
        uint32_t                id = 0;
        vector<Expression *>    stmts;     // straight-line expressions evaluated in this block, in order
        vector<CfgBlock *>      succ;      // successor blocks (control may flow to)
        vector<CfgBlock *>      pred;      // predecessor blocks
        bool                    isExit = false;   // the unique synthetic exit block
        // ===== AST anchors for inserting "at this block's entry/exit" =====
        // A CFG block maps back to the AST in one of two ways:
        //  * astHead != null: the block opens a lexical scope (branch / loop / function body) - it begins
        //    exactly at that ExprBlock's first statement. A scope-exit insertion goes into astHead->finalList.
        //  * contOwner != null: the block is a CONTINUATION (a merge after an if, the code after a loop) -
        //    its statements live in contOwner->list, starting at contBefore. An entry insertion goes into
        //    contOwner->list, right before contBefore (a regular statement, reached only on the
        //    fall-through path - unlike finalList it does NOT cover an escaping early-return).
        // Synthetic blocks reached without a continuation statement (e.g. an `if` that is the last stmt,
        // falling to the implicit return) have neither and offer no insertion point.
        ExprBlock *             astHead = nullptr;
        ExprBlock *             contOwner = nullptr;   // owner AST block for a continuation
        Expression *            contBefore = nullptr;  // first continuation statement (insert before it)
        // ===== loop induction anchor =====
        // set on a for-loop BODY block: the ExprFor whose iteration variables are live in this block.
        // lets a range analysis gen the induction facts (i in [0,bound)) at the loop body's entry -
        // information the flattened cond-less loop header would otherwise lose.
        ExprFor *               loopSource = nullptr;
    };

    struct Cfg {
        Cfg () = default;
        Cfg ( Cfg && ) = default;
        Cfg & operator = ( Cfg && ) = default;
        Cfg ( const Cfg & ) = delete;             // blocks are owned by raw pointers; no copy
        Cfg & operator = ( const Cfg & ) = delete;
        ~Cfg ();

        Function *              func = nullptr;
        CfgBlock *              entry = nullptr;
        CfgBlock *              exit = nullptr;    // unique exit (return / fallthrough converge here)
        vector<CfgBlock *>      blocks;            // owns every block; freed in ~Cfg

        CfgBlock * newBlock ();
        void       addEdge ( CfgBlock * from, CfgBlock * to );
        string     dump () const;                 // human-readable block + edge listing, for debugging
    };

    // Build the CFG for a function's body. Returns an empty CFG (entry==exit, no stmts) for a function
    // with no body (builtin / stub / abstract).
    Cfg buildCfg ( Function * fn );

    // ===== Shared per-function CFG cache =====
    // Built once, as a distinct pass, when any CFG consumer is enabled (escape analysis /
    // bound-check elimination). Each consumer then reads its function's CFG through a const
    // pointer; nobody rebuilds. A function with no CFG entry gets a null from forFunction.
    struct ProgramCfg {
        das_hash_map<Function *, Cfg>   perFunction;
        const Cfg * forFunction ( Function * fn ) const {
            auto it = perFunction.find(fn);
            return it!=perFunction.end() ? &it->second : nullptr;
        }
    };

    // Build and return CFGs for every block-bodied function in the program's module.
    ProgramCfg buildProgramCfg ( Program * program );

}
