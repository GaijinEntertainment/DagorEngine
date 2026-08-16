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
#include <ioSys/dag_zstdIo.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <ui/scriptStrings.h>
#include <util/dag_base64.h>
#include <util/dag_threadPool.h>
#include <util/dag_delayedAction.h>
#include <osApiWrappers/dag_atomic.h>
#include <quirrel/sqEventBus/sqEventBus.h>


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

// 4M words = 128M cells; covers any plausible map while preventing huge allocations by mistake in config.
static constexpr int64_t FOG_OF_WAR_MAX_WORDS = 4 << 20;

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

static bool load_tile_confirm_cb(void *arg)
{
  if (!s_tiled_map_ctx || !arg)
    return false;
  auto req = (TileAsyncLoadRequest *)arg;
  if (req->generation != s_tiled_map_ctx->tileLoadGeneration)
    return false;
  return true;
}
static void load_tiles_cb(
  PICTUREID pid, TEXTUREID tid, d3d::SamplerHandle smp, const Point2 *tcLt, const Point2 *tcRb, const Point2 *picture_sz, void *arg)
{
  G_UNUSED(tcLt);
  G_UNUSED(tcRb);
  G_UNUSED(picture_sz);

  if (!s_tiled_map_ctx || !arg)
    return;
  // load_tile_cb(pid, tid, smp, arg, s_tiled_map_ctx->tiles);
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

  auto it = s_tiled_map_ctx->tiles.find(req->quadKey);
  if (it == s_tiled_map_ctx->tiles.end())
  {
    releasePicTex(pid, tid);
    TILEDMAP_DEBUG("TiledMapContext: load_tile_cb: %s tile not needed anymore", req->quadKey);
  }
  else
  {
    s_tiled_map_ctx->tiles[req->quadKey].picId = pid;
    s_tiled_map_ctx->tiles[req->quadKey].texId = tid;
    s_tiled_map_ctx->tiles[req->quadKey].smpId = smp;
    TILEDMAP_DEBUG("TiledMapContext: load_tile_cb: %s tile loaded", req->quadKey);
  }

  // remove outdated children in quadtree
  eastl::vector<QuadKey> removeTiles;
  for (auto &tile : s_tiled_map_ctx->tiles)
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
    s_tiled_map_ctx->tiles.erase(quadKey);
  }

  requests.erase(req);
  delete req;
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

TiledMapContext &TiledMapContext::operator=(TiledMapContext &&other) = default;

void TiledMapContext::freeAllPictures()
{
  TILEDMAP_DEBUG("TiledMapContext::freeAllPictures");
  if (!s_tiled_map_ctx)
    return;
  s_tiled_map_ctx->canLoadTiles = false;
  s_tiled_map_ctx->tileLoadGeneration++;
  for (auto &tile : s_tiled_map_ctx->tiles)
    releasePicTex(tile.second.picId, tile.second.texId);
  s_tiled_map_ctx->tiles.clear();
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

  eastl::vector<uint32_t> newData(wordCount, 0xFFFFFFFF);
  if (cols <= 0 || rows <= 0 || oldData.empty())
    return newData;

  for (int y = 0; y < rows; y++)
  {
    for (int x = 0; x < cols; x++)
    {
      Point2 pos = Point2(lt.x + x * res, lt.y + y * res);
      if (!fogAt(oldData, oldLt, oldRb, oldRes, pos))
      {
        int idx = y * cols + x;
        newData[idx / 32] &= ~(1u << (idx % 32));
      }
    }
  }
  return newData;
}

static eastl::string data_to_compressed_b64(const eastl::vector<uint32_t> &data)
{
  if (data.empty())
    return {};

  TIME_PROFILE(tiled_map_fog_of_war_compress);
  const size_t srcSize = data.size() * sizeof(uint32_t);
  const size_t bound = zstd_compress_bound(srcSize);
  eastl::vector<uint8_t> compressed(bound);
  const size_t cSize = zstd_compress(compressed.data(), bound, data.data(), srcSize);
  if (cSize > bound)
    return {};

  Base64 b64;
  b64.encode(compressed.data(), (int)cSize);
  return eastl::string(b64.c_str());
}

