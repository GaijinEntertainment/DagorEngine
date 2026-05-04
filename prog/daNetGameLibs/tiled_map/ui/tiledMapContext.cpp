// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "tiledMapContext.h"
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_texMgr.h>
#include <math/dag_Point2.h>
#include <daRg/dag_element.h>
#include <debug/dag_log.h>
#include <drv/3d/dag_renderTarget.h>
#include <EASTL/vector_set.h>
#include <EASTL/algorithm.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <ui/scriptStrings.h>
#include <util/dag_base64.h>


#define VAR(a) ShaderVariableInfo a(#a, true);
VAR(fog_of_war_constraints)
VAR(fog_of_war_width)
VAR(fog_of_war_height)
VAR(fog_of_war_bitset)
#undef VAR

SQ_PRECACHED_STRINGS_IMPLEMENT(TiledMapContext, tiledMapContext);

// #define TILEDMAP_DEBUG debug // enable debug output
#ifndef TILEDMAP_DEBUG
#define TILEDMAP_DEBUG(...) ((void)0)
#endif

inline bool hasLoadedTile(const TilesHashMap &tiles, const QuadKey &quadKey)
{
  auto it = tiles.find(quadKey);
  if (it == tiles.end())
    return false;

  return it->second.texId != BAD_TEXTUREID && it->second.picId != BAD_PICTUREID;
}

struct TileAsyncLoadRequest
{
  QuadKey quadKey;
  bool isFog;
  int generation = 0;
};

static TiledMapContext *s_tiled_map_ctx = NULL;
static eastl::vector_set<TileAsyncLoadRequest *> requests;

inline void releasePicTex(PICTUREID &pid, TEXTUREID &tid)
{
  if (pid != BAD_PICTUREID)
    PictureManager::free_picture(pid);
  else if (tid != BAD_TEXTUREID)
  {
    release_managed_tex(tid);
    if (get_managed_texture_refcount(tid) == 0)
      evict_managed_tex_id(tid);
  }
  pid = BAD_PICTUREID;
  tid = BAD_TEXTUREID;
}

inline void load_tile_cb(PICTUREID pid, TEXTUREID tid, d3d::SamplerHandle smp, void *arg, TilesHashMap &out_hashmap)
{
  auto req = (TileAsyncLoadRequest *)arg;

  if (tid == BAD_TEXTUREID) // when load aborted we receive proper pid and bad tid
  {
    requests.erase(req);
    delete req;
    return;
  }

  // Discard callbacks from a superseded config (e.g. fast context switch between maps).
  // freeAllPictures() bumps tileLoadGeneration, so any in-flight request issued before
  // that point will mismatch and must not write into the current context's tile maps.
  if (req->generation != s_tiled_map_ctx->tileLoadGeneration)
  {
    releasePicTex(pid, tid);
    requests.erase(req);
    delete req;
    return;
  }

  auto it = out_hashmap.find(req->quadKey);
  if (it == out_hashmap.end())
  {
    releasePicTex(pid, tid);
    if (req->isFog)
      TILEDMAP_DEBUG("TiledMapContext: load_tile_cb: %s fog tile not needed anymore", req->quadKey);
    else
      TILEDMAP_DEBUG("TiledMapContext: load_tile_cb: %s tile not needed anymore", req->quadKey);
  }
  else
  {
    out_hashmap[req->quadKey].picId = pid;
    out_hashmap[req->quadKey].texId = tid;
    out_hashmap[req->quadKey].smpId = smp;
    if (req->isFog)
      TILEDMAP_DEBUG("TiledMapContext: load_tile_cb: %s fog tile loaded", req->quadKey);
    else
      TILEDMAP_DEBUG("TiledMapContext: load_tile_cb: %s tile loaded", req->quadKey);
  }

  // remove outdated children in quadtree
  eastl::vector<QuadKey> removeTiles;
  for (auto &tile : out_hashmap)
  {
    eastl::string_view quadKey = tile.first;
    if (tile.first.size() != s_tiled_map_ctx->z && tile.first != req->quadKey && quadKey.starts_with(req->quadKey))
    {
      releasePicTex(tile.second.picId, tile.second.texId);
      removeTiles.push_back(tile.first);
    }
  }

  for (const auto &quadKey : removeTiles)
  {
    TILEDMAP_DEBUG("TiledMapContext: load_tile_cb: cb for %s, remove %d tile", req->quadKey, quadKey);
    out_hashmap.erase(quadKey);
  }

  requests.erase(req);
  delete req;
}

static void load_current_zlvl_tiles_cb(
  PICTUREID pid, TEXTUREID tid, d3d::SamplerHandle smp, const Point2 *tcLt, const Point2 *tcRb, const Point2 *picture_sz, void *arg)
{
  G_UNUSED(tcLt);
  G_UNUSED(tcRb);
  G_UNUSED(picture_sz);

  if (!s_tiled_map_ctx || !arg)
    return;
  load_tile_cb(pid, tid, smp, arg, s_tiled_map_ctx->tiles);
};

