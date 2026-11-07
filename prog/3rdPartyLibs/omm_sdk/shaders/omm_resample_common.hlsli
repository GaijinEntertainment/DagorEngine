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

    // Gaijin: the edge as p0 + t * e and the alpha as a quadratic in t, in place of the slope form, its roots and
    // the point-on-edge test: no square root, no special cases, and no absolute epsilon that rejects real
    // crossings in float32.
    // One axis of the cell [0,1]: narrows [t0, t1] to the part of p + t * e inside it.
    void ClipToCellAxis(PRECISE float p, PRECISE float e, inout PRECISE float t0, inout PRECISE float t1)
    {
        const PRECISE float inv = rcp(e); // inf for e == 0, which the selects below replace
        const PRECISE float ta = -p * inv;
        const PRECISE float tb = (1.0f - p) * inv;
        const bool inSlab = p >= 0.0f && p <= 1.0f;
        t0 = max(t0, e != 0.0f ? min(ta, tb) : (inSlab ? 0.0f : 2.0f));
        t1 = min(t1, e != 0.0f ? max(ta, tb) : (inSlab ? 1.0f : -1.0f));
    }

    // Gaijin: use optimized branchless hyperbola intersection function
    // True when the level line h.x + h.y*x + h.z*y + h.w*x*y = 0 crosses the edge p0-p1 inside the cell [0,1]^2.
    bool TestEdgeHyperbolaIntersection(PRECISE float2 p0, PRECISE float2 p1, const PRECISE float4 h)
    {
        const PRECISE float2 e = p1 - p0;
        PRECISE float t0 = 0.0f;
        PRECISE float t1 = 1.0f;
        ClipToCellAxis(p0.x, e.x, t0, t1);
        ClipToCellAxis(p0.y, e.y, t0, t1);

        const PRECISE float A = h.w * e.x * e.y;
        const PRECISE float B = h.y * e.x + h.z * e.y + h.w * (p0.x * e.y + p0.y * e.x);
        const PRECISE float C = h.x + h.y * p0.x + h.z * p0.y + h.w * p0.x * p0.y;
        const PRECISE float g0 = (A * t0 + B) * t0 + C;
        const PRECISE float g1 = (A * t1 + B) * t1 + C;
        // Same sign at both ends: only the vertex of the parabola can cross. A == 0 puts tv out of range.
        const PRECISE float tv = -B / (2.0f * A);
        const PRECISE float gv = (A * tv + B) * tv + C;
        const bool vertexCross = tv > t0 && tv < t1 && g0 * gv <= 0.0f;
        return t0 <= t1 && (g0 * g1 <= 0.0f || vertexCross);
    }

    // Gaijin: one Gather of the alpha channel for GetStateAtPixelCenter and for the CS opacity table
    PRECISE float4 GatherAlphaTex(PRECISE float2 texCoord)
    {
#if IN_ALPHA_TEXTURE_CHANNEL == 0
        const PRECISE float4 gatherAlpha = OMM_ALPHA_GATHER_R(texCoord);
#elif IN_ALPHA_TEXTURE_CHANNEL == 1
        const PRECISE float4 gatherAlpha = OMM_ALPHA_GATHER_G(texCoord);
#elif IN_ALPHA_TEXTURE_CHANNEL == 2
        const PRECISE float4 gatherAlpha = OMM_ALPHA_GATHER_B(texCoord);
#elif IN_ALPHA_TEXTURE_CHANNEL == 3
        const PRECISE float4 gatherAlpha = OMM_ALPHA_GATHER_A(texCoord);
#else
#error "unexpected value of IN_ALPHA_TEXTURE_CHANNEL"
#endif

        return gatherAlpha;
    }

    void GetStateAtPixelCenter(PRECISE float2 texCoord, PRECISE float2 texel, PRECISE float2 i_verts0, PRECISE float2 i_verts1, PRECISE float2 i_verts2, uint i_primitiveId, inout bool IsOpaque, inout bool IsTransparent)
    {
        TextureFilterMode mode = (TextureFilterMode)g_GlobalConstants.FilterType;

        if (mode == TextureFilterMode::Linear)
        {
            if (g_GlobalConstants.EnableLevelLine)
            {
                const PRECISE float4 gatherAlpha = GatherAlphaTex(texCoord);

                const PRECISE float min_a = min(gatherAlpha.x, min(gatherAlpha.y, min(gatherAlpha.z, gatherAlpha.w)));
                const PRECISE float max_a = max(gatherAlpha.x, max(gatherAlpha.y, max(gatherAlpha.z, gatherAlpha.w)));

                // Gaijin: a bilinear cell takes its extremes at the corners, thus with the cutoff outside
                // [min_a, max_a) the whole cell is on one side: no level line, no need to test corners and edges.
                // The conservative raster square of the texel is this cell, so the micro-triangle covers a part of it.
                if (g_GlobalConstants.AlphaCutoff < min_a || g_GlobalConstants.AlphaCutoff >= max_a)
                {
                    IsOpaque |= g_GlobalConstants.AlphaCutoff < min_a;
                    IsTransparent |= g_GlobalConstants.AlphaCutoff >= max_a;

                    return;
                }

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
            else
            {
                    const PRECISE float4 alpha = GatherAlphaTex(texCoord);

                    const PRECISE float min_a = min(alpha.x, min(alpha.y, min(alpha.z, alpha.w)));
                    const PRECISE float max_a = max(alpha.x, max(alpha.y, max(alpha.z, alpha.w)));

                    IsOpaque |= g_GlobalConstants.AlphaCutoff < max_a;
                    IsTransparent |= g_GlobalConstants.AlphaCutoff >= min_a;
            }
        }
        else // if (mode == TextureFilterMode::Nearest)
        {
            const PRECISE float4 color = OMM_ALPHA_SAMPLE_LEVEL(texCoord.xy);

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
