// Gaijin: the optimized loop of omm_rasterize_cs: the queue of cells to process, the opacity table and the loop itself.
// Include right before main of omm_rasterize_cs.hlsli: it uses StatelessRasterizer from there, and PRECISE,
// g_GlobalConstants and raster:: functions from omm_resample_common.hlsli.
#ifndef DAG_OMM_COMMON_HLSLI
#define DAG_OMM_COMMON_HLSLI

namespace dagor
{
    // helper structure, which fits bit mask for 8x8 grid
    struct Bitmask8x8
    {
        uint2 bits;

        static int2 getSize() { return int2(8, 8); }

        void clear() { bits = uint2(0, 0); }

        // add 2x2 block to bits, offset.y should be even
        void addBlockAligned(uint row0, uint row1, int2 offset)
        {
            const uint block = (row0 | (row1 << 8)) << ((offset.y & 3) * 8 + offset.x);
            bits.x |= offset.y < 4 ? block : 0u;
            bits.y |= offset.y < 4 ? 0u : block;
        }
    };

    // data structure, which helps to reduce divergency in pixel processing
    struct ProcessingQueue
    {
        Bitmask8x8 mask;

        void init() { mask.clear(); }

        bool extractNextPixelOffset(out int2 offset)
        {
            uint slot = (mask.bits.x == 0 ? 1 : 0);
            uint curr_mask = mask.bits.y*slot + mask.bits.x*(1u-slot);
            uint index = firstbitlow(curr_mask);

            mask.bits.x &= 0xFFFFFFFEu << index;
            mask.bits.y ^= slot << index;

            offset.x = index & 7u;
            offset.y = (index >> 3u) + (slot << 2);

            return index != 0xFFFFFFFFu;
        }

        void addBlockAligned(uint squares_mask, int2 offset)
        {
            mask.addBlockAligned(squares_mask & 3u, squares_mask >> 2, offset);
        }

        // Keeps only the cells with x < size.x and y < size.y, size <= 7
        void clipToAabb(int2 size)
        {
            const uint rows4 = ((1u << size.x) - 1u) * 0x01010101u;
            const uint h = (uint)size.y;
            mask.bits &= uint2(h >= 4 ? rows4 : rows4 & ((1u << (h * 8)) - 1u),
                h > 4 ? rows4 & ((1u << ((h - 4) * 8)) - 1u) : 0u);
        }

        static bool hasEnoughSpace(int2 box_size)
        {
            const bool condition = all(box_size <= Bitmask8x8::getSize() - 1);
#if WAVE_INTRINSICS
            return WaveActiveAllTrue(condition);
#else
            return condition;
#endif
        }
    };

    // filled with opacity data for microtriangle aabb
    struct OpacityMask
    {
        Bitmask8x8 mask;

        void init() { mask.clear(); }

        // use one Gather for the 2x2 texel block at min_edge + (x, y), y even
        void addBlockAligned(int2 min_edge, int2 offset)
        {
            // Same uv as raster::GetStateAtPixelCenter uses for the cell of texel min_edge + (x, y)
            const PRECISE float2 uv = (float2(min_edge + offset) + 1.0f) * g_GlobalConstants.InvTexSize;
            // Gather order: (x, y + 1), (x + 1, y + 1), (x + 1, y), (x, y)
            const bool4 isOpaque = g_GlobalConstants.AlphaCutoff < raster::GatherAlphaTex(uv);
            const uint row0 = (isOpaque.w ? 1u : 0u) | (isOpaque.z ? 2u : 0u);
            const uint row1 = (isOpaque.x ? 1u : 0u) | (isOpaque.y ? 2u : 0u);
            mask.addBlockAligned(row0, row1, offset);
        }