static void load_fog_tiles_cb(
  PICTUREID pid, TEXTUREID tid, d3d::SamplerHandle smp, const Point2 *tcLt, const Point2 *tcRb, const Point2 *picture_sz, void *arg)
{
  G_UNUSED(tcLt);
  G_UNUSED(tcRb);
  G_UNUSED(picture_sz);

  if (!s_tiled_map_ctx || !arg)
    return;
  load_tile_cb(pid, tid, smp, arg, s_tiled_map_ctx->fogTiles);
};

QuadKey TiledMapContext::tileXYToQuadKey(int tileX, int tileY, int zoom)
{
  QuadKey quadKey(eastl::string::CtorDoNotInitialize{}, zoom);
  for (int i = zoom; i > 0; i--)
  {
    char digit = '0';
    int mask = 1 << (i - 1);
    if ((tileX & mask) != 0)
    {
      digit++;
    }
    if ((tileY & mask) != 0)
    {
      digit++;
      digit++;
    }
    quadKey.push_back(digit);
  }
  return quadKey;
}

IPoint2 TiledMapContext::quadKeyToTileXY(const QuadKey &quadKey, int zoom)
{
  int tileX = 0;
  int tileY = 0;
  for (int i = 0; i < zoom; i++)
  {
    int mask = 1 << (zoom - i - 1);
    char digit = quadKey[i];
    if (digit & 1)
    {
      tileX |= mask;
    }
    if (digit & 2)
    {
      tileY |= mask;
    }
  }
  return IPoint2(tileX, tileY);
}

TiledMapContext::TiledMapContext()
{
  if (s_tiled_map_ctx != nullptr)
    *this = std::move(*s_tiled_map_ctx);
  s_tiled_map_ctx = this;
}

TiledMapContext::~TiledMapContext()
{
  freeAllPictures();

  if (s_tiled_map_ctx == this)
    s_tiled_map_ctx = nullptr;
}

void TiledMapContext::freeAllPictures()
{
  TILEDMAP_DEBUG("TiledMapContext::freeAllPictures");
  if (!s_tiled_map_ctx)
    return;
  s_tiled_map_ctx->canLoadTiles = false;
  s_tiled_map_ctx->tileLoadGeneration++;
  for (auto &tile : s_tiled_map_ctx->tiles)
    releasePicTex(tile.second.picId, tile.second.texId);
  for (auto &tile : s_tiled_map_ctx->fogTiles)
    releasePicTex(tile.second.picId, tile.second.texId);
  s_tiled_map_ctx->tiles.clear();
  s_tiled_map_ctx->fogTiles.clear();
}

void TiledMapContext::setViewportSizeInner(int width, int height)
{
  if (width == 0 || height == 0)
    return;
  viewportWidth = width;
  viewportHeight = height;
  viewportRatioLargerToSmaller =
    viewportWidth >= viewportHeight ? (float(viewportWidth) / float(viewportHeight)) : (float(viewportHeight) / float(viewportWidth));
}

void TiledMapContext::clampVisibleRadiusRangeToWorldBorder()
{
  const float worldBorderMaxVisibleRadiusViewportHorizontal = worldBorderSize.x / 2;
  const float worldBorderMaxVisibleRadiusViewportVertical = worldBorderSize.y / 2;
  const float worldBorderMaxVisibleRadiusViewportLargerSide =
    viewportWidth >= viewportHeight ? worldBorderMaxVisibleRadiusViewportHorizontal : worldBorderMaxVisibleRadiusViewportVertical;
  const float worldBorderMaxVisibleRadiusViewportSmallerSide =
    viewportWidth >= viewportHeight ? worldBorderMaxVisibleRadiusViewportVertical : worldBorderMaxVisibleRadiusViewportHorizontal;
  float maxVisibleRadiusByClamp = zoomToFitBorderEdges
                                    ? min(worldBorderMaxVisibleRadiusViewportLargerSide / viewportRatioLargerToSmaller,
                                        worldBorderMaxVisibleRadiusViewportSmallerSide)
                                    : max(worldBorderMaxVisibleRadiusViewportLargerSide / viewportRatioLargerToSmaller,
                                        worldBorderMaxVisibleRadiusViewportSmallerSide);

  if (!zoomToFitBorderEdges && zoomToFitMapEdges)
  {
    const float borderToMapHorizontalMinDifference =
      min(worldRightBottom.x - worldBorderRightBottom.x, worldBorderLeftTop.x - worldLeftTop.x);
    const float borderToMapVerticalMinDifference =
      min(worldRightBottom.y - worldBorderRightBottom.y, worldBorderLeftTop.y - worldLeftTop.y);
    const float borderToMapLargerSideMinDifference =
      viewportWidth >= viewportHeight ? borderToMapHorizontalMinDifference : borderToMapVerticalMinDifference;
    const float borderToMapSmallerSideMinDifference =
      viewportWidth >= viewportHeight ? borderToMapVerticalMinDifference : borderToMapHorizontalMinDifference;
    const float worldMapMaxVisibleRadiusViewportLargerSide =
      worldBorderMaxVisibleRadiusViewportLargerSide + borderToMapLargerSideMinDifference;
    const float worldMapMaxVisibleRadiusViewportSmallerSide =
      worldBorderMaxVisibleRadiusViewportSmallerSide + borderToMapSmallerSideMinDifference;
    const float maxVisibleRadiusByClampMap =
      min(worldMapMaxVisibleRadiusViewportLargerSide / viewportRatioLargerToSmaller, worldMapMaxVisibleRadiusViewportSmallerSide);
    maxVisibleRadiusByClamp = min(maxVisibleRadiusByClamp, maxVisibleRadiusByClampMap);
  }

  worldVisibleRadiusRange.x = min(worldVisibleRadiusRangeDefault.x, maxVisibleRadiusByClamp);
  worldVisibleRadiusRange.y = min(worldVisibleRadiusRangeDefault.y, maxVisibleRadiusByClamp);
}

