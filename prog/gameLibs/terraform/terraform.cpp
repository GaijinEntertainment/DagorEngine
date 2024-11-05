// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <terraform/terraform.h>
#include <generic/dag_carray.h>
#include <memory/dag_framemem.h>
#include <math/dag_mathUtils.h>
#include <EASTL/vector.h>
#include <EASTL/vector_set.h>
#include <heightmap/heightmapHandler.h>
#include <gamePhys/collision/collisionLib.h>

const float Terraform::PATCH_SIZE_INV = 1.0f / Terraform::PATCH_SIZE;
const float Terraform::PATCH_ALT_MAX_INV = 1.0f / Terraform::PATCH_ALT_MAX;


Terraform::Terraform(HeightmapHandler &in_hmap, const Desc &in_desc) : hmap(in_hmap), patches() { setDesc(in_desc); }

Terraform::~Terraform()
{
  for (int patchNo = 0; patchNo < patches.size(); ++patchNo)
    del_it(patches[patchNo]);
}

void Terraform::storePatchAlt(const Pcd &pcd, uint8_t alt)
{
  if (!patches[pcd.patchNo])
    allocPatch(pcd.patchNo);
  Patch &patch = *patches[pcd.patchNo];
  if (patch.data[pcd.dataIndex] == alt)
    return;

  patch.data[pcd.dataIndex] = alt;
  patch.invalidate = true;
  updateFlags |= UPDFLAG_PATCHES;

  if (patch.bbChanges.isEmpty())
    patchChanges.push_back(pcd.patchNo);
  patch.bbChanges += getCoordFromPcd(pcd);

  for (Listener *listener : listeners)
    listener->storePatchAlt(pcd, alt);
}

uint8_t Terraform::getAltModePacked(Pcd pcd, float alt, PrimMode mode) const
{
  if (mode == DYN_REPLACE)
    return packAlt(clamp<float>(alt, desc.minLevel, desc.maxLevel));

  float curAlt = getPatchAltUnpacked(pcd);
  float newAlt = alt;
  switch (mode)
  {
    case DYN_ADDITIVE: newAlt = curAlt + alt; break;
    case DYN_MIN: newAlt = min(curAlt, alt); break;
    case DYN_MAX: newAlt = max(curAlt, alt); break;
    default: G_ASSERT(false);
  }

  return packAlt(clamp<float>(newAlt, desc.minLevel, desc.maxLevel));
}

float Terraform::sampleHeightCur(const Point2 &coord) const
{
  float primAlt = sampleFunc(coord, &Terraform::getPrimAltDefMin, IPoint2(resolution.x, resolution.y));
  return max(sampleFunc(coord, &Terraform::getPatchAltUnpackedFromCoord, IPoint2(resolution.x, resolution.y)), primAlt) +
         sampleHmapHeightOrig(coord);
}

void Terraform::invalidate()
{
  for (Patch *patch : patches)
    if (patch)
      patch->invalidate = true;
  invalidatePrims();
  updateFlags |= UPDFLAG_PATCHES;
}

void Terraform::invalidatePrims()
{
  for (QuadPrim &prim : quads)
    prim.changed = true;
  updateFlags |= UPDFLAG_PRIMS;
}

float Terraform::sampleHmapHeightCur(const Point2 &coord) const
{
  return sampleFunc(getHmapCoordFromCoordF(coord), &Terraform::getHmapHeightCur, hmapSizes);
}

float Terraform::sampleHmapHeightOrig(const Point2 &coord) const
{
  return sampleFunc(getHmapCoordFromCoordF(coord), &Terraform::getHmapHeightOrig, hmapSizes);
}

dag::ConstSpan<int> Terraform::getPatchChanges() const { return patchChanges; }

const IBBox2 &Terraform::getPatchBBChanges(int patch_no) const { return patches[patch_no]->bbChanges; }

const BBox2 &Terraform::getPatchWorldBB(int patch_no) const { return patches[patch_no]->worldBBPatch; }

void Terraform::clearPatchChanges()
{
  for (int patchNo : patchChanges)
    patches[patchNo]->bbChanges.setEmpty();
  clear_and_shrink(patchChanges);
}

bool Terraform::submitQuad(const QuadData &quad, uint16_t &prim_id)
{
  int index = prim_id > 0 ? getPrimIndex(prim_id) : -1;
  G_ASSERT(index != -1 || prim_id == 0);
  if (index == -1)
    prim_id = nextPrimId++;
  else if (quads[index].quad == quad)
    return false;

  QuadPrim &prim = index != -1 ? quads[index] : quads.push_back();
  if (index == -1)
  {
    prim.leftAlt = 0.0f;
    prim.maxAlt = 0.0f;
  }
  prim.id = prim_id;
  prim.quad = quad;
  prim.cells.clear();
  rasterizeQuad(prim, prim.cells);

  updateFlags |= UPDFLAG_PRIMS;
  prim.changed = true;
  return true;
}

