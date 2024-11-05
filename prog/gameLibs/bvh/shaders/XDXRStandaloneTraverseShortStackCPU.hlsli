
// Copyright (c) Microsoft. All rights reserved.


#ifndef XDXR_TRAVERSAL_WAVE_SIZE
    #define XDXR_TRAVERSAL_WAVE_SIZE 32
#endif  // XDXR_TRAVERSAL_WAVE_SIZE

// Avoid a conflict by enabling WAVE32 only when traversal wave size is 32.
#ifdef __XBOX_ENABLE_WAVE32
    #undef __XBOX_ENABLE_WAVE32
#endif

// Wave size must be equal to thread group size. This shader uses WaveGetLaneIndex() in lieu of the title providing SV_GroupThreadIndex.
#if XDXR_TRAVERSAL_WAVE_SIZE == 64
    #define __XBOX_ENABLE_WAVE32 0
    #define __XBOX_REQUIRED_THREADGROUP_SIZE 64
#elif XDXR_TRAVERSAL_WAVE_SIZE == 32
    #define __XBOX_ENABLE_WAVE32 1
    #define __XBOX_REQUIRED_THREADGROUP_SIZE 32
#else
    #error XDXR_TRAVERSAL_WAVE_SIZE should be 32 or 64
#endif

#ifdef XDXR_USE_INLINE_AHS_FUNCTION
#warning XDXR_USE_INLINE_AHS_FUNCTION is deprecated and has been replaced with XboxRayQuery<Flags, InlineAHSLambda>. HLSL 2021 is required (dxc.exe -HV 2021).
#endif

#ifdef XDXR_USE_INLINE_IS_FUNCTION
#warning XDXR_USE_INLINE_IS_FUNCTION is deprecated and has been replaced with XboxRayQuery<Flags, InlineISLambda>. HLSL 2021 is required (dxc.exe -HV 2021).
#endif

#if !defined(__HLSL_VERSION) || __HLSL_VERSION >= 2021
#define XDXR_USE_HLSL_2021 1
#else
#define XDXR_USE_HLSL_2021 0
#endif

#define __XBOX_INLINE_SOURCE 1
#define __XBOX_INLINE_SOURCE 1





#define __XBOX_INLINE_SOURCE 1
#define __XBOX_INLINE_SOURCE 1




#define __XBOX_SCARLETT 1

namespace RootSignatureParameterOffset
{
    enum
    {
        PaddingForResourceHeapsPtrs = 0,
        DispatchConstants = 1,

        NumParameters
    };
}

typedef int32_t XdxrStateId_t;


enum {
    XDXR_STATE_ID_TO_POINTER_SHIFT_AMOUNT = 8,
    XDXR_STATE_ID_SIZE_IN_DWORDS = sizeof(XdxrStateId_t) / sizeof(uint32_t),
};




template<typename T>
T SwizzleYZX(
    const T& v)
{
    T t {};
    t.x = v.y;
    t.y = v.z;
    t.z = v.x;
    return t;
}

template<typename T>
T SwizzleZXY(
    const T& v)
{
    T t {};
    t.x = v.z;
    t.y = v.x;
    t.z = v.y;
    return t;
}



__declspec(selectany)
uint64_t g_sharedPointerBias = 0;

struct float2
{
    float x, y;

    float2() = default;

    float2(
        float v) :
            x(v), y(v)
    {
    }

    float2(
        const float2&) = default;

    float2& operator = (
        const float2&) = default;

    float2(
        float a,
        float b) :
            x(a),
            y(b)
    {
    }
};


struct float3
{
    float x, y, z;

    float3() = default;

    float3(
        float v) :
            x(v), y(v), z(v)
    {
    }

    float3(
        float a,
        float b,
        float c) :
            x(a), y(b), z(c)
    {
    }

    float3(
        const float3&) = default;

    float3& operator = (
        const float3&) = default;

    float& operator [] (
        unsigned idx)
    {
        return ((float*)this)[idx];
    }

    float operator [] (
        unsigned idx) const
    {
        return ((float*)this)[idx];
    }

    float3 operator- () const
    {
        return float3( -x, -y, -z );
    }

    float3& operator += (
        const float3& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;

        return *this;
    }

    float3& operator -= (
        const float3& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;

        return *this;
    }

    float3& operator *= (
        const float3& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;

        return *this;
    }

    float3& operator /= (
        const float3& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;

        return *this;
    }
};

struct float4
{
    float x, y, z, w;

    float4() = default;

    float4(
        float v) :
            x(v), y(v), z(v), w(v)
    {
    }

    float4(
        float a,
        float b,
        float c,
        float d) :
        x(a), y(b), z(c), w(d)
    {
    }

    float4(
        const float3& xyz,
        float w) :
        x(xyz.x), y(xyz.y), z(xyz.z), w(w)
    {
    }

    float4(
        const float4&) = default;

    float4& operator = (
        const float4&) = default;

    float& operator [] (
        unsigned idx)
    {
        return ((float*)this)[idx];
    }

    float operator [] (
        unsigned idx) const
    {
        return ((float*)this)[idx];
    }

    float4 operator- () const
    {
        return float4( -x, -y, -z, -w );
    }

    float4& operator += (
        const float4& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;

        return *this;
    }

    float4& operator -= (
        const float4& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;

        return *this;
    }

    float4& operator *= (
        const float4& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        w *= rhs.w;

        return *this;
    }

    float4& operator /= (
        const float4& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        w /= rhs.w;

        return *this;
    }
};

inline
float2 operator + (const float2& a, const float2& b)
{
    return float2(a.x + b.x, a.y + b.y);
}

inline
float2 operator - (const float2& a, const float2& b)
{
    return float2(a.x - b.x, a.y - b.y);
}

inline
float2 operator * (const float2& a, const float2& b)
{
    return float2(a.x * b.x, a.y * b.y);
}

inline
float2 operator / (const float2& a, const float2& b)
{
    return float2(a.x / b.x, a.y / b.y);
}

