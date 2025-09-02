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
#include "geometry.h"

namespace omm
{
    // Edge rasterizer: https://www.cs.drexel.edu/~david/Classes/Papers/comp175-06-pineda.pdf
    // Conservative rasterization extension : https://fileadmin.cs.lth.se/graphics/research/papers/2005/cr/_conservative.pdf
    class StatelessRasterizer {
    public:
        struct EdgeFn {
            float2 N;
            float C;

            EdgeFn(float2 p, float2 q) {
                N = { q.y - p.y, p.x - q.x };
                C = -glm::dot(N, p);
            }
        };

        inline static bool AABBIntersect(const float2& p0, const float2& e0, const float2& p1, const float2& e1) {
            return (std::abs((p0.x + e0.x / 2) - (p1.x + e1.x / 2)) * 2 < (e0.x + e1.x)) &&
                (std::abs((p0.y + e0.y / 2) - (p1.y + e1.y / 2)) * 2 < (e0.y + e1.y));
        }

        inline static float EvalEdge(const EdgeFn& e, const float2& s) {
            return glm::dot(e.N, s) + e.C;
        }

        inline static float EdgeFunction(const float2& a, const float2& b, const float2& c)
        {
            return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
        }

        // Extension of EvalEdge for over-conservative raster, as explained in https://fileadmin.cs.lth.se/graphics/research/papers/2005/cr/_conservative.pdf
        inline static float EvalEdgeCons(const EdgeFn& eFn, const float2& s, const float2& ext) {
            const float e = EvalEdge(eFn, s);
            const float bx = eFn.N.x > 0 ? 0.f : eFn.N.x;
            const float by = eFn.N.y > 0 ? 0.f : eFn.N.y;
            return e + bx * ext.x + by * ext.y;
        }

        // Extension of EvalEdge for under-conservative raster, as explained in https://fileadmin.cs.lth.se/graphics/research/papers/2005/cr/_conservative.pdf
        inline static float EvalEdgeUnderCons(const EdgeFn& eFn, const float2& s, const float2& ext) {
            const float e = EvalEdge(eFn, s);
            const float bx = eFn.N.x < 0 ? 0.f : eFn.N.x;
            const float by = eFn.N.y < 0 ? 0.f : eFn.N.y;
            return e + bx * ext.x + by * ext.y;
        }

        EdgeFn _e0, _e1, _e2;   //< Edge functions of the triangle. Cached for perf reasons.
        float2 _aabb_start;     //< Start point of the aabb
        float2 _aabb_size;      //< Extent of the aabb (p+e = p_end)
        float _area2;           //< area of the triangle multiplied by 2 

    public:

        StatelessRasterizer(const Triangle& t)
            : _e0(t.p0, t.p1)
            , _e1(t.p1, t.p2)
            , _e2(t.p2, t.p0)
        {
            _aabb_start     = t.aabb_s;
            _aabb_size      = t.aabb_e - t.aabb_s;
            _area2          = EdgeFunction(t.p0, t.p1, t.p2);
        }

        // Used for non-conservative rasterization
        inline bool PointInTriangle(const float2& s, const float2& e) const {
            // First check AABB...
            if (!AABBIntersect(s, e, _aabb_start, _aabb_size))
                return false;

            const float eval0 = EvalEdge(_e0, s);
            const float eval1 = EvalEdge(_e1, s);
            const float eval2 = EvalEdge(_e2, s);
            const bool AllNeg = eval0 < 0.f && eval1 < 0.f && eval2 < 0.f;
            return AllNeg;
        }

        // Get barycentric coordinate
        inline float3 GetBarycentrics(const float2& s) const {
            const float3 edges = { EvalEdge(_e0, s), EvalEdge(_e1, s), EvalEdge(_e2, s) };
            const float3 bc = edges / _area2;
            return bc;
        }