void Terraform::removePrim(uint16_t prim_id)
{
  int index = getPrimIndex(prim_id);
  G_ASSERT(index != -1);
  if (index == -1)
    return;
  erase_items(quads, index, 1);
  updateFlags |= UPDFLAG_PRIMS;
}

void Terraform::storePrim(uint16_t prim_id)
{
  alt_hash_map_t altMap(dag::MemPtrAllocator{framemem_ptr()});
  getPrimData(prim_id, altMap);
  for (const auto &item : altMap)
    storePatchAlt(item.first, packAlt(item.second));
  removePrim(prim_id);
}

void Terraform::getPrimData(uint16_t prim_id, alt_hash_map_t &out_data) const
{
  int index = getPrimIndex(prim_id);
  G_ASSERT(index != -1);
  if (index == -1)
    return;
  float leftAlt, maxAlt;
  handleCells(quads[index], out_data, leftAlt, maxAlt);
}

float Terraform::getPrimLeftAlt(uint16_t prim_id) const
{
  int index = getPrimIndex(prim_id);
  G_ASSERT(index != -1);
  if (index == -1)
    return 0.0f;
  return quads[index].leftAlt;
}

float Terraform::getPrimMaxAlt(uint16_t prim_id) const
{
  int index = getPrimIndex(prim_id);
  G_ASSERT(index != -1);
  if (index == -1)
    return 0.0f;
  return quads[index].maxAlt;
}

void Terraform::storeQuad(const QuadData &quad)
{
  uint16_t primId = 0;
  submitQuad(quad, primId);
  storePrim(primId);
}

void Terraform::storeQuad(const Point2 &pos, float width, float height, float depth)
{
  Point2 wh(width * 0.5f, height * 0.5f);
  BBox2 quadBox(pos - wh, pos + wh);
  if (!(hmap.worldBox2 & quadBox))
  {
    logerr("Terraform store quad " FMT_B2 " outside of heightmap" FMT_B2, B2D(quadBox), B2D(hmap.worldBox2));
    return;
  }
  QuadData quad;
  quad.vertices[0] = getCoordFromPos(quadBox.leftTop());
  quad.vertices[1] = getCoordFromPos(quadBox.rightTop());
  quad.vertices[2] = getCoordFromPos(quadBox.rightBottom());
  quad.vertices[3] = getCoordFromPos(quadBox.leftBottom());
  IPoint2 size = quad.vertices[2] - quad.vertices[0];
  quad.diffAlt = (float)((size.x + 1) * (size.y + 1)) * depth;
  storeQuad(quad);
}

void Terraform::storeSphereAlt(const Point2 &pos, float radius, float alt, PrimMode mode)
{
  BBox2 sphBox(pos, radius * 2.f);
  if (!(hmap.worldBox2 & sphBox))
  {
    logerr("Terraform store sphere " FMT_P2 " rad=%f outside of heightmap" FMT_B2, P2D(pos), radius, B2D(hmap.worldBox2));
    return;
  }
  IPoint2 coordStart = getCoordFromPos(sphBox.getMin());
  IPoint2 coordEnd = getCoordFromPos(sphBox.getMax());
  float radiusSq = sqr(radius);

  for (int y = coordStart.y; y < coordEnd.y; ++y)
    for (int x = coordStart.x; x < coordEnd.x; ++x)
    {
      IPoint2 coord(x, y);
      Point2 cellPos = getPosFromCoord(coord);
      float distSq = lengthSq(pos - cellPos);
      if (distSq > radiusSq)
        continue;
      storePatchAltUnpacked(makePcdFromCoord(coord), alt * (1.f - distSq / radiusSq), mode);
    }
}

void Terraform::queueElevationChange(const Point2 &pos, float radius, float alt)
{
  Elevation &elev = elevations.push_back();
  elev.pos = pos;
  elev.radius = radius;
  elev.alt = alt;
  updateFlags |= UPDFLAG_ELEVATIONS | UPDFLAG_PRIMS | UPDFLAG_PATCHES;
}

