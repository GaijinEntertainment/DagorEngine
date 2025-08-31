/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#include "omm_platform.hlsli"
#include "omm.hlsli"
#include "omm_global_cb.hlsli"
#include "omm_global_samplers.hlsli"
#include "omm_rasterize.cs.resources.hlsli"

OMM_DECLARE_GLOBAL_CONSTANT_BUFFER
OMM_DECLARE_GLOBAL_SAMPLERS
OMM_DECLARE_LOCAL_CONSTANT_BUFFER
OMM_DECLARE_INPUT_RESOURCES
OMM_DECLARE_OUTPUT_RESOURCES
OMM_DECLARE_SUBRESOURCES

#include "omm_common.hlsli"
#include "omm_resample_common.hlsli"

namespace bird
{

uint extractEvenBits(uint32_t x)
{
    x &= 0x55555555;
    x = (x | (x >> 1)) & 0x33333333;
    x = (x | (x >> 2)) & 0x0f0f0f0f;
    x = (x | (x >> 4)) & 0x00ff00ff;
    x = (x | (x >> 8)) & 0x0000ffff;
    return x;
}

// Calculate exclusive prefix or (log(n) XOR's and SHF's)
uint prefixEor(uint x)
{
    x ^= x >> 1;
    x ^= x >> 2;
    x ^= x >> 4;
    x ^= x >> 8;
    return x;
}

// Convert distance along the curve to discrete barycentrics
void index2dbary(uint index, out uint u, out uint v, out uint w)
{
    uint b0 = extractEvenBits(index);
    uint b1 = extractEvenBits(index >> 1);

    uint fx = prefixEor(b0);
    uint fy = prefixEor(b0 & ~b1);

    uint t = fy ^ b1;

    u = (fx & ~t) | (b0 & ~t) | (~b0 & ~fx & t);
    v = fy ^ b0;
    w = (~fx & ~t) | (b0 & ~t) | (~b0 & fx & t);
}

// Compute barycentrics for micro triangle
void index2bary(uint index, uint subdivisionLevel, out float2 uv0, out float2 uv1, out float2 uv2)
{
    if (subdivisionLevel == 0)
    {
        uv0 = float2( 0, 0 );
        uv1 = float2( 1, 0 );
        uv2 = float2( 0, 1 );
        return;
    }

    uint iu, iv, iw;
    index2dbary(index, iu, iv, iw);

    // we need to only look at "level" bits
    iu = iu & ((1u << subdivisionLevel) - 1u);
    iv = iv & ((1u << subdivisionLevel) - 1u);
    iw = iw & ((1u << subdivisionLevel) - 1u);

    bool upright = (iu & 1) ^ (iv & 1) ^ (iw & 1);
    if (!upright)
    {
        iu = iu + 1;
        iv = iv + 1;
    }

    const uint levelScalei = ((127u - subdivisionLevel) << 23);
    const float levelScale = asfloat(levelScalei);

    // scale the barycentic coordinate to the global space/scale
    float du = 1.f * levelScale;
    float dv = 1.f * levelScale;

    // scale the barycentic coordinate to the global space/scale
    float u = (float)iu * levelScale;
    float v = (float)iv * levelScale;

    if (!upright)
    {
        du = -du;
        dv = -dv;
    }

    uv0 = float2( u, v );
    uv1 = float2( u + du, v );
    uv2 = float2( u, v + dv );
}

}

PRECISE float3 InitBarycentrics(float2 uv) {
    return float3( 1.f - uv.x - uv.y, uv.x, uv.y );
}

enum class WindingOrder {
    CW,
    CCW,
};

WindingOrder GetWinding(const PRECISE float2 _p0, const PRECISE float2 _p1, const PRECISE float2 _p2) {
    PRECISE float3 a = float3(_p2 - _p0, 0);
    PRECISE float3 b = float3(_p1 - _p0, 0);
    const PRECISE float3 N = cross(a, b);
    const PRECISE float Nz = N.z;
    if (Nz < 0)
        return WindingOrder::CCW;

    return WindingOrder::CW;
}

struct Triangle 
{
    void Init(float2 p0, PRECISE float2 p1, PRECISE float2 p2)
    {
        p[0] = p0;
        p[1] = p1;
        p[2] = p2;
        _winding = GetWinding(p0, p1, p2);
        aabb_s = float2( min(min(p0.x, p1.x), p2.x), min(min(p0.y, p1.y), p2.y) );
        aabb_e = float2( max(max(p0.x, p1.x), p2.x), max(max(p0.y, p1.y), p2.y) );
    }
 
    PRECISE float2 p[3];

    PRECISE float2 aabb_s;          //< Start point of the aabb
    PRECISE float2 aabb_e;          //< End point of the aabb
    WindingOrder _winding;  //< This matters when calculating barycentrics during rasterization.
};

