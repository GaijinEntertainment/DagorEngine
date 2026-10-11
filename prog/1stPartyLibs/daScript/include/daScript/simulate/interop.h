#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/simulate_visit_op.h"

namespace das
{
    template <typename TT>
    struct cast_arg {
        static __forceinline TT to ( Context & ctx, SimNode * x ) {
            return EvalTT<TT>::eval(ctx,x);
        }
    };

    template <>
    struct cast_arg<char *> {
        static __forceinline char * to ( Context & ctx, SimNode * node ) {
            char * res = node->evalPtr(ctx);
            return res ? res : ((char *)"");
        }
    };

    template <>
    struct cast_arg<const char *> {
        static __forceinline char * to ( Context & ctx, SimNode * node ) {
            char * res = node->evalPtr(ctx);
            return res ? res : ((char *)"");
        }
    };

    template <>
    struct cast_arg<Context *> {
        static __forceinline Context * to ( Context & ctx, SimNode * ) {
            return &ctx;
        }
    };

    template <>
    struct cast_arg<LineInfoArg *> {
        static __forceinline LineInfoArg * to ( Context &, SimNode * node ) {
            return (LineInfoArg *) (&node->debugInfo);
        }
    };

    template <>
    struct cast_arg<vec4f> {
        static __forceinline vec4f to ( Context & ctx, SimNode * x ) {
            return x->eval(ctx);
        }
    };

    template <typename TT>
    struct cast_res {
        static __forceinline vec4f from ( TT x, Context * ) {
            return cast<TT>::from(x);
        }
    };

    template <typename>
    struct is_stub_type
    {
      static constexpr bool value = false;
    };

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#elif defined(__EDG__)
#pragma diag_suppress 826,3327
#endif

    template <typename R, typename ...Args, size_t... I>
    __forceinline R CallStaticFunction ( R (* fn) (Args...), Context & ctx, SimNode ** args, index_sequence<I...> ) {
        return fn(cast_arg<Args>::to(ctx,args[I])...);
    }

    template <typename R, typename ...Args>
    __forceinline R CallStaticFunction ( R (* fn) (Args...), Context & ctx, SimNode ** args ) {
        return CallStaticFunction(fn,ctx,args,make_index_sequence<sizeof...(Args)>());
    }

    template <typename T>
    struct is_any_pointer {
        enum { value = is_pointer<T>::value || is_smart_ptr<T>::value };
    };

    template <typename TT>
    struct is_workhorse_type {
        enum {
            value =
                    is_enum<TT>::value
                ||  is_pointer<TT>::value
                ||  is_arithmetic<TT>::value
                ||  is_same<TT,bool>::value
                ||  is_same<TT,Bitfield>::value
                ||  is_same<TT,range>::value
                ||  is_same<TT,urange>::value
                ||  is_same<TT,range64>::value
                ||  is_same<TT,urange64>::value
                ||  is_same<TT,int2>::value
                ||  is_same<TT,int3>::value
                ||  is_same<TT,int4>::value
                ||  is_same<TT,uint2>::value
                ||  is_same<TT,uint3>::value
                ||  is_same<TT,uint4>::value
                ||  is_same<TT,float2>::value
                ||  is_same<TT,float3>::value
                ||  is_same<TT,float4>::value
        };
    };

    // 16/8-bit lattice values (plus the float16_t scalar): carried in the vec4f slot like the
    // classic vector types, but NOT workhorse — a typed eval on a lattice-returning extern is
    // legal (value reinterpret compiles to a plain pass-through, so e.g. `reinterpret<uint>(
    // half2(x))` reads the call node via evalUInt) and must round-trip the vec4f slot
    template <typename TT>
    struct is_lattice_interop_type {
        enum {
            value =
                    is_same<TT,float16_t>::value
                ||  is_same<TT,half2>::value   || is_same<TT,half3>::value   || is_same<TT,half4>::value   || is_same<TT,half8>::value
                ||  is_same<TT,short2>::value  || is_same<TT,short3>::value  || is_same<TT,short4>::value  || is_same<TT,short8>::value
                ||  is_same<TT,ushort2>::value || is_same<TT,ushort3>::value || is_same<TT,ushort4>::value || is_same<TT,ushort8>::value
                ||  is_same<TT,byte2>::value   || is_same<TT,byte3>::value   || is_same<TT,byte4>::value   || is_same<TT,byte8>::value   || is_same<TT,byte16>::value
                ||  is_same<TT,ubyte2>::value  || is_same<TT,ubyte3>::value  || is_same<TT,ubyte4>::value  || is_same<TT,ubyte8>::value  || is_same<TT,ubyte16>::value
        };
    };