        // Used for conservative rasterization
        // s corresponds to the upper left point defining the tile (pixel)
        // e corresponds to the extents of the tile in unit of pixels
        inline bool SquareInTriangle(const float2& s, const float2& e) const {
            // First check AABB...
            if (!AABBIntersect(s, e, _aabb_start, _aabb_size))
                return false;

            // Now do the conservative raster edge function test.
            const float eval0 = EvalEdgeCons(_e0, s, e);
            const float eval1 = EvalEdgeCons(_e1, s, e);
            const float eval2 = EvalEdgeCons(_e2, s, e);
            const bool AllNeg = eval0 < 0.f && eval1 < 0.f && eval2 < 0.f;
            return AllNeg;
        }

        // This function can be used instead of SquareInTriangle if it's known that 
        // s is inside the triangle aabb.
        inline bool SquareInTriangleSkipAABBTest(const float2& s, const float2& e) const {
            // Now do the conservative raster edge function test.
            const float eval0 = EvalEdgeCons(_e0, s, e);
            const float eval1 = EvalEdgeCons(_e1, s, e);
            const float eval2 = EvalEdgeCons(_e2, s, e);
            const bool AllNeg = eval0 < 0.f && eval1 < 0.f && eval2 < 0.f;
            return AllNeg;
        }

        // This function can be used instead of SquareInTriangle if it's known that 
        // s is inside the triangle aabb.
        inline bool SquareEntierlyInTriangleSkipAABBTest(const float2& s, const float2& e) const {
            // Now do the conservative raster edge function test.
            const float eval0 = EvalEdgeUnderCons(_e0, s, e);
            const float eval1 = EvalEdgeUnderCons(_e1, s, e);
            const float eval2 = EvalEdgeUnderCons(_e2, s, e);
            const bool AllNeg = eval0 < 0.f && eval1 < 0.f && eval2 < 0.f;
            return AllNeg;
        }
    };

    // This rasterizer relies on less computation than the stateless rasterizer
    // Edge rasterizer: https://www.cs.drexel.edu/~david/Classes/Papers/comp175-06-pineda.pdf
    // Conservative rasterization extension : https://fileadmin.cs.lth.se/graphics/research/papers/2005/cr/_conservative.pdf
    class IterativeRasterizer {
    public:
        struct EdgeFn {
            float2 N;
            float C;

            EdgeFn(float2 p, float2 q) {
                N = { q.y - p.y, p.x - q.x };
                C = -glm::dot(N, p);
            }
        };

        inline static bool AABBIntersect(const float2& p0, const float2& e0, const float2& p1, const float2& e1) {
            return (std::abs((p0.x + e0.x / 2) - (p1.x + e1.x / 2)) * 2 < (e0.x + e1.x)) &&
                (std::abs((p0.y + e0.y / 2) - (p1.y + e1.y / 2)) * 2 < (e0.y + e1.y));
        }

        inline static float EvalEdge(const EdgeFn& e, const float2& s) {
            return glm::dot(e.N, s) + e.C;
        }

        // Extension of EvalEdge for conservative raster, as explained in https://fileadmin.cs.lth.se/graphics/research/papers/2005/cr/_conservative.pdf
        inline float EvalEdgeCons(const EdgeFn& eFn, float e) const {
            const float bx = eFn.N.x > 0 ? 0.f : eFn.N.x;
            const float by = eFn.N.y > 0 ? 0.f : eFn.N.y;
            return e + bx * _ext.x + by * _ext.y;
        }

        inline static float EdgeFunction(const float2& a, const float2& b, const float2& c)
        {
            return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
        }

        EdgeFn _e0, _e1, _e2;   //< Edge functions of the triangle. Cached for perf reasons.
        float  _f0, _f1, _f2;   //< The result of the edge function evaluation. Will be iteratively udpdated
        float2 _d0, _d1, _d2;   //< Constants to update edge function iteratively.
        float3 _dX, _dY;        //< Constants to update edge function iteratively.
        const float2 _ext;      //< Size of the raster grid pixels
        float2 _aabb_start;     //< Start point of the aabb
        float2 _aabb_size;      //< Extent of the aabb (p+e = p_end)
        float _area;
    public:

