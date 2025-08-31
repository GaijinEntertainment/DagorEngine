/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef IN_ALPHA_TEXTURE_CHANNEL
#error "IN_ALPHA_TEXTURE_CHANNEL must be defined"
#endif

#if 0
#define PRECISE precise
#else
#define PRECISE 
#endif

namespace raster
{
#define ENABLE_LEVEL_LINE_INTERSECTION  (1)
#define ENABLE_EXACT_CLASSIFICATION     (0) // The exact classification is expensive. This should only be enabled for debug purposes.

    bool IsZero(PRECISE float value)
    {
        const PRECISE float kEpsilon = 1e-6;
        return abs(value) < kEpsilon;
    }

    // Borrowed from https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
    struct Triangle
    {
        // private
        PRECISE float _Sign(PRECISE float2 p1, PRECISE float2 p2, PRECISE float2 p3)
        {
            return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
        }
        // public
        void Init(PRECISE float2 v0, PRECISE float2 v1, PRECISE float2 v2)
        {
            _v0 = v0; _v1 = v1; _v2 = v2;
        }

        bool PointInTriangle(PRECISE float2 pt)
        {
            PRECISE float d1, d2, d3;
            bool has_neg, has_pos;

            d1 = _Sign(pt, _v0, _v1);
            d2 = _Sign(pt, _v1, _v2);
            d3 = _Sign(pt, _v2, _v0);

            has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

            return !(has_neg && has_pos);
        }

        PRECISE float2 _v0;
        PRECISE float2 _v1;
        PRECISE float2 _v2;
    };

    struct Edge
    {
        // public
        void Init(PRECISE float2 p0,  PRECISE float2 p1)
        {
            _p0 = p0;
            _p1 = p1;
            _length = length(p1 - p0);
        }

        bool IsPointOnEdge(PRECISE float2 p) {
            return IsZero(length(p - _p0) + length(p - _p1) - _length);
        }
        // private
        PRECISE float2 _p0;
        PRECISE float2 _p1;
        PRECISE float _length;
    };

    void swap(inout PRECISE float2 a, inout PRECISE float2 b)
    {
        PRECISE float2 tmp = a;
        a = b;
        b = tmp;
    }

    bool IsPointInsideUnitSquare(PRECISE float2 p)
    {
        return p.x >= 0.f && p.x <= 1.f && p.y >= 0.f && p.y <= 1.f;
    }

