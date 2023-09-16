#pragma once

namespace das
{
    #define HASH_EMPTY64    0
    #define HASH_KILLED64    1

    // ideas from http://isthe.com/chongo/tech/comp/fnv/

    __forceinline uint64_t hash_block64(const uint8_t * block, size_t size) {
        const uint64_t fnv_prime = 1099511628211ul;
        uint64_t offset_basis = 14695981039346656037ul;
        for ( ; size >=4; size-=4 ) {
            offset_basis = ( offset_basis ^ *block++ ) * fnv_prime;
            offset_basis = ( offset_basis ^ *block++ ) * fnv_prime;
            offset_basis = ( offset_basis ^ *block++ ) * fnv_prime;
            offset_basis = ( offset_basis ^ *block++ ) * fnv_prime;
        }
        if (size & 2u) {
            offset_basis = (offset_basis ^ *block++) * fnv_prime;
            offset_basis = (offset_basis ^ *block++) * fnv_prime;
        }
        if (size & 1u) {
            offset_basis = (offset_basis ^ *block++) * fnv_prime;
        }
        if (offset_basis <= HASH_KILLED64) {
            return fnv_prime;
        }
        return offset_basis;
    }

    __forceinline uint64_t hash_blockz64(const uint8_t * block) {
        const uint64_t fnv_prime = 1099511628211ul;
        uint64_t fnv_bias = 14695981039346656037ul;
        uint64_t offset_basis = fnv_bias;
        for (; *block; block++) {
            offset_basis = ( offset_basis ^ *block ) * fnv_prime;
        }
        if (offset_basis <= HASH_KILLED64) {
            return fnv_prime;
        }
        return offset_basis;
    }

    class HashBlock {
        const uint64_t fnv_prime = 1099511628211ul;
        uint64_t offset_basis = 14695981039346656037ul;
    public:
        __forceinline void write ( const void * pb, size_t size ) {
            const uint8_t * block = (const uint8_t *) pb;
            while ( size-- ) {
                offset_basis = ( offset_basis ^ *block++ ) * fnv_prime;
            }
        }
        __forceinline void write ( const void * pb ) {
            const uint8_t * block = (const uint8_t *) pb;
            for (; *block; block++) {
                offset_basis = ( offset_basis ^ *block ) * fnv_prime;
            }
        }
        __forceinline uint64_t getHash() const  {
            return (offset_basis <= HASH_KILLED64) ? fnv_prime : offset_basis;
        }
    };
}

