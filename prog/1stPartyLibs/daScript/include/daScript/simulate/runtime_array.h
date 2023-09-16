#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/misc/arraytype.h"

#include "daScript/simulate/simulate_visit_op.h"

namespace das
{
    // AT (INDEX)
    struct SimNode_ArrayAt : SimNode {
        DAS_PTR_NODE;
        SimNode_ArrayAt ( const LineInfo & at, SimNode * ll, SimNode * rr, uint32_t sz, uint32_t o)
            : SimNode(at), l(ll), r(rr), stride(sz), offset(o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            DAS_PROFILE_NODE
            Array * pA = (Array *) l->evalPtr(context);
            auto idx = uint32_t(r->evalInt(context));
            if ( idx >= pA->size ) context.throw_error_at(debugInfo,"array index out of range, %u of %u", idx, pA->size);
            return pA->data + idx*stride + offset;
        }
        SimNode * l, * r;
        uint32_t stride, offset;
    };

    template <typename TT>
    struct SimNode_ArrayAtR2V : SimNode_ArrayAt {
        SimNode_ArrayAtR2V ( const LineInfo & at, SimNode * rv, SimNode * idx, uint32_t strd, uint32_t o )
            : SimNode_ArrayAt(at,rv,idx,strd,o) {}
        SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP_TT(ArrayAtR2V);
            V_SUB(l);
            V_SUB(r);
            V_ARG(stride);
            V_ARG(offset);
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            TT * pR = (TT *) compute(context);
            return cast<TT>::from(*pR);
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            DAS_PROFILE_NODE \
            return *(CTYPE *)compute(context);                      \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
    };

    // AT (INDEX)
    struct SimNode_SafeArrayAt : SimNode_ArrayAt {
        DAS_PTR_NODE;
        SimNode_SafeArrayAt ( const LineInfo & at, SimNode * ll, SimNode * rr, uint32_t sz, uint32_t o)
            : SimNode_ArrayAt(at,ll,rr,sz,o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            DAS_PROFILE_NODE
            Array * pA = (Array *) l->evalPtr(context);
            if ( !pA ) return nullptr;
            auto idx = uint32_t(r->evalInt(context));
            if (idx >= pA->size) return nullptr;
            return pA->data + idx*stride + offset;
        }
    };

    struct GoodArrayIterator : Iterator {
        GoodArrayIterator ( Array * arr, uint32_t st ) : array(arr), stride(st) {}
        virtual bool first ( Context & context, char * value ) override;
        virtual bool next  ( Context & context, char * value ) override;
        virtual void close ( Context & context, char * value ) override;
        Array *     array;
        uint32_t    stride;
        char *      data = nullptr;
        char *      array_end = nullptr;
    };

    struct SimNode_GoodArrayIterator : SimNode {
        SimNode_GoodArrayIterator ( const LineInfo & at, SimNode * s, uint32_t st )
            : SimNode(at), source(s), stride(st) { }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override;
        SimNode *   source;
        uint32_t    stride;
    };

    struct FixedArrayIterator : Iterator {
        FixedArrayIterator ( char * d, uint32_t sz, uint32_t st ) : data(d), size(sz), stride(st) {}
        virtual bool first ( Context & context, char * value ) override;
        virtual bool next  ( Context & context, char * value ) override;
        virtual void close ( Context & context, char * value ) override;
        char *      data;
        uint32_t    size;
        uint32_t    stride;
        char *      fixed_array_end = nullptr;
    };

    struct SimNode_FixedArrayIterator : SimNode {
        SimNode_FixedArrayIterator ( const LineInfo & at, SimNode * s, uint32_t sz, uint32_t st )
            : SimNode(at), source(s), size(sz), stride(st) { }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override;
        SimNode *   source;
        uint32_t    size;
        uint32_t    stride;
    };

    struct SimNode_DeleteArray : SimNode_Delete {
        SimNode_DeleteArray ( const LineInfo & a, SimNode * s, uint32_t t, uint32_t st )
            : SimNode_Delete(a,s,t), stride(st) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override;
        uint32_t stride;
    };

