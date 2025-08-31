// © 2021 NVIDIA Corporation

#pragma once

struct bool2 {
    int32_t mask;

public:
    ML_INLINE bool2(int32_t m)
        : mask(m) {
    }

    ML_INLINE operator int2() const;
    ML_INLINE operator uint2() const;
    ML_INLINE operator float2() const;
    ML_INLINE operator double2() const;
};

struct bool3 {
    int32_t mask;

public:
    ML_INLINE bool3(int32_t m)
        : mask(m) {
    }

    ML_INLINE operator int3() const;
    ML_INLINE operator uint3() const;
    ML_INLINE operator float3() const;
    ML_INLINE operator double3() const;
};

struct bool4 {
    int32_t mask;

public:
    ML_INLINE bool4(int32_t m)
        : mask(m) {
    }

    ML_INLINE operator int4() const;
    ML_INLINE operator uint4() const;
    ML_INLINE operator float4() const;
    ML_INLINE operator double4() const;
};

ML_INLINE bool all(bool b) {
    return b;
}

ML_INLINE bool all(bool2 b) {
    return (b.mask & ML_Mask(1, 1, 0, 0)) == ML_Mask(1, 1, 0, 0);
}

ML_INLINE bool all(bool3 b) {
    return (b.mask & ML_Mask(1, 1, 1, 0)) == ML_Mask(1, 1, 1, 0);
}

ML_INLINE bool all(bool4 b) {
    return (b.mask & ML_Mask(1, 1, 1, 1)) == ML_Mask(1, 1, 1, 1);
}

ML_INLINE bool any(bool b) {
    return b;
}

ML_INLINE bool any(bool2 b) {
    return (b.mask & ML_Mask(1, 1, 0, 0)) != 0;
}

ML_INLINE bool any(bool3 b) {
    return (b.mask & ML_Mask(1, 1, 1, 0)) != 0;
}

ML_INLINE bool any(bool4 b) {
    return (b.mask & ML_Mask(1, 1, 1, 1)) != 0;
}
