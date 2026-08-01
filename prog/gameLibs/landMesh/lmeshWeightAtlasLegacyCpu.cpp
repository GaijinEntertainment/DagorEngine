// Copyright (C) Gaijin Games KFT.  All rights reserved.

// The CPU path: decode each cell's exported weight textures and pack them.
// See lmeshWeightAtlasLegacy.h for why this file exists.

#include <landMesh/lmeshWeightAtlas.h>
#include "lmeshWeightAtlasLegacy.h"
#include <drv/3d/dag_tex3d.h>
#include <3d/ddsxTex.h>
#include <3d/ddsFormat.h>
#include <ioSys/dag_memIo.h>
#include <loadDDSx/uniTexCrd.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h> // saturate
#include <image/dag_dxtCompress.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/unique_ptr.h>

static constexpr int DET_NUM = LandWeightAtlas::DET_NUM;

// what the exported pair of weight textures holds: four channels in the first,
// two of the second's; any channel a cell blends past those is not stored
static constexpr int LEGACY_TEX1_CHANNELS = 4, LEGACY_TEX2_CHANNELS = 2;
G_STATIC_ASSERT(LEGACY_TEX1_CHANNELS + LEGACY_TEX2_CHANNELS < DET_NUM);

// ---- legacy ddsx stream -> raw texel bytes -----------------------------------

// Returns the offset of the top mip, which is last when mips are stored in
// reverse order. UnifiedTexGenLoad picks the decoder the ddsx was packed with,
// which is the only way to read one correctly: the packers disagree on framing
// (an oodle payload is one block, a zstd one is a stream).
static bool unpack_ddsx(dag::ConstSpan<uint8_t> data, ddsx::Header &hdr, Tab<uint8_t> &texels, int &top_mip_ofs)
{
  if (data.size() < sizeof(ddsx::Header))
    return false;
  memcpy(&hdr, data.data(), sizeof(hdr));
  if (!hdr.checkLabel() || hdr.memSz == 0)
    return false;

  // reverse mip order puts the top one last, so it starts a surface short of the
  // end; a corrupt header must not make that offset wrap and read before the buffer
  const unsigned topMipSz = (hdr.flags & ddsx::Header::FLG_REV_MIP_ORDER) && hdr.levels > 1 ? hdr.getSurfaceSz(0) : hdr.memSz;
  if (topMipSz > hdr.memSz)
    return false;
  texels.resize(hdr.memSz);
  top_mip_ofs = hdr.memSz - topMipSz;
  InPlaceMemLoadCB mcrd(data.data() + sizeof(hdr), data.size() - sizeof(hdr));
  UnifiedTexGenLoad crd(mcrd, hdr);
  return crd->tryRead(texels.data(), hdr.memSz) == (int)hdr.memSz;
}

// The derivation the legacy per-cell weight textures were decoded with, so the
// packed weights are complete and consumers never repeat this chain. Which
// channel the export leaves for us to derive is NOT the last one: the argb4/rg8
// pair holds the others, so it is the even channel below the count (0,0,2,2,4,4,6
// for one to seven textures). Everything above the count is unused.
static void land_weight_derive(float w[DET_NUM], int num_tex)
{
  num_tex = clamp(num_tex, 1, (int)DET_NUM);
  const int derived = (num_tex - 1) & ~1;
  for (int i = num_tex; i < DET_NUM; i++)
    w[i] = 0;
  w[derived] = 0;
  float sum = 0;
  for (int i = 0; i < num_tex; i++)
    sum += w[i];
  w[derived] = saturate(1 - sum);
}