inline
float3 operator + (const float3& a, const float3& b)
{
    return float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline
float3 operator - (const float3& a, const float3& b)
{
    return float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline
float3 operator * (const float3& a, const float3& b)
{
    return float3(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline
float3 operator / (const float3& a, const float3& b)
{
    return float3(a.x / b.x, a.y / b.y, a.z / b.z);
}

inline
float4 operator + (const float4& a, const float4& b)
{
    return float4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

inline
float4 operator - (const float4& a, const float4& b)
{
    return float4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

inline
float4 operator * (const float4& a, const float4& b)
{
    return float4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

inline
float4 operator / (const float4& a, const float4& b)
{
    return float4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}



struct float4x4
{
    union
    {
        float mat[16];
        float4 rows[4];
    };

    float4x4() = default;

    float4x4(
        const float4x4&) = default;

    float4x4(
        const float4& r0,
        const float4& r1,
        const float4& r2,
        const float4& r3)
    {
        rows[0] = r0;
        rows[1] = r1;
        rows[2] = r2;
        rows[3] = r3;
    }

    float4x4(
        float r0c0, float r0c1, float r0c2, float r0c3,
        float r1c0, float r1c1, float r1c2, float r1c3,
        float r2c0, float r2c1, float r2c2, float r2c3,
        float r3c0, float r3c1, float r3c2, float r3c3)
    {
        rows[0] = float4(r0c0, r0c1, r0c2, r0c3);
        rows[1] = float4(r1c0, r1c1, r1c2, r1c3);
        rows[2] = float4(r2c0, r2c1, r2c2, r2c3);
        rows[3] = float4(r3c0, r3c1, r3c2, r3c3);
    }

    float4x4 operator * (
        const float rhs) const
    {
        return {
            rows[0] * rhs,
            rows[1] * rhs,
            rows[2] * rhs,
            rows[3] * rhs
        };
    }

    float4& operator [](
        unsigned idx)
    {
        return rows[idx];
    }

    const float4& operator [](
        unsigned idx) const
    {
        return rows[idx];
    }
};


struct float2x2
{
    union
    {
        float mat[4];
        float2 rows[2];
    };

    float2x2() = default;

    float2x2(
        const float2x2&) = default;

    float2x2(
        const float2& r0,
        const float2& r1)
    {
        rows[0] = r0;
        rows[1] = r1;
    }

    float2x2(
        float r0c0, float r0c1,
        float r1c0, float r1c1)
    {
        rows[0] = float2(r0c0, r0c1);
        rows[1] = float2(r1c0, r1c1);
    }

    float2x2 operator * (
        const float rhs) const
    {
        return {
            rows[0] * rhs,
            rows[1] * rhs
        };
    }

    float2& operator [](
        unsigned idx)
    {
        return rows[idx];
    }

    const float2& operator [](
        unsigned idx) const
    {
        return rows[idx];
    }
};


struct float3x3
{
    union
    {
        float mat[9];
        float3 rows[3];
    };

    float3x3() = default;

    float3x3(
        const float3x3&) = default;

    float3x3(
        const float3& r0,
        const float3& r1,
        const float3& r2)
    {
        rows[0] = r0;
        rows[1] = r1;
        rows[2] = r2;
    }

    float3x3(
        float r0c0, float r0c1, float r0c2,
        float r1c0, float r1c1, float r1c2,
        float r2c0, float r2c1, float r2c2)
    {
        rows[0] = float3(r0c0, r0c1, r0c2);
        rows[1] = float3(r1c0, r1c1, r1c2);
        rows[2] = float3(r2c0, r2c1, r2c2);
    }

    float3x3 operator * (
        const float rhs) const
    {
        return {
            rows[0] * rhs,
            rows[1] * rhs,
            rows[2] * rhs
        };
    }

    float3& operator [](
        unsigned idx)
    {
        return rows[idx];
    }

    const float3& operator [](
        unsigned idx) const
    {
        return rows[idx];
    }
};



struct float3x4
{
    union
    {
        float mat[12];
        float4 rows[3];
    };

    float3x4() = default;

    float3x4(
        const float3x4&) = default;

    float3x4(
        const float4& r0,
        const float4& r1,
        const float4& r2)
    {
        rows[0] = r0;
        rows[1] = r1;
        rows[2] = r2;
    }

    float4& operator [](
        unsigned idx)
    {
        return rows[idx];
    }

    const float4& operator [](
        unsigned idx) const
    {
        return rows[idx];
    }

    explicit operator float3x3() const
    {
        float3x3 m;

        m.rows[0] = float3(rows[0].x, rows[0].y, rows[0].z);
        m.rows[1] = float3(rows[1].x, rows[1].y, rows[1].z);
        m.rows[2] = float3(rows[2].x, rows[2].y, rows[2].z);

        return m;
    }
};

struct float4x3
{
    union
    {
        float mat[12];
        float3 rows[4];
    };

    float4x3() = default;

    float4x3(
        const float4x3&) = default;

    float4x3(
        const float3& r0,
        const float3& r1,
        const float3& r2,
        const float3& r3)
    {
        rows[0] = r0;
        rows[1] = r1;
        rows[2] = r2;
        rows[3] = r3;
    }

    float3& operator [](
        unsigned idx)
    {
        return rows[idx];
    }

    const float3& operator [](
        unsigned idx) const
    {
        return rows[idx];
    }
};

inline
float3x3 transpose(const float3x3& mat)
{
    return float3x3(
        float3(mat[0][0], mat[1][0], mat[2][0]),
        float3(mat[0][1], mat[1][1], mat[2][1]),
        float3(mat[0][2], mat[1][2], mat[2][2])
    );
}

inline
float4x3 transpose(const float3x4& mat)
{
    return float4x3(
        float3(mat[0][0], mat[1][0], mat[2][0]),
        float3(mat[0][1], mat[1][1], mat[2][1]),
        float3(mat[0][2], mat[1][2], mat[2][2]),
        float3(mat[0][3], mat[1][3], mat[2][3])
    );
}

inline
float3x4 transpose(const float4x3& mat)
{
    return float3x4(
        float4(mat[0][0], mat[1][0], mat[2][0], mat[3][0]),
        float4(mat[0][1], mat[1][1], mat[2][1], mat[3][1]),
        float4(mat[0][2], mat[1][2], mat[2][2], mat[3][2])
    );
}

inline
float4x4 transpose(const float4x4& mat)
{
    return float4x4(
        float4(mat[0][0], mat[1][0], mat[2][0], mat[3][0]),
        float4(mat[0][1], mat[1][1], mat[2][1], mat[3][1]),
        float4(mat[0][2], mat[1][2], mat[2][2], mat[3][2]),
        float4(mat[0][3], mat[1][3], mat[2][3], mat[3][3])
    );
}

typedef UINT uint;

struct uint2
{
    uint x, y;

    uint2()
        = default;

    uint2(
        uint v) :
        x(v),
        y(v)
    {
    }

    uint2(
        uint a,
        uint b) :
        x(a),
        y(b)
    {
    }

    uint& operator[](
        size_t idx)
    {
        return ((uint*)this)[idx];
    }
};

struct int2
{
    int x, y;

    int2()
        = default;

    int2(
        uint v) :
        x(v),
        y(v)
    {
    }

    int2(
        uint a,
        uint b) :
        x(a),
        y(b)
    {
    }

    int& operator[](
        size_t idx)
    {
        return ((int*)this)[idx];
    }
};

struct uint3
{
    uint x, y, z;

    uint3() = default;

    uint3(const uint3&) = default;

    uint3(uint a) :
        x(a),
        y(a),
        z(a)
    {}

    uint3(uint a, uint b, uint c) :
        x(a),
        y(b),
        z(c)
    {}

    explicit uint3(const float3& f) :
        x((uint)f.x),
        y((uint)f.y),
        z((uint)f.z)
    {
    }

    uint3 operator +(const uint3& rhs)
    {
        return uint3
        (
            x + rhs.x,
            y + rhs.y,
            z + rhs.z
        );
    }

    uint3 operator -(const uint3& rhs)
    {
        return uint3
        (
            x - rhs.x,
            y - rhs.y,
            z - rhs.z
        );
    }

    uint3 operator &(const uint3& rhs)
    {
        return uint3
        (
            x & rhs.x,
            y & rhs.y,
            z & rhs.z
        );
    }

    uint3 operator <<(const uint3& rhs)
    {
        return uint3
        (
            x << rhs.x,
            y << rhs.y,
            z << rhs.z
        );
    }

    uint3 operator >>(const uint3& rhs)
    {
        return uint3
        (
            x >> rhs.x,
            y >> rhs.y,
            z >> rhs.z
        );
    }

    uint3& operator -=(uint rhs)
    {
        x -= rhs;
        y -= rhs;
        z -= rhs;

        return *this;
    }

    uint3& operator -=(const uint3& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;

        return *this;
    }

    uint3& operator >>=(const uint3& rhs)
    {
        x >>= rhs.x;
        y >>= rhs.y;
        z >>= rhs.z;

        return *this;
    }

    uint3& operator |=(const uint3& rhs)
    {
        x |= rhs.x;
        y |= rhs.y;
        z |= rhs.z;

        return *this;
    }

    uint3& operator = (const uint3&) = default;

    uint& operator[](size_t idx)
    {
        return ((uint*)this)[idx];
    }

    uint operator[](size_t idx) const
    {
        return ((uint*)this)[idx];
    }
};

struct int3
{
    int x, y, z;

    int3() = default;

    int3(const int3&) = default;

    int3(int a) :
        x(a),
        y(a),
        z(a)
    {}

    int3(int a, int b, int c) :
        x(a),
        y(b),
        z(c)
    {}

    int3(const uint3& f) :
        x((int)f.x),
        y((int)f.y),
        z((int)f.z)
    {
    }

    explicit int3(const float3& f) :
        x((int)f.x),
        y((int)f.y),
        z((int)f.z)
    {
    }

    operator uint3()
    {
        return uint3
        (
            (uint)x,
            (uint)y,
            (uint)z
        );
    }

    int3 operator -(const int3& rhs)
    {
        return int3
        (
            x - rhs.x,
            y - rhs.y,
            z - rhs.z
        );
    }

    int3& operator -=(const int3& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;

        return *this;
    }
};

inline
uint3 operator << (
    uint a,
    const uint3& b)
{
    return uint3(a << b.x, a << b.y, a << b.z);
}



struct uint4
{
    uint x, y, z, w;

    uint4() = default;

    uint4(uint a, uint b, uint c, uint d) :
        x(a),
        y(b),
        z(c),
        w(d)
    {}

    uint& operator[](size_t idx)
    {
        return ((uint*)this)[idx];
    }

    uint operator[](size_t idx) const
    {
        return ((uint*)this)[idx];
    }
};



struct int4
{
    int x, y, z, w;

    int4() = default;

    int4(int a, int b, int c, int d) :
        x(a),
        y(b),
        z(c),
        w(d)
    {}

    int& operator[](size_t idx)
    {
        return ((int*)this)[idx];
    }

    int operator[](size_t idx) const
    {
        return ((int*)this)[idx];
    }
};

typedef unsigned short half;




typedef unsigned short half;


inline
float f16tof32(
    half v)
{
    static const unsigned multiple = 0x77800000;
    const unsigned biasedFloat = (v & 0x8000) << 16 | (v & 0x7FFF) << 13;
    return (float&)biasedFloat * (float&)multiple;
}


inline
half f32tof16(
    float Value)
{
    uint32_t Result;

    auto IValue = reinterpret_cast<uint32_t*>(&Value)[0];
    uint32_t Sign = (IValue & 0x80000000U) >> 16U;
    IValue = IValue & 0x7FFFFFFFU;

    if (IValue > 0x477FE000U)
    {

        if (((IValue & 0x7F800000) == 0x7F800000) && ((IValue & 0x7FFFFF) != 0))
        {
            Result = 0x7FFF;
        }
        else
        {
            Result = 0x7C00U;
        }
    }
    else if (!IValue)
    {
        Result = 0;
    }
    else
    {
        if (IValue < 0x38800000U)
        {


            uint32_t Shift = 113U - (IValue >> 23U);
            IValue = (0x800000U | (IValue & 0x7FFFFFU)) >> Shift;
        }
        else
        {

            IValue += 0xC8000000U;
        }

        Result = ((IValue + 0x0FFFU + ((IValue >> 13U) & 1U)) >> 13U) & 0x7FFFU;
    }
    return static_cast<USHORT>(Result | Sign);
}


inline
float FP16_RAZ(float x)
{
    const uint v = (((((uint&)(x)) + 0x1FFF) & ~0x1FFF));
    return (float&)v;
}


inline
float FP16_RTZ(float x)
{
    const uint v = ((((uint&)(x)) & ~0x1FFF));
    return (float&)v;
}



struct half2
{
    half x, y;
};

struct half3
{
    half x, y, z;
};

struct half4
{
    half x, y, z, w;
};


inline
float3 f16tof32(
    half3 v)
{
    return float3(f16tof32(v.x),
                  f16tof32(v.y),
                  f16tof32(v.z));
}

inline
float3 f16tof32(
    uint3 v)
{
    return float3(f16tof32((half)v.x),
                  f16tof32((half)v.y),
                  f16tof32((half)v.z));
}


using std::abs;

inline
float3 abs(const float3& a)
{
    return float3(abs(a.x), abs(a.y), abs(a.z));
}

inline
float min(float a, float b)
{
    return std::min(a, b);
}

inline
uint min(uint a, uint b)
{
    return std::min(a, b);
}

inline
uint64_t min(uint64_t a, uint64_t b)
{
    return std::min(a, b);
}

inline
float3 min(const float3& a, const float3& b)
{
    return float3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}


inline
float max(float a, float b)
{
    return std::max(a, b);
}

inline
uint max(uint a, uint b)
{
    return std::max(a, b);
}

inline
uint max(int a, int b)
{
    return std::max(a, b);
}







inline
uint3 min(uint3 a, uint3 b)
{
    return uint3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

inline
uint3 max(uint3 a, uint3 b)
{
    return uint3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

inline
int3 max(int3 a, int3 b)
{
    return int3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

inline
float3 max(const float3& a, const float3& b)
{
    return float3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

inline
float sign(float v)
{
    if (v < 0)
        return -1;
    return 1;
}

inline
float dot(float2 a, float2 b)
{
    return a.x * b.x + a.y * b.y;
}

inline
float dot(float3 a, float3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline
float dot(float4 a, float4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline
float3 cross(float3 a, float3 b)
{
    return float3
    {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline
float3 floor(float3 a)
{
    return float3(
        std::floorf(a.x),
        std::floorf(a.y),
        std::floorf(a.z)
    );
}

inline
float3 round(float3 a)
{
    return float3(
        std::floorf(a.x + 0.5f),
        std::floorf(a.y + 0.5f),
        std::floorf(a.z + 0.5f)
    );
}
inline
uint Emulate_tzcnt_u32(
    uint v)
{
    for (uint i=0; i < 32; ++i)
        if (v & (1 << i))
            return i;

    return 0;
}


inline
uint Emulate_tzcnt_u64(
    uint64_t v)
{
    for (uint i=0; i < 64; ++i)
        if (v & (1ull << i))
            return i;

    return 0;
}

inline
uint firstbitlow(uint a)
{

    if (a == 0)
        return ~0ul;

    return Emulate_tzcnt_u32(a);
}

inline
uint firstbitlow(int a)
{
    return firstbitlow((uint)a);
}

inline
uint firstbitlow(uint64_t a)
{

    if (a == 0)
        return ~0;

    return (uint)Emulate_tzcnt_u64(a);
}

inline
uint firstbitlow(int64_t a)
{
    return firstbitlow((uint64_t)a);
}

class RWByteAddressBuffer
{
public:

    RWByteAddressBuffer() : m_bufferPtr(nullptr)
    {
    }

    void operator=(D3D12_GPU_VIRTUAL_ADDRESS startAddr)
    {
        m_bufferPtr = (uint32_t*)startAddr;
    }

    void Assign(D3D12_GPU_VIRTUAL_ADDRESS startAddr)
    {
        m_bufferPtr = (uint32_t*)startAddr;
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetBase() const
    {
        return (D3D12_GPU_VIRTUAL_ADDRESS)m_bufferPtr;
    }

    uint Load(uint byteOffset) const
    {
        return m_bufferPtr[byteOffset / 4];
    }

    uint2 Load2(uint byteOffset) const
    {
        return (uint2&)m_bufferPtr[byteOffset / 4];
    }

    uint3 Load3(uint byteOffset) const
    {
        return (uint3&)m_bufferPtr[byteOffset / 4];
    }

    uint4 Load4(uint byteOffset) const
    {
        return (uint4&)m_bufferPtr[byteOffset / 4];
    }

    void Store(uint byteOffset, uint value) const
    {
        m_bufferPtr[byteOffset / 4] = value;
    }

    void Store2(uint byteOffset, uint2 value) const
    {
        ((uint2&)m_bufferPtr[byteOffset / 4]) = value;
    }

    void Store3(uint byteOffset, uint3 value) const
    {
        ((uint3&)m_bufferPtr[byteOffset / 4]) = value;
    }

    void Store4(uint byteOffset, uint4 value) const
    {
        ((uint4&)m_bufferPtr[byteOffset / 4]) = value;
    }

    void InterlockedAdd(uint byteOffset, uint addend, uint& previousValue)
    {
        uint32_t elementIdx = byteOffset / 4;
        previousValue = m_bufferPtr[elementIdx];
        m_bufferPtr[elementIdx] = previousValue + addend;
    }

    void InterlockedAdd(uint byteOffset, uint addend)
    {
        uint previousValue;
        InterlockedAdd(byteOffset, addend, previousValue);
    }

    void InterlockedCompareExchange(
        uint offsetBytes,
        uint compareValue,
        uint value,
        uint& previousValue)
    {
        previousValue = ::InterlockedCompareExchange(&m_bufferPtr[offsetBytes / 4],
            value,
            compareValue);
    }


    uint32_t* m_bufferPtr;
};

typedef RWByteAddressBuffer ByteAddressBuffer;


template <typename T>
class RWStructuredBuffer
{
public:
    RWStructuredBuffer() : m_bufferPtr(nullptr)
    {
    }

    void operator=(D3D12_GPU_VIRTUAL_ADDRESS startAddr)
    {
        m_bufferPtr = (T*)startAddr;
    }

    void Assign(D3D12_GPU_VIRTUAL_ADDRESS startAddr)
    {
        m_bufferPtr = (T*)startAddr;
    }

    T& operator[] (size_t idx)
    {
        return m_bufferPtr[idx];
    }

private:
    T* m_bufferPtr;
};

inline
uint __XB_s_load_dword(
    uint64_t address)
{
    return *(uint*)(g_sharedPointerBias + address);
}

inline
uint2 __XB_s_load_dwordx2(
    uint64_t address)
{
    return *(uint2*)(g_sharedPointerBias + address);
}

inline
uint3 __XB_s_load_dwordx3(
    uint64_t address)
{
    return *(uint3*)(g_sharedPointerBias + address);
}

inline
uint4 __XB_s_load_dwordx4(
    uint64_t address)
{
    return *(uint4*)(g_sharedPointerBias + address);
}

inline
uint __XB_global_load_dword(
    uint64_t address)
{
    return *(uint*)(g_sharedPointerBias + address);
}

inline
uint2 __XB_global_load_dwordx2(
    uint64_t address)
{
    return *(uint2*)(g_sharedPointerBias + address);
}

inline
uint3 __XB_global_load_dwordx3(
    uint64_t address)
{
    return *(uint3*)(g_sharedPointerBias + address);
}

inline
uint4 __XB_global_load_dwordx4(
    uint64_t address)
{
    return *(uint4*)(g_sharedPointerBias + address);
}


inline
void __XB_global_store_dword(
    uint64_t address,
    uint v)
{
    *(uint*)(g_sharedPointerBias + address) = v;
}

inline
void __XB_global_store_dwordx2(
    uint64_t address,
    uint2 v)
{
    *(uint2*)(g_sharedPointerBias + address) = v;
}

inline
void __XB_global_store_dwordx4(
    uint64_t address,
    uint4 v)
{
    *(uint4*)(g_sharedPointerBias + address) = v;
}


inline
float asfloat(
    uint v)
{
    return (float&)v;
}


inline
float2 asfloat(
    uint2 v)
{
    return (float2&)v;
}

inline
float3 asfloat(
    uint3 v)
{
    return (float3&)v;
}


inline
float4 asfloat(
    uint4 v)
{
    return (float4&)v;
}

inline
int asint(
    float v)
{
    return (int&)v;
}

inline
uint asuint(
    float v)
{
    return (uint&)v;
}

inline
uint3 asuint(
    float3 v)
{
    return (uint3&)v;
}

inline
uint4 asuint(
    float4 v)
{
    return (uint4&)v;
}

inline
bool isfinite(
    float x)
{
    uint expon = (asuint(x) >> 23) & 0xFF;
    return expon < 0xFF;
}

inline
bool isinf(
    float x)
{
    uint expon_and_manti = asuint(x) & 0x7FFFFFFF;
    return expon_and_manti == 0x7F800000;
}

inline
bool isnan(
    float x)
{
    uint expon_and_manti = asuint(x) & 0x7FFFFFFF;
    return expon_and_manti > 0x7F800000;
}

inline
float length(
    float3 v)
{
    return sqrtf(dot(v, v));
}

inline
float length(
    float2 v)
{
    return sqrtf(dot(v, v));
}

inline
float2 normalize(float2 v)
{
    float il = 1.0f / length(v);
    return v * il;
}

inline
float3 normalize(
    float3 v)
{
    float il = 1.f / length(v);
    return {v.x * il, v.y * il, v.z * il};
}

inline
float3 rcp(
    float3 v)
{
    return {1.f/v.x, 1.f/v.y, 1.f/v.z};
}

inline
float rcp(
    float v)
{
    return 1.f/v;
}

inline
uint __XB_GetLaneID()
{
    return 0;
}

inline uint WaveGetLaneIndex()
{
    return 0;
}

inline
float2 mul(
    const float2x2& m,
    const float2& v)
{
    return {
        dot(m[0], v),
        dot(m[1], v)
    };
}


inline
float3 mul(
    const float3x4& m,
    const float4& v)
{
    return {
        dot(m[0], v),
        dot(m[1], v),
        dot(m[2], v)
    };
}


inline
float3 mul(
    const float3x3& m,
    const float3& v)
{
    return {
        dot(m[0], v),
        dot(m[1], v),
        dot(m[2], v)
    };
}


inline
float3 mul(
    const float3& v,
    const float3x3& m)
{
    const float3x3 mt = transpose(m);

    return {
        dot(v, mt[0]),
        dot(v, mt[1]),
        dot(v, mt[2])
    };
}

inline
float4 mul(
    const float4x4& m,
    const float4& v)
{
    return {
        dot(m[0], v),
        dot(m[1], v),
        dot(m[2], v),
        dot(m[3], v)
    };
}


inline
float4 mul(
    const float4& v,
    const float4x4& m)
{
    const float4x4 mt = transpose(m);

    return {
        dot(v, mt[0]),
        dot(v, mt[1]),
        dot(v, mt[2]),
        dot(v, mt[3])
    };
}

inline
float4x4 mul(
    const float4x4& m1,
    const float4x4& m2)
{
    return {
        mul(m1[0], m2),
        mul(m1[1], m2),
        mul(m1[2], m2),
        mul(m1[3], m2)
    };
}

inline
float3x3 mul(
    const float3x3& m1,
    const float3x3& m2)
{
    return {
        mul(m1[0], m2),
        mul(m1[1], m2),
        mul(m1[2], m2)
    };
}
inline
uint2 __XB_Ballot64(
    uint v)
{
    return {};
}
struct StateIdType
{
    XdxrStateId_t v;


    inline
    uint64_t GetPointer64() const
    {
        return (((uint64_t)v) << XDXR_STATE_ID_TO_POINTER_SHIFT_AMOUNT);
    }

    inline XdxrStateId_t GetId() const
    {
        return v;
    }

    inline
    bool Valid() const
    {
        return v > 0 ;
    }

    inline
    bool IsBuiltInAnyhitIgnore() const
    {
        return v == 4 ;
    }

    inline
    void SetInvalid()
    {
        v = 0 ;
    }
};

struct BuiltInTriangleIntersectionAttributes
{
    float2 barycentrics;
};






inline
StateIdType ReadAnyHitFuncPtr(
   uint shaderRecOffsetBytes)
{
    return {};
}



inline
int __XB_MulI24(
    int a,
    int b)
{
    return ((a << 8) >> 8) * ((b << 8) >> 8);
}

inline
int __XB_MadI24(
    int a,
    int b,
    int c)
{
    return __XB_MulI24(a, b) + c;
}

inline
uint __XB_MulU24(
    uint a,
    uint b)
{
    return ((a << 8) >> 8) * ((b << 8) >> 8);
}

inline
uint __XB_MadU24(
    uint a,
    uint b,
    uint c)
{
    return __XB_MulU24(a, b) + c;
}

inline
unsigned __XB_BFI(
    unsigned s0,
    unsigned s1,
    unsigned s2)
{
    return (s0 & s1) | (~s0 & s2);
}



inline
void __XB_LDS_Mskor_U32(
    uint& v,
    uint mask,
    uint val)
{
    v = (v & ~mask) | val;
}



inline
uint firstbithigh(
    uint v)
{
    for (int i=31; i >= 0; --i)
    {
        if (v & (1U << i))
            return i;
    }

    return 0xffffffff;
}

inline
uint __XB_BFM(
    uint width,
    uint offset
)
{
    uint mask = (1U << width) - 1;
    return mask << offset;
}


inline
uint __XB_UBFE(
    uint width,
    uint offset,
    uint value)
{

    const int m = 32 - width;
    const int n = m - offset;
    return uint(value << n) >> m;
}

inline
int __XB_IBFE(
    uint width,
    uint offset,
    uint value)
{

    const int m = 32 - width;
    const int n = m - offset;
    return int(value << n) >> m;
}

inline
float __XB_Min3_F32(
    float a,
    float b,
    float c)
{
    return min(a, min(b, c));
}

inline
float __XB_Max3_F32(
    float a,
    float b,
    float c)
{
    return max(a, max(b, c));
}

inline
uint __XB_Min3_U32(
    uint a,
    uint b,
    uint c)
{
    return min(a, min(b, c));
}

inline
uint __XB_Max3_U32(
    uint a,
    uint b,
    uint c)
{
    return max(a, max(b, c));
}

inline
uint clamp(
    uint a,
    uint b,
    uint c)
{
    return min(max(a, b), c);
}

inline
uint __XB_Med3_U32(
    uint a,
    uint b,
    uint c)
{
    return clamp(a, min(b, c), max(b, c));
}

inline
float clamp(
    float a,
    float b,
    float c)
{
    return min(max(a, b), c);
}

inline
float saturate(
    float v)
{
    return clamp(v, 0.0f, 1.0f);
}

inline
float __XB_Med3_F32(
    float a,
    float b,
    float c)
{
    return clamp(a, min(b, c), max(b, c));
}

inline
float3 clamp(
    float3 v,
    float3 min,
    float3 max)
{
    v.x = clamp(v.x, min.x, max.x);
    v.y = clamp(v.y, min.y, max.y);
    v.z = clamp(v.z, min.z, max.z);
    return v;
}

inline
float3 saturate(
    float3 v)
{
    return clamp(v, 0.0f, 1.0f);
}

inline
float frac(
    float x)
{
    return x - std::floorf(x);
}

inline
uint2 __XB_MemTime()
{
    return 0;
}

inline
void InterlockedExchange(uint& dest, uint newVal, uint& oldVal)
{
    oldVal = dest;
    dest = newVal;
}


inline
uint WaveReadLaneAt(uint v, uint lane)
{
    return v;
}

inline
uint __XB_ReadLane(uint v, uint lane)
{
    return v;
}

inline
uint WaveReadLaneFirst(uint v)
{
    return v;
}

inline
bool WaveActiveAllTrue(bool)
{
    return true;
}

inline
bool WaveActiveAnyTrue(bool)
{
    return true;
}

inline
bool WaveActiveAllEqual(uint)
{
    return true;
}

inline
uint64_t Uint2ToGpuVA(
    uint x,
    uint y)
{
    return ((uint64_t)x) | ((uint64_t)y << 32ull);
}

inline
uint64_t Uint2ToGpuVA(
    uint2 dwords)
{
    return Uint2ToGpuVA(dwords.x, dwords.y);
}

inline
uint GpuVALoad1(
    uint64_t address)
{
    return __XB_global_load_dword(address);
}

inline
uint2 GpuVALoad2(
    uint64_t address)
{
    return __XB_global_load_dwordx2(address);
}

inline
uint3 GpuVALoad3(
    uint64_t address)
{




    return __XB_global_load_dwordx3(address);

}

inline
uint4 GpuVALoad4(
    uint64_t address)
{
    return __XB_global_load_dwordx4(address);
}


inline
uint GpuVALoad1Offset(
    uint64_t address,
    uint offset)
{
    return GpuVALoad1(address + offset);
}

inline
uint2 GpuVALoad2Offset(
    uint64_t address,
    uint offset)
{
    return GpuVALoad2(address + offset);
}

inline
uint3 GpuVALoad3Offset(
    uint64_t address,
    uint offset)
{
    return GpuVALoad3(address + offset);
}

inline
uint4 GpuVALoad4Offset(
    uint64_t address,
    uint offset)
{
    return GpuVALoad4(address + offset);
}


#pragma pack(push, 1)












#pragma pack(push, 1)

enum XboxBvh4NodeType
{
    XDXR_NODE_TYPE_TRI0 = 0,
    XDXR_NODE_TYPE_TRI1 = 1,
    XDXR_NODE_TYPE_TRI2 = 2,
    XDXR_NODE_TYPE_TRI3 = 3,
    XDXR_NODE_TYPE_BOX16 = 4,
    XDXR_NODE_TYPE_BOX32 = 5,
    XDXR_NODE_TYPE_INSTANCE = 6,
    XDXR_NODE_TYPE_AABB_GEOMETRY = 7,
    XDXR_NODE_TYPE_MASK = 0x00000007
};

static const uint32_t XDXR_OFFSET_OF_V0 = 0;
static const uint32_t XDXR_OFFSET_OF_V1 = 12;
static const uint32_t XDXR_OFFSET_OF_V2 = 24;
static const uint32_t XDXR_OFFSET_OF_V3 = 36;
static const uint32_t XDXR_OFFSET_OF_V4 = 48;

static const uint32_t XdxrInvalidNodePtr = 0xFFFFFFFF;


static const uint32_t XdxrSizeOfInt = 4;
static const uint32_t XdxrSizeOfGpuVA = 8;
static const uint32_t XdxrSizeOfVertex = 12;
static const uint32_t XdxrSizeOfUint4 = 16;


typedef uint32_t XboxBvh4NodePtr;




static const uint32_t D3D12XBOX_RAYTRACING_GEOMETRY_FLAG_INTERNAL_FLAG_FLIP_TRI1_BIT = 0x40;
static const uint32_t D3D12XBOX_RAYTRACING_GEOMETRY_FLAG_INTERNAL_FLAG_QUAD_PRIM_BIT = 0x80;



static const uint32_t D3D12XBOX_RAYTRACING_INSTANCE_FLAG_TAPU4 = 0x40;
static const uint32_t D3D12XBOX_RAYTRACING_INSTANCE_FLAG_TAPU16 = 0xC0;




inline
XboxBvh4NodeType XdxrTypeOfNodePointer(
    XboxBvh4NodePtr nodePtr)
{

    return (XboxBvh4NodeType)(nodePtr & XDXR_NODE_TYPE_MASK);
}

inline
uint32_t XdxrSizeOfNode(
    XboxBvh4NodePtr nodePtr)
{
    return XdxrTypeOfNodePointer(nodePtr) == XDXR_NODE_TYPE_BOX32 ? 128 : 64;
}

inline
uint64_t XdxrNodePointerToVA(
    uint64_t vaBVH,
    XboxBvh4NodePtr nodePtr)
{

    return vaBVH + ((uint64_t)(nodePtr >> 3)) * 64ull;
}

inline
XboxBvh4NodePtr XdxrVAToNodePointer(
    uint64_t vaBVH,
    uint64_t vaNode)
{
    return (XboxBvh4NodePtr)( ( (vaNode - vaBVH) / 64ull ) << 3 );
}

inline
bool XdxrIsValidNodePointer(
    XboxBvh4NodePtr nodePtr)
{






    return (int32_t)nodePtr > 0;
}

inline
uint32_t XdxrSizeOfKDOP(uint32_t N)
{



    const uint32_t numSides = 6;
    const uint32_t directionsPerSide = (N + 1) * (N + 1);

    return numSides * directionsPerSide * sizeof(float);
}

inline
uint32_t XdxrSizeOfApexPointMap(uint32_t N)
{


    const uint32_t numSides = 6;
    const uint32_t pointsPerSide = N * N * 2;
    const uint32_t sizeOfPoint = 3 * sizeof(float);

    return numSides * pointsPerSide * sizeOfPoint;
}


struct XdxrBoundingBox16
{
    half3 min;
    half3 max;
};
static const uint32_t XdxrBoundingBox16_Size = 12;


__declspec(align(64)) struct XboxBvh4Node16
{
    XboxBvh4NodePtr children[4];
    XdxrBoundingBox16 bounds[4];
};
static const uint32_t XboxBvh4Node16_Size = 64;
static const uint32_t XboxBvh4Node16_SizeLog2 = 6;


struct XdxrBoundingBox32
{
    float3 min;
    float3 max;
};
static const uint32_t XdxrBoundingBox32_Size = 24;


__declspec(align(64)) struct XboxBvh4Node32
{
    XboxBvh4NodePtr children[4];
    XdxrBoundingBox32 bounds[4];
    uint32_t padding[4];
};
static const uint32_t XboxBvh4Node32_Size = 128;
static const uint32_t XboxBvh4Node32_SizeLog2 = 7;
static const uint32_t XboxBvh4Node32_OffsetToChildren = 0;
static const uint32_t XboxBvh4Node32_OffsetToPad = 112;


static const uint32_t XboxBvh4Node_OffsetToBounds = 16;




struct XboxBvh4BaryRemap
{
    uint8_t vertexI : 2;
    uint8_t vertexJ : 2;
    uint8_t pad__ : 4;
};

struct XboxBvh4NodePacked
{
    XboxBvh4NodePtr children[4];
};




struct XboxBvh4BVVDHeader
{
    uint32_t variantNodeIndex;
    uint8_t variantBitMask : 6;
    uint8_t variantChildNode : 2;
};


__declspec(align(64)) struct XboxBvh4Leaf
{



    float3 vertices[4];


    uint32_t GeometryIndexAndFlags;
    uint32_t BasePrimitiveIndex;
    uint32_t PrimitiveIndex2;




    union
    {
        uint32_t baryRemap;
        XboxBvh4BaryRemap baryRemapStruct[4];
    };

};

static const uint32_t XboxBvh4Leaf_Size = 64;
static const uint32_t XboxBvh4Leaf_OffsetToGeometryIndexAndFlags = 48;
static const uint32_t XboxBvh4Leaf_OffsetToBasePrimitiveIndex = 52;
static const uint32_t XboxBvh4Leaf_OffsetToPrimitiveIndex2 = 56;
static const uint32_t XboxBvh4Leaf_OffsetToBaryRemap = 60;

static const uint32_t XboxBvh4Leaf_BasePrimitiveIndex_IgnoreThisLeafNode = 0xffffffff;
static const uint32_t XboxBvh4LeafPacked_BasePrimitiveIndex_IgnoreThisLeafNode = 0x00ffffff;



struct XboxBvh4LeafPacked
{

    uint32_t GeometryIndexAndFlags;
    uint32_t BasePrimitiveIndex;
    union
    {
        uint32_t PrimitiveIndex2AndBaryRemap;
        struct
        {
            uint32_t PrimitiveIndex2 : 24;
            uint32_t vertexI0 : 2;
            uint32_t vertexJ0 : 2;
            uint32_t vertexI1 : 2;
            uint32_t vertexJ1 : 2;
        };
    };
};



struct XboxBvh4ProceduralPrimitive
{
    uint4 nodePtrs;
    float3 minBound;
    float3 maxBound;
    uint32_t zero0;
    uint32_t zero1;
    uint32_t GeometryIndexAndFlags;
    uint32_t PrimitiveIndex;
    uint32_t ParentNode;
    uint32_t zero2;
};

static const uint32_t XboxBvh4ProceduralPrimitive_Size = 64;
static const uint32_t XboxBvh4ProceduralPrimitive_OffsetToBounds = 16;
static const uint32_t XboxBvh4ProceduralPrimitive_OffsetToGeometryIndexAndFlags = 48;
static const uint32_t XboxBvh4ProceduralPrimitive_OffsetToPrimitiveIndex = 52;



static const uint32_t GeometryIndexAndFlags_Shift_Flags = 0;
static const uint32_t GeometryIndexAndFlags_Mask_Flags = 0xff;
static const uint32_t GeometryIndexAndFlags_Mask_GeometryIndex = 0xffffff00;
static const uint32_t GeometryIndexAndFlags_Shift_GeometryIndex = 8;

static const uint32_t InstanceContributionToHitGroupIndexAndFlags_Shift_Flags = 24;
static const uint32_t InstanceContributionToHitGroupIndexAndFlags_Mask_Flags = 0xff000000;
static const uint32_t InstanceContributionToHitGroupIndexAndFlags_Mask_InstanceContributionToHitGroupIndex = 0x00ffffff;
static const uint32_t InstanceContributionToHitGroupIndexAndFlags_Shift_InstanceContributionToHitGroupIndex = 0x0;

static const uint32_t InstanceIDAndMask_Shift_InstanceID = 0;
static const uint32_t InstanceIDAndMask_Mask_InstanceID = 0x00ffffff;
static const uint32_t InstanceIDAndMask_Shift_Mask = 24;
static const uint32_t InstanceIDAndMask_Mask_Mask = 0xff000000;





__declspec(align(64)) struct XboxBvh4Instance
{

    float4 Transform[3];


    uint32_t InstanceIDAndMask;
    uint32_t InstanceContributionToHitGroupIndexAndFlags;
    uint32_t InternalFlags_RootNodePtr;
    uint32_t AccelerationStructure256BAligned;

    float4 WorldToObject[3];
    uint32_t InstanceIndex_remove;
    uint32_t ParentNode;
    uint32_t CompressionParams;
    uint32_t _Pad;
};
static const uint32_t D3D12_RAYTRACING_INSTANCE_DESC_Size = 64;

static const uint32_t XboxBvh4Instance_SizeOfInstanceShift = 7;
static const uint32_t XboxBvh4Instance_Size = (1U << XboxBvh4Instance_SizeOfInstanceShift);
static const uint32_t XboxBvh4Instance_OffsetToObjectToWorld = 0;
static const uint32_t XboxBvh4Instance_OffsetToInstanceMetaData1 = 48;
static const uint32_t XboxBvh4Instance_InstanceIDAndMask = 48;
static const uint32_t XboxBvh4Instance_InstanceContributionToHitGroupIndexAndFlags = 52;
static const uint32_t XboxBvh4Instance_OffsetToInternalFlags_RootNodePtr = 56;
static const uint32_t XboxBvh4Instance_OffsetToBlasPointer = 60;
static const uint32_t XboxBvh4Instance_OffsetToWorldToObject = 64;
static const uint32_t XboxBvh4Instance_OffsetToInstanceIndex = 112;
static const uint32_t XboxBvh4Instance_OffsetToParentNode = 116;
static const uint32_t XboxBvh4Instance_OffsetToCompressionParams = 120;

static const uint32_t SizeOfTLASElement = XboxBvh4Instance_Size;
static const uint32_t SizeOfBLASElement = XboxBvh4Leaf_Size;




static const uint32_t XDXR_VERSION_2403 = 0x00000006;





static const uint32_t XDXR_VERSION_2210 = 0x00000005;


static const uint32_t XDXR_VERSION_2008 = 0x00000004;



static const uint32_t XDXR_VERSION_TLAS_2007 = 0x00000002;
static const uint32_t XDXR_VERSION_BLAS_2007 = 0x80000003;


static const uint32_t XDXR_VERSION_2006 = 0x00000001;


static const uint32_t XDXR_VERSION_PIX_2008 = 0x00001001;

static const uint32_t XDXR_CURRENT_VERSION = XDXR_VERSION_2403;
static const uint32_t XDXR_FIRST_VERSION = XDXR_VERSION_2006;

static const uint32_t XDXR_VERSION_FAKE_TLAS = 0x40000000;


static const uint32_t XDXR_OFFLINE_BUILD_CRC32_FIELD = 0x0ff1;


static const uint32_t XDXR_INSTANCE_INTERNAL_ROOTNODEPTR_SIZE_BITS = 29;
static const uint32_t XDXR_INSTANCE_INTERNAL_FLAGS_TRANSFORM_HAS_IDENTITY_ROTATION = (0x80000000u);
static const uint32_t XDXR_INSTANCE_INTERNAL_FLAGS_HAS_SCALED_LOCAL_SPACE = (0x40000000u);
static const uint32_t XDXR_INSTANCE_INTERNAL_FLAGS_MASK = (
    XDXR_INSTANCE_INTERNAL_FLAGS_TRANSFORM_HAS_IDENTITY_ROTATION |
    XDXR_INSTANCE_INTERNAL_FLAGS_HAS_SCALED_LOCAL_SPACE
    );


inline
uint32_t XdxrUnpackRootNodePtr(uint32_t packedFlagsAndRootNodePtr)
{
    uint32_t packedRootNodePtr = packedFlagsAndRootNodePtr & ~XDXR_INSTANCE_INTERNAL_FLAGS_MASK;
    return (packedRootNodePtr == ~XDXR_INSTANCE_INTERNAL_FLAGS_MASK) ? XdxrInvalidNodePtr : packedRootNodePtr;
}


static const uint32_t XDXR_UNSCALED_PACKED_EXPONENTS = 0x007F7F7F;

static const uint32_t XDXR_GEOM_ROTATION_BITS_MASK = 0x3F000000;
static const uint32_t XDXR_GEOM_ROTATION_BITS_SHIFT = 24;





__declspec(align(64)) struct XboxBvh4Header
{
    uint32_t flags;
    uint32_t totalSizeIn64BNodes;
    uint32_t crc32;
    uint32_t numElements;

    XdxrBoundingBox32 sceneBounds;
    uint32_t offsetToNodes;
    int32_t offsetToPrimRemap;

    uint32_t rootNodePtr;
    uint32_t offsetToElements;
    uint32_t offsetToParents;
    uint32_t compressionParams;
};


static const uint32_t XboxBvh4Header_Size = 64;

static const uint32_t XboxBvh4Header_Flags_VersionMask = 0x0000FFFF;
static const uint32_t XboxBvh4Header_Flags_BottomLevel = 0x80000000;




static const uint32_t XboxBvh4Header_Flags_Compressed = 0x00010000;

static const uint32_t XboxBvh4Header_Flags_Offline = 0x00020000;

static const uint32_t XboxBvh4Header_Flags_Updateable = 0x00040000;

static const uint32_t XboxBvh4Header_Flags_Compressed_For_GPU = 0x00080000;

static const uint32_t XboxBvh4Header_Flags_Dehydrated = 0x01000000;


static const uint32_t XboxBvh4Header_Flags_kDOP_Mask = 0x00F00000;
static const uint32_t XboxBvh4Header_Flags_kDOP_Shift = 20;
static const uint32_t XboxBvh4Header_kDOP_MaxTesselation = 15;


inline
bool XdxrBvhIsTlas(
    uint32_t flags)
{
    return !(flags & XboxBvh4Header_Flags_BottomLevel);
}

inline
uint32_t XdxrBvhHeaderVersion(
    uint32_t flags)
{
    return (flags & XboxBvh4Header_Flags_VersionMask);
}

inline
uint32_t XdxrBlasHeaderVersion(
    uint32_t flags)
{
    return XdxrBvhIsTlas(flags) ? 0 : (flags & XboxBvh4Header_Flags_VersionMask);
}

inline
uint32_t XdxrTlasHeaderVersion(
    uint32_t flags)
{
    return XdxrBvhIsTlas(flags) ? (flags & XboxBvh4Header_Flags_VersionMask) : 0;
}

inline
bool XdxrBvhIsCompressed(
    uint32_t flags)
{
    return (flags & XboxBvh4Header_Flags_Compressed);
}

inline
bool XdxrBvhIsDehydrated(
    uint32_t flags)
{
    return (flags & XboxBvh4Header_Flags_Dehydrated);
}


inline
uint32_t XdxrBvhGetKDOPTesselationFactor(
    uint32_t flags)
{
    return (flags & XboxBvh4Header_Flags_kDOP_Mask) >> XboxBvh4Header_Flags_kDOP_Shift;
}


inline
uint64_t XdxrBvhGetSizeBytes(



    const XboxBvh4Header& pHeader)

{





    const uint64_t bvhSize = (pHeader.totalSizeIn64BNodes + 1) * 64ULL;

    return bvhSize;
}





static const uint32_t XboxBvh4Header_CompressionFlags_Scaled = 0x80000000;
static const uint32_t XboxBvh4Header_CompressionFlags_Translated = 0x40000000;
static const uint32_t XboxBvh4Header_CompressionFlags_Rotated = 0x3F000000;
static const uint32_t XboxBvh4Header_CompressionFlags_Transformed = 0xFF000000;


inline
bool BvhIsScaled(
    uint32_t compressionParams)
{
    return (compressionParams & XboxBvh4Header_CompressionFlags_Scaled) != 0;
}


inline
bool BvhIsTranslated(
    uint32_t compressionParams)
{
    return (compressionParams & XboxBvh4Header_CompressionFlags_Translated) != 0;
}


inline
bool BvhIsRotated(
    uint32_t compressionParams)
{
    return (compressionParams & XboxBvh4Header_CompressionFlags_Rotated) != 0;
}

inline
uint BvhGetRotationIndex(
    uint32_t compressionParams)
{
    return (compressionParams & XDXR_GEOM_ROTATION_BITS_MASK) >> XDXR_GEOM_ROTATION_BITS_SHIFT;
}


inline
bool BvhIsTransformed(
    uint32_t compressionParams)
{
    return (compressionParams & XboxBvh4Header_CompressionFlags_Transformed) != 0;
}

inline
uint32_t PackScaleExponents(
    float3 scale)
{
    const uint expsX = (asuint(scale.x) >> 23) & 0xFF;
    const uint expsY = (asuint(scale.y) >> 23) & 0xFF;
    const uint expsZ = (asuint(scale.z) >> 23) & 0xFF;
    return expsX | (expsY << 8) | (expsZ << 16);
}

inline
float3 UnpackScaleExponents(
    uint32_t packedExps)
{
    uint32_t expX = packedExps & 0xFF;
    uint32_t expY = (packedExps >> 8) & 0xFF;
    uint32_t expZ = (packedExps >> 16) & 0xFF;

    return asfloat(uint3(expX << 23, expY << 23, expZ << 23));
}

inline
float3 UnpackRcpScaleExponents(
    uint32_t packedExps)
{
    packedExps = 0xFEFEFE - (packedExps & 0xFFFFFF);
    return UnpackScaleExponents(packedExps);
}


inline
float3 BvhGetCompressionScale(
    uint32_t compressionParams)
{
    if (BvhIsScaled(compressionParams))
    {
        return UnpackScaleExponents(compressionParams);
    }
    else
    {
        return 1.0f;
    }
}


inline
float3 BvhGetDecompressionScale(
    uint32_t compressionParams)
{
    if (BvhIsScaled(compressionParams))
    {
        return UnpackRcpScaleExponents(compressionParams);
    }
    else
    {
        return 1.0f;
    }
}

inline
float ComputeScale(float radius)
{

    float clampedRadius = clamp(radius, asfloat(30 << 23), asfloat(0x7F7FFFFF));


    float rcpScale = asfloat(((asuint(clampedRadius) + 0x1FFF) & 0x7F800000) - (15 << 23));
    float scale = asfloat(0x7F000000 - asuint(rcpScale));

    const float eps = 0.0001f;

    return radius < eps ? 1.0f : scale;
}




inline
float3 BvhGetTranslation(
    float3 sceneMin,
    float3 sceneMax)
{

    float tx = (sceneMin.x > 0.0f || sceneMax.x < 0.0f) ? (sceneMin.x + sceneMax.x) * 0.5f : 0.0f;
    float ty = (sceneMin.y > 0.0f || sceneMax.y < 0.0f) ? (sceneMin.y + sceneMax.y) * 0.5f : 0.0f;
    float tz = (sceneMin.z > 0.0f || sceneMax.z < 0.0f) ? (sceneMin.z + sceneMax.z) * 0.5f : 0.0f;
    return float3(tx, ty, tz);
}

inline
uint ChooseCompressionSpace(
    float3 bvhMin,
    float3 bvhMax,
    float3& scale,
    float3& translate,
    bool allowTranslation = true)
{
    bool isTranslated = false;

    if (allowTranslation)
    {
        translate = BvhGetTranslation(bvhMin, bvhMax);
        isTranslated = dot(translate, translate) > 0.0f;
        bvhMin -= translate;
        bvhMax -= translate;
    }

    uint32_t compressionParams = XDXR_UNSCALED_PACKED_EXPONENTS;






    {
        const float3 maxDims = max(abs(bvhMax), abs(bvhMin));


        {
            scale.x = ComputeScale(maxDims.x);
            scale.y = ComputeScale(maxDims.y);
            scale.z = ComputeScale(maxDims.z);

            compressionParams = PackScaleExponents(scale) | XboxBvh4Header_CompressionFlags_Scaled;
        }
    }

    if (isTranslated)
    {
        compressionParams |= XboxBvh4Header_CompressionFlags_Translated;
    }

    return compressionParams;
}

inline
float3x3 BuildRotationMatrix(uint32_t quantizedRotAngle)
{
    const float cosTable[5] = {
        1.0f,
        0.92387953f,
        0.70710678f,
        0.38268343f,
        0.0f
    };

    const uint rotX = quantizedRotAngle & 3;
    const float cosX = cosTable[rotX];
    const float sinX = cosTable[4 - rotX];

    float3x3 mtxX = float3x3(
        1.0f, 0.0f, 0.0f,
        0.0f, cosX, sinX,
        0.0f, -sinX, cosX);

    const uint rotY = (quantizedRotAngle >> 2) & 3;
    const float cosY = cosTable[rotY];
    const float sinY = cosTable[4 - rotY];

    float3x3 mtxY = float3x3(
        cosY, 0.0f, -sinY,
        0.0f, 1.0f, 0.0f,
        sinY, 0.0f, cosY);

    const uint rotZ = (quantizedRotAngle >> 4) & 3;
    const float cosZ = cosTable[rotZ];
    const float sinZ = cosTable[4 - rotZ];

    float3x3 mtxZ = float3x3(
        cosZ, sinZ, 0.0f,
        -sinZ, cosZ, 0.0f,
        0.0f, 0.0f, 1.0f);

    return mul(mtxZ, mul(mtxY, mtxX));
}







inline
const float3x3& GetRotationMatrix(uint32_t rot)
{
    static const float3x3 lookup[64] =

    {
        BuildRotationMatrix(0x00), BuildRotationMatrix(0x01), BuildRotationMatrix(0x02), BuildRotationMatrix(0x03),
        BuildRotationMatrix(0x04), BuildRotationMatrix(0x05), BuildRotationMatrix(0x06), BuildRotationMatrix(0x07),
        BuildRotationMatrix(0x08), BuildRotationMatrix(0x09), BuildRotationMatrix(0x0A), BuildRotationMatrix(0x0B),
        BuildRotationMatrix(0x0C), BuildRotationMatrix(0x0D), BuildRotationMatrix(0x0E), BuildRotationMatrix(0x0F),

        BuildRotationMatrix(0x10), BuildRotationMatrix(0x11), BuildRotationMatrix(0x12), BuildRotationMatrix(0x13),
        BuildRotationMatrix(0x14), BuildRotationMatrix(0x15), BuildRotationMatrix(0x16), BuildRotationMatrix(0x17),
        BuildRotationMatrix(0x18), BuildRotationMatrix(0x19), BuildRotationMatrix(0x1A), BuildRotationMatrix(0x1B),
        BuildRotationMatrix(0x1C), BuildRotationMatrix(0x1D), BuildRotationMatrix(0x1E), BuildRotationMatrix(0x1F),

        BuildRotationMatrix(0x20), BuildRotationMatrix(0x21), BuildRotationMatrix(0x22), BuildRotationMatrix(0x23),
        BuildRotationMatrix(0x24), BuildRotationMatrix(0x25), BuildRotationMatrix(0x26), BuildRotationMatrix(0x27),
        BuildRotationMatrix(0x28), BuildRotationMatrix(0x29), BuildRotationMatrix(0x2A), BuildRotationMatrix(0x2B),
        BuildRotationMatrix(0x2C), BuildRotationMatrix(0x2D), BuildRotationMatrix(0x2E), BuildRotationMatrix(0x2F),

        BuildRotationMatrix(0x30), BuildRotationMatrix(0x31), BuildRotationMatrix(0x32), BuildRotationMatrix(0x33),
        BuildRotationMatrix(0x34), BuildRotationMatrix(0x35), BuildRotationMatrix(0x36), BuildRotationMatrix(0x37),
        BuildRotationMatrix(0x38), BuildRotationMatrix(0x39), BuildRotationMatrix(0x3A), BuildRotationMatrix(0x3B),
        BuildRotationMatrix(0x3C), BuildRotationMatrix(0x3D), BuildRotationMatrix(0x3E), BuildRotationMatrix(0x3F)
    };

    return lookup[rot];
}

inline
float3x3 GetInverseRotationMatrix(uint32_t rot)
{
    return transpose(GetRotationMatrix(rot));
}



static const uint32_t XboxBvh4Header_OffsetToFlags = 0;
static const uint32_t XboxBvh4Header_OffsetToTotalSizeIn64BNodes = 4;
static const uint32_t XboxBvh4Header_OffsetToCRC32 = 8;
static const uint32_t XboxBvh4Header_OffsetToNumElements = 12;
static const uint32_t XboxBvh4Header_OffsetToSceneBounds = 16;
static const uint32_t XboxBvh4Header_OffsetToOffsetToNodes = 40;
static const uint32_t XboxBvh4Header_OffsetToOffsetToPrimRemap = 44;
static const uint32_t XboxBvh4Header_OffsetToRootNodePtr = 48;
static const uint32_t XboxBvh4Header_OffsetToOffsetToElements = 52;
static const uint32_t XboxBvh4Header_OffsetToOffsetToParents = 56;
static const uint32_t XboxBvh4Header_OffsetToCompressionParams = 60;


static const uint32_t XboxBvh4Header_OffsetToSceneBounds_Min_X = 16;
static const uint32_t XboxBvh4Header_OffsetToSceneBounds_Min_Y = 20;
static const uint32_t XboxBvh4Header_OffsetToSceneBounds_Min_Z = 24;
static const uint32_t XboxBvh4Header_OffsetToSceneBounds_Max_X = 28;
static const uint32_t XboxBvh4Header_OffsetToSceneBounds_Max_Y = 32;
static const uint32_t XboxBvh4Header_OffsetToSceneBounds_Max_Z = 36;


static_assert(alignof(XboxBvh4Node16) == 64, "Incorrect BVH4 node alignment");
static_assert(alignof(XboxBvh4Node32) == 64, "Incorrect BVH4 node alignment");
static_assert(alignof(XboxBvh4Leaf) == 64, "Incorrect BVH4 node alignment");
static_assert(sizeof(XboxBvh4Header) == XboxBvh4Header_Size, "Incorrect BVH4 header size");
static_assert(sizeof(XboxBvh4Node16) == XboxBvh4Node16_Size, "Incorrect BVH4 node size");
static_assert(sizeof(XboxBvh4Node32) == XboxBvh4Node32_Size, "Incorrect BVH4 node size");
static_assert(sizeof(XboxBvh4Leaf) == XboxBvh4Leaf_Size, "Incorrect BVH4 node size");
static_assert(sizeof(XboxBvh4Instance) == XboxBvh4Instance_Size, "Incorrect BVH4 node size");
static_assert(sizeof(XdxrBoundingBox32) == XdxrBoundingBox32_Size, "Incorrect sizeof for XdxrBoundingBox32");
static_assert(sizeof(XboxBvh4Instance) == SizeOfTLASElement, "Incorrect sizeof for TLAS element");
static_assert(sizeof(XboxBvh4Leaf) == SizeOfBLASElement, "Incorrect sizeof for BLAS element");

static_assert(XboxBvh4Node_OffsetToBounds == offsetof(XboxBvh4Node32, bounds), "");
static_assert(XboxBvh4Node_OffsetToBounds == offsetof(XboxBvh4Node16, bounds), "");

static_assert(XboxBvh4Leaf_OffsetToGeometryIndexAndFlags == offsetof(XboxBvh4Leaf, GeometryIndexAndFlags), "");
static_assert(XboxBvh4Leaf_OffsetToBasePrimitiveIndex == offsetof(XboxBvh4Leaf, BasePrimitiveIndex), "");
static_assert(XboxBvh4Leaf_OffsetToPrimitiveIndex2 == offsetof(XboxBvh4Leaf, PrimitiveIndex2), "");
static_assert(XboxBvh4Leaf_OffsetToBaryRemap == offsetof(XboxBvh4Leaf, baryRemap), "");

static_assert(XboxBvh4Instance_OffsetToInstanceMetaData1 == offsetof(XboxBvh4Instance, InstanceIDAndMask), "");
static_assert(XboxBvh4Instance_OffsetToBlasPointer == offsetof(XboxBvh4Instance, AccelerationStructure256BAligned), "");
static_assert(XboxBvh4Instance_OffsetToInstanceIndex == offsetof(XboxBvh4Instance, InstanceIndex_remove), "");
static_assert(XboxBvh4Instance_OffsetToInternalFlags_RootNodePtr == offsetof(XboxBvh4Instance, InternalFlags_RootNodePtr), "");
static_assert(XboxBvh4Instance_OffsetToCompressionParams == offsetof(XboxBvh4Instance, CompressionParams), "");

static_assert(XboxBvh4Header_OffsetToNumElements == offsetof(XboxBvh4Header, numElements), "");
static_assert(XboxBvh4Header_OffsetToCRC32 == offsetof(XboxBvh4Header, crc32), "");
static_assert(XboxBvh4Header_OffsetToTotalSizeIn64BNodes == offsetof(XboxBvh4Header, totalSizeIn64BNodes), "");
static_assert(XboxBvh4Header_OffsetToRootNodePtr == offsetof(XboxBvh4Header, rootNodePtr), "");
static_assert(XboxBvh4Header_OffsetToOffsetToNodes == offsetof(XboxBvh4Header, offsetToNodes), "");
static_assert(XboxBvh4Header_OffsetToOffsetToPrimRemap == offsetof(XboxBvh4Header, offsetToPrimRemap), "");
static_assert(XboxBvh4Header_OffsetToOffsetToElements == offsetof(XboxBvh4Header, offsetToElements), "");
static_assert(XboxBvh4Header_OffsetToOffsetToParents == offsetof(XboxBvh4Header, offsetToParents), "");
static_assert(XboxBvh4Header_OffsetToSceneBounds == offsetof(XboxBvh4Header, sceneBounds), "");
static_assert(XboxBvh4Header_OffsetToSceneBounds_Min_X == offsetof(XboxBvh4Header, sceneBounds.min.x), "");
static_assert(XboxBvh4Header_OffsetToSceneBounds_Min_Y == offsetof(XboxBvh4Header, sceneBounds.min.y), "");
static_assert(XboxBvh4Header_OffsetToSceneBounds_Min_Z == offsetof(XboxBvh4Header, sceneBounds.min.z), "");
static_assert(XboxBvh4Header_OffsetToSceneBounds_Max_X == offsetof(XboxBvh4Header, sceneBounds.max.x), "");
static_assert(XboxBvh4Header_OffsetToSceneBounds_Max_Y == offsetof(XboxBvh4Header, sceneBounds.max.y), "");
static_assert(XboxBvh4Header_OffsetToSceneBounds_Max_Z == offsetof(XboxBvh4Header, sceneBounds.max.z), "");
static_assert(XboxBvh4Header_OffsetToCompressionParams == offsetof(XboxBvh4Header, compressionParams), "");
static_assert(XboxBvh4Header_OffsetToFlags == offsetof(XboxBvh4Header, flags), "");

static_assert(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) == D3D12_RAYTRACING_INSTANCE_DESC_Size, "Incorrect sizeof Instance Desc");
static_assert(offsetof(XboxBvh4Instance, WorldToObject) == D3D12_RAYTRACING_INSTANCE_DESC_Size, "Offset of ObjectToWorld is 64 in HLSL");
static_assert(offsetof(XboxBvh4Instance, WorldToObject) == XboxBvh4Instance_OffsetToWorldToObject, "Offset of ObjectToWorld is 64 in HLSL");

static_assert(XboxBvh4ProceduralPrimitive_Size == sizeof(XboxBvh4ProceduralPrimitive), "");
static_assert(XboxBvh4ProceduralPrimitive_OffsetToBounds == offsetof(XboxBvh4ProceduralPrimitive, minBound), "");
static_assert(XboxBvh4ProceduralPrimitive_OffsetToGeometryIndexAndFlags == offsetof(XboxBvh4ProceduralPrimitive, GeometryIndexAndFlags), "");
static_assert(XboxBvh4ProceduralPrimitive_OffsetToPrimitiveIndex == offsetof(XboxBvh4ProceduralPrimitive, PrimitiveIndex), "");



static_assert(XboxBvh4ProceduralPrimitive_OffsetToGeometryIndexAndFlags == XboxBvh4Leaf_OffsetToGeometryIndexAndFlags, "");
static_assert(XboxBvh4ProceduralPrimitive_OffsetToPrimitiveIndex == XboxBvh4Leaf_OffsetToBasePrimitiveIndex, "");



static_assert(XboxBvh4Leaf_OffsetToGeometryIndexAndFlags + 4 == XboxBvh4Leaf_OffsetToBasePrimitiveIndex, "");
static_assert(XboxBvh4Leaf_OffsetToBasePrimitiveIndex + 4 == XboxBvh4Leaf_OffsetToPrimitiveIndex2, "");




#pragma pack(pop)




inline
uint32_t XdxrBvhNodePtrToOffset(
    XboxBvh4NodePtr ptr)
{
    return (ptr >> 3) << 6;
}

inline
XboxBvh4NodePtr XdxrOffsetToBvhNodePtr(
    uint32_t offset,
    XboxBvh4NodeType type)
{
    return ((offset >> 6) << 3) | type;
}
inline
void XdxrGetVertexIndicesForNodeType(
    uint32_t i[3],
    uint32_t nodeType)
{


    const uint32_t triangleVert0 = ((0) | (1 << 4) | (2 << 8) | (2 << 12));
    const uint32_t triangleVert1 = ((1) | (3 << 4) | (3 << 8) | (4 << 12));
    const uint32_t triangleVert2 = ((2) | (2 << 4) | (4 << 8) | (0 << 12));


    const uint32_t triangleVert0And1 = triangleVert0 |
        (triangleVert1 << 16);

    const uint32_t shift = nodeType * 4;

    i[0] = (triangleVert0And1 >> shift) & 0x7;
    i[1] = (triangleVert0And1 >> (16 + shift)) & 0x7;
    i[2] = (triangleVert2 >> shift) & 0x7;
}







inline
void XboxBvh4Leaf_GetTriangleFromLeaf(
    float3 (&tri)[3],
    XboxBvh4NodePtr leafType,
    const XboxBvh4Leaf* pLeaf)
{
    unsigned indices[3];
    XdxrGetVertexIndicesForNodeType(indices,
        XdxrTypeOfNodePointer(leafType));

    for (unsigned i=0; i < 3; ++i)
    {
        tri[i] = pLeaf->vertices[indices[i]];
    }
}




inline
bool IsFloat3Equal(
    const float3& v1,
    const float3& v2)
{
    return v1.x == v2.x &&
           v1.y == v2.y &&
           v1.z == v2.z;
}

inline
bool IsFloat4Equal(
    const float4& v1,
    const float4& v2)
{
    return v1.x == v2.x &&
           v1.y == v2.y &&
           v1.z == v2.z &&
           v1.w == v2.w;
}
struct XdxrMatrix
{
    float4 row0;
    float4 row1;
    float4 row2;

    float3 Mul3x3(
        float3 v)
    {
        float x = row0.x * v.x + row0.y * v.y + row0.z * v.z;
        float y = row1.x * v.x + row1.y * v.y + row1.z * v.z;
        float z = row2.x * v.x + row2.y * v.y + row2.z * v.z;

        return float3(x, y, z);
    }

    float3 Mul3x4(
        float3 v)
    {
        float x = row0.x * v.x + row0.y * v.y + row0.z * v.z + row0.w;
        float y = row1.x * v.x + row1.y * v.y + row1.z * v.z + row1.w;
        float z = row2.x * v.x + row2.y * v.y + row2.z * v.z + row2.w;

        return float3(x, y, z);
    }
};






inline
bool IsXdxrMatrixEqual(
    const XdxrMatrix& matrix1,
    const XdxrMatrix& matrix2)
{
    return memcmp(&matrix1, &matrix2, sizeof(XdxrMatrix)) == 0;
}





inline
void XdxrMatrix_PackInstanceWorldToObject(
    uint4 instanceTransform[3],
    XdxrMatrix worldToObject)
{
    instanceTransform[0].x = asuint(worldToObject.row0.w);
    instanceTransform[0].y = asuint(worldToObject.row1.w);
    instanceTransform[0].z = asuint(worldToObject.row2.w);

    instanceTransform[0].w = asuint(worldToObject.row0.x);
    instanceTransform[1].x = asuint(worldToObject.row0.y);
    instanceTransform[1].y = asuint(worldToObject.row0.z);

    instanceTransform[1].z = asuint(worldToObject.row1.x);
    instanceTransform[1].w = asuint(worldToObject.row1.y);
    instanceTransform[2].x = asuint(worldToObject.row1.z);

    instanceTransform[2].y = asuint(worldToObject.row2.x);
    instanceTransform[2].z = asuint(worldToObject.row2.y);
    instanceTransform[2].w = asuint(worldToObject.row2.z);
}


inline
void XdxrMatrix_UnpackInstanceWorldToObject(
    XdxrMatrix& worldToObject,
    uint4 row0,
    uint4 row1,
    uint4 row2)
{
    worldToObject.row0[0] = asfloat(row0.w);
    worldToObject.row0[1] = asfloat(row1.x);
    worldToObject.row0[2] = asfloat(row1.y);
    worldToObject.row0[3] = asfloat(row0.x);

    worldToObject.row1[0] = asfloat(row1.z);
    worldToObject.row1[1] = asfloat(row1.w);
    worldToObject.row1[2] = asfloat(row2.x);
    worldToObject.row1[3] = asfloat(row0.y);

    worldToObject.row2[0] = asfloat(row2.y);
    worldToObject.row2[1] = asfloat(row2.z);
    worldToObject.row2[2] = asfloat(row2.w);
    worldToObject.row2[3] = asfloat(row0.z);
}


inline
float3x4 XdxrMatrix_UnpackInstanceWorldToObject(
    uint4 row0,
    uint4 row1,
    uint4 row2)
{
    XdxrMatrix worldToObject;
    XdxrMatrix_UnpackInstanceWorldToObject(worldToObject, row0, row1, row2);
    return float3x4(worldToObject.row0, worldToObject.row1, worldToObject.row2);
}


inline
uint32_t XboxBvh4Instance_GetInstanceIndexFromNodePtr(
    uint64_t tlasVA,
    XboxBvh4NodePtr instanceNodePtr)
{
    uint64_t nodePtr = XdxrNodePointerToVA(tlasVA, instanceNodePtr);

    uint32_t index = __XB_global_load_dword(nodePtr + XboxBvh4Instance_OffsetToInstanceIndex);

    return index;
}






inline
uint64_t XboxBvh4Instance_GetBlasPointerFromMetadata(
    uint64_t vaInstance)
{

    uint32_t blasPtr = GpuVALoad1Offset(vaInstance, XboxBvh4Instance_OffsetToBlasPointer);
    return ((uint64_t)blasPtr) << 8;
}
inline
void XboxBvh4Leaf_ExtractNodeVertexPositions(
    float3& v0,
    float3& v1,
    float3& v2,
    uint64_t nodePtr,
    uint32_t nodeType)
{
    const uint32_t baryRemap = GpuVALoad1Offset(nodePtr, XboxBvh4Leaf_OffsetToBaryRemap) >> (8 * nodeType);

    const uint32_t vertex1Index = (baryRemap >> 0) & 0x3;
    const uint32_t vertex2Index = (baryRemap >> 2) & 0x3;


    const uint32_t indexOfIndex0 = baryRemap & 0xf;







    const uint32_t index0Array = ((0 << 0) | (2 << 2) | (1 << 4) | (0 << 6) | (2 << 8) | (0 << 10) | (0 << 12) | (0 << 14) | (1 << 16) | (0 << 18) | (0 << 20) | (0 << 22) | (0 << 24) | (0 << 26) | (0 << 28) | (0 << 30));

    const uint32_t vertex0Index = (index0Array >> (indexOfIndex0 * 2)) & 0x3;

    uint32_t i[3];
    XdxrGetVertexIndicesForNodeType(i, nodeType);


    const uint32_t packedTri = i[0] | (i[1] << 4) | (i[2] << 8);


    uint32_t i0 = (packedTri >> (vertex0Index * 4)) & 0x7;
    uint32_t i1 = (packedTri >> (vertex1Index * 4)) & 0x7;
    uint32_t i2 = (packedTri >> (vertex2Index * 4)) & 0x7;


    v0 = asfloat(GpuVALoad3Offset(nodePtr, i0 * 12));
    v1 = asfloat(GpuVALoad3Offset(nodePtr, i1 * 12));
    v2 = asfloat(GpuVALoad3Offset(nodePtr, i2 * 12));
}
inline
uint32_t XboxBvh4Node_GetParentTableOffset(
    uint64_t vaBVH)
{

    const uint32_t offsetToElements = GpuVALoad1Offset(vaBVH, XboxBvh4Header_OffsetToOffsetToElements);
    const uint32_t offsetToParents = GpuVALoad1Offset(vaBVH, XboxBvh4Header_OffsetToOffsetToParents);
    return (offsetToElements / 64) - (offsetToParents / 4);
}





inline
uint32_t XboxBvh4Node_LoadParentPointer(
    uint64_t vaBVH,
    XboxBvh4NodePtr childPtr,
    uint32_t parentTableOffset)
{
    const uint32_t parentPtrDwordOffset = (childPtr >> 3) - parentTableOffset;
    const uint32_t parentPtr = GpuVALoad1Offset(vaBVH, parentPtrDwordOffset * 4);
    return parentPtr;
}







static
bool BVHIsValidVersion(
    unsigned flags)
{
    const unsigned version = XdxrBvhHeaderVersion(flags);
    return version >= XDXR_FIRST_VERSION && version <= XDXR_CURRENT_VERSION;
}

static
bool BVHIsValidPixVersion(
    unsigned flags)
{
    const unsigned version = XdxrBvhHeaderVersion(flags);
    return version == XDXR_VERSION_PIX_2008;
}




static
HRESULT BVHGetSizeCPU(
    uint64_t& sizeInBytes,
    const void* pTree)
{
    const XboxBvh4Header* pBVHHeader = (const XboxBvh4Header*)pTree;

    if (!BVHIsValidVersion(pBVHHeader->flags))
    {
        return E_FAIL;
    }

    sizeInBytes = 0ull;
    if (pBVHHeader->rootNodePtr != XdxrInvalidNodePtr)
    {


        sizeInBytes = (pBVHHeader->totalSizeIn64BNodes + 1) * 64ull;
    }

    return S_OK;
}





static
uint64_t GetInstanceNodeBlasPtr(
    const XboxBvh4Instance* pLeaf)
{
    return (uint64_t)pLeaf->AccelerationStructure256BAligned << 8;
}




static
uint64_t GetInstanceNodeBlasPtr(
    const XboxBvh4Header* pTree,
    XboxBvh4NodePtr nodePtr)
{
    const XboxBvh4Instance* pLeaf = (const XboxBvh4Instance*)XdxrNodePointerToVA((uintptr_t)pTree, nodePtr);

    return GetInstanceNodeBlasPtr(pLeaf);
}





static
uintptr_t GetPointerToElements(
    const XboxBvh4Header* pHeader)
{
    return (uintptr_t)pHeader + pHeader->offsetToElements;
}



inline
uint32_t GetNumInternalBVH2Nodes(
    uint32_t numLeaves)
{
    if (numLeaves == 0)
        return 0;

    return max(1u, numLeaves - 1);
}


inline
uint32_t GetTotalNumBVH2Nodes(
    uint32_t numLeaves)
{
    return numLeaves + GetNumInternalBVH2Nodes(numLeaves);
}
inline
uint32_t GetMaxBVH4Nodes(
    uint32_t numLeaves,
    bool tlas)
{
        return max(1u, (numLeaves + 1) / 2 + numLeaves / 4);






}



#pragma pack(pop)







inline
XdxrBoundingBox32 XdxrLoadBox32(uint64_t vaOfNode32, uint boxIndex)
{
    uint64_t boxAddress = vaOfNode32 + XboxBvh4Node_OffsetToBounds + boxIndex * XdxrBoundingBox32_Size;

    XdxrBoundingBox32 ret;
    ret.min = asfloat(GpuVALoad3Offset(boxAddress, 0));
    ret.max = asfloat(GpuVALoad3Offset(boxAddress, 12));
    return ret;
}

inline
XdxrBoundingBox32 XdxrDecompressBox16(uint3 box16)
{
    XdxrBoundingBox32 box32;
    box32.min = f16tof32(uint3(box16.x, box16.x >> 16, box16.y));
    box32.max = f16tof32(uint3(box16.y >> 16, box16.z, box16.z >> 16));
    box32.min.x = __XB_Med3_F32(box32.min.x, -FLT_MAX, +FLT_MAX);
    box32.min.y = __XB_Med3_F32(box32.min.y, -FLT_MAX, +FLT_MAX);
    box32.min.z = __XB_Med3_F32(box32.min.z, -FLT_MAX, +FLT_MAX);
    box32.max.x = __XB_Med3_F32(box32.max.x, -FLT_MAX, +FLT_MAX);
    box32.max.y = __XB_Med3_F32(box32.max.y, -FLT_MAX, +FLT_MAX);
    box32.max.z = __XB_Med3_F32(box32.max.z, -FLT_MAX, +FLT_MAX);
    return box32;
}

inline
XdxrBoundingBox32 XdxrLoadBox16(uint64_t vaOfNode16, uint boxIndex)
{
    uint64_t boxAddress = vaOfNode16 + XboxBvh4Node_OffsetToBounds + boxIndex * XdxrBoundingBox16_Size;
    return XdxrDecompressBox16(GpuVALoad3(boxAddress));
}


inline
XdxrBoundingBox32 XdxrLoadBox(
    uint64_t nodeVA,
    uint nodeType,
    uint boxIndex)
{
    if (nodeType == XDXR_NODE_TYPE_BOX32)
        return XdxrLoadBox32(nodeVA, boxIndex);

    return XdxrLoadBox16(nodeVA, boxIndex);
}





inline
uint64_t GpuVALoadPointer(
    uint64_t address)
{
    return Uint2ToGpuVA(GpuVALoad2(address));
}

inline
uint64_t GpuVALoadPointer(
    ByteAddressBuffer buf,
    uint offset)
{
    return Uint2ToGpuVA(buf.Load2(offset));
}


inline
void GpuVALoadInstanceTransformUint(
    uint4 outMat[3],
    uint64_t address)
{
    outMat[0] = GpuVALoad4(address);
    outMat[1] = GpuVALoad4Offset(address, 16);
    outMat[2] = GpuVALoad4Offset(address, 32);
}

inline
void GpuVALoadInstanceTransformFloat(
    float3x4& outMat,
    uint64_t address)
{
    uint4 transform[3];
    GpuVALoadInstanceTransformUint(transform, address);

    outMat = float3x4(asfloat(transform[0]),
                      asfloat(transform[1]),
                      asfloat(transform[2]));
}

inline
uint GpuVAToNodePointer(
    uint64_t vaOfNode,
    uint64_t vaBVHStart)
{
    return XdxrOffsetToBvhNodePtr(uint(vaOfNode - vaBVHStart), XDXR_NODE_TYPE_TRI0 );
}
inline bool IsBoxDegenerate(XdxrBoundingBox32 box)
{
    uint numNonZeroExtents = 0;
    numNonZeroExtents += (box.max.x > box.min.x) ? 1 : 0;
    numNonZeroExtents += (box.max.y > box.min.y) ? 1 : 0;
    numNonZeroExtents += (box.max.z > box.min.z) ? 1 : 0;

    return numNonZeroExtents < 2;
}

inline
void ExpandBox(
    XdxrBoundingBox32& a,
    XdxrBoundingBox32 b)
{
    a.min = min(a.min, b.min);
    a.max = max(a.max, b.max);
}

inline
void ExpandBox(
    XdxrBoundingBox32& a,
    float3 v)
{
    a.min = min(a.min, v);
    a.max = max(a.max, v);
}

inline
XdxrBoundingBox32 IntersectBox(
    XdxrBoundingBox32 a,
    XdxrBoundingBox32 b)
{
    XdxrBoundingBox32 outputAABB;

    outputAABB.min = max(a.min, b.min);
    outputAABB.max = min(a.max, b.max);

    return outputAABB;
}

inline
float3x4 InverseAffineTransform(
    float3x4 xform,
    float& det)
{

    const float3 row0 = float3(xform[0][0], xform[1][0], xform[2][0]);
    const float3 row1 = float3(xform[0][1], xform[1][1], xform[2][1]);
    const float3 row2 = float3(xform[0][2], xform[1][2], xform[2][2]);
    const float3 row3 = float3(xform[0][3], xform[1][3], xform[2][3]);

    const float3x3 adjoint3x3 = float3x3(
        cross(row1, row2),
        cross(row2, row0),
        cross(row0, row1));


    det = dot(adjoint3x3[0], row0);
    const float invDet = (det == 0.0f) ? 0.0f : (1.0f / det);


    const float3x3 inverse3x3 = adjoint3x3 * invDet;


    return float3x4(
        float4(inverse3x3[0], -dot(inverse3x3[0], row3)),
        float4(inverse3x3[1], -dot(inverse3x3[1], row3)),
        float4(inverse3x3[2], -dot(inverse3x3[2], row3)));
}





inline
float3x4 MultiplyAffineTransforms(float3x4 A, float3x4 B)
{
    const float4x3 BT = transpose(B);

    float3x4 ret;

    ret[0][0] = dot(A[0], float4(BT[0], 0));
    ret[1][0] = dot(A[1], float4(BT[0], 0));
    ret[2][0] = dot(A[2], float4(BT[0], 0));

    ret[0][1] = dot(A[0], float4(BT[1], 0));
    ret[1][1] = dot(A[1], float4(BT[1], 0));
    ret[2][1] = dot(A[2], float4(BT[1], 0));

    ret[0][2] = dot(A[0], float4(BT[2], 0));
    ret[1][2] = dot(A[1], float4(BT[2], 0));
    ret[2][2] = dot(A[2], float4(BT[2], 0));

    ret[0][3] = dot(A[0], float4(BT[3], 1));
    ret[1][3] = dot(A[1], float4(BT[3], 1));
    ret[2][3] = dot(A[2], float4(BT[3], 1));

    return ret;
}

inline
void CompressWorldToObject(
    float3x4& worldToObject,
    float3 objectToLocalScale,
    float3 objectCenter,
    uint rotationMatrixIdx)
{
    if (rotationMatrixIdx != 0)
    {
        float3x3 rotMat = GetRotationMatrix(rotationMatrixIdx);
        float3x4 affineRotation = float3x4(
            float4(rotMat[0], 0),
            float4(rotMat[1], 0),
            float4(rotMat[2], 0));

        worldToObject = MultiplyAffineTransforms(affineRotation, worldToObject);
    }

    worldToObject[0][3] -= objectCenter.x;
    worldToObject[1][3] -= objectCenter.y;
    worldToObject[2][3] -= objectCenter.z;
    worldToObject[0] *= objectToLocalScale.x;
    worldToObject[1] *= objectToLocalScale.y;
    worldToObject[2] *= objectToLocalScale.z;
}

inline
float3x4 DecompressObjectToWorld(
    float3x4 objectToWorld,
    float3 localToObjectScale,
    float3 objectCenter,
    uint rotationMatrixIdx)
{
    float3x4 localToObject = float3x4(
        float4(localToObjectScale.x, 0, 0, objectCenter.x),
        float4(0, localToObjectScale.y, 0, objectCenter.y),
        float4(0, 0, localToObjectScale.z, objectCenter.z)
    );

    if (rotationMatrixIdx != 0)
    {
        float3x3 rotMat = GetInverseRotationMatrix(rotationMatrixIdx);
        float3x4 affineRotation = float3x4(
            float4(rotMat[0], 0),
            float4(rotMat[1], 0),
            float4(rotMat[2], 0));

        localToObject = MultiplyAffineTransforms(affineRotation, localToObject);
    }

    return MultiplyAffineTransforms(objectToWorld, localToObject);
}

inline
XdxrBoundingBox32 TransformAABB(
    XdxrBoundingBox32 box,
    float3x4 transform)
{



    const uint verticesPerAABB = 8;
                float4 boxVertices[verticesPerAABB];
    boxVertices[0] = float4(box.min.x, box.min.y, box.min.z, 1.0);
    boxVertices[1] = float4(box.min.x, box.min.y, box.max.z, 1.0);
    boxVertices[2] = float4(box.min.x, box.max.y, box.max.z, 1.0);
    boxVertices[3] = float4(box.min.x, box.max.y, box.min.z, 1.0);
    boxVertices[4] = float4(box.max.x, box.min.y, box.min.z, 1.0);
    boxVertices[6] = float4(box.max.x, box.min.y, box.max.z, 1.0);
    boxVertices[5] = float4(box.max.x, box.max.y, box.min.z, 1.0);
    boxVertices[7] = float4(box.max.x, box.max.y, box.max.z, 1.0);

    XdxrBoundingBox32 transformedBox;
    transformedBox.min = float3(+FLT_MAX, +FLT_MAX, +FLT_MAX);
    transformedBox.max = float3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (uint i = 0; i < verticesPerAABB; i++)
    {
                    float3 tranformedVertex = mul(transform, boxVertices[i]);
        transformedBox.min = min(transformedBox.min, tranformedVertex);
        transformedBox.max = max(transformedBox.max, tranformedVertex);
    }
    return transformedBox;
}

inline
XdxrBoundingBox32 ComputeInstanceBoundsChildNoTriangleNoNullNoKdop(
    uint64_t vaBLAS,
    uint parentNodePtr,
    uint boxIndex,
    float3x4 geomToWorld)
{
    const uint nodeType = XdxrTypeOfNodePointer(parentNodePtr);

    const uint64_t vaOfRootNode = XdxrNodePointerToVA(vaBLAS, parentNodePtr);

    XdxrBoundingBox32 box = XdxrLoadBox(vaOfRootNode, nodeType, boxIndex);

    XdxrBoundingBox32 aabb = TransformAABB(box, geomToWorld);


    aabb.min = clamp(aabb.min, -FLT_MAX, FLT_MAX);
    aabb.max = clamp(aabb.max, -FLT_MAX, FLT_MAX);

    return aabb;
}

inline
uint ComputeMaxSplitNodes(
    uint64_t vaBlas,
    uint rootNodePtr,
    uint rebraidMode)
{

    const uint nodeType = XdxrTypeOfNodePointer(rootNodePtr);
    if (nodeType <= XDXR_NODE_TYPE_TRI3)
        return 1;


    if (nodeType == XDXR_NODE_TYPE_AABB_GEOMETRY)
        return 1;


    if (rebraidMode == 0)
        return 1;


    const uint64_t vaOfRootNode = XdxrNodePointerToVA(vaBlas, rootNodePtr);
    const uint4 nodePtrs = GpuVALoad4(vaOfRootNode);

    for (int i=0; i < 4; ++i)
    {
        if (XdxrIsValidNodePointer(nodePtrs[i]) &&
            (XdxrTypeOfNodePointer(nodePtrs[i]) != XDXR_NODE_TYPE_BOX16 &&
             XdxrTypeOfNodePointer(nodePtrs[i]) != XDXR_NODE_TYPE_BOX32))
        {
            return 1;
        }
    }

    uint numValid = 0;

    if (rebraidMode == D3D12XBOX_RAYTRACING_INSTANCE_FLAG_TAPU4)
    {
        for (int i=0; i < 4; ++i)
        {
            if (XdxrIsValidNodePointer(nodePtrs[i]))
                numValid++;
        }
    }
    else if (rebraidMode == D3D12XBOX_RAYTRACING_INSTANCE_FLAG_TAPU16)
    {
        for (int i=0; i < 4; ++i)
        {
            if (XdxrIsValidNodePointer(nodePtrs[i]))
            {
                const uint64_t vaOfRootNode = XdxrNodePointerToVA(vaBlas, nodePtrs[i]);
                const uint4 nodePtrsA = GpuVALoad4(vaOfRootNode);

                int j;
                for (j=0; j < 4; ++j)
                {
                    if (XdxrIsValidNodePointer(nodePtrsA[j]) &&
                        (XdxrTypeOfNodePointer(nodePtrsA[j]) != XDXR_NODE_TYPE_BOX16 &&
                         XdxrTypeOfNodePointer(nodePtrsA[j]) != XDXR_NODE_TYPE_BOX32))
                    {
                        return 1;
                    }
                }

                for (j=0; j < 4; ++j)
                {
                    if (XdxrIsValidNodePointer(nodePtrsA[j]))
                        numValid++;
                }
            }
        }
    }

    return numValid;
}

inline
XdxrBoundingBox32 XdxrLoadBox32(
    uint64_t va)
{
    XdxrBoundingBox32 b;
    b.min = asfloat(GpuVALoad3Offset(va, 0));
    b.max = asfloat(GpuVALoad3Offset(va, 12));
    return b;
}

inline
float3 TransformPoint(float3x4 xform, float3 pt)
{
                float3 ret = mul(xform, float4(pt, 1));
    return ret;
}

inline
float3 TransformVector(float3x4 xform, float3 pt)
{
                float3 ret = mul(xform, float4(pt, 0));
    return ret;
}

inline
float SurfaceArea(XdxrBoundingBox32 a)
{
    const float sx = a.max.x - a.min.x;
    const float sy = a.max.y - a.min.y;
    const float sz = a.max.z - a.min.z;

    return sx * sy + sx * sz + sy * sz;
}


inline
uint FloatToSortableUint(float f)
{
    uint u = asuint(f);
    uint mask = -int(u >> 31) | 0x80000000;
 return u ^ mask;
}


inline
float SortableUintToFloat(uint u)
{
    uint mask = ((u >> 31) - 1) | 0x80000000;
 return asfloat(u ^ mask);
}




struct XdxrHlslRuntime
{

    float3 WorldRayDirection;
    float3 WorldRayOrigin;
    float RayTMin;
    uint Tlas256BAligned;


    uint RayFlags;
    uint PackedMaskAndIndices;


    uint CommittedInstanceNodePtr;
    uint CommittedBlasLeafNodePtr;
    uint CommittedReserved_HitKind;
    float CommittedRayTCurrent;
    uint CommittedAttrDwords[2];


    uint PendingInstanceNodePtr;
    uint PendingBlasLeafNodePtr;
    uint PendingAnyHitResult_HitKind;
    float PendingRayTCurrent;
    uint PendingAttrDwords[2];


};

extern __declspec(selectany)
XdxrHlslRuntime g_xdxrRuntimeData;



inline
float XDXR_RayTMin()
{
    return g_xdxrRuntimeData.RayTMin;
}


inline
void XDXR_SetPendingInstanceNodePtr(
    uint nodeptr)
{
    g_xdxrRuntimeData.PendingInstanceNodePtr = nodeptr;
}

inline
uint XDXR_GetPendingInstanceNodePtr()
{
    return g_xdxrRuntimeData.PendingInstanceNodePtr;
}

inline
void XDXR_SetCommittedInstanceNodePtr(
    uint nodeptr)
{
    g_xdxrRuntimeData.CommittedInstanceNodePtr = nodeptr;
}

inline
float XDXR_RayTCurrent()
{
    return g_xdxrRuntimeData.CommittedRayTCurrent;
}


inline
void XDXR_SetPendingAttr(
    BuiltInTriangleIntersectionAttributes attr_t)
{
    g_xdxrRuntimeData.PendingAttrDwords[0] = asuint(attr_t.barycentrics.x);
    g_xdxrRuntimeData.PendingAttrDwords[1] = asuint(attr_t.barycentrics.y);
}

inline
void XDXR_SetPendingRayTCurrent(
    float t)
{
    g_xdxrRuntimeData.PendingRayTCurrent = t;
}

inline
float XDXR_GetPendingRayTCurrent()
{
    return g_xdxrRuntimeData.PendingRayTCurrent;
}

inline
void XDXR_SetPendingHitKind(
    uint hitKind)
{
    g_xdxrRuntimeData.PendingAnyHitResult_HitKind = hitKind;
}

inline
void XDXR_CommitHit()
{
    g_xdxrRuntimeData.CommittedBlasLeafNodePtr = g_xdxrRuntimeData.PendingBlasLeafNodePtr;
    g_xdxrRuntimeData.CommittedInstanceNodePtr = g_xdxrRuntimeData.PendingInstanceNodePtr;
    g_xdxrRuntimeData.CommittedAttrDwords[0] = g_xdxrRuntimeData.PendingAttrDwords[0];
    g_xdxrRuntimeData.CommittedAttrDwords[1] = g_xdxrRuntimeData.PendingAttrDwords[1];
    g_xdxrRuntimeData.CommittedRayTCurrent = g_xdxrRuntimeData.PendingRayTCurrent;
    g_xdxrRuntimeData.CommittedReserved_HitKind = g_xdxrRuntimeData.PendingAnyHitResult_HitKind;
}

inline
uint XDXR_GetCommittedInstanceNodePtr()
{
    return g_xdxrRuntimeData.CommittedInstanceNodePtr;
}

inline
void XDXR_SetPendingBlasLeafNodePtr(
    uint blasLeafNodePtr)
{
    g_xdxrRuntimeData.PendingBlasLeafNodePtr = blasLeafNodePtr;
}


inline
uint XDXR_GetPendingBlasLeafNodePtr()
{
    return g_xdxrRuntimeData.PendingBlasLeafNodePtr;
}


inline
void XDXR_SetCommittedBlasLeafNodePtr(
    uint blasLeafNodePtr)
{
    g_xdxrRuntimeData.CommittedBlasLeafNodePtr = blasLeafNodePtr;
}

inline
uint XDXR_GetCommittedBlasLeafNodePtr()
{
    return g_xdxrRuntimeData.CommittedBlasLeafNodePtr;
}

inline
void XDXR_SetPendingAttributeDword(
    uint index,
    uint v)
{
    if (index < 2)
        g_xdxrRuntimeData.PendingAttrDwords[index] = v;
}

inline
void XDXR_SetCommittedAttributeDword(
    uint index,
    uint v)
{
    if (index < 2)
        g_xdxrRuntimeData.CommittedAttrDwords[index] = v;
}

inline
uint XDXR_GetPendingAttributeDword(
    uint index)
{
    return g_xdxrRuntimeData.PendingAttrDwords[index];
}

inline
uint XDXR_GetCommittedAttributeDword(
    uint index)
{
    return g_xdxrRuntimeData.CommittedAttrDwords[index];
}

inline
uint XDXR_GetRayFlags()
{
    return g_xdxrRuntimeData.RayFlags;
}

inline
float XDXR_WorldRayDirection(
    uint i)
{
    return g_xdxrRuntimeData.WorldRayDirection[i];
}

inline
float XDXR_WorldRayOrigin(
    uint i)
{
    return g_xdxrRuntimeData.WorldRayOrigin[i];
}


inline
uint64_t XDXR_GetTLASPtr()
{
    return (uint64_t)g_xdxrRuntimeData.Tlas256BAligned << 8;
}


inline
uint XDXR_PackedMaskAndIndices()
{
    return g_xdxrRuntimeData.PackedMaskAndIndices;
}
static uint g_hitGroupShaderTableStride = 0;
static uint g_pendingShaderRecordOffset = 0;
static uint g_commitedShaderRecordOffset = 0;


inline
uint XDXR_HitGroupShTableStride()
{
    return g_hitGroupShaderTableStride;
}

inline
void XDXR_SetPendingShaderRecordOffset(
    uint shaderRecordOffset)
{
    g_pendingShaderRecordOffset = shaderRecordOffset;
}

inline
void XDXR_SetCommittedShaderRecordOffset(
    uint shaderRecordOffset)
{
    g_commitedShaderRecordOffset = shaderRecordOffset;
}

inline
uint XDXR_GetPendingShaderRecordOffset()
{
    return g_pendingShaderRecordOffset;
}

inline
uint XDXR_GetCommittedShaderRecordOffset()
{
    return g_commitedShaderRecordOffset;
}
uint XDXR_Runtime_Callback_PackTraceRayParams2345(uint instanceInclusionMask, uint rayContributionToHitGroupIndex, uint multiplierForGeometryContributionToHitGroupIndex, uint missShaderIndex);

inline
void XDXR_SetupHlslRuntime(

    uint64_t tlas,



    uint InstanceInclusionMask,
    uint RayFlags,
    float3 RayOrigin,
    float3 RayDirection,
    float RayTMin,
    float RayTMax)
{

    g_xdxrRuntimeData.Tlas256BAligned = (uint)(tlas >> 8);



    g_xdxrRuntimeData.RayFlags = RayFlags;

    g_xdxrRuntimeData.RayTMin = RayTMin;
    g_xdxrRuntimeData.CommittedRayTCurrent = RayTMax;

    g_xdxrRuntimeData.WorldRayOrigin = RayOrigin;
    g_xdxrRuntimeData.WorldRayDirection = RayDirection;

    g_xdxrRuntimeData.PendingInstanceNodePtr = 0;
    g_xdxrRuntimeData.PendingBlasLeafNodePtr = 0;
    g_xdxrRuntimeData.CommittedInstanceNodePtr = 0;
    g_xdxrRuntimeData.CommittedBlasLeafNodePtr = 0;

    uint rayContributionToHitGroupIndex = 0;
    uint multiplierForGeometryContributionToHitGroupIndex = 0;
    uint missShaderIndex = 0;

    g_xdxrRuntimeData.PackedMaskAndIndices =
        XDXR_Runtime_Callback_PackTraceRayParams2345(InstanceInclusionMask,
            rayContributionToHitGroupIndex,
            multiplierForGeometryContributionToHitGroupIndex,
            missShaderIndex);
}





inline uint XDXR_Runtime_Callback_PackTraceRayParams2345(
    uint instanceInclusionMask,
    uint rayContributionToHitGroupIndex,
    uint multiplierForGeometryContributionToHitGroupIndex,
    uint missShaderIndex)
{
    return ((instanceInclusionMask & 0xff) << 0) |
           ((rayContributionToHitGroupIndex & 0xf) << 8) |
           ((multiplierForGeometryContributionToHitGroupIndex & 0xf) << 12) |
           ((missShaderIndex & 0xffff) << 16);
}


inline
void UnpackTraceRayParams2345(
    uint& instanceInclusionMask,
    uint& rayContributionToHitGroupIndex,
    uint& multiplierForGeometryContributionToHitGroupIndex,
    uint& missShaderIndex,
    uint packed)
{
    instanceInclusionMask = (packed >> 0) & 0xff;
    rayContributionToHitGroupIndex = (packed >> 8) & 0xf;
    multiplierForGeometryContributionToHitGroupIndex = (packed >> 12) & 0xf;
    missShaderIndex = (packed >> 16) & 0xffff;
}





inline uint64_t XDXR_Runtime_Callback_PackTraceRayParams01(
    uint64_t tlas,
    uint rayFlags)
{
    return ((tlas & 0xffffffff00ull) >> 8) |
           (((uint64_t)(rayFlags)) << 32);
}


                inline
uint XDXR_Runtime_Callback_UnpackTraceRayParams01RayFlags(
    uint64_t tlasAndRayFlags)
{
    return (uint)(tlasAndRayFlags >> 32);
}

inline
uint64_t UnpackTraceRayParams01Tlas(
    uint64_t tlasAndRayFlags)
{
    uint p = (uint)tlasAndRayFlags;
    return ((uint64_t)p) << 8;
}

                inline
uint64_t XDXR_Runtime_Callback_UnpackTraceRayParams01Tlas(
    uint64_t tlasAndRayFlags)
{
    return UnpackTraceRayParams01Tlas(tlasAndRayFlags);
}

inline
float3x4 GetWorldToObjectMatrix(
    uint64_t instanceVA)
{

    uint4 instanceTransform[3];
    GpuVALoadInstanceTransformUint(instanceTransform, instanceVA + XboxBvh4Instance_OffsetToWorldToObject);

    uint32_t compressionParams = GpuVALoad1(instanceVA + XboxBvh4Instance_OffsetToCompressionParams);


    float3x4 worldToObject =
        XdxrMatrix_UnpackInstanceWorldToObject(instanceTransform[0], instanceTransform[1], instanceTransform[2]);


    if (BvhIsScaled(compressionParams))
    {
        float3 localToObjectScale = UnpackRcpScaleExponents(compressionParams);
        worldToObject[0] *= localToObjectScale.x;
        worldToObject[1] *= localToObjectScale.y;
        worldToObject[2] *= localToObjectScale.z;
    }

    if (BvhIsTranslated(compressionParams))
    {
        uint32_t vaBLAS_aligned = GpuVALoad1(instanceVA + XboxBvh4Instance_OffsetToBlasPointer);
        uint64_t vaBLAS = uint64_t(vaBLAS_aligned) << 8ull;
        float3 minBound = asfloat(GpuVALoad3(vaBLAS + XboxBvh4Header_OffsetToSceneBounds_Min_X));
        float3 maxBound = asfloat(GpuVALoad3(vaBLAS + XboxBvh4Header_OffsetToSceneBounds_Max_X));
        float3 objectCenter = BvhGetTranslation(minBound, maxBound);
        worldToObject[0][3] += objectCenter.x;
        worldToObject[1][3] += objectCenter.y;
        worldToObject[2][3] += objectCenter.z;
    }

    if (BvhIsRotated(compressionParams))
    {
        float3x3 rotMat = GetInverseRotationMatrix(BvhGetRotationIndex(compressionParams));
        float3x4 affineRotation = float3x4(
            float4(rotMat[0], 0),
            float4(rotMat[1], 0),
            float4(rotMat[2], 0));

        worldToObject = MultiplyAffineTransforms(affineRotation, worldToObject);
    }

    return worldToObject;
}
inline
float3 XDXR_Runtime_Callback_GetObjectRayOrigin(
    uint64_t tlasPtrPacked,
    uint instanceNodePtr)
{
    uint64_t instanceVA = XdxrNodePointerToVA(UnpackTraceRayParams01Tlas(tlasPtrPacked), instanceNodePtr);

    const float3x4 worldToObject = GetWorldToObjectMatrix(instanceVA);

    const float4 worldRayOrigin = float4(
        XDXR_WorldRayOrigin(0),
        XDXR_WorldRayOrigin(1),
        XDXR_WorldRayOrigin(2),
        1);

    return mul(worldToObject, worldRayOrigin);
}



inline
float3 XDXR_Runtime_Callback_GetObjectRayDirection(
    uint64_t tlasPtrPacked,
    uint instanceNodePtr)
{
    uint64_t instanceVA = XdxrNodePointerToVA(UnpackTraceRayParams01Tlas(tlasPtrPacked), instanceNodePtr);

    const float3x4 worldToObject = GetWorldToObjectMatrix(instanceVA);

    const float4 worldRayDirection = float4(
        XDXR_WorldRayDirection(0),
        XDXR_WorldRayDirection(1),
        XDXR_WorldRayDirection(2),
        0);

    return mul(worldToObject, worldRayDirection);
}



inline
float3x4 XDXR_Runtime_Callback_GetObjectToWorld(
    uint64_t tlasPtrPacked,
    uint instanceNodePtr)
{
    uint64_t instanceVA = XdxrNodePointerToVA(UnpackTraceRayParams01Tlas(tlasPtrPacked), instanceNodePtr);

    float3x4 objectToWorld;
    GpuVALoadInstanceTransformFloat(objectToWorld, instanceVA + XboxBvh4Instance_OffsetToObjectToWorld);

    return objectToWorld;
}


inline
float3x4 XDXR_Runtime_Callback_GetWorldToObject(
    uint64_t tlasPtrPacked,
    uint instanceNodePtr)
{
    uint64_t instanceVA = XdxrNodePointerToVA(UnpackTraceRayParams01Tlas(tlasPtrPacked), instanceNodePtr);

    return GetWorldToObjectMatrix(instanceVA);
}


inline
uint XDXR_Runtime_Callback_GetInstanceID(
    uint64_t tlasPtrPacked,
    uint instanceNodePtr)
{
    uint64_t instanceVA = XdxrNodePointerToVA(UnpackTraceRayParams01Tlas(tlasPtrPacked), instanceNodePtr);
    const uint4 metadata1 = GpuVALoad4Offset(instanceVA, XboxBvh4Instance_OffsetToInstanceMetaData1);
    const uint InstanceID = metadata1.x & InstanceIDAndMask_Mask_InstanceID;
    return InstanceID;
}

inline
uint XDXR_Runtime_Callback_GetInstanceIndex(
    uint64_t tlasPtrPacked,
    uint instanceNodePtr)
{
    const uint64_t tlas = UnpackTraceRayParams01Tlas(tlasPtrPacked);
    const uint instanceIndex = XboxBvh4Instance_GetInstanceIndexFromNodePtr(tlas, instanceNodePtr);
    return instanceIndex;
}




inline
uint XDXR_Runtime_Callback_GetGeometryIndex(
    uint64_t tlasPtrPacked,
    uint instanceNodePtr,
    uint blasLeafNodePtr)
{
    if (!instanceNodePtr ||
        !blasLeafNodePtr)
    {
        return 0;
    }


    const uint64_t vaTLAS = UnpackTraceRayParams01Tlas(tlasPtrPacked);
    const uint64_t vaInstance = XdxrNodePointerToVA(vaTLAS, instanceNodePtr);


    const uint64_t vaBVH = XboxBvh4Instance_GetBlasPointerFromMetadata(vaInstance);


    const uint64_t vaLeaf = XdxrNodePointerToVA(vaBVH, blasLeafNodePtr);


    const uint geometryIndexAndFlags = GpuVALoad1Offset(vaLeaf, XboxBvh4Leaf_OffsetToGeometryIndexAndFlags);


    const uint geometryContributionToHitGroupIndex =
        geometryIndexAndFlags >> GeometryIndexAndFlags_Shift_GeometryIndex;

    return geometryContributionToHitGroupIndex;
}



inline
uint XDXR_Runtime_Callback_GetPrimitiveIndex(
    uint64_t tlasPtrPacked,
    uint instanceNodePtr,
    uint blasLeafNodePtr)
{
    if (!instanceNodePtr ||
        !blasLeafNodePtr)
    {
        return -1;
    }


    const uint64_t vaTLAS = UnpackTraceRayParams01Tlas(tlasPtrPacked);
    const uint64_t vaInstance = XdxrNodePointerToVA(vaTLAS, instanceNodePtr);


    const uint64_t vaBVH = XboxBvh4Instance_GetBlasPointerFromMetadata(vaInstance);


    const uint64_t vaLeaf = XdxrNodePointerToVA(vaBVH, blasLeafNodePtr);


    const uint nodeType = XdxrTypeOfNodePointer(blasLeafNodePtr);
    const uint indexToPrimitiveIndex = nodeType < XDXR_NODE_TYPE_TRI2 ? nodeType : 0;

    const uint primitiveIndexFromMetadata =
        GpuVALoad1Offset(vaLeaf, XboxBvh4Leaf_OffsetToBasePrimitiveIndex + indexToPrimitiveIndex * 4);

    return primitiveIndexFromMetadata;
}









static const uint XDXR_FUNCTION_TABLES_ENTRY_INDEX_WORST_CASE_TRAVERSAL = 0;
static const uint XDXR_FUNCTION_TABLES_ENTRY_INDEX_SHORT_CIRCUIT = 1;
static const uint XDXR_FUNCTION_TABLES_ENTRY_INDEX_WORST_CASE_HYBRID_TRAVERSAL = 2;
static const uint XDXR_FUNCTION_TABLES_NUM_ENTRIES = 3;




                inline
XdxrStateId_t XDXR_Runtime_Callback_SelectTraverseFunction(
    uint functionPointersTableVgpr,
    uint64_t packedTlasPointerAndFlags,


    uint packedMaskAndIndices)
{
    uint64_t tlas;
    uint rayFlags;
    uint instanceInclusionMask;
    uint rayContributionToHitGroupIndex;
    uint multiplierForGeometryContributionToHitGroupIndex;
    uint missShaderIndex;

    tlas = UnpackTraceRayParams01Tlas(packedTlasPointerAndFlags);
    rayFlags = XDXR_Runtime_Callback_UnpackTraceRayParams01RayFlags(packedTlasPointerAndFlags);
    UnpackTraceRayParams2345(instanceInclusionMask,
        rayContributionToHitGroupIndex,
        multiplierForGeometryContributionToHitGroupIndex,
        missShaderIndex,
        packedMaskAndIndices);


    uint index = XDXR_FUNCTION_TABLES_ENTRY_INDEX_WORST_CASE_TRAVERSAL;


    if (WaveActiveAllEqual(rayFlags))
    {

        rayFlags = WaveReadLaneFirst(rayFlags);

        if (rayFlags == 0x3)
            index = XDXR_FUNCTION_TABLES_ENTRY_INDEX_SHORT_CIRCUIT;
        if (rayFlags & D3D12XBOX_RAY_FLAG_COHERENT_RAYS_HINT)
            index = XDXR_FUNCTION_TABLES_ENTRY_INDEX_WORST_CASE_HYBRID_TRAVERSAL;
    }




    const XdxrStateId_t stateId = (XdxrStateId_t)(__XB_ReadLane(functionPointersTableVgpr, index));
    return stateId;
}







inline
uint XDXR_Runtime_Callback_GetDispatchRaysIndex(
    uint rayIndex,
    uint rayIndexUnpacker16bits,
    uint i);
                inline
bool XDXR_GetHitTriangleVertexPositions(
    float3& v0,
    float3& v1,
    float3& v2,
    uint instanceNodePtr,
    uint leafNodePtr)
{

    const uint nodeType = XdxrTypeOfNodePointer(leafNodePtr);
    if (nodeType > XDXR_NODE_TYPE_TRI3)
        return false;


    const uint64_t tlasBasePtr = XDXR_GetTLASPtr();


    const uint64_t vaInstance = XdxrNodePointerToVA(tlasBasePtr, instanceNodePtr);


    const uint64_t vaBVH = XboxBvh4Instance_GetBlasPointerFromMetadata(vaInstance);


    const uint64_t nodePtr = XdxrNodePointerToVA(vaBVH, leafNodePtr);

    XboxBvh4Leaf_ExtractNodeVertexPositions(v0, v1, v2, nodePtr, nodeType);

    const uint32_t compressionParams = GpuVALoad1(vaBVH + XboxBvh4Header_OffsetToCompressionParams);
    if (BvhIsTransformed(compressionParams))
    {
        const float3 localToObjectScale = BvhGetDecompressionScale(compressionParams);
        v0 *= localToObjectScale;
        v1 *= localToObjectScale;
        v2 *= localToObjectScale;

        if (BvhIsTranslated(compressionParams))
        {
            const float3 sceneMin = asfloat(GpuVALoad3(vaBVH + XboxBvh4Header_OffsetToSceneBounds_Min_X));
            const float3 sceneMax = asfloat(GpuVALoad3(vaBVH + XboxBvh4Header_OffsetToSceneBounds_Max_X));
            const float3 objectCenter = BvhGetTranslation(sceneMin, sceneMax);
            v0 += objectCenter;
            v1 += objectCenter;
            v2 += objectCenter;
        }

        uint32_t rotationIdx = BvhGetRotationIndex(compressionParams);
        if (rotationIdx)
        {
            float3x3 rotMat = GetInverseRotationMatrix(rotationIdx);
            v0 = mul(rotMat, v0);
            v1 = mul(rotMat, v1);
            v2 = mul(rotMat, v2);
        }
    }

    return true;
}







                inline
bool XDXR_GetCommittedHitTriangleVertexPositions(
    float3& v0,
    float3& v1,
    float3& v2)
{

    const uint instanceNodePtr = XDXR_GetCommittedInstanceNodePtr();
    const uint leafNodePtr = XDXR_GetCommittedBlasLeafNodePtr();


    if (!instanceNodePtr ||
        !leafNodePtr)
    {
        return false;
    }

    return XDXR_GetHitTriangleVertexPositions(v0, v1, v2, instanceNodePtr, leafNodePtr);
}







                inline
bool XDXR_GetPendingHitTriangleVertexPositions(
    float3& v0,
    float3& v1,
    float3& v2)
{

    const uint instanceNodePtr = XDXR_GetPendingInstanceNodePtr();
    const uint leafNodePtr = XDXR_GetPendingBlasLeafNodePtr();


    if (!instanceNodePtr ||
        !leafNodePtr)
    {
        return false;
    }

    return XDXR_GetHitTriangleVertexPositions(v0, v1, v2, instanceNodePtr, leafNodePtr);
}




                inline
void XDXR_SetShortCircuitIntersectNodes(
    uint instanceNodePtr,
    uint blasLeafNodePtr)
{
    XDXR_SetCommittedAttributeDword(0, instanceNodePtr);
    XDXR_SetCommittedAttributeDword(1, blasLeafNodePtr);
}


inline
uint XDXR_GetRayContributionToHitGroupIndex()
{
    uint instanceInclusionMask;
    uint rayContributionToHitGroupIndex;
    uint multiplierForGeometryContributionToHitGroupIndex;
    uint missShaderIndex;

    UnpackTraceRayParams2345(instanceInclusionMask,
        rayContributionToHitGroupIndex,
        multiplierForGeometryContributionToHitGroupIndex,
        missShaderIndex,
        XDXR_PackedMaskAndIndices());

    return rayContributionToHitGroupIndex;
}


inline
uint XDXR_GetInstanceInclusionMask()
{
    uint instanceInclusionMask;
    uint rayContributionToHitGroupIndex;
    uint multiplierForGeometryContributionToHitGroupIndex;
    uint missShaderIndex;

    UnpackTraceRayParams2345(instanceInclusionMask,
        rayContributionToHitGroupIndex,
        multiplierForGeometryContributionToHitGroupIndex,
        missShaderIndex,
        XDXR_PackedMaskAndIndices());

    return instanceInclusionMask;
}


inline
uint XDXR_GetMultiplierForGeometryContributionToHitGroupIndex()
{
    uint instanceInclusionMask;
    uint rayContributionToHitGroupIndex;
    uint multiplierForGeometryContributionToHitGroupIndex;
    uint missShaderIndex;

    UnpackTraceRayParams2345(instanceInclusionMask,
        rayContributionToHitGroupIndex,
        multiplierForGeometryContributionToHitGroupIndex,
        missShaderIndex,
        XDXR_PackedMaskAndIndices());

    return multiplierForGeometryContributionToHitGroupIndex;
}


inline
uint XDXR_GetMissShaderIndex()
{
    uint instanceInclusionMask;
    uint rayContributionToHitGroupIndex;
    uint multiplierForGeometryContributionToHitGroupIndex;
    uint missShaderIndex;

    UnpackTraceRayParams2345(instanceInclusionMask,
        rayContributionToHitGroupIndex,
        multiplierForGeometryContributionToHitGroupIndex,
        missShaderIndex,
        XDXR_PackedMaskAndIndices());

    return missShaderIndex;
}






struct RayData
{
    float3 Origin;
    float3 Direction;
};




inline
void Read3Verts(
    uint64_t vaLeaf,
    uint triIdx,
    float3& v0,
    float3& v1,
    float3& v2)
{

    switch (triIdx)
    {
    default:
    case XDXR_NODE_TYPE_TRI0:
        v0 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V0));
        v1 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V1));
        v2 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V2));
        break;
    case XDXR_NODE_TYPE_TRI1:
        v0 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V1));
        v1 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V3));
        v2 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V2));
        break;
    case XDXR_NODE_TYPE_TRI2:
        v0 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V2));
        v1 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V3));
        v2 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V4));
        break;
    case XDXR_NODE_TYPE_TRI3:
        v0 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V2));
        v1 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V4));
        v2 = asfloat(GpuVALoad3Offset(vaLeaf, XDXR_OFFSET_OF_V0));
        break;
    }
}
struct CPUTraceLogEntry
{
    enum Type
    {
        RayStart,
        OuterLoopHit,
        InnerLoopHit,
        StepBegins,
        HandleBoxTests,
        MarkBlasEntry,
        BackToTlas,
        StackPop,
        NoteNode,
        BoxIntersection,
        RecursionLimit,
    };