// Async fog-of-war compression. Fog data can reach FOG_OF_WAR_MAX_WORDS (16 MB), so zstd+base64
// is offloaded from the main thread. The single job is reused: a request arriving while one is
// still queued/running is dropped and re-issued by the script-side save throttle. The compressed
// result is handed back to the UI VM via sqeventbus on the main thread (VM and eventbus are not
// thread-safe), so no C++ owns a Quirrel callback across the async boundary.
static const char *FOG_OF_WAR_COMPRESSED_EVENT = "tiledMap.fogOfWarCompressed";

struct FogOfWarCompressJob final : public cpujobs::IJob
{
  eastl::vector<uint32_t> data;
  eastl::string savePath;

  const char *getJobName(bool &copy_str) const override
  {
    copy_str = false;
    return "FogOfWarCompressJob";
  }

  void doJob() override
  {
    TIME_PROFILE(tiled_map_fog_of_war_compress_job);
    run_action_on_main_thread([path = savePath, b64c = data_to_compressed_b64(data)]() {
      sqeventbus::write_event_main_thread(FOG_OF_WAR_COMPRESSED_EVENT, [&](sqeventbus::Value &v) {
        v["path"] = path.c_str();
        v["b64c"] = b64c.c_str();
      });
    });
  }
};

static FogOfWarCompressJob s_fog_of_war_compress_job;

static eastl::vector<uint32_t> compressed_b64_to_data(const char *b64c, int expectedWords)
{
  if (!b64c || !*b64c || expectedWords <= 0)
    return {};

  eastl::vector<uint8_t> compressed;
  str_b64_to_data(compressed, b64c);
  if (compressed.empty())
    return {};

  eastl::vector<uint32_t> out(expectedWords, 0);
  const size_t dstSize = (size_t)expectedWords * sizeof(uint32_t);
  const size_t dSize = zstd_decompress(out.data(), dstSize, compressed.data(), compressed.size());
  if (dSize != dstSize)
    return {};
  return out;
}

struct FogOfWarSource
{
  eastl::vector<uint32_t> data;
  Point2 leftTop;
  Point2 rightBottom;
  float resolution;
};

// Blend a list of legacy per-scene fog sources into a target frame.
// A cell in the target is CLEARED if ANY source has that world position CLEARED.
static eastl::vector<uint32_t> blendFogOfWarSources(Point2 lt, Point2 rb, float res, const eastl::vector<FogOfWarSource> &sources)
{
  const int64_t cols = int64_t((rb.x - lt.x) / res);
  const int64_t rows = int64_t((rb.y - lt.y) / res);
  if (cols <= 0 || rows <= 0 || (rows * cols) / 32 + 1 > FOG_OF_WAR_MAX_WORDS)
    return eastl::vector<uint32_t>(1, 0xFFFFFFFF);

  const int64_t wordCount = (cols * rows) / 32 + 1;
  eastl::vector<uint32_t> newData(wordCount, 0xFFFFFFFF);
  if (sources.empty())
    return newData;

  for (int64_t y = 0; y < rows; y++)
  {
    for (int64_t x = 0; x < cols; x++)
    {
      Point2 pos = Point2(lt.x + x * res, lt.y + y * res);
      for (const FogOfWarSource &s : sources)
      {
        if (!fogAt(s.data, s.leftTop, s.rightBottom, s.resolution, pos))
        {
          int64_t idx = y * cols + x;
          newData[idx / 32] &= ~(1u << (idx % 32));
          break;
        }
      }
    }
  }
  return newData;
}

