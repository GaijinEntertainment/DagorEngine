//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_bounds2.h>
#include <generic/dag_tab.h>

// Compact spatial grid for PhysMap decals.
// One arena of shared streams instead of per-cell mesh copies: strip-coded
// topology, bit-packed palette UV indices, per-chunk u16 quantized verts,
// and per-cell triangle runs. Quantization moves decal boundaries by <= half
// a vert quantum: ~0.5 mm for typical 64 m-packed chunks, ~1 mm at the 128 m
// max; a single wider tri gets its own chunk that keeps exact f32 verts, so
// precision never depends on tri size.
struct PhysMapCompactDecals
{
  static constexpr int MAX_CHUNK_VERTS = 255;      // u8 local indices, 255 reserved
  static constexpr int MAX_CHUNK_TRIS = 4096;      // u16 run offsets
  static constexpr float CHUNK_PACK_EXTENT = 64.f; // m; packing target, keeps the typical vert quantum ~0.5mm
  // m; widest chunk that quantizes (bounds the error to ~1mm); a single wider
  // tri opens its own chunk, which stores raw f32 verts instead (vertsF32)
  static constexpr float MAX_CHUNK_EXTENT = 128.f;

  struct Chunk
  {
    uint16_t qbox[4]; // minx,miny,maxx,maxy relative to the padded basis, rounded outward; cull box and vert dequant basis
    uint8_t matId;
    uint8_t bitmapTexId; // 0xff = none
    uint8_t vertCountM1;
    uint8_t uvBits; // u palette index bits + vBits of global v dictionary index
    uint16_t triCount;
    uint16_t vertsF32; // 1: chunk is wider than MAX_CHUNK_EXTENT, vertStart indexes fverts (raw f32)
    uint32_t vertStart;
    uint32_t topoStart;  // byte offset into topo: 6-bit tri codes section, then escape bytes
    uint32_t uvBitStart; // bit offset into uvStream
    uint32_t uPaletteStart;
  };
  struct CellEntry
  {
    uint32_t chunkId;
    uint32_t runStart;
    uint32_t runCount;
  };
  struct TriRun
  {
    uint16_t first; // chunk-local tri id
    uint16_t count;
  };

  Tab<uint16_t> qverts;  // 2 per vert, normalized to chunk qbox
  Tab<Point2> fverts;    // raw verts of the rare vertsF32 chunks
  Tab<uint8_t> uvStream; // bit-packed per vert
  Tab<float> uPalettes;  // per-chunk exact u floats
  Tab<float> vDict;      // global exact v floats, sorted ascending
  Tab<uint8_t> topo;
  Tab<Chunk> chunks;
  Tab<uint32_t> cellStart; // gridSz*gridSz + 1 offsets into cellEntries
  Tab<CellEntry> cellEntries;
  Tab<TriRun> runs;

  int gridSz = 0;
  int vBits = 0; // bits of the global v dict index inside each uv field
  float gridScale = 1.f, invGridScale = 1.f;
  Point2 worldOffset = Point2(0.f, 0.f); // padded qbox basis, not the map origin
  Point2 mapQStep = Point2(0.f, 0.f);    // qbox dequant step

  size_t memBytes() const
  {
    return data_size(qverts) + data_size(fverts) + data_size(uvStream) + data_size(uPalettes) + data_size(vDict) + data_size(topo) +
           data_size(chunks) + data_size(cellStart) + data_size(cellEntries) + data_size(runs);
  }

  Point2 chunkDequantOfs(const Chunk &c) const
  {
    return Point2(worldOffset.x + c.qbox[0] * mapQStep.x, worldOffset.y + c.qbox[1] * mapQStep.y);
  }
  Point2 chunkDequantStep(const Chunk &c) const
  {
    return Point2((c.qbox[2] - c.qbox[0]) * mapQStep.x / 65535.f, (c.qbox[3] - c.qbox[1]) * mapQStep.y / 65535.f);
  }
  BBox2 chunkBox(const Chunk &c) const
  {
    Point2 lt(worldOffset.x + c.qbox[0] * mapQStep.x, worldOffset.y + c.qbox[1] * mapQStep.y);
    Point2 rb(worldOffset.x + c.qbox[2] * mapQStep.x, worldOffset.y + c.qbox[3] * mapQStep.y);
    return BBox2(lt, rb);
  }

  // Resumable per-chunk decode scratch for rendering: topology, world verts,
  // uv. ~16KB; allocate per query (framemem), valid for one chunk at a time.
  struct DecodeCtx
  {
    // scratch filled by decodeChunk; only [0, decoded*) is ever read, so leaving it uninitialized is intended
    uint8_t topo[MAX_CHUNK_TRIS * 3 + 1]; //-V730_NOINIT +1: SIMD decode stores 4 bytes per tri
    Point2 verts[MAX_CHUNK_VERTS + 1];    //-V730_NOINIT +1: SIMD decode stores 2 verts at a time
    Point2 uv[MAX_CHUNK_VERTS + 1];       //-V730_NOINIT
    uint32_t chunkId = ~0u;
    int decodedTris = 0;
    int decodedVerts = 0;
    int decodedUVVerts = 0; // may lag decodedVerts when resumes vary need_uv
    int maxVertSeen = -1;
    uint32_t payloadOfs = 0;
    uint8_t prev[3] = {0xff, 0xff, 0xff};
  };
  // Ensures ctx holds decoded tris [0, lastTri), the verts they use, and,
  // with need_uv, their uv. need_uv=false skips uv decode (chunks without a
  // live bitmap mask).
  void decodeChunk(DecodeCtx &ctx, uint32_t chunk_id, int last_tri, bool need_uv = true) const;

  // Decoded triangle visitor for debug/tools: cb(Point2 v0, Point2 v1, Point2 v2)
  template <typename Cb>
  void forEachCellTri(int cell_id, Cb cb) const;
};

// Builds the compact structure from the source decal meshes; cell assignment
// bins each tri bbox to grid cells, preserving source draw order per cell.
// Returns false (after logerr) for source shapes the layout cannot encode;
// the caller keeps the source decals un-gridded.
struct PhysMap;
bool build_compact_decals(const PhysMap &pm, int grid_sz, PhysMapCompactDecals &out);

template <typename Cb>
inline void PhysMapCompactDecals::forEachCellTri(int cell_id, Cb cb) const
{
  DecodeCtx *ctx = new DecodeCtx; // debug path, not perf sensitive
  for (uint32_t ei = cellStart[cell_id], eie = cellStart[cell_id + 1]; ei < eie; ++ei)
  {
    const CellEntry &e = cellEntries[ei];
    int lastTri = 0;
    for (uint32_t ri = e.runStart, rie = e.runStart + e.runCount; ri < rie; ++ri)
      lastTri = lastTri > runs[ri].first + runs[ri].count ? lastTri : runs[ri].first + runs[ri].count;
    decodeChunk(*ctx, e.chunkId, lastTri, false);
    for (uint32_t ri = e.runStart, rie = e.runStart + e.runCount; ri < rie; ++ri)
      for (int t = runs[ri].first, te = runs[ri].first + runs[ri].count; t < te; ++t)
        cb(ctx->verts[ctx->topo[t * 3 + 0]], ctx->verts[ctx->topo[t * 3 + 1]], ctx->verts[ctx->topo[t * 3 + 2]]);
  }
  delete ctx;
}