void Terraform::updateInternal()
{
  dacoll::fetch_sim_res(true);

  int curUpdateFlags = updateFlags;
  updateFlags = 0;
  IBBox2 changedCoord;

  if (curUpdateFlags & (UPDFLAG_PATCHES | UPDFLAG_PRIMS | UPDFLAG_ELEVATIONS))
    hmap.checkOrAllocateOwnData();

  const int upwardRaw = (int)(hmap.getMaxUpwardDisplacement() / hmap.getHeightScaleRaw()),
            downwardRaw = (int)(hmap.getMaxDownwardDisplacement() / hmap.getHeightScaleRaw());
  if (curUpdateFlags & UPDFLAG_ELEVATIONS)
  {
    eastl::vector_set<uint32_t, eastl::less<uint32_t>, framemem_allocator> updated;
    for (const auto &elev : elevations)
      addOrigHmapAltSph(elev.pos, elev.radius, elev.alt, updated, changedCoord);
    clear_and_shrink(elevations);
  }

  uint32_t numTotal = max<uint32_t>(hmapCellResolution.x * hmapCellResolution.y, 1);
  IPoint2 hmapCellsPerPatch = IPoint2::xy(PATCH_SIZE * hmapCellResolutionInv);

  if (curUpdateFlags & UPDFLAG_PATCHES)
  {
    for (int py = 0, hcy = 0, patchNo = 0; py < patchNumY; ++py, hcy += hmapCellsPerPatch.y)
      for (int px = 0, hcx = 0; px < patchNumX; ++px, hcx += hmapCellsPerPatch.x, ++patchNo)
      {
        if (!patches[patchNo] || !patches[patchNo]->invalidate)
          continue;
        Patch &patch = *patches[patchNo];
        patch.invalidate = false;

        for (int hy = 0, dy = 0; hy < hmapCellsPerPatch.y; ++hy, dy += hmapCellResolution.y)
          for (int hx = 0, dx = 0; hx < hmapCellsPerPatch.x; ++hx, dx += hmapCellResolution.x)
          {
            uint32_t avgAlt = 0;
            for (int y = 0; y < hmapCellResolution.y; ++y)
              for (int x = 0; x < hmapCellResolution.x; ++x)
                avgAlt += patch.data[(dy + y) * PATCH_SIZE + dx + x];
            avgAlt /= numTotal;

            IPoint2 hmapCoord(hcx + hx, hcy + hy);
            uint16_t origHeight = patch.hmapSaved[hy * hmapCellsPerPatch.x + hx];
            uint16_t heightRes = addAltToHmapAlt(origHeight, avgAlt);
            auto changed = hmap.setHeightmapHeightUnsafe(hmapCoord, heightRes);

            if (changed != HeightmapPhysHandler::UpdateHtResult::UNCHANGED)
            {
              hmap.changedHeightmapCellUnsafe(hmapCoord);
              if ((int(heightRes) < int(origHeight - downwardRaw) || int(heightRes) > int(origHeight + upwardRaw)))
                changedCoord += hmapCoord;
            }
            hmap.tesselatePatch(hmapCoord, heightRes != origHeight);
          }

        if (renderer)
          renderer->invalidatePatch(patchNo);
      }
  }

  if (curUpdateFlags & UPDFLAG_PRIMS)
  {
    primAltMap.clear();
    hmap.clearHeightmapHeightsVisual();

    for (QuadPrim &prim : quads)
    {
      prim.changed = false;
      handleCells(prim, primAltMap, prim.leftAlt, prim.maxAlt);
    }

    ska::flat_hash_map<uint32_t, Tab<uint8_t>, eastl::hash<uint32_t>, eastl::equal_to<uint32_t>, dag::MemPtrAllocator> tiledAltMap(
      dag::MemPtrAllocator{framemem_ptr()});
    for (const auto &item : primAltMap)
    {
      Pcd pcd = Pcd(item.first);
      IPoint2 coord = getCoordFromPcd(pcd);
      IPoint2 hmapCoord = getHmapCoordFromCoord(coord);
      uint32_t index = hmapCoord.y * hmapSizes.x + hmapCoord.x;
      auto iter = tiledAltMap.find(index);
      if (iter == tiledAltMap.end())
      {
        iter = tiledAltMap.insert(eastl::make_pair(index, Tab<uint8_t>())).first;
        iter->second.resize(numTotal);
        for (auto &elem : iter->second)
          elem = zeroAlt;
      }
      IPoint2 hxy = coord - IPoint2(hmapCoord.x * hmapCellResolution.x, hmapCoord.y * hmapCellResolution.y);
      iter->second[hxy.y * hmapCellResolution.x + hxy.x] = packAlt(item.second);
    }
    for (const auto &item : tiledAltMap)
    {
      IPoint2 hmapCoord(item.first % hmapSizes.x, item.first / hmapSizes.x);
      IPoint2 coord = getCoordFromHmapCoord(hmapCoord);
      Pcd pcd = makePcdFromCoord(coord);
      Patch *patch = patches[pcd.patchNo];
      IPoint2 patchXY = getPatchXYFromCoord(coord);
      int hx = hmapCoord.x - patchXY.x * hmapCellsPerPatch.x;
      int hy = hmapCoord.y - patchXY.y * hmapCellsPerPatch.y;
      int dx = coord.x - patchXY.x * PATCH_SIZE;
      int dy = coord.y - patchXY.y * PATCH_SIZE;

      uint32_t avgAlt = 0;
      for (int y = 0, index = 0; y < hmapCellResolution.y; ++y)
        for (int x = 0; x < hmapCellResolution.x; ++x, ++index)
        {
          uint8_t alt = item.second[index];
          if (alt == zeroAlt && patch)
            alt = patch->data[(dy + y) * PATCH_SIZE + dx + x];
          avgAlt += alt;
        }
      avgAlt /= numTotal;

      uint16_t origHeight = patch ? patch->hmapSaved[hy * hmapCellsPerPatch.x + hx] : hmap.getHeightmapHeightUnsafe(hmapCoord);
      uint16_t heightRes = addAltToHmapAlt(origHeight, avgAlt);
      bool changed = hmap.setHeightmapHeightUnsafeVisual(hmapCoord, heightRes);
      if (changed)
        hmap.changedHeightmapCellUnsafe(hmapCoord);
    }

    if (renderer)
      renderer->invalidatePrims();
  }
  if (!changedCoord.isEmpty())
    hmap.invalidateCulling(changedCoord);
}