static SQInteger blend_and_compress_fog_of_war_sq(HSQUIRRELVM vm)
{
  Sqrat::Var<Sqrat::Array> sourcesVar(vm, 2);
  Sqrat::Var<Point2> newLTVar(vm, 3);
  Sqrat::Var<Point2> newRBVar(vm, 4);
  Sqrat::Var<float> newResVar(vm, 5);

  Sqrat::Array sources = sourcesVar.value;
  const Point2 newLT = newLTVar.value;
  const Point2 newRB = newRBVar.value;
  const float newRes = newResVar.value;

  if (newRes <= 0.f || newLT.x >= newRB.x || newLT.y >= newRB.y)
  {
    logerr("fog_of_war: invalid target frame for blend_and_compress");
    sq_pushstring(vm, "", 0);
    return 1;
  }

  const int64_t newCols = int64_t((newRB.x - newLT.x) / newRes);
  const int64_t newRows = int64_t((newRB.y - newLT.y) / newRes);
  if (newCols <= 0 || newRows <= 0 || (newRows * newCols) / 32 + 1 > FOG_OF_WAR_MAX_WORDS)
  {
    logerr("fog_of_war: target frame too large for blend_and_compress");
    sq_pushstring(vm, "", 0);
    return 1;
  }

  eastl::vector<FogOfWarSource> parsed;
  parsed.reserve(sources.Length());
  for (SQInteger i = 0; i < sources.Length(); ++i)
  {
    Sqrat::Table src = sources.GetValue<Sqrat::Table>(SQInteger(i));
    if (src.GetType() != OT_TABLE)
      continue;
    eastl::string b64 = src.GetSlotValue<eastl::string>("b64", eastl::string());
    Point2 lt = src.GetSlotValue<Point2>("leftTop", Point2(0, 0));
    Point2 rb = src.GetSlotValue<Point2>("rightBottom", Point2(0, 0));
    float res = src.GetSlotValue<float>("resolution", 0.f);
    if (b64.empty() || res <= 0.f || lt.x >= rb.x || lt.y >= rb.y)
      continue;

    FogOfWarSource s;
    str_b64_to_data(s.data, b64.c_str());
    if (s.data.empty())
      continue;
    const int64_t srcCols = int64_t((rb.x - lt.x) / res);
    const int64_t srcRows = int64_t((rb.y - lt.y) / res);
    const int64_t expectedWords = (srcCols * srcRows) / 32 + 1;
    if (srcCols <= 0 || srcRows <= 0 || expectedWords > FOG_OF_WAR_MAX_WORDS || s.data.size() != static_cast<size_t>(expectedWords))
      continue;
    s.leftTop = lt;
    s.rightBottom = rb;
    s.resolution = res;
    parsed.push_back(eastl::move(s));
  }

  if (parsed.empty())
  {
    sq_pushstring(vm, "", 0);
    return 1;
  }

  eastl::vector<uint32_t> blended = blendFogOfWarSources(newLT, newRB, newRes, parsed);
  eastl::string outB64c = data_to_compressed_b64(blended);
  sq_pushstring(vm, outB64c.c_str(), (SQInteger)outB64c.size());
  return 1;
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

    fogOfWarTexInited = false;
    if (!fogOfWarTex)
      logerr("%s: fogOfWarTex = false", __FUNCTION__);

    if (!fogOfWarTileShader.inited)
      if (!fogOfWarTileShader.init("fog_of_war_tile", false))
        logerr("%s: failed to init `fog_of_war_tile` shader", __FUNCTION__);
  }

  if (fogOfWarEnabled)
  {
    fogOfWarLeftTop = cfg.RawGetSlotValue<Point2>("fogOfWarLeftTop", worldLeftTop);
    fogOfWarRightBottom = cfg.RawGetSlotValue<Point2>("fogOfWarRightBottom", worldRightBottom);
    fogOfWarResolution = cfg.RawGetSlotValue<float>("fogOfWarResolution", 1.0f);

    const int64_t fogOfWarCols64 =
      fogOfWarResolution > 0.f ? int64_t((fogOfWarRightBottom.x - fogOfWarLeftTop.x) / fogOfWarResolution) : 0;
    const int64_t fogOfWarRows64 =
      fogOfWarResolution > 0.f ? int64_t((fogOfWarRightBottom.y - fogOfWarLeftTop.y) / fogOfWarResolution) : 0;
    if (fogOfWarCols64 <= 0 || fogOfWarRows64 <= 0 || (fogOfWarRows64 * fogOfWarCols64) / 32 + 1 > FOG_OF_WAR_MAX_WORDS)
    {
      logerr("%s: fog-of-war setup out of range (cols=%lld rows=%lld res=%g), disabling", __FUNCTION__, (long long)fogOfWarCols64,
        (long long)fogOfWarRows64, fogOfWarResolution);
      fogOfWarEnabled = false;
    }
    else
    {
      int fogOfWarCols = (int)fogOfWarCols64;
      int fogOfWarRows = (int)fogOfWarRows64;

      // shader have no idea about the world coordinates, so we need to pass the constraints in texture coordinates
      Point2 tc_lt = s_tiled_map_ctx->worldToTc(fogOfWarLeftTop);
      Point2 tc_rb = s_tiled_map_ctx->worldToTc(fogOfWarRightBottom);

      ShaderGlobal::set_float4(fog_of_war_constraints, tc_lt.x, tc_lt.y, tc_rb.x, tc_rb.y);
      ShaderGlobal::set_int(fog_of_war_width, fogOfWarCols);
      ShaderGlobal::set_int(fog_of_war_height, fogOfWarRows);
      int fogOfWarWords = (int)((fogOfWarRows64 * fogOfWarCols64) / 32 + 1);
      ShaderGlobal::set_buffer(fog_of_war_bitset, BAD_D3DRESID); // unbind before recreate to avoid race on cleanup
      fogOfWarBitsetSb = dag::buffers::create_persistent_sr_structured(sizeof(uint32_t), fogOfWarWords, "fog_of_war_bitset");
      ShaderGlobal::set_buffer(fog_of_war_bitset, fogOfWarBitsetSb); // another container for auto bind?

      Point2 fogOfWarOldLeftTop = cfg.RawGetSlotValue<Point2>("fogOfWarOldLeftTop", worldLeftTop);
      Point2 fogOfWarOldRightBottom = cfg.RawGetSlotValue<Point2>("fogOfWarOldRightBottom", worldRightBottom);
      float fogOfWarOldResolution = cfg.RawGetSlotValue<float>("fogOfWarOldResolution", 1.0f);
      eastl::string fogOfWarOldDataBase64 = cfg.RawGetSlotValue<eastl::string>("fogOfWarOldDataBase64", "");
      eastl::string fogOfWarOldDataBase64Compressed = cfg.RawGetSlotValue<eastl::string>("fogOfWarOldDataBase64Compressed", "");

      eastl::vector<uint32_t> oldData;
      eastl::vector<uint32_t> newData;

      if (!fogOfWarOldDataBase64Compressed.empty())
      {
        int64_t oldCols = 0, oldRows = 0;
        if (fogOfWarOldResolution > 0.f)
        {
          oldCols = int64_t((fogOfWarOldRightBottom.x - fogOfWarOldLeftTop.x) / fogOfWarOldResolution);
          oldRows = int64_t((fogOfWarOldRightBottom.y - fogOfWarOldLeftTop.y) / fogOfWarOldResolution);
        }
        const int64_t oldWords = (oldRows * oldCols) / 32 + 1;
        if (oldCols <= 0 || oldRows <= 0 || oldWords > FOG_OF_WAR_MAX_WORDS)
        {
          logerr("%s: saved fog-of-war metadata out of range (cols=%lld rows=%lld res=%g), skipping restore", __FUNCTION__,
            (long long)oldCols, (long long)oldRows, fogOfWarOldResolution);
        }
        else
        {
          oldData = compressed_b64_to_data(fogOfWarOldDataBase64Compressed.c_str(), (int)oldWords);
          if (oldData.empty())
            logerr("%s: failed to decompress saved fog-of-war data, fog will be reset", __FUNCTION__);
        }
      }
      else if (!fogOfWarOldDataBase64.empty())
      {
        str_b64_to_data(oldData, fogOfWarOldDataBase64.c_str());
      }

      if (oldData.empty())
      {
        newData = eastl::vector<uint32_t>(fogOfWarWords, 0xFFFFFFFF);
      }
      else
      {
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
  const eastl::string &prefix,
  int tileZLevel,
  PictureManager::async_load_done_cb_t cb)
{
  TIME_PROFILE(dispatchTiles);

  eastl::vector<QuadKey> keepTiles;
  keepTiles.reserve(requiredTiles.size());
  for (const auto &quadKey : requiredTiles)
  {
    keepTiles.push_back(quadKey);
    if (hasLoadedTile(tilesHashMap, quadKey))
      continue;
    else if (eastl::find_if(requests.begin(), requests.end(), [&quadKey](TileAsyncLoadRequest *req) {
               return req->quadKey == quadKey && req->generation == s_tiled_map_ctx->tileLoadGeneration;
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
      req->generation = s_tiled_map_ctx->tileLoadGeneration;
      eastl::string filename =
        eastl::string(eastl::string::CtorSprintf{}, "%s/%s.avif", prefix.c_str(), quadKey.empty() ? "combined" : quadKey.c_str());
      int sync = PictureManager::get_picture_ex(filename.c_str(), tile.picId, tile.texId, tile.smpId, nullptr, nullptr, nullptr, cb,
        req, load_tile_confirm_cb);
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
    collectChildrenTiles(quadKey, keepTiles, tilesHashMap, tileZLevel);
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

    eastl::find_if(requests.begin(), requests.end(), [&tile](TileAsyncLoadRequest *req) {
      if (req->quadKey != tile || req->generation != s_tiled_map_ctx->tileLoadGeneration)
        return false;
      req->generation = s_tiled_map_ctx->tileLoadGeneration - 1; //~0u;
      return true;
    });
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

  auto buildRequiredForZoom = [&](int zoom, eastl::vector<QuadKey> &out) {
    float tileW = getTileWorldWidth(zoom);
    if (tileW <= 0.f)
      return;
    IPoint2 lt_idx = IPoint2(floor(lb.x / tileW), floor(rt.z / tileW));
    IPoint2 rb_idx = IPoint2(floor(rt.x / tileW), floor(lb.z / tileW));
    int n = 1 << zoom;
    lt_idx = clamp(lt_idx, IPoint2(0, 0), IPoint2(n - 1, n - 1));
    rb_idx = clamp(rb_idx, IPoint2(0, 0), IPoint2(n - 1, n - 1));
    for (int i = lt_idx.x; i <= rb_idx.x; i++)
      for (int j = lt_idx.y; j <= rb_idx.y; j++)
        out.push_back(tileXYToQuadKey(i, n - 1 - j, zoom));
  };

  eastl::vector<QuadKey> requiredTiles;
  requiredTiles.reserve(tiles.size());
  buildRequiredForZoom(z, requiredTiles);
  // zlevel 0 ("combined.avif") is always kept as a background while higher-zoom tiles are loading.
  if (z > 0)
    requiredTiles.push_back(QuadKey());

  TILEDMAP_DEBUG("TiledMapContext: before dispatch tiles: %d", tiles.size());
  dispatchTiles(requiredTiles, tiles, tilesPath, z, load_tiles_cb);
  TILEDMAP_DEBUG("TiledMapContext: requiredTiles: %d", requiredTiles.size());
  TILEDMAP_DEBUG("TiledMapContext: after dispatch tiles: %d", tiles.size());

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

eastl::string TiledMapContext::getFogOfWarBase64Compressed() const
{
  eastl::vector<uint32_t> data = tiled_map_fog_of_war_get_data();
  return data_to_compressed_b64(data);
}

void TiledMapContext::requestFogOfWarBase64Compressed(const char *save_path) const
{
  TIME_PROFILE(tiled_map_fog_of_war_request_compress);
  // Skip while a compression is still queued or running (done is set by the worker on completion).
  // Overlap is rare given the script-side throttle; the dropped request is re-issued on the next tick.
  if (!interlocked_acquire_load(s_fog_of_war_compress_job.done))
    return;

  s_fog_of_war_compress_job.data = tiled_map_fog_of_war_get_data();
  if (s_fog_of_war_compress_job.data.empty())
    return;
  s_fog_of_war_compress_job.savePath = save_path ? save_path : "";

  threadpool::add(&s_fog_of_war_compress_job, threadpool::PRIO_LOW);
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

void tiled_map_fog_of_war_render_ui(const RenderEventUI &evt)
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

  d3d::set_render_target({}, DepthAccess::RW, {{s_tiled_map_ctx->fogOfWarTex.getBaseTex(), 0, 0}});
  s_tiled_map_ctx->fogOfWarShader.render();
  s_tiled_map_ctx->fogOfWarTexInited = true;

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
    .Func("getWorldPos", &TiledMapContext::getWorldPos)
    .Func("setViewportSize", &TiledMapContext::setViewportSize)
    .Func("setWorldBorder", &TiledMapContext::setWorldBorder)
    .Func("resetWorldBorder", &TiledMapContext::resetWorldBorder)
    .Func("getViewCentered", &TiledMapContext::getViewCentered)
    .Func("setViewCentered", &TiledMapContext::setViewCentered)
    .Func("getFogOfWarBase64", &TiledMapContext::getFogOfWarBase64)
    .Func("getFogOfWarBase64Compressed", &TiledMapContext::getFogOfWarBase64Compressed)
    .Func("requestFogOfWarBase64Compressed", &TiledMapContext::requestFogOfWarBase64Compressed)
    .Func("toggleFogOfWar", &TiledMapContext::toggleFogOfWar)
    /**/;

  exports.Bind("TiledMapContext", sqTiledMapContext);
  exports.SquirrelFuncDeclString(blend_and_compress_fog_of_war_sq,
    "blend_and_compress_fog_of_war(sources: array, newLT: instance, newRB: instance, newRes: float): string");
  return exports;
}