        IterativeRasterizer(const Triangle& t, const float2& s, float2 e)
            : _e0(t.p0, t.p1)
            , _e1(t.p1, t.p2)
            , _e2(t.p2, t.p0)
            , _ext(e)
        {
            _f0 = EvalEdge(_e0, s);
            _f1 = EvalEdge(_e1, s);
            _f2 = EvalEdge(_e2, s);
            _d0 = float2(EvalEdge(_e0, s + float2(e.x, 0)) - _f0, EvalEdge(_e0, s + float2(0, e.y)) - _f0);
            _d1 = float2(EvalEdge(_e1, s + float2(e.x, 0)) - _f1, EvalEdge(_e1, s + float2(0, e.y)) - _f1);
            _d2 = float2(EvalEdge(_e2, s + float2(e.x, 0)) - _f2, EvalEdge(_e2, s + float2(0, e.y)) - _f2);
            _aabb_start = t.aabb_s;
            _aabb_size = t.aabb_e - t.aabb_s;
            _area = EdgeFunction(t.p0, t.p1, t.p2); // area of the triangle multiplied by 2 
        }

        struct State {
            float3  _edgeF; //< The evaluated edge function for a given position.
            float3 _dX, _dY;

            void StepX() {
                _edgeF += _dX;
            }

            void StepY() {
                _edgeF += _dY;
            }
        };

        // Initialize a state at pos s.
        State InitState(const float2& s) {
            State state;
            state._edgeF.x = EvalEdge(_e0, s);
            state._edgeF.y = EvalEdge(_e1, s);
            state._edgeF.z = EvalEdge(_e2, s);

            state._dX = float3(
                EvalEdge(_e0, s + float2(_ext.x, 0)) - state._edgeF.x,
                EvalEdge(_e1, s + float2(_ext.x, 0)) - state._edgeF.y,
                EvalEdge(_e2, s + float2(_ext.x, 0)) - state._edgeF.z);

            state._dY = float3(
                EvalEdge(_e0, s + float2(0, _ext.y)) - state._edgeF.x,
                EvalEdge(_e1, s + float2(0, _ext.y)) - state._edgeF.y,
                EvalEdge(_e2, s + float2(0, _ext.y)) - state._edgeF.z);

            return state;
        }

        // This function can be used instead of SquareInTriangle if it's known that 
        // s is inside the triangle aabb.
        inline bool EvalConservative(const State& s) const {
            // Now do the conservative raster edge function test.
            const float eval0 = EvalEdgeCons(_e0, s._edgeF.x);
            const float eval1 = EvalEdgeCons(_e1, s._edgeF.y);
            const float eval2 = EvalEdgeCons(_e2, s._edgeF.z);
            const bool AllNeg = eval0 < 0.f && eval1 < 0.f && eval2 < 0.f;
            return AllNeg;
        }

        // s is inside the triangle aabb.
        inline bool Eval(const State& s) const {
            const float eval0 = s._edgeF.x;
            const float eval1 = s._edgeF.y;
            const float eval2 = s._edgeF.z;
            const bool AllNeg = eval0 < 0.f && eval1 < 0.f && eval2 < 0.f;
            return AllNeg;
        }

        // s is inside the triangle aabb.
        inline float3 GetBarycentrics(const State& s) const {
            const float eval0 = s._edgeF.x;
            const float eval1 = s._edgeF.y;
            const float eval2 = s._edgeF.z;
            return float3(eval0, eval1, eval2) / _area;
        }
    };

    enum RasterMode {
        Default,
        OverConservative,
        UnderConservative,
    };

    enum Coverage {
        PartiallyCovered,
        FullyCovered,
    };