void Terraform::setDesc(const Desc &in_desc)
{
  G_ASSERT(in_desc.maxLevel > in_desc.minLevel);
  desc = in_desc;
  // check > 0 and power of two
  G_ASSERT(desc.cellsPerMeter > 0 && (desc.cellsPerMeter & (desc.cellsPerMeter - 1)) == 0);
  G_ASSERT(desc.maxLevel > desc.minLevel);

  BBox3 hmapBox = hmap.getWorldBox();
  pivot = Point2::xz(hmapBox.center());
  worldSize = Point2::xz(hmapBox.width());
  resolution = IPoint2(min<int>(int(worldSize.x * ALT_PRECISION + 0.5f) * desc.cellsPerMeter / 100, MAX_CELLS_SIZE),
    min<int>(int(worldSize.y * ALT_PRECISION + 0.5f) * desc.cellsPerMeter / 100, MAX_CELLS_SIZE));
  hmap.setMaxDownwardDisplacement(-desc.getMinLevel());

  packAltScale = PATCH_ALT_MAX / (desc.maxLevel - desc.minLevel);
  packAltOffset = -PATCH_ALT_MAX * desc.minLevel / (desc.maxLevel - desc.minLevel);
  unpackAltScale = (desc.maxLevel - desc.minLevel) / PATCH_ALT_MAX;
  unpackAltOffset = desc.minLevel;
  minLevelInt = desc.minLevel * ALT_PRECISION + 0.5f;
  maxLevelInt = desc.maxLevel * ALT_PRECISION + 0.5f;
  G_STATIC_ASSERT((HMAP_ALT_MAX % PATCH_ALT_MAX) == 0);
  hmapAltScaleInt = (maxLevelInt - minLevelInt) * (int)(HMAP_ALT_MAX / PATCH_ALT_MAX);
  hmapAltOffsetInt = minLevelInt * (int)HMAP_ALT_MAX;
  hmapScaleLevelInt = hmap.getHeightScale() * ALT_PRECISION + 0.5f;
  zeroAlt = computeZeroAlt(minLevelInt, maxLevelInt);
  minDAlt = MIN_DELTA_ALT_BITS * (desc.maxLevel - desc.minLevel) * PATCH_ALT_MAX_INV;

  G_ASSERT((resolution.x % PATCH_SIZE) == 0 && (resolution.y % PATCH_SIZE) == 0);
  patchNumX = resolution.x / PATCH_SIZE;
  patchNumY = resolution.y / PATCH_SIZE;
  G_ASSERT(patchNumX * patchNumY - 1 <= UINT16_MAX);
  patches.resize(patchNumX * patchNumY);
  mem_set_0(patches);

  hmapSizes = IPoint2(hmap.getHeightmapSizeX(), hmap.getHeightmapSizeY());
  G_ASSERT(hmapSizes.x > 0 && hmapSizes.y > 0);
  G_ASSERT((resolution.x % hmapSizes.x) == 0 && (resolution.y % hmapSizes.y) == 0);
  G_ASSERT((hmapSizes.x % patchNumX) == 0 && (hmapSizes.y % patchNumY) == 0);
  hmapCellResolution = IPoint2(resolution.x / hmapSizes.x, resolution.y / hmapSizes.y);
  hmapCellResolutionInv = Point2(1.0f / hmapCellResolution.x, 1.0f / hmapCellResolution.y);
  G_ASSERT((hmapCellResolution.x % 2) == 0 && (hmapCellResolution.y % 2) == 0);
  hmapPatchSize = IPoint2(hmapSizes.x / patchNumX, hmapSizes.y / patchNumY);
  Point2 cellWorldSize(worldSize.x / resolution.x, worldSize.y / resolution.y);
  primSpreadRadius = clamp<int>(desc.maxLevel / clamp(min(cellWorldSize.x, cellWorldSize.y), 0.001f, 1.0f), 1, desc.maxSpreadRadius);

  posToCoordScale = Point2(1.0f / worldSize.x * resolution.x, 1.0f / worldSize.y * resolution.y);
  posToCoordOffset = Point2((-pivot.x / worldSize.x + 0.5f) * resolution.x + hmapCellResolution.x / 2,
    (-pivot.y / worldSize.y + 0.5f) * resolution.y + hmapCellResolution.y / 2);
  coordToPosScale = Point2(1.0f / posToCoordScale.x, 1.0f / posToCoordScale.y);
  coordToPosOffset = Point2(-posToCoordOffset.x / posToCoordScale.x, -posToCoordOffset.y / posToCoordScale.y);
}

