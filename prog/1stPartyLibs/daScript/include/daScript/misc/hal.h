#pragma once

#include <vecmath/dag_vecMath.h>

__forceinline void * v_extract_ptr(vec4i a) {
#if INTPTR_MAX == INT32_MAX
    return (void*)v_extract_xi(a);
#else
    return (void*)v_extract_xi64(a);
#endif

}

#if defined(_MSC_VER) && !defined(__clang__) && INTPTR_MAX == INT32_MAX//MSVC generates flawed code, for example in _builtin_binary_save, so make it out of line
extern vec4i VECTORCALL v_ldu_ptr(const void * a);
#else
__forceinline vec4i VECTORCALL v_ldu_ptr(const void * a) {
#if INTPTR_MAX == INT32_MAX
    return v_seti_x((int32_t)a);
#else
    return v_ldui_half(&a);
#endif
}
#endif

__forceinline vec4i v_divi2(vec4i a, vec4i b) {
    union {
        vec4i       v;
        int32_t     a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] / B.a[0];   C.a[1] = A.a[1] / B.a[1];
    return C.v;
}

__forceinline vec4i v_modi2(vec4i a, vec4i b) {
    union {
        vec4i       v;
        int32_t     a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] % B.a[0];   C.a[1] = A.a[1] % B.a[1];
    return C.v;
}

__forceinline vec4i v_divi3(vec4i a, vec4i b) {
    union {
        vec4i       v;
        int32_t     a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] / B.a[0];   C.a[1] = A.a[1] / B.a[1];   C.a[2] = A.a[2] / B.a[2];
    return C.v;
}

__forceinline vec4i v_modi3(vec4i a, vec4i b) {
    union {
        vec4i       v;
        int32_t     a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] % B.a[0];   C.a[1] = A.a[1] % B.a[1];   C.a[2] = A.a[2] % B.a[2];
    return C.v;
}


__forceinline vec4i v_divi4(vec4i a, vec4i b) {
    union {
        vec4i       v;
        int32_t     a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] / B.a[0];   C.a[1] = A.a[1] / B.a[1];   C.a[2] = A.a[2] / B.a[2];   C.a[3] = A.a[3] / B.a[3];
    return C.v;
}

__forceinline vec4i v_modi4(vec4i a, vec4i b) {
    union {
        vec4i       v;
        int32_t     a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] % B.a[0];   C.a[1] = A.a[1] % B.a[1];   C.a[2] = A.a[2] % B.a[2];   C.a[3] = A.a[3] % B.a[3];
    return C.v;
}

__forceinline vec4i v_mulu(vec4i a, vec4i b) {
    union {
        vec4i       v;
        uint32_t    a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] * B.a[0];   C.a[1] = A.a[1] * B.a[1];   C.a[2] = A.a[2] * B.a[2];   C.a[3] = A.a[3] * B.a[3];
    return C.v;
}

__forceinline vec4i v_divu2(vec4i a, vec4i b) {
    union {
        vec4i       v;
        uint32_t    a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] / B.a[0];   C.a[1] = A.a[1] / B.a[1];
    return C.v;
}

__forceinline vec4i v_modu2(vec4i a, vec4i b) {
    union {
        vec4i       v;
        uint32_t    a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] % B.a[0];   C.a[1] = A.a[1] % B.a[1];
    return C.v;
}

__forceinline vec4i v_divu3(vec4i a, vec4i b) {
    union {
        vec4i       v;
        uint32_t    a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] / B.a[0];   C.a[1] = A.a[1] / B.a[1];   C.a[2] = A.a[2] / B.a[2];
    return C.v;
}

__forceinline vec4i v_modu3(vec4i a, vec4i b) {
    union {
        vec4i       v;
        uint32_t    a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] % B.a[0];   C.a[1] = A.a[1] % B.a[1];   C.a[2] = A.a[2] % B.a[2];
    return C.v;
}

__forceinline vec4i v_divu4(vec4i a, vec4i b) {
    union {
        vec4i       v;
        uint32_t    a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] / B.a[0];   C.a[1] = A.a[1] / B.a[1];   C.a[2] = A.a[2] / B.a[2];   C.a[3] = A.a[3] / B.a[3];
    return C.v;
}

__forceinline vec4i v_modu4(vec4i a, vec4i b) {
    union {
        vec4i       v;
        uint32_t    a[4];
    } A, B, C;
    A.v = a;
    B.v = b;
    C.a[0] = A.a[0] % B.a[0];   C.a[1] = A.a[1] % B.a[1];   C.a[2] = A.a[2] % B.a[2];   C.a[3] = A.a[3] % B.a[3];
    return C.v;
}