// decodes one weight texture into 4 planes (r,g,b,a), each size*size bytes
static bool decode_weight_tex(dag::ConstSpan<uint8_t> ddsx_data, int size, uint8_t *planes[4])
{
  ddsx::Header hdr;
  Tab<uint8_t> texels(framemem_ptr());
  int topMipOfs = 0;
  if (!unpack_ddsx(ddsx_data, hdr, texels, topMipOfs))
    return false;
  if (hdr.w != size || hdr.h != size)
    return false;
  const uint8_t *t = texels.data() + topMipOfs;
  if (hdr.d3dFormat == D3DFMT_DXT1 || hdr.d3dFormat == D3DFMT_DXT5)
  {
    const bool dxt1 = hdr.d3dFormat == D3DFMT_DXT1;
    if (texels.size() - topMipOfs < (size / 4) * (size / 4) * (dxt1 ? 8 : 16))
      return false;
    Tab<uint8_t> rgba(framemem_ptr());
    rgba.resize(size * size * 4);
    decompress_dxt(rgba.data(), size, size, size * 4, (unsigned char *)t, dxt1);
    for (int i = 0; i < size * size; i++) // bgra
    {
      planes[0][i] = rgba[i * 4 + 2];
      planes[1][i] = rgba[i * 4 + 1];
      planes[2][i] = rgba[i * 4 + 0];
      planes[3][i] = dxt1 ? 0 : rgba[i * 4 + 3];
    }
    return true;
  }
  if (hdr.d3dFormat == D3DFMT_A4R4G4B4)
  {
    if (texels.size() - topMipOfs < size * size * 2)
      return false;
    for (int i = 0; i < size * size; i++)
    {
      uint16_t v = t[i * 2] | (t[i * 2 + 1] << 8);
      planes[0][i] = ((v >> 8) & 15) * 17;
      planes[1][i] = ((v >> 4) & 15) * 17;
      planes[2][i] = (v & 15) * 17;
      planes[3][i] = (v >> 12) * 17;
    }
    return true;
  }
  if (hdr.d3dFormat == D3DFMT_A8R8G8B8)
  {
    if (texels.size() - topMipOfs < size * size * 4)
      return false;
    for (int i = 0; i < size * size; i++)
    {
      planes[0][i] = t[i * 4 + 2];
      planes[1][i] = t[i * 4 + 1];
      planes[2][i] = t[i * 4 + 0];
      planes[3][i] = t[i * 4 + 3];
    }
    return true;
  }
  return false; // unsupported source (e.g. ASTC): the cell degrades, caller reports
}

// Packs a cell from the detail texture map as daEditor paints it: argb4 holds
// the first four landclass weights as 4-bit texels and rg8 the next two, the
// last being derived. One entry per page texel, border included. The editor
// plugin hands the texels over in that shape because it cannot link this
// library and reads them one by one out of the map it edits.
void LandWeightAtlas::setCellFromDetailTexels(int index, const uint32_t *argb4, const uint32_t *rg8, const uint8_t *det_tex_ids,
  int num_tex)
{
  if (uint32_t(index) >= uint32_t(cellsX * cellsY) || !pages.size() || !argb4)
    return;
  Tab<uint8_t> planeBuf(framemem_ptr());
  planeBuf.resize(DET_NUM * pageW * pageW);
  const uint8_t *planes[DET_NUM];
  for (int ch = 0; ch < DET_NUM; ch++)
    planes[ch] = planeBuf.data() + ch * pageW * pageW;

  for (int t = 0; t < pageW * pageW; t++)
  {
    const uint32_t u = argb4[t], u2 = rg8 ? rg8[t] : 0;
    static const int nibble[LEGACY_TEX1_CHANNELS] = {8, 4, 0, 12}; // r, g, b, a
    float w[DET_NUM] = {0};
    for (int ch = 0; ch < LEGACY_TEX1_CHANNELS; ch++)
      w[ch] = ((u >> nibble[ch]) & 0xF) / 15.f;
    for (int ch = 0; ch < LEGACY_TEX2_CHANNELS; ch++)
      w[LEGACY_TEX1_CHANNELS + ch] = ((u2 >> (16 - ch * 8)) & 0xFF) / 255.f;
    land_weight_derive(w, num_tex);
    for (int ch = 0; ch < DET_NUM; ch++)
      planeBuf[ch * pageW * pageW + t] = (uint8_t)clamp((int)(w[ch] * 255.f + 0.5f), 0, 255);
  }
  setCellWeights(index, planes, det_tex_ids, num_tex);
}

// ---- conversion of the legacy per-cell weight textures ------------------------