bool Terraform::testRegionChanged(const IBBox2 &region) const
{
  IPoint2 patchXY1 = getPatchXYFromCoord(region.lim[0]);
  IPoint2 patchXY2 = getPatchXYFromCoord(region.lim[1]);
  for (int y = patchXY1.y; y <= patchXY2.y; ++y)
    for (int x = patchXY1.x; x <= patchXY2.x; ++x)
    {
      int patchNo = getPatchNoFromXY(IPoint2(x, y));
      if (patchNo >= 0 && patchNo < patches.size() && patches[patchNo])
        return true;
    }
  return false;
}

bool Terraform::testRegionChanged(const BBox2 &region) const
{
  return testRegionChanged(IBBox2(getCoordFromPos(region.lim[0]), getCoordFromPos(region.lim[1])));
}

void Terraform::copyOriginalHeightMap(uint16_t *dst) const
{
  for (int patchY = 0; patchY < patchNumY; patchY++)
    for (int patchX = 0; patchX < patchNumX; patchX++)
    {
      int fullMapOffset = patchY * hmapPatchSize.y * hmapSizes.x + patchX * hmapPatchSize.x;
      uint16_t *dstPatch = dst + fullMapOffset;
      int patchNo = patchY * patchNumX + patchX;
      const uint16_t *src = nullptr;
      int stride = 0;
      if (patches[patchNo])
      {
        src = patches[patchNo]->hmapSaved.data();
        stride = hmapPatchSize.x;
        for (int row = 0; row < hmapPatchSize.y; row++)
          memcpy(dstPatch + hmapSizes.x * row, src + stride * row, hmapPatchSize.x * sizeof(*src));
      }
      else
      {
        unsigned x0 = patchX * hmapPatchSize.x, y0 = patchY * hmapPatchSize.y;
        hmap.getCompressedData().iteratePixels(x0, y0, hmapPatchSize.x, hmapPatchSize.y,
          [&](uint32_t x, uint32_t y, uint32_t ht) { dstPatch[hmapSizes.x * (y - y0) + (x - x0)] = ht; });
      }
    }
}

void Terraform::copy(const Terraform &rhs)
{
  for (int i = 0, sz = patches.size(); i < sz; ++i)
    del_it(patches[i]);

  desc = rhs.desc;

  setDesc(rhs.desc);

  for (int i = 0, sz = patches.size(); i < sz; ++i)
    if (const Patch *other = rhs.patches[i])
    {
      if (!patches[i])
        allocPatch(i);
      *patches[i] = *other;
    }
}

bool Terraform::isEqual(const Terraform &rhs) const
{
  if (getPatchNum() != rhs.getPatchNum())
    return false;

  for (int i = 0, sz = patches.size(); i < sz; ++i)
  {
    const Patch *me = patches[i];
    const Patch *other = rhs.patches[i];
    if (me && !other)
      return false;
    if (!me && other)
      return false;
    if (me && other && *me != *other)
      return false;
  }

  return true;
}

uint8_t Terraform::computeZeroAlt(int min_level_int, int max_level_int)
{
  return (-min_level_int * (int)PATCH_ALT_MAX) / (max_level_int - min_level_int);
}

uint8_t Terraform::computeZeroAltF(float min_level, float max_level)
{
  return computeZeroAlt(min_level * ALT_PRECISION + 0.5f, max_level * ALT_PRECISION + 0.5f);
}

