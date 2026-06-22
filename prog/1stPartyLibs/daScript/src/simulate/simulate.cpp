#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/simulate_nodes.h"
#include "daScript/simulate/runtime_string.h"
#include "daScript/simulate/debug_print.h"
#include "daScript/misc/fpe.h"
#include "daScript/misc/string_writer.h"
#include "daScript/misc/debug_break.h"
#include "daScript/ast/ast.h"
#include "misc/include_fmt.h"

#include <atomic>

namespace das
{

    GcRootLambda::GcRootLambda( const Lambda & that, Context * _context ) : Lambda(that.capture) {
        context = _context;
        context->addGcRoot( (void *)capture, nullptr );
    }

    GcRootLambda::~GcRootLambda() {
        if ( capture && context && (context->category.value & uint32_t(das::ContextCategory::dead)) == 0u ) {
            context->removeGcRoot( (void *)capture );
        }
    }

    // this is here to occasionally investigate untyped evaluation paths
    #define WARN_SLOW_CAST(TYPE)
    // #define WARN_SLOW_CAST(TYPE)    DAS_ASSERTF(0, "internal perofrmance issue, casting eval to eval##TYPE" );

    SimNode * SimNode::copyNode ( Context &, NodeAllocator * code ) {
        auto prefix = ((NodePrefix *)this) - 1;
#ifndef DAS_NO_ASSERTIONS
        DAS_ASSERTF(prefix->magic==0xdeadc0de,"node was allocated on the heap without prefix");
#endif
        char * newNode;
        if ( code->prefixWithHeader ) {
            newNode = code->allocate(prefix->size + sizeof(NodePrefix));
            memcpy ( newNode, ((char *)this) - sizeof(NodePrefix), sizeof(NodePrefix));
            newNode += sizeof(NodePrefix);
        } else {
            newNode = code->allocate(prefix->size);
        }
        memcpy ( newNode, (char *)this, prefix->size );
        return (SimNode *) newNode;
    }

    bool SimNode::evalBool ( Context & context ) {
        WARN_SLOW_CAST(Bool);
        return cast<bool>::to(eval(context));
    }

    float SimNode::evalFloat ( Context & context ) {
        WARN_SLOW_CAST(Float);
        return cast<float>::to(eval(context));
    }

    double SimNode::evalDouble(Context & context) {
        WARN_SLOW_CAST(Double);
        return cast<double>::to(eval(context));
    }

    int32_t SimNode::evalInt ( Context & context ) {
        WARN_SLOW_CAST(Int);
        return cast<int32_t>::to(eval(context));
    }

    uint32_t SimNode::evalUInt ( Context & context ) {
        WARN_SLOW_CAST(UInt);
        return cast<uint32_t>::to(eval(context));
    }

    int64_t SimNode::evalInt64 ( Context & context ) {
        WARN_SLOW_CAST(Int64);
        return cast<int64_t>::to(eval(context));
    }

    uint64_t SimNode::evalUInt64 ( Context & context ) {
        WARN_SLOW_CAST(UInt64);
        return cast<uint64_t>::to(eval(context));
    }

    char * SimNode::evalPtr ( Context & context ) {
        WARN_SLOW_CAST(Ptr);
        return cast<char *>::to(eval(context));
    }

    SimNode * SimNode_WithErrorMessage::copyNode ( Context & context, NodeAllocator * code ) {
        SimNode_WithErrorMessage * that = (SimNode_WithErrorMessage *) SimNode::copyNode(context, code);
        if ( errorMessage ) {
            that->errorMessage = errorMessage[0]==0 ? "" : code->allocateName(errorMessage);
        }
        return that;
    }

    vec4f SimNode_NOP::eval ( Context & ) {
        return v_zero();
    }