LandWeightAtlasBuilder::LandWeightAtlasBuilder(int cells_x, int cells_y, int tex_size, int elem_size, unsigned tex_cflg) :
  cells(tmpmem), cellsX(cells_x), cellsY(cells_y), texSize(tex_size), elemSize(elem_size), texCflg(tex_cflg)
{
  cells.resize(cells_x * cells_y);
  for (auto &c : cells)
    memset(c.detTexIds, 0xFF, sizeof(c.detTexIds));
}

LandWeightAtlasBuilder::~LandWeightAtlasBuilder() {}

bool LandWeightAtlasBuilder::addCell(int index, const uint8_t *det_tex_ids, dag::ConstSpan<uint8_t> tex1_ddsx,
  dag::ConstSpan<uint8_t> tex2_ddsx)
{
  if (uint32_t(index) >= uint32_t(cells.size()) || texSize < 4)
    return false;
  Cell &c = cells[index];
  memcpy(c.detTexIds, det_tex_ids, sizeof(c.detTexIds));
  int numTex = 0;
  for (int i = 0; i < DET_NUM; i++)
    if (c.detTexIds[i] != 0xFF)
      numTex++;
  if (!numTex)
    return true;

  Tab<uint8_t> planes1(framemem_ptr()), planes2(framemem_ptr());
  planes1.resize(texSize * texSize * 4);
  planes2.resize(texSize * texSize * 4);
  uint8_t *p1[4], *p2[4];
  for (int i = 0; i < 4; i++)
  {
    p1[i] = planes1.data() + i * texSize * texSize;
    p2[i] = planes2.data() + i * texSize * texSize;
  }
  bool has1 = tex1_ddsx.size() > 0;
  bool has2 = tex2_ddsx.size() > 0;
  if ((has1 && !decode_weight_tex(tex1_ddsx, texSize, p1)) || (has2 && !decode_weight_tex(tex2_ddsx, texSize, p2)))
  {
    // undecodable source (e.g. ASTC-packed mobile export): render the cell as its
    // first landclass until the location is re-exported; finish() reports the count
    undecodedCells++;
    return true;
  }

  for (int ch = 0; ch < numTex; ch++) // channels are dense: DET_NUM - numTex stay empty
    c.chan[ch].resize(texSize * texSize);
  for (int t = 0; t < texSize * texSize; t++)
  {
    float w[DET_NUM] = {0};
    for (int ch = 0; ch < LEGACY_TEX1_CHANNELS; ch++)
      w[ch] = has1 ? p1[ch][t] / 255.f : 0.f;
    for (int ch = 0; ch < LEGACY_TEX2_CHANNELS; ch++)
      w[LEGACY_TEX1_CHANNELS + ch] = has2 ? p2[ch][t] / 255.f : 0.f;
    land_weight_derive(w, numTex);
    for (int ch = 0; ch < numTex; ch++)
      c.chan[ch][t] = (uint8_t)clamp((int)(w[ch] * 255.f + 0.5f), 0, 255);
  }
  return true;
}

// derived weight of landclass lc_id at texel (x, y) of cell (cx, cy);
// null when the cell is out of the map or does not blend that landclass
const uint8_t *LandWeightAtlasBuilder::cellTexel(int cx, int cy, int lc_id, int x, int y) const
{
  if (uint32_t(cx) >= uint32_t(cellsX) || uint32_t(cy) >= uint32_t(cellsY))
    return nullptr;
  const Cell &c = cells[cy * cellsX + cx];
  for (int ch = 0; ch < DET_NUM; ch++)
    if (c.detTexIds[ch] == lc_id && c.chan[ch].size())
      return &c.chan[ch][clamp(y, 0, texSize - 1) * texSize + clamp(x, 0, texSize - 1)];
  return nullptr;
}

