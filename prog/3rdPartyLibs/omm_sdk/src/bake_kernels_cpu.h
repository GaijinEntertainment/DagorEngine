/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "util/math.h"
#include "util/cpu_raster.h"
#include "util/texture.h"
#include "util/util.h"

namespace omm
{

struct OmmCoverage 
{
    uint32_t numAboveAlpha = 0;
    uint32_t numBelowAlpha = 0;
};

static ommOpacityState GetStateFromCoverage(ommFormat vmFormat, ommUnknownStatePromotion mode, ommOpacityState alphaCutoffGT, ommOpacityState alphaCutoffLE, const OmmCoverage& coverage)
{
    const bool isUnknown = coverage.numAboveAlpha != 0 && coverage.numBelowAlpha != 0;
    if (isUnknown)
    {
        if (vmFormat == ommFormat_OC1_4_State)
        {
            if (mode == ommUnknownStatePromotion_ForceOpaque)
                return ommOpacityState_UnknownOpaque;
            else if (mode == ommUnknownStatePromotion_ForceTransparent)
                return ommOpacityState_UnknownTransparent;
            OMM_ASSERT(mode == ommUnknownStatePromotion_Nearest);

            return coverage.numAboveAlpha >= coverage.numBelowAlpha ? GetUnknownVersionOf(alphaCutoffGT) : GetUnknownVersionOf(alphaCutoffLE);
        }
        else // if (vmFormat == ommFormat_OC1_2_State)
        {
            OMM_ASSERT(vmFormat == ommFormat_OC1_2_State);

            if (mode == ommUnknownStatePromotion_ForceOpaque)
                return ommOpacityState_Opaque;
            else if (mode == ommUnknownStatePromotion_ForceTransparent)
                return ommOpacityState_Transparent;
            OMM_ASSERT(mode == ommUnknownStatePromotion_Nearest);
            return coverage.numAboveAlpha >= coverage.numBelowAlpha ? alphaCutoffGT : alphaCutoffLE;
        }
    }
    else if (coverage.numAboveAlpha == 0)
    {
        return alphaCutoffLE;
    }
    else // if (coverage.numBelowAlpha == 0) 
    {
        OMM_ASSERT(coverage.numBelowAlpha == 0);
        return alphaCutoffGT;
    }
};

// ~~~~~~ LevelLineIntersectionKernel ~~~~~~ 
// 
struct LevelLineIntersectionKernel
{
    struct Params {
        OmmCoverage*            vmCoverage;
        const Triangle*         triangle;
        float2                  invSize;
        int2                    size;
        const TextureImpl*      texture;
        float                   alphaCutoff;
        float                   borderAlpha;
        uint32_t                mipLevel;
    };

private:
#if 0
    // Borrowed from https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
    struct Triangle
    {
        // private
        float _Sign(const float2& p1, const float2& p2, const float2& p3)
        {
            return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
        }
        // public
        void Init(const float2& v0, const float2& v1, const float2& v2)
        {
            _v0 = v0; _v1 = v1; _v2 = v2;
        }

        bool PointInTriangle(const float2& pt)
        {
            float d1, d2, d3;
            bool has_neg, has_pos;

            d1 = _Sign(pt, _v0, _v1);
            d2 = _Sign(pt, _v1, _v2);
            d3 = _Sign(pt, _v2, _v0);

            has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

            return !(has_neg && has_pos);
        }

        float2 _v0;
        float2 _v1;
        float2 _v2;
    };
#endif

    inline static bool IsZero(float value, float kEpsilon = 1e-6f) {
        return value < kEpsilon && value > -kEpsilon;
    };

    // Gaijin: the edge as p0 + t * e and the alpha as a quadratic in t, in place of the slope form, its roots and
    // the point-on-edge test: no square root, no special cases, and no absolute epsilon that rejects real
    // crossings in float32. Same math as TestEdgeHyperbolaIntersection in omm_resample_common.hlsli.
    // One axis of the cell [0,1]: narrows [t0, t1] to the part of p + t * e inside it.
    inline static void ClipToCellAxis(float p, float e, float& t0, float& t1)
    {
        if (e != 0.f)
        {
            const float ta = -p / e;
            const float tb = (1.f - p) / e;
            t0 = std::max(t0, std::min(ta, tb));
            t1 = std::min(t1, std::max(ta, tb));
        }
        else if (p < 0.f || p > 1.f)
        {
            t0 = 2.f;
            t1 = -1.f;
        }
    }

