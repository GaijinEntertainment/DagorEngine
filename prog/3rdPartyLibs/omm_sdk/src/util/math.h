/*
Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#define GLM_FORCE_INLINE 
//#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_INTRINSICS
//#define GLM_FORCE_AVX2
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/compatibility.hpp>

#define OMM_GLM_DEFINE_DEFAULT_P glm::aligned_highp 

using double2 = glm::vec<2, double, glm::highp>;
using double3 = glm::vec<3, double, glm::highp>;
using double4 = glm::vec<4, double, glm::highp>;
#if 1
using float4 = glm::vec<4, float, OMM_GLM_DEFINE_DEFAULT_P>;
using float3 = glm::vec<3, float, OMM_GLM_DEFINE_DEFAULT_P>;
using float2 = glm::vec<2, float, OMM_GLM_DEFINE_DEFAULT_P>;
using float1 = glm::vec<1, float, OMM_GLM_DEFINE_DEFAULT_P>;
using int2 = glm::vec<2, int, OMM_GLM_DEFINE_DEFAULT_P>;
using int4 = glm::vec<4, int, OMM_GLM_DEFINE_DEFAULT_P>;
#else
using float4 = glm::vec<4, float>;
using float3 = glm::vec<3, float>;
using float2 = glm::vec<2, float>;
using float1 = glm::vec<1, float>;
using int2 = glm::ivec2;
using int4 = glm::ivec4;
#endif
using bool2 = glm::bvec1;

using uint2 = glm::uvec2;
using uint3 = glm::uvec3;
using uchar1 = glm::u8vec1;
using uchar2 = glm::u8vec2;
using uchar3 = glm::u8vec3;
using uchar4 = glm::u8vec4;
using uint = uint32_t;

namespace math
{
    // Quad layout:
    // [x, y]
    // [z, w]
    inline float Bilinear(float4 quad, const float2& p) {
        const float ac = glm::lerp<float>(quad.x, quad.z, p.x);
        const float bd = glm::lerp<float>(quad.y, quad.w, p.x);
        return glm::lerp(ac, bd, p.y);
    }

    template<class T>
    inline T DivUp(T x, T y)
    {
        return (x + y - 1) / y;
    }

    template<class T>
    constexpr inline T Align(T size, T alignment)
    {
        return (size + alignment - 1) & ~(alignment - 1);
    }
}