void Terraform::allocPatch(int patch_no)
{
  G_ASSERT(!patches[patch_no]);

  patches[patch_no] = new Patch();
  Patch &patch = *patches[patch_no];
  patch.invalidate = false;
  patch.bbChanges.setEmpty();

  int patchY = (patch_no / patchNumX);
  int patchX = (patch_no % patchNumX);
  Point2 pos0 = getPosFromCoord(IPoint2(patchX * PATCH_SIZE, patchY * PATCH_SIZE));
  Point2 pos1 = getPosFromCoord(IPoint2((patchX + 1) * PATCH_SIZE, (patchY + 1) * PATCH_SIZE));
  patch.worldBBPatch += pos0;
  patch.worldBBPatch += pos1;

  for (int i = 0; i < patch.data.size(); ++i)
    patch.data[i] = zeroAlt;

  patch.hmapSaved.resize(hmapPatchSize.x * hmapPatchSize.y);

  int y = patchY * hmapPatchSize.y;
  int x = patchX * hmapPatchSize.x;
  uint16_t *dstBase = patch.hmapSaved.data();
  hmap.getCompressedData().iteratePixels(x, y, hmapPatchSize.x, hmapPatchSize.y,
    [&](uint32_t px, uint32_t py, uint32_t ht) { dstBase[hmapPatchSize.x * (py - y) + (px - x)] = ht; });

  for (Listener *listener : listeners)
    listener->allocPatch(patch_no);
}

void Terraform::rasterizeQuad(const QuadPrim &prim, Tab<IPoint2> &out_cells) const
{
  struct Context
  {
    Context(const Terraform &in_tform, Tab<IPoint2> &in_cells) : tform(in_tform), cells(in_cells) {}
    const Terraform &tform;
    Tab<IPoint2> &cells;
  };
  Context ctx(*this, out_cells);
  struct FillCell
  {
    FillCell(Context &in_ctx) : ctx(in_ctx) {}

    void operator()(int x, int y) const
    {
      if (x < 0 || y < 0 || x >= ctx.tform.resolution.x || y >= ctx.tform.resolution.y)
        return;
      IPoint2 &cell = ctx.cells.push_back();
      cell = IPoint2(x, y);
    }
    Context &ctx;
  } fillCell(ctx);

  rastr_tri(prim.quad.vertices[0], prim.quad.vertices[1], prim.quad.vertices[2], fillCell);
  rastr_tri(prim.quad.vertices[0], prim.quad.vertices[2], prim.quad.vertices[3], fillCell);

  /*struct CellsCompare
  {
    CellsCompare(const Context &in_ctx): ctx(in_ctx) {}

    int order(const DigCell &cell1, const DigCell &cell2) const
    {
      IPoint2 localCoord1 = cell1.coord - ctx.digCtx.worldBox[0];
      int index1 = localCoord1.x * ctx.boxSize + localCoord1.y;
      IPoint2 localCoord2 = cell2.coord - ctx.digCtx.worldBox[0];
      int index2 = localCoord2.x * ctx.boxSize + localCoord2.y;
      return index2 - index1;
    }
    const Context &ctx;
  };
  sort(ctx.cells, CellsCompare(ctx));*/
}

int Terraform::getPrimIndex(uint16_t prim_id) const
{
  if (prim_id > 0)
    for (int i = 0; i < quads.size(); ++i)
      if (quads[i].id == prim_id)
        return i;
  return -1;
}

template <typename Func>
float Terraform::sampleFunc(const Point2 &coord, Func func, const IPoint2 &sample_size) const
{
  IPoint2 coord0 = IPoint2::xy(coord);

  if (coord.x >= 0 && coord.y >= 0 && (coord.x + 1) < sample_size.x && (coord.y + 1) < sample_size.y)
  {
    IPoint2 coord1 = coord0 + IPoint2(1, 0);
    IPoint2 coord2 = coord0 + IPoint2(0, 1);
    IPoint2 coord3 = coord0 + IPoint2(1, 1);
    Point2 lerpK = coord - coord0;

    float ht0 = (this->*func)(coord0);
    float ht1 = (this->*func)(coord1);
    float ht2 = (this->*func)(coord2);
    float ht3 = (this->*func)(coord3);

    return lerp(lerp(ht0, ht1, lerpK.x), lerp(ht2, ht3, lerpK.x), lerpK.y);
  }
  else
  {
    coord0.x = clamp(coord0.x, 0, sample_size.x - 1);
    coord0.y = clamp(coord0.y, 0, sample_size.y - 1);
    return (this->*func)(coord0);
  }
}