void TiledMapContext::setViewportSize(int width, int height)
{
  if ((viewportWidth == width && viewportHeight == height) || width == 0 || height == 0)
    return;
  setViewportSizeInner(width, height);

  if (isClampToBorder)
    clampToWorldBorder();
  setVisibleRadiusInner(getVisibleRadius());
  updateVisibleTiles();
}

void TiledMapContext::setWorldPos(const Point3 &pos)
{
  worldPos = pos;
  if (isClampToBorder)
    clampPosToWorldBorder();
  updateVisibleTiles();
}

void TiledMapContext::setWorldBorderInner(Point2 leftTop, Point2 rightBottom, bool clampToWorld)
{
  if (leftTop.x > rightBottom.x)
    eastl::swap(leftTop.x, rightBottom.x);
  if (leftTop.y > rightBottom.y)
    eastl::swap(leftTop.y, rightBottom.y);

  if (clampToWorld)
  {
    worldBorderLeftTop.x = max(leftTop.x, worldLeftTop.x);
    worldBorderLeftTop.y = max(leftTop.y, worldLeftTop.y);
    worldBorderRightBottom.x = min(rightBottom.x, worldRightBottom.x);
    worldBorderRightBottom.y = min(rightBottom.y, worldRightBottom.y);
  }
  else
  {
    worldBorderLeftTop = leftTop;
    worldBorderRightBottom = rightBottom;
  }

  worldBorderSize = ::abs(worldBorderRightBottom - worldBorderLeftTop);
}

void TiledMapContext::setWorldBorder(Point2 leftTop, Point2 rightBottom, bool clampToWorld)
{
  setWorldBorderInner(leftTop, rightBottom, clampToWorld);
  if (isClampToBorder)
  {
    clampToWorldBorder();
    updateVisibleTiles();
  }
}

void TiledMapContext::resetWorldBorder()
{
  worldBorderLeftTop = worldLeftTop;
  worldBorderRightBottom = worldRightBottom;
  worldBorderSize = worldSize;
  if (isClampToBorder)
  {
    clampToWorldBorder();
    updateVisibleTiles();
  }
}

int TiledMapContext::calcZoomLevel(float visibleRadius)
{
  int zlevel = 0;
  int minDim = min(viewportWidth, viewportHeight);
  if (minDim == 0 || tileWidth == 0)
    return zlevel;

  float resolution = 2 * visibleRadius / minDim; // [m/px]
  if (resolution == 0 || worldSize.x == 0)
    return zlevel;

  while (resolution < worldSize.x / (tileWidth * (1 << zlevel)) && zlevel < zlevels)
    zlevel++;
  return zlevel;
}

float TiledMapContext::calcVisibleRadius(int zoom)
{
  if (z < 0 || z > zlevels)
    return 0;
  int minDim = min(viewportWidth, viewportHeight);
  return getTileResolution(zoom) * minDim / 2;
}

void TiledMapContext::setVisibleRadiusInner(float r)
{
  float newRadiusViewportSmallerSide = clamp(r, worldVisibleRadiusRange.x, worldVisibleRadiusRange.y);
  float newRadiusViewportLargerSide = newRadiusViewportSmallerSide * viewportRatioLargerToSmaller;

  if (viewportWidth >= viewportHeight)
  {
    worldVisibleRadius = Point2(newRadiusViewportLargerSide, newRadiusViewportSmallerSide);
    viewportResolution = 2 * safediv(newRadiusViewportSmallerSide, viewportHeight);
  }
  else
  {
    worldVisibleRadius = Point2(newRadiusViewportSmallerSide, newRadiusViewportLargerSide);
    viewportResolution = 2 * safediv(newRadiusViewportSmallerSide, viewportWidth);
  }

  z = calcZoomLevel(newRadiusViewportSmallerSide);
}

float TiledMapContext::setVisibleRadius(float r)
{
  const float currentRadius = getVisibleRadius();
  if (r == currentRadius)
    return currentRadius;
  setVisibleRadiusInner(r);

  if (isClampToBorder)
    clampPosToWorldBorder();
  updateVisibleTiles();

  return getVisibleRadius();
}