    /////////////////
    // FOR GOOD ARRAY
    /////////////////

    template <int totalCount>
    struct SimNode_ForGoodArray : public SimNode_ForBase {
        SimNode_ForGoodArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, totalCount, "ForGoodArray");
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            Array * __restrict pha[totalCount];
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pha[t] = cast<Array *>::to(sources[t]->eval(context));
                array_lock(context, *pha[t], &this->debugInfo);
                ph[t]  = pha[t]->data;
            }
            char ** __restrict pi[totalCount];
            int szz = INT_MAX;
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + stackTop[t]);
                szz = das::min(szz, int(pha[t]->size));
            }
            SimNode ** __restrict tail = list + total;
            for (int i = 0; i!=szz; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += strides[t];
                }
                SimNode ** __restrict body = list;
            loopbegin:;
                for (; body!=tail; ++body) {
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS(break);
                }
            }
        loopend:;
            for ( int t=0; t!=totalCount; ++t ) {
                array_unlock(context, *pha[t], &this->debugInfo);
            }
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForGoodArray<0> : public SimNode_ForBase {
        SimNode_ForGoodArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN_CR();
            V_OP(ForGoodArray_0);
            V_FINAL();
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            evalFinal(context);
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForGoodArray<1> : public SimNode_ForBase {
        SimNode_ForGoodArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, 1, "ForGoodArray");
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            Array * __restrict pha;
            char * __restrict ph;
            pha = cast<Array *>::to(sources[0]->eval(context));
            array_lock(context, *pha, &this->debugInfo);
            ph = pha->data;
            char ** __restrict pi;
            int szz = int(pha->size);
            pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            SimNode ** __restrict tail = list + total;
            for (int i = 0; i!=szz; ++i) {
                *pi = ph;
                ph += stride;
                SimNode ** __restrict body = list;
            loopbegin:;
                for (; body!=tail; ++body) {
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS(break);
                }
            }
        loopend:;
            evalFinal(context);
            array_unlock(context, *pha, &this->debugInfo);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <int totalCount>
    struct SimNode_ForGoodArray1 : public SimNode_ForBase {
        SimNode_ForGoodArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, totalCount, "ForGoodArray1");
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            Array * __restrict pha[totalCount];
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pha[t] = cast<Array *>::to(sources[t]->eval(context));
                array_lock(context, *pha[t], &this->debugInfo);
                ph[t]  = pha[t]->data;
            }
            char ** __restrict pi[totalCount];
            int szz = INT_MAX;
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + stackTop[t]);
                szz = das::min(szz, int(pha[t]->size));
            }
            SimNode * __restrict body = list[0];
            for (int i = 0; i!=szz && !context.stopFlags; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += strides[t];
                }
                body->eval(context);
                DAS_PROCESS_LOOP1_FLAGS(continue);
            }
        loopend:;
            evalFinal(context);
            for ( int t=0; t!=totalCount; ++t ) {
                array_unlock(context, *pha[t], &this->debugInfo);
            }
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForGoodArray1<0> : public SimNode_ForBase {
        SimNode_ForGoodArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN_CR();
            V_OP(ForGoodArray1_0);
            V_FINAL();
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            evalFinal(context);
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForGoodArray1<1> : public SimNode_ForBase {
        SimNode_ForGoodArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, 1, "ForGoodArray1");
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            Array * __restrict pha;
            char * __restrict ph;
            pha = cast<Array *>::to(sources[0]->eval(context));
            array_lock(context, *pha, &this->debugInfo);
            ph = pha->data;
            char ** __restrict pi;
            int szz = int(pha->size);
            pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            SimNode * __restrict body = list[0];
            for (int i = 0; i!=szz && !context.stopFlags; ++i) {
                *pi = ph;
                ph += stride;
                body->eval(context);
                DAS_PROCESS_LOOP1_FLAGS(continue);
            }
        loopend:;
            evalFinal(context);
            array_unlock(context, *pha, &this->debugInfo);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    ///////////////////////
    // FOR GOOD ARRAY DEBUG
    ///////////////////////

#if DAS_DEBUGGER

    template <int totalCount>
    struct SimNodeDebug_ForGoodArray : public SimNode_ForGoodArray<totalCount> {
        SimNodeDebug_ForGoodArray ( const LineInfo & at ) : SimNode_ForGoodArray<totalCount>(at) {}
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            Array * __restrict pha[totalCount];
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pha[t] = cast<Array *>::to(this->sources[t]->eval(context));
                array_lock(context, *pha[t], &this->debugInfo);
                ph[t]  = pha[t]->data;
            }
            char ** __restrict pi[totalCount];
            int szz = INT_MAX;
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + this->stackTop[t]);
                szz = das::min(szz, int(pha[t]->size));
            }
            SimNode ** __restrict tail = this->list + this->total;
            for (int i = 0; i!=szz; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += this->strides[t];
                }
                SimNode ** __restrict body = this->list;
            loopbegin:;
                for (; body!=tail; ++body) {
                    DAS_SINGLE_STEP(context,(*body)->debugInfo,true);
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS(break);
                }
            }
        loopend:;
            for ( int t=0; t!=totalCount; ++t ) {
                array_unlock(context, *pha[t], &this->debugInfo);
            }
            this->evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNodeDebug_ForGoodArray<0> : public SimNode_ForGoodArray<0> {
        SimNodeDebug_ForGoodArray ( const LineInfo & at ) : SimNode_ForGoodArray<0>(at) {}
    };

    template <>
    struct SimNodeDebug_ForGoodArray<1> : public SimNode_ForGoodArray<1> {
        SimNodeDebug_ForGoodArray ( const LineInfo & at ) : SimNode_ForGoodArray<1>(at) {}
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            Array * __restrict pha;
            char * __restrict ph;
            pha = cast<Array *>::to(sources[0]->eval(context));
            array_lock(context, *pha, &this->debugInfo);
            ph = pha->data;
            char ** __restrict pi;
            int szz = int(pha->size);
            pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            SimNode ** __restrict tail = list + total;
            for (int i = 0; i!=szz; ++i) {
                *pi = ph;
                ph += stride;
                SimNode ** __restrict body = list;
            loopbegin:;
                for (; body!=tail; ++body) {
                    DAS_SINGLE_STEP(context,(*body)->debugInfo,true);
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS(break);
                }
            }
        loopend:;
            evalFinal(context);
            array_unlock(context, *pha, &this->debugInfo);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <int totalCount>
    struct SimNodeDebug_ForGoodArray1 : public SimNode_ForGoodArray1<totalCount> {
        SimNodeDebug_ForGoodArray1 ( const LineInfo & at ) : SimNode_ForGoodArray1<totalCount>(at) {}
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            Array * __restrict pha[totalCount];
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pha[t] = cast<Array *>::to(this->sources[t]->eval(context));
                array_lock(context, *pha[t], &this->debugInfo);
                ph[t]  = pha[t]->data;
            }
            char ** __restrict pi[totalCount];
            int szz = INT_MAX;
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + this->stackTop[t]);
                szz = das::min(szz, int(pha[t]->size));
            }
            SimNode * __restrict body = this->list[0];
            for (int i = 0; i!=szz && !context.stopFlags; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += this->strides[t];
                }
                DAS_SINGLE_STEP(context,body->debugInfo,true);
                body->eval(context);
                DAS_PROCESS_LOOP1_FLAGS(continue);
            }
        loopend:;
            this->evalFinal(context);
            for ( int t=0; t!=totalCount; ++t ) {
                array_unlock(context, *pha[t], &this->debugInfo);
            }
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNodeDebug_ForGoodArray1<0> : public SimNode_ForGoodArray1<0> {
        SimNodeDebug_ForGoodArray1 ( const LineInfo & at ) : SimNode_ForGoodArray1<0>(at) {}
    };

    template <>
    struct SimNodeDebug_ForGoodArray1<1> : public SimNode_ForGoodArray1<1> {
        SimNodeDebug_ForGoodArray1 ( const LineInfo & at ) : SimNode_ForGoodArray1<1>(at) {}
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            Array * __restrict pha;
            char * __restrict ph;
            pha = cast<Array *>::to(sources[0]->eval(context));
            array_lock(context, *pha, &this->debugInfo);
            ph = pha->data;
            char ** __restrict pi;
            int szz = int(pha->size);
            pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            for (int i = 0; i!=szz && !context.stopFlags; ++i) {
                *pi = ph;
                ph += stride;
                SimNode * body = list[0];    // note: instruments
                DAS_SINGLE_STEP(context,body->debugInfo,true);
                body->eval(context);
                DAS_PROCESS_LOOP1_FLAGS(continue);
            }
        loopend:;
            evalFinal(context);
            array_unlock(context, *pha, &this->debugInfo);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

#endif

    //////////////////
    // FOR FIXED ARRAY
    //////////////////

    // FOR
    template <int totalCount>
    struct SimNode_ForFixedArray : SimNode_ForBase {
        SimNode_ForFixedArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, totalCount, "ForFixedArray");
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                ph[t] = cast<char *>::to(sources[t]->eval(context));
            }
            char ** __restrict pi[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + stackTop[t]);
            }
            SimNode ** __restrict tail = list + total;
            for (uint32_t i=0, is=size; i!=is; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += strides[t];
                }
                SimNode ** __restrict body = list;
            loopbegin:;
                for (; body!=tail; ++body) {
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS(break);
                }
            }
        loopend:;
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForFixedArray<0> : SimNode_ForBase {
        SimNode_ForFixedArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN_CR();
            V_OP(ForFixedArray_0);
            V_FINAL();
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            evalFinal(context);
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForFixedArray<1> : SimNode_ForBase {
        SimNode_ForFixedArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, 1, "ForFixedArray");
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            char * __restrict ph = cast<char *>::to(sources[0]->eval(context));
            char ** __restrict pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            SimNode ** __restrict tail = list + total;
            for (uint32_t i=0, is=size; i!=is; ++i) {
                *pi = ph;
                ph += stride;
                SimNode ** __restrict body = list;
            loopbegin:;
                for (; body!=tail; ++body) {
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS(break);
                }
            }
        loopend:;
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    // FOR
    template <int totalCount>
    struct SimNode_ForFixedArray1 : SimNode_ForBase {
        SimNode_ForFixedArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, totalCount, "ForFixedArray1");
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                ph[t] = cast<char *>::to(sources[t]->eval(context));
            }
            char ** __restrict pi[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + stackTop[t]);
            }
            SimNode * __restrict body = list[0];
            for (uint32_t i=0, is=size; i!=is && !context.stopFlags; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += strides[t];
                }
                body->eval(context);
                DAS_PROCESS_LOOP1_FLAGS(continue);
            }
        loopend:;
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForFixedArray1<0> : SimNode_ForBase {
        SimNode_ForFixedArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN_CR();
            V_OP(ForFixedArray1_0);
            V_FINAL();
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            evalFinal(context);
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForFixedArray1<1> : SimNode_ForBase {
        SimNode_ForFixedArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, 1, "ForFixedArray1");
        }
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            char * __restrict ph = cast<char *>::to(sources[0]->eval(context));
            char ** __restrict pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            SimNode * __restrict body = list[0];
            for (uint32_t i=0, is=size; i!=is && !context.stopFlags; ++i) {
                *pi = ph;
                ph += stride;
                body->eval(context);
                DAS_PROCESS_LOOP1_FLAGS(continue);
            }
        loopend:;
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    ////////////////////////
    // FOR FIXED ARRAY DEBUG
    ////////////////////////