    // t - the triangle to rasterize
    // r - the pixel resolution to rasterize at.
    // f - the function callback, _should_ be inlined when using lambdas.
    template <RasterMode eRasterMode, bool EnableParallel, bool EnableBarycentrics, typename F>
    inline void RasterizeTriImpl(const Triangle& _t, int2 r, const float2& offset, F f, void* context = nullptr) {

        // Obvious optimizations this rasterizer does _not_ do:
        // No coarse raster step - might be useful for large triangles,
        // Tight triangle traversal, right now it searches row wise and terminates on first exit.
        
        // constexpr bool EnableBarycentrics = false;

        OMM_ASSERT(!_t.GetIsDegenerate());

        // Scanline approaches could be investigated as well.
        const bool isCCW = _t.GetIsCCW();

        const float2 rf = float2(r);
        const float2 invSize = 1.f / rf;
        // Rasterizer expects CCW triangles.
        Triangle t = isCCW ? Triangle(_t.p0 * rf + offset, _t.p1 * rf + offset, _t.p2 * rf + offset)
                                    : Triangle(_t.p2 * rf + offset, _t.p1 * rf + offset, _t.p0 * rf + offset);
        OMM_ASSERT(t.GetIsCCW());

        const int2 min = int2{ glm::floor(t.aabb_s) };
        const int2 max = int2{ glm::ceil(t.aabb_e) };

        OMM_ASSERT(min.x < max.x);
        OMM_ASSERT(min.y < max.y);

        const StatelessRasterizer _tix(t);

        const float2 pixelSize(1, 1);

        #pragma omp parallel for if (EnableParallel)
        for (int y = min.y; y < max.y; ++y) {
            bool wasInside = false;

            for (int x = min.x; x < max.x; ++x) {
                int2 it = int2({ x, y });
                if constexpr (eRasterMode == RasterMode::OverConservative) {
                    const float2 s = float2(x, y);

                    if (_tix.SquareInTriangleSkipAABBTest(s, pixelSize)) {
                        const float2 s_c = (s + 0.5f);

                        if constexpr (EnableBarycentrics)
                        {
                            float3 bc = _tix.GetBarycentrics(s_c);
                            if (!isCCW)
                                bc = { bc.z, bc.y, bc.x };

                            f(int2({ x, y }), &bc, context);
                        }
                        else
                        {
                           f(int2({ x, y }), context);
                        }
                        wasInside = true;
                    }
                    else if (wasInside)
                        break;
                } else if constexpr (eRasterMode == RasterMode::UnderConservative) {

                    const float2 s = float2(x, y);

                    if (_tix.SquareEntierlyInTriangleSkipAABBTest(s, pixelSize)) {
                        const float2 s_c = (s + 0.5f);

                        if constexpr (EnableBarycentrics)
                        {
                            float3 bc = _tix.GetBarycentrics(s_c);
                            if (!isCCW)
                                bc = { bc.z, bc.y, bc.x };

                            f(int2({ x, y }), &bc, context);
                        }
                        else
                        {
                            f(int2({ x, y }), context);
                        }
                        wasInside = true;
                    }
                    else if (wasInside)
                        break;
                }
                else if constexpr (eRasterMode == RasterMode::Default) {

                    const float2 s = (float2(x, y) + 0.5f);
                    if (_tix.PointInTriangle(s, pixelSize)) 
                    {
                        if constexpr (EnableBarycentrics)
                        {
                            float3 bc = _tix.GetBarycentrics(s);
                            if (!isCCW)
                                bc = { bc.z, bc.y, bc.x };
                            f(int2({ x, y }), &bc, context);
                        }
                        else
                        {
                           f(int2({ x, y }), context);
                        }
                        wasInside = true;
                    }
                    else if (wasInside)
                        break;
                }
            }
        }
    }