    vec4f SimNode_DeleteStructPtr::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pStruct = (char **) subexpr->evalPtr(context);
        pStruct = pStruct + total - 1;
        for ( uint32_t i=0, is=total; i!=is; ++i, pStruct-- ) {
            if ( *pStruct ) {
                if ( persistent ) {
                    das_aligned_free16(*pStruct);
                } else if ( isLambda ) {
                    context.free(*pStruct - 16, structSize + 16, &debugInfo);
                } else {
                    context.free(*pStruct, structSize, &debugInfo);
                }
                *pStruct = nullptr;
            }
        }
        return v_zero();
    }

    vec4f SimNode_DeleteClassPtr::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pStruct = (char **) subexpr->evalPtr(context);
        pStruct = pStruct + total - 1;
        auto sizeOf = sizeexpr->evalInt(context);
        for ( uint32_t i=0, is=total; i!=is; ++i, pStruct-- ) {
            if ( *pStruct ) {
                if (persistent) {
                    das_aligned_free16(*pStruct);
                } else {
                    context.free(*pStruct, sizeOf, &debugInfo);
                }
                *pStruct = nullptr;
            }
        }
        return v_zero();
    }

    vec4f SimNode_DeleteLambda::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pLambda = (Lambda *) subexpr->evalPtr(context);
        pLambda = pLambda + total - 1;
        for ( uint32_t i=0, is=total; i!=is; ++i, pLambda-- ) {
            if ( pLambda->capture ) {
                SimFunction ** fnMnh = (SimFunction **) pLambda->capture;
                SimFunction * simFunc = fnMnh[1];
                if (!simFunc) context.throw_error_at(debugInfo, "lambda finalizer is a null function%s", errorMessage);
                vec4f argValues[1] = {
                    cast<void *>::from(pLambda->capture)
                };
                context.call(simFunc, argValues, 0);
                pLambda->capture = nullptr;
            }
        }
        return v_zero();
    }

    vec4f SimNode_Swizzle::eval ( Context & context ) {
        DAS_PROFILE_NODE
        union {
            vec4f   res;
            int32_t val[4];
        } R, S;
        S.res = value->eval(context);
        R.val[0] = S.val[fields[0]];
        R.val[1] = S.val[fields[1]];
        R.val[2] = S.val[fields[2]];
        R.val[3] = S.val[fields[3]];
        return R.res;
    }

    vec4f SimNode_Swizzle64::eval ( Context & context ) {
        DAS_PROFILE_NODE
        union {
            vec4f   res;
            int64_t val[2];
        } R, S;
        S.res = value->eval(context);
        R.val[0] = S.val[fields[0]];
        R.val[1] = S.val[fields[1]];
        return R.res;
    }

    // SimNode_MakeBlock

    vec4f SimNode_MakeBlock::eval ( Context & context )  {
        DAS_PROFILE_NODE
        Block * block = (Block *) ( context.stack.sp() + stackTop );
        block->stackOffset = context.stack.spi();
        block->argumentsOffset = argStackTop ? (context.stack.spi() + argStackTop) : 0;
        block->body = subexpr;
        block->aotFunction = nullptr;
        block->jitFunction = nullptr;
        block->functionArguments = context.abiArguments();
        block->info = info;
        return cast<Block *>::from(block);
    }

    // SimNode_Debug

    void das_debug ( Context * context, TypeInfo * typeInfo, const char * FILE, int LINE, vec4f res, const char * message ) {
        TextWriter ssw;
        if ( message ) ssw << message << " ";
        ssw << debug_type(typeInfo) << " = " << debug_value(res, typeInfo, PrintFlags::debugger)
        << " at " << FILE << ":" << LINE << "\n";
        context->to_out(nullptr, ssw.str().c_str());
    }

    vec4f SimNode_Debug::eval ( Context & context ) {
        DAS_PROFILE_NODE
        FPE_DISABLE;
        vec4f res = subexpr->eval(context);
        TextWriter ssw;
        if ( message ) ssw << message << " ";
        ssw << debug_type(typeInfo) << " = " << debug_value(res, typeInfo, PrintFlags::debugger)
            << " at " << debugInfo.describe() << "\n";
        context.to_out(&debugInfo, ssw.str().c_str());
        return res;
    }

    // SimNode_Assert

    vec4f SimNode_Assert::eval ( Context & context ) {
        DAS_PROFILE_NODE
        if ( !subexpr->evalBool(context) ) {
            {
                string error_message = "assert failed";
                if ( message )
                    error_message = error_message + ", " + message;
                string error = reportError(debugInfo, error_message, "", "");
#ifdef DAS_NO_ASSERTIONS
                error = context.getStackWalk(&debugInfo, false, false) + error;
#else
                error = context.getStackWalk(&debugInfo, true, true) + error;
#endif
                context.to_err(&debugInfo, error.c_str());
            }
            context.throw_error_at(debugInfo,"assert failed");
        }
        return v_zero();
    }

    // SimNode_CopyRefValue

    vec4f SimNode_CopyRefValue::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pr = r->evalPtr(context);  // right, then left
        auto pl = l->evalPtr(context);
        memcpy ( pl, pr, size );
        return v_zero();
    }

    // SimNode_MoveRefValue

    vec4f SimNode_MoveRefValue::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pr = r->evalPtr(context);  // right, then left
        auto pl = l->evalPtr(context);
        if ( pl != pr ) {
            memcpy ( pl, pr, size );
            memset ( pr, 0, size );
        }
        return v_zero();
    }

    // SimNode_ForBase

    void SimNode_ForBase::allocateFor ( NodeAllocator * code, uint32_t t ) {
        totalSources = t;
        auto bytes = code->allocate( totalSources * ( sizeof(SimNode*) + sizeof(uint32_t)*2 ) );
        sources = (SimNode **) (bytes);
        strides = (uint32_t *) (bytes + totalSources * sizeof(SimNode *));
        stackTop = (uint32_t *) (bytes + totalSources * sizeof(SimNode *) + totalSources * sizeof(uint32_t));
    }

    SimNode * SimNode_ForBase::copyNode ( Context & context, NodeAllocator * code ) {
        SimNode_ForBase * that = (SimNode_ForBase *) SimNode_Block::copyNode(context, code);
        if ( totalSources ) {
            auto bytes = code->allocate( totalSources * ( sizeof(SimNode*) + sizeof(uint32_t)*2 ) );
            auto newSources = (SimNode **) (bytes);
            memcpy ( newSources, that->sources, totalSources*sizeof(SimNode *));
            that->sources = newSources;
            auto newStrides = (uint32_t *) (bytes + totalSources * sizeof(SimNode *));
            memcpy( newStrides, that->strides, totalSources*sizeof(uint32_t));
            that->strides = newStrides;
            auto newStackTop = (uint32_t *) (bytes + totalSources * sizeof(SimNode *) + totalSources * sizeof(uint32_t));
            memcpy ( newStackTop, that->stackTop, totalSources * sizeof(uint32_t));
            that->stackTop = newStackTop;
        } else {
            sources = nullptr;
            strides = nullptr;
            stackTop = nullptr;
        }
        return that;
    }

    // SimNode_ForWithIterator

    void SimNode_ForWithIteratorBase::allocateFor ( NodeAllocator * code, uint32_t t ) {
        totalSources = t;
        auto bytes = code->allocate( totalSources * ( sizeof(SimNode*) + sizeof(uint32_t) ) );
        source_iterators = (SimNode **) (bytes);
        stackTop = (uint32_t *) (bytes + totalSources * sizeof(SimNode *));
    }

    SimNode * SimNode_ForWithIteratorBase::copyNode ( Context & context, NodeAllocator * code ) {
        SimNode_ForWithIteratorBase * that = (SimNode_ForWithIteratorBase *) SimNode_Block::copyNode(context, code);
        if ( totalSources ) {
            auto bytes = code->allocate( totalSources * ( sizeof(SimNode*) + sizeof(uint32_t) ) );
            auto new_source_iterators = (SimNode **) (bytes);
            memcpy ( new_source_iterators, that->source_iterators, totalSources*sizeof(SimNode *));
            that->source_iterators = new_source_iterators;
            auto newStackTop = (uint32_t *) (bytes + totalSources * sizeof(SimNode *));
            memcpy ( newStackTop, that->stackTop, totalSources * sizeof(uint32_t));
            that->stackTop = newStackTop;
        } else {
            source_iterators = nullptr;
            stackTop = nullptr;
        }
        return that;
    }

    void SimNode_ForWithIteratorBase::closeIterators ( Iterator ** sources, char ** pi, Context & context ) {
        for ( int t=int(totalSources)-1; t>=0; --t ) {
            sources[t]->close(context, pi[t]);
        }

    }

    vec4f SimNode_ForWithIteratorBase::eval ( Context & context ) {
        // note: this is the 'slow' version, to which we fall back when there are too many sources
        DAS_PROFILE_NODE
        int totalCount = int(totalSources);
        vector<char *> pi(totalCount);
        for ( int t=0; t!=totalCount; ++t ) {
            pi[t] = context.stack.sp() + stackTop[t];
        }
        vector<Iterator *> sources(totalCount);
        for ( int t=0; t!=totalCount; ++t ) {
            vec4f ll = source_iterators[t]->eval(context);
            sources[t] = cast<Iterator *>::to(ll);
        }
        bool needLoop = true;
        SimNode ** __restrict tail = list + total;
        for ( int t=0; t!=totalCount; ++t ) {
            sources[t]->isOpen = true;
            needLoop = sources[t]->first(context, pi[t]) && needLoop;
            if ( context.stopFlags ) goto loopend;
        }
        if ( !needLoop ) goto loopend;
        while ( !context.stopFlags ) {
            SimNode ** __restrict body = list;
        loopbegin:;
            for (; body!=tail; ++body) {
                (*body)->eval(context);
                DAS_PROCESS_LOOP_FLAGS(break);
            }
            for ( int t=0; t!=totalCount; ++t ){
                if ( !sources[t]->next(context, pi[t]) ) goto loopend;
                if ( context.stopFlags ) goto loopend;
            }
        }
    loopend:
        closeIterators(sources.data(), pi.data(), context);
        evalFinal(context);
        context.stopFlags &= ~EvalFlags::stopForBreak;
        return v_zero();
    }