    Type type;

    union
    {
        float t;
        unsigned u;

        struct
        {
            float pos[3];
            float dir[3];
            float mint;
            float maxt;
        } rayStart;

        struct
        {
            unsigned vaBVH_256BAligned;
            unsigned parentNode;
            unsigned rayTest0;
            unsigned rayTest1;
            unsigned rayTest2;
            unsigned rayTest3;
        } handleBoxTests;

        struct
        {
            unsigned tlasLeafPtr;
            unsigned vaTLAS_256BAligned;
            unsigned blasRootPtr;
            unsigned vaBLAS_256BAligned;
        } markBlasEntry;

        struct
        {
            unsigned nodePtr;
            unsigned loopCond;
            unsigned vaBVH_256BAligned;
            unsigned vaTLAS_256BAligned;
        } stackPop;

        struct
        {
            unsigned node;
            unsigned vaBVH_256BAligned;
        } noteNode;

        struct
        {
            float childBounds[4][6];
            unsigned pointers[4];
            float results[4];
            bool b16Bit;
        } boxIntersection;
    };
};


__declspec(selectany) std::vector<CPUTraceLogEntry> g_traverseFunctionCpuLog;
__declspec(selectany) bool g_traverseFunctionEnableLog;




inline
void CPUTraceLog_AddRayStart()
{
    if (!g_traverseFunctionEnableLog)
        return;

    g_traverseFunctionCpuLog.clear();

    char temp[1024];
    sprintf_s(temp, "[%d] RayStart: %f %f %f, %f %f %f, %f %f\n", (unsigned)g_traverseFunctionCpuLog.size(),
        XDXR_WorldRayOrigin(0), XDXR_WorldRayOrigin(1), XDXR_WorldRayOrigin(2),
        XDXR_WorldRayDirection(0), XDXR_WorldRayDirection(1), XDXR_WorldRayDirection(2),
        XDXR_RayTMin(), XDXR_RayTCurrent());
    OutputDebugStringA(temp);

    CPUTraceLogEntry e {};
    e.type = CPUTraceLogEntry::Type::RayStart;
    e.rayStart.pos[0] = XDXR_WorldRayOrigin(0);
    e.rayStart.pos[1] = XDXR_WorldRayOrigin(1);
    e.rayStart.pos[2] = XDXR_WorldRayOrigin(2);
    e.rayStart.dir[0] = XDXR_WorldRayDirection(0);
    e.rayStart.dir[1] = XDXR_WorldRayDirection(1);
    e.rayStart.dir[2] = XDXR_WorldRayDirection(2);
    e.rayStart.mint = XDXR_RayTMin();
    e.rayStart.maxt = XDXR_RayTCurrent();
    g_traverseFunctionCpuLog.push_back(e);
}


