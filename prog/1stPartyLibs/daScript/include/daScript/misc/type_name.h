#pragma once

#include "daScript/misc/vectypes.h"
#include "daScript/misc/arraytype.h"
#include "daScript/misc/rangetype.h"

namespace das {

    template <typename TT>
    struct typeName;

    template <typename TT> struct typeName<const TT> : typeName<TT> {};

    template <typename TT, typename PP> struct typeName<smart_ptr<TT,PP>> {
        static string name() {
            return string("smart_ptr`") + typeName<TT>::name(); // TODO: compilation time concat
        }
    };

    template <typename FT, typename ST> struct typeName<pair<FT,ST>> {
        static string name() {
            return string("pair`") + typeName<FT>::name() + "`" + typeName<ST>::name(); // TODO: compilation time concat
        }
    };

    template <> struct typeName<int32_t>  { constexpr static const char * name() { return "int"; } };
    template <> struct typeName<uint32_t> { constexpr static const char * name() { return "uint"; } };
    template <> struct typeName<Bitfield> { constexpr static const char * name() { return "bitfield"; } };
    template <> struct typeName<int8_t>   { constexpr static const char * name() { return "int8"; } };
    template <> struct typeName<uint8_t>  { constexpr static const char * name() { return "uint8"; } };
    template <> struct typeName<int16_t>  { constexpr static const char * name() { return "int16"; } };
    template <> struct typeName<uint16_t> { constexpr static const char * name() { return "uint16"; } };
    template <> struct typeName<int64_t>  { constexpr static const char * name() { return "int64"; } };
    template <> struct typeName<uint64_t> { constexpr static const char * name() { return "uint64"; } };
    template <> struct typeName<bool>     { constexpr static const char * name() { return "bool"; } };
    template <> struct typeName<float>    { constexpr static const char * name() { return "float"; } };
    template <> struct typeName<double>   { constexpr static const char * name() { return "double"; } };
    template <> struct typeName<int2>     { constexpr static const char * name() { return "int2"; } };
    template <> struct typeName<int3>     { constexpr static const char * name() { return "int3"; } };
    template <> struct typeName<int4>     { constexpr static const char * name() { return "int4"; } };
    template <> struct typeName<uint2>    { constexpr static const char * name() { return "uint2"; } };
    template <> struct typeName<uint3>    { constexpr static const char * name() { return "uint3"; } };
    template <> struct typeName<uint4>    { constexpr static const char * name() { return "uint4"; } };
    template <> struct typeName<float2>   { constexpr static const char * name() { return "float2"; } };
    template <> struct typeName<float3>   { constexpr static const char * name() { return "float3"; } };
    template <> struct typeName<float4>   { constexpr static const char * name() { return "float4"; } };
    template <> struct typeName<range>    { constexpr static const char * name() { return "range"; } };
    template <> struct typeName<urange>   { constexpr static const char * name() { return "urange"; } };
    template <> struct typeName<range64>  { constexpr static const char * name() { return "range64"; } };
    template <> struct typeName<urange64> { constexpr static const char * name() { return "urange64"; } };
    template <> struct typeName<char *>   { constexpr static const char * name() { return "string"; } };
    template <> struct typeName<const char *> { constexpr static const char * name() { return "string"; } };
    template <> struct typeName<void *>   { constexpr static const char * name() { return "pointer"; } };
    template <> struct typeName<Func>     { constexpr static const char * name() { return "Func"; } };
    template <> struct typeName<Lambda>   { constexpr static const char * name() { return "Lambda"; } };
    template <> struct typeName<Block>    { constexpr static const char * name() { return "Block"; } };
    template <> struct typeName<Tuple>    { constexpr static const char * name() { return "Tuple"; } };
    template <> struct typeName<Variant>  { constexpr static const char * name() { return "Variant"; } };
}
