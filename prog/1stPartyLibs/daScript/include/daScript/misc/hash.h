#pragma once
#include <stdint.h>

namespace das {

    // use FNV32 hash for tags. it's fast and good enough for our purposes
    constexpr uint32_t hash_tag(const char* block) {
        constexpr uint32_t FNV_offset_basis = 2166136261u;
        constexpr uint32_t FNV_prime = 16777619u;
        uint32_t h = FNV_offset_basis;
        while (*block) {
            h ^= uint8_t(*block++);
            h *= FNV_prime;
        }
        return h;
    }

    // use FNV32 hash for tags. it's fast and good enough for our purposes
    constexpr uint32_t hash_tag_file_name(const char* block) {
        constexpr uint32_t FNV_offset_basis = 2166136261u;
        constexpr uint32_t FNV_prime = 16777619u;
        uint32_t h = FNV_offset_basis;
        while (*block) {
            uint8_t C = *block++;
            if ( C=='/' || C=='\\' ) {
                h = FNV_offset_basis;
            } else {
                h ^= C;
                h *= FNV_prime;
            }
        }
        return h;
    }
}