    template <typename FunctionType>
    struct ImplCallStaticFunctionAndCopy;

    template <typename R, typename ...Args>
    struct ImplCallStaticFunctionAndCopy < R (*)(Args...) > {
        static __forceinline void call ( R (*fn)(Args...), Context & ctx, void * res, SimNode ** args ) {
            using result = typename remove_const<R>::type;
            // note: copy is closer to AOT, but placement new is correct behavior under simulation
            // *((result *)res) = CallStaticFunction<R,Args...>(fn,ctx,args);
            (void)fn;
            (void)ctx;
            (void)res;
            (void)args;
            if constexpr (das::is_stub_type<R>::value) { DAS_ASSERTF(false, "STUB!"); }
            else new (res) result ( CallStaticFunction<R,Args...>(fn,ctx,args) );
        }
    };

#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__EDG__)
#pragma diag_default 826,3327
#endif

    struct SimNode_ExtFuncCallBase : SimNode_CallBase {
        const char* extFnName = nullptr;
        SimNode_ExtFuncCallBase(const LineInfo& at, const char* fnName)
            : SimNode_CallBase(at,"") {
            extFnName = fnName;
        }
        virtual SimNode* copyNode(Context& context, NodeAllocator* code) override {
            auto that = (SimNode_ExtFuncCallBase *) SimNode_CallBase::copyNode(context, code);
            that->extFnName = code->allocateName(extFnName);
            return that;
        }
        virtual SimNode* visit(SimVisitor& vis) override {
            V_BEGIN();
            vis.op(extFnName);
            V_CALL();
            V_END();
        }
    };

    // the carrier is the one typed-eval slot a return type owns; this tag carries
    // vec4f-boxed results that do NOT license typed-slot reads
    struct vec4f_boxed {};

    template <typename R>
    struct ext_ret_carrier {
        using RR = typename remove_const<R>::type;
        static_assert(!is_same<RR,void>::value, "void handled by specialization");
        // WrapType routes handle-like types (distinct ints, Handle<T>, Time) to the
        // scalar slot they are ABI-identical to, so the scalar chain keys on WR
        using WR = typename WrapType<RR>::rettype;
        using type =
            typename conditional<is_same<RR,bool>::value,     bool,
            typename conditional<is_any_pointer<RR>::value,   char *,
            typename conditional<is_enum<RR>::value,
                typename conditional<sizeof(RR)==8, int64_t, int32_t>::type,
            typename conditional<is_same<WR,float>::value,    float,
            typename conditional<is_same<WR,double>::value,   double,
            typename conditional<is_integral<WR>::value && sizeof(WR)==4,
                typename conditional<is_signed<WR>::value, int32_t, uint32_t>::type,
            typename conditional<is_integral<WR>::value && sizeof(WR)==8,
                typename conditional<is_signed<WR>::value, int64_t, uint64_t>::type,
            typename conditional<is_lattice_interop_type<RR>::value, vec4f,
            vec4f_boxed>::type>::type>::type>::type>::type>::type>::type>::type;
    };

    template <>
    struct ext_ret_carrier<void> {
        using type = void;
    };

    // the value type a carrier's computeDirect traffics in (the boxed tag
    // itself never crosses an ABI — it stands for a vec4f payload)
    template <typename C> struct ext_carrier_value { using type = C; };
    template <> struct ext_carrier_value<vec4f_boxed> { using type = vec4f; };

    // R -> carrier value conversion (pointers decay, smart pointers orphan/get,
    // enums widen to their slot's integer)
    template <typename R, typename Carrier>
    struct ext_ret_to_carrier {
        static __forceinline Carrier from ( R v ) { return (Carrier) v; }
    };
    template <typename R>
    struct ext_ret_to_carrier<smart_ptr<R>, char *> {
        static __forceinline char * from ( smart_ptr<R> v ) { return (char *) v.orphan(); }
    };
    template <typename R>
    struct ext_ret_to_carrier<smart_ptr_raw<R>, char *> {
        static __forceinline char * from ( smart_ptr_raw<R> v ) { return (char *) v.get(); }
    };
    template <typename R>
    struct ext_ret_to_carrier<R *, char *> {
        static __forceinline char * from ( R * v ) { return (char *) v; }
    };

    // cross-slot typed evals: signedness pairs forward (uint-returning externs are
    // read via evalInt in indexing shapes); anything else throws through one shared path