    // True when the level line h.x + h.y*x + h.z*y + h.w*x*y = 0 crosses the edge p0-p1 inside the cell [0,1]^2.
    inline static bool TestEdgeHyperbolaIntersection(const float2& p0, const float2& p1, const float4& h)
    {
        const float2 e = p1 - p0;
        float t0 = 0.f;
        float t1 = 1.f;
        ClipToCellAxis(p0.x, e.x, t0, t1);
        ClipToCellAxis(p0.y, e.y, t0, t1);
        if (t0 > t1)
            return false;

        const float A = h.w * e.x * e.y;
        const float B = h.y * e.x + h.z * e.y + h.w * (p0.x * e.y + p0.y * e.x);
        const float C = h.x + h.y * p0.x + h.z * p0.y + h.w * p0.x * p0.y;
        const float g0 = (A * t0 + B) * t0 + C;
        const float g1 = (A * t1 + B) * t1 + C;
        if (g0 * g1 <= 0.f)
            return true;

        // Same sign at both ends: only the vertex of the parabola can cross.
        if (A == 0.f)
            return false;
        const float tv = -B / (2.f * A);
        const float gv = (A * tv + B) * tv + C;
        return tv > t0 && tv < t1 && g0 * gv <= 0.f;
    }
public:

    template<ommCpuTextureFormat eFormat, ommTextureAddressMode eTextureAddressMode, TilingMode eTilingMode, bool bIsDegenerate, bool bTexIsPow2>
    static void run(int2 pixel, void* ctx)
    {
        Params* p = (Params*)ctx;

        const float2& invSize = p->texture->GetRcpSize(p->mipLevel);

        // We add +0.5 here in order to compensate for the raster offset.
        const float2 pixelf = (float2)pixel + 0.5f;
        const float2 invPixelf = pixelf * invSize;

        int2 coord00, coord10, coord01, coord11;
        omm::GatherTexCoord4<eTextureAddressMode, bTexIsPow2>(pixel, p->texture->GetSize(p->mipLevel), p->texture->GetSizeLog2(p->mipLevel), coord00, coord10, coord01, coord11);

        auto IsBorder = [](int2 coord) {
            return (coord.x == kTexCoordBorder || coord.y == kTexCoordBorder);
        };

        float4 gatherRed;
        if constexpr (eTextureAddressMode == ommTextureAddressMode_Border)
        {
            gatherRed.x = IsBorder(coord00) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord00, p->mipLevel);
            gatherRed.y = IsBorder(coord01) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord01, p->mipLevel);
            gatherRed.z = IsBorder(coord11) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord11, p->mipLevel);
            gatherRed.w = IsBorder(coord10) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord10, p->mipLevel);
        }
        else
        {
            gatherRed.x = p->texture->Load<eFormat, eTilingMode>(coord00, p->mipLevel);
            gatherRed.y = p->texture->Load<eFormat, eTilingMode>(coord01, p->mipLevel);
            gatherRed.z = p->texture->Load<eFormat, eTilingMode>(coord11, p->mipLevel);
            gatherRed.w = p->texture->Load<eFormat, eTilingMode>(coord10, p->mipLevel);
        }

        // ~~~ Look for internal extremes ~~~ 
		if (!bIsDegenerate)
        {
            const bool IsOpaque0 = p->alphaCutoff < gatherRed.x;
            const bool IsOpaque1 = p->alphaCutoff < gatherRed.y;
            const bool IsOpaque2 = p->alphaCutoff < gatherRed.z;
            const bool IsOpaque3 = p->alphaCutoff < gatherRed.w;


            const float2 p0x0 = invPixelf;
            const float2 p0x1 = invPixelf + float2(0.0f, invSize.y);
            const float2 p1x1 = invPixelf + invSize;
            const float2 p1x0 = invPixelf + float2(invSize.x, 0.0f);

            bool IsInside0 = true;
            bool IsInside1 = true;
            bool IsInside2 = true;
            bool IsInside3 = true;

            //if (coverage == Coverage::PartiallyCovered)
            {
                IsInside0 = p->triangle->PointInTriangle(p0x0);
                IsInside1 = p->triangle->PointInTriangle(p0x1);
                IsInside2 = p->triangle->PointInTriangle(p1x1);
                IsInside3 = p->triangle->PointInTriangle(p1x0);
            }

            bool IsOpaque = false;
            bool IsTransparent = false;

            IsOpaque |= IsInside0 && IsOpaque0;
            IsTransparent |= IsInside0 && !IsOpaque0;

            IsOpaque |= IsInside1 && IsOpaque1;
            IsTransparent |= IsInside1 && !IsOpaque1;

            IsOpaque |= IsInside2 && IsOpaque2;
            IsTransparent |= IsInside2 && !IsOpaque2;

            IsOpaque |= IsInside3 && IsOpaque3;
            IsTransparent |= IsInside3 && !IsOpaque3;

            if (IsOpaque) {
                p->vmCoverage->numAboveAlpha += 1;
            }

            if (IsTransparent)
            {
                p->vmCoverage->numBelowAlpha += 1;
            }

            // We've already concluded it's unknown -> return!
            if (IsOpaque && IsTransparent)
            {
                return;
            }
        }

        {
            // Intersections with level lines is loosley based on
            // "Extraction of the Level Lines of a Bilinear Image"
            // https://www.ipol.im/pub/art/2019/269/article.pdf

            // Compute hyperbolic paraboloid params, surface is given by:
            // f(x, y) = a + b * x + c * y + d * x * y
            const float a = gatherRed.x;
            const float b = gatherRed.w - gatherRed.x;
            const float c = gatherRed.y - gatherRed.x;
            const float d = gatherRed.x + gatherRed.z - gatherRed.y - gatherRed.w;

            if (IsZero(b) && IsZero(c) && IsZero(d))
            {
                ///< All points on the same level. Alpha cutoff is either entierly above, or entierly below.
                if (p->alphaCutoff < a) {
                    p->vmCoverage->numAboveAlpha += 1;
                }
                else
                {
                    p->vmCoverage->numBelowAlpha += 1;
                }
            }
            else
            {
                if (bIsDegenerate)
                {
                    // Transform the edge to the local coordinate system of the texel.
                    float2 p0 = (float2)p->size * p->triangle->aabb_s - pixelf;
                    float2 p1 = (float2)p->size * p->triangle->aabb_e - pixelf;

                    // Hyperbolic paraboloid (3D surface) => Hyperbola (2D line)
                    // f(x, y) = a + b * x + c * y + d * x * y where f(x, y) = p->alphaCutoff =>
                    // a - alpha + b * x + c * y + d * x * y = 0  
                    const float4 h(a - p->alphaCutoff, b, c, d);

                    if (TestEdgeHyperbolaIntersection(p0, p1, h))
                    {
                        p->vmCoverage->numAboveAlpha += 1;
                        p->vmCoverage->numBelowAlpha += 1;
                    }
                }
                else
                {
                    for (uint32_t edge = 0; edge < 3; ++edge) 
                    {
                        // Transform the edge to the local coordinate system of the texel.
                        float2 p0 = (float2)p->size * p->triangle->getP(edge % 3) - pixelf;
                        float2 p1 = (float2)p->size * p->triangle->getP((edge + 1) % 3) - pixelf;

                        // Hyperbolic paraboloid (3D surface) => Hyperbola (2D line)
                        // f(x, y) = a + b * x + c * y + d * x * y where f(x, y) = p->alphaCutoff =>
                        // a - alpha + b * x + c * y + d * x * y = 0  
                        const float4 h(a - p->alphaCutoff, b, c, d);

                        if (TestEdgeHyperbolaIntersection(p0, p1, h))
                        {
                            p->vmCoverage->numAboveAlpha += 1;
                            p->vmCoverage->numBelowAlpha += 1;
                            break;
                        }
                    }
                }
                
            }
        }
    }
};