inline
void CPUTraceLog_OuterLoopHit(
    float t)
{
    if (!g_traverseFunctionEnableLog)
        return;

    char temp[1024];
    sprintf_s(temp, "Outer loop hit minT <- %f\n", t);
    OutputDebugStringA(temp);

    CPUTraceLogEntry e {};
    e.type = CPUTraceLogEntry::Type::OuterLoopHit;
    e.t = t;
    g_traverseFunctionCpuLog.push_back(e);
}

inline
void CPUTraceLog_InnerLoopHit(
    float t)
{
    if (!g_traverseFunctionEnableLog)
        return;

    char temp[1024];
    sprintf_s(temp, "Inner loop hit minT <- %f\n", t);
    OutputDebugStringA(temp);

    CPUTraceLogEntry e {};
    e.type = CPUTraceLogEntry::Type::InnerLoopHit;
    e.t = t;
    g_traverseFunctionCpuLog.push_back(e);
}

inline
void CPUTraceLog_StepBegins(
    float t)
{
    if (!g_traverseFunctionEnableLog)
        return;

    char temp[1024];
    sprintf_s(temp, "TraverseStep minT <- %f\n", t);
    OutputDebugStringA(temp);

    CPUTraceLogEntry e {};
    e.type = CPUTraceLogEntry::Type::StepBegins;
    e.t = t;
    g_traverseFunctionCpuLog.push_back(e);
}

