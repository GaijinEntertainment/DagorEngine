/*
Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/compatibility.hpp>

using double2 = glm::vec<2, double, glm::highp>;
using double3 = glm::vec<3, double, glm::highp>;
using double4 = glm::vec<4, double, glm::highp>;
using float4 = glm::vec4;
using float3 = glm::vec3;
using float2 = glm::vec2;
using float1 = glm::vec1;
using bool2 = glm::bvec1;
using int2 = glm::ivec2;
using int4 = glm::ivec4;
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