void Terraform::handleCells(const Terraform::QuadPrim &prim, alt_hash_map_t &out_alt_map, float &out_left_alt,
  float &out_max_alt) const
{
  const int RAD = primSpreadRadius;
  const int RADSQ = sqr(RAD);


  auto getCachedAlt = [&](Pcd pcd) -> float & {
    uint32_t i = pcd.toInt();
    auto it = out_alt_map.find(i);
    if (it == out_alt_map.end())
      it = out_alt_map.insert(eastl::make_pair(i, getPatchAltUnpacked(pcd))).first;
    return it->second;
  };

  float diffAlt = prim.quad.diffAlt;
  out_max_alt = desc.minLevel;
  for (int cellNo = 0; cellNo < prim.cells.size(); ++cellNo)
  {
    const IPoint2 &cell = prim.cells[cellNo];
    float addAltOrig = diffAlt / (prim.cells.size() - cellNo);
    float addAlt = addAltOrig;
    float totalSpaceAlt = 0;

    for (int ry = -RAD + 1; ry < RAD; ++ry)
      for (int rx = -RAD + 1; rx < RAD; ++rx)
      {
        if (lengthSq(IPoint2(rx, ry)) > RADSQ)
          continue;
        IPoint2 coord(cell.x + rx, cell.y + ry);
        Pcd pcd = makePcdFromCoord(coord);

        float curAlt = getCachedAlt(pcd);
        out_max_alt = max(curAlt, out_max_alt);
        totalSpaceAlt += desc.getMaxLevel() - curAlt;
      }

    int levW = 0;
    for (int rd = 0; rd < RAD; ++rd)
      levW += (RAD - rd) * max(rd * 8, 1);
    for (int rd = 0; rd < RAD && addAlt > 0; ++rd)
    {
      int wd = max(rd * 2, 1);
      for (int cd = 0; cd < max(rd * 8, 1) && addAlt > 0; ++cd)
      {
        int qd = cd / wd;
        int rx, ry;
        switch (qd)
        {
          case 0:
            rx = cd;
            ry = 0;
            break;
          case 1:
            rx = wd;
            ry = cd - wd;
            break;
          case 2:
            rx = wd - cd % wd;
            ry = wd;
            break;
          default:
            rx = 0;
            ry = wd - cd % wd;
            break;
        }
        rx -= rd;
        ry -= rd;
        const float lenN = RAD - rd;

        if (lengthSq(IPoint2(rx, ry)) > RADSQ)
        {
          levW -= lenN;
          continue;
        }

        IPoint2 coord(cell.x + rx, cell.y + ry);
        if (coord.x < 0 || coord.y < 0 || coord.x >= resolution.x || coord.y >= resolution.x)
          continue;
        Pcd pcd = makePcdFromCoord(coord);

        float &curAlt = getCachedAlt(pcd);
        float spaceAlt = desc.getMaxLevel() - curAlt;
        float spentAlt = min(addAlt * lenN / levW + max(addAlt - totalSpaceAlt, 0.0f), addAlt);
        levW -= lenN;
        totalSpaceAlt -= spaceAlt;

        spentAlt = min(spentAlt, spaceAlt);
        addAlt -= spentAlt;

        float newAlt = curAlt + spentAlt;
        curAlt = newAlt;
        out_max_alt = max(newAlt, out_max_alt);
      }
    }
    diffAlt -= addAltOrig - addAlt;
  }

  out_left_alt = diffAlt;
}

float Terraform::getHmapHeightCurVal(const IPoint2 &hmap_coord) const { return getHmapHeightCur(hmap_coord); }

float Terraform::getHmapHeightOrigVal(const IPoint2 &hmap_coord) const { return getHmapHeightOrig(hmap_coord); }

__forceinline float Terraform::getHmapAlt(const IPoint2 &hmap_coord) const
{
  float height0 = hmap.unpackHeightmapHeight(getHmapHeightOrigPacked(hmap_coord));
  return hmap.unpackHeightmapHeight(hmap.getHeightmapHeightUnsafe(hmap_coord)) - height0;
}

__forceinline float Terraform::getHmapHeightCur(const IPoint2 &hmap_coord) const
{
  if (DAGOR_LIKELY(unsigned(hmap_coord.x) < unsigned(hmapSizes.x) && unsigned(hmap_coord.y) < unsigned(hmapSizes.y)))
    return hmap.unpackHeightmapHeight(hmap.getHeightmapHeightUnsafe(hmap_coord));
  return 0.f;
}

__forceinline float Terraform::getHmapHeightOrig(const IPoint2 &hmap_coord) const
{
  if (DAGOR_LIKELY(unsigned(hmap_coord.x) < unsigned(hmapSizes.x) && unsigned(hmap_coord.y) < unsigned(hmapSizes.y)))
    return hmap.unpackHeightmapHeight(getHmapHeightOrigPacked(hmap_coord));
  return 0.f;
}