    bool TestEdgeHyperbolaIntersection(
        PRECISE float2 p0, PRECISE float2 p1,       // 'Edge'       - Defined by the end points (in any order)
        const PRECISE float4 h              // 'Hyperbola'  - Hyperbolic curve on the form x * h.x + y * h.y + x * y + h.z + h.w = 0
    )
    {
        if (p0.x > p1.x)
            swap(p0, p1);

        Edge edge;
        edge.Init(p0, p1);

        const PRECISE float a = h.x;
        const PRECISE float b = h.y;
        const PRECISE float c = h.z;
        const PRECISE float d = h.w;

        const PRECISE float k_denum = (p1.x - p0.x);

        if (IsZero(k_denum))
        {
            const PRECISE float x = p0.x;
            const PRECISE float n = x;

            const PRECISE float c0 = d * n + c;
            const PRECISE float c1 = a + b * n;

            if (IsZero(c0))
            {
                // (edge is identical to hyperbola asymptote => no intersection)
                return false;
            }
            else
            {
                const PRECISE float y = -c1 / c0;

                return IsPointInsideUnitSquare(float2(x, y)) && edge.IsPointOnEdge(float2(x, y));
            }
        }
        else // k_denum != 0
        {
            const PRECISE float k_enum = (p1.y - p0.y);
            const PRECISE float k = k_enum / k_denum;
            const PRECISE float m = p1.y - p1.x * k;

            const PRECISE float c0 = d * k;
            const PRECISE float c1 = c * k + d * m + b;
            const PRECISE float c2 = a + c * m;

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
                    const PRECISE float x = -c2 / c1;
                    const PRECISE float y = k * x + m;

                    return IsPointInsideUnitSquare(float2(x, y)) && edge.IsPointOnEdge(float2(x, y));
                }
            }
            else  // c0 != 0
            {
                // Hyperbola - straight line intersection.

                const PRECISE float innerRoot = c1 * c1 - 4 * c0 * c2;
                const bool isRealValued = innerRoot > 0.f;

                if (isRealValued)
                {
                    const PRECISE float root = sqrt(innerRoot); // NOTE: check that it's > 0!
                    const PRECISE float x0 = 0.5f * (-c1 + root) / c0;
                    const PRECISE float x1 = 0.5f * (-c1 - root) / c0;

                    const PRECISE float2 pX0 = float2(x0, k * x0 + m);
                    const PRECISE float2 pX1 = float2(x1, k * x1 + m);

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

        return false;
    }

    void GetStateAtPixelCenter(PRECISE float2 texCoord, PRECISE float2 texel, PRECISE float2 i_verts0, PRECISE float2 i_verts1, PRECISE float2 i_verts2, uint i_primitiveId, inout bool IsOpaque, inout bool IsTransparent)
    {
        TextureFilterMode mode = (TextureFilterMode)g_GlobalConstants.FilterType;

        if (mode == TextureFilterMode::Linear)
        {
            if (g_GlobalConstants.EnableLevelLine)
            {

            #if IN_ALPHA_TEXTURE_CHANNEL == 0
                const PRECISE float4 gatherAlpha = t_alphaTexture.GatherRed(s_samplers[g_GlobalConstants.SamplerIndex], texCoord.xy, 0);
            #elif IN_ALPHA_TEXTURE_CHANNEL == 1
                const PRECISE float4 gatherAlpha = t_alphaTexture.GatherGreen(s_samplers[g_GlobalConstants.SamplerIndex], texCoord.xy, 0);
            #elif IN_ALPHA_TEXTURE_CHANNEL == 2
                const PRECISE float4 gatherAlpha = t_alphaTexture.GatherBlue(s_samplers[g_GlobalConstants.SamplerIndex], texCoord.xy, 0);
            #elif IN_ALPHA_TEXTURE_CHANNEL == 3
                const PRECISE float4 gatherAlpha = t_alphaTexture.GatherAlpha(s_samplers[g_GlobalConstants.SamplerIndex], texCoord.xy, 0);
            #else
            #error "unexpected value of IN_ALPHA_TEXTURE_CHANNEL"
            #endif

                // ~~~ Look for internal extremes ~~~ 
                if (true)
                {
                    // Output order from GatherAlpha : (-, +), (+, +), (+, -), (-, -)
                    const PRECISE float4 gatherAlphaSwizzled = gatherAlpha.wzxy;
                    // Order of gatherAlphaSwizzled : (-, -), (+, -), (-, +), (+, +)
                    // 0--->1
                    //    / 
                    //  /   
                    // 2--->3

                    const PRECISE float2 halfTexel = 0.5 * g_GlobalConstants.InvTexSize;

                    const PRECISE float2 p0 = texCoord.xy + halfTexel * float2(-1, -1);
                    const PRECISE float2 p1 = texCoord.xy + halfTexel * float2(1, -1);
                    const PRECISE float2 p2 = texCoord.xy + halfTexel * float2(-1, 1);
                    const PRECISE float2 p3 = texCoord.xy + halfTexel * float2(1, 1);

                    Triangle tri;
                    tri.Init(i_verts0, i_verts1, i_verts2);

                    const bool IsInside0 = tri.PointInTriangle(p0);
                    const bool IsInside1 = tri.PointInTriangle(p1);
                    const bool IsInside2 = tri.PointInTriangle(p2);
                    const bool IsInside3 = tri.PointInTriangle(p3);

                    const bool IsOpaque0 = g_GlobalConstants.AlphaCutoff < gatherAlphaSwizzled.x;
                    const bool IsOpaque1 = g_GlobalConstants.AlphaCutoff < gatherAlphaSwizzled.y;
                    const bool IsOpaque2 = g_GlobalConstants.AlphaCutoff < gatherAlphaSwizzled.z;
                    const bool IsOpaque3 = g_GlobalConstants.AlphaCutoff < gatherAlphaSwizzled.w;

                    IsOpaque =  IsOpaque ||
                                (IsInside0 && IsOpaque0) ||
                                (IsInside1 && IsOpaque1) || 
                                (IsInside2 && IsOpaque2) || 
                                (IsInside3 && IsOpaque3);
                        
                    IsTransparent =  IsTransparent ||
                                    (IsInside0 && !IsOpaque0) ||
                                    (IsInside1 && !IsOpaque1) || 
                                    (IsInside2 && !IsOpaque2) || 
                                    (IsInside3 && !IsOpaque3);
                    

                    // We've already concluded it's unknown -> return!
                    if (IsOpaque && IsTransparent)
                    {
                        return;
                    }
                }

                // ~~~ Look for external extremes that may cause intersections ~~~ 
                // Optionally disable this in a "fast"-mode.
                if (true)
                {
                    // Output order from GatherAlpha : (-, +), (+, +), (+, -), (-, -)
                    const PRECISE float4 gatherAlphaSwizzled = gatherAlpha.wxyz;
                    // Order of gatherAlphaSwizzled : (-, -), (-, +), (+, +), (+, -)
                    // 0--->1
                    //      |
                    //      |
                    // 3<---2

                    const PRECISE float min_a = min(gatherAlphaSwizzled.x, min(gatherAlphaSwizzled.y, min(gatherAlphaSwizzled.z, gatherAlphaSwizzled.w)));
                    const PRECISE float max_a = max(gatherAlphaSwizzled.x, max(gatherAlphaSwizzled.y, max(gatherAlphaSwizzled.z, gatherAlphaSwizzled.w)));

                    // Intersections with level lines is loosley based on
                    // "Extraction of the Level Lines of a Bilinear Image"
                    // https://www.ipol.im/pub/art/2019/269/article.pdf
                    // 

                    const PRECISE float p0 = gatherAlphaSwizzled.x;
                    const PRECISE float p1 = gatherAlphaSwizzled.y;
                    const PRECISE float p2 = gatherAlphaSwizzled.z;
                    const PRECISE float p3 = gatherAlphaSwizzled.w;

                    //  Compute hyperbolic params...
                    const PRECISE float a = p0;
                    const PRECISE float b = p3 - p0;
                    const PRECISE float c = p1 - p0;
                    const PRECISE float d = p0 + p2 - p1 - p3;

                    if (IsZero(b) && IsZero(c) && IsZero(d))  ///< All points on the same level.
                    {
                        if (g_GlobalConstants.AlphaCutoff < a) {
                            IsOpaque = 1;
                        }
                        else
                        {
                            IsTransparent = 1;
                        }
                    }
                    else
                    {
                        [unroll]
                        for (uint edge = 0; edge < 3; ++edge) {

                            PRECISE float2 p0;
                            PRECISE float2 p1;

                            if (edge == 0)
                            {
                                p0 = i_verts0;
                                p1 = i_verts1;
                            }
                            else if (edge == 1)
                            {
                                p0 = i_verts1;
                                p1 = i_verts2;
                            }
                            else
                            {
                                p0 = i_verts2;
                                p1 = i_verts0;
                            }

                            if (p0.x > p1.x)
                            {
                                PRECISE float2 tmp = p1;
                                p1 = p0;
                                p0 = tmp;
                            }

                            p0 = p0 * g_GlobalConstants.TexSize - (float2)(texel);
                            p1 = p1 * g_GlobalConstants.TexSize - (float2)(texel);
                            // Hyperbolic paraboloid (3D surface) => Hyperbola (2D line)
                            // f(x, y) = a + b * x + c * y + d * x * y where f(x, y) = p->alphaCutoff =>
                            // a - alpha + b * x + c * y + d * x * y = 0  
                            const PRECISE float4 h = float4(a - g_GlobalConstants.AlphaCutoff, b, c, d);

                            if (TestEdgeHyperbolaIntersection(p0, p1, h))
                            {
                                IsOpaque = 1;
                                IsTransparent = 1;
                                return;
                            }
                        }
                    }
                }
            }
            else
            {
                #if IN_ALPHA_TEXTURE_CHANNEL == 0
                    const PRECISE float4 alpha = t_alphaTexture.GatherRed(s_samplers[g_GlobalConstants.SamplerIndex], texCoord.xy, 0);
                #elif IN_ALPHA_TEXTURE_CHANNEL == 1
                    const PRECISE float4 alpha = t_alphaTexture.GatherGreen(s_samplers[g_GlobalConstants.SamplerIndex], texCoord.xy, 0);
                #elif IN_ALPHA_TEXTURE_CHANNEL == 2
                    const PRECISE float4 alpha = t_alphaTexture.GatherBlue(s_samplers[g_GlobalConstants.SamplerIndex], texCoord.xy, 0);
                #elif IN_ALPHA_TEXTURE_CHANNEL == 3
                    const PRECISE float4 alpha = t_alphaTexture.GatherAlpha(s_samplers[g_GlobalConstants.SamplerIndex], texCoord.xy, 0);
                #else
                #error "unexpected value of IN_ALPHA_TEXTURE_CHANNEL"
                #endif

                    const PRECISE float min_a = min(alpha.x, min(alpha.y, min(alpha.z, alpha.w)));
                    const PRECISE float max_a = max(alpha.x, max(alpha.y, max(alpha.z, alpha.w)));

                    IsOpaque |= g_GlobalConstants.AlphaCutoff < max_a;
                    IsTransparent |= g_GlobalConstants.AlphaCutoff >= min_a;
            }
        }
        else // if (mode == TextureFilterMode::Nearest)
        {
            const PRECISE float4 color = t_alphaTexture.SampleLevel(s_samplers[g_GlobalConstants.SamplerIndex], texCoord.xy, 0);

        #if IN_ALPHA_TEXTURE_CHANNEL == 0
                const PRECISE float alpha = color.r;
        #elif IN_ALPHA_TEXTURE_CHANNEL == 1
                const PRECISE float alpha = color.g;
        #elif IN_ALPHA_TEXTURE_CHANNEL == 2
                const PRECISE float alpha = color.b;
        #elif IN_ALPHA_TEXTURE_CHANNEL == 3
                const PRECISE float alpha = color.a;
        #else
        #error "unexpected value of IN_ALPHA_TEXTURE_CHANNEL"
        #endif
            IsOpaque = g_GlobalConstants.AlphaCutoff < alpha;
            IsTransparent = g_GlobalConstants.AlphaCutoff >= alpha;
        }
    }
}