        // Bit (x, y) of the result is the AND of bits (x, y), (x + 1, y), (x, y + 1), (x + 1, y + 1) of b.
        // For x == 7 it reads the next row, but the aabb has no cell there.
        static uint2 andCellTexels(uint2 b)
        {
            const uint2 horz = b & uint2((b.x >> 1) | (b.y << 31), b.y >> 1);
            return horz & uint2((horz.x >> 8) | (horz.y << 24), horz.y >> 8);
        }

        // Cells (bit y * 8 + x, the Bitmask8x8 layout) with all 4 texels opaque, and with all 4 transparent
        void getUniformCells(out uint2 all_opaque, out uint2 all_transparent)
        {
            all_opaque = andCellTexels(mask.bits);
            all_transparent = andCellTexels(~mask.bits);
        }
    };

    // Gaijin: the optimized processing of the cells of a micro-triangle aabb that fits ProcessingQueue, for the
    // Linear filter with level lines. Returns false with no change when the aabb does not fit or in another mode:
    // the caller then runs the generic loop. With is_opaque_enough an opaque state is final, thus the drain stops when
    // it is found.
    bool tryOpacityStateFastPath(int2 min_edge, int2 max_edge, StatelessRasterizer tix, PRECISE float2 v[3], uint primitiveIndex,
        inout bool is_opaque, inout bool is_transparent, in bool is_opaque_enough)
    {
        const int2 aabbSize = max_edge - min_edge;
        if (!ProcessingQueue::hasEnoughSpace(aabbSize))
            return false;

        if ((TextureFilterMode)g_GlobalConstants.FilterType != TextureFilterMode::Linear || !g_GlobalConstants.EnableLevelLine)
            return false;

        ProcessingQueue queue;
        queue.init();
        OpacityMask opacity;
        opacity.init();

        // phase 1: fill processing queue and opacity mask using 2x2 blocks
        // it's possible to fill unused data, but net effect is positive
        {
            const int blockCountX = (aabbSize.x + 2) >> 1;
            const int blockCount = all(aabbSize > 0) ? blockCountX * ((aabbSize.y + 2) >> 1) : 0;

            int2 offset = int2(0, 0);
            for (int block = 0; block < blockCount; ++block)
            {
                const uint squaresMask = tix.SquaresInTriangleSkipAABBTest2x2(float2(min_edge + offset));

                queue.addBlockAligned(squaresMask, offset);
                opacity.addBlockAligned(min_edge, offset);

                const bool rowEnd = offset.x + 2 > aabbSize.x;
                offset.y += rowEnd ? 2 : 0;
                offset.x = rowEnd ? 0 : offset.x + 2;
            }
            queue.clipToAabb(aabbSize);
        }

        // phase 2: apply opacity mask to extract data for trivial cases (all 4 texels opaque or transparent)
        // cells with all 4 texels on one side of the cutoff take that state and leave the queue
        {
            uint2 allOpaque, allTransparent;
            opacity.getUniformCells(allOpaque, allTransparent);
            allOpaque &= queue.mask.bits;
            allTransparent &= queue.mask.bits;

            is_opaque = is_opaque || any(allOpaque != 0);
            is_transparent = is_transparent || any(allTransparent != 0);
            queue.mask.bits &= ~(allOpaque | allTransparent);
        }

        // phase 3: all remaining slow pixels are combined together and will be processed with low divergency
        // early-out is possible if microtriangle can be both opaque and transparent
        int2 pixelOffset;
        while (!(is_opaque && (is_transparent || is_opaque_enough)) && queue.extractNextPixelOffset(pixelOffset))
        {
            int2 pixel = min_edge + pixelOffset;

            const PRECISE float2 texel = float2(pixel) + 0.5f;
            const PRECISE float2 texCoord = (float2(pixel) + 1.0f) * g_GlobalConstants.InvTexSize;
            raster::GetStateAtPixelCenter(texCoord, texel, v[0], v[1], v[2], primitiveIndex, is_opaque, is_transparent);
        }

        return true;
    }
}

#endif // DAG_OMM_COMMON_HLSLI