__forceinline uint16_t Terraform::getHmapHeightOrigPacked(const IPoint2 &hmap_coord) const
{
  unsigned patchNo = (hmap_coord.y / hmapPatchSize.y) * patchNumX + (hmap_coord.x / hmapPatchSize.x);
  unsigned dataIndex = (hmap_coord.y % hmapPatchSize.y) * hmapPatchSize.x + (hmap_coord.x % hmapPatchSize.x);
  return (patchNo < patches.size() && patches[patchNo] && dataIndex < patches[patchNo]->hmapSaved.size())
           ? patches[patchNo]->hmapSaved[dataIndex]
           : hmap.getHeightmapHeightUnsafe(hmap_coord);
}

template <typename HS>
void Terraform::addOrigHmapAltSph(const Point2 &pos, float radius, float alt, HS &updated, IBBox2 &updatedCoord)
{
  if (radius <= 0.f)
    return;

  const float hScale = hmap.getHeightScale();
  alt *= (hScale > 0.f) ? (65535.0f / hScale) : 0.0f;
  const int hmap_alt_coef = alt > 0.f ? 1 : -1; // make it in undo/redo friendly way
  if (hmap_alt_coef < 0)
    alt = -alt;
  if (alt > 0.0f && alt < 0.50001f)
    alt = 0.50001f;

  Point2 radiusVec = Point2(radius, radius);

  const Point2 hmapPos = getHmapCoordFromCoordF(getCoordFromPosF(pos));
  const Point2 hmapStart = getHmapCoordFromCoordF(getCoordFromPosF(pos - radiusVec));
  const Point2 hmapEnd = getHmapCoordFromCoordF(getCoordFromPosF(pos + radiusVec));

  const float hmapRadius = hmapEnd.x - hmapPos.x;
  const float hmapRadiusSq = hmapRadius * hmapRadius;

  const IPoint2 coordMin = IPoint2(0, 0);
  const IPoint2 coordMax = IPoint2(hmapSizes.x - 1, hmapSizes.y - 1);
  const IPoint2 coordStart = clamp(IPoint2((int)floorf(hmapStart.x), (int)floorf(hmapStart.y)), coordMin, coordMax);
  const IPoint2 coordEnd = clamp(IPoint2((int)ceilf(hmapEnd.x), (int)ceilf(hmapEnd.y)), coordMin, coordMax);

  for (int y = coordStart.y; y < coordEnd.y; ++y)
    for (int x = coordStart.x; x < coordEnd.x; ++x)
    {
      IPoint2 hmapCoord(x, y);
      Point2 cellPos = Point2(x + 0.5f, y + 0.5f);
      float distSq = lengthSq(hmapPos - cellPos);
      if (distSq > hmapRadiusSq)
        continue;

      const float cellAlt = alt * (1.f - distSq / hmapRadiusSq);
      const int hmap_alt = hmap_alt_coef * (int)floorf(cellAlt + 0.5f);
      if (!hmap_alt)
        continue;

      unsigned patchNo = (hmapCoord.y / hmapPatchSize.y) * patchNumX + (hmapCoord.x / hmapPatchSize.x);
      unsigned dataIndex = (hmapCoord.y % hmapPatchSize.y) * hmapPatchSize.x + (hmapCoord.x % hmapPatchSize.x);
      if (patchNo < patches.size() && patches[patchNo] && dataIndex < patches[patchNo]->hmapSaved.size())
      {
        Patch *patch = patches[patchNo];
        patch->invalidate = true;
        uint16_t &hmapValue = patch->hmapSaved[dataIndex];
        hmapValue = (uint16_t)clamp(hmapValue + hmap_alt, 0, (int)HMAP_ALT_MAX);
      }
      else
      {
        uint16_t hmapValue = hmap.getHeightmapHeightUnsafe(hmapCoord);
        hmapValue = (uint16_t)clamp(hmapValue + hmap_alt, 0, (int)HMAP_ALT_MAX);
        if (hmap.setHeightmapHeightUnsafe(hmapCoord, hmapValue) != HeightmapPhysHandler::UpdateHtResult::UNCHANGED)
          updatedCoord += hmapCoord;
      }

      const uint32_t updKey = x | (y << 16);
      if (updated.insert(updKey).second)
      {
        hmap.changedHeightmapCellUnsafe(hmapCoord);
        hmap.tesselatePatch(hmapCoord, true);
      }
    }
}

float Terraform::sampleHmapAlt(const Point2 &hmap_coord) const { return sampleFunc(hmap_coord, &Terraform::getHmapAlt, hmapSizes); }

float Terraform::getPrimAltDefMin(const IPoint2 &coord) const
{
  auto iter = primAltMap.find(makePcdFromCoord(coord).toInt());
  if (iter == primAltMap.end())
    return desc.minLevel;
  return iter->second;
}
