/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include <shared/math.h>
#include <shared/cpu_raster.h>
#include <shared/texture.h>
#include <shared/util.h>

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

    struct Edge
    {
        Edge(const float2& p0, const float2& p1)
            : _p0(p0)
            , _p1(p1)
            , _length(length(p1 - p0))
        {

        }

        bool IsPointOnEdge(const float2& p) const {
            const float l = length(p - _p0) + length(p - _p1) - _length;
            return IsZero(l, 1e-5f);
        }
    private:
        const float2 _p0;
        const float2 _p1;
        const float _length;
    };
     
    static bool IsZero(float value, float kEpsilon = 1e-6f) {
        return std::abs(value) < kEpsilon;
    };

    static bool IsPointInsideUnitSquare(const float2& p)
    {
        return p.x >= 0.f && p.x <= 1.f && p.y >= 0.f && p.y <= 1.f;
    }

    static bool TestEdgeHyperbolaIntersection(
        float2 p0, float2 p1,       // 'Edge'       - Defined by the end points (in any order)
        const float4& h             // 'Hyperbola'  - Hyperbolic curve on the form x * h.x + y * h.y + x * y + h.z + h.w = 0
    )
    {
        if (p0.x > p1.x)
            std::swap(p0, p1);

        const Edge edge(p0, p1);

        const float a = h.x;
        const float b = h.y;
        const float c = h.z;
        const float d = h.w;

        const float k_denum = (p1.x - p0.x);

        if (IsZero(k_denum))
        {
            const float x = p0.x;
            const float n = x;
            
            const float c0 = d * n + c;
            const float c1 = a + b * n;

            if (IsZero(c0))
            {
                // (edge is identical to hyperbola asymptote => no intersection)
                return false;
            }
            else
            {
                const float y = -c1 / c0;

                return IsPointInsideUnitSquare(float2(x, y)) && edge.IsPointOnEdge(float2(x, y));
            }
        }
        else // k_denum != 0
        { 
            const float k_enum  = (p1.y - p0.y);
            const float k       = k_enum / k_denum; 
            const float m       = p1.y - p1.x * k;

            const float c0      = d * k;
            const float c1      = c * k + d * m + b;
            const float c2      = a + c * m;

            if (IsZero(c0))  // Hyperbola is not a hyperbola. It's a straight line.
            {
                if (IsZero(c1))
                {
                    // Lines are parallel -> no solution
                    return false;
                }
                else
                {
                    // Intersection point of non-parallel straight lines
                    const float x = -c2 / c1;
                    const float y = k * x + m;

                    return IsPointInsideUnitSquare(float2(x, y)) && edge.IsPointOnEdge(float2(x, y));
                }
            }
            else  // c0 != 0
            {
                // Hyperbola - straight line intersection.

                const float innerRoot   = c1 * c1 - 4 * c0 * c2;
                const bool isRealValued = innerRoot > 0.f;

                if (isRealValued)
                {
                    const float root = sqrt(innerRoot); // NOTE: check that it's > 0!
                    const float x0 = 0.5f * (-c1 + root) / c0;
                    const float x1 = 0.5f * (-c1 - root) / c0;

                    const float2 pX0 = float2(x0, k * x0 + m);
                    const float2 pX1 = float2(x1, k * x1 + m);

                    const bool pX0Intersects = IsPointInsideUnitSquare(pX0) && edge.IsPointOnEdge(pX0);
                    const bool pX1Intersects = IsPointInsideUnitSquare(pX1) && edge.IsPointOnEdge(pX1);
                    // At least a single intersection point inside the triangle
                    return pX0Intersects || pX1Intersects; 
                }
                else
                {
                    // No real valued roots -> no intersection point.
                    return false;
                }
            }
        }

        OMM_ASSERT(false);
        return false;
    }
public:

    template<ommCpuTextureFormat eFormat, ommTextureAddressMode eTextureAddressMode, TilingMode eTilingMode, bool bIsDegenerate>
    static void run(int2 pixel, void* ctx)
    {
        // We add +0.5 here in order to compensate for the raster offset.
        const float2 pixelf = (float2)pixel + 0.5f;

        Params* p = (Params*)ctx;
        int2 coord[TexelOffset::MAX_NUM];
        omm::GatherTexCoord4<eTextureAddressMode>(glm::floor(pixelf), p->size, coord);

        auto IsBorder = [](int2 coord) {
            return eTextureAddressMode == ommTextureAddressMode_Border && (coord.x == kTexCoordBorder || coord.y == kTexCoordBorder);
        };

        float4 gatherRed;
        gatherRed.x = IsBorder(coord[TexelOffset::I0x0]) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord[TexelOffset::I0x0], p->mipLevel);
        gatherRed.y = IsBorder(coord[TexelOffset::I0x1]) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord[TexelOffset::I0x1], p->mipLevel);
        gatherRed.z = IsBorder(coord[TexelOffset::I1x1]) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord[TexelOffset::I1x1], p->mipLevel);
        gatherRed.w = IsBorder(coord[TexelOffset::I1x0]) ? p->borderAlpha : p->texture->Load<eFormat, eTilingMode>(coord[TexelOffset::I1x0], p->mipLevel);


        // ~~~ Look for internal extremes ~~~ 
		if (!bIsDegenerate)
        {
            const bool IsOpaque0 = p->alphaCutoff < gatherRed.x;
            const bool IsOpaque1 = p->alphaCutoff < gatherRed.y;
            const bool IsOpaque2 = p->alphaCutoff < gatherRed.z;
            const bool IsOpaque3 = p->alphaCutoff < gatherRed.w;


            Triangle t;
            t.Init(p->triangle->p0, p->triangle->p1, p->triangle->p2);

            const float2 p0x0 = p->invSize * (pixelf + float2(0.0f, 0.0f));
            const float2 p0x1 = p->invSize * (pixelf + float2(0.0f, 1.0f));
            const float2 p1x1 = p->invSize * (pixelf + float2(1.0f, 1.0f));
            const float2 p1x0 = p->invSize * (pixelf + float2(1.0f, 0.0f));

            const bool IsInside0 = t.PointInTriangle(p0x0);
            const bool IsInside1 = t.PointInTriangle(p0x1);
            const bool IsInside2 = t.PointInTriangle(p1x1);
            const bool IsInside3 = t.PointInTriangle(p1x0);

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
                    const float2 p0 = (float2)p->size * p->triangle->aabb_s - pixelf;
                    const float2 p1 = (float2)p->size * p->triangle->aabb_e - pixelf;

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
                        const float2 p0 = (float2)p->size * p->triangle->getP(edge % 3) - pixelf;
                        const float2 p1 = (float2)p->size * p->triangle->getP((edge + 1) % 3) - pixelf;

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
        const TextureImpl*      texture;
        float                   alphaCutoff;
        float                   borderAlpha;
        uint32_t                mipLevel;
    };

    template<ommCpuTextureFormat eFormat, ommTextureAddressMode eTextureAddressMode, TilingMode eTilingMode>
    static void run(int2 pixel, void* ctx)
    {
        // We add +0.5 here in order to compensate for the raster offset.
        const float2 pixelf = (float2)pixel + 0.5f;

        Params* p = (Params*)ctx;
        int2 coord[TexelOffset::MAX_NUM];
        omm::GatherTexCoord4<eTextureAddressMode>(glm::floor(pixelf), p->size, coord);

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