void TiledMapContext::calcItmFromView(TMatrix &itm_out) const
{
  Point3 pos = worldPos;

  Point3 up(0, 1, 0);
  Point3 fwd(1, 0, 0);
  Point3 right(up % fwd); // world coordinates is left-handed but cross product is right-handed, so reverse the order

  itm_out.setcol(0, fwd);
  itm_out.setcol(1, up);
  itm_out.setcol(2, right);
  itm_out.setcol(3, pos);

  itm_out = itm_out * rotyTM(northAngle);
}

void TiledMapContext::calcTmFromView(TMatrix &tm_out) const
{
  TMatrix itm;
  calcItmFromView(itm);
  tm_out = inverse(itm);
}

Point3 TiledMapContext::mapToWorld(const Point2 &mapPos) const
{
  TMatrix itm;
  calcItmFromView(itm);
  return itm * Point3(mapPos.x * viewportResolution, 0, mapPos.y * viewportResolution);
}

Point2 TiledMapContext::worldToMap(const Point3 &pos) const
{
  if (viewportResolution == 0.f)
    return Point2(0, 0);
  TMatrix tm;
  calcTmFromView(tm);
  Point3 mapPos = tm * pos;
  return Point2(mapPos.x, mapPos.z) / viewportResolution;
}

Point2 TiledMapContext::worldToTc(const Point3 &pos) const
{
  if (viewportWidth == 0 || viewportHeight == 0)
    return Point2(0, 0);
  Point2 mapPos = worldToMap(pos);
  return Point2(mapPos.x / viewportWidth + 0.5f, -mapPos.y / viewportHeight + 0.5f);
}

Point2 TiledMapContext::worldToTc(const Point2 &pos) const { return worldToTc(Point3(pos.x, 0, pos.y)); }

bool fogAt(const eastl::vector<uint32_t> &data, Point2 lt, Point2 rb, float res, Point2 pos)
{
  int cols = int((rb.x - lt.x) / res);
  int rows = int((rb.y - lt.y) / res);
  int x = int((pos.x - lt.x) / res);
  int y = int((pos.y - lt.y) / res);
  if (x < 0 || x >= cols || y < 0 || y >= rows)
    return true;
  int idx = y * cols + x;
  int word = idx / 32;
  return (data[word] & (1 << (idx % 32))) != 0;
}

eastl::vector<uint32_t> migrateFogOfWarData(
  Point2 lt, Point2 rb, float res, const eastl::vector<uint32_t> &oldData, Point2 oldLt, Point2 oldRb, float oldRes)
{
  int cols = int((rb.x - lt.x) / res);
  int rows = int((rb.y - lt.y) / res);
  int wordCount = (cols * rows) / 32 + 1;
  Point2 center = Point2((lt.x + rb.x) / 2, (lt.y + rb.y) / 2);
  float zoneRadiusSq = sqr((rb.x - lt.x) / 2.0);

  eastl::vector<uint32_t> newData(wordCount, 0xFFFFFFFF); // all covered by fog
  for (int y = 0; y < rows; y++)
  {
    for (int x = 0; x < cols; x++)
    {
      Point2 pos = Point2(lt.x + x * res, lt.y + y * res);
      // check if point is inside the new fog area (circle)
      if (lengthSq(pos - center) > zoneRadiusSq)
        continue;
      if (!fogAt(oldData, oldLt, oldRb, oldRes, pos))
      {
        int idx = y * cols + x;
        int word = idx / 32;
        int bit = idx % 32;
        newData[word] &= ~(1 << bit);
      }
    }
  }
  return newData;
}