PRECISE float2 InterpolateTriangleUV(float3 bc, Triangle triangleUVs)
{
    return triangleUVs.p[0] * bc.x + triangleUVs.p[1] * bc.y + triangleUVs.p[2] * bc.z;
}

Triangle GetMicroTriangle(const Triangle t, uint index, uint subdivisionLevel)
{
    float2 uv0;
    float2 uv1;
    float2 uv2;
    bird::index2bary(index, subdivisionLevel, uv0, uv1, uv2);

    const float2 uP0 = InterpolateTriangleUV(InitBarycentrics(uv0), t);
    const float2 uP1 = InterpolateTriangleUV(InitBarycentrics(uv1), t);
    const float2 uP2 = InterpolateTriangleUV(InitBarycentrics(uv2), t);

    Triangle retT;
    retT.Init(uP0, uP1, uP2);
    return retT;
}

Triangle GetMacroTriangle(uint iPrimitiveIndex, out uint vmArrayOffset, out uint vmPrimitiveIndex, out OMMFormat vmFormat)
{
    const uint primitiveIndexOffset = (g_LocalConstants.PrimitiveIdOffset + iPrimitiveIndex) + g_LocalConstants.SubdivisionLevel * g_GlobalConstants.PrimitiveCount;

    vmArrayOffset = OMM_SUBRESOURCE_LOAD(RasterItemsBuffer, 8 * primitiveIndexOffset);
    const uint vmFormatAndPrimitiveIndex = OMM_SUBRESOURCE_LOAD(RasterItemsBuffer, 8 * primitiveIndexOffset + 4);
    vmFormat = (OMMFormat)(vmFormatAndPrimitiveIndex >> 30);
    vmPrimitiveIndex = vmFormatAndPrimitiveIndex & 0x3FFFFFFF;

    const TexCoords texCoords = FetchTexCoords(t_indexBuffer, t_texCoordBuffer, vmPrimitiveIndex);

    Triangle retT;
    retT.Init(texCoords.p0, texCoords.p1, texCoords.p2);
    return retT;
}

// Edge rasterizer: https://www.cs.drexel.edu/~david/Classes/Papers/comp175-06-pineda.pdf
    // Conservative rasterization extension : https://fileadmin.cs.lth.se/graphics/research/papers/2005/cr/_conservative.pdf
struct StatelessRasterizer
{
// private:
    struct EdgeFn {
        PRECISE float2 N;
        PRECISE float C;

        void Init(float2 p, PRECISE float2 q)
        {
            N = float2( q.y - p.y, p.x - q.x );
            C = -dot(N, p);
        }
    };

    bool AABBIntersect(float2 p0, PRECISE float2 e0, PRECISE float2 p1, PRECISE float2 e1) {
        return (abs((p0.x + e0.x / 2) - (p1.x + e1.x / 2)) * 2 < (e0.x + e1.x)) &&
            (abs((p0.y + e0.y / 2) - (p1.y + e1.y / 2)) * 2 < (e0.y + e1.y));
    }

    PRECISE float EvalEdge(EdgeFn e, PRECISE float2 s) {
        return dot(e.N, s) + e.C;
    }