    [[noreturn]] inline void ext_wrong_slot ( Context & ctx, const char * fnName ) {
        ctx.throw_error_ex("internal binding error: typed eval on wrong extern return kind, %s", fnName ? fnName : "<unknown>");
        abort();
    }

    template <typename Carrier, typename Slot>
    struct ext_slot_cast {
        enum { valid = false };
        static __forceinline Slot to ( Carrier ) { return Slot(); }
    };
    template <typename Same>
    struct ext_slot_cast<Same,Same> {
        enum { valid = true };
        static __forceinline Same to ( Same v ) { return v; }
    };
    template <> struct ext_slot_cast<uint32_t,int32_t> {
        enum { valid = true };
        static __forceinline int32_t to ( uint32_t v ) { return (int32_t) v; }
    };
    template <> struct ext_slot_cast<int32_t,uint32_t> {
        enum { valid = true };
        static __forceinline uint32_t to ( int32_t v ) { return (uint32_t) v; }
    };
    template <> struct ext_slot_cast<uint64_t,int64_t> {
        enum { valid = true };
        static __forceinline int64_t to ( uint64_t v ) { return (int64_t) v; }
    };
    template <> struct ext_slot_cast<int64_t,uint64_t> {
        enum { valid = true };
        static __forceinline uint64_t to ( int64_t v ) { return (uint64_t) v; }
    };

    // one base per carrier kind, not per bind; a base slot body only runs for
    // cross-slot plumbing or the shared wrong-slot throw — the direct layer owns the rest

    template <typename Carrier>
    struct SimNode_ExtFuncCallRet : SimNode_ExtFuncCallBase {
        SimNode_ExtFuncCallRet(const LineInfo & at, const char * fnName)
            : SimNode_ExtFuncCallBase(at,fnName) { }
#define EVAL_NODE(TYPE,CTYPE) \
        virtual CTYPE eval##TYPE ( Context & context ) override { \
            if constexpr ( ext_slot_cast<Carrier,CTYPE>::valid ) { \
                return ext_slot_cast<Carrier,CTYPE>::to(cast<Carrier>::to(this->eval(context))); \
            } else { \
                ext_wrong_slot(context, extFnName); \
            } \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    template <>
    struct SimNode_ExtFuncCallRet<vec4f> : SimNode_ExtFuncCallBase {
        SimNode_ExtFuncCallRet(const LineInfo & at, const char * fnName)
            : SimNode_ExtFuncCallBase(at,fnName) { }
        // lattice results consumed through a typed slot round-trip the vec4f
        // box exactly like a local hop would
#define EVAL_NODE(TYPE,CTYPE) \
        virtual CTYPE eval##TYPE ( Context & context ) override { \
            return cast<CTYPE>::to(this->eval(context)); \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    template <>
    struct SimNode_ExtFuncCallRet<vec4f_boxed> : SimNode_ExtFuncCallBase {
        SimNode_ExtFuncCallRet(const LineInfo & at, const char * fnName)
            : SimNode_ExtFuncCallBase(at,fnName) { }
#define EVAL_NODE(TYPE,CTYPE) \
        virtual CTYPE eval##TYPE ( Context & context ) override { \
            ext_wrong_slot(context, extFnName); \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    template <>
    struct SimNode_ExtFuncCallRet<void> : SimNode_ExtFuncCallBase {
        SimNode_ExtFuncCallRet(const LineInfo & at, const char * fnName)
            : SimNode_ExtFuncCallBase(at,fnName) { }
#define EVAL_NODE(TYPE,CTYPE) \
        virtual CTYPE eval##TYPE ( Context & context ) override { \
            ext_wrong_slot(context, extFnName); \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    // CRTP over the leaf: eval() and the carrier's slot call the non-virtual
    // computeDirect, so a consumed extern costs exactly one virtual call

    template <typename LeafT, typename Carrier>
    struct SimNode_ExtFuncCallDirect;

#define EVAL_NODE(TYPE,CTYPE) \
    template <typename LeafT> \
    struct SimNode_ExtFuncCallDirect<LeafT, CTYPE> : SimNode_ExtFuncCallRet<CTYPE> { \
        SimNode_ExtFuncCallDirect(const LineInfo & at, const char * fnName) \
            : SimNode_ExtFuncCallRet<CTYPE>(at,fnName) { } \
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            return cast<CTYPE>::from(static_cast<LeafT *>(this)->computeDirect(context)); \
        } \
        virtual CTYPE eval##TYPE ( Context & context ) override { \
            DAS_PROFILE_NODE \
            return static_cast<LeafT *>(this)->computeDirect(context); \
        } \
    };
    DAS_EVAL_NODE
#undef  EVAL_NODE