inline
void CPUTraceLog_HandleBoxTests(
    unsigned vaBVH_256BAligned,
    unsigned parentNode,
    unsigned rayTest0,
    unsigned rayTest1,
    unsigned rayTest2,
    unsigned rayTest3)
{
    if (!g_traverseFunctionEnableLog)
        return;

    char temp[1024];
    sprintf_s(temp, "[%d] HandleBoxTests: parent: %x, children: %x %x %x %x\n ", (unsigned)g_traverseFunctionCpuLog.size(),
        parentNode, rayTest0, rayTest1, rayTest2, rayTest3);
    OutputDebugStringA(temp);

    CPUTraceLogEntry e {};
    e.type = CPUTraceLogEntry::Type::HandleBoxTests;
    e.handleBoxTests.vaBVH_256BAligned = vaBVH_256BAligned;
    e.handleBoxTests.parentNode = parentNode;
    e.handleBoxTests.rayTest0 = rayTest0;
    e.handleBoxTests.rayTest1 = rayTest1;
    e.handleBoxTests.rayTest2 = rayTest2;
    e.handleBoxTests.rayTest3 = rayTest3;
    g_traverseFunctionCpuLog.push_back(e);
}


inline
void CPUTraceLog_MarkBlasEntry(
    unsigned tlasLeafPtr,
    unsigned vaTLAS_256BAligned,
    unsigned blasRootPtr,
    unsigned vaBLAS_256BAligned)
{
    if (!g_traverseFunctionEnableLog)
        return;

    char temp[1024];
    sprintf_s(temp, "[%d] MarkBlasEntry: tlasLeafPtr: %x, vaTLAS_256: %x, blasRootPtr: %x, vaBLAS_256: %x\n", (unsigned)g_traverseFunctionCpuLog.size(),
        tlasLeafPtr, vaTLAS_256BAligned, blasRootPtr, vaBLAS_256BAligned);
    OutputDebugStringA(temp);

    CPUTraceLogEntry e {};
    e.type = CPUTraceLogEntry::Type::MarkBlasEntry;
    e.markBlasEntry.tlasLeafPtr = tlasLeafPtr;
    e.markBlasEntry.vaTLAS_256BAligned = vaTLAS_256BAligned;
    e.markBlasEntry.blasRootPtr = blasRootPtr;
    e.markBlasEntry.vaBLAS_256BAligned = vaBLAS_256BAligned;
    g_traverseFunctionCpuLog.push_back(e);
}


