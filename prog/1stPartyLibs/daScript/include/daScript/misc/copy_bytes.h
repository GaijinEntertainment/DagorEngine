#pragma once

namespace das {

    namespace bytes {
        struct bytes12 { char dummy[12]; };
        struct bytes16 { char dummy[16]; };
        struct bytes20 { char dummy[20]; };
        struct bytes24 { char dummy[24]; };
        struct bytes28 { char dummy[28]; };
        struct bytes32 { char dummy[32]; };
    }

    template <int typeSize>
    struct CopyBytes {
        static __forceinline void copy(char * dst, char * src) {
            memcpy(dst, src, typeSize);
        }
    };
    template<> struct CopyBytes<1>  { static __forceinline void copy(char * dst, char *src) { *dst = *src; } };
    template<> struct CopyBytes<2>  { static __forceinline void copy(char * dst, char *src) { *(int16_t *)dst = *(int16_t *)src; } };
    template<> struct CopyBytes<4>  { static __forceinline void copy(char * dst, char *src) { *(int32_t *)dst = *(int32_t *)src; } };
    template<> struct CopyBytes<8>  { static __forceinline void copy(char * dst, char *src) { *(int64_t *)dst = *(int64_t *)src; } };
    template<> struct CopyBytes<12> { static __forceinline void copy(char * dst, char *src) { *(bytes::bytes12 *)dst = *(bytes::bytes12 *)src; } };
    template<> struct CopyBytes<16> { static __forceinline void copy(char * dst, char *src) { *(bytes::bytes16 *)dst = *(bytes::bytes16 *)src; } };
    template<> struct CopyBytes<20> { static __forceinline void copy(char * dst, char *src) { *(bytes::bytes20 *)dst = *(bytes::bytes20 *)src; } };
    template<> struct CopyBytes<24> { static __forceinline void copy(char * dst, char *src) { *(bytes::bytes24 *)dst = *(bytes::bytes24 *)src; } };
    template<> struct CopyBytes<28> { static __forceinline void copy(char * dst, char *src) { *(bytes::bytes28 *)dst = *(bytes::bytes28 *)src; } };
    template<> struct CopyBytes<32> { static __forceinline void copy(char * dst, char *src) { *(bytes::bytes32 *)dst = *(bytes::bytes32 *)src; } };

    __forceinline bool isFastCopyBytes(int typeSize) {
        return (typeSize == 1) || (typeSize == 2) || ((typeSize > 0) && (typeSize <=32) && ((typeSize & 3) == 0));
    }
}
