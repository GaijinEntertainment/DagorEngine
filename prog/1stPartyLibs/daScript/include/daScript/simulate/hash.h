#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/misc/anyhash.h"

namespace das {
    __forceinline uint64_t hash_function ( Context &, const void * x, size_t size ) {
        return hash_block64((uint8_t *)x, size);
    }

    __forceinline uint32_t stringLength ( Context &, const char * str ) { // str!=nullptr
        return uint32_t(strlen(str));
    }

    __forceinline uint32_t stringLengthSafe ( Context & ctx, const char * str ) {//accepts nullptr
        return str ? stringLength(ctx,str) : 0;
    }

    template <typename TT>
    __forceinline uint64_t hash_function ( Context &, const TT x ) {
        return hash_block64((uint8_t *)&x, sizeof(x));
    }

    template <>
    __forceinline uint64_t hash_function ( Context &, char * str ) {
        return hash_blockz64((uint8_t *)str);
    }
    template <>
    __forceinline uint64_t hash_function ( Context &, const char * str ) {
        return hash_blockz64((uint8_t *)str);
    }
    template <>
    __forceinline uint64_t hash_function ( Context &, const string & str ) {
        return hash_blockz64((uint8_t *)str.c_str());
    }
    template <> __forceinline uint64_t hash_function ( Context &, bool value ) {
        return hash_uint32(value);
    }
    template <> __forceinline uint64_t hash_function ( Context &, int8_t value ) {
        return hash_uint32(value);
    }
    template <> __forceinline uint64_t hash_function ( Context &, uint8_t value ) {
        return hash_uint32(value);
    }
    template <> __forceinline uint64_t hash_function ( Context &, int16_t value ) {
        return hash_uint32(value);
    }
    template <> __forceinline uint64_t hash_function ( Context &, uint16_t value ) {
        return hash_uint32(value);
    }
    template <> __forceinline uint64_t hash_function ( Context &, int32_t value ) {
        return hash_uint32(value);
    }
    template <> __forceinline uint64_t hash_function ( Context &, uint32_t value ) {
        return hash_uint32(value);
    }
    template <> __forceinline uint64_t hash_function ( Context &, Bitfield value ) {
        return hash_uint32(value.value);
    }
    template <> __forceinline uint64_t hash_function ( Context &, float value ) {
        return hash_uint32(*((uint32_t *)&value));
    }
    template <> __forceinline uint64_t hash_function ( Context &, int64_t value ) {
        return hash_uint64(value);
    }
    template <> __forceinline uint64_t hash_function ( Context &, uint64_t value ) {
        return hash_uint64(value);
    }
    template <> __forceinline uint64_t hash_function ( Context &, double value ) {
        return hash_uint64(*((uint64_t *)&value));
    }
    template <> __forceinline uint64_t hash_function ( Context &, void * value ) {
        return hash_uint64(uint64_t(intptr_t(value)));
    }
    template <> __forceinline uint64_t hash_function ( Context &, range value ) {
        return hash_uint64(*((uint64_t *)&value));
    }
    template <> __forceinline uint64_t hash_function ( Context &, urange value ) {
        return hash_uint64(*((uint64_t *)&value));
    }
    uint64_t hash_value ( Context & ctx, void * pX, TypeInfo * info );
    uint64_t hash_value ( Context & ctx, vec4f value, TypeInfo * info );
}