// ~~~~~~ ConservativeBilinearKernel ~~~~~~ 
// 
struct ConservativeBilinearKernel
{
    struct Params {
        OmmCoverage*            vmCoverage;
        float2                  invSize;
        int2                    size;
        int2                    sizeLog2;
        const TextureImpl*      texture;
        float                   alphaCutoff;
        float                   borderAlpha;
        uint32_t                mipLevel;
    };

    template<ommCpuTextureFormat eFormat, ommTextureAddressMode eTextureAddressMode, TilingMode eTilingMode, bool bTexIsPow2>
    static void run(int2 pixel, void* ctx)
    {
        // We add +0.5 here in order to compensate for the raster offset.
        const float2 pixelf = (float2)pixel + 0.5f;

        Params* p = (Params*)ctx;
        int2 coord[TexelOffset::MAX_NUM];
        omm::GatherTexCoord4<eTextureAddressMode, bTexIsPow2>(int2(pixelf), p->size, p->sizeLog2, coord);

        auto IsBorder = [](int2 coord) {
            return eTextureAddressMode == ommTextureAddressMode_Border && (coord.x == kTexCoordBorder || coord.y == kTexCoordBorder);
        };

        float4 gatherRed;
        gatherRed.x = IsBorder(coord[TexelOffset::I0x0]) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord[TexelOffset::I0x0], p->mipLevel);
        gatherRed.y = IsBorder(coord[TexelOffset::I0x1]) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord[TexelOffset::I0x1], p->mipLevel);
        gatherRed.z = IsBorder(coord[TexelOffset::I1x1]) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord[TexelOffset::I1x1], p->mipLevel);
        gatherRed.w = IsBorder(coord[TexelOffset::I1x0]) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord[TexelOffset::I1x0], p->mipLevel);

        const float min = std::min(std::min(std::min(gatherRed.x, gatherRed.y), gatherRed.z), gatherRed.w);
        const float max = std::max(std::max(std::max(gatherRed.x, gatherRed.y), gatherRed.z), gatherRed.w);

        const bool IsOpaque         = p->alphaCutoff < max;
        const bool IsTransparent    = p->alphaCutoff > min;

        if (IsOpaque) {
            p->vmCoverage->numAboveAlpha += 1;
        }

        if (IsTransparent)
        {
            p->vmCoverage->numBelowAlpha += 1;
        }
    }
};

} // namespace omm