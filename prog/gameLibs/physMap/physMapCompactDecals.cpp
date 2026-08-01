// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <physMap/physMapCompactDecals.h>
#include <physMap/physMap.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_mathBase.h>
#include <math/dag_check_nan.h>
#include <util/dag_stlqsort.h>
#include <vecmath/dag_vecMath.h>
#include <debug/dag_log.h>
#include <string.h>

void PhysMapCompactDecals::decodeChunk(DecodeCtx &ctx, uint32_t chunk_id, int last_tri, bool need_uv) const
{
  const Chunk &chunk = chunks[chunk_id];
  if (ctx.chunkId != chunk_id)
  {
    ctx.chunkId = chunk_id;
    ctx.decodedTris = 0;
    ctx.decodedVerts = 0;
    ctx.decodedUVVerts = 0;
    ctx.maxVertSeen = -1;
    ctx.payloadOfs = (uint32_t(chunk.triCount) * 6 + 7) / 8;
    ctx.prev[0] = ctx.prev[1] = ctx.prev[2] = 0xff;
  }

  if (ctx.decodedTris < last_tri)
  {
    const uint8_t *codes = topo.data() + chunk.topoStart;
    const uint8_t *payload = codes + ctx.payloadOfs;
    int maxV = ctx.maxVertSeen;
#if _TARGET_SIMD_SSE >= 4
    // one pshufb per tri: src = [prev0..2, _, pay0..3, ...], pattern by 6-bit code
    static const struct StripLut
    {
      alignas(16) uint8_t pat[64][16];
      uint8_t esc[64];
      StripLut()
      {
        for (int c = 0; c < 64; ++c)
        {
          int escs = 0;
          for (int a = 0; a < 3; ++a)
          {
            int slot = (c >> (a * 2)) & 3;
            pat[c][a] = slot ? slot - 1 : 4 + escs++;
          }
          for (int a = 3; a < 16; ++a)
            pat[c][a] = 0x80;
          esc[c] = escs;
        }
      }
    } lut; // magic static: thread-safe init
    const __m128i mask3 = _mm_set_epi32(0, 0, 0, 0x00ffffff);
    __m128i prevV = _mm_and_si128(_mm_set1_epi8(char(ctx.prev[0])), mask3);
    prevV = _mm_insert_epi8(prevV, ctx.prev[1], 1);
    prevV = _mm_insert_epi8(prevV, ctx.prev[2], 2);
    __m128i vmax = _mm_setzero_si128();
    for (int t = ctx.decodedTris; t < last_tri; ++t)
    {
      uint32_t bit = uint32_t(t) * 6;
      uint32_t triCode = ((codes[bit >> 3] | (uint32_t(codes[(bit >> 3) + 1]) << 8)) >> (bit & 7)) & 63;
      int pay4;
      memcpy(&pay4, payload, 4);
      __m128i src = _mm_or_si128(_mm_and_si128(prevV, mask3), _mm_slli_si128(_mm_cvtsi32_si128(pay4), 4));
      __m128i dst = _mm_shuffle_epi8(src, _mm_load_si128((const __m128i *)lut.pat[triCode]));
      int triple = _mm_cvtsi128_si32(dst);
      memcpy(ctx.topo + t * 3, &triple, 4); // ctx.topo has +1 pad
      vmax = _mm_max_epu8(vmax, _mm_and_si128(dst, mask3));
      prevV = dst;
      payload += lut.esc[triCode];
    }
    int m4 = _mm_cvtsi128_si32(vmax);
    maxV = max(maxV, max(int(m4 & 0xff), max(int((m4 >> 8) & 0xff), int((m4 >> 16) & 0xff))));
    ctx.prev[0] = ctx.topo[(last_tri - 1) * 3 + 0];
    ctx.prev[1] = ctx.topo[(last_tri - 1) * 3 + 1];
    ctx.prev[2] = ctx.topo[(last_tri - 1) * 3 + 2];
#else
    for (int t = ctx.decodedTris; t < last_tri; ++t)
    {
      uint32_t bit = uint32_t(t) * 6;
      uint32_t triCode = (codes[bit >> 3] | (uint32_t(codes[(bit >> 3) + 1]) << 8)) >> (bit & 7);
      uint8_t *dst = ctx.topo + t * 3;
      for (int a = 0; a < 3; ++a)
      {
        uint32_t code = (triCode >> (a * 2)) & 3;
        dst[a] = code ? ctx.prev[code - 1] : *payload++;
        maxV = max(maxV, int(dst[a]));
      }
      ctx.prev[0] = dst[0];
      ctx.prev[1] = dst[1];
      ctx.prev[2] = dst[2];
    }
#endif
    ctx.decodedTris = last_tri;
    ctx.payloadOfs = uint32_t(payload - codes);
    ctx.maxVertSeen = maxV;
  }

  if (ctx.decodedVerts <= ctx.maxVertSeen)
  {
    const int vFrom = ctx.decodedVerts, vEnd = ctx.maxVertSeen + 1;
    if (chunk.vertsF32) // oversized chunk: verts are stored raw, nothing to dequantize
      memcpy(&ctx.verts[vFrom], fverts.data() + chunk.vertStart + vFrom, size_t(vEnd - vFrom) * sizeof(Point2));
    else
    {
      // 2 verts per iteration; scratch has +1 vert slack and the qverts
      // stream is padded, so the overrun of the last odd store/load is safe
      const uint16_t *qv = qverts.data() + chunk.vertStart * 2;
      Point2 dqOfs = chunkDequantOfs(chunk);
      Point2 dqStep = chunkDequantStep(chunk);
      vec4f vStep = v_make_vec4f(dqStep.x, dqStep.y, dqStep.x, dqStep.y);
      vec4f vOfs = v_make_vec4f(dqOfs.x, dqOfs.y, dqOfs.x, dqOfs.y);
      for (int v = vFrom; v < vEnd; v += 2)
        v_stu(&ctx.verts[v].x, v_madd(v_cvt_vec4f(v_lduush(qv + v * 2)), vStep, vOfs));
    }
    ctx.decodedVerts = vEnd;
  }

  // uv progress is tracked separately from verts: need_uv may vary between
  // resumes of one chunk, so the uv fill can lag behind the vert fill
  if (need_uv && ctx.decodedUVVerts <= ctx.maxVertSeen)
  {
    const int vFrom = ctx.decodedUVVerts, vEnd = ctx.maxVertSeen + 1;
    const uint8_t *uvStreamP = uvStream.data();
    const float *uPal = uPalettes.data() + chunk.uPaletteStart;
    const float *vD = vDict.data();
    const int uBits = chunk.uvBits - vBits;
    const uint32_t uMask = (1u << uBits) - 1, vMask = (1u << vBits) - 1;
    // two consecutive fields per u64 read (uvBits <= 28, so 2*uvBits + 7 <= 64)
    int v = vFrom;
    for (; v + 1 < vEnd; v += 2)
    {
      uint64_t bit = uint64_t(chunk.uvBitStart) + uint32_t(v) * chunk.uvBits;
      uint64_t w;
      memcpy(&w, uvStreamP + (bit >> 3), 8);
      w >>= (bit & 7);
      uint32_t w0 = uint32_t(w), w1 = uint32_t(w >> chunk.uvBits);
      ctx.uv[v] = Point2(uPal[w0 & uMask], vD[(w0 >> uBits) & vMask]);
      ctx.uv[v + 1] = Point2(uPal[w1 & uMask], vD[(w1 >> uBits) & vMask]);
    }
    for (; v < vEnd; ++v)
    {
      uint64_t bit = uint64_t(chunk.uvBitStart) + uint32_t(v) * chunk.uvBits;
      uint64_t w;
      memcpy(&w, uvStreamP + (bit >> 3), 8);
      w >>= (bit & 7);
      uint32_t w0 = uint32_t(w);
      ctx.uv[v] = Point2(uPal[w0 & uMask], vD[(w0 >> uBits) & vMask]);
    }
    ctx.decodedUVVerts = vEnd;
  }
}