#if DAS_ENABLE_KEEPALIVE

    vec4f SimNodeKeepAlive_ForWithIteratorBase::eval ( Context & context ) {
        // note: this is the 'slow' version, to which we fall back when there are too many sources
        DAS_PROFILE_NODE
        int totalCount = int(totalSources);
        vector<char *> pi(totalCount);
        for ( int t=0; t!=totalCount; ++t ) {
            pi[t] = context.stack.sp() + stackTop[t];
        }
        vector<Iterator *> sources(totalCount);
        for ( int t=0; t!=totalCount; ++t ) {
            vec4f ll = source_iterators[t]->eval(context);
            sources[t] = cast<Iterator *>::to(ll);
        }
        bool needLoop = true;
        SimNode ** __restrict tail = list + total;
        for ( int t=0; t!=totalCount; ++t ) {
            sources[t]->isOpen = true;
            needLoop = sources[t]->first(context, pi[t]) && needLoop;
            if ( context.stopFlags ) goto loopend;
        }
        if ( !needLoop ) goto loopend;
        while ( !context.stopFlags ) {
            SimNode ** __restrict body = list;
        loopbegin:;
            DAS_KEEPALIVE_LOOP(&context);
            for (; body!=tail; ++body) {
                (*body)->eval(context);
                DAS_PROCESS_LOOP_FLAGS(break);
            }
            for ( int t=0; t!=totalCount; ++t ){
                if ( !sources[t]->next(context, pi[t]) ) goto loopend;
                if ( context.stopFlags ) goto loopend;
            }
        }
    loopend:
        closeIterators(sources.data(), pi.data(), context);
        evalFinal(context);
        context.stopFlags &= ~EvalFlags::stopForBreak;
        return v_zero();
    }


