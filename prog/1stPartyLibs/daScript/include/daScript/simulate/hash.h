#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/misc/fnv.h"

namespace das {

    class HashBuilder {
        const uint64_t fnv_prime = 1099511628211ul;
        uint64_t fnv_bias = 14695981039346656037ul;
    public:
        HashBuilder() { }

        uint64_t getHash() {
            if (fnv_bias <= HASH_KILLED64) {
                return fnv_prime;
            }
            return fnv_bias;
        }

        void writeStr(char *str) {
            if ( !str ) {
                fnv_bias *= fnv_prime;
                return;
            }
            uint8_t * block = (uint8_t *) str;
            while ( *block ) {
                fnv_bias = ( fnv_bias ^ *block++ ) * fnv_prime;
            }
        }
    };

    __forceinline uint64_t builtin_build_hash ( const TBlock<void,HashBuilder> & block, Context * context, LineInfoArg * at ) {
        HashBuilder writer;
        vec4f args[1];
        args[0] = cast<HashBuilder *>::from(&writer);
        context->invoke(block, args, nullptr, at);
        return writer.getHash();
    }

    template <typename TT>
    uint64_t builtin_build_hash_T ( TT && block, Context * context, LineInfoArg * ) {
        HashBuilder writer;
        block(writer);
        return writer.getHash();
    }

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
        return hash_block64((const uint8_t *)&x, sizeof(x));
    }

    template <>
    __forceinline uint64_t hash_function ( Context &, char * str ) {
        return str ? hash_blockz64((uint8_t *)str) : 1099511628211ul;
    }

    template <>
    __forceinline uint64_t hash_function ( Context &, const char * str ) {
        return str ? hash_blockz64((uint8_t *)str) : 1099511628211ul;
    }

    uint64_t hash_value ( Context & ctx, void * pX, TypeInfo * info );
    uint64_t hash_value ( Context & ctx, vec4f value, TypeInfo * info );
}