#if DAS_DEBUGGER

    // FOR
    template <int totalCount>
    struct SimNodeDebug_ForFixedArray : SimNode_ForFixedArray<totalCount> {
        SimNodeDebug_ForFixedArray ( const LineInfo & at ) : SimNode_ForFixedArray<totalCount>(at) {}
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                ph[t] = cast<char *>::to(this->sources[t]->eval(context));
            }
            char ** __restrict pi[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + this->stackTop[t]);
            }
            SimNode ** __restrict tail = this->list + this->total;
            for (uint32_t i=0, is=this->size; i!=is; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += this->strides[t];
                }
                SimNode ** __restrict body = this->list;
            loopbegin:;
                for (; body!=tail; ++body) {
                    DAS_SINGLE_STEP(context,(*body)->debugInfo,true);
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS(break);
                }
            }
        loopend:;
            this->evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNodeDebug_ForFixedArray<0> : SimNode_ForFixedArray<0> {
        SimNodeDebug_ForFixedArray ( const LineInfo & at ) : SimNode_ForFixedArray<0>(at) {}
    };

    template <>
    struct SimNodeDebug_ForFixedArray<1> : SimNode_ForFixedArray<1> {
        SimNodeDebug_ForFixedArray ( const LineInfo & at ) : SimNode_ForFixedArray<1>(at) {}
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            char * __restrict ph = cast<char *>::to(sources[0]->eval(context));
            char ** __restrict pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            SimNode ** __restrict tail = list + total;
            for (uint32_t i=0, is=size; i!=is; ++i) {
                *pi = ph;
                ph += stride;
                SimNode ** __restrict body = list;
            loopbegin:;
                for (; body!=tail; ++body) {
                    DAS_SINGLE_STEP(context,(*body)->debugInfo,true);
                    (*body)->eval(context);
                    DAS_PROCESS_LOOP_FLAGS(break);
                }
            }
        loopend:;
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    // FOR
    template <int totalCount>
    struct SimNodeDebug_ForFixedArray1 : SimNode_ForFixedArray1<totalCount> {
        SimNodeDebug_ForFixedArray1 ( const LineInfo & at ) : SimNode_ForFixedArray1<totalCount>(at) {}
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                ph[t] = cast<char *>::to(this->sources[t]->eval(context));
            }
            char ** __restrict pi[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + this->stackTop[t]);
            }
            for (uint32_t i=0, is=this->size; i!=is && !context.stopFlags; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += this->strides[t];
                }
                SimNode * body = this->list[0]; // note: instruments
                DAS_SINGLE_STEP(context,body->debugInfo,true);
                body->eval(context);
                DAS_PROCESS_LOOP1_FLAGS(continue);
            }
        loopend:;
            this->evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNodeDebug_ForFixedArray1<0> : SimNode_ForFixedArray1<0> {
        SimNodeDebug_ForFixedArray1 ( const LineInfo & at ) : SimNode_ForFixedArray1<0>(at) {}
    };

    template <>
    struct SimNodeDebug_ForFixedArray1<1> : SimNode_ForFixedArray1<1> {
        SimNodeDebug_ForFixedArray1 ( const LineInfo & at ) : SimNode_ForFixedArray1<1>(at) {}
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
            DAS_PROFILE_NODE
            char * __restrict ph = cast<char *>::to(sources[0]->eval(context));
            char ** __restrict pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            for (uint32_t i=0, is=size; i!=is && !context.stopFlags; ++i) {
                *pi = ph;
                ph += stride;
                SimNode * body = list[0];    // note: instruments
                DAS_SINGLE_STEP(context,body->debugInfo,true);
                body->eval(context);
                DAS_PROCESS_LOOP1_FLAGS(continue);
            }
        loopend:;
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

#endif

}

#include "daScript/simulate/simulate_visit_op_undef.h"
