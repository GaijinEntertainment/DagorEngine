#pragma once

#include "daScript/simulate/simulate.h"

namespace das {

template <> struct WrapType<float2> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<float3> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<float4> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<int2> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<int3> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<int4> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<uint2> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<uint3> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<uint4> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<range> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<urange> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<range64> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };
template <> struct WrapType<urange64> { enum { value = true }; typedef vec4f type; typedef vec4f rettype; };

template <> struct WrapType<Func> { enum { value = true }; typedef void * type; typedef void * rettype; };
template <> struct WrapType<Lambda> { enum { value = true }; typedef void * type; typedef void * rettype; };

template <typename T>
struct WrapType<smart_ptr_raw<T>> { enum { value = true }; typedef smart_ptr_jit * type; typedef smart_ptr_jit * rettype; };

template <typename T>
struct WrapType<smart_ptr<T>> { enum { value = true }; typedef smart_ptr_jit * type; typedef smart_ptr_jit * rettype; };


template <typename... Ts> struct AnyVectorType;
template <> struct AnyVectorType<> { enum { value = false }; };
template <typename T> struct AnyVectorType<T> { enum { value = WrapType<T>::value }; };
template <typename Head, typename... Tail> struct AnyVectorType<Head, Tail...>
    { enum { value = WrapType<Head>::value || AnyVectorType<Tail...>::value }; };

template <typename TT> struct NeedVectorWrap;
template <typename Ret, typename ... Args> struct NeedVectorWrap< Ret(*)(Args...) > {
    enum {
        result = WrapType<Ret>::value,
        arguments = AnyVectorType<Args...>::value,
        value = result || arguments
    };
};

template <int CMRES, int wrap, typename FuncT, FuncT fn> struct ImplWrapCall;

template <typename FuncT, FuncT fn>     // no cmres, no wrap
struct ImplWrapCall<false,false,FuncT,fn> {
    static void * get_builtin_address() { return (void *) fn; }
};

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4191)
#endif

template <int wrap, typename RetT, typename ...Args, RetT(*fn)(Args...)>    // cmres
struct ImplWrapCall<true,wrap,RetT(*)(Args...),fn> {                        // when cmres, we always wrap
    static void static_call (typename remove_cv<RetT>::type * result, typename WrapType<Args>::type... args ) {
        typedef RetT (* FuncType)(typename WrapArgType<Args>::type...);
        auto fnPtr = reinterpret_cast<FuncType>(fn);
        new (result) RetT (fnPtr(args...));
    };
    static void * get_builtin_address() { return (void *) &static_call; }
};