inline
void CPUTraceLog_BackToTlas(
    unsigned vaTLAS_256BAligned)
{
    if (!g_traverseFunctionEnableLog)
        return;

    char temp[1024];
    sprintf_s(temp, "[%d] BackToTlas: vaTLAS_256: %x\n", (unsigned)g_traverseFunctionCpuLog.size(),
        vaTLAS_256BAligned);
    OutputDebugStringA(temp);

    CPUTraceLogEntry e {};
    e.type = CPUTraceLogEntry::Type::BackToTlas;
    e.u = vaTLAS_256BAligned;
    g_traverseFunctionCpuLog.push_back(e);
}

inline
void CPUTraceLog_StackPop(
    unsigned nodePtr,
    unsigned loopCond,
    unsigned vaBVH_256BAligned,
    unsigned vaTLAS_256BAligned)
{
    if (!g_traverseFunctionEnableLog)
        return;

    char temp[1024];
    sprintf_s(temp, "[%d] StackPop: nodePtr: %x, loopCond: %d, vaBVH_256: %x, vaTLAS_256: %x\n", (unsigned)g_traverseFunctionCpuLog.size(),
        nodePtr, loopCond, vaBVH_256BAligned, vaTLAS_256BAligned);
    OutputDebugStringA(temp);

    CPUTraceLogEntry e {};
    e.type = CPUTraceLogEntry::Type::StackPop;
    e.stackPop.nodePtr = nodePtr;
    e.stackPop.loopCond = loopCond;
    e.stackPop.vaBVH_256BAligned = vaBVH_256BAligned;
    e.stackPop.vaTLAS_256BAligned = vaTLAS_256BAligned;
    g_traverseFunctionCpuLog.push_back(e);
}

inline
const char* CPUTraceLog_NodeTypeToStr(
    unsigned node)
{
    switch (node & 7)
    {
        default: return "---";
        case XDXR_NODE_TYPE_TRI0: return "tr0";
        case XDXR_NODE_TYPE_TRI1: return "tr1";
        case XDXR_NODE_TYPE_TRI2: return "tr2";
        case XDXR_NODE_TYPE_TRI3: return "tr3";
        case XDXR_NODE_TYPE_BOX16: return "b16";
        case XDXR_NODE_TYPE_BOX32: return "b32";
        case XDXR_NODE_TYPE_INSTANCE: return "ins";
        case XDXR_NODE_TYPE_AABB_GEOMETRY: return "box";
    }
}

inline
void CPUTraceLog_NoteNode(
    unsigned node,
    unsigned vaBVH_256BAligned)
{
    if (!g_traverseFunctionEnableLog)
        return;

    char temp[1024];
    sprintf_s(temp,
        "[%d] Node: nodePtr: %x (%s), vaBVH_256: %x\n",
        (unsigned)g_traverseFunctionCpuLog.size(),
        node,
        CPUTraceLog_NodeTypeToStr(node),
        vaBVH_256BAligned);
    OutputDebugStringA(temp);

    CPUTraceLogEntry e {};
    e.type = CPUTraceLogEntry::Type::NoteNode;
    e.noteNode.node = node;
    e.noteNode.vaBVH_256BAligned = vaBVH_256BAligned;
    g_traverseFunctionCpuLog.push_back(e);
}


inline
void CPUTraceLog_AddBoxIntersection(
    const float* childBounds,
    const unsigned* childPointers,
    const unsigned* pointers,
    const float* results,
    bool b16Bit)
{
    if (!g_traverseFunctionEnableLog)
        return;

    char temp[1024];
    sprintf_s(temp,
        "[%d] BoxIntersection:\n"
        "\t\t%x {%.2f, %.2f, %.2f ; %.2f, %.2f, %.2f},\n"
        "\t\t%x {%.2f, %.2f, %.2f ; %.2f, %.2f, %.2f},\n"
        "\t\t%x {%.2f, %.2f, %.2f ; %.2f, %.2f, %.2f},\n"
        "\t\t%x {%.2f, %.2f, %.2f ; %.2f, %.2f, %.2f},\n"
        "\t\tsorted distances/nodePtr %.2f/%x, %.2f/%x, %.2f/%x, %.2f/%x, b16=%d\n",
        (unsigned)g_traverseFunctionCpuLog.size(),

        childPointers[0], childBounds[0], childBounds[1], childBounds[2], childBounds[3], childBounds[4], childBounds[5],
        childPointers[1], childBounds[6], childBounds[7], childBounds[8], childBounds[9], childBounds[10], childBounds[11],
        childPointers[2], childBounds[12], childBounds[13], childBounds[14], childBounds[15], childBounds[16], childBounds[17],
        childPointers[3], childBounds[18], childBounds[19], childBounds[20], childBounds[21], childBounds[22], childBounds[23],

        results[0], pointers[0],
        results[1], pointers[1],
        results[2], pointers[2],
        results[3], pointers[3],

        b16Bit);
    OutputDebugStringA(temp);

    CPUTraceLogEntry e {};
    e.type = CPUTraceLogEntry::Type::BoxIntersection;
    e.boxIntersection.b16Bit = b16Bit;
    memcpy(e.boxIntersection.childBounds, childBounds, sizeof(e.boxIntersection.childBounds));
    memcpy(e.boxIntersection.pointers, pointers, sizeof(e.boxIntersection.pointers));
    memcpy(e.boxIntersection.results, results, sizeof(e.boxIntersection.results));
    g_traverseFunctionCpuLog.push_back(e);
}

inline
void CPUTraceLog_RecursionLimit(unsigned limit)
{
    if (!g_traverseFunctionEnableLog)
        return;

    char temp[1024];
    sprintf_s(temp, "Hit recursion limit of %u. Terminating intersection search.", limit);
    OutputDebugStringA(temp);

    CPUTraceLogEntry e {};
    e.type = CPUTraceLogEntry::Type::RecursionLimit;
    e.u = limit;
    g_traverseFunctionCpuLog.push_back(e);
}
inline
float RayBoxTest(
    float ray_extent,
    float3 ray_origin,
    float3 ray_dir,
    float3 ray_inv_dir,
    XdxrBoundingBox32 box)
{

    if (box.min[0] > box.max[0] ||
        box.min[1] > box.max[1] ||
        box.min[2] > box.max[2])
    {
        return asfloat(0x7F800000);
    }

    float tnear = 0.0;
    float tfar = ray_extent;

    for (uint i = 0; i < 3; ++i)
    {

        if (ray_dir[i] == 0.0)
        {
            if (ray_origin[i] < box.min[i] || ray_origin[i] > box.max[i])
                return asfloat(0x7F800000);
        }
        else
        {

            float t1 = (box.min[i] - ray_origin[i]) * ray_inv_dir[i];
            float t2 = (box.max[i] - ray_origin[i]) * ray_inv_dir[i];


            tnear = max(tnear, min(t1, t2));
            tfar = min(tfar, max(t1, t2));


            if (tfar < tnear)
                return asfloat(0x7F800000);
        }
    }

    return tnear;
}

inline
void Swap(float& a, float& b)
{
    float t = a; a = b; b = t;
}


inline
void Swap(uint& a, uint& b)
{
    uint t = a; a = b; b = t;
}

inline
float SoftwareIntersectFirstBox32Child(
    uint64_t nodePtr,
    float3 rayOrigin,
    float3 rayDirection,
    float3 rayInverseDirection,
    float rayExtent)
{
    const uint64_t vaNode = (nodePtr >> 3) << 6;
    const uint nodeType = XdxrTypeOfNodePointer((uint)nodePtr);

    uint firstChildPointer = GpuVALoad1(vaNode);

    if (!(nodeType == XDXR_NODE_TYPE_BOX32 || nodeType == XDXR_NODE_TYPE_AABB_GEOMETRY) || firstChildPointer == XdxrInvalidNodePtr)
    {
        return asfloat(0x7F800000);
    }

    XdxrBoundingBox32 firstChildBounds = XdxrLoadBox32(vaNode, 0);

    return RayBoxTest(rayExtent, rayOrigin, rayDirection, rayInverseDirection, firstChildBounds);
}


inline
uint4 TestXboxBvh4Node(
    uint64_t vaOfBoxNode,
    float3 rayOrigin,
    float3 rayDirection,
    float3 rayInverseDirection,
    float rayExtent,
    bool b16bit,
    bool sortResults)
{
    uint4 pointers = GpuVALoad4(vaOfBoxNode);
    float4 results = asfloat(0x7F800000);


    XdxrBoundingBox32 childBounds[4];
    for (uint childIdx = 0; childIdx < 4; ++childIdx)
    {
        uint childPtr = pointers[childIdx];



        if (b16bit)
            childBounds[childIdx] = XdxrLoadBox16(vaOfBoxNode, childIdx);
        else
            childBounds[childIdx] = XdxrLoadBox32(vaOfBoxNode, childIdx);

        results[childIdx] = RayBoxTest(rayExtent, rayOrigin, rayDirection, rayInverseDirection, childBounds[childIdx]);
    }

    uint4 origPointers = pointers;


    pointers.x = (results.x <= rayExtent ? pointers.x : XdxrInvalidNodePtr);
    pointers.y = (results.y <= rayExtent ? pointers.y : XdxrInvalidNodePtr);
    pointers.z = (results.z <= rayExtent ? pointers.z : XdxrInvalidNodePtr);
    pointers.w = (results.w <= rayExtent ? pointers.w : XdxrInvalidNodePtr);

    if (sortResults)
    {

        if (results.x > results.z)
        {
            Swap(results.x, results.z);
            Swap(pointers.x, pointers.z);
        }

        if (results.y > results.w)
        {
            Swap(results.y, results.w);
            Swap(pointers.y, pointers.w);
        }

        if (results.x > results.y)
        {
            Swap(results.x, results.y);
            Swap(pointers.x, pointers.y);
        }

        if (results.z > results.w)
        {
            Swap(results.z, results.w);
            Swap(pointers.z, pointers.w);
        }

        if (results.y > results.z)
        {
            Swap(results.y, results.z);
            Swap(pointers.y, pointers.z);
        }
    }


    CPUTraceLog_AddBoxIntersection((float*)&childBounds, (unsigned*)&origPointers, (unsigned*)&pointers, (float*)&results, b16bit);


    return pointers;
}







inline
void BreakTieOnEdge(float Ay, float By, float Cy, bool winding_order, bool& missed)
{

    if (abs(By) == 0.0f && abs(Cy) == 0.0f)
    {
        bool bottom_edge = Ay > 0.0f;
        missed |= bottom_edge;
    }
    else
    {

        bool first_vertex_below_second = By < 0.0f || (abs(By) == 0.0f && Cy > 0.0f);
        bool right_edge = first_vertex_below_second ^ winding_order;
        missed |= right_edge;
    }
}


inline
float4 TestXboxBvh4Leaf(
    uint64_t vaLeaf,
    uint triIdx,
    float3 origin,
    float3 dir)
{

    float3 v0, v1, v2;
    Read3Verts(vaLeaf, triIdx, v0, v1, v2);


    uint remap = GpuVALoad1Offset(vaLeaf, XboxBvh4Leaf_OffsetToBaryRemap) >> (triIdx * 8);


    float3 mag = abs(dir);
    float max_mag = max(mag.x, max(mag.y, mag.z));

    if (mag.x == max_mag)
    {
        dir = SwizzleYZX(dir);
        origin = SwizzleYZX(origin);
        v0 = SwizzleYZX(v0);
        v1 = SwizzleYZX(v1);
        v2 = SwizzleYZX(v2);
    }
    else if (mag.y == max_mag)
    {
        dir = SwizzleZXY(dir);
        origin = SwizzleZXY(origin);
        v0 = SwizzleZXY(v0);
        v1 = SwizzleZXY(v1);
        v2 = SwizzleZXY(v2);
    }


    v0 -= origin;
    v1 -= origin;
    v2 -= origin;





    float3 A = float3(v0.x * dir.z - dir.x * v0.z,
                      v0.y * dir.z - dir.y * v0.z,
                      v0.z);
    float3 B = float3(v1.x * dir.z - dir.x * v1.z,
                      v1.y * dir.z - dir.y * v1.z,
                      v1.z);
    float3 C = float3(v2.x * dir.z - dir.x * v2.z,
                      v2.y * dir.z - dir.y * v2.z,
                      v2.z);

                float U = C.x * B.y - C.y * B.x;
                float V = A.x * C.y - A.y * C.x;
                float W = B.x * A.y - B.y * A.x;

    const float t_num = U * A.z + V * B.z + W * C.z;
    const float t_denom = U * dir.z + V * dir.z + W * dir.z;



    bool triangle_missed = (U < -0.0 || V < -0.0 || W < -0.0) &&
                           (U > 0.0 || V > 0.0 || W > 0.0);


    triangle_missed |= abs(t_denom) == 0.0;


    triangle_missed |= t_num * asfloat((asuint(t_denom) & 0x80000000) | asuint(float(1.f))) <= 0.0;


    triangle_missed |= !!isnan(t_num);


    const bool winding_order = t_denom > 0.0;
    if (abs(U) == 0.0)
        BreakTieOnEdge(A.y, B.y, C.y, winding_order, triangle_missed);
    if (abs(V) == 0.0)
        BreakTieOnEdge(B.y, C.y, A.y, winding_order, triangle_missed);
    if (abs(W) == 0.0)
        BreakTieOnEdge(C.y, A.y, B.y, winding_order, triangle_missed);

    if (triangle_missed)
        return float4(asfloat(0x7F800000), 1.0, 0.0, 0.0);

    float i_num = U;
    i_num = ((remap >> 0) & 3) == 1 ? V : i_num;
    i_num = ((remap >> 0) & 3) == 2 ? W : i_num;

    float j_num = U;
    j_num = ((remap >> 2) & 3) == 1 ? V : j_num;
    j_num = ((remap >> 2) & 3) == 2 ? W : j_num;


    if ((i_num < 0 && t_denom > 0) ||
        (i_num > 0 && t_denom < 0))
    {
        i_num = -i_num;
        j_num = -j_num;
    }

    return float4(t_num, t_denom, i_num, j_num);
}