    template <bool EnableParallel, bool EnableBarycentrics, typename F>
    inline void RasterizeLineImpl(const Line& _l, int2 r, const float2& offset, F f, void* context = nullptr)
    {
        Line l = _l.p0.x > _l.p1.x ? Line(_l.p1, _l.p0) : _l;

        // Bresenham's line algorithm
        // src: https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

        int x0 = (int)(l.p0.x * r.x);
        int x1 = (int)(l.p1.x * r.x);
        int y0 = (int)(l.p0.y * r.y);
        int y1 = (int)(l.p1.y * r.y);

        auto plotLineLow = [&f, context](int x0, int y0, int x1, int y1) {
            int dx = x1 - x0;
            int dy = y1 - y0;
            int yi = 1;
            if (dy < 0)
            {
                yi = -1;
                dy = -dy;
            }
            int D = (2 * dy) - dx;
            int y = y0;

            for (int x = x0; x <= x1; ++x)
            {
                if constexpr (EnableBarycentrics)
                {
                    const float3 zero(0, 0, 0);
                    f(int2({ x, y }), &zero, context);
                }
                else
                {
                    f(int2({ x, y }), context);
                }

                if (D > 0)
                {
                    y = y + 1;
                    D = D + (2 * (dy - dx));
                }
                else
                {
                    D = D + 2 * dy;
                }
            }
            };

        auto plotLineHigh = [&f, context](int x0, int y0, int x1, int y1) {
            int dx = x1 - x0;
            int dy = y1 - y0;
            int xi = 1;
            if (dx < 0)
            {
                xi = -1;
                dx = -dx;
            }
            int D = (2 * dx) - dy;
            int x = x0;

            for (int y = y0; y <= y1; ++y)
            {
                if constexpr (EnableBarycentrics)
                {
                    const float3 zero(0, 0, 0);
                    f(int2({ x, y }), &zero, context);
                }
                else
                {
                    f(int2({ x, y }), context);
                }

                if (D > 0)
                {
                    x = x + xi;
                    D = D + (2 * (dx - dy));
                }
                else
                {
                    D = D + 2 * dx;
                }
            }
            };

        if (abs(y1 - y0) < abs(x1 - x0))
        {
            if (x0 > x1)
                plotLineLow(x1, y1, x0, y0);
            else
                plotLineLow(x0, y0, x1, y1);
        }
        else
        {
            if (y0 > y1)
                plotLineHigh(x1, y1, x0, y0);
            else
                plotLineHigh(x0, y0, x1, y1);
        }
    }

    template <typename F>
    inline void RasterizeLineConservativeImpl(const Line& _l, int2 r, const float2& offset, F f, void* context = nullptr)
    {
        const float2 rf(r);
        Line l = Line(_l.p0 * rf + offset, _l.p1 * rf + offset);

        if (l.p0.x > l.p1.x)
        {
            l = Line(l.p1, l.p0);
        }

        const int2 gridSize = r;
        const float2 rayDirection = l.p1 - l.p0;
        const float2 rayOrigin = l.p0;
        const float tEnd = glm::length(rayDirection);

        int x = static_cast<int>(glm::floor(l.p0.x));
        int y = static_cast<int>(glm::floor(l.p0.y));

        int stepX = (rayDirection.x > 0) ? 1 : ((rayDirection.x < 0) ? -1 : 0);
        int stepY = (rayDirection.y > 0) ? 1 : ((rayDirection.y < 0) ? -1 : 0);

        float tDeltaX = (stepX != 0) ? 1.f / std::abs(rayDirection.x) : std::numeric_limits<float>::infinity();
        float tDeltaY = (stepY != 0) ? 1.f / std::abs(rayDirection.y) : std::numeric_limits<float>::infinity();

        // Calculate tMax (distance to the next cell boundary)
        float tMaxX, tMaxY;

        if (stepX != 0) {
            float nextCellBoundary = (x + (stepX > 0 ? 1.f : 0.f));
            tMaxX = (nextCellBoundary - rayOrigin.x) / rayDirection.x;
        }
        else {
            tMaxX = std::numeric_limits<float>::infinity();
        }

        if (stepY != 0) {
            float nextCellBoundary = (y + (stepY > 0 ? 1.f : 0.f));
            tMaxY = (nextCellBoundary - rayOrigin.y) / rayDirection.y;
        }
        else {
            tMaxY = std::numeric_limits<float>::infinity();
        }

        if (stepX == 0 && stepY == 0)
        {
            f(int2(x, y), context);
            return;
        }

        const int yMin = (int)std::min(glm::floor(l.p0.y), glm::floor(l.p1.y));
        const int yMax = (int)std::max(glm::ceil(l.p0.y), glm::ceil(l.p1.y));

        const int xMin = (int)std::min(glm::floor(l.p0.x), glm::floor(l.p1.x));
        const int xMax = (int)std::max(glm::ceil(l.p0.x), glm::ceil(l.p1.x));

        while (x >= xMin && x <= xMax && y >= yMin && y <= yMax) 
        {
            f(int2(x, y), context);

            if (tMaxX < tMaxY) {
                x += stepX;
                tMaxX += tDeltaX;
            }
            else {
                y += stepY;
                tMaxY += tDeltaY;
            }
        }
    }