bool build_compact_decals(const PhysMap &pm, int grid_sz, PhysMapCompactDecals &out)
{
  // reject source shapes the compact layout cannot encode BEFORE touching out;
  // the caller keeps the source decals un-gridded on false
  for (const PhysMap::DecalMesh &mesh : pm.decals)
  {
    // NaN never compares equal, so the uv palette encode search would run past
    // its end; int(NaN/inf) in the cell-coverage and u16 quantize steps is UB
    for (const Point2 &tc : mesh.texCoords)
      if (!check_finite(tc.x) || !check_finite(tc.y))
      {
        logerr("physmap: non-finite decal texcoord, decals stay un-gridded");
        return false;
      }
    for (const Point2 &v : mesh.vertices)
      if (!check_finite(v.x) || !check_finite(v.y))
      {
        logerr("physmap: non-finite decal vertex, decals stay un-gridded");
        return false;
      }
    const bool sameAsIndices = (mesh.flags & PhysMap::DecalMesh::TINDICES_SAME_AS_INDICES) != 0;
    for (const PhysMap::DecalMesh::MaterialIndices &mi : mesh.matIndices)
    {
      // separate tindices (a uv seam) are encoded as (pos, uv) wedges below;
      // they must be one tindex per triangle corner, parallel to indices
      if (!sameAsIndices && !mi.tindices.empty())
      {
        if (mi.tindices.size() != mi.indices.size())
        {
          logerr("physmap: tindices count %d != indices %d, decals stay un-gridded", (int)mi.tindices.size(), (int)mi.indices.size());
          return false;
        }
      }
      // otherwise uv is indexed by the position index, so the arrays must
      // match; empty texCoords are fine (maskless v0 decals get zero uv,
      // matching the renderer's empty-tindices fallback)
      else if (!mesh.texCoords.empty() && mesh.texCoords.size() != mesh.vertices.size())
      {
        logerr("physmap: non-parallel texCoords (%d for %d verts), decals stay un-gridded", (int)mesh.texCoords.size(),
          (int)mesh.vertices.size());
        return false;
      }
      // the streams come from the level file: bound every index before the
      // builder dereferences vertex/texcoord arrays with it
      for (uint16_t idx : mi.indices)
        if (idx >= mesh.vertices.size())
        {
          logerr("physmap: decal index %d out of %d verts, decals stay un-gridded", idx, (int)mesh.vertices.size());
          return false;
        }
      if (!sameAsIndices)
        for (uint16_t t : mi.tindices)
          if (t >= mesh.texCoords.size())
          {
            logerr("physmap: decal tindex %d out of %d texcoords, decals stay un-gridded", t, (int)mesh.texCoords.size());
            return false;
          }
      // chunks store material ids as u8 with 0xff = invalid (the legacy
      // renderer produced the same 0xff via int->u8 truncation of -1)
      if (mi.matId >= 255)
      {
        logerr("physmap: matId %d needs more than 8 bits, decals stay un-gridded", mi.matId);
        return false;
      }
      // chunks store mask ids as u8 with 0xff = none; an id the renderers
      // treat as live (see physMatSwRenderer) must survive that narrowing
      if (mi.bitmapTexId >= 255 && mi.bitmapTexId < pm.physTextures.size())
      {
        logerr("physmap: bitmapTexId %d needs more than 8 bits, decals stay un-gridded", mi.bitmapTexId);
        return false;
      }
    }
    // the legacy grid merged same-key blocks per cell with a cell-dependent
    // order no single stream can reproduce; real exporters emit one block
    // per key, so reject the ambiguous shape instead of changing its output
    for (int a = 1; a < mesh.matIndices.size(); ++a)
      for (int b = 0; b < a; ++b)
        if (mesh.matIndices[a].matId == mesh.matIndices[b].matId && mesh.matIndices[a].bitmapTexId == mesh.matIndices[b].bitmapTexId)
        {
          logerr("physmap: duplicate (matId %d, tex %d) blocks in one mesh, decals stay un-gridded", mesh.matIndices[a].matId,
            mesh.matIndices[a].bitmapTexId);
          return false;
        }
  }

  out = PhysMapCompactDecals(); // the builder appends to the streams; start from a clean object
  out.gridSz = grid_sz;
  out.gridScale = float(pm.size) / grid_sz * pm.scale;
  out.invGridScale = safeinv(out.gridScale);
  // the qbox basis is padded beyond the map so quantized chunks (extent <=
  // MAX_CHUNK_EXTENT) overhanging the edge (cell-coverage truncation admits up
  // to one grid cell, plus that extent) dequantize to their true positions;
  // wider chunks keep f32 verts and use the (clamped) qbox for culling only
  const float qPad = out.gridScale + PhysMapCompactDecals::MAX_CHUNK_EXTENT;
  out.worldOffset = pm.worldOffset - Point2(qPad, qPad);
  out.mapQStep = Point2((pm.size * pm.scale + 2.f * qPad) / 65535.f, (pm.size * pm.scale + 2.f * qPad) / 65535.f);

  const int numCells = grid_sz * grid_sz;
  struct TmpEntry
  {
    uint32_t chunk;
    Tab<PhysMapCompactDecals::TriRun> runs;
  };
  Tab<Tab<TmpEntry>> tmpCells;
  tmpCells.resize(numCells);
  Tab<int> cellLastChunk;
  cellLastChunk.resize(numCells);
  mem_set_ff(cellLastChunk);

  Tab<int> vertRemap;   // source vert -> chunk-local, ff = not present
  Tab<Point2> verts;    // exact f32 verts per compact vert; quantized at the end
  Tab<Point2> uvFloat;  // exact source uv per compact vert
  Tab<BBox2> chunkFBox; // exact float box per chunk (qbox derived at the end)
  // per-material wedge expansion for separate tindices (uv seams): a position
  // shared with two uvs becomes two verts; deduped by (pos, tindex) via a
  // per-position chain (wNextForPos) so shared wedges are not duplicated
  Tab<Point2> wVert, wUV;
  Tab<uint16_t> wIdx, wTindex;
  Tab<int> posWedgeHead, wNextForPos;

  for (const PhysMap::DecalMesh &mesh : pm.decals)
  {
    for (const PhysMap::DecalMesh::MaterialIndices &mi : mesh.matIndices)
    {
      // separate tindices: pre-split into (pos, uv) wedges so each compact vert
      // carries one uv, mirroring the renderer's tindices path; the common case
      // (uv indexed by position) feeds the source arrays unchanged
      const Tab<Point2> *srcVert = &mesh.vertices;
      const Tab<Point2> *srcUV = &mesh.texCoords;
      const Tab<uint16_t> *srcIdx = &mi.indices;
      if (!(mesh.flags & PhysMap::DecalMesh::TINDICES_SAME_AS_INDICES) && !mi.tindices.empty())
      {
        wVert.clear();
        wUV.clear();
        wTindex.clear();
        wNextForPos.clear();
        wIdx.resize(mi.indices.size());
        posWedgeHead.resize(mesh.vertices.size());
        mem_set_ff(posWedgeHead);
        bool overflow = false;
        for (int k = 0; k < mi.indices.size(); ++k)
        {
          const uint16_t p = mi.indices[k], t = mi.tindices[k];
          int w = posWedgeHead[p];
          while (w >= 0 && wTindex[w] != t)
            w = wNextForPos[w];
          if (w < 0)
          {
            if (wVert.size() >= 0xffff) // wIdx is u16; a mesh this dense is pathological
            {
              overflow = true;
              break;
            }
            w = wVert.size();
            wVert.push_back(mesh.vertices[p]);
            wUV.push_back(mesh.texCoords[t]);
            wTindex.push_back(t);
            wNextForPos.push_back(posWedgeHead[p]);
            posWedgeHead[p] = w;
          }
          wIdx[k] = uint16_t(w);
        }
        if (overflow)
        {
          logerr("physmap: decal mesh exceeds 65535 uv wedges, decals stay un-gridded");
          return false;
        }
        srcVert = &wVert;
        srcUV = &wUV;
        srcIdx = &wIdx;
      }

      vertRemap.resize(srcVert->size());
      mem_set_ff(vertRemap);
      int chunkId = -1, curVerts = 0;
      PhysMapCompactDecals::Chunk *chunk = nullptr;

      for (int i = 0; i < srcIdx->size(); i += 3)
      {
        uint16_t si[3] = {(*srcIdx)[i + 0], (*srcIdx)[i + 1], (*srcIdx)[i + 2]};

        // cell coverage first: tris covering no cell are never rasterized
        // (region queries only visit in-grid cells), drop them
        Point2 v0 = (*srcVert)[si[0]], v1 = (*srcVert)[si[1]], v2 = (*srcVert)[si[2]];
        Point2 leftTop = min(v0, min(v1, v2));
        Point2 rightBottom = max(v0, max(v1, v2));
        // membership is decided on exact verts but rendering uses dequantized
        // ones, which move by up to a chunk quantum: inflate coverage so a
        // rounded vert cannot cross into a cell that has no entry for its tri
        const float qErr = (PhysMapCompactDecals::MAX_CHUNK_EXTENT + 2.f * out.mapQStep.x) / 65535.f; // max QUANTIZED qbox extent
        IPoint2 leftTopCell = IPoint2::xy((leftTop - Point2(qErr, qErr) - pm.worldOffset) * out.invGridScale);
        IPoint2 rightBottomCell = IPoint2::xy((rightBottom + Point2(qErr, qErr) - pm.worldOffset) * out.invGridScale);
        int cy0 = max(leftTopCell.y, 0), cy1 = min(rightBottomCell.y, grid_sz - 1);
        int cx0 = max(leftTopCell.x, 0), cx1 = min(rightBottomCell.x, grid_sz - 1);
        if (cy0 > cy1 || cx0 > cx1)
          continue;

        int newVerts = 0;
        for (int a = 0; a < 3; ++a)
        {
          bool isNew = vertRemap[si[a]] < 0;
          for (int b = 0; b < a && isNew; ++b)
            isNew = si[b] != si[a];
          newVerts += isNew ? 1 : 0;
        }
        bool extentOk = true;
        if (chunk)
        {
          BBox2 grown = chunkFBox.back();
          grown += v0;
          grown += v1;
          grown += v2;
          extentOk =
            grown.width().x <= PhysMapCompactDecals::CHUNK_PACK_EXTENT && grown.width().y <= PhysMapCompactDecals::CHUNK_PACK_EXTENT;
        }
        if (!chunk || !extentOk || curVerts + newVerts > PhysMapCompactDecals::MAX_CHUNK_VERTS ||
            chunk->triCount + 1 > PhysMapCompactDecals::MAX_CHUNK_TRIS)
        {
          // open new chunk; verts already placed stay with the old chunk,
          // shared verts are duplicated into the new one
          if (chunk)
            mem_set_ff(vertRemap);
          chunkId = out.chunks.size();
          chunk = &out.chunks.push_back();
          memset(chunk, 0, sizeof(*chunk)); //-V780
          // loader yields PHYSMAT_INVALID (-1) for unknown material names;
          // store it as 0xff, the same byte the legacy render path produced
          chunk->matId = mi.matId < 0 ? 0xff : uint8_t(mi.matId);
          // ids out of the physTextures range decode as none (0xff), exactly
          // like the renderers already treat them; live ids fit u8 (preamble)
          chunk->bitmapTexId = (mi.bitmapTexId >= 0 && mi.bitmapTexId < pm.physTextures.size()) ? uint8_t(mi.bitmapTexId) : 0xff;
          chunk->vertStart = verts.size();
          chunk->triCount = 0;
          curVerts = 0;
          BBox2 &fb = chunkFBox.push_back();
          fb.setempty();
        }
        BBox2 &fbox = chunkFBox.back();
        for (int a = 0; a < 3; ++a)
        {
          if (vertRemap[si[a]] < 0)
          {
            vertRemap[si[a]] = curVerts++;
            verts.push_back((*srcVert)[si[a]]);
            // empty texCoords (maskless v0 decals): zero uv, matching the
            // renderer's empty-tindices fallback
            uvFloat.push_back(srcUV->empty() ? Point2(0.f, 0.f) : (*srcUV)[si[a]]);
            fbox += (*srcVert)[si[a]];
          }
        }
        chunk->vertCountM1 = uint8_t(curVerts - 1);
        // temp topology as plain triplets in topo; strip-encoded in a post-pass
        out.topo.push_back(uint8_t(vertRemap[si[0]]));
        out.topo.push_back(uint8_t(vertRemap[si[1]]));
        out.topo.push_back(uint8_t(vertRemap[si[2]]));
        int localTri = chunk->triCount++;
        if (chunk->triCount == 1)
          chunk->topoStart = (out.topo.size() - 3) / 3; // temp: tri index; rewritten by strip pass

        for (int y = cy0; y <= cy1; ++y)
          for (int x = cx0; x <= cx1; ++x)
          {
            int cellId = y * grid_sz + x;
            TmpEntry *e;
            if (cellLastChunk[cellId] != chunkId)
            {
              cellLastChunk[cellId] = chunkId;
              e = &tmpCells[cellId].push_back();
              e->chunk = chunkId;
            }
            else
              e = &tmpCells[cellId].back();
            if (!e->runs.empty() && e->runs.back().first + e->runs.back().count == localTri)
              e->runs.back().count++;
            else
            {
              PhysMapCompactDecals::TriRun &r = e->runs.push_back();
              r.first = localTri;
              r.count = 1;
            }
          }
      }
      // chunk ids are global and never reused, cellLastChunk needs no reset
    }
  }

  // qbox from exact chunk boxes (outward rounding keeps culls conservative);
  // the padded basis covers every retained tri, so the clamp never engages;
  // floor/ceil round an already-rounded float division, so nudge each side
  // until the box chunkBox() reconstructs provably contains the float box
  for (int c = 0; c < out.chunks.size(); ++c)
  {
    PhysMapCompactDecals::Chunk &chunk = out.chunks[c];
    const BBox2 &fb = chunkFBox[c];
    Point2 rel0 = (fb.lim[0] - out.worldOffset), rel1 = (fb.lim[1] - out.worldOffset);
    chunk.qbox[0] = uint16_t(clamp(int(floorf(rel0.x / out.mapQStep.x)), 0, 65535));
    chunk.qbox[1] = uint16_t(clamp(int(floorf(rel0.y / out.mapQStep.y)), 0, 65535));
    chunk.qbox[2] = uint16_t(clamp(int(ceilf(rel1.x / out.mapQStep.x)), 0, 65535));
    chunk.qbox[3] = uint16_t(clamp(int(ceilf(rel1.y / out.mapQStep.y)), 0, 65535));
    BBox2 rb = out.chunkBox(chunk);
    while (rb.lim[0].x > fb.lim[0].x && chunk.qbox[0] > 0)
      chunk.qbox[0]--, rb = out.chunkBox(chunk);
    while (rb.lim[0].y > fb.lim[0].y && chunk.qbox[1] > 0)
      chunk.qbox[1]--, rb = out.chunkBox(chunk);
    while (rb.lim[1].x < fb.lim[1].x && chunk.qbox[2] < 65535)
      chunk.qbox[2]++, rb = out.chunkBox(chunk);
    while (rb.lim[1].y < fb.lim[1].y && chunk.qbox[3] < 65535)
      chunk.qbox[3]++, rb = out.chunkBox(chunk);
  }

  // uv encode: global v dict + per-chunk u palettes, bit-packed fixed width
  // per chunk; v values repeat across the level (mask atlas rows), so one
  // sorted dict of every distinct v serves all chunks, as wide as it needs
  // to be, while u is too diverse for that and gets per-chunk palettes
  {
    Tab<float> allV;
    allV.resize(uvFloat.size());
    for (int i = 0; i < uvFloat.size(); ++i)
      allV[i] = uvFloat[i].y;
    stlsort::sort(allV.begin(), allV.end());
    for (float v : allV)
      if (out.vDict.empty() || out.vDict.back() != v)
        out.vDict.push_back(v);
    int vBits = 1;
    while ((1 << vBits) < out.vDict.size())
      vBits++;
    out.vBits = vBits;

    Tab<float> pal;
    for (PhysMapCompactDecals::Chunk &chunk : out.chunks)
    {
      int vertCount = chunk.vertCountM1 + 1;
      pal.clear();
      for (int v = 0; v < vertCount; ++v)
      {
        float u = uvFloat[chunk.vertStart + v].x;
        bool found = false;
        for (float p : pal)
          if (p == u)
          {
            found = true;
            break;
          }
        if (!found)
          pal.push_back(u);
      }
      int uBits = 1;
      while ((1 << uBits) < pal.size())
        uBits++;
      // decoder contract: two consecutive fields per shifted u64 read
      if (uBits + vBits > 28)
      {
        logerr("physmap: uv index needs %d bits > 28, decals stay un-gridded", uBits + vBits);
        return false;
      }
      chunk.uvBits = uint8_t(uBits + vBits);
      chunk.uPaletteStart = out.uPalettes.size();
      append_items(out.uPalettes, pal.size(), pal.data());
    }
    uint64_t bitCursor = 0;
    for (PhysMapCompactDecals::Chunk &chunk : out.chunks)
    {
      chunk.uvBitStart = uint32_t(bitCursor);
      bitCursor += uint64_t(chunk.vertCountM1 + 1) * chunk.uvBits;
    }
    G_ASSERTF(bitCursor < (uint64_t(1) << 32), "uv bitstream too large");
    out.uvStream.resize((bitCursor + 7) / 8 + 8); // +8: decoder reads two fields per u64
    mem_set_0(out.uvStream);
    for (const PhysMapCompactDecals::Chunk &chunk : out.chunks)
    {
      int uBits = chunk.uvBits - vBits;
      uint64_t bit = chunk.uvBitStart;
      for (int v = 0; v <= chunk.vertCountM1; ++v, bit += chunk.uvBits)
      {
        const Point2 &t = uvFloat[chunk.vertStart + v];
        int ui = -1;
        for (int p = 0; ui < 0; ++p)
          if (out.uPalettes[chunk.uPaletteStart + p] == t.x)
            ui = p;
        // vDict is sorted and built from these very values: the search hits
        int lo = 0, hi = out.vDict.size() - 1;
        while (lo < hi)
        {
          int mid = (lo + hi) >> 1;
          if (out.vDict[mid] < t.y)
            lo = mid + 1;
          else
            hi = mid;
        }
        uint32_t packed = uint32_t(ui) | (uint32_t(lo) << uBits);
        uint32_t byteOfs = uint32_t(bit >> 3), shift = uint32_t(bit & 7);
        uint64_t w;
        memcpy(&w, &out.uvStream[byteOfs], 8);
        w |= uint64_t(packed) << shift;
        memcpy(&out.uvStream[byteOfs], &w, 8);
      }
    }
  }

  // strip-encode topology: per tri 6 bits (3 slots x 2), slot 0 = escape byte,
  // 1..3 = previous tri vert; codes section then payload, per chunk
  {
    Tab<uint8_t> packed;
    packed.reserve(out.topo.size() / 2);
    for (PhysMapCompactDecals::Chunk &chunk : out.chunks)
    {
      const uint8_t *src = out.topo.data() + chunk.topoStart * 3;
      uint32_t codeBytes = (uint32_t(chunk.triCount) * 6 + 7) / 8;
      uint32_t codeStart = packed.size();
      packed.resize(codeStart + codeBytes);
      memset(&packed[codeStart], 0, codeBytes);
      uint8_t prev[3] = {0xff, 0xff, 0xff};
      for (int t = 0; t < chunk.triCount; ++t)
      {
        uint32_t triCode = 0;
        uint8_t cur[3] = {src[t * 3 + 0], src[t * 3 + 1], src[t * 3 + 2]};
        for (int a = 0; a < 3; ++a)
        {
          int code = 0;
          for (int k = 0; k < 3; ++k)
            if (cur[a] == prev[k])
            {
              code = k + 1;
              break;
            }
          if (code == 0)
            packed.push_back(cur[a]); // escape payload (codes section already sized)
          triCode |= uint32_t(code) << (a * 2);
        }
        uint32_t bit = uint32_t(t) * 6;
        packed[codeStart + (bit >> 3)] |= uint8_t(triCode << (bit & 7));
        if ((bit & 7) > 2)
          packed[codeStart + (bit >> 3) + 1] |= uint8_t(triCode >> (8 - (bit & 7)));
        prev[0] = cur[0];
        prev[1] = cur[1];
        prev[2] = cur[2];
      }
      chunk.topoStart = codeStart;
    }
    for (int i = 0; i < 4; ++i) // decoder reads payload 4 bytes at a time
      packed.push_back(0);
    out.topo = packed;
  }

  // quantize verts to u16 against each chunk's dequant basis; a chunk wider
  // than MAX_CHUNK_EXTENT (its opening tri was wider) keeps raw f32 verts in
  // fverts instead, so precision never depends on tri size
  out.qverts.resize(verts.size() * 2 + 4); // +4: decoder loads 2 verts at a time
  for (int c = 0; c < out.chunks.size(); ++c)
  {
    PhysMapCompactDecals::Chunk &chunk = out.chunks[c];
    const uint32_t vertCount = chunk.vertCountM1 + 1;
    const Point2 fbw = chunkFBox[c].width();
    if (max(fbw.x, fbw.y) > PhysMapCompactDecals::MAX_CHUNK_EXTENT)
    {
      chunk.vertsF32 = 1;
      const uint32_t src = chunk.vertStart; // uv/topo encode above used it; decode reads it only as the vert stream index
      chunk.vertStart = out.fverts.size();
      append_items(out.fverts, vertCount, verts.data() + src);
      continue;
    }
    Point2 ofs = out.chunkDequantOfs(chunk);
    Point2 step = out.chunkDequantStep(chunk);
    Point2 inv(safeinv(step.x), safeinv(step.y));
    for (uint32_t v = chunk.vertStart, ve = chunk.vertStart + vertCount; v < ve; ++v)
    {
      Point2 q = verts[v] - ofs;
      out.qverts[v * 2 + 0] = uint16_t(clamp(int(q.x * inv.x + 0.5f), 0, 65535));
      out.qverts[v * 2 + 1] = uint16_t(clamp(int(q.y * inv.y + 0.5f), 0, 65535));
    }
  }

  // flatten cells
  out.cellStart.resize(numCells + 1);
  int totalEntries = 0, totalRuns = 0;
  for (int c = 0; c < numCells; ++c)
  {
    out.cellStart[c] = totalEntries;
    totalEntries += tmpCells[c].size();
    for (const TmpEntry &e : tmpCells[c])
      totalRuns += e.runs.size();
  }
  out.cellStart[numCells] = totalEntries;
  out.cellEntries.reserve(totalEntries);
  out.runs.reserve(totalRuns);
  for (int c = 0; c < numCells; ++c)
    for (const TmpEntry &e : tmpCells[c])
    {
      PhysMapCompactDecals::CellEntry &ce = out.cellEntries.push_back();
      ce.chunkId = e.chunk;
      ce.runStart = out.runs.size();
      ce.runCount = e.runs.size();
      append_items(out.runs, e.runs.size(), e.runs.data());
    }

  out.topo.shrink_to_fit();
  out.chunks.shrink_to_fit();
  out.uPalettes.shrink_to_fit();
  out.runs.shrink_to_fit();
  out.cellEntries.shrink_to_fit();
  return true;
}