void TiledMapContext::setup(Sqrat::Object cfg)
{
  TILEDMAP_DEBUG("TiledMapContext::setup");

  if (dgs_get_settings()->getBool("generate_tiled_map", false))
    return;

  eastl::string newTilesPath = cfg.RawGetSlotValue<eastl::string>("tilesPath", "");
  if (newTilesPath != tilesPath)
    freeAllPictures();

  tilesPath = newTilesPath;
  if (!tilesPath.empty())
    canLoadTiles = true;

  worldLeftTop = cfg.RawGetSlotValue<Point2>("leftTop", Point2(0, 0));
  worldRightBottom = cfg.RawGetSlotValue<Point2>("rightBottom", Point2(0, 0));
  if (worldLeftTop.x > worldRightBottom.x)
    eastl::swap(worldLeftTop.x, worldRightBottom.x);
  if (worldLeftTop.y > worldRightBottom.y)
    eastl::swap(worldLeftTop.y, worldRightBottom.y);
  worldSize = ::abs(worldRightBottom - worldLeftTop);

  worldBorderLeftTop = cfg.RawGetSlotValue<Point2>("leftTopBorder", worldLeftTop);
  worldBorderRightBottom = cfg.RawGetSlotValue<Point2>("rightBottomBorder", worldRightBottom);
  if (worldBorderLeftTop == worldBorderRightBottom)
  {
    TILEDMAP_DEBUG("TiledMapContext: Border can't be one point! Use static map instead");
    worldBorderLeftTop = worldLeftTop;
    worldBorderRightBottom = worldRightBottom;
  }
  setWorldBorderInner(worldBorderLeftTop, worldBorderRightBottom, false);

  worldVisibleRadiusRange = cfg.RawGetSlotValue<Point2>("visibleRange", Point2(0, 1000));
  worldVisibleRadiusRangeDefault = worldVisibleRadiusRange;
  worldPos = cfg.RawGetSlotValue<Point3>("worldPos", Point3(0, 0, 0));
  northAngle = DegToRad(cfg.RawGetSlotValue<float>("northAngle", 0));

  viewportWidth = cfg.RawGetSlotValue<int>("viewportWidth", 0);
  viewportHeight = cfg.RawGetSlotValue<int>("viewportHeight", 0);
  setViewportSizeInner(viewportWidth, viewportHeight);

  zlevels = cfg.RawGetSlotValue<int>("zlevels", 0);
  tileWidth = cfg.RawGetSlotValue<int>("tileWidth", 0);
  if (zlevels >= 0)
  {
    tileWorldWidths.resize(zlevels + 1);
    tileResolutions.resize(zlevels + 1);
    for (int i = 0; i <= zlevels; ++i)
    {
      int n = 1 << i;
      tileWorldWidths[i] = (worldRightBottom.x - worldLeftTop.x) / n;
      tileResolutions[i] = tileWorldWidths[i] / tileWidth;
    }
  }
  else
  {
    tileWorldWidths.clear();
    tileResolutions.clear();
  }

  isViewCentered = cfg.RawGetSlotValue<bool>("isViewCentered", false);
  isClampToBorder = cfg.RawGetSlotValue<bool>("isClampToBorder", false);
  zoomToFitMapEdges = cfg.RawGetSlotValue<bool>("zoomToFitMapEdges", false);
  zoomToFitBorderEdges = cfg.RawGetSlotValue<bool>("zoomToFitBorderEdges", zoomToFitMapEdges);

  setVisibleRadiusInner((worldVisibleRadiusRange.y - worldVisibleRadiusRange.x) / 2);
  if (isClampToBorder)
    clampToWorldBorder();

  fogOfWarEnabled = cfg.RawGetSlotValue<bool>("fogOfWarEnabled", false);

  if (fogOfWarEnabled && viewportWidth > 0 && viewportHeight > 0)
  {
    int texcf = TEXFMT_R8 | TEXCF_RTARGET;
    fogOfWarTex = dag::create_tex(nullptr, viewportWidth, viewportHeight, texcf, 1, "tiled_map_fog_of_war_tex");
    fogOfWarSampler = get_texture_separate_sampler(fogOfWarTex.getTexId());

    if (!fogOfWarTex)
      logerr("%s: fogOfWarTex = false", __FUNCTION__);
  }

  if (fogOfWarEnabled)
  {
    fogOfWarLeftTop = cfg.RawGetSlotValue<Point2>("fogOfWarLeftTop", worldLeftTop);
    fogOfWarRightBottom = cfg.RawGetSlotValue<Point2>("fogOfWarRightBottom", worldRightBottom);
    fogOfWarResolution = cfg.RawGetSlotValue<float>("fogOfWarResolution", 1.0f);

    int fogOfWarCols = int((fogOfWarRightBottom.x - fogOfWarLeftTop.x) / fogOfWarResolution);
    int fogOfWarRows = int((fogOfWarRightBottom.y - fogOfWarLeftTop.y) / fogOfWarResolution);

    // shader have no idea about the world coordinates, so we need to pass the constraints in texture coordinates
    Point2 tc_lt = s_tiled_map_ctx->worldToTc(fogOfWarLeftTop);
    Point2 tc_rb = s_tiled_map_ctx->worldToTc(fogOfWarRightBottom);

    ShaderGlobal::set_float4(fog_of_war_constraints, tc_lt.x, tc_lt.y, tc_rb.x, tc_rb.y);
    ShaderGlobal::set_int(fog_of_war_width, fogOfWarCols);
    ShaderGlobal::set_int(fog_of_war_height, fogOfWarRows);
    int fogOfWarWords = (fogOfWarRows * fogOfWarCols) / 32 + 1;
    ShaderGlobal::set_buffer(fog_of_war_bitset, BAD_D3DRESID); // unbind before recreate to avoid race on cleanup
    fogOfWarBitsetSb = dag::buffers::create_persistent_sr_structured(sizeof(uint32_t), fogOfWarWords, "fog_of_war_bitset");
    ShaderGlobal::set_buffer(fog_of_war_bitset, fogOfWarBitsetSb); // another container for auto bind?

    Point2 fogOfWarOldLeftTop = cfg.RawGetSlotValue<Point2>("fogOfWarOldLeftTop", worldLeftTop);
    Point2 fogOfWarOldRightBottom = cfg.RawGetSlotValue<Point2>("fogOfWarOldRightBottom", worldRightBottom);
    float fogOfWarOldResolution = cfg.RawGetSlotValue<float>("fogOfWarOldResolution", 1.0f);
    eastl::string fogOfWarOldDataBase64 = cfg.RawGetSlotValue<eastl::string>("fogOfWarOldDataBase64", "");

    eastl::vector<uint32_t> oldData;
    eastl::vector<uint32_t> newData;

    // if we have no old data, just fill the new data
    if (fogOfWarOldDataBase64.empty())
    {
      newData = eastl::vector<uint32_t>(fogOfWarWords, 0xFFFFFFFF);
    }
    else
    {
      str_b64_to_data(oldData, fogOfWarOldDataBase64.c_str());
      newData = migrateFogOfWarData(fogOfWarLeftTop, fogOfWarRightBottom, fogOfWarResolution, oldData, fogOfWarOldLeftTop,
        fogOfWarOldRightBottom, fogOfWarOldResolution);
    }

    tiled_map_fog_of_war_set_data(newData);

    // force update the fog of war texture
    fogOfWarDataGen = 0;
    fogOfWarPrevDataGen = -1;

    bool res = s_tiled_map_ctx->fogOfWarBitsetSb->updateData(0, fogOfWarWords * sizeof(uint32_t), newData.data(), VBLOCK_WRITEONLY);
    if (!res)
      logerr("%s: fogOfWarBitsetSb->updateData failed", __FUNCTION__);
  }
  if (canLoadTiles && !tilesPath.empty())
    updateVisibleTiles();
}