#endif

#if DAS_DEBUGGER

    vec4f SimNodeDebug_ForWithIteratorBase::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto totalCount = int(totalSources);
        vector<char *> pi(totalCount);
        for ( int t=0; t!=totalCount; ++t ) {
            pi[t] = context.stack.sp() + this->stackTop[t];
        }
        vector<Iterator *> sources(totalCount);
        for ( int t=0; t!=totalCount; ++t ) {
            vec4f ll = this->source_iterators[t]->eval(context);
            sources[t] = cast<Iterator *>::to(ll);
        }
        bool needLoop = true;
        SimNode ** __restrict tail = this-> list + this->total;
        for ( int t=0; t!=totalCount; ++t ) {
            sources[t]->isOpen = true;
            needLoop = sources[t]->first(context, pi[t]) && needLoop;
            if ( context.stopFlags ) goto loopend;
        }
        if ( !needLoop ) goto loopend;
        while ( !context.stopFlags ) {
            SimNode ** __restrict body = this->list;
        loopbegin:;
            for (; body!=tail; ++body) {
                DAS_SINGLE_STEP(context,(*body)->debugInfo,true);
                (*body)->eval(context);
                DAS_PROCESS_LOOP_FLAGS(break);
            }
            for ( int t=0; t!=totalCount; ++t ){
                if ( !sources[t]->next(context, pi[t]) ) goto loopend;
                if ( context.stopFlags ) goto loopend;
            }
        }
    loopend:
        closeIterators(sources.data(), pi.data(), context);
        this->evalFinal(context);
        context.stopFlags &= ~EvalFlags::stopForBreak;
        return v_zero();
    }