inline
uint4 SoftwareBvhIntersectRay(
    uint64_t nodePtr,
    float3 rayOrigin,
    float3 rayDirection,
    float3 rayInverseDirection,
    float rayExtent,
    bool sort)
{
    const uint64_t vaNode = (nodePtr >> 3) << 6;
    const uint nodeType = XdxrTypeOfNodePointer((uint)nodePtr);

    uint4 res;

    if (nodeType <= XDXR_NODE_TYPE_TRI3)
    {
        res = asuint(TestXboxBvh4Leaf(vaNode,
                        nodeType,
                        rayOrigin,
                        rayDirection));
    }
    else
    {
        res = TestXboxBvh4Node(vaNode,
            rayOrigin,
            rayDirection,
            rayInverseDirection,
            rayExtent,
            nodeType == XDXR_NODE_TYPE_BOX16,
            sort);
    }

    return res;
}



inline
uint4 BvhIntersectRay(
    uint64_t nodePtr,
    RayData ray,
    bool sort)
{
    return SoftwareBvhIntersectRay(
        nodePtr,
        ray.Origin,
        ray.Direction,
        1.f / ray.Direction ,
        XDXR_RayTCurrent(),
        sort);
}















#define __XBOX_DISABLE_LICM





#define __XBOX_STRUCTURIZER 2
inline
uint64_t Uint2ToUint64(
    uint2 v)
{
    return ((uint64_t)v.x) | ((uint64_t)v.y << 32ull);
}
static const uint TRAVERSE_STEP_RESULT_BREAK = 0;
static const uint TRAVERSE_STEP_RESULT_INTERSECT_ANYHIT_ALLOWED = 1;
static const uint TRAVERSE_STEP_RESULT_INTERSECT_PROCEDURAL = 2;
static const uint TRAVERSE_STEP_RESULT_ITERATE = 3;
static const uint TRAVERSE_STEP_RESULT_ITERATE_BACK_TO_TLAS = 4;


float3 XDXR_WorldRayOriginV()
{
    return float3(XDXR_WorldRayOrigin(0),
        XDXR_WorldRayOrigin(1),
        XDXR_WorldRayOrigin(2));
}

float3 XDXR_WorldRayDirectionV()
{
    return float3(XDXR_WorldRayDirection(0),
        XDXR_WorldRayDirection(1),
        XDXR_WorldRayDirection(2));
}

RayData InitRay()
{
    RayData ray;
    ray.Origin = XDXR_WorldRayOriginV();
    ray.Direction = XDXR_WorldRayDirectionV();
    return ray;
}

void ResetRay(
    RayData& ray)
{
    ray.Origin = XDXR_WorldRayOriginV();
    ray.Direction = XDXR_WorldRayDirectionV();
}

void GetBvhHeaderStateTLAS(
    uint64_t vaBVH,
    uint& nodePtr)
{
    nodePtr = GpuVALoad1Offset(vaBVH, XboxBvh4Header_OffsetToRootNodePtr);
}
static uint g_xdxrCountInstance;
static uint g_xdxrCountLeaf;
static uint g_xdxrCountBox;
static uint g_xdxrCountIterations;
static uint g_xdxrMaxIterations;



uint* p_xdxrCountIterations = &g_xdxrCountIterations;





static const bool c_XDXR_ENABLE_ITERATION_LIMIT = true;
static uint c_XDXR_DEFAULT_ITERATION_LIMIT = 512;
struct NullStruct {};

struct RayState
{
    RayData m_rayData;
    uint m_packedState;


    void SetRayAndInstanceContributionToHitGroup(
        uint v)
    {

        m_packedState = __XB_BFI(0xffffff80, v << 7, m_packedState);
    }


    uint GetRayAndInstanceContributionToHitGroup()
    {
        return __XB_UBFE(25, 7, m_packedState);
    }

    static const uint FrontFacesCCW_Shift = 1;
    static const uint FaceCullDir_Shift = 2;
    static const uint InstanceOpacity_Shift = 4;




    void InitState()
    {
        const uint faceCullDir = 0;
        const uint instanceOpacity = 0;
        const uint frontFacesCCW = 0;
        m_packedState = 0;
        m_packedState |= frontFacesCCW << FrontFacesCCW_Shift;
        m_packedState |= faceCullDir << FaceCullDir_Shift;
        m_packedState |= instanceOpacity << InstanceOpacity_Shift;


        SetRayAndInstanceContributionToHitGroup(0);


        m_rayData = InitRay();
    }





    void UpdateFlagsForInstance(uint instanceOffsetAndFlags)
    {
        const uint instanceFlags = instanceOffsetAndFlags >> InstanceContributionToHitGroupIndexAndFlags_Shift_Flags;



        m_packedState = __XB_BFI(D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE, instanceFlags, m_packedState);




        const int rayFaceCullDir = max(-1, __XB_IBFE(2, 4, XDXR_GetRayFlags()));





        const int frontFace = __XB_IBFE(2, 0, instanceFlags ^ 1);
        const int cullEnable = frontFace & 1;
        const int instanceFaceCullModifier = __XB_MulI24(frontFace, cullEnable);
        const int faceCullDir = __XB_MulI24(instanceFaceCullModifier, rayFaceCullDir);


        const uint rayForceOpaqueFlags = (XDXR_GetRayFlags() << 2) & 0xC;
        const uint instanceForceOpaqueFlags = instanceFlags & 0xC;
        const uint forceOpaqueFlags = rayForceOpaqueFlags ? rayForceOpaqueFlags : instanceForceOpaqueFlags;


        const uint newFields = forceOpaqueFlags | (faceCullDir & 3);
        m_packedState = __XB_BFI(0xfU << FaceCullDir_Shift, newFields << FaceCullDir_Shift, m_packedState);

        uint rayAndInstanceContributionToHitGroup = instanceOffsetAndFlags &
            InstanceContributionToHitGroupIndexAndFlags_Mask_InstanceContributionToHitGroupIndex;

        rayAndInstanceContributionToHitGroup += XDXR_GetRayContributionToHitGroupIndex();

        SetRayAndInstanceContributionToHitGroup(rayAndInstanceContributionToHitGroup);
    }

    bool IsOpaque(uint geomFlags)
    {





        const uint forceFlags = __XB_UBFE(2, InstanceOpacity_Shift, m_packedState);
        const uint geomOpaque = geomFlags & 1;
        return (forceFlags | geomOpaque) == 1;
    }

    bool ShouldCullFace(float denom, bool flipped=false)
    {
        const int faceCullDir = __XB_IBFE(2, FaceCullDir_Shift, m_packedState);
        return __XB_MulI24(asint(denom) >> 16, faceCullDir) < 0;
    }

    uint DetermineHitKind(float denom)
    {

        uint hitKind = (denom > 0.0f) ? 254 : 255;


        uint frontFacesCCW = __XB_UBFE(1, FrontFacesCCW_Shift, m_packedState);
        return hitKind ^ frontFacesCCW;



    }

    uint ComputeHitGroupRecordOffset(
        uint geometryContributionToHitGroupIndex)
    {
        const uint hitGroupIndex = __XB_MadU24(
            geometryContributionToHitGroupIndex,
            XDXR_GetMultiplierForGeometryContributionToHitGroupIndex(),
            GetRayAndInstanceContributionToHitGroup());


        const uint hitGroupRecordOffset = __XB_MulU24(hitGroupIndex, XDXR_HitGroupShTableStride());

        return hitGroupRecordOffset;
    }
};
struct XboxRayQueryProxy
{
    uint loopCond;
    void Abort()
    {
        loopCond = TRAVERSE_STEP_RESULT_BREAK;
    }

    bool CandidateProceduralPrimitiveNonOpaque()
    {

        return !XDXR_GetPendingAttributeDword(0);
    }

    void CommitNonOpaqueTriangleHit()
    {
        XDXR_CommitHit();

        if (XDXR_GetRayFlags() & D3D12_RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH)
        {
            loopCond = TRAVERSE_STEP_RESULT_BREAK;
        }
    }

    void CommitProceduralPrimitiveHit(float tHit)
    {
        XDXR_SetPendingRayTCurrent(tHit);
        XDXR_CommitHit();

        if (XDXR_GetRayFlags() & D3D12_RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH)
        {
            loopCond = TRAVERSE_STEP_RESULT_BREAK;
        }
    }

    COMMITTED_STATUS CommittedStatus()
    {
        uint nodePtr = XDXR_GetCommittedBlasLeafNodePtr();
        if(nodePtr != 0)
        {
            XboxBvh4NodeType nodeType = XdxrTypeOfNodePointer(nodePtr);

            if (nodeType <= XDXR_NODE_TYPE_TRI3)
            {
                return COMMITTED_TRIANGLE_HIT;
            }
            else
            {
                return COMMITTED_PROCEDURAL_PRIMITIVE_HIT;
            }
        }

        return COMMITTED_NOTHING;
    }

    float RayTMin()
    {
        return XDXR_RayTMin();
    }

    uint RayFlags()
    {
        return XDXR_GetRayFlags();
    }

    float3 WorldRayOrigin()
    {
        return float3(XDXR_WorldRayOrigin(0),
            XDXR_WorldRayOrigin(1),
            XDXR_WorldRayOrigin(2));
    }

    float3 WorldRayDirection()
    {
        return float3(XDXR_WorldRayDirection(0),
            XDXR_WorldRayDirection(1),
            XDXR_WorldRayDirection(2));
    }

    float CandidateTriangleRayT()
    {
        return XDXR_GetPendingRayTCurrent();
    }

    float CommittedRayT()
    {
        return XDXR_RayTCurrent();
    }

    uint CandidateInstanceIndex()
    {
        return XDXR_Runtime_Callback_GetInstanceIndex(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.PendingInstanceNodePtr);
    }

    uint CommittedInstanceIndex()
    {
        return XDXR_Runtime_Callback_GetInstanceIndex(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.CommittedInstanceNodePtr);
    }

    uint CandidateInstanceContributionToHitGroupIndex()
    {



        const uint64_t vaInstance = XdxrNodePointerToVA(XDXR_GetTLASPtr(), g_xdxrRuntimeData.PendingInstanceNodePtr);
        const uint instanceOffsetAndFlags = GpuVALoad1Offset(vaInstance, XboxBvh4Instance_OffsetToInstanceMetaData1 + 4);

        const uint instanceContributionToHitGroup = (instanceOffsetAndFlags &
            InstanceContributionToHitGroupIndexAndFlags_Mask_InstanceContributionToHitGroupIndex) >>
                InstanceContributionToHitGroupIndexAndFlags_Shift_InstanceContributionToHitGroupIndex;

        return instanceContributionToHitGroup;
    }

    uint CommittedInstanceContributionToHitGroupIndex()
    {



        const uint64_t vaInstance = XdxrNodePointerToVA(XDXR_GetTLASPtr(), g_xdxrRuntimeData.CommittedInstanceNodePtr);
        const uint instanceOffsetAndFlags = GpuVALoad1Offset(vaInstance, XboxBvh4Instance_OffsetToInstanceMetaData1 + 4);

        const uint instanceContributionToHitGroup = (instanceOffsetAndFlags &
            InstanceContributionToHitGroupIndexAndFlags_Mask_InstanceContributionToHitGroupIndex) >>
                InstanceContributionToHitGroupIndexAndFlags_Shift_InstanceContributionToHitGroupIndex;

        return instanceContributionToHitGroup;
    }

    uint CandidateGeometryIndex()
    {
        return XDXR_Runtime_Callback_GetGeometryIndex(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.PendingInstanceNodePtr,
            g_xdxrRuntimeData.PendingBlasLeafNodePtr);
    }

    uint CommittedGeometryIndex()
    {
        return XDXR_Runtime_Callback_GetGeometryIndex(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.CommittedInstanceNodePtr,
            g_xdxrRuntimeData.CommittedBlasLeafNodePtr);
    }

    uint CandidatePrimitiveIndex()
    {
        return XDXR_Runtime_Callback_GetPrimitiveIndex(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.PendingInstanceNodePtr,
            g_xdxrRuntimeData.PendingBlasLeafNodePtr);
    }

    uint CommittedPrimitiveIndex()
    {
        return XDXR_Runtime_Callback_GetPrimitiveIndex(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.CommittedInstanceNodePtr,
            g_xdxrRuntimeData.CommittedBlasLeafNodePtr);
    }

    float3 CandidateObjectRayOrigin()
    {

        return XDXR_Runtime_Callback_GetObjectRayOrigin(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.PendingInstanceNodePtr);
    }

    float3 CommittedObjectRayOrigin()
    {
        return XDXR_Runtime_Callback_GetObjectRayOrigin(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.CommittedInstanceNodePtr);
    }

    float3 CandidateObjectRayDirection()
    {
        return XDXR_Runtime_Callback_GetObjectRayDirection(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.PendingInstanceNodePtr);
    }

    float3 CommittedObjectRayDirection()
    {
        return XDXR_Runtime_Callback_GetObjectRayDirection(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.CommittedInstanceNodePtr);
    }

    float3x4 CandidateObjectToWorld3x4()
    {
        return XDXR_Runtime_Callback_GetObjectToWorld(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.PendingInstanceNodePtr);
    }

    float3x4 CommittedObjectToWorld3x4()
    {
        return XDXR_Runtime_Callback_GetObjectToWorld(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.CommittedInstanceNodePtr);
    }

    float3x4 CandidateWorldToObject3x4()
    {
        return XDXR_Runtime_Callback_GetWorldToObject(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.PendingInstanceNodePtr);
    }

    float3x4 CommittedWorldToObject3x4()
    {
        return XDXR_Runtime_Callback_GetWorldToObject(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.CommittedInstanceNodePtr);
    }

    uint CandidateInstanceID()
    {
        return XDXR_Runtime_Callback_GetInstanceID(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.PendingInstanceNodePtr);
    }

    uint CommittedInstanceID()
    {
        return XDXR_Runtime_Callback_GetInstanceID(g_xdxrRuntimeData.Tlas256BAligned,
            g_xdxrRuntimeData.CommittedInstanceNodePtr);
    }

    float2 CandidateTriangleBarycentrics()
    {
        return float2(asfloat(g_xdxrRuntimeData.PendingAttrDwords[0]),
            asfloat(g_xdxrRuntimeData.PendingAttrDwords[1]));
    }

    float2 CommittedTriangleBarycentrics()
    {
        return float2(asfloat(g_xdxrRuntimeData.CommittedAttrDwords[0]),
            asfloat(g_xdxrRuntimeData.CommittedAttrDwords[1]));
    }

    bool CandidateTriangleFrontFace()
    {
        return g_xdxrRuntimeData.PendingAnyHitResult_HitKind == 254;
    }

    bool CommittedTriangleFrontFace()
    {
        return g_xdxrRuntimeData.CommittedReserved_HitKind == 254;
    }

    uint CurrentNumIterations()
    {


        return g_xdxrCountIterations;
    }

    void SetMaxNumIterations(uint maxNumIterations)
    {
        g_xdxrMaxIterations = maxNumIterations;
    }
};


#ifndef XDXR_USE_INLINE_AHS_FUNCTION
static const bool c_XDXR_USE_INLINE_AHS_FUNCTION = false;
#else
static const bool c_XDXR_USE_INLINE_AHS_FUNCTION = XDXR_USE_INLINE_AHS_FUNCTION;
#endif


#ifndef XDXR_USE_INLINE_IS_FUNCTION
static const bool c_XDXR_USE_INLINE_IS_FUNCTION = false;
#else
static const bool c_XDXR_USE_INLINE_IS_FUNCTION = XDXR_USE_INLINE_IS_FUNCTION;
#endif


#if XDXR_USE_INLINE_AHS_FUNCTION
void InlineAHS(XboxRayQueryProxy& rqp);
#endif


#if XDXR_USE_INLINE_IS_FUNCTION
void InlineIS(XboxRayQueryProxy& rqp);
#endif

struct InlineAHSBaseLambda
{
    bool EnableInlineAHS()
    {
        return true;
    }

    bool EnableInlineIS()
    {
        return false;
    }

    void InlineIS(XboxRayQueryProxy& rqp){}
};

struct InlineISBaseLambda
{
    bool EnableInlineAHS()
    {
        return false;
    }

    bool EnableInlineIS()
    {
        return true;
    }

    void InlineAHS(XboxRayQueryProxy& rqp){}
};

struct InlineAHSAndISBaseLambda
{
    bool EnableInlineAHS()
    {
        return true;
    }

    bool EnableInlineIS()
    {
        return true;
    }
};

struct InlineNullLambda
{
    bool EnableInlineAHS()
    {
        return c_XDXR_USE_INLINE_AHS_FUNCTION;
    }

    bool EnableInlineIS()
    {
        return c_XDXR_USE_INLINE_IS_FUNCTION;
    }

    void InlineAHS(XboxRayQueryProxy& rqp)
    {

#if XDXR_USE_INLINE_AHS_FUNCTION
        ::InlineAHS(rqp);
#endif
    }

    void InlineIS(XboxRayQueryProxy& rqp)
    {

#if XDXR_USE_INLINE_IS_FUNCTION
        ::InlineIS(rqp);
#endif
    }
};







static const uint32_t DIV_STACK_DEPTH_SHIFT = (4);
static const uint32_t DIV_STACK_DEPTH = (1U << DIV_STACK_DEPTH_SHIFT);
static const uint32_t DIV_STACK_SPACE = (DIV_STACK_DEPTH * XDXR_TRAVERSAL_WAVE_SIZE);
static const uint32_t DIV_MAX_STACK_PTR = (DIV_STACK_SPACE);



            uint g_divergentStack[DIV_STACK_SPACE];

static const uint32_t DIV_MAX_STACK_PTR_MASK = ((DIV_STACK_SPACE - 1));
static const uint32_t DIV_MAX_STACK_BYTE_PTR_MASK = ((DIV_MAX_STACK_PTR_MASK) << 2);


static const uint32_t DIV_STACK_PTR_MASK = (DIV_MAX_STACK_PTR_MASK << 2);
static const uint32_t DIV_STACK_INCREMENT = (XDXR_TRAVERSAL_WAVE_SIZE << 2);
struct TraversalStackState
{
    uint3 m_stackCache;






    uint m_stackPtr;


    uint m_lastValidNode;


    int m_parentTableOffset;




    void StackPush(uint nodePtr)
    {
        assert(__XB_UBFE(12, 7, m_stackPtr) < 4095);

        m_stackPtr += DIV_STACK_INCREMENT;
        g_divergentStack[(m_stackPtr & DIV_STACK_PTR_MASK) / 4] = nodePtr;
    }




    uint FindParentNode(uint childPtr, uint vaBVH_256BAligned)
    {
        assert(XdxrIsValidNodePointer(childPtr));
        assert(XdxrTypeOfNodePointer(childPtr) > XDXR_NODE_TYPE_TRI3);

        const uint64_t vaBVH = (uint64_t)vaBVH_256BAligned << 8;


        if (m_parentTableOffset == 0)
            m_parentTableOffset = XboxBvh4Node_GetParentTableOffset(vaBVH);

        const uint parentPtr = XboxBvh4Node_LoadParentPointer(vaBVH, childPtr, m_parentTableOffset);


        if (XdxrIsValidNodePointer(parentPtr))
        {
            const uint64_t vaParentNode = XdxrNodePointerToVA(vaBVH, parentPtr);
            const uint32_t* allChildren = (uint32_t*)(g_sharedPointerBias + vaParentNode);
            assert(childPtr == allChildren[0] ||
                   childPtr == allChildren[1] ||
                   childPtr == allChildren[2] ||
                   childPtr == allChildren[3]);
        }


        return parentPtr;
    }




    void MarkBlasEntry(
        uint ,
        uint ,
        uint blasRootPtr,
        uint )
    {
        m_lastValidNode = blasRootPtr;


        m_stackPtr |= (m_stackPtr >> 7) << 20;


        m_parentTableOffset = 0;
    }






    inline uint CleanPointer(
        uint dirtyPtr)
    {





        return __XB_IBFE(29, 0, dirtyPtr);

    }





    inline uint CleanValidPointer(
        uint dirtyPtr)
    {
        return __XB_UBFE(29, 0, dirtyPtr);
    }







    void WorkAfterTraversalIntrinsicIssued()
    {
        const uint node1 = __XB_Min3_U32(m_stackCache.x, m_stackCache.y, m_stackCache.z);

        if (node1 != XdxrInvalidNodePtr)
        {
            const uint node2 = __XB_Med3_U32(m_stackCache.x, m_stackCache.y, m_stackCache.z);

            if (node2 != XdxrInvalidNodePtr)
            {
                const uint node3 = __XB_Max3_U32(m_stackCache.x, m_stackCache.y, m_stackCache.z);

                if (node3 != XdxrInvalidNodePtr)
                    StackPush(CleanValidPointer(node3));

                StackPush(CleanValidPointer(node2));
            }
            StackPush(CleanValidPointer(node1));
        }

        m_stackCache.x = XdxrInvalidNodePtr;
        m_stackCache.y = XdxrInvalidNodePtr;
        m_stackCache.z = XdxrInvalidNodePtr;
    }




    uint HandleBoxTests(
        uint parentNode,
        uint4 rayTest)
    {

        assert(XdxrIsValidNodePointer(parentNode));
        rayTest.x |= ((rayTest.x & 4) | 0) << 29;
        rayTest.y |= ((rayTest.y & 4) | 1) << 29;
        rayTest.z |= ((rayTest.z & 4) | 2) << 29;
        rayTest.w |= ((rayTest.w & 4) | 3) << 29;


        m_stackCache.x = __XB_Min3_U32(rayTest.y, rayTest.z, rayTest.w);
        m_stackCache.y = __XB_Med3_U32(rayTest.y, rayTest.z, rayTest.w);
        m_stackCache.z = __XB_Max3_U32(rayTest.y, rayTest.z, rayTest.w);


        uint retNode = min(rayTest.x, m_stackCache.x);
        m_stackCache.x = max(rayTest.x, m_stackCache.x);




        if (m_lastValidNode != parentNode)
        {
            assert(g_divergentStack[(m_stackPtr & DIV_STACK_PTR_MASK) / 4] == XdxrInvalidNodePtr);



            rayTest.x = retNode;
            rayTest.y = __XB_Min3_U32(m_stackCache.x, m_stackCache.y, m_stackCache.z);
            rayTest.z = __XB_Med3_U32(m_stackCache.x, m_stackCache.y, m_stackCache.z);
            rayTest.w = __XB_Max3_U32(m_stackCache.x, m_stackCache.y, m_stackCache.z);



            m_stackCache.x = XdxrInvalidNodePtr;
            m_stackCache.y = XdxrInvalidNodePtr;
            m_stackCache.z = XdxrInvalidNodePtr;

            retNode = XdxrInvalidNodePtr;

            if (CleanPointer(rayTest.x) == m_lastValidNode)
            {
                m_stackCache.x = rayTest.w;
                m_stackCache.y = rayTest.z;
                retNode = rayTest.y;
            }
            else if (CleanPointer(rayTest.y) == m_lastValidNode)
            {
                m_stackCache.x = rayTest.w;
                retNode = rayTest.z;
            }
            else if (CleanPointer(rayTest.z) == m_lastValidNode)
            {
                retNode = rayTest.w;
            }



            assert(XdxrTypeOfNodePointer(retNode) > XDXR_NODE_TYPE_TRI3);

        }






        const bool returnIsValidNonTriangle = (int)retNode < -1;


        retNode = CleanPointer(retNode);



        m_lastValidNode = returnIsValidNonTriangle ? retNode : parentNode;


        return retNode;
    }