void collectChildrenTiles(const QuadKey &quadKey, eastl::vector<QuadKey> &keepTiles, TilesHashMap &tilesHashMap, int zlevels)
{
  if (quadKey.size() >= zlevels)
    return;

  for (int i = 0; i < 4; ++i)
  {
    QuadKey childQuadKey = quadKey;
    childQuadKey.push_back('0' + i);
    if (hasLoadedTile(tilesHashMap, childQuadKey))
      keepTiles.push_back(childQuadKey);
    else
      collectChildrenTiles(childQuadKey, keepTiles, tilesHashMap, zlevels);
  }
}

void TiledMapContext::dispatchTiles(const eastl::vector<QuadKey> &requiredTiles,
  TilesHashMap &tilesHashMap,
  eastl::string prefix,
  PictureManager::async_load_done_cb_t cb)
{
  TIME_PROFILE(dispatchTiles);

  bool isFog = eastl::string_view(prefix).ends_with("/fog");

  eastl::vector<QuadKey> keepTiles;
  keepTiles.reserve(requiredTiles.size());
  for (const auto &quadKey : requiredTiles)
  {
    keepTiles.push_back(quadKey);
    if (hasLoadedTile(tilesHashMap, quadKey))
      continue;
    else if (eastl::find_if(requests.begin(), requests.end(), [&quadKey, isFog](TileAsyncLoadRequest *req) {
               return req->quadKey == quadKey && req->isFog == isFog && req->generation == s_tiled_map_ctx->tileLoadGeneration;
             }) != requests.end())
    {
      // with many calls to updateVisibleTiles() we can have tilesHashMap without the tile and pending request for the
      // same tile, so we need to repopulate the tileHashMap, the TileHandle here will be updated in the callback.
      TileHandle tile;
      tilesHashMap.insert_or_assign(quadKey, tile);
    }
    else
    {
      TileHandle tile;
      TileAsyncLoadRequest *req = new TileAsyncLoadRequest();
      req->quadKey = quadKey;
      req->isFog = isFog;
      req->generation = s_tiled_map_ctx->tileLoadGeneration;
      eastl::string filename =
        eastl::string(eastl::string::CtorSprintf{}, "%s/%s.avif", prefix.c_str(), quadKey.empty() ? "combined" : quadKey.c_str());
      int sync =
        PictureManager::get_picture_ex(filename.c_str(), tile.picId, tile.texId, tile.smpId, nullptr, nullptr, nullptr, cb, req);
      if (sync)
      {
        TILEDMAP_DEBUG("TiledMapContext: sync load tile %s", quadKey.c_str());
        delete req;
        tilesHashMap.insert_or_assign(quadKey, tile);
        continue;
      }
      else
      {
        TILEDMAP_DEBUG("TiledMapContext: async load tile request %s", quadKey.c_str());
        requests.insert(req);
        tilesHashMap.insert_or_assign(quadKey, tile);
      }
    }

    // inspect parent tiles
    bool found = false;
    for (int i = quadKey.size() - 1; i > 0; --i)
    {
      QuadKey parentQuadKey(quadKey.begin(), quadKey.begin() + i);
      if (hasLoadedTile(tilesHashMap, parentQuadKey))
      {
        found = true;
        TILEDMAP_DEBUG("TiledMapContext: keep parent tile %s", parentQuadKey.c_str());
        keepTiles.push_back(parentQuadKey);
        break;
      }
    }
    if (found)
      continue;

    // inspect children tiles
    collectChildrenTiles(quadKey, keepTiles, tilesHashMap, zlevels);
  }

  eastl::vector<QuadKey> removeTiles;
  for (const auto tile : tilesHashMap)
    if (eastl::find(keepTiles.begin(), keepTiles.end(), tile.first) == keepTiles.end())
      removeTiles.push_back(tile.first);

  for (const auto &tile : removeTiles)
  {
    TILEDMAP_DEBUG("TiledMapContext: dispatchTiles: remove tile %s", tile.c_str());
    releasePicTex(tilesHashMap[tile].picId, tilesHashMap[tile].texId);
    tilesHashMap.erase(tile);
  }
}