template <typename RetT, typename ...Args, RetT(*fn)(Args...)>
struct ImplWrapCall<false,true,RetT(*)(Args...),fn> {   // no cmres, wrap
    static typename WrapType<RetT>::rettype static_call (typename WrapType<Args>::type... args ) {
        typedef typename WrapRetType<RetT>::type (* FuncType)(typename WrapArgType<Args>::type...);
        auto fnPtr = reinterpret_cast<FuncType>(fn);
        return static_cast<typename WrapType<RetT>::rettype>(fnPtr(args...));   // note explicit cast
    };
    static void * get_builtin_address() { return (void *) &static_call; }
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace detail {
    template <typename Fn, Fn fn>
    using TableWrap = ImplWrapCall<false, NeedVectorWrap<decltype(fn)>::value, Fn, fn>;
}

#define JIT_TABLE_FUNCTION(TAB_FUN) \
    switch ( baseType ) { \
        case Type::tBool:           return detail::TableWrap<decltype(TAB_FUN<bool>), TAB_FUN<bool>>::get_builtin_address(); \
        case Type::tInt8:           return detail::TableWrap<decltype(TAB_FUN<int8_t>), TAB_FUN<int8_t>>::get_builtin_address(); \
        case Type::tUInt8:          return detail::TableWrap<decltype(TAB_FUN<uint8_t>), TAB_FUN<uint8_t>>::get_builtin_address(); \
        case Type::tInt16:          return detail::TableWrap<decltype(TAB_FUN<int16_t>), TAB_FUN<int16_t>>::get_builtin_address(); \
        case Type::tUInt16:         return detail::TableWrap<decltype(TAB_FUN<uint16_t>), TAB_FUN<uint16_t>>::get_builtin_address(); \
        case Type::tInt64:          return detail::TableWrap<decltype(TAB_FUN<int64_t>), TAB_FUN<int64_t>>::get_builtin_address(); \
        case Type::tUInt64:         return detail::TableWrap<decltype(TAB_FUN<uint64_t>), TAB_FUN<uint64_t>>::get_builtin_address(); \
        case Type::tEnumeration:    return detail::TableWrap<decltype(TAB_FUN<int32_t>), TAB_FUN<int32_t>>::get_builtin_address(); \
        case Type::tEnumeration8:   return detail::TableWrap<decltype(TAB_FUN<int8_t>), TAB_FUN<int8_t>>::get_builtin_address(); \
        case Type::tEnumeration16:  return detail::TableWrap<decltype(TAB_FUN<int16_t>), TAB_FUN<int16_t>>::get_builtin_address(); \
        case Type::tEnumeration64:  return detail::TableWrap<decltype(TAB_FUN<int64_t>), TAB_FUN<int64_t>>::get_builtin_address(); \
        case Type::tInt:            return detail::TableWrap<decltype(TAB_FUN<int32_t>), TAB_FUN<int32_t>>::get_builtin_address(); \
        case Type::tInt2:           return detail::TableWrap<decltype(TAB_FUN<int2>), TAB_FUN<int2>>::get_builtin_address(); \
        case Type::tInt3:           return detail::TableWrap<decltype(TAB_FUN<int3>), TAB_FUN<int3>>::get_builtin_address(); \
        case Type::tInt4:           return detail::TableWrap<decltype(TAB_FUN<int4>), TAB_FUN<int4>>::get_builtin_address(); \
        case Type::tUInt:           return detail::TableWrap<decltype(TAB_FUN<uint32_t>), TAB_FUN<uint32_t>>::get_builtin_address(); \
        case Type::tBitfield:       return detail::TableWrap<decltype(TAB_FUN<uint32_t>), TAB_FUN<uint32_t>>::get_builtin_address(); \
        case Type::tUInt2:          return detail::TableWrap<decltype(TAB_FUN<uint2>), TAB_FUN<uint2>>::get_builtin_address(); \
        case Type::tUInt3:          return detail::TableWrap<decltype(TAB_FUN<uint3>), TAB_FUN<uint3>>::get_builtin_address(); \
        case Type::tUInt4:          return detail::TableWrap<decltype(TAB_FUN<uint4>), TAB_FUN<uint4>>::get_builtin_address(); \
        case Type::tFloat:          return detail::TableWrap<decltype(TAB_FUN<float>), TAB_FUN<float>>::get_builtin_address(); \
        case Type::tFloat2:         return detail::TableWrap<decltype(TAB_FUN<float2>), TAB_FUN<float2>>::get_builtin_address(); \
        case Type::tFloat3:         return detail::TableWrap<decltype(TAB_FUN<float3>), TAB_FUN<float3>>::get_builtin_address(); \
        case Type::tFloat4:         return detail::TableWrap<decltype(TAB_FUN<float4>), TAB_FUN<float4>>::get_builtin_address(); \
        case Type::tRange:          return detail::TableWrap<decltype(TAB_FUN<range>), TAB_FUN<range>>::get_builtin_address(); \
        case Type::tURange:         return detail::TableWrap<decltype(TAB_FUN<urange>), TAB_FUN<urange>>::get_builtin_address(); \
        case Type::tRange64:        return detail::TableWrap<decltype(TAB_FUN<range64>), TAB_FUN<range64>>::get_builtin_address(); \
        case Type::tURange64:       return detail::TableWrap<decltype(TAB_FUN<urange64>), TAB_FUN<urange64>>::get_builtin_address(); \
        case Type::tString:         return detail::TableWrap<decltype(TAB_FUN<char *>), TAB_FUN<char *>>::get_builtin_address(); \
        case Type::tDouble:         return detail::TableWrap<decltype(TAB_FUN<double>), TAB_FUN<double>>::get_builtin_address(); \
        case Type::tPointer:        return detail::TableWrap<decltype(TAB_FUN<void *>), TAB_FUN<void *>>::get_builtin_address(); \
        default:                    context->throw_error_at(at, "unsupported key type %s", das_to_string(Type(baseType)).c_str() ); \
    } \
    return nullptr;

}