#endif

    // SimNode_CallBase

    SimNode * SimNode_CallBase::copyNode ( Context & context, NodeAllocator * code ) {
        SimNode_CallBase * that = (SimNode_CallBase *) SimNode_WithErrorMessage::copyNode(context, code);
        if ( nArguments ) {
            SimNode ** newArguments = (SimNode **) code->allocate(nArguments * sizeof(SimNode *));
            memcpy ( newArguments, that->arguments, nArguments * sizeof(SimNode *));
            that->arguments = newArguments;
            if ( that->types ) {
                TypeInfo ** newTypes = (TypeInfo **) code->allocate(nArguments * sizeof(TypeInfo **));
                memcpy ( newTypes, that->types, nArguments * sizeof(TypeInfo **));
                that->types = newTypes;
            }
        }
        if ( fnPtr ) {
            that->fnPtr = context.fnByMangledName(fnPtr->mangledNameHash);
            // printf("CALL %p -> %p\n", fnPtr, fnPtr );
        }
        return that;
    }

    // SimNode_Final

    SimNode * SimNode_Final::copyNode ( Context & context, NodeAllocator * code ) {
        SimNode_Final * that = (SimNode_Final *) SimNode::copyNode(context, code);
        if ( totalFinal ) {
            SimNode ** newList = (SimNode **) code->allocate(totalFinal * sizeof(SimNode *));
            memcpy ( newList, that->finalList, totalFinal*sizeof(SimNode *));
            that->finalList = newList;
        }
        return that;
    }

    // SimNode_Block

    SimNode * SimNode_Block::copyNode ( Context & context, NodeAllocator * code ) {
        SimNode_Block * that = (SimNode_Block *) SimNode_Final::copyNode(context, code);
        if ( total ) {
            SimNode ** newList = (SimNode **) code->allocate(total * sizeof(SimNode *));
            memcpy ( newList, that->list, total*sizeof(SimNode *));
            that->list = newList;
        }
        if ( totalLabels ) {
            uint32_t * newLabels = (uint32_t *) code->allocate(totalLabels * sizeof(uint32_t));
            memcpy ( newLabels, that->labels, totalLabels*sizeof(uint32_t));
            that->labels = newLabels;
        }
        return that;
    }

    vec4f SimNode_Block::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        for (SimNode ** __restrict body = list; body!=tail; ++body) {
            (*body)->eval(context);
            if ( context.stopFlags ) break;
        }
        evalFinal(context);
        return v_zero();
    }