void TiledMapContext::updateVisibleTiles()
{
  TIME_PROFILE(updateVisibleTiles);

  if (!canLoadTiles || tilesPath.empty())
    return;
  TILEDMAP_DEBUG("\nTiledMapContext: updateVisibleTiles: current zlvl: %d", z);

  float viewportWidthExpanded = viewportWidth * 1.5;
  float viewportHeightExpanded = viewportHeight * 1.5;
  // note: mapToWorld transforms from right-handed to left-handed coordinate system so lt becomes lb and rb becomes rt
  Point3 lb = mapToWorld(Point2(-viewportWidthExpanded / 2, -viewportHeightExpanded / 2)) - Point3(worldLeftTop.x, 0, worldLeftTop.y);
  Point3 rt = mapToWorld(Point2(viewportWidthExpanded / 2, viewportHeightExpanded / 2)) - Point3(worldLeftTop.x, 0, worldLeftTop.y);

  eastl::vector<QuadKey> requiredTiles;
  requiredTiles.reserve(tiles.size());

  float tileWorldWidth = getTileWorldWidth(z);
  IPoint2 lt_idx = IPoint2(floor(lb.x / tileWorldWidth), floor(rt.z / tileWorldWidth));
  IPoint2 rb_idx = IPoint2(floor(rt.x / tileWorldWidth), floor(lb.z / tileWorldWidth));
  int n = 1 << z;
  lt_idx = clamp(lt_idx, IPoint2(0, 0), IPoint2(n - 1, n - 1));
  rb_idx = clamp(rb_idx, IPoint2(0, 0), IPoint2(n - 1, n - 1));

  for (int i = lt_idx.x; i <= rb_idx.x; i++)
  {
    for (int j = lt_idx.y; j <= rb_idx.y; j++)
    {
      QuadKey quadKey = tileXYToQuadKey(i, n - 1 - j, z);
      requiredTiles.push_back(quadKey);
    }
  }

  TILEDMAP_DEBUG("TiledMapContext: before dispatch tiles: %d, fogTiles: %d", tiles.size(), fogTiles.size());

  if (fogOfWarEnabled)
    dispatchTiles(requiredTiles, fogTiles, tilesPath + "/fog", load_fog_tiles_cb);
  dispatchTiles(requiredTiles, tiles, tilesPath, load_current_zlvl_tiles_cb);

  TILEDMAP_DEBUG("TiledMapContext: requiredTiles: %d", requiredTiles.size());
  TILEDMAP_DEBUG("TiledMapContext: after dispatch tiles: %d, fogTiles: %d", tiles.size(), fogTiles.size());

  lastWorldPosAfterVisibleTilesUpdate = worldPos;
}

TiledMapContext *TiledMapContext::get_from_element(const darg::Element *elem)
{
  auto strings = ui_strings.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, nullptr);

  return elem->props.scriptDesc.RawGetSlotValue<TiledMapContext *>(strings->tiledMapContext, nullptr);
}

void TiledMapContext::clampPosToWorldBorder()
{
  if (worldBorderSize.x > 2 * worldVisibleRadius.x)
    worldPos.x = ::clamp(worldPos.x, worldBorderLeftTop.x + worldVisibleRadius.x, worldBorderRightBottom.x - worldVisibleRadius.x);
  else
    worldPos.x = worldBorderLeftTop.x + (worldBorderSize.x / 2);

  if (worldBorderSize.y > 2 * worldVisibleRadius.y)
    worldPos.z = ::clamp(worldPos.z, worldBorderLeftTop.y + worldVisibleRadius.y, worldBorderRightBottom.y - worldVisibleRadius.y);
  else
    worldPos.z = worldBorderLeftTop.y + (worldBorderSize.y / 2);
}

eastl::string TiledMapContext::getFogOfWarBase64() const
{
  eastl::string fogOfWarDataBase64;
  eastl::vector<uint32_t> data = tiled_map_fog_of_war_get_data();
  if (data.empty())
    return fogOfWarDataBase64;

  Base64 b64;
  b64.encode((uint8_t *)data.data(), data.size() * sizeof(uint32_t));
  fogOfWarDataBase64 = b64.c_str();
  return fogOfWarDataBase64;
}

void tiled_map_on_render_ui(const RenderEventUI &evt)
{
  if (s_tiled_map_ctx)
  {
    const TMatrix &viewItm = evt.get<1>();
    float wk = evt.get<3>().wk;

    s_tiled_map_ctx->curViewItm = viewItm;
    s_tiled_map_ctx->perspWk = wk;

    if (s_tiled_map_ctx->isViewCentered)
    {
      s_tiled_map_ctx->setWorldPos(viewItm.getcol(3));
    }
  }
}

