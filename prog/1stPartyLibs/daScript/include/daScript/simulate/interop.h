#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/simulate_visit_op.h"

#ifndef DAS_INTEROP_DETAILS
// enabling it requires removing -fno-rtti from CMakeCommon.txt and such
#define DAS_INTEROP_DETAILS 0
#endif

namespace das
{
    template <typename TT>
    struct cast_arg {
        static __forceinline TT to ( Context & ctx, SimNode * x ) {
            return EvalTT<TT>::eval(ctx,x);
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

    template <typename FunctionType>
    struct ImplCallStaticFunction;

    template <typename R, typename ...Args>
    struct ImplCallStaticFunction<R (*)(Args...)> {
        static _msc_inline_bug vec4f call( R (*fn)(Args...), Context & ctx, SimNode ** args ) {
            // The function is attempting to return result by value,
            // which type is not compatible with the current binding (missing cast<>).
            // To bind functions that return non-vec4f types by value on the stack,
            // you need to use the SimNode_ExtFuncCallAndCopyOrMove template argument
            // to copy or move the returned value.
            return cast_res<R>::from(CallStaticFunction<R,Args...>(fn,ctx,args),&ctx);
        }
    };

    template <typename R, typename ...Args>
    struct ImplCallStaticFunction<smart_ptr<R> (*)(Args...)> {
        static __forceinline vec4f call ( smart_ptr<R> (*fn)(Args...), Context & ctx, SimNode ** args ) {
            return cast<R *>::from(CallStaticFunction<smart_ptr<R>,Args...>(fn,ctx,args).orphan());
        }
    };

    template <typename R, typename ...Args>
    struct ImplCallStaticFunction<smart_ptr_raw<R> (*)(Args...)> {
        static __forceinline vec4f call ( smart_ptr_raw<R> (*fn)(Args...), Context & ctx, SimNode ** args ) {
            return cast<R *>::from(CallStaticFunction<smart_ptr_raw<R>,Args...>(fn,ctx,args).get());
        }
    };

    template <typename ...Args>
    struct ImplCallStaticFunction<void (*)(Args...)> {
        static __forceinline vec4f call ( void (*fn)(Args...), Context & ctx, SimNode ** args ) {
            CallStaticFunction<void,Args...>(fn,ctx,args);
            return v_zero();
        }
    };

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

    template <typename CType, bool Pointer, bool IsEnum, typename Result, typename ...Args>
    struct ImplCallStaticFunctionImpl {
        static __forceinline CType call( Result (*fn)(Args...), Context & context, SimNode ** args ) {
            using WrapResult = typename WrapType<Result>::rettype;
            if constexpr ( !is_workhorse_type<Result>::value && is_same<WrapResult,CType>::value) {
                // if we match a WrapType, we can call it directly (with just the cast)
                return static_cast<CType>(CallStaticFunction<Result,Args...>(fn,context,args));
            } else if constexpr ( !is_workhorse_type<Result>::value && is_same<WrapResult,Result>::value ) {
                // if the WrapType is the same as Result, we are missing WrapType implementation, or its not included
                #if DAS_INTEROP_DETAILS
                    context.throw_error_ex("internal integration error, missing WrapType implementation %s or it's not included",
                        typeid(WrapResult).name());
                #else
                    context.throw_error("internal integration error, missing WrapType implementation or it's not included");
                #endif
                return CType();
            } else if constexpr ( !is_workhorse_type<Result>::value ) {
                // we should never be here, since we are asking for a WrapResult which is not the same as CType
                #if DAS_INTEROP_DETAILS
                    context.throw_error_ex("internal integration error. WrapType %s is not the same as CType %s",
                                        typeid(WrapResult).name(), typeid(CType).name());
                #else
                    context.throw_error("internal integration error, WrapType is not the same as CType");
                #endif
                return CType();
            } else {
                // this is workhorse <-> workhorse cross-pollination. somehow. like wrong node implementation or something
                #if DAS_INTEROP_DETAILS
                    context.throw_error_ex("internal integration error. %s <-> %s workhorse cross-pollination",
                                            typeid(WrapResult).name(), typeid(CType).name());
                #else
                    context.throw_error("internal integration error, workhorse cross-pollination");
                #endif
                return CType();
            }
        }
    };

    template <typename CType, typename Result, typename ...Args>   // smart_ptr
    struct ImplCallStaticFunctionImpl<CType, true, false, smart_ptr<Result>, Args...> {
        static __forceinline CType call ( smart_ptr<Result> (*fn)(Args...), Context & ctx, SimNode ** args ) {
            return (CType) CallStaticFunction<smart_ptr<Result>,Args...>(fn,ctx,args).orphan();
        }
    };

    template <typename CType, typename Result, typename ...Args>   // smart_ptr_raw
    struct ImplCallStaticFunctionImpl<CType, true, false, smart_ptr_raw<Result>, Args...> {
        static __forceinline CType call ( smart_ptr_raw<Result> (*fn)(Args...), Context & ctx, SimNode ** args ) {
            return (CType) CallStaticFunction<smart_ptr_raw<Result>,Args...>(fn,ctx,args).get();
        }
    };

    template <typename CType, typename Result, typename ...Args>   // any pointer
    struct ImplCallStaticFunctionImpl<CType, true, false, Result, Args...> {
        static __forceinline CType call ( Result (*fn)(Args...), Context & ctx, SimNode ** args) {
            return (CType) CallStaticFunction<Result,Args...>(fn,ctx,args);
        }
    };


    template <typename CType, typename Result, typename ...Args>   // any enum
    struct ImplCallStaticFunctionImpl<CType, false, true, Result, Args...> {
        static __forceinline CType call ( Result (*fn)(Args...), Context & ctx, SimNode ** args ) {
            return (CType) CallStaticFunction<Result,Args...>(fn,ctx,args);
        }
    };

    template <typename Result, typename ...Args>
    struct ImplCallStaticFunctionImpl<Result, false, false, Result, Args...> {   // no cast
        static __forceinline Result call ( Result (*fn)(Args...), Context & ctx, SimNode ** args ) {
            return CallStaticFunction<Result,Args...>(fn,ctx,args);;
        }
    };

    // note: this is here because SimNode_At and such can call evalInt, while index is UInt
    //  this is going to be allowed for now, since the fix will result either in duplicating SimNode_AtU or a cast node
    template <typename ...Args>
    struct ImplCallStaticFunctionImpl<int32_t, false, false, uint32_t, Args...> {   // int <- uint
        static __forceinline int32_t call ( uint32_t (*fn)(Args...), Context & ctx, SimNode ** args ) {
            return CallStaticFunction<uint32_t,Args...>(fn,ctx,args);;
        }
    };

    // note: those two are here because int64_t and uint64_t types are long long on windows and long on e.g. linux
    // so any long long return type is not handled correctly on platforms where int64 is long
    template <typename ...Args>
    struct ImplCallStaticFunctionImpl<int64_t, false, false, long long, Args...> {
        static __forceinline int64_t call ( long long (*fn)(Args...), Context & ctx, SimNode ** args ) {
            return (int64_t) CallStaticFunction<long long, Args...>(fn,ctx,args);;
        }
    };

    template <typename ...Args>
    struct ImplCallStaticFunctionImpl<uint64_t, false, false, unsigned long long, Args...> {
        static __forceinline uint64_t call ( unsigned long long (*fn)(Args...), Context & ctx, SimNode ** args ) {
            return (uint64_t) CallStaticFunction<unsigned long long, Args...>(fn,ctx,args);;
        }
    };

    template <bool Pointer, bool IsEnum, typename ...Args> // void
    struct ImplCallStaticFunctionImpl<void,Pointer,IsEnum,void,Args...> {
        static __forceinline void call ( void (*fn)(Args...), Context & ctx, SimNode ** args ) {
            CallStaticFunction<void,Args...>(fn,ctx,args);
        }
    };

    template <typename FuncType, typename CType>
    struct ImplCallStaticFunctionImm;

    template <typename R, typename ...Args, typename CType>
    struct ImplCallStaticFunctionImm<R (*)(Args...),CType>
        : ImplCallStaticFunctionImpl<
            CType,
            is_any_pointer<R>::value && is_any_pointer<CType>::value,
            is_enum<R>::value,
            R,
            Args...> {
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

#if DAS_SLOW_CALL_INTEROP
    template <typename FuncT>
#else
    template <typename FuncT, FuncT fn>
#endif
    struct SimNode_ExtFuncCall : SimNode_ExtFuncCallBase {
        enum { IS_CMRES = false };
#if DAS_SLOW_CALL_INTEROP
        FuncT  fn;
        SimNode_ExtFuncCall ( const LineInfo & at, const char * fnName, FuncT fnp )
            : SimNode_ExtFuncCallBase(at,fnName), fn(fnp) { }
#else
        SimNode_ExtFuncCall ( const LineInfo & at, const char * fnName )
            : SimNode_ExtFuncCallBase(at,fnName) { }
#endif
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            return ImplCallStaticFunction<FuncT>::call(*fn, context, arguments);
        }
#if !(DAS_SLOW_CALL_INTEROP)
#define EVAL_NODE(TYPE,CTYPE)\
        virtual CTYPE eval##TYPE ( Context & context ) override { \
                DAS_PROFILE_NODE \
                return ImplCallStaticFunctionImm<FuncT,CTYPE>::call(*fn, context, arguments); \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
#endif
    };

#if DAS_SLOW_CALL_INTEROP
    template <typename FuncT>
#else
    template <typename FuncT, FuncT fn>
#endif
    struct SimNode_ExtFuncCallAndCopyOrMove : SimNode_ExtFuncCallBase {
        enum { IS_CMRES = true };
#if DAS_SLOW_CALL_INTEROP
        FuncT  fn;
        SimNode_ExtFuncCallAndCopyOrMove ( const LineInfo & at, const char * fnName, FuncT fnp )
            : SimNode_ExtFuncCallBase(at,fnName), fn(fnp) { }
#else
        SimNode_ExtFuncCallAndCopyOrMove ( const LineInfo & at, const char * fnName )
            : SimNode_ExtFuncCallBase(at,fnName) { }
#endif
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            void * cmres = cmresEval->evalPtr(context);
            ImplCallStaticFunctionAndCopy<FuncT>::call(*fn, context, cmres, arguments);
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

#if DAS_SLOW_CALL_INTEROP
    template <typename FuncT>
#else
    template <typename FuncT, FuncT fn>
#endif
    struct SimNode_ExtFuncCallRef : SimNode_ExtFuncCallBase {
        DAS_PTR_NODE;
        enum { IS_CMRES = false };
#if DAS_SLOW_CALL_INTEROP
        FuncT  fn;
        SimNode_ExtFuncCallRef ( const LineInfo & at, const char * fnName, FuncT fnp )
            : SimNode_ExtFuncCallBase(at,fnName), fn(fnp) { }
#else
        SimNode_ExtFuncCallRef ( const LineInfo & at, const char * fnName )
            : SimNode_ExtFuncCallBase(at,fnName) { }
#endif
        __forceinline char * compute(Context & context) {
            DAS_PROFILE_NODE
            return ImplCallStaticFunctionRef<FuncT>::call(*fn, context, arguments);
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