LandWeightAtlas *LandWeightAtlasBuilder::finish()
{
  int64_t reft = ref_time_ticks();
  if (texSize < 4 || elemSize < 1)
    return nullptr;
  eastl::unique_ptr<LandWeightAtlas> a(new LandWeightAtlas(cellsX, cellsY, elemSize, texCflg));
  const int pageW = a->getCellTexSize();
  if (pageW > 4096)
    return nullptr;

  // border texels come from the neighbor cell blending the same landclass;
  // a neighbor without it means the landclass genuinely fades to 0 there
  auto fetch = [&](int ci, int ch, int px, int py) -> uint8_t {
    int cx = ci % cellsX, cy = ci / cellsX;
    int k[2] = {px - 2, py - 2}, cc[2] = {cx, cy};
    for (int axis = 0; axis < 2; axis++)
    {
      // a cell owns elemSize texels; the exported texture is that rounded up to
      // a power of two, so anything past the cell belongs to the neighbour even
      // where the texture still has rows of its own
      if (k[axis] < 0)
        cc[axis]--, k[axis] += elemSize;
      else if (k[axis] >= elemSize)
        cc[axis]++, k[axis] -= elemSize;
    }
    if (cc[0] == cx && cc[1] == cy)
      return cells[ci].chan[ch][k[1] * texSize + k[0]];
    if (uint32_t(cc[0]) >= uint32_t(cellsX) || uint32_t(cc[1]) >= uint32_t(cellsY)) // map edge: clamp own cell
      return cells[ci].chan[ch][clamp(py - 2, 0, elemSize - 1) * texSize + clamp(px - 2, 0, elemSize - 1)];
    const uint8_t *t = cellTexel(cc[0], cc[1], cells[ci].detTexIds[ch], k[0], k[1]);
    return t ? *t : 0;
  };

  Tab<uint8_t> planeBuf(framemem_ptr());
  planeBuf.resize(DET_NUM * pageW * pageW);
  for (int ci = 0; ci < cells.size(); ci++)
  {
    const uint8_t *planes[DET_NUM];
    int numTex = 0;
    for (int ch = 0; ch < DET_NUM; ch++)
    {
      uint8_t *p = planeBuf.data() + ch * pageW * pageW;
      planes[ch] = p;
      if (!cells[ci].chan[ch].size())
      {
        memset(p, 0, pageW * pageW);
        continue;
      }
      numTex = ch + 1;
      for (int py = 0; py < pageW; py++)
        for (int px = 0; px < pageW; px++)
          p[py * pageW + px] = fetch(ci, ch, px, py);
    }
    a->setCellWeights(ci, planes, cells[ci].detTexIds, numTex);
  }

  if (!a->upload())
    return nullptr;

#if DAGOR_DBGLEVEL > 0
  // read every cell back the way the shader will: a change to how pages are
  // packed then shows up here, not as weights in the game. The error is what
  // DXT1 costs on three independent weight channels sharing a page - a cell
  // blending two landclasses packs about three times better than one blending
  // four, and the level's mix decides the number.
  {
    double sse = 0, maxErr = 0;
    int64_t n = 0;
    for (int ci = 0; ci < cells.size(); ci++)
      for (int y = 0; y < elemSize; y += 5)
        for (int x = 0; x < elemSize; x += 5)
        {
          float w[DET_NUM];
          if (!a->sampleCell(ci, x, y, w))
            continue;
          for (int ch = 0; ch < DET_NUM; ch++)
          {
            const float ref = cells[ci].chan[ch].size() ? cells[ci].chan[ch][y * texSize + x] / 255.f : 0.f;
            const double err = fabsf(w[ch] - ref);
            sse += err * err;
            maxErr = max(maxErr, err);
            n++;
          }
        }
    debug("land weight atlas selfcheck: rmse %.5f max %.4f over %d samples", sqrt(sse / max<int64_t>(n, 1)), maxErr, (int)n);
  }
#endif

  a->dropCpuPages();

  if (undecodedCells)
    logerr("land weight atlas: %d of %d cells have undecodable weight textures and render as a "
           "single landclass; re-export the location",
      undecodedCells, (int)cells.size());

  debug("land weight atlas: %dx%d cells, page %d (elem %d of source %d + border), %d pages, DXT1, in %d us", cellsX, cellsY, pageW,
    elemSize, texSize, a->getPageCount(), (int)get_time_usec(reft));
  return a.release();
}