    template <typename F>
    inline void RasterizeConservativeSerial(const Triangle& t, int2 r, F f, void* context = nullptr) { RasterizeTriImpl<RasterMode::OverConservative, false, false>(t, r, float2{0,0}, f, context); };

    template <typename F>
    inline void RasterizeConservativeSerialBarycentrics(const Triangle& t, int2 r, F f, void* context = nullptr) { RasterizeTriImpl<RasterMode::OverConservative, false, true>(t, r, float2{ 0,0 }, f, context); };

    template <typename F>
    inline void RasterizeConservativeSerialWithOffset(const Triangle& t, int2 r, float2 offset, F f, void* context = nullptr) { RasterizeTriImpl<RasterMode::OverConservative, false, false>(t, r, offset, f, context); };

    template <typename F>
    inline void RasterizeConservativeSerialWithOffsetCoverage(const Triangle& t, int2 r, float2 offset, F f, void* context = nullptr) { RasterizeTriImpl<RasterMode::OverConservative, false, false>(t, r, offset, f, context); };

    template <typename F>
    inline void RasterizeConservativeParallel(const Triangle& t, int2 r, F f, void* context = nullptr) { RasterizeTriImpl<RasterMode::OverConservative, true, false>(t, r, float2{ 0,0 }, f, context); };

    template <typename F>
    inline void RasterizeConservativeParallelBarycentrics(const Triangle& t, int2 r, F f, void* context = nullptr) { RasterizeTriImpl<RasterMode::OverConservative, true, true>(t, r, float2{ 0,0 }, f, context); };

    template <typename F>
    inline void RasterizeUnderConservative(const Triangle& t, int2 r, F f, void* context = nullptr) { RasterizeTriImpl<RasterMode::UnderConservative, false, false>(t, r, float2{ 0,0 }, f, context); };

    template <typename F>
    inline void RasterizeUnderConservativeBarycentrics(const Triangle& t, int2 r, F f, void* context = nullptr) { RasterizeTriImpl<RasterMode::UnderConservative, false, true>(t, r, float2{ 0,0 }, f, context); };

    template <typename F>
    inline void RasterizeSerial(const Triangle& t, int2 r, F f, void* context = nullptr) { RasterizeTriImpl<RasterMode::Default, false, false>(t, r, float2{ 0,0 }, f, context); };

    template <typename F>
    inline void RasterizeParallel(const Triangle& t, int2 r, F f, void* context = nullptr) { RasterizeTriImpl<RasterMode::Default, true, false>(t, r, float2{ 0,0 }, f, context); };

    template <typename F>
    inline void RasterizeParallelBarycentrics(const Triangle& t, int2 r, F f, void* context = nullptr) { RasterizeTriImpl<RasterMode::Default, true, true>(t, r, float2{ 0,0 }, f, context); };

    template <typename F>
    inline void RasterizeLine(const Line& l, int2 r, F f, void* context = nullptr) { RasterizeLineImpl<false, false>(l, r, float2{ 0,0 }, f, context); };

    template <typename F>
    inline void RasterizeConservativeLine(const Line& l, int2 r, F f, void* context = nullptr) { RasterizeLineConservativeImpl(l, r, float2{ 0,0 }, f, context); };

    template <typename F>
    inline void RasterizeConservativeLineWithOffset(const Line& l, int2 r, float2 offset, F f, void* context = nullptr) { RasterizeLineConservativeImpl(l, r, offset, f, context); };

} // namespace omm