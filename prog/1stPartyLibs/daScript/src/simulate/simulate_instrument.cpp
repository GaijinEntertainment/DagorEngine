#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/simulate_nodes.h"

namespace das {

#if DAS_DEBUGGER

    struct SimInstVisitor : SimVisitor {
        bool isCorrectFileAndLine ( const LineInfo & info ) {
            if ( cmpBlk ) {
                vec4f args[1];
                args[0] = cast<LineInfo&>::from(info);
                auto res = blkContext->invoke(*cmpBlk, args, nullptr, blkLine);    // note: no line-info
                return cast<bool>::to(res);
            } else {
                return true;
            }
        }
        SimNode * instrumentNode ( SimNode * expr ) {
            if ( !expr->rtti_node_isInstrument() ) {
                return context->code->makeNode<SimNodeDebug_Instrument>(expr->debugInfo, expr);
            } else {
                return expr;
            }
        }
        SimNode * clearNode ( SimNode * expr ) {
            if ( expr->rtti_node_isInstrument() ) {
                auto si = (SimNodeDebug_Instrument *) expr;
                return si->subexpr;
            } else {
                return expr;
            }
        }
        virtual SimNode * visit ( SimNode * node ) override {
            if ( node->rtti_node_isBlock() ) {
                SimNode_Block * blk = (SimNode_Block *) node;
                for ( uint32_t i=0, is=blk->total; i!=is; ++i ) {
                    auto & expr = blk->list[i];
                    if ( anyLine || isCorrectFileAndLine(expr->debugInfo) ) {
                        expr = isInstrumenting ? instrumentNode(expr) : clearNode(expr);
                    }
                }
                for ( uint32_t i=0, is=blk->totalFinal; i!=is; ++i ) {
                    auto & expr = blk->finalList[i];
                    if ( anyLine || isCorrectFileAndLine(expr->debugInfo) ) {
                        expr = isInstrumenting ? instrumentNode(expr) : clearNode(expr);
                    }
                }
            }
            else if ( node->rtti_node_isIf() ) {
                SimNode_IfTheElseAny * cond = (SimNode_IfTheElseAny *) node;
                if ( cond->if_true && !cond->if_true->rtti_node_isBlock() ) {
                    auto & expr = cond->if_true;
                    if ( anyLine || isCorrectFileAndLine(expr->debugInfo) ) {
                        expr = isInstrumenting ? instrumentNode(expr) : clearNode(expr);
                    }
                }
                if ( cond->if_false && !cond->if_false->rtti_node_isBlock() ) {
                    auto & expr = cond->if_false;
                    if ( anyLine || isCorrectFileAndLine(expr->debugInfo) ) {
                        expr = isInstrumenting ? instrumentNode(expr) : clearNode(expr);
                    }
                }
            }
            return node;
        }
        Context * context = nullptr;
        const Block * cmpBlk = nullptr;
        Context * blkContext = nullptr;
        LineInfo * blkLine = nullptr;
        bool isInstrumenting = true;
        bool anyLine = false;
    };

    void Context::instrumentContextNode ( const Block & blk, bool isInstrumenting, Context * context, LineInfo * line ) {
        SimInstVisitor instrument;
        instrument.context = this;
        instrument.cmpBlk = &blk;
        instrument.blkContext = context;
        instrument.blkLine = line;
        instrument.isInstrumenting = isInstrumenting;
        runVisitor(&instrument);
    }

    void Context::clearInstruments() {
        SimInstVisitor instrument;
        instrument.context = this;
        instrument.isInstrumenting = false;
        instrument.anyLine = true;
        runVisitor(&instrument);
        instrumentFunction(0u, false, 0ul, false);
    }

    void Context::instrumentFunction ( SimFunction * FNPTR, bool isInstrumenting, uint64_t userData, bool threadLocal ) {
        auto instFn = [&](SimFunction * fun, uint64_t fnMnh) {
            if ( !fun->code ) return;
            if ( isInstrumenting ) {
                if ( !fun->code->rtti_node_isInstrumentFunction() ) {
                    if ( threadLocal ) {
                        fun->code = code->makeNode<SimNodeDebug_InstrumentFunctionThreadLocal>(fun->code->debugInfo, fun, fnMnh, fun->code, userData);
                    } else {
                        fun->code = code->makeNode<SimNodeDebug_InstrumentFunction>(fun->code->debugInfo, fun, fnMnh, fun->code, userData);
                    }
                }
            } else {
                if ( fun->code->rtti_node_isInstrumentFunction() ) {
                    auto inode = (SimNodeDebug_InstrumentFunction *) fun->code;
                    fun->code = inode->subexpr;
                }
            }
        };
        if ( FNPTR==nullptr ) {
            for ( int fni=0, fnis=totalFunctions; fni!=fnis; ++fni ) {
                instFn(&functions[fni], functions[fni].mangledNameHash);
            }
        } else {
            instFn(FNPTR, FNPTR->mangledNameHash);
        }
    }
#else
    void Context::instrumentFunction ( SimFunction *, bool, uint64_t, bool ) {}
    void Context::instrumentContextNode ( const Block &, bool, Context *, LineInfo * ) {}
    void Context::clearInstruments() {}
#endif

}
