/*
Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "math.h"
#include "omm.h"
#include "assert.h"

namespace omm
{
    struct Line
    {
        Line() = default;
        Line& operator=(const Line&) = default;
        Line(const float2& _p0, const float2& _p1) :
            p0(_p0),
            p1(_p1)
        {
            aabb_s = { std::min(p0.x, p1.x), std::min(p0.y, p1.y) };
            aabb_e = { std::max(p0.x, p1.x), std::max(p0.y, p1.y) };
        }

        float2 p0;
        float2 p1;
        float2 aabb_s;     //< Start point of the aabb
        float2 aabb_e;     //< End point of the aabb
    };

    static bool IsInvalid(const float2& p0, const float2& p1, const float2& p2)
    {
        const bool anyNan = glm::any(glm::isnan(p0)) || glm::any(glm::isnan(p1)) || glm::any(glm::isnan(p2));
        const bool anyInf = glm::any(glm::isinf(p0)) || glm::any(glm::isinf(p1)) || glm::any(glm::isinf(p2));
        return anyNan || anyInf;
    }

    static inline bool IsDegenerate(const float2& p0, const float2& p1, const float2& p2) {
        float area = 0.5f * std::abs(p0.x * (p1.y - p2.y) + p1.x * (p2.y - p0.y) + p2.x * (p0.y - p1.y));
        return area < 1e-9;
    }

    static inline bool IsCCW(const float2& p0, const float2& p1, const float2& p2) {
        double3 a = double3(p2 - p0, 0);
        double3 b = double3(p1 - p0, 0);
        const double3 N = glm::cross(a, b);
        const double Nz = N.z;
        return Nz < 0;
    }

    struct Triangle {

        Triangle() = default;
        Triangle& operator=(const Triangle&) = default;
        Triangle(const float2& _p0, const float2& _p1, const float2& _p2) :
            p0(_p0),
            p1(_p1),
            p2(_p2)
        {
            aabb_s = { std::min(std::min(p0.x, p1.x), p2.x), std::min(std::min(p0.y, p1.y), p2.y) };
            aabb_e = { std::max(std::max(p0.x, p1.x), p2.x), std::max(std::max(p0.y, p1.y), p2.y) };
        }

        inline float2 getP(uint32_t index) const {
            if (index == 0)
                return p0;
            else if (index == 1)
                return p1;
            OMM_ASSERT(index == 2);
            return p2;
        }

        inline bool GetIsInvalid() const {
            return IsInvalid(p0, p1, p2);
        }

        inline bool GetIsDegenerate() const {
            return IsDegenerate(p0, p1, p2);
        }

        inline bool GetIsCCW() const {
            OMM_ASSERT(!GetIsInvalid());
            OMM_ASSERT(!GetIsDegenerate());
            return IsCCW(p0, p1, p2);
        }

        float2 p0;
        float2 p1;
        float2 p2;

        float2 aabb_s;         //< Start point of the aabb
        float2 aabb_e;         //< End point of the aabb
    };

    template <class T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    static inline uint64_t GetHash(const Triangle& t) {
        std::size_t seed = 42;
        hash_combine(seed, t.p0);
        hash_combine(seed, t.p1);
        hash_combine(seed, t.p2);
        return seed;
    }

    static float float16ToFloat32(uint16_t fp16)
    {
        union { uint32_t ui32; float fp32; } output;

        if (fp16 == 0x8000 || fp16 == 0x0000)
        {
            output.ui32 = 0;
        }
        else
        {
            int e = (uint32_t(fp16) & 0x7c00) >> 10;
            int m = (uint32_t(fp16) & 0x03ff) << 13;
            e = e - 15 + 127;
            output.ui32 = ((e << 23) | m) | ((uint32_t(fp16) & 0x8000) << 16);
        }
        return output.fp32;
    };

    template<typename StorageType>
    static StorageType getUvComponantStorage(const void* texCoords, uint32_t texCoordStrideInBytes, uint32_t index, uint32_t component)
    {
        const StorageType* offset = (const StorageType * )((const uint8_t*)texCoords + texCoordStrideInBytes * index);
        return *(offset + component);
    }

    static float2 FetchUV(const void* texCoords, uint32_t texCoordStrideInBytes, ommTexCoordFormat texCoordFormat, const uint32_t index)
    {
        const uint32_t* texCoords32 = (const uint32_t*)texCoords;
        switch (texCoordFormat)
        {
        case ommTexCoordFormat_UV16_UNORM:
            return glm::unpackUnorm2x16(getUvComponantStorage<uint32_t>(texCoords, texCoordStrideInBytes, index, 0));
        case ommTexCoordFormat_UV16_FLOAT:
            return glm::unpackHalf2x16(getUvComponantStorage<uint32_t>(texCoords, texCoordStrideInBytes, index, 0));
        case ommTexCoordFormat_UV32_FLOAT:
            return {
                getUvComponantStorage<float>(texCoords, texCoordStrideInBytes, index, 0),
                getUvComponantStorage<float>(texCoords, texCoordStrideInBytes, index, 1)
            };
        default:
            return { 0,0 };
        }
    }

    static Triangle FetchUVTriangle(const void* texCoords, uint32_t texCoordStrideInBytes, ommTexCoordFormat texCoordFormat, const uint32_t index[3])
    {
        Triangle t = Triangle(
            FetchUV(texCoords, texCoordStrideInBytes, texCoordFormat, index[0]),
            FetchUV(texCoords, texCoordStrideInBytes, texCoordFormat, index[1]),
            FetchUV(texCoords, texCoordStrideInBytes, texCoordFormat, index[2]));
        return t;
    }

    static void GetUInt32Indices(ommIndexFormat indexFormat, const void* indices, size_t triIndexIndex, uint32_t outIndices[3])
    {
        if (indexFormat == ommIndexFormat_UINT_16)
        {
            outIndices[0] = ((const uint16_t*)indices)[triIndexIndex + 0];
            outIndices[1] = ((const uint16_t*)indices)[triIndexIndex + 1];
            outIndices[2] = ((const uint16_t*)indices)[triIndexIndex + 2];
        }
        else
        {
            outIndices[0] = ((const uint32_t*)indices)[triIndexIndex + 0];
            outIndices[1] = ((const uint32_t*)indices)[triIndexIndex + 1];
            outIndices[2] = ((const uint32_t*)indices)[triIndexIndex + 2];
        }
    }

    static float2 InterpolateTriangleUV(const float3& bc, const Triangle& triangleUVs)
    {
        return triangleUVs.p0 * bc.x + triangleUVs.p1 * bc.y + triangleUVs.p2 * bc.z;
    }

    static inline float3 InitBarycentrics(const float2& uv) {
        return {  1.f - uv.x - uv.y, uv.x, uv.y };
    }
   
} // namespace omm