void tiled_map_fog_of_war_update_data(
  const UpdateStageInfoBeforeRender &evt, const ecs::IntList &fog_of_war__data, const int fog_of_war__dataGen)
{
  G_UNUSED(evt);

  if (!s_tiled_map_ctx || !s_tiled_map_ctx->fogOfWarEnabled)
    return;

  s_tiled_map_ctx->fogOfWarDataGen = fog_of_war__dataGen;

  // skip update if data is the same
  if (s_tiled_map_ctx->fogOfWarDataGen == s_tiled_map_ctx->fogOfWarPrevDataGen)
    return;

  int fogOfWarCols =
    int((s_tiled_map_ctx->fogOfWarRightBottom.x - s_tiled_map_ctx->fogOfWarLeftTop.x) / s_tiled_map_ctx->fogOfWarResolution);
  int fogOfWarRows =
    int((s_tiled_map_ctx->fogOfWarRightBottom.y - s_tiled_map_ctx->fogOfWarLeftTop.y) / s_tiled_map_ctx->fogOfWarResolution);

  int words = fogOfWarRows * fogOfWarCols / 32 + 1;
  if (fog_of_war__data.size() == words && fog_of_war__dataGen != s_tiled_map_ctx->fogOfWarPrevDataGen)
  {
    bool res = s_tiled_map_ctx->fogOfWarBitsetSb->updateData(0, words * sizeof(uint32_t), fog_of_war__data.data(), VBLOCK_WRITEONLY);
    if (!res)
      logerr("%s: fogOfWarBitsetSb->updateData failed", __FUNCTION__);
  }
}

void tiled_map_fog_of_war_before_render(const UpdateStageInfoBeforeRender &evt)
{
  G_UNUSED(evt);

  if (!s_tiled_map_ctx || !s_tiled_map_ctx->fogOfWarEnabled)
    return;

  const int visibleRadius = s_tiled_map_ctx->getVisibleRadius();
  const Point3 worldPos = s_tiled_map_ctx->getWorldPos();

  // skip update if data is the same
  if (s_tiled_map_ctx->fogOfWarDataGen == s_tiled_map_ctx->fogOfWarPrevDataGen &&
      visibleRadius == s_tiled_map_ctx->fogOfWarPrevVisibleRadius && worldPos == s_tiled_map_ctx->fogOfWarPrevWorldPos)
    return;

  ScopeRenderTarget scopeRT;
  ScopeResetShaderBlocks scopedNoBlocks;

  Point2 tc_lt = s_tiled_map_ctx->worldToTc(s_tiled_map_ctx->fogOfWarLeftTop);
  Point2 tc_rb = s_tiled_map_ctx->worldToTc(s_tiled_map_ctx->fogOfWarRightBottom);

  ShaderGlobal::set_float4(fog_of_war_constraints, tc_lt.x, tc_lt.y, tc_rb.x, tc_rb.y);

  d3d::set_render_target(s_tiled_map_ctx->fogOfWarTex.getBaseTex(), 0);
  s_tiled_map_ctx->fogOfWarShader.render();

  s_tiled_map_ctx->fogOfWarPrevVisibleRadius = visibleRadius;
  s_tiled_map_ctx->fogOfWarPrevWorldPos = worldPos;
  s_tiled_map_ctx->fogOfWarPrevDataGen = s_tiled_map_ctx->fogOfWarDataGen;
}

// note: if fog_of_war provided via config and not connected to ecs entity, force darg to setup new map config.
void tiled_map_fog_of_war_after_reset()
{
  s_tiled_map_ctx->fogOfWarDataGen = 1;
  s_tiled_map_ctx->fogOfWarPrevDataGen = 0;
}

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_tiled_map_classes, "tiledMap", sq::VM_UI_ALL)
{
  Sqrat::Table exports(vm);

  Sqrat::Class<TiledMapContext, Sqrat::NoCopy<TiledMapContext>> sqTiledMapContext(vm, "TiledMapContext");
  sqTiledMapContext //
    .Func("setup", &TiledMapContext::setup)
    .Func("getVisibleRadiusRange", &TiledMapContext::getVisibleRadiusRange)
    .Func("getVisibleRadiusWidth", &TiledMapContext::getVisibleRadiusWidth)
    .Func("getVisibleRadiusHeight", &TiledMapContext::getVisibleRadiusHeight)
    .Func("getVisibleRadius", &TiledMapContext::getVisibleRadius)
    .Func("setVisibleRadius", &TiledMapContext::setVisibleRadius)
    .Func("mapToWorld", &TiledMapContext::mapToWorld)
    .Func("worldToMap", &TiledMapContext::worldToMap)
    .Func("setWorldPos", &TiledMapContext::setWorldPos)
    .Func("setViewportSize", &TiledMapContext::setViewportSize)
    .Func("setWorldBorder", &TiledMapContext::setWorldBorder)
    .Func("resetWorldBorder", &TiledMapContext::resetWorldBorder)
    .Func("getViewCentered", &TiledMapContext::getViewCentered)
    .Func("setViewCentered", &TiledMapContext::setViewCentered)
    .Func("getFogOfWarBase64", &TiledMapContext::getFogOfWarBase64)
    .Func("toggleFogOfWar", &TiledMapContext::toggleFogOfWar)
    /**/;

  exports.Bind("TiledMapContext", sqTiledMapContext);
  return exports;
}