#if DAS_DEBUGGER

    vec4f SimNodeDebug_Block::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        for (SimNode ** __restrict body = list; body!=tail; ++body) {
            DAS_SINGLE_STEP(context,(*body)->debugInfo,false);
            (*body)->eval(context);
            if ( context.stopFlags ) break;
        }
        evalFinalSingleStep(context);
        return v_zero();
    }

#endif

    vec4f SimNode_BlockNF::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        for (SimNode ** __restrict body = list; body!=tail; ++body) {
            (*body)->eval(context);
            if ( context.stopFlags ) break;
        }
        return v_zero();
    }

#if DAS_DEBUGGER

    vec4f SimNodeDebug_BlockNF::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        for (SimNode ** __restrict body = list; body!=tail; ++body) {
            DAS_SINGLE_STEP(context,(*body)->debugInfo,false);
            (*body)->eval(context);
            if ( context.stopFlags ) break;
        }
        return v_zero();
    }

#endif

    vec4f SimNode_ClosureBlock::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        for (SimNode ** __restrict body = list; body!=tail; ++body) {
            (*body)->eval(context);
            if ( context.stopFlags ) break;
        }
        evalFinal(context);
        if ( context.stopFlags & EvalFlags::stopForReturn ) {
            context.stopFlags &= ~EvalFlags::stopForReturn;
            return context.abiResult();
        } else {
            if ( needResult ) context.throw_error_at(debugInfo,"end of block without return");
            return v_zero();
        }
    }

#if DAS_DEBUGGER

    vec4f SimNodeDebug_ClosureBlock::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        for (SimNode ** __restrict body = list; body!=tail; ++body) {
            DAS_SINGLE_STEP(context,(*body)->debugInfo,false);
            (*body)->eval(context);
            if ( context.stopFlags ) break;
        }
        evalFinalSingleStep(context);
        if ( context.stopFlags & EvalFlags::stopForReturn ) {
            context.stopFlags &= ~EvalFlags::stopForReturn;
            return context.abiResult();
        } else {
            if ( needResult ) context.throw_error_at(debugInfo,"end of block without return");
            return v_zero();
        }
    }

#endif

// SimNode_BlockWithLabels

    vec4f SimNode_BlockWithLabels::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        SimNode ** __restrict body = list;
    loopbegin:;
        for (; body!=tail; ++body) {
            (*body)->eval(context);
            { if ( context.stopFlags ) {
                if (context.stopFlags&EvalFlags::jumpToLabel) {
                    if (  context.gotoLabel>=totalLabels ) {
                        context.throw_error_at(debugInfo, "invalid label index %u", context.gotoLabel);
                    }
                    body=list+labels[context.gotoLabel];
                    if ( body>=list && body<tail ) {
                        context.stopFlags &= ~EvalFlags::jumpToLabel;
                        goto loopbegin;
                    } else {
                        context.throw_error_at(debugInfo, "jump to label %u failed", context.gotoLabel);
                    }
                }
                goto loopend;
            } }
        }
    loopend:;
        evalFinal(context);
        return v_zero();
    }

#if DAS_DEBUGGER

    vec4f SimNodeDebug_BlockWithLabels::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        SimNode ** __restrict body = list;
    loopbegin:;
        for (; body!=tail; ++body) {
            DAS_SINGLE_STEP(context,(*body)->debugInfo,false);
            (*body)->eval(context);
            { if ( context.stopFlags ) {
                if (context.stopFlags&EvalFlags::jumpToLabel) {
                    if (  context.gotoLabel>=totalLabels ) {
                        context.throw_error_at(debugInfo, "invalid label index %u", context.gotoLabel);
                    }
                    body=list+labels[context.gotoLabel];
                    if ( body>=list && body<tail ) {
                        context.stopFlags &= ~EvalFlags::jumpToLabel;
                        goto loopbegin;
                    } else {
                        context.throw_error_at(debugInfo, "jump to label %u failed", context.gotoLabel);
                    }
                }
                goto loopend;
            } }
        }
    loopend:;
        evalFinalSingleStep(context);
        return v_zero();
    }

