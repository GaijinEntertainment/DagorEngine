#pragma once

#include "daScript/simulate/simulate.h"

namespace das
{
    template <typename TRange>
    struct RangeIterator : Iterator {
        using baseType = typename TRange::baseType;
        RangeIterator ( const TRange & r ) : rng(r) {}
        virtual bool first ( Context &, char * _value ) override {
            baseType * value = (baseType *) _value;
            *value      = rng.from;
            range_to    = rng.to;
            return rng.from < rng.to;
        }

        virtual bool next  ( Context &, char * _value ) override {
            baseType * value = (baseType *) _value;
            baseType nextValue = *value + 1;
            *value = nextValue;
            return (nextValue != range_to);
        }

        virtual void close ( Context & context, char * ) override {
            context.heap->free((char *)this, sizeof(RangeIterator<TRange>));
        }
        TRange  rng;
        baseType range_to;
    };

    template <typename TRange>
    struct SimNode_RangeIterator : SimNode {
        SimNode_RangeIterator ( const LineInfo & at, SimNode * rng )
            : SimNode(at), subexpr(rng) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            vec4f ll = subexpr->eval(context);
            TRange r = cast<TRange>::to(ll);
            char * iter = context.heap->allocate(sizeof(RangeIterator<TRange>));
            context.heap->mark_comment(iter,"range iterator");
            context.heap->mark_location(iter,&debugInfo);
            new (iter) RangeIterator<TRange>(r);
            return cast<char *>::from(iter);
        }
        SimNode * subexpr;
    };

    ////////////
    // FOR RANGE
    ////////////

    struct SimNode_ForRangeBase : SimNode_ForBase {
        SimNode_ForRangeBase ( const LineInfo & at )
            : SimNode_ForBase(at) {}
    };

    template <typename TRange>
    struct SimNode_ForRange : SimNode_ForRangeBase  {
        SimNode_ForRange ( const LineInfo & at )
            : SimNode_ForRangeBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    template <typename TRange>
    struct SimNode_ForRangeNF : SimNode_ForRangeBase  {
        SimNode_ForRangeNF ( const LineInfo & at )
            : SimNode_ForRangeBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    template <typename TRange>
    struct SimNode_ForRange1 : SimNode_ForRangeBase  {
        SimNode_ForRange1 ( const LineInfo & at )
            : SimNode_ForRangeBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    template <typename TRange>
    struct SimNode_ForRangeNF1 : SimNode_ForRangeBase  {
        SimNode_ForRangeNF1 ( const LineInfo & at )
            : SimNode_ForRangeBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    //////////////////
    // FOR RANGE DEBUG
    //////////////////

#if DAS_DEBUGGER

    template <typename TRange>
    struct SimNodeDebug_ForRange : SimNode_ForRange<TRange>  {
        SimNodeDebug_ForRange ( const LineInfo & at )
            : SimNode_ForRange<TRange>(at) {}
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    template <typename TRange>
    struct SimNodeDebug_ForRangeNF : SimNode_ForRangeNF<TRange>  {
        SimNodeDebug_ForRangeNF ( const LineInfo & at )
            : SimNode_ForRangeNF<TRange>(at) {}
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    template <typename TRange>
    struct SimNodeDebug_ForRange1 : SimNode_ForRange1<TRange>  {
        SimNodeDebug_ForRange1 ( const LineInfo & at )
            : SimNode_ForRange1<TRange>(at) {}
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    template <typename TRange>
    struct SimNodeDebug_ForRangeNF1 : SimNode_ForRangeNF1<TRange>  {
        SimNodeDebug_ForRangeNF1 ( const LineInfo & at )
            : SimNode_ForRangeNF1<TRange>(at) {}
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

#endif

    ////////////
    // FOR RANGE
    ////////////

    template <typename TRange>
    vec4f SimNode_ForRange<TRange>::eval ( Context & context ) {
        using baseType = typename TRange::baseType;
        DAS_PROFILE_NODE
        vec4f ll = this->sources[0]->eval(context);
        TRange r = cast<TRange>::to(ll);
        baseType * pi = (baseType *)(context.stack.sp() + this->stackTop[0]);
        baseType r_to = r.to;
        if ( r.from >= r_to ) goto loopend;
        { SimNode ** __restrict tail = this->list + this->total;
        for (baseType i = r.from; i != r_to; ++i) {
            *pi = i;
            SimNode ** __restrict body = this->list;
        loopbegin:;
            for (; body!=tail; ++body) {
                (*body)->eval(context);
                DAS_PROCESS_LOOP_FLAGS(break);
            }
        } }
    loopend:;
        this->evalFinal(context);
        context.stopFlags &= ~EvalFlags::stopForBreak;
        return v_zero();
    }

    template <typename TRange>
    vec4f SimNode_ForRangeNF<TRange>::eval ( Context & context ) {
        using baseType = typename TRange::baseType;
        DAS_PROFILE_NODE
        vec4f ll = this->sources[0]->eval(context);
        TRange r = cast<TRange>::to(ll);
        baseType * pi = (baseType *)(context.stack.sp() + this->stackTop[0]);
        baseType r_to = r.to;
        if ( r.from >= r_to ) goto loopend;
        { SimNode ** __restrict tail = this->list + this->total;
        for (baseType i = r.from; i != r_to; ++i) {
            *pi = i;
            for (SimNode ** __restrict body = this->list; body!=tail; ++body) {
                (*body)->eval(context);
            }
        } }
    loopend:;
        this->evalFinal(context);
        context.stopFlags &= ~(EvalFlags::stopForBreak | EvalFlags::stopForContinue);
        return v_zero();
    }

    template <typename TRange>
    vec4f SimNode_ForRange1<TRange>::eval ( Context & context ) {
        using baseType = typename TRange::baseType;
        DAS_PROFILE_NODE
        vec4f ll = this->sources[0]->eval(context);
        TRange r = cast<TRange>::to(ll);
        baseType * pi = (baseType *)(context.stack.sp() + this->stackTop[0]);
        baseType r_to = r.to;
        if ( r.from >= r_to ) goto loopend;
        { SimNode * __restrict pbody = this->list[0];
        for (baseType i = r.from; i != r_to; ++i) {
            *pi = i;
            pbody->eval(context);
            DAS_PROCESS_LOOP1_FLAGS(continue);
        } }
    loopend:;
        this->evalFinal(context);
        context.stopFlags &= ~(EvalFlags::stopForBreak | EvalFlags::stopForContinue);
        return v_zero();
    }

    template <typename TRange>
    vec4f SimNode_ForRangeNF1<TRange>::eval ( Context & context ) {
        using baseType = typename TRange::baseType;
        DAS_PROFILE_NODE
        vec4f ll = this->sources[0]->eval(context);
        TRange r = cast<TRange>::to(ll);
        baseType * pi = (baseType *)(context.stack.sp() + this->stackTop[0]);
        baseType r_to = r.to;
        if ( r.from >= r_to ) goto loopend;
        { SimNode * __restrict pbody = this->list[0];
        for (baseType i = r.from; i != r_to; ++i) {
            *pi = i;
            pbody->eval(context);
        } }
    loopend:;
        this->evalFinal(context);
        context.stopFlags &= ~(EvalFlags::stopForBreak | EvalFlags::stopForContinue);
        return v_zero();
    }

#if DAS_DEBUGGER

    //////////////////
    // FOR RANGE DEBUG
    //////////////////

    template <typename TRange>
    vec4f SimNodeDebug_ForRange<TRange>::eval ( Context & context ) {
        using baseType = typename TRange::baseType;
        DAS_PROFILE_NODE
        vec4f ll = this->sources[0]->eval(context);
        TRange r = cast<TRange>::to(ll);
        baseType * pi = (baseType *)(context.stack.sp() + this->stackTop[0]);
        baseType r_to = r.to;
        if ( r.from >= r_to ) goto loopend;
        { SimNode ** __restrict tail = this->list + this->total;
        for (baseType i = r.from; i != r_to; ++i) {
            *pi = i;
            SimNode ** __restrict body = this->list;
        loopbegin:;
            for (; body!=tail; ++body) {
                DAS_SINGLE_STEP(context,(*body)->debugInfo,true);
                (*body)->eval(context);
                DAS_PROCESS_LOOP_FLAGS(break);
            }
        } }
    loopend:;
        this->evalFinal(context);
        context.stopFlags &= ~EvalFlags::stopForBreak;
        return v_zero();
    }

    template <typename TRange>
    vec4f SimNodeDebug_ForRangeNF<TRange>::eval ( Context & context ) {
        using baseType = typename TRange::baseType;
        DAS_PROFILE_NODE
        vec4f ll = this->sources[0]->eval(context);
        TRange r = cast<TRange>::to(ll);
        baseType * pi = (baseType *)(context.stack.sp() + this->stackTop[0]);
        baseType r_to = r.to;
        if ( r.from >= r_to ) goto loopend;
        { SimNode ** __restrict tail = this->list + this->total;
        for (baseType i = r.from; i != r_to; ++i) {
            *pi = i;
            for (SimNode ** __restrict body = this->list; body!=tail; ++body) {
                DAS_SINGLE_STEP(context,(*body)->debugInfo,true);
                (*body)->eval(context);
            }
        } }
    loopend:;
        this->evalFinal(context);
        context.stopFlags &= ~(EvalFlags::stopForBreak | EvalFlags::stopForContinue);
        return v_zero();
    }

    template <typename TRange>
    vec4f SimNodeDebug_ForRange1<TRange>::eval ( Context & context ) {
        using baseType = typename TRange::baseType;
        DAS_PROFILE_NODE
        vec4f ll = this->sources[0]->eval(context);
        TRange r = cast<TRange>::to(ll);
        baseType * pi = (baseType *)(context.stack.sp() + this->stackTop[0]);
        baseType r_to = r.to;
        if ( r.from >= r_to ) goto loopend;
        {
        for (baseType i = r.from; i != r_to; ++i) {
            *pi = i;
            SimNode * pbody = this->list[0];   // note: instruments
            DAS_SINGLE_STEP(context,pbody->debugInfo,true);
            pbody->eval(context);
            DAS_PROCESS_LOOP1_FLAGS(continue);
        } }
    loopend:;
        this->evalFinal(context);
        context.stopFlags &= ~(EvalFlags::stopForBreak | EvalFlags::stopForContinue);
        return v_zero();
    }

    template <typename TRange>
    vec4f SimNodeDebug_ForRangeNF1<TRange>::eval ( Context & context ) {
        using baseType = typename TRange::baseType;
        DAS_PROFILE_NODE
        vec4f ll = this->sources[0]->eval(context);
        TRange r = cast<TRange>::to(ll);
        baseType * pi = (baseType *)(context.stack.sp() + this->stackTop[0]);
        baseType r_to = r.to;
        if ( r.from >= r_to ) goto loopend;
        {
        for (baseType i = r.from; i != r_to; ++i) {
            *pi = i;
            SimNode * pbody = this->list[0];   // note: instruments
            DAS_SINGLE_STEP(context,pbody->debugInfo,true);
            pbody->eval(context);
        } }
    loopend:;
        this->evalFinal(context);
        context.stopFlags &= ~(EvalFlags::stopForBreak | EvalFlags::stopForContinue);
        return v_zero();
    }

#endif

}