    PRECISE float EdgeFunction(float2 a, PRECISE float2 b, PRECISE float2 c)
    {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    // Extension of EvalEdge for over-conservative raster, as explained in https://fileadmin.cs.lth.se/graphics/research/papers/2005/cr/_conservative.pdf
    PRECISE float EvalEdgeCons(EdgeFn eFn, PRECISE float2 s, PRECISE float2 ext) {
        const PRECISE float e = EvalEdge(eFn, s);
        const PRECISE float bx = eFn.N.x > 0 ? 0.f : eFn.N.x;
        const PRECISE float by = eFn.N.y > 0 ? 0.f : eFn.N.y;
        return e + bx * ext.x + by * ext.y;
    }

    // Extension of EvalEdge for under-conservative raster, as explained in https://fileadmin.cs.lth.se/graphics/research/papers/2005/cr/_conservative.pdf
    PRECISE float EvalEdgeUnderCons(EdgeFn eFn, PRECISE float2 s, PRECISE float2 ext) {
        const PRECISE float e = EvalEdge(eFn, s);
        const PRECISE float bx = eFn.N.x < 0 ? 0.f : eFn.N.x;
        const PRECISE float by = eFn.N.y < 0 ? 0.f : eFn.N.y;
        return e + bx * ext.x + by * ext.y;
    }

    EdgeFn _e0, _e1, _e2;   //< Edge functions of the triangle. Cached for perf reasons.
    PRECISE float2 _aabb_start;     //< Start point of the aabb
    PRECISE float2 _aabb_size;      //< Extent of the aabb (p+e = p_end)
    PRECISE float _area2;           //< area of the triangle multiplied by 2 

// public:

    void Init(Triangle t)
    {
        _e0.Init(t.p[0], t.p[1]);
        _e1.Init(t.p[1], t.p[2]);
        _e2.Init(t.p[2], t.p[0]);

        _aabb_start = t.aabb_s;
        _aabb_size = t.aabb_e - t.aabb_s;
        _area2 = EdgeFunction(t.p[0], t.p[1], t.p[2]);
    }

    // Used for non-conservative rasterization
    bool PointInTriangle(float2 s, PRECISE float2 e) {
        // First check AABB...
        if (!AABBIntersect(s, e, _aabb_start, _aabb_size))
            return false;

        const PRECISE float eval0 = EvalEdge(_e0, s);
        const PRECISE float eval1 = EvalEdge(_e1, s);
        const PRECISE float eval2 = EvalEdge(_e2, s);
        const bool AllNeg = eval0 < 0.f && eval1 < 0.f && eval2 < 0.f;
        return AllNeg;
    }

    // Get barycentric coordinate
    PRECISE float3 GetBarycentrics(float2 s) {
        const PRECISE float3 edges = { EvalEdge(_e0, s), EvalEdge(_e1, s), EvalEdge(_e2, s) };
        const PRECISE float3 bc = edges / _area2;
        return bc;
    }

    // Used for conservative rasterization
    // s corresponds to the upper left point defining the tile (pixel)
    // e corresponds to the extents of the tile in unit of pixels
    bool SquareInTriangle(float2 s, const PRECISE float2 e) {
        // First check AABB...
        if (!AABBIntersect(s, e, _aabb_start, _aabb_size))
            return false;

        // Now do the conservative raster edge function test.
        const PRECISE float eval0 = EvalEdgeCons(_e0, s, e);
        const PRECISE float eval1 = EvalEdgeCons(_e1, s, e);
        const PRECISE float eval2 = EvalEdgeCons(_e2, s, e);
        const bool AllNeg = eval0 < 0.f && eval1 < 0.f && eval2 < 0.f;
        return AllNeg;
    }

    // This function can be used instead of SquareInTriangle if it's known that 
    // s is inside the triangle aabb.
    bool SquareInTriangleSkipAABBTest(float2 s, PRECISE float2 e) {
        // Now do the conservative raster edge function test.
        const PRECISE float eval0 = EvalEdgeCons(_e0, s, e);
        const PRECISE float eval1 = EvalEdgeCons(_e1, s, e);
        const PRECISE float eval2 = EvalEdgeCons(_e2, s, e);
        const bool AllNeg = eval0 < 0.f && eval1 < 0.f && eval2 < 0.f;
        return AllNeg;
    }

    // This function can be used instead of SquareInTriangle if it's known that 
    // s is inside the triangle aabb.
    bool SquareEntierlyInTriangleSkipAABBTest(float2 s, PRECISE float2 e) {
        // Now do the conservative raster edge function test.
        const PRECISE float eval0 = EvalEdgeUnderCons(_e0, s, e);
        const PRECISE float eval1 = EvalEdgeUnderCons(_e1, s, e);
        const PRECISE float eval2 = EvalEdgeUnderCons(_e2, s, e);
        const bool AllNeg = eval0 < 0.f && eval1 < 0.f && eval2 < 0.f;
        return AllNeg;
    }
};

uint GetMaxThreadCount()
{
    const uint strideInBytes = 4; // sizeof(uint32_t)
    const uint offset = strideInBytes * (g_LocalConstants.SubdivisionLevel);
    return OMM_SUBRESOURCE_LOAD(IEBakeCsThreadCountBuffer, offset);
}

void StoreMacroTriangleState(OpacityState opacityState, uint primitiveIndex)
{
    const uint bakeResultMacroOffset = 12 * (primitiveIndex);

    if (g_GlobalConstants.EnablePostDispatchInfoStats)
    {
        uint dummy;
        if (opacityState == OpacityState::Opaque)
            OMM_SUBRESOURCE_INTERLOCKEDADD(SpecialIndicesStateBuffer, bakeResultMacroOffset, 1, dummy);
        else if (opacityState == OpacityState::Transparent)
            OMM_SUBRESOURCE_INTERLOCKEDADD(SpecialIndicesStateBuffer, bakeResultMacroOffset + 4, 1, dummy);
        else // if (opacityState == OpacityState::UnknownTransparent || opacityState == OpacityState::UnknownOpaque)
            OMM_SUBRESOURCE_INTERLOCKEDADD(SpecialIndicesStateBuffer, bakeResultMacroOffset + 8, 1, dummy);
    }
    else if (g_GlobalConstants.EnableSpecialIndices)
    {
        if (opacityState == OpacityState::Opaque)
            OMM_SUBRESOURCE_STORE(SpecialIndicesStateBuffer, bakeResultMacroOffset, 1);
        else if (opacityState == OpacityState::Transparent)
            OMM_SUBRESOURCE_STORE(SpecialIndicesStateBuffer, bakeResultMacroOffset + 4, 1);
        else // if (opacityState == OpacityState::UnknownTransparent || opacityState == OpacityState::UnknownOpaque)
            OMM_SUBRESOURCE_STORE(SpecialIndicesStateBuffer, bakeResultMacroOffset + 8, 1);
    }
}

[numthreads(128, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x >= GetMaxThreadCount())
	    return;

    const uint threadID            = tid.x;
    const uint numMicroTriangles   = GetNumMicroTriangles(g_LocalConstants.SubdivisionLevel);
    const uint primitiveIndex      = threadID / numMicroTriangles;
    const uint microTriIndex       = threadID % numMicroTriangles;

    uint vmPrimitiveIndex;
    uint ommArrayOffset;
    OMMFormat ommFormat;
    const Triangle macroTri        = GetMacroTriangle(primitiveIndex, ommArrayOffset, vmPrimitiveIndex, ommFormat);
    const Triangle microTri        = GetMicroTriangle(macroTri, microTriIndex, g_LocalConstants.SubdivisionLevel);

    // 1. - Sample base state for micro triangle.
    // Either we find a counter-example => state is unknown
    // We fail to faind counter example => base state remains.
    bool isOpaque = false;
    bool isTransparent = false;
    {
        const PRECISE float4 color = t_alphaTexture.SampleLevel(s_samplers[g_GlobalConstants.SamplerIndex], microTri.p[0].xy, 0);
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

        isOpaque = g_GlobalConstants.AlphaCutoff < alpha;
        isTransparent = !isOpaque;
    }

    // 2. Resample properly
    {
        const bool isBackfacing = microTri._winding == WindingOrder::CW;
        const PRECISE float2 offset = float2(-0.5, -0.5);

        Triangle t;
        if (isBackfacing)
            t.Init(
                g_GlobalConstants.TexSize * microTri.p[2] + offset,
                g_GlobalConstants.TexSize * microTri.p[1] + offset,
                g_GlobalConstants.TexSize * microTri.p[0] + offset);
        else
            t.Init(
                g_GlobalConstants.TexSize * microTri.p[0] + offset,
                g_GlobalConstants.TexSize * microTri.p[1] + offset, 
                g_GlobalConstants.TexSize * microTri.p[2] + offset);

        int2 min = (int2)floor(t.aabb_s);
        int2 max = (int2)ceil(t.aabb_e);

        StatelessRasterizer _tix;
        _tix.Init(t);
        const PRECISE float2 pixelSize = float2(1, 1);

        for (int y = min.y; y < max.y; ++y)
        {
            bool wasInside = false;
            for (int x = min.x; x < max.x; ++x)
            {
                // if constexpr(eRasterMode == RasterMode::OverConservative) 
                {
                    const PRECISE float2 s = float2(x, y);

                    if (_tix.SquareInTriangleSkipAABBTest(s, pixelSize))
                    {
                        const PRECISE float2 texel = float2(x, y) + 0.5f;
                        const PRECISE float2 texCoord = (float2(x, y) + 1.0f) * g_GlobalConstants.InvTexSize;
                        raster::GetStateAtPixelCenter(texCoord, texel, microTri.p[0], microTri.p[1], microTri.p[2], primitiveIndex, isOpaque, isTransparent);

                        wasInside = true;
                    }
                    else if (wasInside)
                        break;
                }
            }
        }
    }

    const OpacityState opacityState = GetOpacityState(isOpaque, isTransparent, g_GlobalConstants.AlphaCutoffGreater, g_GlobalConstants.AlphaCutoffLessEqual, ommFormat);

    StoreMacroTriangleState(opacityState, vmPrimitiveIndex);

    const uint statesPerDW      = ommFormat == OMMFormat::OC1_4 ?  16 : 32;
    const uint logBitsPerState  = ommFormat == OMMFormat::OC1_4 ?   1 : 0;
    const uint stateMask        = ommFormat == OMMFormat::OC1_4 ? 0x3 : 0x1;

    const uint dwOffset         = microTriIndex / statesPerDW;
    const uint i                = microTriIndex % statesPerDW;

    const uint state            = stateMask & (uint)opacityState;
    const uint data             = (state << (i << logBitsPerState));

    u_vmArrayBuffer.InterlockedOr(ommArrayOffset + 4 * dwOffset, data);
}