#endif

    // SimNode_Let

    vec4f SimNode_Let::eval ( Context & context ) {
        DAS_PROFILE_NODE
        for ( uint32_t i=0, is=total; i!=is && !context.stopFlags; ) {
            list[i++]->eval(context);
        }
        return v_zero();
    }

    // SimNode_While

    vec4f SimNode_While::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        if ( this->totalFinal == 0 ) {
            while ( cond->evalBool(context) && !context.stopFlags ) {
                SimNode ** __restrict body = list;
            loopbegin:;
                for (; body!=tail; ++body) {
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS(break);
                }
            }
        } else {
            while ( cond->evalBool(context) && !context.stopFlags ) {
                SimNode ** __restrict body = list;
            loopbegin_fin:;
                for (; body!=tail; ++body) {
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS_LABELED(loopbegin_fin,loopend_fin,break);
                }
            loopend_fin:;
                evalFinal(context);
            }
        }
    loopend:;
        context.stopFlags &= ~EvalFlags::stopForBreak;
        return v_zero();
    }

#if DAS_DEBUGGER

    vec4f SimNodeDebug_While::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        if ( this->totalFinal == 0 ) {
            while ( cond->evalBool(context) && !context.stopFlags ) {
                SimNode ** __restrict body = list;
            loopbegin:;
                for (; body!=tail; ++body) {
                    DAS_SINGLE_STEP(context,(*body)->debugInfo,true);
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS(break);
                }
            }
        } else {
            while ( cond->evalBool(context) && !context.stopFlags ) {
                SimNode ** __restrict body = list;
            loopbegin_fin:;
                for (; body!=tail; ++body) {
                    DAS_SINGLE_STEP(context,(*body)->debugInfo,true);
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS_LABELED(loopbegin_fin,loopend_fin,break);
                }
            loopend_fin:;
                evalFinalSingleStep(context);
            }
        }
    loopend:;
        context.stopFlags &= ~EvalFlags::stopForBreak;
        return v_zero();
    }

#endif

#if DAS_ENABLE_KEEPALIVE
    vec4f SimNodeKeepAlive_While::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        if ( this->totalFinal == 0 ) {
            while ( cond->evalBool(context) && !context.stopFlags ) {
                SimNode ** __restrict body = list;
            loopbegin:;
                DAS_KEEPALIVE_LOOP(&context);
                for (; body!=tail; ++body) {
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS(break);
                }
            }
        } else {
            while ( cond->evalBool(context) && !context.stopFlags ) {
                SimNode ** __restrict body = list;
            loopbegin_fin:;
                DAS_KEEPALIVE_LOOP(&context);
                for (; body!=tail; ++body) {
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS_LABELED(loopbegin_fin,loopend_fin,break);
                }
            loopend_fin:;
                evalFinal(context);
            }
        }
    loopend:;
        context.stopFlags &= ~EvalFlags::stopForBreak;
        return v_zero();
    }