    void StackPop(
        uint& node,
        uint& loopCond,
        const uint vaBVH_256BAligned,
        const uint vaTLAS_256BAligned)
    {

        InterlockedExchange(g_divergentStack[(m_stackPtr & DIV_STACK_PTR_MASK) / 4], XdxrInvalidNodePtr, node);

        const uint numPushes = __XB_UBFE(12, 7, m_stackPtr);
        const uint numPushesInTLAS = __XB_UBFE(12, 20, m_stackPtr);

        if (XdxrIsValidNodePointer(node))
        {

            m_stackPtr -= DIV_STACK_INCREMENT;






            m_lastValidNode = __XB_BFI(__XB_IBFE(1, 2, node), node, m_lastValidNode);


            if (numPushes == numPushesInTLAS)
            {
                loopCond = TRAVERSE_STEP_RESULT_ITERATE_BACK_TO_TLAS;
                m_stackPtr = __XB_UBFE(12 + 7, 0, m_stackPtr);
            }
        }
        else if (numPushes == 0)
        {

            loopCond = TRAVERSE_STEP_RESULT_BREAK;
        }
        else
        {


            m_stackPtr = (m_stackPtr & DIV_STACK_PTR_MASK) | 0x40000;


            assert(XdxrTypeOfNodePointer(m_lastValidNode) > XDXR_NODE_TYPE_TRI3);


            node = FindParentNode(m_lastValidNode, vaBVH_256BAligned);


            if (!XdxrIsValidNodePointer(node))
            {
                m_lastValidNode = XDXR_GetPendingInstanceNodePtr();

                if (XdxrIsValidNodePointer(m_lastValidNode))
                {
                    uint64_t vaTLAS = (uint64_t)vaTLAS_256BAligned << 8ull;
                    m_parentTableOffset = XboxBvh4Node_GetParentTableOffset(vaTLAS);
                    uint64_t vaInstance = XdxrNodePointerToVA(vaTLAS, m_lastValidNode);
                    node = GpuVALoad1Offset(vaInstance, XboxBvh4Instance_OffsetToParentNode);


                    loopCond = TRAVERSE_STEP_RESULT_ITERATE_BACK_TO_TLAS;
                }
                else
                {
                    loopCond = TRAVERSE_STEP_RESULT_BREAK;
                }
            }
        }

        assert(XdxrIsValidNodePointer(node) || loopCond == TRAVERSE_STEP_RESULT_BREAK);
    }




    void StackInit(
        uint rootNodePtr,
        uint64_t vaTLAS)
    {

        m_stackPtr = (WaveGetLaneIndex()) * 4;
        g_divergentStack[m_stackPtr / 4] = XdxrInvalidNodePtr;

        m_stackCache.x = XdxrInvalidNodePtr;
        m_stackCache.y = XdxrInvalidNodePtr;
        m_stackCache.z = XdxrInvalidNodePtr;


        m_lastValidNode = rootNodePtr;
        m_parentTableOffset = 0;
    }

};
#if XDXR_USE_HLSL_2021
template<typename TLambda>
#else
typedef InlineNullLambda TLambda;
#endif
struct TraversalState_Divergent
{
    uint m_vaBVH_256BAligned;
    uint m_nodePtr;
    RayState m_rayState;
    TraversalStackState m_stack;

    void ResetRayData()
    {
        ResetRay(m_rayState.m_rayData);
    }

    RayData GetRayData()
    {
        return m_rayState.m_rayData;
    }

    void Initialize()
    {
        m_vaBVH_256BAligned = (uint)(XDXR_GetTLASPtr() >> 8);

        uint64_t vaBVH = (uint64_t)m_vaBVH_256BAligned << 8;
        GetBvhHeaderStateTLAS(vaBVH, m_nodePtr);

        m_rayState.InitState();



        XDXR_SetCommittedInstanceNodePtr(0);


        m_stack.StackInit(m_nodePtr, vaBVH);
    }




    uint Traverse_Step(
        bool disableAHS,
        bool disableIS,
        TLambda& lambda)
    {
        CPUTraceLog_StepBegins(XDXR_RayTCurrent());

        uint loopCond = TRAVERSE_STEP_RESULT_ITERATE;




        do
        {
            g_xdxrCountIterations++;;

            uint4 rayTest;







            if (!XdxrIsValidNodePointer(m_nodePtr))
            {


                m_stack.StackPop(m_nodePtr, loopCond, m_vaBVH_256BAligned, (uint)(XDXR_GetTLASPtr() >> 8));
                CPUTraceLog_StackPop(m_nodePtr, loopCond, m_vaBVH_256BAligned, (uint)(XDXR_GetTLASPtr() >> 8));
            }

            CPUTraceLog_NoteNode(m_nodePtr, m_vaBVH_256BAligned);

            if (c_XDXR_ENABLE_ITERATION_LIMIT)
            {

                if (g_xdxrCountIterations >= g_xdxrMaxIterations)
                {
                    loopCond = TRAVERSE_STEP_RESULT_BREAK;
                    CPUTraceLog_RecursionLimit(g_xdxrMaxIterations);
                }
            }


            if (loopCond == TRAVERSE_STEP_RESULT_ITERATE_BACK_TO_TLAS)
            {
                loopCond = TRAVERSE_STEP_RESULT_ITERATE;
                m_vaBVH_256BAligned = (uint)(XDXR_GetTLASPtr() >> 8);
                ResetRayData();
                XDXR_SetPendingInstanceNodePtr(0);
                CPUTraceLog_BackToTlas((uint)(XDXR_GetTLASPtr() >> 8));
            }

            if (loopCond == TRAVERSE_STEP_RESULT_ITERATE)
            {



                XboxBvh4NodeType nodeType = XdxrTypeOfNodePointer(m_nodePtr);



                const uint64_t nodePtr64 = ((((uint64_t)m_vaBVH_256BAligned << 8) >> 6) << 3) + m_nodePtr;
                const bool sortingEnabled = true;
                rayTest = BvhIntersectRay(nodePtr64, GetRayData(), sortingEnabled);
                uint4 metadata1Temp;





                const bool metadataLoad = (nodeType != XDXR_NODE_TYPE_BOX16) && (nodeType != XDXR_NODE_TYPE_BOX32);
                if (metadataLoad)
                {
                    uint loadOfs = XboxBvh4Leaf_OffsetToGeometryIndexAndFlags;
                    if (nodeType == XDXR_NODE_TYPE_INSTANCE)
                    {
                        loadOfs = XboxBvh4Instance_OffsetToInstanceMetaData1;

                        XDXR_SetPendingInstanceNodePtr(m_nodePtr);

                        ResetRayData();
                    }
                    else
                    {
                        XDXR_SetPendingBlasLeafNodePtr(m_nodePtr);
                    }

                    const uint64_t loadAddr = XdxrNodePointerToVA(((uint64_t)m_vaBVH_256BAligned << 8), m_nodePtr) + loadOfs;
                    metadata1Temp = GpuVALoad4(loadAddr);
                }







                m_stack.WorkAfterTraversalIntrinsicIssued();
                if (nodeType == XDXR_NODE_TYPE_INSTANCE || (disableIS && nodeType > XDXR_NODE_TYPE_INSTANCE))
                {
                    g_xdxrCountInstance++;;


                    bool included = (metadata1Temp.x & (XDXR_GetInstanceInclusionMask() << InstanceIDAndMask_Shift_Mask)) != 0;

                    if (disableIS)
                    {
                        included &= (nodeType != XDXR_NODE_TYPE_AABB_GEOMETRY);
                    }




                    included = XDXR_PC_AcceptInstance(included, XDXR_GetTLASPtr(), m_nodePtr);
                    if (included)
                    {
                        const uint tlasLeafPtr = m_nodePtr;

                        uint64_t vaInstance = XdxrNodePointerToVA(XDXR_GetTLASPtr(), m_nodePtr);



                        const uint instanceOffsetAndFlags = metadata1Temp.y;


                        m_nodePtr = XdxrUnpackRootNodePtr(metadata1Temp.z);


                        m_vaBVH_256BAligned = metadata1Temp.w;


                        m_stack.MarkBlasEntry(tlasLeafPtr, (uint)(XDXR_GetTLASPtr() >> 8), m_nodePtr, m_vaBVH_256BAligned);
                        CPUTraceLog_MarkBlasEntry(tlasLeafPtr, (uint)(XDXR_GetTLASPtr() >> 8), m_nodePtr, m_vaBVH_256BAligned);

                        const uint64_t instanceTransformVA = vaInstance + XboxBvh4Instance_OffsetToWorldToObject;
                        const bool instanceRotationNeeded = !(metadata1Temp.z & XDXR_INSTANCE_INTERNAL_FLAGS_TRANSFORM_HAS_IDENTITY_ROTATION);

                        uint3 pos = GpuVALoad3(instanceTransformVA);

                        if (instanceRotationNeeded)
                        {


                            uint4 instanceTransform[3];
                            instanceTransform[0].x = pos.x;
                            instanceTransform[0].y = pos.y;
                            instanceTransform[0].z = pos.z;

                            instanceTransform[0].w = GpuVALoad1Offset(instanceTransformVA, 12);
                            instanceTransform[1].x = GpuVALoad1Offset(instanceTransformVA, 16);
                            instanceTransform[1].y = GpuVALoad1Offset(instanceTransformVA, 20);
                            instanceTransform[1].z = GpuVALoad1Offset(instanceTransformVA, 24);
                            instanceTransform[1].w = GpuVALoad1Offset(instanceTransformVA, 28);
                            instanceTransform[2].x = GpuVALoad1Offset(instanceTransformVA, 32);
                            instanceTransform[2].y = GpuVALoad1Offset(instanceTransformVA, 36);
                            instanceTransform[2].z = GpuVALoad1Offset(instanceTransformVA, 40);
                            instanceTransform[2].w = GpuVALoad1Offset(instanceTransformVA, 44);

                            XdxrMatrix worldToObject;
                            XdxrMatrix_UnpackInstanceWorldToObject(worldToObject, instanceTransform[0], instanceTransform[1], instanceTransform[2]);

                            m_rayState.m_rayData.Origin = worldToObject.Mul3x3(XDXR_WorldRayOriginV());
                            m_rayState.m_rayData.Direction = worldToObject.Mul3x3(XDXR_WorldRayDirectionV());
                        }

                        m_rayState.m_rayData.Origin.x += asfloat(pos.x);
                        m_rayState.m_rayData.Origin.y += asfloat(pos.y);
                        m_rayState.m_rayData.Origin.z += asfloat(pos.z);

                        m_rayState.UpdateFlagsForInstance(instanceOffsetAndFlags);




                    }
                    else
                    {

                        m_nodePtr = XdxrInvalidNodePtr;


                        nodeType = (XboxBvh4NodeType)(XDXR_NODE_TYPE_MASK + 1);
                    }
                }
                if ((!disableIS && nodeType == XDXR_NODE_TYPE_AABB_GEOMETRY) ||
                    nodeType <= XDXR_NODE_TYPE_TRI3)
                {
                    g_xdxrCountLeaf++;;

                    const uint geometryIndexAndFlags = metadata1Temp.x;
                    const uint geometryContributionToHitGroupIndex = geometryIndexAndFlags >> GeometryIndexAndFlags_Shift_GeometryIndex;
                    const bool isOpaque = m_rayState.IsOpaque(geometryIndexAndFlags);


                    const uint cullFlag = isOpaque ? D3D12_RAY_FLAG_CULL_OPAQUE : D3D12_RAY_FLAG_CULL_NON_OPAQUE;


                    const bool isQuad = (geometryIndexAndFlags & D3D12XBOX_RAYTRACING_GEOMETRY_FLAG_INTERNAL_FLAG_QUAD_PRIM_BIT);
                    const bool isTri0 = (nodeType == XDXR_NODE_TYPE_TRI0);
                    const bool culled = (XDXR_GetRayFlags() & cullFlag);
                    const bool flipped = !isTri0 && bool(geometryIndexAndFlags & D3D12XBOX_RAYTRACING_GEOMETRY_FLAG_INTERNAL_FLAG_FLIP_TRI1_BIT);


                    m_nodePtr |= (isQuad & isTri0 & !culled) ? XDXR_NODE_TYPE_TRI1 : XdxrInvalidNodePtr;




                    if (!culled)
                    {
                        const uint hitGroupRecordOffset = m_rayState.ComputeHitGroupRecordOffset(
                            geometryContributionToHitGroupIndex);

                        XDXR_SetPendingShaderRecordOffset(hitGroupRecordOffset);


                        if (!disableIS && nodeType == XDXR_NODE_TYPE_AABB_GEOMETRY)
                        {
                            XDXR_SetPendingAttributeDword(0, isOpaque? 1 : 0);

                            if (lambda.EnableInlineIS())
                            {
                                XboxRayQueryProxy rqp;
                                rqp.loopCond = TRAVERSE_STEP_RESULT_ITERATE;
                                lambda.InlineIS(rqp);
                                loopCond = rqp.loopCond;
                            }
                            else

                            {
                                loopCond = TRAVERSE_STEP_RESULT_INTERSECT_PROCEDURAL;
                            }

                        }
                        else
                        {
                            const float t_num = asfloat(rayTest.x);
                            const float t_den = asfloat(rayTest.y);

                            const float rcpDen = rcp(t_den);
                            const float hitT = t_num * rcpDen;




                            if (!m_rayState.ShouldCullFace(t_den, flipped) &&
                                !(hitT < XDXR_RayTMin() || hitT >= XDXR_RayTCurrent()))
                            {
                                const float i_num = asfloat(rayTest.z);
                                const float j_num = asfloat(rayTest.w);

                                BuiltInTriangleIntersectionAttributes attr;
                                attr.barycentrics = float2(i_num, j_num) * rcpDen;

                                const uint hitKind = m_rayState.DetermineHitKind(rcpDen);

                                XDXR_SetPendingAttr(attr);
                                XDXR_SetPendingRayTCurrent(hitT);
                                XDXR_SetPendingHitKind(hitKind);


                                if (disableAHS | isOpaque)
                                {
                                    XDXR_CommitHit();

                                    CPUTraceLog_InnerLoopHit(XDXR_RayTCurrent());

                                    if (XDXR_GetRayFlags() & D3D12_RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH)
                                        loopCond = TRAVERSE_STEP_RESULT_BREAK;


                                }
                                else
                                {

                                    if (lambda.EnableInlineAHS())
                                    {
                                        XboxRayQueryProxy rqp;
                                        rqp.loopCond = TRAVERSE_STEP_RESULT_ITERATE;
                                        lambda.InlineAHS(rqp);
                                        loopCond = rqp.loopCond;
                                    }
                                    else

                                    {
                                        loopCond = TRAVERSE_STEP_RESULT_INTERSECT_ANYHIT_ALLOWED;
                                    }
                                }
                            }
                        }
                    }
                }


                if (nodeType == XDXR_NODE_TYPE_BOX16 ||
                    nodeType == XDXR_NODE_TYPE_BOX32)
                {
                    g_xdxrCountBox++;;

                    CPUTraceLog_HandleBoxTests(m_vaBVH_256BAligned, m_nodePtr, rayTest[0], rayTest[1], rayTest[2], rayTest[3]);
                    m_nodePtr = m_stack.HandleBoxTests(m_nodePtr, rayTest);
                }
            }
        }
        while (loopCond == TRAVERSE_STEP_RESULT_ITERATE);

        return loopCond;
    }
};



enum XDXR_CANDIDATE_TYPE : uint
{
    XDXR_CANDIDATE_NON_OPAQUE_TRIANGLE = CANDIDATE_NON_OPAQUE_TRIANGLE,
    XDXR_CANDIDATE_PROCEDURAL_PRIMITIVE = CANDIDATE_PROCEDURAL_PRIMITIVE,
    XDXR_CANDIDATE_INVALID
};






#if XDXR_USE_HLSL_2021
template<uint TFlags = 0, typename TLambda = InlineNullLambda>
#else
typedef InlineNullLambda TLambda;
#endif
struct XboxRayQuery
{




#if XDXR_USE_HLSL_2021
TraversalState_Divergent<TLambda> state;
#else
static const uint TFlags = 0;
TraversalState_Divergent state;
#endif


    bool disableAHS;
    bool disableIS;

    XboxRayQueryProxy rayQueryProxy;

    void TraceRayInline(

        uint64_t tlas,



        uint RayFlags,
        uint InstanceInclusionMask,
        RayDesc Ray)
    {
        XDXR_SetupHlslRuntime(tlas,
            InstanceInclusionMask,
            RayFlags | TFlags,
            float3(Ray.Origin.x, Ray.Origin.y, Ray.Origin.z),
            float3(Ray.Direction.x, Ray.Direction.y, Ray.Direction.z),
            Ray.TMin,
            Ray.TMax);







        state.Initialize();

        rayQueryProxy.loopCond = TRAVERSE_STEP_RESULT_ITERATE;

        TLambda statelessLambda;
        disableAHS = (bool)(TFlags & D3D12_RAY_FLAG_FORCE_OPAQUE) || statelessLambda.EnableInlineIS();
        disableIS = (bool)(TFlags & D3D12_RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES) || statelessLambda.EnableInlineAHS();

        g_xdxrCountInstance = 0; g_xdxrCountLeaf = 0; g_xdxrCountBox = 0; g_xdxrCountIterations = 0; g_xdxrMaxIterations = c_XDXR_DEFAULT_ITERATION_LIMIT;;
    }


    void TraceRayInlineNoAHS(

        uint64_t tlas,



        uint RayFlags,
        uint InstanceInclusionMask,
        RayDesc Ray )
    {
        TraceRayInline(tlas, RayFlags, InstanceInclusionMask, Ray);
        disableAHS = true;
    }

    void TraceRayInlineNoIS(

        uint64_t tlas,



        uint RayFlags,
        uint InstanceInclusionMask,
        RayDesc Ray )
    {
        TraceRayInline(tlas, RayFlags, InstanceInclusionMask, Ray);
        disableIS = true;
    }

    void TraceRayInlineNoAHSNoIS(

        uint64_t tlas,



        uint RayFlags,
        uint InstanceInclusionMask,
        RayDesc Ray )
    {
        TraceRayInline(tlas, RayFlags, InstanceInclusionMask, Ray);
        disableAHS = true;
        disableIS = true;
    }

    bool Proceed(TLambda& lambda)
    {
        if (rayQueryProxy.loopCond == TRAVERSE_STEP_RESULT_BREAK)
        {
                        ;
            return false;
        }


        rayQueryProxy.loopCond = state.Traverse_Step(disableAHS, disableIS, lambda);


        if (rayQueryProxy.loopCond == TRAVERSE_STEP_RESULT_BREAK)
        {
                        ;
            return false;
        }
        else
        {
            return true;
        }

    }

    bool Proceed()
    {
        TLambda statelessLambda;
        return Proceed(statelessLambda);
    }

    void Abort()
    {
        rayQueryProxy.Abort();
    }

    uint CandidateType()
    {
        if (rayQueryProxy.loopCond == TRAVERSE_STEP_RESULT_INTERSECT_ANYHIT_ALLOWED)
            return (uint)XDXR_CANDIDATE_NON_OPAQUE_TRIANGLE;

        if (rayQueryProxy.loopCond == TRAVERSE_STEP_RESULT_INTERSECT_PROCEDURAL)
            return (uint)XDXR_CANDIDATE_PROCEDURAL_PRIMITIVE;

        return (uint)XDXR_CANDIDATE_INVALID;
    }

    bool CandidateProceduralPrimitiveNonOpaque()
    {
        return rayQueryProxy.CandidateProceduralPrimitiveNonOpaque();
    }

    void CommitNonOpaqueTriangleHit()
    {
        rayQueryProxy.CommitNonOpaqueTriangleHit();
    }

    void CommitProceduralPrimitiveHit(float tHit)
    {
        rayQueryProxy.CommitProceduralPrimitiveHit(tHit);
    }

    COMMITTED_STATUS CommittedStatus()
    {
        return rayQueryProxy.CommittedStatus();
    }

    float RayTMin()
    {
        return rayQueryProxy.RayTMin();
    }

    uint RayFlags()
    {
        return rayQueryProxy.RayFlags();
    }

    float3 WorldRayOrigin()
    {
        return rayQueryProxy.WorldRayOrigin();
    }

    float3 WorldRayDirection()
    {
        return rayQueryProxy.WorldRayDirection();
    }

    float CandidateTriangleRayT()
    {
        return rayQueryProxy.CandidateTriangleRayT();
    }

    float CommittedRayT()
    {
        return rayQueryProxy.CommittedRayT();
    }

    uint CandidateInstanceIndex()
    {
        return rayQueryProxy.CandidateInstanceIndex();
    }

    uint CommittedInstanceIndex()
    {
        return rayQueryProxy.CommittedInstanceIndex();
    }

    uint CandidateInstanceContributionToHitGroupIndex()
    {
        return rayQueryProxy.CandidateInstanceContributionToHitGroupIndex();
    }

    uint CommittedInstanceContributionToHitGroupIndex()
    {
        return rayQueryProxy.CommittedInstanceContributionToHitGroupIndex();
    }

    uint CandidateGeometryIndex()
    {
        return rayQueryProxy.CandidateGeometryIndex();
    }

    uint CommittedGeometryIndex()
    {
        return rayQueryProxy.CommittedGeometryIndex();
    }

    uint CandidatePrimitiveIndex()
    {
        return rayQueryProxy.CandidatePrimitiveIndex();
    }

    uint CommittedPrimitiveIndex()
    {
        return rayQueryProxy.CommittedPrimitiveIndex();
    }

    float3 CandidateObjectRayOrigin()
    {
        return rayQueryProxy.CandidateObjectRayOrigin();
    }

    float3 CommittedObjectRayOrigin()
    {
        return rayQueryProxy.CommittedObjectRayOrigin();
    }

    float3 CandidateObjectRayDirection()
    {
        return rayQueryProxy.CandidateObjectRayDirection();
    }

    float3 CommittedObjectRayDirection()
    {
        return rayQueryProxy.CommittedObjectRayDirection();
    }

    float3x4 CandidateObjectToWorld3x4()
    {
        return rayQueryProxy.CandidateObjectToWorld3x4();
    }

    float3x4 CommittedObjectToWorld3x4()
    {
        return rayQueryProxy.CommittedObjectToWorld3x4();
    }

    float3x4 CandidateWorldToObject3x4()
    {
        return rayQueryProxy.CandidateWorldToObject3x4();
    }

    float3x4 CommittedWorldToObject3x4()
    {
        return rayQueryProxy.CommittedWorldToObject3x4();
    }

    uint CandidateInstanceID()
    {
        return rayQueryProxy.CandidateInstanceID();
    }

    uint CommittedInstanceID()
    {
        return rayQueryProxy.CommittedInstanceID();
    }

    float2 CandidateTriangleBarycentrics()
    {
        return rayQueryProxy.CandidateTriangleBarycentrics();
    }

    float2 CommittedTriangleBarycentrics()
    {
        return rayQueryProxy.CommittedTriangleBarycentrics();
    }

    bool CandidateTriangleFrontFace()
    {
        return rayQueryProxy.CandidateTriangleFrontFace();
    }

    bool CommittedTriangleFrontFace()
    {
        return rayQueryProxy.CommittedTriangleFrontFace();
    }

    uint CurrentNumIterations()
    {
        return rayQueryProxy.CurrentNumIterations();
    }

    void SetMaxNumIterations(uint maxNumIterations)
    {
        rayQueryProxy.SetMaxNumIterations(maxNumIterations);
    }
};
// ver:xb_gdk_2406zn:20160101-0000