    template <typename LeafT>
    struct SimNode_ExtFuncCallDirect<LeafT, vec4f> : SimNode_ExtFuncCallRet<vec4f> {
        SimNode_ExtFuncCallDirect(const LineInfo & at, const char * fnName)
            : SimNode_ExtFuncCallRet<vec4f>(at,fnName) { }
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            return static_cast<LeafT *>(this)->computeDirect(context);
        }
    };

    template <typename LeafT>
    struct SimNode_ExtFuncCallDirect<LeafT, vec4f_boxed> : SimNode_ExtFuncCallRet<vec4f_boxed> {
        SimNode_ExtFuncCallDirect(const LineInfo & at, const char * fnName)
            : SimNode_ExtFuncCallRet<vec4f_boxed>(at,fnName) { }
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            return static_cast<LeafT *>(this)->computeDirect(context);
        }
    };

    template <typename LeafT>
    struct SimNode_ExtFuncCallDirect<LeafT, void> : SimNode_ExtFuncCallRet<void> {
        SimNode_ExtFuncCallDirect(const LineInfo & at, const char * fnName)
            : SimNode_ExtFuncCallRet<void>(at,fnName) { }
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            static_cast<LeafT *>(this)->computeDirect(context);
            return v_zero();
        }
    };

    template <typename R, typename ...Args>
    __forceinline typename ext_carrier_value<typename ext_ret_carrier<R>::type>::type
    ext_compute ( R (*fnp)(Args...), Context & context, SimNode ** args ) {
        using Carrier = typename ext_ret_carrier<R>::type;
        if constexpr ( is_same<Carrier,vec4f>::value || is_same<Carrier,vec4f_boxed>::value ) {
            return cast_res<R>::from(CallStaticFunction<R,Args...>(fnp,context,args),&context);
        } else if constexpr ( is_same<R,void>::value ) {
            CallStaticFunction<void,Args...>(fnp,context,args);
        } else {
            return ext_ret_to_carrier<R,Carrier>::from(CallStaticFunction<R,Args...>(fnp,context,args));
        }
    }

    // fn is a runtime member, so all binds of one signature share a single
    // instantiation; the typed-eval matrix lives in the per-carrier base

    template <typename FuncT>
    struct SimNode_ExtFuncCall;

    template <typename R, typename ...Args>
    struct SimNode_ExtFuncCall<R (*)(Args...)>
        : SimNode_ExtFuncCallDirect<SimNode_ExtFuncCall<R (*)(Args...)>, typename ext_ret_carrier<R>::type> {
        using FuncT = R (*)(Args...);
        using Carrier = typename ext_ret_carrier<R>::type;
        using BaseT = SimNode_ExtFuncCallDirect<SimNode_ExtFuncCall<R (*)(Args...)>, Carrier>;
        enum { IS_CMRES = false };
        FuncT fn;
        SimNode_ExtFuncCall ( const LineInfo & at, const char * fnName, FuncT fnp )
            : BaseT(at,fnName), fn(fnp) { }
        __forceinline typename ext_carrier_value<Carrier>::type computeDirect ( Context & context ) {
            return ext_compute<R,Args...>(fn, context, this->arguments);
        }
    };


    template <typename FuncT, FuncT fn>
    struct SimNode_ExtFuncCallInline;

    template <typename R, typename ...Args, R (*fn)(Args...)>
    struct SimNode_ExtFuncCallInline<R (*)(Args...), fn>
        : SimNode_ExtFuncCallDirect<SimNode_ExtFuncCallInline<R (*)(Args...), fn>, typename ext_ret_carrier<R>::type> {
        using Carrier = typename ext_ret_carrier<R>::type;
        using BaseT = SimNode_ExtFuncCallDirect<SimNode_ExtFuncCallInline<R (*)(Args...), fn>, Carrier>;
        enum { IS_CMRES = false };
        SimNode_ExtFuncCallInline ( const LineInfo & at, const char * fnName )
            : BaseT(at,fnName) { }
        __forceinline typename ext_carrier_value<Carrier>::type computeDirect ( Context & context ) {
            return ext_compute<R,Args...>(fn, context, this->arguments);
        }
    };

    template <typename FuncT>
    struct SimNode_ExtFuncCallAndCopyOrMove;

    template <typename R, typename ...Args>
    struct SimNode_ExtFuncCallAndCopyOrMove<R (*)(Args...)> : SimNode_ExtFuncCallBase {
        using FuncT = R (*)(Args...);
        enum { IS_CMRES = true };
        FuncT fn;
        SimNode_ExtFuncCallAndCopyOrMove ( const LineInfo & at, const char * fnName, FuncT fnp )
            : SimNode_ExtFuncCallBase(at,fnName), fn(fnp) { }
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            void * cmres = cmresEval->evalPtr(context);
            ImplCallStaticFunctionAndCopy<FuncT>::call(fn, context, cmres, arguments);
            return cast<void *>::from(cmres);
        }
    };

    template <typename FuncT> struct result_of_func_ptr;
    template <typename R, typename ...Args>
    struct result_of_func_ptr<R (*)(Args...)> {
        using type = R;
    };

    template <typename CType, typename ...Args>
    struct SimNode_PlacementNew : SimNode_ExtFuncCallBase {
        SimNode_PlacementNew(const LineInfo & at, const char * fnName)
            : SimNode_ExtFuncCallBase(at,fnName) {}
        template <size_t ...I>
        static __forceinline void CallPlacementNew ( void * cmres, Context & ctx, SimNode ** args, index_sequence<I...> ) {
            (void)ctx;
            (void)args;
            (void)cmres;
            if constexpr (das::is_stub_type<CType>::value) { DAS_ASSERTF(false, "STUB!"); }
            else new (cmres) CType(cast_arg<Args>::to(ctx,args[I])...);
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            auto cmres = cmresEval->evalPtr(context);
            CallPlacementNew(cmres,context,arguments,make_index_sequence<sizeof...(Args)>());
            return cast<void *>::from(cmres);
        }
    };

    template <typename CType, typename ...Args>
    struct SimNode_Using : SimNode_ExtFuncCallBase {
        SimNode_Using(const LineInfo & at)
            : SimNode_ExtFuncCallBase(at,"using") {}
        template <size_t ...I>
        __forceinline void CallUsing ( const Block & blk, Context & ctx, SimNode ** args, index_sequence<I...> ) {
            (void)blk;
            (void)ctx;
            (void)args;
            if constexpr (!das::is_stub_type<CType>::value)
            {
              CType value((cast_arg<Args>::to(ctx, args[I]))...);
              vec4f bargs[1];
              bargs[0] = cast<CType *>::from(&value);
              ctx.invoke(blk, bargs, nullptr, &debugInfo);
            }
            else
            {
              DAS_ASSERTF(false, "STUB!");
            }
        }
        DAS_EVAL_ABI virtual vec4f eval(Context & context) override {
            DAS_ASSERT(nArguments == (sizeof...(Args) + 1) );
            auto pblock = cast_arg<const Block *>::to(context,arguments[nArguments-1]);
            CallUsing(*pblock,context,arguments,make_index_sequence<sizeof...(Args)>());
            return v_zero();
        }
    };

    template <typename FunctionType>
    struct ImplCallStaticFunctionRef;

    template <typename R, typename ...Args>
    struct ImplCallStaticFunctionRef < R (*)(Args...) > {
        static __forceinline char * call ( R (*fn)(Args...), Context & ctx, SimNode ** args ) {
            return (char *) & CallStaticFunction<R,Args...>(fn,ctx,args);
        }
    };

    template <typename FuncT>
    struct SimNode_ExtFuncCallRef;

    template <typename R, typename ...Args>
    struct SimNode_ExtFuncCallRef<R (*)(Args...)> : SimNode_ExtFuncCallBase {
        using FuncT = R (*)(Args...);
        DAS_PTR_NODE;
        enum { IS_CMRES = false };
        FuncT fn;
        SimNode_ExtFuncCallRef ( const LineInfo & at, const char * fnName, FuncT fnp )
            : SimNode_ExtFuncCallBase(at,fnName), fn(fnp) { }
        __forceinline char * compute(Context & context) {
            DAS_PROFILE_NODE
            return ImplCallStaticFunctionRef<FuncT>::call(fn, context, arguments);
        }
    };

    typedef vec4f ( InteropFunction ) ( Context & context, SimNode_CallBase * node, vec4f * args );

    template <InteropFunction fn>
    struct SimNode_InteropFuncCall : SimNode_ExtFuncCallBase {
        SimNode_InteropFuncCall ( const LineInfo & at, const char * fnName )
            : SimNode_ExtFuncCallBase(at,fnName) { }
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            vec4f * args = (vec4f *)(alloca(nArguments * sizeof(vec4f)));
            evalArgs(context, args);
            return fn(context,this,args);
        }
    };
}

#include "daScript/simulate/simulate_visit_op_undef.h"