#endif

    // Return

    vec4f SimNode_Return::eval ( Context & context ) {
        DAS_PROFILE_NODE
        if ( subexpr ) context.abiResult() = subexpr->eval(context);
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnNothing::eval ( Context & context ) {
        DAS_PROFILE_NODE
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnConst::eval ( Context & context ) {
        DAS_PROFILE_NODE
        context.abiResult() = value;
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnConstString::eval ( Context & context ) {
        DAS_PROFILE_NODE
        context.abiResult() = cast<char *>::from(value);
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnRefAndEval::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pl = context.abiCopyOrMoveResult();
        DAS_ASSERT(pl);
        auto pR = ((char **)(context.stack.sp() + stackTop));
        *pR = pl;
        subexpr->evalPtr(context);;
        context.abiResult() = cast<char *>::from(pl);
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnAndCopy::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pr = subexpr->evalPtr(context);
        auto pl = context.abiCopyOrMoveResult();
        DAS_ASSERT(pl);
        memcpy ( pl, pr, size);
        context.abiResult() = cast<char *>::from(pl);
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnAndMove::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pr = subexpr->evalPtr(context);
        auto pl = context.abiCopyOrMoveResult();
        DAS_ASSERT(pl);
        if ( pl != pr ) {
            memcpy ( pl, pr, size);
            memset ( pr, 0, size);
        }
        context.abiResult() = cast<char *>::from(pl);
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnReference::eval ( Context & context ) {
        DAS_PROFILE_NODE
        char * ref = subexpr->evalPtr(context);
        if ( context.stack.bottom()<=ref && ref<context.stack.sp()) {
            context.throw_error_at(debugInfo,"reference bellow current function stack frame");
            return v_zero();
        }
#if DAS_ENABLE_STACK_WALK
        auto pp = (Prologue *) context.stack.sp();
        auto top = context.stack.sp() + pp->info->stackSize;
        if ( context.stack.sp()<=ref && ref<top ) {
            context.throw_error_at(debugInfo,"reference to current function stack frame");
            return v_zero();
        }
#endif
        context.abiResult() = cast<char *>::from(ref);
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnRefAndEvalFromBlock::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto ba = (BlockArguments *) ( context.stack.sp() + argStackTop );
        auto pl = ba->copyOrMoveResult;
        DAS_ASSERT(pl);
        auto pR = ((char **)(context.stack.sp() + stackTop));
        *pR = pl;
        subexpr->evalPtr(context);;
        context.abiResult() = cast<char *>::from(pl);
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnAndCopyFromBlock::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pr = subexpr->evalPtr(context);
        auto ba = (BlockArguments *) ( context.stack.sp() + argStackTop );
        auto pl = ba->copyOrMoveResult;
        memcpy ( pl, pr, size);
        context.abiResult() = cast<char *>::from(pl);
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnAndMoveFromBlock::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pr = subexpr->evalPtr(context);
        auto ba = (BlockArguments *) ( context.stack.sp() + argStackTop );
        auto pl = ba->copyOrMoveResult;
        if ( pl != pr ) {
            memcpy ( pl, pr, size);
            memset ( pr, 0, size);
        }
        context.abiResult() = cast<char *>::from(pl);
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnReferenceFromBlock::eval ( Context & context ) {
        DAS_PROFILE_NODE
        char * ref = subexpr->evalPtr(context);
        if ( context.stack.bottom()<=ref && ref<context.stack.ap() ) {
            context.throw_error_at(debugInfo,"reference bellow current call chain stack frame");
            return v_zero();
        }
        context.abiResult() = cast<char *>::from(ref);
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    vec4f SimNode_ReturnLocalCMRes::eval ( Context & context ) {
        DAS_PROFILE_NODE
        SimNode ** __restrict tail = list + total;
        SimNode ** __restrict body = list;
        for (; body!=tail; ++body) {
            (*body)->eval(context);
            if (context.stopFlags) break;
        }
        context.abiResult() = cast<char *>::from(context.abiCopyOrMoveResult());
        context.stopFlags |= EvalFlags::stopForReturn;
        return v_zero();
    }

    const LineInfo * SimFunction::getLineInfo() const { return &code->debugInfo; }
}

//workaround compiler bug in MSVC 32 bit
#if defined(_MSC_VER) && !defined(__clang__) && INTPTR_MAX == INT32_MAX
VECTORCALL vec4i v_ldu_ptr(const void * a) {return v_seti_x((int32_t)a);}
#endif
