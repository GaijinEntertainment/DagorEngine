// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/clipMap.h>
#include <landMesh/lmeshRenderer.h>
#include <landMesh/riLandClass.h>
#include <landMesh/heightmapQuery.h>
#include <math/dag_Point4.h>
#include <math/dag_Point2.h>
#include <math/dag_adjpow2.h>
#include <math/dag_hlsl_floatx.h>
#include <math/integer/dag_IBBox2.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_convar.h>
#include <util/dag_stlqsort.h>
#include <util/dag_string.h>
#include <util/dag_console.h>
#include <util/dag_threadPool.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <workCycle/dag_workCycle.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/vector_set.h>
#include <EASTL/vector_map.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include <EASTL/span.h>
#include <EASTL/numeric.h>
#include <EASTL/bitset.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_enumerate.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <memory/dag_framemem.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_dynShaderBuf.h>
#include <shaders/dag_computeShaders.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_renderStates.h>
#include <3d/dag_ringDynBuf.h>
#include <3d/dag_eventQueryHolder.h>
#include <3d/dag_lockTexture.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_render.h>
#include <render/dxtcompress.h>
#include <render/bcCompressor.h>

#define USE_VIRTUAL_TEXTURE 1

#include "landMesh/vtex.hlsli"
static constexpr float TILE_WIDTH_F = TILE_WIDTH; // float version to avoid static analyzer warnings

static constexpr int TERRAIN_RI_OFFSET = 0;
static constexpr int RI_INVALID_ID = -2; // -1 is invalid source RI, -2 is invalid target RI, they can't match

#include "clipmapDebug.h"


#define VTEX_CONST_LIST VAR(uav_feedback_const_no)

#define VAR(a) static int a = -1;
VTEX_CONST_LIST
#undef VAR

static void init_shader_vars()
{
#define VAR(a)                             \
  {                                        \
    int tmp = get_shader_variable_id(#a);  \
    if (tmp >= 0)                          \
      a = ShaderGlobal::get_int_fast(tmp); \
  }
  VTEX_CONST_LIST
#undef VAR
}

namespace var
{
static ShaderVariableInfo supports_uav("supports_uav", true);
static ShaderVariableInfo requires_uav("requires_uav", true);
static ShaderVariableInfo clipmap_use_ri_vtex("clipmap_use_ri_vtex", true);
static ShaderVariableInfo clipmap_feedback_stride("clipmap_feedback_stride", true);
static ShaderVariableInfo clipmap_feedback_readback_stride("clipmap_feedback_readback_stride", true);
static ShaderVariableInfo clipmap_feedback_readback_size("clipmap_feedback_readback_size", true);
static ShaderVariableInfo clipmap_feedback_buf("clipmap_feedback_buf", true);
static ShaderVariableInfo clipmap_tex_mips("clipmap_tex_mips", true);
static ShaderVariableInfo clipmap_max_tex_mips("clipmap_max_tex_mips", true);
static ShaderVariableInfo uav_feedback_exists("uav_feedback_exists", true);
static ShaderVariableInfo ri_landclass_changed_indicex_bits("ri_landclass_changed_indicex_bits", true);
static ShaderVariableInfo rendering_feedback_view_scale("rendering_feedback_view_scale", true);
static ShaderVariableInfo rendinst_landscape_area_left_top_right_bottom("rendinst_landscape_area_left_top_right_bottom", true);
static ShaderVariableInfo clipmap_histogram_buf("clipmap_histogram_buf", true);
static ShaderVariableInfo feedback_downsample_dim("feedback_downsample_dim", true);
static ShaderVariableInfo clipmap_histogram_buf_rw("clipmap_histogram_buf_rw", true);
static ShaderVariableInfo clipmap_tile_info_buf_rw("clipmap_tile_info_buf_rw", true);
static ShaderVariableInfo clipmap_tile_info_buf_max_elements("clipmap_tile_info_buf_max_elements", true);
static ShaderVariableInfo feedback_downsample_in("feedback_downsample_in", true);
static ShaderVariableInfo feedback_downsample_out("feedback_downsample_out", true);
static ShaderVariableInfo clipmap_cache_output_count("clipmap_cache_output_count", true);
static ShaderVariableInfo clipmap_feedback_order_prefix("clipmap_feedback_order_prefix", true);
static ShaderVariableInfo bypass_cache_no("bypass_cache_no", true);
static ShaderVariableInfo src_mip("src_mip", true);
static ShaderVariableInfo indirection_tex("indirection_tex", true);
static ShaderVariableInfo landscape2uv("landscape2uv", true);
static ShaderVariableInfo mippos_offset("mippos_offset", true);
static ShaderVariableInfo world_dd_scale("world_dd_scale", true);
static ShaderVariableInfo g_VTexDim("g_VTexDim", true);
static ShaderVariableInfo g_TileSize("g_TileSize", true);
static ShaderVariableInfo g_cache2uv("g_cache2uv", true);
static ShaderVariableInfo fallback_info0("fallback_info0", true);
static ShaderVariableInfo fallback_info1("fallback_info1", true);
static ShaderVariableInfo use_secondary_terrain_vtex{"use_secondary_terrain_vtex", true};
static ShaderVariableInfo secondary_terrain_landscape2uv{"secondary_terrain_landscape2uv", true};
static ShaderVariableInfo secondary_terrain_mippos_offset{"secondary_terrain_mippos_offset", true};
static ShaderVariableInfo enable_debug_color{"clipmap_enable_debug_color", true};
static ShaderVariableInfo feedback_tex_unfiltered{"feedback_tex_unfiltered", true};
static ShaderVariableInfo clipmap_compression_gather4_allowed{"clipmap_compression_gather4_allowed", true};

} // namespace var

// Static for simplicity instead of being ClipmapImpl member,
// because inited once and depends on assumed clipmap_use_ri_vtex
static bool use_ri_vtex = false;
static size_t packed_tile_size = sizeof(uint);
static size_t max_ri_offset = 1;
static size_t ri_offset_bits = 0;
static size_t clipmap_hist_total_elements = CLIPMAP_HIST_NO_RI_VTEX_ELEMENTS;

struct MipPosition
{
  IPoint2 offset = IPoint2::ZERO;
  int tileZoomShift = 0;

  __forceinline int getZoom() const { return (1 << tileZoomShift); }
};

struct TexTileFeedback
{
  static constexpr int TILE_FEEDBACK_FORMAT = TEXFMT_A8R8G8B8;

  uint8_t ri_offset_or_unused;
  uint8_t y;
  uint8_t x;
  uint8_t mip;

  template <bool ri_vtex>
  uint8_t getRiOffset() const
  {
    if constexpr (ri_vtex)
      return ri_offset_or_unused;
    else
      return 0;
  }
};

// Few words about the coordinate system:
// In theory, tiles are on a world-space grid with different mip levels. All levels share the same pivot point 0,0.
// Grid step of each mip level is twice as large as the one bellow it.
// Thus a tile at TexTilePos::mip = A during MipPosition::tileZoomShift = B will have "grid step"/length of 2^(A + B) mip0 tiles.
// We keep as subregion of that world-space grid in memory; MipContext::currentTexMipCnt mips with TILE_WIDTH^2 subgrid for each mip
// level. The center of each mip level subgrid, relative to the theoretical world-space grid, is determined by flooring
// MipPosition::offset position on that grid. MipPosition::offset is not necessary aligned with the mip level's subgrid (thus the
// flooring).

struct TexTilePos
{
  int8_t x = 0, y = 0;
  uint8_t mip = -1;
  uint8_t ri_offset = 0;

  TexTilePos() = default;
  TexTilePos(int X, int Y, int MIP, int RI_OFFSET) : x(X), y(Y), mip(MIP), ri_offset(RI_OFFSET) {}

  bool isValid() const
  {
    return x >= -HALF_TILE_WIDTH && y >= -HALF_TILE_WIDTH && x < HALF_TILE_WIDTH && y < HALF_TILE_WIDTH && mip < TEX_MIPS &&
           ri_offset < max_ri_offset;
  }

  uint32_t getTileTag() const
  {
    G_FAST_ASSERT(isValid());
    return ((uint32_t)mip << (TILE_WIDTH_BITS + TILE_WIDTH_BITS + ri_offset_bits)) |
           ((uint32_t)ri_offset << (TILE_WIDTH_BITS + TILE_WIDTH_BITS)) | ((uint32_t)((int)y + HALF_TILE_WIDTH) << TILE_WIDTH_BITS) |
           (uint32_t)((int)x + HALF_TILE_WIDTH);
  }
  uint32_t getTileBitIndex() const { return getTileTag(); }

  eastl::optional<TexTilePos> migrateMipPosition(const MipPosition &old_mippos, const MipPosition &new_mippos, int mip_count) const
  {
    G_FAST_ASSERT(isValid());
    eastl::optional<TexTilePos> result = migrateMipPosition(*this, old_mippos, new_mippos, mip_count);

#if DAGOR_DBGLEVEL > 0
    if (result)
    {
      eastl::optional<TexTilePos> demigration = migrateMipPosition(*result, new_mippos, old_mippos, TEX_MIPS); // -V764
      G_ASSERT((bool)demigration && demigration->getTileTag() == getTileTag());
    }
#endif

    return result;
  }

  IBBox2 projectOnLowerMip(const MipPosition &mippos, int lower_mip) const
  {
    G_FAST_ASSERT(isValid());
    IBBox2 result = projectOnLowerMip(IPoint2(x, y), mip, mippos, lower_mip);

#if DAGOR_DBGLEVEL > 0 // crosschecking...
    IPoint2 reprojectedPosLim0 = getHigherMipProjectedPos(result.lim[0], lower_mip, mippos, mip);
    G_ASSERT(reprojectedPosLim0 == IPoint2(x, y));
    IPoint2 reprojectedPosLim1 = getHigherMipProjectedPos(result.lim[1], lower_mip, mippos, mip);
    G_ASSERT(reprojectedPosLim1 == IPoint2(x, y));
#endif

    return result;
  }

  IPoint2 getHigherMipProjectedPos(const MipPosition &mippos, int higher_mip) const
  {
    G_FAST_ASSERT(isValid());
    IPoint2 result = getHigherMipProjectedPos(IPoint2(x, y), mip, mippos, higher_mip);

#if DAGOR_DBGLEVEL > 0 // crosschecking...
    IBBox2 reprojectBox = projectOnLowerMip(result, higher_mip, mippos, mip);
    G_ASSERT(reprojectBox & IPoint2(x, y));
#endif

    return result;
  }

  static IBBox2 projectOnLowerMip(IPoint2 pos, int mip, const MipPosition &mippos, int lower_mip)
  {
    G_FAST_ASSERT(lower_mip <= mip);

    int mipDiff = mip - lower_mip;
    int boxLength = (1 << mipDiff) - 1;

    IPoint2 lowerMipOffset = mippos.offset >> lower_mip;
    IPoint2 higherMipOffset = mippos.offset >> mip;

    IPoint2 higherWorldPos = pos + higherMipOffset;
    IPoint2 lowerWorldPos = higherWorldPos << mipDiff;
    IPoint2 lim0 = lowerWorldPos - lowerMipOffset;
    IPoint2 lim1 = lim0 + IPoint2(boxLength, boxLength);

    return IBBox2(lim0, lim1);
  }

  static IPoint2 getHigherMipProjectedPos(IPoint2 pos, int mip, const MipPosition &mippos, int higher_mip)
  {
    G_FAST_ASSERT(higher_mip >= mip);

    int mipDiff = higher_mip - mip;

    IPoint2 lowerMipOffset = mippos.offset >> mip;
    IPoint2 higherMipOffset = mippos.offset >> higher_mip;

    IPoint2 lowerWorldPos = pos + lowerMipOffset;
    IPoint2 higherWorldPos = lowerWorldPos >> mipDiff;
    IPoint2 projectionPos = higherWorldPos - higherMipOffset;

    return projectionPos;
  }

private:
  static __forceinline eastl::optional<TexTilePos> migrateMipPosition(const TexTilePos &tile, const MipPosition &old_mippos,
    const MipPosition &new_mippos, int mip_count)
  {
    int mipDiff = new_mippos.tileZoomShift - old_mippos.tileZoomShift;
    int newMip = (int)tile.mip - mipDiff;
    if (!(newMip >= 0 && newMip < mip_count))
      return eastl::nullopt;

    IPoint2 oldMipOffset = old_mippos.offset >> tile.mip;
    IPoint2 newMipOffset = new_mippos.offset >> newMip;

    IPoint2 worldPos = IPoint2(tile.x, tile.y) + oldMipOffset;
    IPoint2 newPos = worldPos - newMipOffset;

    if (newPos.x >= -HALF_TILE_WIDTH && newPos.y >= -HALF_TILE_WIDTH && newPos.x < HALF_TILE_WIDTH && newPos.y < HALF_TILE_WIDTH)
      return TexTilePos{newPos.x, newPos.y, newMip, tile.ri_offset};
    else
      return eastl::nullopt;
  }
};

struct TexTileInfo : TexTilePos
{
  uint16_t count = 0;

  TexTileInfo() = default;
  TexTileInfo(int X, int Y, int MIP, int RI_OFFSET, int COUNT) : TexTilePos(X, Y, MIP, RI_OFFSET), count(COUNT) {}
  explicit TexTileInfo(const TexTilePos &other, int COUNT = 0) : TexTilePos(other), count(COUNT) {}
};

struct TexTileInfoOrder : TexTileInfo
{
  uint64_t sortOrder = 0;

  TexTileInfoOrder() = default;
  TexTileInfoOrder(int X, int Y, int MIP, int RI_OFFSET, int COUNT) : TexTileInfo(X, Y, MIP, RI_OFFSET, COUNT) {}
  explicit TexTileInfoOrder(const TexTileInfo &other) : TexTileInfo(other) {}

  void updateOrder()
  {
    uint32_t scaledCount = ri_offset >= RI_VTEX_SECONDARY_OFFSET_START ? (TILE_WIDTH * TILE_WIDTH) : count;
    scaledCount *= (TEX_MIPS - mip);
    uint32_t index = getTileTag();
    sortOrder = ~(uint64_t)scaledCount << 32 | index;
  }
  static bool sortByOrder(const TexTileInfoOrder &a, const TexTileInfoOrder &b) { return a.sortOrder < b.sortOrder; };

  // for debugging only
  bool operator==(const TexTileInfoOrder &other) const
  {
    return ri_offset == other.ri_offset && x == other.x && y == other.y && mip == other.mip && count == other.count &&
           sortOrder == other.sortOrder;
  }
  bool operator!=(const TexTileInfoOrder &other) const { return !(*this == other); }
};

struct CacheCoords
{
  uint8_t x = 0, y = 0;

  CacheCoords() = default;
  CacheCoords(int X, int Y) : x(X), y(Y) {}

  // for error stats
  bool operator==(const CacheCoords &other) const { return x == other.x && y == other.y; }
};

// for error stats
template <>
struct eastl::hash<CacheCoords>
{
  size_t operator()(const CacheCoords &c) const { return eastl::hash<uint16_t>()((uint16_t(c.y) << 8) | (uint16_t(c.x))); }
};

struct TexLRU
{
  TexTilePos tilePos;
  CacheCoords cacheCoords;
  uint8_t flags = 0;

  enum
  {
    NEED_UPDATE = (1 << 0),
    IS_USED = (1 << 1),    // According to latest feedback, tile is visible.
    IS_INVALID = (1 << 2), // Has different (previous) tile in atlas. This atlas cache can not be registered, since it has old data in
                           // atlas but refers to new position already. However as long as we do not update a tile with "IS_INVALID",
                           // indirection texture is valid.
  };

  TexLRU() = default;
  TexLRU(CacheCoords CACHECOORDS) : cacheCoords(CACHECOORDS) {}
  TexLRU(CacheCoords CACHECOORDS, TexTilePos TILEPOS) : cacheCoords(CACHECOORDS), tilePos(TILEPOS) {}

  TexLRU &setFlags(uint8_t flag)
  {
    flags = flag;
    return *this;
  }
};

typedef Tab<TexLRU> TexLRUList;

struct TexTileIndirection
{
  CacheCoords cacheCoords;
  uint8_t mip = 0;
  uint8_t tag = 0;

  TexTileIndirection() = default;
  TexTileIndirection(CacheCoords CACHECOORDS, int MIP, int TAG) : cacheCoords(CACHECOORDS), mip(MIP), tag(TAG) {}
};

struct RiIndices : public eastl::array<int, MAX_RI_VTEX_CNT>
{
  using CompareBitset = eastl::bitset<MAX_RI_VTEX_CNT>;
  RiIndices() { fill(RI_INVALID_ID); }

  CompareBitset compareEqual(const RiIndices &other) const
  {
    CompareBitset indicesEqual;
    for (int i = 0; i < MAX_RI_VTEX_CNT; ++i)
      indicesEqual[i] = (*this)[i] == other[i] && (*this)[i] != RI_INVALID_ID;

    return indicesEqual;
  }
};

using RiMipPosArray = eastl::array<MipPosition, MAX_RI_VTEX_CNT>;

struct CaptureMetadata
{
  MipPosition position;
  RiIndices riIndices;
  int texMipCnt = -1;

  enum class CompResult
  {
    INVALID,
    ZOOM_OR_OFFSET_CHANGED,
    EQUAL
  };
  CompResult compare(const MipPosition &other) const
  {
    if (!isValid())
      return CompResult::INVALID;
    else if (position.tileZoomShift != other.tileZoomShift || position.offset != other.offset)
      return CompResult::ZOOM_OR_OFFSET_CHANGED;
    else
      return CompResult::EQUAL;
  }

  bool isValid() const { return texMipCnt > 0; }
};

static constexpr int MAX_FRAMES_AHEAD = 3;

template <bool ri_vtex>
using PackedTile = eastl::conditional_t<ri_vtex, uint2, uint32_t>;


using CacheCoordErrorMap = ska::flat_hash_map<CacheCoords, carray<BcCompressor::ErrorStats, Clipmap::MAX_TEXTURES>>;
struct MipContext
{
  TexLRUList LRU;
  MipPosition LRUPos;
  int currentTexMipCnt = 0;

  MipPosition bindedPos;
  RiIndices bindedRiIndices;
  int bindedTexMipCnt = 0;

  UniqueTexHolder indirection;
  uint32_t captureTargetCounter = 0;
  int oldestCaptureTargetCounter = 0;

  Tab<uint16_t> updateLRUIndices;
  bool invalidateIndirection = false;

  PostFxRenderer *feedbackTexFilterRenderer = nullptr;
  carray<CaptureMetadata, MAX_FRAMES_AHEAD> captureMetadata;
  carray<EventQueryHolder, MAX_FRAMES_AHEAD> captureTexFence;
  // non-UAV HW feedback textures
  UniqueTex captureTexRtUnfiltered;
  carray<UniqueTex, MAX_FRAMES_AHEAD> captureTexRt;
  // UAV HW feedback textures
  uint32_t feedbackUAVAtomicPrefix = 0;
  UniqueBuf captureStagingBuf;
  UniqueBuf intermediateHistBuf;
  carray<UniqueBuf, MAX_FRAMES_AHEAD> captureTileInfoBuf;

  CaptureMetadata lockedCaptureMetadata;

  int lockedPixelsBytesStride = 0;
  size_t lockedPixelsMemCopySize = 0;
  uint8_t *lockedPixelsMemCopy = nullptr;

  bool use_uav_feedback = false;
  IPoint2 feedbackSize = IPoint2::ZERO;
  int feedbackOversampling = 1;
  uint32_t maxTilesFeedback = 0;
  bool useFeedbackTexFilter = false;

  // Consumes slightly more memory if ri vtex is not used.
  using InCacheBitmap = eastl::bitset<CLIPMAP_HIST_WITH_RI_VTEX_ELEMENTS>;
  InCacheBitmap inCacheBitmap; // keep it last for cache locality.

  void init(bool use_uav_feedback, IPoint2 feedback_size, int feedback_oversampling, uint32_t max_tiles_feedback,
    bool useFeedbackTexFilter);
  void initIndirection(int max_tex_mips);
  void closeIndirection();
  void closeHWFeedback();
  void initHWFeedback();
  void reset(int cacheDimX, int cacheDimY, bool force_redraw, int tex_tile_size, int feedback_type);
  inline void resetInCacheBitmap();

  void checkBitmapLRUCoherence() const;

  struct Fallback
  {
    int pagesDim = 0;
    float texelSize = 2.0f;
    int texTileSize = 0;
    IPoint2 originXZ = {-1000, 10000};
    bool drawDecals = true;
    bool sameSettings(int dim, float texel, bool draw_decals) const
    {
      return pagesDim == dim && texelSize == texel && drawDecals == draw_decals;
    }
    const Point2 getOrigin() const { return Point2::xy(originXZ) * texelSize; }
    const int getTexelsSize() const { return (pagesDim * texTileSize); }
    const float getSize() const { return getTexelsSize() * texelSize; }
    const bool isValid() const { return pagesDim > 0; }
  } fallback;
  bool allocateFallback(int dim, int tex_tile_size); // return true if changed indirection
};

static constexpr int calc_squared_area_around(int tile_width) { return sqr(tile_width * 2 + 1); }
static constexpr int MAX_RI_VTEX_CNT_WITHOUT_SECONDARY = (MAX_RI_VTEX_CNT - 1) / 2;

class RiLandclassDataManager
{
  carray<Point4, MAX_RI_VTEX_CNT> ri_landscape2uv_arr;
  carray<IPoint4, MAX_RI_VTEX_CNT> ri_mippos_offset_arr;
  carray<Point4, MAX_RI_VTEX_CNT> ri_landclass_pos_arr;
  carray<Point4, MAX_RI_VTEX_CNT> ri_landclass_normal_arr;

  carray<Point4, MAX_RI_VTEX_CNT> ri_landclass_mapping_arr;
  carray<Point4, MAX_RI_VTEX_CNT> ri_landclass_inv_tm_x_arr;
  carray<Point4, MAX_RI_VTEX_CNT> ri_landclass_inv_tm_y_arr;
  carray<Point4, MAX_RI_VTEX_CNT> ri_landclass_inv_tm_z_arr;
  carray<Point4, MAX_RI_VTEX_CNT> ri_landclass_inv_tm_w_arr;

  carray<Point4, MAX_RI_VTEX_CNT_WITHOUT_SECONDARY> world_to_hmap_tex_ofs_arr;
  carray<Point4, MAX_RI_VTEX_CNT_WITHOUT_SECONDARY> world_to_hmap_ofs_arr;
  static constexpr int RI_INDICES_ARR4_SIZE = (MAX_RI_VTEX_CNT + 4 - 1) / 4;
  carray<IPoint4, RI_INDICES_ARR4_SIZE> riIndicesArr4;

  static constexpr int RI_LANDCLASS_DATA_BUFFER_SIZE =
    9 * MAX_RI_VTEX_CNT + 2 * MAX_RI_VTEX_CNT_WITHOUT_SECONDARY + RI_INDICES_ARR4_SIZE;
  carray<Point4, RI_LANDCLASS_DATA_BUFFER_SIZE> gpuData;

  UniqueBufHolder riLandclassDataBuf;

public:
  void clear()
  {
    for (int i = 0; i < MAX_RI_VTEX_CNT; i++)
    {
      ri_landscape2uv_arr[i] = Point4::ZERO;
      ri_mippos_offset_arr[i] = IPoint4::ZERO;
      ri_landclass_pos_arr[i] = Point4::ZERO;
      ri_landclass_normal_arr[i] = Point4::ZERO;
      ri_landclass_mapping_arr[i] = Point4::ZERO;
      ri_landclass_inv_tm_x_arr[i] = Point4::ZERO;
      ri_landclass_inv_tm_y_arr[i] = Point4::ZERO;
      ri_landclass_inv_tm_z_arr[i] = Point4::ZERO;
      ri_landclass_inv_tm_w_arr[i] = Point4::ZERO;

      if (i < MAX_RI_VTEX_CNT_WITHOUT_SECONDARY)
      {
        world_to_hmap_tex_ofs_arr[i] = Point4::ZERO;
        world_to_hmap_ofs_arr[i] = Point4::ZERO;
      }

      if (i < RI_INDICES_ARR4_SIZE)
        riIndicesArr4[i] = IPoint4::ZERO;
    }
  }

  void init()
  {
    clear();
    riLandclassDataBuf = dag::buffers::create_persistent_cb(RI_LANDCLASS_DATA_BUFFER_SIZE, "ri_landclass_data_buffer", RESTAG_LAND);
  }

  RiLandclassDataManager() { init(); }


#define SET_LANDCLASS_MANAGER_OFS_VAL(array, idx, val) \
  G_ASSERT(idx < MAX_RI_VTEX_CNT_WITHOUT_SECONDARY);   \
  array[idx] = val;

  void setHmapTexOfs(int idx, const Point4 &val) { SET_LANDCLASS_MANAGER_OFS_VAL(world_to_hmap_tex_ofs_arr, idx, val); }
  void setHmapOfs(int idx, const Point4 &val) { SET_LANDCLASS_MANAGER_OFS_VAL(world_to_hmap_ofs_arr, idx, val); }

  template <typename Landscape2uvCb, typename MipPositionCb>
  void updateLandclass2uv(RiIndices riIndices, Landscape2uvCb getLandscape2uv, MipPositionCb getMipPosition)
  {
    for (int i = 0; i < MAX_RI_VTEX_CNT; ++i)
    {
      ri_landscape2uv_arr[i] = riIndices[i] != RI_INVALID_ID ? (getLandscape2uv(i + 1)) : Point4(1, 1, 0, 0);

      IPoint2 mippos = riIndices[i] != RI_INVALID_ID ? (getMipPosition(i + 1).offset) : IPoint2::ZERO;
      ri_mippos_offset_arr[i] = IPoint4(mippos.x, mippos.y, 0, 0);
    }

    ShaderGlobal::set_float4(var::secondary_terrain_landscape2uv, ri_landscape2uv_arr[RI_VTEX_SECONDARY_OFFSET_START - 1]);
    ShaderGlobal::set_int4(var::secondary_terrain_mippos_offset, ri_mippos_offset_arr[RI_VTEX_SECONDARY_OFFSET_START - 1]);
  }

  void updateClosestIndices(RiIndices riIndices)
  {
    G_STATIC_ASSERT(MAX_RI_VTEX_CNT < 32); // limitation due to firstbithigh in get_clipmap_indirection_offset(but it's also expensive)
    auto getRiIndex = [&](int i) { return i < riIndices.size() ? riIndices[i] : RI_INVALID_ID; };
    for (int i = 0; i < riIndicesArr4.size(); ++i)
      riIndicesArr4[i] = IPoint4(getRiIndex(i * 4 + 0), getRiIndex(i * 4 + 1), getRiIndex(i * 4 + 2), getRiIndex(i * 4 + 3));
  }

  void updateLandclassData(int idx, const RendinstLandclassData &riLandclass)
  {
    G_ASSERT(idx < MAX_RI_VTEX_CNT);

    const auto &matrix = riLandclass.matrix;
    const auto &invMatrix = riLandclass.matrixInv;

    ri_landclass_mapping_arr[idx] = riLandclass.mapping;
    ri_landclass_pos_arr[idx] = Point4(matrix.getcol(3).x, matrix.getcol(3).y, matrix.getcol(3).z, 1);
    ri_landclass_normal_arr[idx] = Point4(matrix.getcol(1).x, matrix.getcol(1).y, matrix.getcol(1).z, 0);

    ri_landclass_inv_tm_x_arr[idx] = Point4(invMatrix.getcol(0).x, invMatrix.getcol(0).y, invMatrix.getcol(0).z, 0);
    ri_landclass_inv_tm_y_arr[idx] = Point4(invMatrix.getcol(1).x, invMatrix.getcol(1).y, invMatrix.getcol(1).z, 0);
    ri_landclass_inv_tm_z_arr[idx] = Point4(invMatrix.getcol(2).x, invMatrix.getcol(2).y, invMatrix.getcol(2).z, 0);
    ri_landclass_inv_tm_w_arr[idx] = Point4(invMatrix.getcol(3).x, invMatrix.getcol(3).y, invMatrix.getcol(3).z, 1);
  }

  void updateGpuData()
  {
    for (int i = 0; i < MAX_RI_VTEX_CNT; ++i)
    {
      gpuData[i + MAX_RI_VTEX_CNT * 0] = ri_landscape2uv_arr[i];
      gpuData[i + MAX_RI_VTEX_CNT * 1] =
        Point4(bitwise_cast<float>(ri_mippos_offset_arr[i].x), bitwise_cast<float>(ri_mippos_offset_arr[i].y), 0, 0);
      gpuData[i + MAX_RI_VTEX_CNT * 2] = ri_landclass_pos_arr[i];
      gpuData[i + MAX_RI_VTEX_CNT * 3] = ri_landclass_normal_arr[i];

      gpuData[i + MAX_RI_VTEX_CNT * 4] = ri_landclass_mapping_arr[i];
      gpuData[i + MAX_RI_VTEX_CNT * 5] = ri_landclass_inv_tm_x_arr[i];
      gpuData[i + MAX_RI_VTEX_CNT * 6] = ri_landclass_inv_tm_y_arr[i];
      gpuData[i + MAX_RI_VTEX_CNT * 7] = ri_landclass_inv_tm_z_arr[i];
      gpuData[i + MAX_RI_VTEX_CNT * 8] = ri_landclass_inv_tm_w_arr[i];
      if (i < MAX_RI_VTEX_CNT_WITHOUT_SECONDARY)
      {
        gpuData[i + MAX_RI_VTEX_CNT * 9 + MAX_RI_VTEX_CNT_WITHOUT_SECONDARY * 0] = world_to_hmap_tex_ofs_arr[i];
        gpuData[i + MAX_RI_VTEX_CNT * 9 + MAX_RI_VTEX_CNT_WITHOUT_SECONDARY * 1] = world_to_hmap_ofs_arr[i];
      }

      if (i < RI_INDICES_ARR4_SIZE)
        gpuData[i + MAX_RI_VTEX_CNT * 9 + MAX_RI_VTEX_CNT_WITHOUT_SECONDARY * 2] = Point4(bitwise_cast<float>(riIndicesArr4[i].x),
          bitwise_cast<float>(riIndicesArr4[i].y), bitwise_cast<float>(riIndicesArr4[i].z), bitwise_cast<float>(riIndicesArr4[i].w));
    }

    riLandclassDataBuf.getBuf()->updateData(0, gpuData.size() * sizeof(gpuData[0]), gpuData.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  }
};

struct HistBuffer
{
  uint16_t *p = nullptr;
  size_t size = 0;

  void alloc()
  {
    close();
    size = clipmap_hist_total_elements;
    G_STATIC_ASSERT(sizeof(vec4i) <= 16);
    p = reinterpret_cast<uint16_t *>(defaultmem->alloc(size * sizeof(uint16_t)));
  }

  void close()
  {
    if (!p)
      return;
    defaultmem->free(p);
    p = nullptr;
    size = 0;
  }

  void clear()
  {
    if (!p)
      return;
    mem_set_0(dag::Span{p, static_cast<intptr_t>(size)});
  }
};

class ClipmapImpl
{
public:
  static constexpr int SECONDARY_OFFSET_START_NO_TERRAIN = RI_VTEX_SECONDARY_OFFSET_START - 1;

  static constexpr int NUM_TILES_AROUND_0 = 2;
  static constexpr int NUM_TILES_AROUND_1 = 1;
  static constexpr int NUM_TILES_AROUND_LASTMIP = 1;
  static constexpr int FAKE_TILE_MIP_AROUND_CAMERA_0 = 2;
  static constexpr int FAKE_TILE_MIP_AROUND_CAMERA_1 = 5;

  // based on processTilesAroundCamera
  static constexpr int MAX_SECONDARY_FAKE_TILE_CNT =
    (calc_squared_area_around(NUM_TILES_AROUND_0 * 2) + calc_squared_area_around(NUM_TILES_AROUND_1 * 2) +
      calc_squared_area_around(NUM_TILES_AROUND_LASTMIP)) *
    (MAX_VTEX_CNT - RI_VTEX_SECONDARY_OFFSET_START);

  static constexpr int MAX_PRIMARY_FAKE_TILE_CNT = // based on processTilesAroundCamera
    (calc_squared_area_around(NUM_TILES_AROUND_0) + calc_squared_area_around(NUM_TILES_AROUND_1)) * RI_VTEX_SECONDARY_OFFSET_START +
    calc_squared_area_around(NUM_TILES_AROUND_LASTMIP) * (RI_VTEX_SECONDARY_OFFSET_START - 1);

  static constexpr int MAX_FAKE_TILE_CNT = max(MAX_PRIMARY_FAKE_TILE_CNT, MAX_SECONDARY_FAKE_TILE_CNT);

  static constexpr int UNIQUE_SECONDARY_TERRAIN_VTEX_INDEX = -3; // -1 and -2 are used for invalid values, see RI_INVALID_ID

  static constexpr int ALIGNED_MIPS = 5; // Distance between round points is 2^(ALIGNED_MIPS - 1) tiles.
  G_STATIC_ASSERT(1 << (ALIGNED_MIPS - 1) <= HALF_TILE_WIDTH);

  static constexpr int RI_INDICES_MAX_MIP_CNT = 7;

  enum
  {
    SOFTWARE_FEEDBACK,
    CPU_HW_FEEDBACK
  };

  constexpr uint32_t getMaxTileCaptureInfoElementCount() { return MAX_TILE_INFO_BUF_CAPTURE_ELEMENENTS; }

  void initVirtualTexture(int cacheDimX, int cacheDimY, int tex_tile_size, int virtual_texture_mip_cnt, int virtual_texture_anisotropy,
    float maxEffectiveTargetResolution);
  void closeVirtualTexture();

  // fallbackTexelSize is fallbackPage size in meters
  // can be set to something like max(4, (1<<(getMaxTexMips()-1))*getPixelRatio*8) - 8 times bigger texel than last clip, but
  // not less than 4 meters if we allocate two pages, it results in minimum 4*256*2 == 1024meters (512 meters radius) of fallback
  // pages_dim*pages_dim is amount of pages allocated for fallback
  void initFallbackPage(int pages_dim, float fallback_texel_size);

  void update(ClipmapRenderer &renderer, bool turn_off_decals_on_fallback);
  void updateCache(ClipmapRenderer &renderer);

  void resetSecondaryFeedbackCenter() { nextSecondaryFeedbackCenter.reset(); }
  void setSecondaryFeedbackCenter(const Point3 &center) { nextSecondaryFeedbackCenter = center; }

  void startSecondaryFeedback() { ShaderGlobal::set_int(var::use_secondary_terrain_vtex, 1); }
  void endSecondaryFeedback() { ShaderGlobal::set_int(var::use_secondary_terrain_vtex, 0); }

  Point3 getMappedRiPos(int ri_offset) const;

  Point3 getMappedRelativeRiPos(int ri_offset, const Point3 &viewer_position, float &dist_to_ground) const;

  void restoreIndirectionFromLRU(SmallTab<TexTileIndirection, MidmemAlloc> *tiles); // debug
  int getMaxTexMips() const { return maxTexMipCnt; }
  int getCurrentTexMips(int ri_offset = 0) const
  {
    // Due to implementation reasons, RI's mip count should not be greater than currentContext->currentTexMipCnt.
    return ri_offset == TERRAIN_RI_OFFSET ? currentContext->currentTexMipCnt
                                          : min(RI_INDICES_MAX_MIP_CNT, currentContext->currentTexMipCnt);
  }
  void setMaxTileUpdateCount(int count) { maxTileUpdateCount = count; }
  __forceinline int getZoom() const { return currentContext->bindedPos.getZoom(); }
  __forceinline float getPixelRatio() const { return getZoom() * pixelRatio; }

  __forceinline TexTileIndirection &atTile(SmallTab<TexTileIndirection, MidmemAlloc> *tiles, int mip, int addr)
  {
    // G_ASSERTF(currentContext, "current context");
    return tiles[mip][addr];
  }
  void setMaxTexMips(int tex_mips);
  ClipmapImpl(bool in_use_uav_feedback) : // same as TEXFMT_A8R8G8B8
    viewerPosition(0.f, 0.f, 0.f),
    tileInfoSize(0),
    mipCnt(1),
    cacheWidth(0),
    cacheHeight(0),
    cacheDimX(0),
    cacheDimY(0),
    maxTexMipCnt(0),
    maxTileUpdateCount(1024),
    use_uav_feedback(is_uav_supported() && in_use_uav_feedback),
    feedbackSize(IPoint2(0, 0)),
    feedbackOversampling(1),
    moreThanFeedback(1),
    moreThanFeedbackInv(0xFFFF - 1)
  {
    G_STATIC_ASSERT((1 << TILE_WIDTH_BITS) == TILE_WIDTH);

    debug("clipmap: use_uav_feedback=%d", (int)use_uav_feedback);
    if (use_uav_feedback != in_use_uav_feedback)
      logwarn("clipmap: UAV feedback was preferred but it is not supported.");

    mem_set_0(compressor);
    eastl::fill(bufferTexFlags.begin(), bufferTexFlags.end(), Clipmap::BAD_TEX_FLAGS);
    eastl::fill(compressorTexFlags.begin(), compressorTexFlags.end(), Clipmap::BAD_TEX_FLAGS);
    texTileBorder = 1; // bilinear only
    texTileInnerSize = texTileSize - 2 * texTileBorder;
    feedbackType = SOFTWARE_FEEDBACK;
    feedbackDepthTex.close();
    failLockCount = 0;
    // END OF VIRTUAL TEXTURE INITIALIZATION
  }
  ~ClipmapImpl() { close(); }
  // sz - texture size of each clip, clip_count - number of clipmap stack. multiplayer - size of next clip
  // size in meters of latest clip, virtual_texture_mip_cnt - amount of mips(lods) of virtual texture containing tiles
  void init(float st_texel_size, uint32_t feedbackType,
    FeedbackProperties feedback_properties = Clipmap::getDefaultFeedbackProperties(), int texMips = 6, bool use_own_buffers = true);
  void close();

  // returns bits of rendered clip, or 0 if no clip was rendered
  void setMaxTexelSize(float max_texel_size) { maxTexelSize = max_texel_size; }

  float getStartTexelSize() const { return pixelRatio; }
  void setStartTexelSize(float st_texel_size) // same as in init
  {
    pixelRatio = st_texel_size;
    invalidate(true);
  }

  void setTargetSize(int w, int h, float mip_bias)
  {
    targetSize = IPoint2(w, h);
    targetMipBias = mip_bias;
  };

  void prepareRender(ClipmapRenderer &render, dag::Span<Texture *> buffer_tex, dag::Span<Texture *> compressor_tex,
    bool turn_off_decals_on_fallback = false);

  void prepareFeedback(const Point3 &viewer_pos, const TMatrix4 &globtm, const TMatrix4 *mirror_globtm_opt,
    const ZoomFeeedbackParams &zoom_params, const SoftwareFeedbackParams &software_fb, bool force_update = false,
    UpdateFeedbackThreadPolicy thread_policy = UpdateFeedbackThreadPolicy::PRIO_LOW);

  template <bool ri_vtex>
  void prepareFeedback(const Point3 &viewer_pos, const TMatrix4 &globtm, const TMatrix4 *mirror_globtm_opt,
    const ZoomFeeedbackParams &zoom_params, const SoftwareFeedbackParams &software_fb, bool force_update = false,
    UpdateFeedbackThreadPolicy thread_policy = UpdateFeedbackThreadPolicy::PRIO_LOW);

  void renderFallbackFeedback(ClipmapRenderer &renderer, const TMatrix4 &globtm);
  void finalizeFeedback();

  void invalidatePointi(const IPoint2 &ti, int lastMip = 0xFF); // not forces redraw!  (int tile coords)
  void invalidatePoint(const Point2 &world_xz);                 // not forces redraw!
  void invalidateBox(const BBox2 &world_xz);                    // not forces redraw!
  void invalidate(bool force_redraw = true);

  void recordCompressionErrorStats(bool enable);
  void dump();

  bool getBBox(BBox2 &ret) const;
  bool getMaximumBBox(BBox2 &ret) const;
  void setFeedbackType(uint32_t ftp);
  uint32_t getFeedbackType() const { return feedbackType; }
  void setSoftwareFeedbackRadius(int inner_tiles, int outer_tiles);
  void setSoftwareFeedbackMipTiles(int mip, int tiles_for_mip);

  void startUAVFeedback();
  void endUAVFeedback();
  void increaseUAVAtomicPrefix();
  void resetUAVAtomicPrefix();

  void copyUAVFeedback();
  BcCompressor *createCacheCompressor(int cache_id, uint32_t format, int compress_buf_width, int compress_buf_height);
  void createCaches(const uint32_t *formats, const uint32_t *uncompressed_formats, uint32_t cnt, const uint32_t *buffer_formats,
    uint32_t buffer_cnt);
  String getCacheBufferName(int idx);
  IPoint2 getCacheBufferDim();
  dag::ConstSpan<uint32_t> getCacheBufferFlags();
  dag::ConstSpan<uint32_t> getCacheCompressorFlags();
  int getCacheBufferMips();
  void setCacheBufferCount(int cache_count, int buffer_count);
  void setDynamicDDScale(const DynamicDDScaleParams &params);
  bool setAnisotropy(int anisotropy);
  void beforeReset();
  void afterReset();

  void getRiLandclassIndices(dag::Vector<int> &ri_landclass_indices)
  {
    if (!use_ri_vtex)
      return;

    ri_landclass_indices.clear();
    int cnt = min((int)rendinstLandclasses.size(), SECONDARY_OFFSET_START_NO_TERRAIN);
    for (int i = 0; i < cnt; ++i)
    {
      if (riIndices[i] == RI_INVALID_ID)
        break;
      ri_landclass_indices.push_back(riIndices[i]);
    }
  }

  RiLandclassDataManager *getRiLandclassDataManager() { return riLandclassDataManager ? &riLandclassDataManager.value() : nullptr; }
  void updateLandclassData();

  static bool is_uav_supported();

  int getLastUpdatedTileCount() const { return updatedTilesCnt; }

  // debug
  void toggleManualUpdate(bool manual_update) { triggerUpdatesManually = manual_update; };
  void requestManualUpdates(int count) { manualUpdates = count; };
  void setDebugColorMode(Clipmap::DebugMode mode) { ShaderGlobal::set_int(var::enable_debug_color, eastl::to_underlying(mode)); }

private:
  void beginUpdateTiles(ClipmapRenderer &renderer);
  void updateTileBlock(ClipmapRenderer &renderer, const CacheCoords &cacheCoords, const TexTilePos &tilePos);
  void updateTileBlockRaw(ClipmapRenderer &renderer, const CacheCoords &cacheCoords, const BBox2 &region);
  void endUpdateTiles(ClipmapRenderer &renderer);
  void setDDScale();
  void setAnisotropyImpl(int anisotropy);

  void processUAVFeedback(int captureTargetIdx);

  void copyAndAdvanceCaptureTarget(int captureTargetIdx);
  void prepareSoftwareFeedback(int zoomPow2, const Point3 &viewer_pos, const TMatrix &view_itm, const TMatrix4 &globtm,
    float approx_height, float maxDist0, float maxDist1, bool force_update);
  void prepareSoftwarePoi(const int vx, const int vy, const int mip, const int mipsize,
    carray<uint64_t, (TEX_MIPS * TILE_WIDTH * TILE_WIDTH + 63) / 64> &bitarrayUsed, const Point4 &uv2landscapetile,
    const vec4f *planes2d, int planes2dCnt);

  IPoint2 centerOffset(const Point3 &position, float zoom) const;
  void scheduleUpdate(const CaptureMetadata &capture_metadata, bool force_update);
  void scheduleUpdateFromFeedback(CaptureMetadata capture_metadata, const TMatrix4 &globtm, const TMatrix4 *mirror_globtm_opt,
    bool force_update);

  Point4 getLandscape2uv(const IPoint2 &offset, float zoom) const;
  Point4 getLandscape2uv(int ri_offset) const;
  Point4 getUV2landacape(const IPoint2 &offset, float zoom) const;
  Point4 getUV2landacape(int ri_offset) const;

  const MipPosition &getMipPosition(int ri_offset) const;
  bool isRiOffsetValid(int ri_offset) const;

  int getZoomLimit(int texMips) const;
  IBBox2 getMaximumIBBox(int ri_offset) const;

  void checkConsistency();

  void GPUrestoreIndirectionFromLRUFull();
  void GPUrestoreIndirectionFromLRU();

  BBox2 currentLRUBox;
  uint32_t feedbackType;
  static constexpr int MAX_CACHE_TILES = 1024; // 8192x8192
  int maxTileUpdateCount;

  friend class PrepareFeedbackJob;
  int swFeedbackInnerMipTiles = 2;
  int swFeedbackOuterMipTiles = 3;

  ShaderMaterial *constantFillerMat = 0;
  ShaderElement *constantFillerElement = 0;
  ShaderMaterial *directUncompressMat = 0;
  ShaderElement *directUncompressElem = 0;
  DynShaderQuadBuf *constantFillerBuf = 0;
  RingDynamicVB *constantFillerVBuf = 0;

  int cacheAnisotropy = 1;

  bool useOwnBuffers = true;
  carray<UniqueTex, Clipmap::MAX_TEXTURES> internalBufferTex;
  carray<int, Clipmap::MAX_TEXTURES> bufferTexVarId;

  int bufferCnt = 0;
  int maxBufferCnt = 0;

  eastl::fixed_vector<Texture *, Clipmap::MAX_TEXTURES, false> bindedBufferTex = {};     // temporally used during prepareRender.
  eastl::fixed_vector<Texture *, Clipmap::MAX_TEXTURES, false> bindedCompressorTex = {}; // temporally used during prepareRender.

  carray<uint32_t, Clipmap::MAX_TEXTURES> bufferTexFlags;
  carray<uint32_t, Clipmap::MAX_TEXTURES> compressorTexFlags;
  IPoint2 bufferTexDim = IPoint2::ZERO;

  carray<BcCompressor *, Clipmap::MAX_TEXTURES> compressor;
  carray<UniqueTexHolder, Clipmap::MAX_TEXTURES> cache;

  int cacheCnt = 0;
  int maxCacheCnt = 0;
  int mipCnt = 1;
  int fallbackFramesUpdate = 0;
  int updatedTilesCnt = 0;

  float dynamicDDScale = 1.f;
  DynamicDDScaleParams dynamicDDScaleParams;

  bool migrateLRU(const MipPosition &old_mipPos, const MipPosition &new_mipPos);
  void changeFallback(ClipmapRenderer &renderer, const Point3 &viewer, bool turn_off_decals_on_fallback);

  void initHWFeedback();
  void closeHWFeedback();
  void updateRenderStates();
  void finalizeCompressionQueue();
  void addCompressionQueue(const CacheCoords &cacheCoords);
  void sortFeedback();
  void applyFeedback(const CaptureMetadata &capture_metadata, bool force_update);

  void updateRendinstLandclassRenderState();
  void updateRendinstLandclassParams();

  template <bool ri_vtex>
  void prepareTileFeedbackFromTextureFeedback();

  void prepareTileFeedbackFromTextureFeedback();

  void processTileFeedback(const CaptureMetadata &capture_metadata, const TMatrix4 &globtm, const TMatrix4 *mirror_globtm_opt);

private:
  HistBuffer hist;
  UniqueTex feedbackDepthTex;


  using RiLandclasses = eastl::array<RendinstLandclassData, MAX_RI_VTEX_CNT>;
  RiLandclasses rendinstLandclasses = {};
  eastl::array<int, MAX_RI_VTEX_CNT> changedRiIndices;

  RiIndices riIndices;
  RiMipPosArray riPos;

  eastl::optional<RiLandclassDataManager> riLandclassDataManager = eastl::nullopt;

  eastl::unique_ptr<ComputeShaderElement> clearHistogramCs;
  eastl::unique_ptr<ComputeShaderElement> processFeedbackCs;
  eastl::unique_ptr<ComputeShaderElement> buildTileInfoCs;

  static constexpr int COMPRESS_QUEUE_SIZE = 4;
  StaticTab<CacheCoords, COMPRESS_QUEUE_SIZE> compressionQueue;
  int maxTexMipCnt = 0;
  int texTileBorder, texTileInnerSize;
  void setTileSizeVars();
  int failLockCount = 0;

  int cacheWidth = 0, cacheHeight = 0, cacheDimX = 0, cacheDimY = 0;

  int tileInfoSize = 0; // used elements of tileInfo array.

  // for error stats
  CacheCoordErrorMap tilesError;
  bool recordTilesError = false;

  IPoint2 targetSize = {-1, -1};
  float targetMipBias = 0.f;
  float maxEffectiveTargetResolution = float(-1);
  float pixelDerivativesScale = 1.f;

  float pixelRatio = 1.f, maxTexelSize = 32.f;
  int texTileSize = 256;
  eastl::array<int, TEX_MIPS> swFeedbackTilesPerMip;

  int fallbackPages = 0;
  float fallbackTexelSize = 1;

  Point3 viewerPosition;
  eastl::optional<Point3> secondaryFeedbackCenter;
  eastl::optional<Point3> nextSecondaryFeedbackCenter;

  bool use_uav_feedback = true;
  bool feedbackPendingChanges = true;
  IPoint2 feedbackSize;
  uint16_t moreThanFeedback;
  uint16_t moreThanFeedbackInv;
  int feedbackOversampling;
  bool useFeedbackTexFilter = false;

  static constexpr int MAX_TILE_INFO_BUF_ELEMENTS_FACTOR = 4;
  static constexpr int MAX_TILE_INFO_BUF_ELEMENTS = MAX_TILE_INFO_BUF_ELEMENTS_FACTOR * MAX_CACHE_TILES;
  static constexpr int MAX_TILE_INFO_BUF_CAPTURE_ELEMENENTS = MAX_TILE_INFO_BUF_ELEMENTS - MAX_FAKE_TILE_CNT;

  bool triggerUpdatesManually = false;
  int manualUpdates = 0;

  // keep currentContext, tileInfo, preparedTilesFeedback last to help cache locality of above data.
  eastl::optional<MipContext> currentContext;
  eastl::array<TexTileInfoOrder, MAX_TILE_INFO_BUF_ELEMENTS> tileInfo;
  eastl::fixed_vector<TexTileInfo, MAX_TILE_INFO_BUF_CAPTURE_ELEMENENTS, false> preparedTilesFeedback;

  template <typename Funct>
  void prepareFakeTiles(const TMatrix4 &globtm, const TMatrix4 *mirror_globtm_opt, Funct funct)
  {
    for (int ri_offset = 0; ri_offset < max_ri_offset; ++ri_offset) // -V1008
    {
      if (!isRiOffsetValid(ri_offset))
        continue;
      if (ri_offset >= RI_VTEX_SECONDARY_OFFSET_START && !secondaryFeedbackCenter) // -V560
        continue;
      // if we have secondary, only update that
      if (ri_offset < RI_VTEX_SECONDARY_OFFSET_START && secondaryFeedbackCenter) // -V560
        continue;

      float dist_to_ground;
      Point3 absoluteCamPos = ri_offset >= RI_VTEX_SECONDARY_OFFSET_START ? *secondaryFeedbackCenter : viewerPosition; //-V547
      Point3 relativeCamPos = getMappedRelativeRiPos(ri_offset, absoluteCamPos, dist_to_ground);
      processTilesAroundCamera(getCurrentTexMips(ri_offset), globtm, mirror_globtm_opt, getUV2landacape(ri_offset),
        getLandscape2uv(ri_offset), relativeCamPos, getLandscape2uv(TERRAIN_RI_OFFSET), ri_offset, dist_to_ground,
        getMipPosition(ri_offset), funct);
    }
  }
};

const IPoint2 Clipmap::FEEDBACK_DEFAULT_SIZE = IPoint2(FEEDBACK_DEFAULT_WIDTH, FEEDBACK_DEFAULT_HEIGHT);
const int Clipmap::MAX_TEX_MIP_CNT = TEX_MIPS;

void Clipmap::initVirtualTexture(int cacheDimX, int cacheDimY, int tex_tile_size, int virtual_texture_mip_cnt,
  int virtual_texture_anisotropy, float maxEffectiveTargetResolution)
{
  clipmapImpl->initVirtualTexture(cacheDimX, cacheDimY, tex_tile_size, virtual_texture_mip_cnt, virtual_texture_anisotropy,
    maxEffectiveTargetResolution);
}
void Clipmap::closeVirtualTexture() { clipmapImpl->closeVirtualTexture(); }

void Clipmap::resetSecondaryFeedbackCenter() { clipmapImpl->resetSecondaryFeedbackCenter(); }
void Clipmap::setSecondaryFeedbackCenter(const Point3 &center) { clipmapImpl->setSecondaryFeedbackCenter(center); }

void Clipmap::startSecondaryFeedback() { clipmapImpl->startSecondaryFeedback(); }
void Clipmap::endSecondaryFeedback() { clipmapImpl->endSecondaryFeedback(); }

void Clipmap::initFallbackPage(int pages_dim, float fallback_texel_size)
{
  clipmapImpl->initFallbackPage(pages_dim, fallback_texel_size);
}

int Clipmap::getMaxTexMips() const { return clipmapImpl->getMaxTexMips(); }
void Clipmap::setMaxTileUpdateCount(int count) { clipmapImpl->setMaxTileUpdateCount(count); }
int Clipmap::getZoom() const { return clipmapImpl->getZoom(); }
float Clipmap::getPixelRatio() const { return clipmapImpl->getPixelRatio(); }

void Clipmap::setMaxTexMips(int tex_mips) { clipmapImpl->setMaxTexMips(tex_mips); }

Clipmap::Clipmap(bool in_use_uav_feedback) : clipmapImpl(eastl::make_unique<ClipmapImpl>(in_use_uav_feedback)) {}
Clipmap::~Clipmap() { close(); }


void Clipmap::init(float st_texel_size, uint32_t feedbackType, FeedbackProperties feedback_properties, int texMips,
  bool use_own_buffers)
{
  clipmapImpl->init(st_texel_size, feedbackType, feedback_properties, texMips, use_own_buffers);
}

void Clipmap::close() { clipmapImpl->close(); }


void Clipmap::setMaxTexelSize(float max_texel_size) { clipmapImpl->setMaxTexelSize(max_texel_size); }
float Clipmap::getStartTexelSize() const { return clipmapImpl->getStartTexelSize(); }
void Clipmap::setStartTexelSize(float st_texel_size) { clipmapImpl->setStartTexelSize(st_texel_size); }
void Clipmap::setTargetSize(int w, int h, float mip_bias) { clipmapImpl->setTargetSize(w, h, mip_bias); }
void Clipmap::prepareRender(ClipmapRenderer &render, dag::Span<Texture *> buffer_tex, dag::Span<Texture *> compressor_tex,
  bool turn_off_decals_on_fallback)
{
  clipmapImpl->prepareRender(render, buffer_tex, compressor_tex, turn_off_decals_on_fallback);
}
void Clipmap::prepareFeedback(const Point3 &viewer_pos, const TMatrix4 &globtm, const TMatrix4 *mirror_globtm_opt,
  const ZoomFeeedbackParams &zoom_params, const SoftwareFeedbackParams &software_fb, bool force_update,
  UpdateFeedbackThreadPolicy thread_policy)
{
  clipmapImpl->prepareFeedback(viewer_pos, globtm, mirror_globtm_opt, zoom_params, software_fb, force_update, thread_policy);
}
void Clipmap::renderFallbackFeedback(ClipmapRenderer &render, const TMatrix4 &globtm)
{
  clipmapImpl->renderFallbackFeedback(render, globtm);
}
void Clipmap::finalizeFeedback() { clipmapImpl->finalizeFeedback(); }
void Clipmap::invalidatePointi(const IPoint2 &ti, int lastMip) { clipmapImpl->invalidatePointi(ti, lastMip); }
void Clipmap::invalidatePoint(const Point2 &world_xz) { clipmapImpl->invalidatePoint(world_xz); }
void Clipmap::invalidateBox(const BBox2 &world_xz) { clipmapImpl->invalidateBox(world_xz); }
void Clipmap::invalidate(bool force_redraw) { clipmapImpl->invalidate(force_redraw); }
void Clipmap::recordCompressionErrorStats(bool enable) { clipmapImpl->recordCompressionErrorStats(enable); }
void Clipmap::dump() { clipmapImpl->dump(); }
bool Clipmap::getBBox(BBox2 &ret) const { return clipmapImpl->getBBox(ret); }
bool Clipmap::getMaximumBBox(BBox2 &ret) const { return clipmapImpl->getMaximumBBox(ret); }
void Clipmap::setFeedbackType(uint32_t ftp) { clipmapImpl->setFeedbackType(ftp); }
uint32_t Clipmap::getFeedbackType() const { return clipmapImpl->getFeedbackType(); }
void Clipmap::setSoftwareFeedbackRadius(int inner_tiles, int outer_tiles)
{
  clipmapImpl->setSoftwareFeedbackRadius(inner_tiles, outer_tiles);
}
void Clipmap::setSoftwareFeedbackMipTiles(int mip, int tiles_for_mip) { clipmapImpl->setSoftwareFeedbackMipTiles(mip, tiles_for_mip); }
void Clipmap::startUAVFeedback() { clipmapImpl->startUAVFeedback(); }
void Clipmap::endUAVFeedback() { clipmapImpl->endUAVFeedback(); }
void Clipmap::increaseUAVAtomicPrefix() { clipmapImpl->increaseUAVAtomicPrefix(); };
void Clipmap::resetUAVAtomicPrefix() { clipmapImpl->resetUAVAtomicPrefix(); };
void Clipmap::copyUAVFeedback() { clipmapImpl->copyUAVFeedback(); }
void Clipmap::createCaches(const uint32_t *formats, const uint32_t *uncompressed_formats, uint32_t cnt, const uint32_t *buffer_formats,
  uint32_t buffer_cnt)
{
  clipmapImpl->createCaches(formats, uncompressed_formats, cnt, buffer_formats, buffer_cnt);
}
String Clipmap::getCacheBufferName(int idx) { return clipmapImpl->getCacheBufferName(idx); }
IPoint2 Clipmap::getCacheBufferDim() { return clipmapImpl->getCacheBufferDim(); }
dag::ConstSpan<uint32_t> Clipmap::getCacheBufferFlags() { return clipmapImpl->getCacheBufferFlags(); }
dag::ConstSpan<uint32_t> Clipmap::getCacheCompressorFlags() { return clipmapImpl->getCacheCompressorFlags(); }
int Clipmap::getCacheBufferMips() { return clipmapImpl->getCacheBufferMips(); }
void Clipmap::setCacheBufferCount(int cache_count, int buffer_count) { clipmapImpl->setCacheBufferCount(cache_count, buffer_count); }
void Clipmap::setDynamicDDScale(const DynamicDDScaleParams &params) { clipmapImpl->setDynamicDDScale(params); }
bool Clipmap::setAnisotropy(int anisotropy) { return clipmapImpl->setAnisotropy(anisotropy); }
void Clipmap::beforeReset() { clipmapImpl->beforeReset(); }
void Clipmap::afterReset() { clipmapImpl->afterReset(); }
int Clipmap::getLastUpdatedTileCount() const { return clipmapImpl->getLastUpdatedTileCount(); }

bool Clipmap::is_uav_supported() { return ClipmapImpl::is_uav_supported(); }

void Clipmap::toggleManualUpdate(bool manual_update) { clipmapImpl->toggleManualUpdate(manual_update); }
void Clipmap::requestManualUpdates(int count) { clipmapImpl->requestManualUpdates(count); }

void Clipmap::getRiLandclassIndices(dag::Vector<int> &ri_landclass_indices)
{
  clipmapImpl->getRiLandclassIndices(ri_landclass_indices);
}

Point3 Clipmap::getMappedRelativeRiPos(int ri_offset, const Point3 &viewer_position, float &dist_to_ground) const
{
  return clipmapImpl->getMappedRelativeRiPos(ri_offset, viewer_position, dist_to_ground);
}

void Clipmap::setDebugColorMode(DebugMode mode) { clipmapImpl->setDebugColorMode(mode); }

void Clipmap::setHmapOfsAndTexOfs(int idx, const Point4 &hamp_ofs, const Point4 &hmap_tex_ofs)
{
  if (auto riLandclassDataManager = clipmapImpl->getRiLandclassDataManager())
  {
    riLandclassDataManager->setHmapTexOfs(idx, hmap_tex_ofs);
    riLandclassDataManager->setHmapOfs(idx, hamp_ofs);
  }
}

void Clipmap::updateLandclassData() { clipmapImpl->updateLandclassData(); }

CONSOLE_INT_VAL("clipmap", ri_invalid_frame_cnt, 6, 0, 20);
CONSOLE_FLOAT_VAL_MINMAX("clipmap", ri_fake_tile_max_height_0, 10.0f, 1.0f, 100.0f);
CONSOLE_FLOAT_VAL_MINMAX("clipmap", ri_fake_tile_max_height_1, 20.0f, 1.0f, 100.0f);

CONSOLE_INT_VAL("clipmap", fixed_zoom, -1, -1, 64);
CONSOLE_FLOAT_VAL("clipmap", zoom_pixel_size_scale, 0.75);
CONSOLE_BOOL_VAL("clipmap", disable_fake_tiles, false);
CONSOLE_BOOL_VAL("clipmap", disable_feedback, false);
CONSOLE_BOOL_VAL("clipmap", log_ri_indirection_change, false);
CONSOLE_BOOL_VAL("clipmap", always_report_unobtainable_feedback, false);
CONSOLE_BOOL_VAL("clipmap", always_report_high_wk, false);

CONSOLE_BOOL_VAL("clipmap", freeze_feedback, false);

static constexpr int RI_LANDCLASS_FIXED_ZOOM_SHIFT = 0;
static constexpr int RI_LANDCLASS_FIXED_ZOOM = 1 << RI_LANDCLASS_FIXED_ZOOM_SHIFT;

template <typename T, size_t N>
using TempSet =
  eastl::vector_set<T, eastl::less<T>, framemem_allocator, eastl::fixed_vector<T, N, /*overflow*/ true, framemem_allocator>>;

template <typename K, typename T, size_t N>
using TempMap = eastl::vector_map<K, T, eastl::less<K>, framemem_allocator,
  eastl::fixed_vector<eastl::pair<K, T>, N, /*overflow*/ true, framemem_allocator>>;

#define CHECK_MEMGUARD 0

#define MAX_ZOOM_TO_ALLOW_FAKE_AROUND_CAMERA 64 // all tiles!

#define RESTORE_IN_INDIRECTION_ONLY_USED_LRU \
  0 // change this to 1, if we need only used LRU tiles in indirection. However, on SLI/Crossfire we always restore only used

static const uint4 TILE_PACKED_BITS = uint4(TILE_PACKED_X_BITS, TILE_PACKED_Y_BITS, TILE_PACKED_MIP_BITS, TILE_PACKED_COUNT_BITS);
static const uint4 TILE_PACKED_SHIFT =
  uint4(0, TILE_PACKED_BITS.x, TILE_PACKED_BITS.x + TILE_PACKED_BITS.y, TILE_PACKED_BITS.x + TILE_PACKED_BITS.y + TILE_PACKED_BITS.z);
static const uint4 TILE_PACKED_MASK =
  uint4((1 << TILE_PACKED_BITS.x) - 1, (1 << TILE_PACKED_BITS.y) - 1, (1 << TILE_PACKED_BITS.z) - 1, (1 << TILE_PACKED_BITS.w) - 1);
static const uint16_t MAX_GPU_FEEDBACK_COUNT = TILE_PACKED_MASK.w;


// this scales feedback view and so WASTES cache a lot
// but it helps to fight with latency on feedback
static const float feedback_view_scale = 0.95;

template <typename T>
static uint4 unpackTileInfo(T packed_data, uint32_t &ri_offset);

template <>
uint4 unpackTileInfo(uint2 packed_data, uint32_t &ri_offset)
{
  uint4 data;
  for (int i = 0; i < 4; ++i)
    data[i] = (packed_data.x >> TILE_PACKED_SHIFT[i]) & TILE_PACKED_MASK[i];
  ri_offset = packed_data.y;
  G_FAST_ASSERT(ri_offset < max_ri_offset);
  return data;
}

template <>
uint4 unpackTileInfo(uint packed_data, uint32_t &ri_offset)
{
  uint4 data;
  ri_offset = TERRAIN_RI_OFFSET;
  for (int i = 0; i < 4; ++i)
    data[i] = (packed_data >> TILE_PACKED_SHIFT[i]) & TILE_PACKED_MASK[i];
  return data;
}

static float calc_landclass_to_point_dist_sq(const Point3 &rel_pos, float ri_radius)
{
  if (rel_pos.y < 0) // TODO: maybe use a little breathing room to avoid sharp changes (eg. for displacement)
    return FLT_MAX;

  float distSq = rel_pos.y * rel_pos.y;

  Point2 relPos2d = Point2(rel_pos.x, rel_pos.z);
  float dist2dSq = lengthSq(relPos2d);
  if (dist2dSq > ri_radius * ri_radius)
  {
    Point2 pos2d = relPos2d * (ri_radius / sqrt(dist2dSq));
    Point3 edgePos = Point3(pos2d.x, 0, pos2d.y);
    distSq = lengthSq(rel_pos - edgePos);
  }
  return distSq;
}

static float calc_landclass_to_point_dist_sq(const TMatrix &ri_matrix_inv, const Point3 &point, float ri_radius, Point3 &out_rel_pos)
{
  out_rel_pos = ri_matrix_inv * point;
  return calc_landclass_to_point_dist_sq(out_rel_pos, ri_radius);
}

static float calc_landclass_sort_order(const TMatrix &ri_matrix_inv, float ri_radius, const Point3 &point)
{
  Point3 relPos;
  return calc_landclass_to_point_dist_sq(ri_matrix_inv, point, ri_radius, relPos);
}

void MipContext::init(bool in_use_uav_feedback, IPoint2 in_feedback_size, int in_feedback_oversampling, uint32_t in_max_tiles_feedback,
  bool in_use_feedback_fex_filter)
{
  use_uav_feedback = in_use_uav_feedback;
  feedbackSize = in_feedback_size;
  feedbackOversampling = in_feedback_oversampling;
  maxTilesFeedback = in_max_tiles_feedback;
  useFeedbackTexFilter = in_use_feedback_fex_filter;

  closeHWFeedback();
  captureTargetCounter = oldestCaptureTargetCounter = 0;
}

void MipContext::resetInCacheBitmap() { inCacheBitmap.reset(); }

void MipContext::reset(int cacheDimX, int cacheDimY, bool force_redraw, int tex_tile_size, int feedback_type)
{
  // Invalidate all captureMetadata to signal that all previously scheduled feedback processing is no longer relevant
  if (force_redraw && feedback_type == ClipmapImpl::CPU_HW_FEEDBACK)
  {
    for (auto &metadata : captureMetadata)
      metadata = CaptureMetadata();
    lockedCaptureMetadata = CaptureMetadata();
  }

  invalidateIndirection = true;
  fallback.originXZ = IPoint2(-100000, 100000);
  if (force_redraw)
  {
    int lruDimX = cacheDimX / tex_tile_size;
    int lruDimY = cacheDimY / tex_tile_size;
    LRU.clear();
    updateLRUIndices.clear();
    LRU.reserve(lruDimY * lruDimX);
    for (int y = 0; y < lruDimY; ++y)
      for (int x = (y < fallback.pagesDim ? fallback.pagesDim : 0); x < lruDimX; ++x)
        LRU.push_back(TexLRU({x, y}));
    resetInCacheBitmap();
  }
  else
  {
    for (int i = 0; i < LRU.size(); ++i)
      if (LRU[i].tilePos.mip != 0xFF) // && (LRU[i].flags & LRU[i].IS_USED))
        LRU[i].flags |= LRU[i].NEED_UPDATE;
  }
}


__forceinline void calc_frustum_planes(vec4f *camPlanes, mat44f_cref v_globtm)
{
  camPlanes[0] = v_norm3(v_sub(v_globtm.col3, v_globtm.col0)); // right
  camPlanes[1] = v_norm3(v_add(v_globtm.col3, v_globtm.col0)); // left
  camPlanes[2] = v_norm3(v_sub(v_globtm.col3, v_globtm.col1)); // top
  camPlanes[3] = v_norm3(v_add(v_globtm.col3, v_globtm.col1)); // bottom
  camPlanes[4] = v_norm3(v_sub(v_globtm.col3, v_globtm.col2)); // far
  camPlanes[5] = v_norm3(v_globtm.col2);                       // near
}

__forceinline void calc_frustum_points(vec4f *points, const vec4f *camPlanes) // 8 points
{
  vec4f invalid; // should be always valid as it is frustum
  // far
  points[0] = three_plane_intersection(camPlanes[4], camPlanes[2], camPlanes[1], invalid); // topleft
  points[1] = three_plane_intersection(camPlanes[4], camPlanes[3], camPlanes[1], invalid); // bottomleft
  points[2] = three_plane_intersection(camPlanes[4], camPlanes[2], camPlanes[0], invalid); // topright
  points[3] = three_plane_intersection(camPlanes[4], camPlanes[3], camPlanes[0], invalid); // bottomright
  // near
  points[4] = three_plane_intersection(camPlanes[5], camPlanes[2], camPlanes[1], invalid); // topleft
  points[5] = three_plane_intersection(camPlanes[5], camPlanes[3], camPlanes[1], invalid); // bottomleft
  points[6] = three_plane_intersection(camPlanes[5], camPlanes[2], camPlanes[0], invalid); // topright
  points[7] = three_plane_intersection(camPlanes[5], camPlanes[3], camPlanes[0], invalid); // bottomright
}

__forceinline void calc_planes_ground_signs(int *planeSign, const vec4f *camPlanes)
{
  for (int i = 0; i < 6; ++i)
  {
    uint32_t yi = v_extract_yi(v_cast_vec4i(camPlanes[i]));
    planeSign[i] = (yi & 0x80000000) ? -1 : 1;
  }
}

__forceinline void prepare_2d_frustum(const vec4f *points, const vec4f *groundfarpoints, int *planeSign, plane3f *planes2d,
  int &planes2dCnt)
{
  planes2dCnt = 0;

  for (int i = 0; i < 4; ++i)
  {
    if (planeSign[1 - (i >> 1)] + planeSign[2 + (i & 1)] != 0)
      continue;
    int pi = i | 4;
    int ni = i;
    vec4f dir = v_sub(groundfarpoints[ni], points[pi]);
    // fixme: check intersection with lowest bound of world!
    // planes2d[planes2dCnt] = v_perm_zyxw(v_or(v_and(dir, (vec4f)V_CI_MASK1000),
    //                              v_and(v_neg(dir), (vec4f)V_CI_MASK0010)));
    planes2d[planes2dCnt] = v_or(v_and(v_perm_zxyw(dir), (vec4f)V_CI_MASK1000), v_and(v_neg(v_splat_x(dir)), (vec4f)V_CI_MASK0010));
    // planes2d[planes2dCnt] = v_make_vec4f(v_extract_z(points[ni]) - v_extract_z(points[pi]),
    //   0, v_extract_x(points[pi])-v_extract_x(points[ni]), 0);
    planes2d[planes2dCnt] = v_or(planes2d[planes2dCnt], v_and((vec4f)V_CI_MASK0001, v_neg(v_dot3(planes2d[planes2dCnt], points[pi]))));
    // if (v_test_vec_x_lt_0(v_plane_dist_x(planes2d[planes2dCnt], groundfarpoints[(ni+1)&3])))
    //   planes2d[planes2dCnt] = v_neg(planes2d[planes2dCnt]);
    planes2d[planes2dCnt] = v_sel(planes2d[planes2dCnt], v_neg(planes2d[planes2dCnt]),
      v_cmp_gt(v_zero(), v_plane_dist(planes2d[planes2dCnt], groundfarpoints[(ni + 1) & 3])));
    planes2dCnt++;
  }
  static const int prevNeighbour[4] = {1, 3, 0, 2};
  static const int planesOrder[4] = {1, 3, 2, 0};
  for (int i = 0; i < 4; ++i)
  {
    if (planeSign[4] + planeSign[planesOrder[i]] != 0)
      continue;
    int pi = prevNeighbour[i];
    int ni = i;
    vec4f dir = v_sub(groundfarpoints[ni], groundfarpoints[pi]);
    planes2d[planes2dCnt] = v_or(v_and(v_perm_zxyw(dir), (vec4f)V_CI_MASK1000), v_and(v_neg(v_splat_x(dir)), (vec4f)V_CI_MASK0010));
    // planes2d[planes2dCnt] = v_make_vec4f(v_extract_z(points[ni]) - v_extract_z(points[pi]),
    //   0, v_extract_x(points[pi])-v_extract_x(points[ni]), 0);
    planes2d[planes2dCnt] =
      v_or(planes2d[planes2dCnt], v_and((vec4f)V_CI_MASK0001, v_neg(v_dot3(planes2d[planes2dCnt], groundfarpoints[pi]))));

    planes2d[planes2dCnt] = v_sel(planes2d[planes2dCnt], v_neg(planes2d[planes2dCnt]),
      v_cmp_gt(v_zero(), v_plane_dist(planes2d[planes2dCnt], points[ni | 4])));
    // if (v_test_vec_x_lt_0(v_plane_dist_x(planes2d[planes2dCnt], points[ni|4])))
    //   planes2d[planes2dCnt] = v_neg(planes2d[planes2dCnt]);
    planes2dCnt++;
  }
  for (int i = 4; i < 8; ++i)
  {
    if (planeSign[0] + planeSign[planesOrder[i & 3]] != 0)
      continue;
    int pi = prevNeighbour[i & 3] | 4;
    int ni = i;
    // todo: find intersection with highest possible land (bounding box of level)
    vec4f dir = v_sub(points[ni], points[pi]);
    planes2d[planes2dCnt] = v_or(v_and(v_perm_zxyw(dir), (vec4f)V_CI_MASK1000), v_and(v_neg(v_splat_x(dir)), (vec4f)V_CI_MASK0010));
    // planes2d[planes2dCnt] = v_make_vec4f(v_extract_z(points[ni]) - v_extract_z(points[pi]),
    //   0, v_extract_x(points[pi])-v_extract_x(points[ni]), 0);
    planes2d[planes2dCnt] = v_or(planes2d[planes2dCnt], v_and((vec4f)V_CI_MASK0001, v_neg(v_dot3(planes2d[planes2dCnt], points[pi]))));
    // if (v_test_vec_x_lt_0(v_plane_dist_x(planes2d[planes2dCnt], groundfarpoints[ni&3])))
    //   planes2d[planes2dCnt] = v_neg(planes2d[planes2dCnt]);
    planes2d[planes2dCnt] = v_sel(planes2d[planes2dCnt], v_neg(planes2d[planes2dCnt]),
      v_cmp_gt(v_zero(), v_plane_dist(planes2d[planes2dCnt], groundfarpoints[ni & 3])));
    planes2dCnt++;
  }
}

__forceinline void prepare_2d_frustum(const TMatrix4 &globtm, plane3f *planes2d, int &planes2dCnt)
{
  mat44f v_globtm;
  v_globtm.col0 = v_ldu(globtm.m[0]);
  v_globtm.col1 = v_ldu(globtm.m[1]);
  v_globtm.col2 = v_ldu(globtm.m[2]);
  v_globtm.col3 = v_ldu(globtm.m[3]);
  v_mat44_transpose(v_globtm, v_globtm);

  plane3f camPlanes[6];
  calc_frustum_planes(camPlanes, v_globtm);
  int planeSign[6];
  calc_planes_ground_signs(planeSign, camPlanes);

  vec3f points[8];

  calc_frustum_points(points, camPlanes);

  prepare_2d_frustum(points, points, planeSign, planes2d, planes2dCnt);
}

__forceinline bool is_2d_frustum_visible(vec4f pos, vec4f width, vec4f height, const plane3f *planes2d, int &planes2dCnt)
{
  vec4f tilePoints[4];
  // cx = (cx>>mip)<<mip;
  // cy = (cy>>mip)<<mip;
  tilePoints[0] = pos;
  tilePoints[1] = v_add(tilePoints[0], width);
  tilePoints[2] = v_add(tilePoints[0], height);
  tilePoints[3] = v_add(tilePoints[1], height);
#if !_TARGET_SIMD_SSE
  vec4f invisible = v_zero();
#endif
  for (int pi = 0; pi < planes2dCnt; ++pi)
  {
    vec4f result;
    result = v_plane_dist_x(planes2d[pi], tilePoints[0]);
    result = v_and(result, v_plane_dist_x(planes2d[pi], tilePoints[1]));
    result = v_and(result, v_plane_dist_x(planes2d[pi], tilePoints[2]));
    result = v_and(result, v_plane_dist_x(planes2d[pi], tilePoints[3]));
#if _TARGET_SIMD_SSE
    result = v_and(V_CI_SIGN_MASK, result);
    if ((_mm_movemask_ps(result) & 1))
    {
      return false;
    }
#else
    invisible = v_or(invisible, result);
#endif
  }
#if !_TARGET_SIMD_SSE
  // invisible = v_cmp_eqi(v_and(invisible, V_CI_SIGN_MASK), V_CI_SIGN_MASK);
  invisible = v_and(invisible, v_msbit());
  if (!v_test_vec_x_eqi_0(invisible))
  {
    return false;
  }
#endif
  return true;
}


template <typename T>
static void sort_tile_info_list(const eastl::span<T> tileInfo0)
{
  for (T &tile : tileInfo0)
    tile.updateOrder();
  stlsort::sort(tileInfo0.begin(), tileInfo0.end(), T::sortByOrder);
}

static bool compare_tile_info_results(const eastl::span<TexTileInfoOrder> tileInfo0, const eastl::span<TexTileInfoOrder> tileInfo1,
  int &firstMismatchId)
{
  firstMismatchId = -1;
  if (tileInfo0.size() != tileInfo1.size())
    return false;
  if (tileInfo0.size() == 0)
    return true;
  for (int i = 0; i < tileInfo0.size(); ++i)
    if (tileInfo0[i] != tileInfo1[i])
    {
      firstMismatchId = i;
      return false;
    }
  return true;
}


template <typename Funct>
static void processTilesAroundCamera(int tex_mips, const TMatrix4 &globtm, const TMatrix4 *mirror_globtm_opt, const Point4 &uv2l,
  const Point4 &l2uv, const Point3 &viewerPosition, const Point4 &l2uv0, int ri_offset, float dist_to_ground,
  const MipPosition &mippos, Funct funct)
{
  auto insertTilesTerrain = [&globtm, mirror_globtm_opt, uv2l, funct, &mippos](IPoint2 tile_center, int mipToAdd, int tilesAround) {
    carray<plane3f, 12> planes2d;
    int planes2dCnt = 0;
    prepare_2d_frustum(globtm, planes2d.data(), planes2dCnt);

    carray<plane3f, 12> mirrorPlanes2d;
    int mirrorPlanes2dCnt = 0;
    if (mirror_globtm_opt)
      prepare_2d_frustum(*mirror_globtm_opt, mirrorPlanes2d.data(), mirrorPlanes2dCnt);

    static constexpr int ri_offset = TERRAIN_RI_OFFSET;
    Point4 uv2landscapetile(uv2l.x / TILE_WIDTH, uv2l.y / TILE_WIDTH, uv2l.z, uv2l.w);
    vec4f width = v_make_vec4f(uv2landscapetile.x * (1 << mipToAdd), 0, 0, 0);
    vec4f height = v_make_vec4f(0, 0, uv2landscapetile.y * (1 << mipToAdd), 0);
    IPoint2 uv = TexTilePos::getHigherMipProjectedPos(tile_center, 0, mippos, mipToAdd);
    for (int y = -tilesAround; y <= tilesAround; ++y)
    {
      int cy = uv.y + y;
      if (cy >= -HALF_TILE_WIDTH && cy < HALF_TILE_WIDTH)
      {
        for (int x = -tilesAround; x <= tilesAround; ++x)
        {
          int cx = uv.x + x;
          if (cx >= -HALF_TILE_WIDTH && cx < HALF_TILE_WIDTH)
          {
            // software 2d culling to save some cache
            TexTilePos tile{cx, cy, mipToAdd, ri_offset};
            IPoint2 worldCi = tile.projectOnLowerMip(mippos, 0).lim[0];
            vec4f pos = v_make_vec4f(uv2landscapetile.x * worldCi.x + uv2landscapetile.z, 0,
              uv2landscapetile.y * worldCi.y + uv2landscapetile.w, 0);

            if (planes2dCnt == 0 || is_2d_frustum_visible(pos, width, height, planes2d.data(), planes2dCnt) ||
                (mirror_globtm_opt &&
                  (mirrorPlanes2dCnt == 0 || is_2d_frustum_visible(pos, width, height, mirrorPlanes2d.data(), mirrorPlanes2dCnt))))
              funct({cx, cy, mipToAdd, ri_offset});
          }
        }
      }
    }
  };
  auto insertTilesRI = [ri_offset, funct, &mippos](IPoint2 tile_center, int mipToAdd, int tilesAround) {
    G_ASSERT(ri_offset != TERRAIN_RI_OFFSET);
    IPoint2 uv = TexTilePos::getHigherMipProjectedPos(tile_center, 0, mippos, mipToAdd);
    for (int y = -tilesAround; y <= tilesAround; ++y)
    {
      int cy = uv.y + y;
      if (cy >= -HALF_TILE_WIDTH && cy < HALF_TILE_WIDTH)
      {
        for (int x = -tilesAround; x <= tilesAround; ++x)
        {
          int cx = uv.x + x;
          if (cx >= -HALF_TILE_WIDTH && cx < HALF_TILE_WIDTH)
          {
            // TODO: bbox culling for RI
            funct({cx, cy, mipToAdd, ri_offset});
          }
        }
      }
    }
  };
  auto calcTileCenter = [viewerPosition](const Point4 &l2uv) {
    Point2 viewerUV = Point2(viewerPosition.x * l2uv.x + l2uv.z, viewerPosition.z * l2uv.y + l2uv.w);
    return IPoint2(floor(viewerUV.x * TILE_WIDTH), floor(viewerUV.y * TILE_WIDTH));
  };

  if (ri_offset != TERRAIN_RI_OFFSET)
  {
    IPoint2 tileCenter = calcTileCenter(l2uv);
    const int lastMip = tex_mips - 1;
    const int tilesAroundMultiplier = ri_offset >= RI_VTEX_SECONDARY_OFFSET_START ? 2 : 1;

    // insert the least detailed tiles to RI indirection (so we never need to use the terrain fallback)
    insertTilesRI(IPoint2::ZERO, lastMip, ClipmapImpl::NUM_TILES_AROUND_LASTMIP);

    // insert tiles close to camera to RI indirection
    if (dist_to_ground < ri_fake_tile_max_height_0)
      insertTilesRI(tileCenter, ClipmapImpl::FAKE_TILE_MIP_AROUND_CAMERA_0, ClipmapImpl::NUM_TILES_AROUND_0 * tilesAroundMultiplier);
    if (dist_to_ground < ri_fake_tile_max_height_1)
      insertTilesRI(tileCenter, ClipmapImpl::FAKE_TILE_MIP_AROUND_CAMERA_1, ClipmapImpl::NUM_TILES_AROUND_1 * tilesAroundMultiplier);
  }
  else
  {
    IPoint2 tileCenter = calcTileCenter(l2uv0);
    insertTilesTerrain(tileCenter, ClipmapImpl::FAKE_TILE_MIP_AROUND_CAMERA_0, ClipmapImpl::NUM_TILES_AROUND_0);
    insertTilesTerrain(tileCenter, ClipmapImpl::FAKE_TILE_MIP_AROUND_CAMERA_1, ClipmapImpl::NUM_TILES_AROUND_1);
  }
}

void ClipmapImpl::scheduleUpdateFromFeedback(CaptureMetadata capture_metadata, const TMatrix4 &globtm,
  const TMatrix4 *mirror_globtm_opt, bool force_update)
{
  TIME_PROFILE(clipmap_scheduleUpdateFromFeedback);
  G_ASSERT(capture_metadata.isValid());

  if (!use_uav_feedback)
  {
    prepareTileFeedbackFromTextureFeedback();
  }

  processTileFeedback(capture_metadata, globtm, mirror_globtm_opt);

  if (tileInfoSize)
  {
    scheduleUpdate(capture_metadata, force_update);
  }
}


void ClipmapImpl::prepareTileFeedbackFromTextureFeedback()
{
  if (use_ri_vtex)
    prepareTileFeedbackFromTextureFeedback<true>();
  else
    prepareTileFeedbackFromTextureFeedback<false>();
}

template <bool ri_vtex>
void ClipmapImpl::prepareTileFeedbackFromTextureFeedback()
{
  // we do not have correct feedback for a stub driver, and random data
  // will cause the memory corruption
  if (d3d::is_stub_driver() || disable_feedback)
  {
    preparedTilesFeedback.resize(0);
    return;
  }

  TIME_PROFILE_DEV(clipmap_updateFromReadPixels);
  G_ASSERT(hist.p);

  preparedTilesFeedback.resize(getMaxTileCaptureInfoElementCount());
  int tilesSize = 0;

  hist.clear();

  auto appendHist = [this](const TexTilePos &tilePos) {
    int hist_ofs = tilePos.getTileTag();
    G_ASSERTF(hist_ofs < hist.size, "%d, %d, %d, %d", tilePos.x, tilePos.y, tilePos.mip, tilePos.ri_offset);
    hist.p[hist_ofs]++;
  };

  int texMips = currentContext->lockedCaptureMetadata.texMipCnt;

  G_ASSERT(currentContext->lockedPixelsMemCopy);
  for (const TexTileFeedback &fb : Image2DView<const TexTileFeedback>(const_cast<uint8_t *>(currentContext->lockedPixelsMemCopy),
         feedbackSize.x, feedbackSize.y, currentContext->lockedPixelsBytesStride, TexTileFeedback::TILE_FEEDBACK_FORMAT))
  {
    if (fb.mip < texMips)
      appendHist({(int)fb.x - HALF_TILE_WIDTH, (int)fb.y - HALF_TILE_WIDTH, fb.mip, fb.getRiOffset<ri_vtex>()});
  }

  {
    TIME_PROFILE_DEV(clipmap_processHist);
    // According to C++17 specs, TexTileInfo are constructed in-place.
    uint16_t *__restrict pHist = static_cast<uint16_t *>(hist.p);
    G_ASSERT(!(((intptr_t)pHist) & 0xF));
    G_STATIC_ASSERT(!(TILE_WIDTH & 7));
    vec4f *__restrict pHistVec = (vec4f *)pHist;
    for (uint32_t mip = 0; mip < texMips && tilesSize + 8 < preparedTilesFeedback.size(); mip++)
      for (uint32_t ri_offset = 0; ri_offset < max_ri_offset; ri_offset++) // -V1008
      {
        static constexpr int X_MASK = (1 << TILE_WIDTH_BITS) - 1;
        for (uint32_t i = 0; i < (TILE_WIDTH * TILE_WIDTH) && tilesSize + 8 < preparedTilesFeedback.size(); i += 8, pHistVec++)
        {
          if (v_test_all_bits_zeros(*pHistVec))
            continue;
          int y = ((i >> TILE_WIDTH_BITS) & X_MASK) - HALF_TILE_WIDTH;
          int x = (i & X_MASK) - HALF_TILE_WIDTH;
          uint32_t *chist = ((uint32_t *)pHistVec);
          uint32_t count2;

          count2 = chist[0];
          if (uint32_t count = (count2 & 0xffff))
            preparedTilesFeedback[tilesSize++] = TexTileInfo(x, y, mip, ri_offset, count);
          if (uint32_t count = (count2 >> 16))
            preparedTilesFeedback[tilesSize++] = TexTileInfo(x + 1, y, mip, ri_offset, count);

          count2 = chist[1];
          if (uint32_t count = (count2 & 0xffff))
            preparedTilesFeedback[tilesSize++] = TexTileInfo(x + 2, y, mip, ri_offset, count);
          if (uint32_t count = (count2 >> 16))
            preparedTilesFeedback[tilesSize++] = TexTileInfo(x + 3, y, mip, ri_offset, count);

          count2 = chist[2];
          if (uint32_t count = (count2 & 0xffff))
            preparedTilesFeedback[tilesSize++] = TexTileInfo(x + 4, y, mip, ri_offset, count);
          if (uint32_t count = (count2 >> 16))
            preparedTilesFeedback[tilesSize++] = TexTileInfo(x + 5, y, mip, ri_offset, count);

          count2 = chist[3];
          if (uint32_t count = (count2 & 0xffff))
            preparedTilesFeedback[tilesSize++] = TexTileInfo(x + 6, y, mip, ri_offset, count);
          if (uint32_t count = (count2 >> 16))
            preparedTilesFeedback[tilesSize++] = TexTileInfo(x + 7, y, mip, ri_offset, count);
        }
      }
  }

  preparedTilesFeedback.resize(tilesSize);
}

void ClipmapImpl::processTileFeedback(const CaptureMetadata &capture_metadata, const TMatrix4 &globtm,
  const TMatrix4 *mirror_globtm_opt)
{
  TIME_PROFILE_DEV(clipmap_processTileFeedback);

  TempMap<uint32_t, TexTileInfo, MAX_FAKE_TILE_CNT> aroundCameraTileIndexMap;
  if (!disable_fake_tiles && currentContext->LRUPos.getZoom() <= MAX_ZOOM_TO_ALLOW_FAKE_AROUND_CAMERA)
  {
    auto funct = [&aroundCameraTileIndexMap, moreThanFeedback = this->moreThanFeedback](const TexTilePos &tilePos) {
      aroundCameraTileIndexMap.insert(eastl::pair(tilePos.getTileTag(), TexTileInfo(tilePos, moreThanFeedback)));
      if (aroundCameraTileIndexMap.size() > MAX_FAKE_TILE_CNT) // TempMap can overflow
        logerr("clipmap: Max fake tiles count exceeded: %d > %d", aroundCameraTileIndexMap.size(), MAX_FAKE_TILE_CNT);
    };
    prepareFakeTiles(globtm, mirror_globtm_opt, funct);
  }

  G_ASSERT(preparedTilesFeedback.size() <= getMaxTileCaptureInfoElementCount());

  const RiIndices::CompareBitset equalRiIndices = capture_metadata.riIndices.compareEqual(riIndices);

  bool migrationNeeded = capture_metadata.compare(currentContext->LRUPos) == CaptureMetadata::CompResult::ZOOM_OR_OFFSET_CHANGED ||
                         capture_metadata.texMipCnt != currentContext->currentTexMipCnt;

  tileInfoSize = 0;
  for (auto tile : dag::ConstSpan<TexTileInfo>(preparedTilesFeedback.data(), !disable_feedback ? preparedTilesFeedback.size() : 0))
  {
    if (tile.ri_offset > 0 && (!equalRiIndices[tile.ri_offset - 1] || tile.mip >= getCurrentTexMips(tile.ri_offset)))
      continue;

    if (tile.ri_offset != 0 || !migrationNeeded) {}
    else if (
      auto newTilePos = tile.migrateMipPosition(capture_metadata.position, currentContext->LRUPos, currentContext->currentTexMipCnt))
      tile.TexTilePos::operator=(*newTilePos);
    else
      continue; // We cannot migrate the feedback to current LRUPos.

    uint32_t tag = tile.getTileTag();
    auto it = aroundCameraTileIndexMap.find(tag);
    if (it != aroundCameraTileIndexMap.end())
    {
      TexTileInfo &taggedTileRef = it->second;
      taggedTileRef.count = min(tile.count, moreThanFeedbackInv) + moreThanFeedback;
    }
    else
    {
      tileInfo[tileInfoSize++] = TexTileInfoOrder(tile);
    }
  }

  for (const auto &pair : aroundCameraTileIndexMap)
    tileInfo[tileInfoSize++] = TexTileInfoOrder(pair.second);
}

void ClipmapImpl::updateCache(ClipmapRenderer &renderer)
{
  updatedTilesCnt = 0;

  if (currentContext->updateLRUIndices.empty())
    return;
  TIME_D3D_PROFILE(updateCache);
  SCOPE_RENDER_TARGET;

  beginUpdateTiles(renderer);
  updatedTilesCnt = currentContext->updateLRUIndices.size();
  for (size_t idx : currentContext->updateLRUIndices)
  {
    TexLRU &ti = currentContext->LRU[idx];
    G_ASSERT(ti.flags & TexLRU::NEED_UPDATE);

    updateTileBlock(renderer, ti.cacheCoords, ti.tilePos);
    currentContext->inCacheBitmap[ti.tilePos.getTileTag()] = true;
    currentContext->invalidateIndirection |= (ti.flags & TexLRU::IS_INVALID) != 0;
    ti.flags &= ~(TexLRU::NEED_UPDATE | TexLRU::IS_INVALID);
  }
  currentContext->updateLRUIndices.clear();

  finalizeCompressionQueue();
  endUpdateTiles(renderer);
}

void ClipmapImpl::scheduleUpdate(const CaptureMetadata &capture_metadata, bool force_update)
{
  TIME_PROFILE_DEV(clipmap_scheduleUpdate);
  sortFeedback();
  // currentContext->checkBitmapLRUCoherence(); // debug helper
  applyFeedback(capture_metadata, force_update);
}

void ClipmapImpl::sortFeedback()
{
  TIME_PROFILE_DEV(sortFeedback);

  if (feedbackType != SOFTWARE_FEEDBACK) // no reasonable count
    sort_tile_info_list(eastl::span(tileInfo.begin(), tileInfoSize));

#if DAGOR_DBGLEVEL > 0
  const int LRUsize = currentContext->LRU.size();
  static int maxDroppedTiles = 0;
  if ((always_report_unobtainable_feedback || tileInfoSize - LRUsize > maxDroppedTiles) && tileInfoSize > LRUsize)
  {
    maxDroppedTiles = max(tileInfoSize - LRUsize, maxDroppedTiles);
    logwarn("clipmap: feedback requested: %d tiles, but cache can handle up to %d tiles (%d tiles dropped). In total, dropped tiles "
            "had %d requests.",
      tileInfoSize, LRUsize, tileInfoSize - LRUsize,
      eastl::accumulate(tileInfo.begin() + LRUsize, tileInfo.begin() + tileInfoSize, 0,
        [](int acc, const TexTileInfo &tile) { return acc + tile.count; }));
  }
#endif
}

void MipContext::checkBitmapLRUCoherence() const
{
#if DAGOR_DBGLEVEL > 0
  MipContext::InCacheBitmap inCacheBitmapCheck;
  for (int i = 0; i < LRU.size(); ++i)
  {
    const TexLRU &ti = LRU[i];
    if (ti.tilePos.mip < TEX_MIPS)
    {
      uint32_t addr = ti.tilePos.getTileBitIndex();
      G_ASSERT(inCacheBitmap[addr]);
      inCacheBitmapCheck[addr] = true;
    }
  }
  G_ASSERT(inCacheBitmap == inCacheBitmapCheck);
#endif
}

template <typename T, std::size_t pow>
eastl::array<T, pow + 1> constexpr get_pow_lut(T base)
{
  eastl::array<T, pow + 1> result{};
  T temp = 1;
  for (T &p : result)
  {
    p = temp;
    temp *= base;
  }
  return result;
};

// used at cascadeScore
struct TileScore
{
  uint16_t cascadeCount = 0;
  uint16_t finalCount = 0;
  uint8_t distFirstInCacheMip = TEX_MIPS;

  // We are using coefficient=7/8=0.875.
  // That mean that mip level X, compared to higher mip level Y, is (1 - (7/8)^(Y - X))*100% more visually pleasant.
  // For example mip X, compared to X + 1 mip, is 12.5% more visually pleasant. Compared to X + 2 mip, +23.4%.
  // 3/4=0.75 also behaved good. (numerator = 3, leftShift = 2)
  // Increasing numerator is susceptible to uint32_t overflow (look samplescap). Better use floats?
  static constexpr uint32_t numerator = 7;
  static constexpr uint32_t rightShift = 3; // denominator = 1<<3 = 8
  static constexpr auto powlut = get_pow_lut<uint64_t, TEX_MIPS - 1>(numerator);
  static constexpr uint64_t samplesCap = uint64_t(-1) / powlut.back();
  G_STATIC_ASSERT(samplesCap > uint16_t(-1));        // Purpose 1: cascadeCount * powlut[dist] won't overflow.
  G_STATIC_ASSERT((TEX_MIPS - 1) * rightShift < 64); // Purpose 2: 1u << dist * rightShift won't overflow

  void updateFinalCount()
  {
    finalCount = cascadeCount;
    if (distFirstInCacheMip < TEX_MIPS)
    {
      // Shifting << on 64bit register to avoid overflow ("Purpose 2" above)
      const auto &dist = distFirstInCacheMip;
      finalCount = max<int>(cascadeCount * (((uint64_t)1 << (dist * rightShift)) - powlut[dist]) >> (dist * rightShift), 1);
    }
  }
  uint16_t getFurtherCascadedCount(unsigned dist) const
  {
    G_FAST_ASSERT(dist >= 0 && dist < TEX_MIPS);
    // Multiplying in 64bit registers to avoid overflow ("Purpose 1" above)
    return ((uint64_t)cascadeCount * powlut[dist]) >> (dist * rightShift);
  }
};

void ClipmapImpl::applyFeedback(const CaptureMetadata &capture_metadata, const bool force_update)
{
  TIME_PROFILE_DEV(applyFeedback);

  const int LRUSize = currentContext->LRU.size();
  const int newSize = min<int>(tileInfoSize, LRUSize);
  G_ASSERT(LRUSize <= MAX_CACHE_TILES);
  G_ASSERT(currentContext->updateLRUIndices.empty()); // LRU matches inCacheBitmap.

  if (triggerUpdatesManually && !(bool)(manualUpdates))
    return;
  manualUpdates = max(0, manualUpdates - 1);

  TexLRUList &LRU = currentContext->LRU;
  const bool softwareFeedback = feedbackType == SOFTWARE_FEEDBACK;

  const RiIndices::CompareBitset equalRiIndices = capture_metadata.riIndices.compareEqual(riIndices);

  // If feedback is not up-to-speed with new RI indices changes, then we should "blindly" follow fake tiles info.
  auto feedbackIsRepresentative = [&equalRiIndices](int ri_offset) {
    return ri_offset == TERRAIN_RI_OFFSET || equalRiIndices[ri_offset - 1];
  };

  eastl::fixed_vector<TexTilePos, MAX_FAKE_TILE_CNT, false> invisibleFakeTilesNotInLRU;
  eastl::fixed_vector<TexTilePos, MAX_FAKE_TILE_CNT, false> fakeTilesNotInLRU;
  eastl::fixed_vector<TexTileInfo, MAX_CACHE_TILES, false> tilesNotInLRU;
  using TilesMap = ska::flat_hash_map<uint32_t, uint16_t, eastl::hash<uint32_t>, eastl::equal_to<uint32_t>, framemem_allocator>;
  TilesMap tileToCountInLRU;
  tileToCountInLRU.reserve(newSize);
  for (int fbi = 0; fbi < newSize; ++fbi)
  {
    TexTileInfo &fb = tileInfo[fbi];
    uint32_t addr = fb.getTileBitIndex();
    if (currentContext->inCacheBitmap[addr])
    {
      tileToCountInLRU[fb.getTileTag()] = fb.count;
    }
    else
    {
      if (fb.count < moreThanFeedback || softwareFeedback)
        tilesNotInLRU.emplace_back(fb);
      else if (fb.count > moreThanFeedback || !feedbackIsRepresentative(fb.ri_offset))
        fakeTilesNotInLRU.emplace_back(fb);
      else
        invisibleFakeTilesNotInLRU.emplace_back(fb);
    }
  }

  eastl::fixed_vector<TexLRU, MAX_CACHE_TILES, false> superfluousLRU;
  eastl::fixed_vector<eastl::pair<uint16_t, uint16_t>, 32, true> needUpdateInLRUIndicesCount;
  {
    eastl::fixed_vector<TexLRU, MAX_CACHE_TILES, false> usedLRU;
    for (TexLRU &ti : LRU)
    {
      auto tileToCount = tileToCountInLRU.end();
      if (ti.tilePos.mip >= TEX_MIPS || (tileToCount = tileToCountInLRU.find(ti.tilePos.getTileTag())) == tileToCountInLRU.end())
      {
        ti.flags &= ~ti.IS_USED;
        superfluousLRU.push_back(ti);
      }
      else
      {
        ti.flags |= ti.IS_USED;
        usedLRU.emplace_back(ti);
        if (
          (ti.flags & ti.NEED_UPDATE) && (needUpdateInLRUIndicesCount.size() < needUpdateInLRUIndicesCount.max_size() || force_update))
        {
          auto count = tileToCount->second;
          if (ti.tilePos.ri_offset >= RI_VTEX_SECONDARY_OFFSET_START)
            count = eastl::numeric_limits<decltype(count)>::max(); // top priority for secondary vtex (eg. portal baking)
          else if (!softwareFeedback && count == moreThanFeedback)
            count = 0; // invisible fake tile -> force update last
          needUpdateInLRUIndicesCount.emplace_back(usedLRU.size() - 1, count);
        }
      }
    }
    LRU.resize(usedLRU.size());
    memcpy(LRU.data(), usedLRU.data(), data_size(LRU));
  }

  auto stealSuperfluousLRU = [&]() -> TexLRU // Take an atlas entry.
  {
    G_ASSERTF_RETURN(superfluousLRU.size(), TexLRU(), "superfluousLRU.size()=%d > 0, lru = %d, newSize = %d", superfluousLRU.size(),
      LRU.size(), newSize);
    const TexLRU &ti = superfluousLRU.back();
    if (ti.tilePos.mip < TEX_MIPS)
    {
      uint32_t addr = ti.tilePos.getTileBitIndex();
      G_ASSERT(currentContext->inCacheBitmap[addr]);
      currentContext->inCacheBitmap[addr] = false;
    }
    TexLRU nullLRU{ti.cacheCoords};
    superfluousLRU.pop_back();
    return nullLRU;
  };

  auto setLRU = [](TexLRU &LRU, const TexTilePos &tile_pos, bool is_used) -> void {
    LRU.tilePos = tile_pos;
    LRU.flags = TexLRU::NEED_UPDATE | TexLRU::IS_INVALID | (is_used ? TexLRU::IS_USED : 0);
  };

  int totalHeadroom = !force_update ? maxTileUpdateCount : MAX_CACHE_TILES;

  if (secondaryFeedbackCenter)
    totalHeadroom = MAX_FAKE_TILE_CNT; // TODO: we only need secondary feedback immediately, also, we can make it multi-frame

  auto &updateLRUIndices = currentContext->updateLRUIndices;
  updateLRUIndices.reserve(totalHeadroom);

  // Add all visible fake tiles that are not in LRU.
  for (int i = 0; totalHeadroom > 0 && i < fakeTilesNotInLRU.size(); i++, totalHeadroom--)
  {
    const TexTilePos &tile = fakeTilesNotInLRU[i];
    TexLRU newLRU = stealSuperfluousLRU();
    setLRU(newLRU, tile, true);
    LRU.emplace_back(newLRU);
    updateLRUIndices.emplace_back(LRU.size() - 1);
  }

  // From the remaining totalHeadroom, at least every 4th tile should be update of existing tile or invisible fake tile draw.
  stlsort::sort(needUpdateInLRUIndicesCount.begin(), needUpdateInLRUIndicesCount.end(),
    [](const auto &lhs, const auto &rhs) { return lhs.second > rhs.second; }); // TODO nth_element?

  int remainHeadroom = max(totalHeadroom / 4, totalHeadroom - static_cast<int>(tilesNotInLRU.size()));
  for (int i = 0; remainHeadroom > 0 && i < needUpdateInLRUIndicesCount.size(); i++, remainHeadroom--)
  {
    G_ASSERT(totalHeadroom);
    totalHeadroom--;

    G_ASSERT(LRU[needUpdateInLRUIndicesCount[i].first].flags & TexLRU::NEED_UPDATE);
    updateLRUIndices.emplace_back(needUpdateInLRUIndicesCount[i].first);
  }
  for (int i = 0; remainHeadroom > 0 && i < invisibleFakeTilesNotInLRU.size(); i++, remainHeadroom--)
  {
    G_ASSERT(totalHeadroom);
    totalHeadroom--;

    const TexTilePos &tile = invisibleFakeTilesNotInLRU[i];
    TexLRU newLRU = stealSuperfluousLRU();
    setLRU(newLRU, tile, false);
    LRU.emplace_back(newLRU);
    updateLRUIndices.emplace_back(LRU.size() - 1);
  }

  constexpr unsigned maxTilesCascadeScoring = 32;
  int newTilesHeadroom = min<int>(totalHeadroom, superfluousLRU.size());
  if (tilesNotInLRU.size() <= newTilesHeadroom || newTilesHeadroom > maxTilesCascadeScoring || softwareFeedback)
  {
    for (int i = 0; i < newTilesHeadroom && i < tilesNotInLRU.size(); i++)
    {
      G_ASSERT(totalHeadroom);
      totalHeadroom--;

      const TexTileInfo &tile = tilesNotInLRU[i];
      TexLRU newLRU = stealSuperfluousLRU();
      setLRU(newLRU, tile, true);
      LRU.emplace_back(newLRU);
      updateLRUIndices.emplace_back(LRU.size() - 1);
    }
  }
  else if (newTilesHeadroom > 0)
  {
    // For the remaining totalHeadroom, find which totalHeadroom tiles in tilesNotInLRU will provide the best
    // visual improvement if updated. When a tile is not available, indirection tex will provide the best mip available.
    // Thus, we can draw a higher LOD first if that can provide a better visual improvement.
    TIME_PROFILE_DEV(cascadeScoring);

    // stealSuperfluousLRU now in order to update inCacheBitmap.
    G_ASSERT(newTilesHeadroom <= maxTilesCascadeScoring);
    eastl::fixed_vector<TexLRU, maxTilesCascadeScoring, false> stealedLRUs;
    for (int i = 0; i < newTilesHeadroom; ++i)
      stealedLRUs.push_back(stealSuperfluousLRU());

    using MipScoreMap = ska::flat_hash_map<uint16_t, TileScore, eastl::hash<uint16_t>, eastl::equal_to<uint16_t>, framemem_allocator>;
    carray<carray<MipScoreMap, TEX_MIPS>, MAX_VTEX_CNT> scoreMaps;
    auto packPos = [](int8_t x, int8_t y) -> uint16_t { return (static_cast<uint16_t>(x) << 8) | (static_cast<uint16_t>(y) & 0xFF); };
    auto unpackPos = [](uint16_t p) -> eastl::pair<int8_t, int8_t> {
      return {static_cast<int8_t>(p >> 8), static_cast<int8_t>(p & 0xFF)};
    };
    for (const TexTileInfo &tile : tilesNotInLRU)
    {
      if (tile.mip < TEX_MIPS)
        scoreMaps[tile.ri_offset][tile.mip][packPos(tile.x, tile.y)] = {tile.count};
    }

    for (int ri_offset = 0; ri_offset < max_ri_offset; ri_offset++) // -V1008
    {
      if (!isRiOffsetValid(ri_offset))
        continue;

      const MipPosition mippos = getMipPosition(ri_offset);
      const int texMips = getCurrentTexMips(ri_offset);
      for (int mip = 0; mip < texMips; mip++)
      {
        for (auto &posScorePair : scoreMaps[ri_offset][mip])
        {
          const auto [x, y] = unpackPos(posScorePair.first);

          for (int i = 1; i < texMips - mip; i++)
          {
            IPoint2 projectedPos = TexTilePos(x, y, mip, ri_offset).getHigherMipProjectedPos(mippos, mip + i);
            TexTilePos tileMipPos{projectedPos.x, projectedPos.y, (uint8_t)(mip + i), (uint8_t)ri_offset};
            if (currentContext->inCacheBitmap[TexTilePos(tileMipPos).getTileBitIndex()])
            {
              posScorePair.second.distFirstInCacheMip = i;
              break;
            }
          }
          posScorePair.second.updateFinalCount();

          static constexpr int cascadeMaxSearchDepth = 2;
          int cascaseDepth = min(min(cascadeMaxSearchDepth + 1, texMips - mip), (int)posScorePair.second.distFirstInCacheMip);
          for (int i = 1; i < cascaseDepth; i++)
          {
            IPoint2 projectedPos = TexTilePos(x, y, mip, ri_offset).getHigherMipProjectedPos(mippos, mip + i);
            if (auto scoreSearch = scoreMaps[ri_offset][mip + i].find(packPos(projectedPos.x, projectedPos.y));
                scoreSearch != scoreMaps[ri_offset][mip + i].end())
            {
              scoreSearch->second.cascadeCount += posScorePair.second.getFurtherCascadedCount(i);
              break;
            }
          }
        }
      }
    }

    // We have an optimization problem here.
    // We know how much updating each tile will improve visual result. However, updating one tile affects the potential visual
    // improvement other tiles, in the mip chain, will offer. Thus, just picking the tiles with the highest visual improvement
    // potential (aka finalCount) is not the best solution. We should consider that affect.
    // A greedy algorithm follows...
    struct TexTilePosScore : TexTilePos
    {
      TileScore score;

      TexTilePosScore() = default;
      TexTilePosScore(int X, int Y, int MIP, int RI_OFFSET, const TileScore &SCORE) : TexTilePos(X, Y, MIP, RI_OFFSET), score(SCORE) {}

      bool operator<(const TexTilePosScore &rhs) const { return this->score.finalCount < rhs.score.finalCount; }
      bool operator>(const TexTilePosScore &rhs) const { return this->score.finalCount > rhs.score.finalCount; }
    };

    unsigned candidatesCount = 2 * newTilesHeadroom;
    G_ASSERT(candidatesCount <= 2 * maxTilesCascadeScoring);
    eastl::fixed_vector<TexTilePosScore, 2 * maxTilesCascadeScoring, false> queueHeap(candidatesCount);

    for (int ri_offset = 0; ri_offset < max_ri_offset; ri_offset++) // -V1008
    {
      const int texMips = getCurrentTexMips(ri_offset);
      for (int mip = 0; mip < texMips; mip++)
      {
        for (const auto &posCountPair : scoreMaps[ri_offset][mip])
        {
          auto [x, y] = unpackPos(posCountPair.first);
          const TileScore &score = posCountPair.second;
          G_ASSERT(score.finalCount < moreThanFeedback);
          if (score.finalCount > queueHeap.back().score.finalCount)
          {
            queueHeap.back() = TexTilePosScore(x, y, mip, ri_offset, score);
            eastl::push_heap(queueHeap.begin(), queueHeap.end(), eastl::greater<TexTilePosScore>());
            eastl::pop_heap(queueHeap.begin(), queueHeap.end(), eastl::greater<TexTilePosScore>());
          }
        }
      }
    }

    for (int i = 0; i < newTilesHeadroom; ++i)
    {
      auto maxCountIt = eastl::max_element(queueHeap.begin(), queueHeap.end(), eastl::less<TexTilePosScore>());

      const TexTilePosScore maxTile = *maxCountIt;
      *maxCountIt = queueHeap.back();
      queueHeap.pop_back();

      const MipPosition &mippos = getMipPosition(maxTile.ri_offset);

      // Update queueHeap satisfication.
      // TODO: Break O(n^2) for high queueHeap > 64.
      for (auto &candidateTile : queueHeap)
      {
        if (candidateTile.ri_offset != maxTile.ri_offset || candidateTile.score.finalCount == 0)
          continue;

        int comparisonMip = max(candidateTile.mip, maxTile.mip);
        IPoint2 tileProj = maxTile.getHigherMipProjectedPos(mippos, comparisonMip);
        IPoint2 candidateProj = candidateTile.getHigherMipProjectedPos(mippos, comparisonMip);

        if (tileProj == candidateProj)
        {
          int mipDiff = candidateTile.mip - maxTile.mip;

          G_ASSERT(candidateTile.mip != maxTile.mip);
          G_ASSERT(candidateTile.mip < maxTile.mip || +mipDiff != maxTile.score.distFirstInCacheMip);
          G_ASSERT(candidateTile.mip > maxTile.mip || -mipDiff != candidateTile.score.distFirstInCacheMip);
          if (candidateTile.mip > maxTile.mip && +mipDiff < maxTile.score.distFirstInCacheMip)
          {
            // Let's assume that tile's count has been cascaded. That's always true for cascadeMaxSearchDepth >= texMips - 1.
            // Because maxTile will be rendered, we should subtrack from candidateTile the score it gave by being under it.
            candidateTile.score.cascadeCount =
              max<int>((int)candidateTile.score.cascadeCount - (int)maxTile.score.getFurtherCascadedCount(mipDiff), 1);
            candidateTile.score.updateFinalCount();
          }
          else if (candidateTile.mip < maxTile.mip && -mipDiff < candidateTile.score.distFirstInCacheMip)
          {
            // Because maxTile will be rendered, it will provide a better fallback for candidateTile. We should update its final count.
            candidateTile.score.distFirstInCacheMip = -mipDiff;
            candidateTile.score.updateFinalCount();
          }
        }
      }

      // Add for update.
      G_ASSERT(totalHeadroom);
      totalHeadroom--;

      TexLRU &newLRU = stealedLRUs.back();
      setLRU(newLRU, maxTile, true);
      G_ASSERT(currentContext->inCacheBitmap[newLRU.tilePos.getTileBitIndex()] == 0);
      LRU.emplace_back(newLRU);
      updateLRUIndices.emplace_back(LRU.size() - 1);
      stealedLRUs.pop_back();
    }
  }

  if (use_ri_vtex)
    for (int i = 0; i < changedRiIndices.size(); ++i)
      changedRiIndices[i] = max<int>(changedRiIndices[i] - 1, 0);

  append_items(LRU, superfluousLRU.size(), superfluousLRU.data());
  G_ASSERT(LRU.size() == LRUSize);

  if (clipmap_should_collect_debug_stats())
  {
    clipmapDebugStats.reset();
    clipmapDebugStats.cacheDim = IPoint2(cacheDimX, cacheDimY);
    clipmapDebugStats.texMips = maxTexMipCnt;
    clipmapDebugStats.texTileSize = texTileSize;
    clipmapDebugStats.texTileInnerSize = texTileInnerSize;
    clipmapDebugStats.tilesCount = (cacheDimX / texTileSize) * (cacheDimY / texTileSize);
    clipmapDebugStats.texelSize = pixelRatio;
    clipmapDebugStats.tileZoom = currentContext->LRUPos.getZoom();
    for (int i = 0; i < tileInfoSize; ++i)
    {
      clipmapDebugStats.tilesFromFeedback[tileInfo[i].mip]++;
    }
    for (int i = 0; i < currentContext->LRU.size(); ++i)
    {
      const TexLRU &ti = currentContext->LRU[i];
      if (ti.tilePos.mip >= TEX_MIPS)
      {
        clipmapDebugStats.tilesUnassigned++;
        continue;
      }
      const bool isInvalid = (ti.flags & TexLRU::IS_INVALID) == TexLRU::IS_INVALID;
      const bool needUpdate = (ti.flags & TexLRU::NEED_UPDATE) == TexLRU::NEED_UPDATE;
      const bool isUsed = (ti.flags & TexLRU::IS_USED) == TexLRU::IS_USED;
      if (isInvalid)
        clipmapDebugStats.tilesInvalid[ti.tilePos.mip]++;
      if (needUpdate)
        clipmapDebugStats.tilesNeedUpdate[ti.tilePos.mip]++;
      if (isUsed)
        clipmapDebugStats.tilesUsed[ti.tilePos.mip]++;
      if (!isInvalid && !needUpdate && !isUsed)
        clipmapDebugStats.tilesUnused[ti.tilePos.mip]++;
    }
  }
}

void ClipmapImpl::setFeedbackType(uint32_t ftp)
{
  finalizeFeedback();

  if (ftp == feedbackType)
    return;
  feedbackType = ftp;
  if (ftp == SOFTWARE_FEEDBACK)
    closeHWFeedback();
  else
    initHWFeedback();
}

void ClipmapImpl::setSoftwareFeedbackRadius(int inner_tiles, int outer_tiles)
{
  finalizeFeedback();

  swFeedbackInnerMipTiles = inner_tiles;
  swFeedbackOuterMipTiles = outer_tiles;
}
void ClipmapImpl::setSoftwareFeedbackMipTiles(int mip, int tiles_for_mip)
{
  finalizeFeedback();

  if (mip >= 0 && mip < TEX_MIPS)
  {
    swFeedbackTilesPerMip[mip] = tiles_for_mip;
  }
  else
  {
    logerr("ClipmapImpl::setSoftwareFeedbackMipTiles: mip %d is out of bounds", mip);
  }
}

void MipContext::initHWFeedback()
{
  d3d::GpuAutoLock gpuLock;
  closeHWFeedback();

  captureTargetCounter = 0;
  oldestCaptureTargetCounter = 0;

  for (int i = 0; i < MAX_FRAMES_AHEAD; ++i)
    captureTexFence[i].reset(d3d::create_event_query());

  if (!use_uav_feedback)
  {
    int captureTexFlags = TEXCF_RTARGET | TexTileFeedback::TILE_FEEDBACK_FORMAT;
    for (int i = 0; i < MAX_FRAMES_AHEAD; ++i)
    {
      String captureTexName(128, "capture_tex_rt_%d", i);
      captureTexRt[i] =
        dag::create_tex(nullptr, feedbackSize.x, feedbackSize.y, captureTexFlags, 1, captureTexName.str(), RESTAG_LAND);
      d3d_err(captureTexRt[i].getTex2D());
    }
    if (useFeedbackTexFilter)
    {
      captureTexRtUnfiltered =
        dag::create_tex(nullptr, feedbackSize.x, feedbackSize.y, captureTexFlags, 1, "capture_tex_rt_uf", RESTAG_LAND);
      d3d_err(captureTexRtUnfiltered.getTex2D());

      feedbackTexFilterRenderer = create_postfx_renderer("clipmap_feedback_filter");
      G_ASSERT(feedbackTexFilterRenderer);
    }
    return;
  }

  IPoint2 oversampledFeedbackSize = feedbackSize * feedbackOversampling;
  captureStagingBuf = dag::buffers::create_ua_sr_byte_address(oversampledFeedbackSize.x * oversampledFeedbackSize.y,
    "capture_staging_buf", d3d::buffers::Init::No, RESTAG_LAND);
  d3d_err(captureStagingBuf.getBuf());

  for (int i = 0; i < MAX_FRAMES_AHEAD; ++i)
  {
    String tileInfoName(128, "clipmap_tile_info_buf_%d", i);
    uint32_t subElementCnt = packed_tile_size / d3d::buffers::BYTE_ADDRESS_ELEMENT_SIZE;
    G_ASSERT(subElementCnt > 0);
    G_ASSERT(packed_tile_size % sizeof(uint32_t) == 0);
    captureTileInfoBuf[i] = dag::buffers::create_ua_byte_address_readback(subElementCnt * (maxTilesFeedback + 1u), tileInfoName.str(),
      d3d::buffers::Init::No, RESTAG_LAND);
    d3d_err(captureTileInfoBuf[i].getBuf());
  }
  ShaderGlobal::set_int(var::clipmap_tile_info_buf_max_elements, maxTilesFeedback);

  const size_t histBufSize = clipmap_hist_total_elements;
  intermediateHistBuf =
    dag::buffers::create_ua_sr_byte_address(histBufSize, "clipmap_histogram_buf", d3d::buffers::Init::No, RESTAG_LAND);
  d3d_err(intermediateHistBuf.getBuf());
}

void MipContext::closeHWFeedback()
{
  for (int i = 0; i < MAX_FRAMES_AHEAD; ++i)
  {
    captureTexRt[i].close();
    captureTileInfoBuf[i].close();
    captureTexFence[i].reset();
    captureMetadata[i] = CaptureMetadata();
  }
  lockedCaptureMetadata = CaptureMetadata();

  captureTexRtUnfiltered.close();
  del_it(feedbackTexFilterRenderer);

  captureStagingBuf.close();
  intermediateHistBuf.close();

  free(lockedPixelsMemCopy);
  lockedPixelsMemCopy = nullptr;
  lockedPixelsMemCopySize = 0;
  lockedPixelsBytesStride = 0;
}


void ClipmapImpl::initHWFeedback()
{
  if (!use_uav_feedback)
  {
    hist.alloc();
    feedbackDepthTex = dag::create_tex(nullptr, feedbackSize.x, feedbackSize.y, TEXFMT_DEPTH32 | TEXCF_RTARGET | TEXCF_TC_COMPATIBLE,
      1, "feedback_depth_tex", RESTAG_LAND);
    d3d_err(feedbackDepthTex.getTex2D());
  }
  if (currentContext)
    currentContext->initHWFeedback();
}

void ClipmapImpl::closeHWFeedback()
{
  if (currentContext)
    currentContext->closeHWFeedback();
  hist.close();
  feedbackDepthTex.close();
  preparedTilesFeedback.clear();
}

void ClipmapImpl::setMaxTexMips(int tex_mips)
{
  if (tex_mips == maxTexMipCnt)
    return;
  G_ASSERTF(tex_mips > 3 && tex_mips <= TEX_MIPS, "tex_mips is %d, and should be 4..%d", tex_mips, TEX_MIPS);

  maxTexMipCnt = clamp(tex_mips, (int)4, (int)TEX_MIPS);
  ShaderGlobal::set_int(var::clipmap_max_tex_mips, maxTexMipCnt);

  debug("clipmap: tex_mips now = %d", tex_mips);

  if (currentContext)
  {
    finalizeFeedback();

    currentContext->closeIndirection();
    currentContext->initIndirection(maxTexMipCnt);

    if (feedbackType == CPU_HW_FEEDBACK)
      currentContext->initHWFeedback();

    invalidate(true);
  }
}

void ClipmapImpl::setTileSizeVars()
{
  ShaderGlobal::set_float4(var::g_VTexDim, Color4(TILE_WIDTH * texTileInnerSize, TILE_WIDTH * texTileInnerSize, 0, 0));
  ShaderGlobal::set_float4(var::g_TileSize, Color4(texTileInnerSize, 1.0f / texTileInnerSize, texTileSize, log2f(cacheAnisotropy)));
  float brd = texTileBorder;
  float cpx = 1.0f / cacheDimX;
  float cpy = 1.0f / cacheDimY;
  ShaderGlobal::set_float4(var::g_cache2uv, Color4(cpx, cpy, (brd)*cpx, (brd)*cpy));
}

static int get_platform_adjusted_mip_cnt(int mipCnt, int anisotropy)
{
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_APPLE    // MAC version of ATI cant do an anisotropy with mips properly
#define PS4_ANISO_WA 0 // and caused grids of black stripes.
#else
#define PS4_ANISO_WA 0
  if (anisotropy > 1)
  {
    if (d3d::get_driver_desc().info.vendor == GpuVendor::ATI)
    {
      mipCnt = max(mipCnt, 2); // But ATI on PC requires 2 valid mips for aniso without harsh moire.
    }
  }
#endif

  return mipCnt;
}

void ClipmapImpl::initVirtualTexture(int argCacheDimX, int argCacheDimY, int tex_tile_size, int virtual_texture_mip_cnt,
  int virtual_texture_anisotropy, float argMaxEffectiveTargetResolution)
{
  init_shader_vars();
  for (int ci = bufferCnt; ci < bufferTexVarId.size(); ++ci)
    bufferTexVarId[ci] = ::get_shader_glob_var_id(getCacheBufferName(ci), true);


  maxEffectiveTargetResolution = argMaxEffectiveTargetResolution;
  debug("clipmap: maxEffectiveTargetResolution: %d", maxEffectiveTargetResolution);

  argCacheDimX = min(argCacheDimX, d3d::get_driver_desc().maxtexw);
  argCacheDimY = min(argCacheDimY, d3d::get_driver_desc().maxtexh);
  virtual_texture_mip_cnt = get_platform_adjusted_mip_cnt(virtual_texture_mip_cnt, virtual_texture_anisotropy);

  if (cacheDimX == argCacheDimX && cacheDimY == argCacheDimY && texTileSize == tex_tile_size && mipCnt == virtual_texture_mip_cnt &&
      cacheAnisotropy == virtual_texture_anisotropy && !feedbackPendingChanges) // already inited
    return;
  closeVirtualTexture();
  feedbackPendingChanges = false;

  texTileSize = tex_tile_size;
  mipCnt = virtual_texture_mip_cnt;
  setAnisotropyImpl(virtual_texture_anisotropy);

  debug("clipmap: init mipCnt = %d, texTileSize = %d", mipCnt, texTileSize);

  cacheDimX = argCacheDimX;
  cacheDimY = argCacheDimY;
  tileInfoSize = 0;
  debug("clipmap: cacheDim: %d x %d", cacheDimX, cacheDimY);

  G_ASSERTF(((cacheDimX % texTileSize) == 0), "Cache X size must be evenly divisible by TileSize");
  G_ASSERTF(((cacheDimY % texTileSize) == 0), "Cache Y size must be evenly divisible by TileSize");
  G_ASSERT(cacheDimX * cacheDimY / (texTileSize * texTileSize) <= MAX_CACHE_TILES);

  // context

  constantFillerMat = new_shader_material_by_name("tileConstantFiller");
  G_ASSERT(constantFillerMat);
  if (constantFillerMat)
    constantFillerElement = constantFillerMat->make_elem();
  constantFillerBuf = new DynShaderQuadBuf;
  CompiledShaderChannelId channels[2] = {{SCTYPE_SHORT2, SCUSAGE_POS, 0, 0}, {SCTYPE_E3DCOLOR, SCUSAGE_TC, 0, 0}};
  directUncompressMat = new_shader_material_by_name_optional("direct_cache_bypass");
  if (directUncompressMat)
    directUncompressElem = directUncompressMat->make_elem();

  int maxQuads = (argCacheDimX * argCacheDimY / texTileSize / texTileSize) * 4;
  constantFillerVBuf = new RingDynamicVB();
  constantFillerVBuf->init(maxQuads * TEX_MIPS * 4 * 2, sizeof(E3DCOLOR) * 2, "virtual_tex_ring_vb"); // allow up to 4 frames ahead
                                                                                                      // with no lock
  constantFillerBuf->setRingBuf(constantFillerVBuf);
  constantFillerBuf->init(make_span_const(channels, 2), maxQuads); // allow up to 4 frames ahead with no lock
  constantFillerBuf->setCurrentShader(constantFillerElement);

  currentContext = MipContext();
  currentContext->init(use_uav_feedback, feedbackSize, feedbackOversampling, getMaxTileCaptureInfoElementCount(),
    useFeedbackTexFilter);
  currentContext->initIndirection(maxTexMipCnt);
  currentContext->reset(cacheDimX, cacheDimY, true, texTileSize, feedbackType);

  if (feedbackType == CPU_HW_FEEDBACK)
    initHWFeedback();

  setTileSizeVars();
}

void MipContext::initIndirection(int max_tex_mips)
{
  currentTexMipCnt = max_tex_mips;

  String ind_tex(30, "ind_tex_%p", this);
  unsigned indFlags = TEXFMT_A8R8G8B8 | TEXCF_RTARGET;

  // TODO: we only need half of MAX_VTEX_CNT (with ri vtex),
  // but need to rebuild it after primary/secondary switch,
  // and use offsets to correct it
  indirection = {dag::create_tex(nullptr, TILE_WIDTH * max_tex_mips, TILE_WIDTH * max_ri_offset, indFlags, 1, ind_tex, RESTAG_LAND),
    "indirection_tex"};
}

void MipContext::closeIndirection() { indirection.close(); }

static class PrepareFeedbackJob final : public cpujobs::IJob
{
public:
  ClipmapImpl *clipmap = 0;
  CaptureMetadata captureMetadata;
  TMatrix4 globtm;
  eastl::optional<TMatrix4> mirrorGlobtm;
  bool forceUpdate;

  enum class FeedbackType
  {
    UPDATE_FROM_READPIXELS,
    UPDATE_FROM_SOFTWARE,
    UPDATE_INVALID,
  } updateFeedbackType;

  PrepareFeedbackJob()
  {
    updateFeedbackType = FeedbackType::UPDATE_INVALID;
    forceUpdate = false;
  }
  ~PrepareFeedbackJob() {}
  void initFromHWFeedback(const CaptureMetadata &in_captureMetadata, const TMatrix4 &gtm, const TMatrix4 *mirror_gtm_opt,
    ClipmapImpl &clip_, bool force_update)
  {
    captureMetadata = in_captureMetadata;
    globtm = gtm;
    clipmap = &clip_;
    mirrorGlobtm = mirror_gtm_opt ? eastl::optional(*mirror_gtm_opt) : eastl::nullopt;
    updateFeedbackType = FeedbackType::UPDATE_FROM_READPIXELS;
    forceUpdate = force_update;
  }
  void initFromSoftware(ClipmapImpl &clip_, bool force_update)
  {
    clipmap = &clip_;
    captureMetadata = {};
    updateFeedbackType = FeedbackType::UPDATE_FROM_SOFTWARE;
    forceUpdate = force_update;
  }

  const char *getJobName(bool &) const override { return "PrepareFeedbackJob"; }

  virtual void doJob() override
  {
    if (updateFeedbackType == FeedbackType::UPDATE_FROM_READPIXELS)
    {
      clipmap->scheduleUpdateFromFeedback(captureMetadata, globtm, mirrorGlobtm ? &mirrorGlobtm.value() : nullptr, forceUpdate);
    }
    else if (updateFeedbackType == FeedbackType::UPDATE_FROM_SOFTWARE)
    {
      clipmap->scheduleUpdate(captureMetadata, forceUpdate);
    }
  }
} feedbackJob;

void ClipmapImpl::update(ClipmapRenderer &renderer, bool turn_off_decals_on_fallback)
{
  changeFallback(renderer, viewerPosition, turn_off_decals_on_fallback);
  updateCache(renderer);
  checkConsistency();
  if (currentContext->invalidateIndirection)
    GPUrestoreIndirectionFromLRU();
}

void ClipmapImpl::closeVirtualTexture()
{
  threadpool::wait(&feedbackJob);
  ShaderGlobal::set_texture(var::indirection_tex, BAD_TEXTUREID);

  del_it(directUncompressMat);
  directUncompressElem = 0;

  del_it(constantFillerBuf);
  constantFillerElement = nullptr; // deleted by refcount within constantFillerMat
  del_it(constantFillerMat);

  closeHWFeedback();

  if (currentContext)
    currentContext->closeIndirection();

  for (int ci = 0; ci < cache.size(); ++ci)
  {
    del_it(compressor[ci]);
    cache[ci].close();
  }
  tilesError.clear();

  currentContext.reset();
}

bool ClipmapImpl::migrateLRU(const MipPosition &old_mipPos, const MipPosition &new_mipPos)
{
  carray<TexLRU, MAX_CACHE_TILES> newLRU;
  int newLRUUsed = 0;
  TexLRUList &LRU = currentContext->LRU;
  G_ASSERT(currentContext->updateLRUIndices.empty());
  currentContext->resetInCacheBitmap();
  currentContext->invalidateIndirection = true;
  for (int i = 0; i < LRU.size(); ++i)
  {
    TexLRU &ti = LRU[i];
    bool discard = true;
    if (ti.tilePos.mip < TEX_MIPS)
    {
      if (ti.tilePos.ri_offset != TERRAIN_RI_OFFSET)
      {
        // RI indirection has fixed zoom
        discard = false;
        newLRU[newLRUUsed++] = ti;
        currentContext->inCacheBitmap[ti.tilePos.getTileBitIndex()] = true;
      }
      else if (auto newTexTilePos = ti.tilePos.migrateMipPosition(old_mipPos, new_mipPos, currentContext->currentTexMipCnt))
      {
        discard = false;
        currentContext->inCacheBitmap[newTexTilePos->getTileBitIndex()] = true;
        newLRU[newLRUUsed++] = TexLRU(ti.cacheCoords, *newTexTilePos).setFlags(ti.flags);
      }
    }
    if (discard)
      newLRU[newLRUUsed++] = TexLRU(ti.cacheCoords);
  }
  G_ASSERT(newLRUUsed == LRU.size());
  LRU.resize(newLRUUsed);
  memcpy(LRU.data(), newLRU.data(), data_size(LRU));
  return true;
}

bool MipContext::allocateFallback(int ndim, int tex_tile_size)
{
  const int oDim = fallback.pagesDim;
  if (ndim == oDim)
    return false;

  fallback.pagesDim = ndim;
  fallback.texTileSize = tex_tile_size;
  if (ndim < oDim)
  {
    for (int y = ndim; y < oDim; ++y)
      for (int x = ndim; x < oDim; ++x)
        LRU.push_back(TexLRU({x, y}));
    return false;
  }

  bool changed = false;
  auto destrIt = LRU.begin();
  for (auto it = LRU.begin(), end = LRU.end(); it != end; ++it)
  {
    if (it->cacheCoords.x >= ndim || it->cacheCoords.y >= ndim)
    {
      *(destrIt++) = *it;
      continue;
    }

    // remove it if it used
    if (it->tilePos.mip < TEX_MIPS)
    {
      uint32_t addr = it->tilePos.getTileBitIndex();
      inCacheBitmap[addr] = false;
      changed = true;

      int dist = eastl::distance(LRU.begin(), destrIt);
      auto searchUpdateIndices = eastl::find(updateLRUIndices.begin(), updateLRUIndices.end(), dist);
      if (searchUpdateIndices != updateLRUIndices.end())
        updateLRUIndices.erase(searchUpdateIndices);
      for (auto &idx : updateLRUIndices)
      {
        G_ASSERT(idx != dist);
        if (idx > dist)
          idx--;
      }
    }
  }
  LRU.resize(eastl::distance(LRU.begin(), destrIt));

  invalidateIndirection |= changed;
  return changed;
}

void ClipmapImpl::initFallbackPage(int pages_dim, float texel_size)
{
  if (!VariableMap::isGlobVariablePresent(var::fallback_info0) || !VariableMap::isGlobVariablePresent(var::fallback_info1))
    return;
  fallbackPages = clamp(pages_dim, (int)0, (int)4);
  fallbackTexelSize = clamp(texel_size, 0.0001f, 1e4f);
}

void ClipmapImpl::changeFallback(ClipmapRenderer &renderer, const Point3 &viewer, bool turn_off_decals_on_fallback)
{
  if (currentContext->fallback.pagesDim == 0 && fallbackPages == 0)
    return;
  static constexpr int BLOCK_SZ = 8; // 4 texel align is minimum (block size) alignment, we keep aligned better than that for
                                     // supersampling if ever
  const IPoint2 newOrigin = ipoint2(floor(Point2::xz(viewer) / (BLOCK_SZ * fallbackTexelSize) + Point2(0.5, 0.5))) * BLOCK_SZ;
  if (!currentContext->fallback.sameSettings(fallbackPages, fallbackTexelSize, !turn_off_decals_on_fallback))
  {
    currentContext->fallback.texelSize = fallbackTexelSize;
    currentContext->fallback.drawDecals = !turn_off_decals_on_fallback;
    currentContext->fallback.originXZ = newOrigin + IPoint2(-10000, 10000);
  }
  currentContext->allocateFallback(fallbackPages, texTileSize);

  fallbackFramesUpdate++;

  const IPoint2 amove = abs(currentContext->fallback.originXZ - newOrigin);
  const int moveThreshold = fallbackPages * texTileSize / 16;
  if (fallbackPages && (max(amove.x, amove.y) >= moveThreshold || fallbackFramesUpdate >= 1000))
  {
    TIME_D3D_PROFILE(fallback);
    // render fallback
    fallbackFramesUpdate = 0;
    currentContext->fallback.originXZ = newOrigin;

    SCOPE_RENDER_TARGET;
    const Point3 saveViewerPos = viewerPosition;
    beginUpdateTiles(renderer);
    const float pageMeters = currentContext->fallback.texelSize * texTileSize;
    const float fallBackSize = pageMeters * fallbackPages;
    const Point2 alignedOrigin = currentContext->fallback.getOrigin();
    viewerPosition = Point3::xVy(alignedOrigin, 0.f);
    uint32_t oldFlags = LandMeshRenderer::lmesh_render_flags;
    if (turn_off_decals_on_fallback)
      LandMeshRenderer::lmesh_render_flags &= ~(LandMeshRenderer::RENDER_DECALS | LandMeshRenderer::RENDER_COMBINED);
    for (int y = 0; y < fallbackPages; ++y)
      for (int x = 0; x < fallbackPages; ++x)
      {
        Point2 pageLt = alignedOrigin - Point2(fallBackSize / 2.f, fallBackSize / 2.f) + Point2(x, y) * pageMeters;
        updateTileBlockRaw(renderer, {x, y}, BBox2(pageLt, pageLt + Point2(pageMeters, pageMeters)));
      }
    finalizeCompressionQueue();
    endUpdateTiles(renderer);
    LandMeshRenderer::lmesh_render_flags = oldFlags;
    viewerPosition = saveViewerPos;
  }
}

struct TwoInt16
{
  TwoInt16(int16_t x_, int16_t y_) : x(x_), y(y_) {}
  TwoInt16(IPoint2 p) : x((int16_t)p.x), y((int16_t)p.y) {}
  int16_t x, y;
};
typedef TwoInt16 int16_tx2;

struct ConstantVert
{
  int16_tx2 pos_wh;
  TexTileIndirection tti; // instead of tag, use target mip
};

void ClipmapImpl::GPUrestoreIndirectionFromLRUFull()
{
  const TexLRUList &LRU = currentContext->LRU;
  carray<carray<uint16_t, MAX_CACHE_TILES>, TEX_MIPS> mippedLRU;
  carray<int, TEX_MIPS> mippedLRUCount;
  mem_set_0(mippedLRUCount);
  IBBox2 totalbox;
  int totalUpdated = 0;
  for (int i = 0; i < LRU.size(); ++i)
  {
    const TexLRU &ti = LRU[i];
    if (ti.tilePos.mip >= maxTexMipCnt || (ti.flags & ti.IS_INVALID))
      continue;
    mippedLRU[ti.tilePos.mip][mippedLRUCount[ti.tilePos.mip]++] = i;

    IBBox2 lruIBox = ti.tilePos.projectOnLowerMip(getMipPosition(ti.tilePos.ri_offset), 0);
    totalbox += lruIBox;

    totalUpdated++;
  }
  float ratio = texTileInnerSize * currentContext->LRUPos.getZoom() * pixelRatio;
  currentLRUBox[0] = Point2(totalbox[0] + currentContext->LRUPos.offset) * ratio;
  currentLRUBox[1] = Point2(totalbox[1] + currentContext->LRUPos.offset + IPoint2::ONE) * ratio;

  int maxQuadCount = 0;
  for (int i = 0; i < maxTexMipCnt; ++i)
    maxQuadCount += mippedLRUCount[i] * (i + 1);
  maxQuadCount++;

  ConstantVert *quad = (ConstantVert *)constantFillerBuf->lockData(maxQuadCount);
  if (!quad) // device lost?
  {
    currentContext->invalidateIndirection = true;
    return;
  }

  const IBBox2 indirectionArea{IPoint2(-HALF_TILE_WIDTH, -HALF_TILE_WIDTH), IPoint2(HALF_TILE_WIDTH, HALF_TILE_WIDTH) - IPoint2::ONE};

  const ConstantVert *basequad = quad;
  TexTileIndirection tti({0xff, 0xff}, 0xff, 0xff);
  quad[0].pos_wh = int16_tx2(0, 0);
  quad[0].tti = tti;
  quad[1].pos_wh = int16_tx2(TILE_WIDTH * TEX_MIPS, 0);
  quad[1].tti = tti;
  quad[2].pos_wh = int16_tx2(TILE_WIDTH * TEX_MIPS, TILE_WIDTH * max_ri_offset);
  quad[2].tti = tti;
  quad[3].pos_wh = int16_tx2(0, TILE_WIDTH * max_ri_offset);
  quad[3].tti = tti;
  quad += 4;
  for (int mipI = maxTexMipCnt - 1; mipI >= 0; --mipI)
  {
    for (int i = 0; i < mippedLRUCount[mipI]; ++i)
    {
      const TexLRU &ti = LRU[mippedLRU[mipI][i]];
      G_ASSERT(ti.tilePos.mip == mipI);
      TexTileIndirection tti(ti.cacheCoords, ti.tilePos.mip, 0xFF);
      for (int targI = 0; targI <= mipI; ++targI)
      {
        const IBBox2 tiProj = ti.tilePos.projectOnLowerMip(getMipPosition(ti.tilePos.ri_offset), targI);
        G_ASSERT_CONTINUE(!tiProj.isEmpty());

        IBBox2 tiArea = tiProj; // The area that will indirect to the tile in cache texture. Can be overwriten by smaller mips.
        indirectionArea.clipBox(tiArea);
        if (tiArea.isEmpty())
          continue;

        const IPoint2 offset = {HALF_TILE_WIDTH + targI * TILE_WIDTH, HALF_TILE_WIDTH + ti.tilePos.ri_offset * TILE_WIDTH};
        tiArea.lim[0] += offset;
        tiArea.lim[1] += offset;

        tti.tag = targI;
        quad[0].pos_wh = int16_tx2(tiArea.leftTop() + IPoint2(0, 0));
        quad[0].tti = tti;
        quad[1].pos_wh = int16_tx2(tiArea.rightTop() + IPoint2(1, 0));
        quad[1].tti = tti;
        quad[2].pos_wh = int16_tx2(tiArea.rightBottom() + IPoint2(1, 1));
        quad[2].tti = tti;
        quad[3].pos_wh = int16_tx2(tiArea.leftBottom() + IPoint2(0, 1));
        quad[3].tti = tti;
        quad += 4;
      }
    }
  }

  SCOPE_RENDER_TARGET;
  if (::grs_draw_wire)
    d3d::setwire(false);

  d3d::set_render_target(currentContext->indirection.getTex2D(), 0);
  d3d::clearview(CLEAR_DISCARD_TARGET, 0xFFFFFFFF, 1.f, 0);
  constantFillerBuf->unlockDataAndFlush((quad - basequad) / 4);

  currentContext->invalidateIndirection = false;
  d3d::resource_barrier({currentContext->indirection.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});

  // static int fullId = 0;
  // save_rt_image_as_tga(currentContext->indirection_tex,String(128, "indirection_full_%02d.tga", fullId));
  // fullId++;

  if (::grs_draw_wire)
    d3d::setwire(true);
}

void ClipmapImpl::GPUrestoreIndirectionFromLRU()
{
  if (!currentContext->invalidateIndirection)
    return;
  TIME_D3D_PROFILE(GPUrestoreIndirectionFromLRU);
  GPUrestoreIndirectionFromLRUFull();

  checkConsistency();
}

void ClipmapImpl::recordCompressionErrorStats(bool enable)
{
  recordTilesError = enable;
  tilesError.clear();
  invalidate(true);
}

void ClipmapImpl::dump()
{
  int used = 0;
  if (currentContext)
  {
    debug("zoom = %d, offset.x = %d, offset.y = %d", currentContext->LRUPos.getZoom(), currentContext->LRUPos.offset.x,
      currentContext->LRUPos.offset.y);
    BBox2 box;
    const TexLRUList &LRU = currentContext->LRU;
    float ratio = texTileInnerSize * currentContext->LRUPos.getZoom() * pixelRatio;
    carray<Point4, Clipmap::MAX_TEXTURES> cacheMSE;
    eastl::fill(cacheMSE.begin(), cacheMSE.end(), Point4::ZERO);
    int countMSE = 0;
    for (int i = 0; i < LRU.size(); ++i)
    {
      const TexLRU &ti = LRU[i];
      if (ti.tilePos.mip == 0xFF)
      {
        debug("tile = %d", i);
        continue;
      }

      IBBox2 lruIBox = ti.tilePos.projectOnLowerMip(getMipPosition(ti.tilePos.ri_offset), 0);
      BBox2 lruBox{(Point2)lruIBox.lim[0] * ratio, (Point2)(lruIBox.lim[1] + IPoint2::ONE) * ratio};

      box += lruBox;
      used++;
      debug("tile = %d x =%d y=%d mip= %d, flags = %d tx = %d ty = %d worldbbox = (%g %g) (%g %g)", i, ti.cacheCoords.x,
        ti.cacheCoords.y, ti.tilePos.mip, ti.flags, ti.tilePos.x, ti.tilePos.y, P2D(lruBox[0]), P2D(lruBox[1]));
      // todo: make different priority BETTER_UPDATE and UPDATE_LATER (for not in frustum) and update only if previous priority has
      // been already updated
      if (recordTilesError && ti.tilePos.mip < TEX_MIPS && !(ti.flags & (ti.NEED_UPDATE | ti.IS_INVALID)))
      {
        auto stats = tilesError[ti.cacheCoords];
        for (int ci = 0; ci < cacheCnt; ++ci)
        {
          debug(" - at cache %i, PSNR: R: %f, G: %f, B: %f, A: %f, RGBA: %f", ci, stats[ci].r.PSNR, stats[ci].g.PSNR, stats[ci].b.PSNR,
            stats[ci].a.PSNR, stats[ci].getRGBA().PSNR);
          cacheMSE[ci] += Point4(stats[ci].r.MSE, stats[ci].g.MSE, stats[ci].b.MSE, stats[ci].a.MSE);
        }

        countMSE++;
      }
    }
    debug("total LRU Box = (%g %g) (%g %g)", P2D(box[0]), P2D(box[1]));
    for (int ci = 0; ci < cacheCnt && recordTilesError && countMSE; ++ci)
    {
      BcCompressor::ErrorStats cacheError(cacheMSE[ci] / (float)countMSE);
      debug("in total cache %i, PSNR: R: %f, G: %f, B: %f, A: %f, RGBA: %f", ci, cacheError.r.PSNR, cacheError.g.PSNR,
        cacheError.b.PSNR, cacheError.a.PSNR, cacheError.getRGBA().PSNR);
      if (mipCnt > 1)
        debug("Note: PSNR accounts only for cache mip-level 0. Cache has %i mip levels.", mipCnt);
    }
  }
  debug("used = %d", used);
}

Point3 ClipmapImpl::getMappedRiPos(int ri_offset) const
{
  Point3 mappedCenter = Point3::ZERO;
  if (!use_ri_vtex)
    return mappedCenter;

  G_ASSERTF_RETURN(ri_offset > TERRAIN_RI_OFFSET && ri_offset < max_ri_offset, mappedCenter, "ri_offset = %d", ri_offset);

  if (ri_offset == RI_VTEX_SECONDARY_OFFSET_START)
  {
    if (!secondaryFeedbackCenter)
      return Point3::ZERO;
    return Point3(secondaryFeedbackCenter->x, 0, secondaryFeedbackCenter->z);
  }

  int realRiOffset = ri_offset - 1;

  G_ASSERTF_RETURN(riIndices[realRiOffset] != RI_INVALID_ID, mappedCenter, "ri_offset %d is invalid", ri_offset);

  Color4 rendinstLandscapeAreaLeftTopRightBottom = ShaderGlobal::get_float4(var::rendinst_landscape_area_left_top_right_bottom);
  Point4 riLandscapeTcToWorld = Point4(rendinstLandscapeAreaLeftTopRightBottom.b - rendinstLandscapeAreaLeftTopRightBottom.r,
    rendinstLandscapeAreaLeftTopRightBottom.a - rendinstLandscapeAreaLeftTopRightBottom.g, rendinstLandscapeAreaLeftTopRightBottom.r,
    rendinstLandscapeAreaLeftTopRightBottom.g);

  Point4 mapping = rendinstLandclasses[realRiOffset].mapping;
  Point2 mappedCenterUV = Point2(mapping.z, mapping.w); // only correct if the UVs center is at 0,0
  mappedCenter = Point3(mappedCenterUV.x * riLandscapeTcToWorld.x + riLandscapeTcToWorld.z, 0,
    mappedCenterUV.y * riLandscapeTcToWorld.y + riLandscapeTcToWorld.w);

  return mappedCenter;
};


Point3 ClipmapImpl::getMappedRelativeRiPos(int ri_offset, const Point3 &viewer_position, float &dist_to_ground) const
{
  dist_to_ground = 0;
  if (!use_ri_vtex || ri_offset == TERRAIN_RI_OFFSET)
    return viewer_position;

  G_ASSERTF_RETURN(ri_offset > 0 && ri_offset < max_ri_offset, viewer_position, "ri_offset = %d", ri_offset);
  int realRiOffset = ri_offset - 1;

  G_ASSERTF_RETURN(riIndices[realRiOffset] != RI_INVALID_ID, viewer_position, "ri_offset %d is invalid", ri_offset);

  Point3 relPos;
  float distSq = calc_landclass_to_point_dist_sq(rendinstLandclasses[realRiOffset].matrixInv, viewer_position,
    rendinstLandclasses[realRiOffset].radius, relPos);

  dist_to_ground = sqrt(distSq);

  if (ri_offset == RI_VTEX_SECONDARY_OFFSET_START)
  {
    dist_to_ground = 0;
    return secondaryFeedbackCenter ? *secondaryFeedbackCenter : viewer_position;
  }

  return getMappedRiPos(ri_offset) + relPos;
};

Point4 ClipmapImpl::getLandscape2uv(const IPoint2 &offset, float zoom) const
{
  float pRatio = zoom * pixelRatio;
  Point2 TO = Point2(offset) / TILE_WIDTH_F;
  Point2 iLS;
  iLS.x = 1.0f / (TILE_WIDTH_F * texTileInnerSize);
  iLS.y = 1.0f / (TILE_WIDTH_F * texTileInnerSize);
  Point2 US;
  US.x = 1.0f / pRatio;
  US.y = 1.0f / pRatio;
  // this one is for the regular texture
  Point4 landscape2uv;
  landscape2uv.x = iLS.x * US.x;
  landscape2uv.y = iLS.y * US.y;
  landscape2uv.z = -TO.x;
  landscape2uv.w = -TO.y;
  return landscape2uv;
}

Point4 ClipmapImpl::getLandscape2uv(int ri_offset) const
{
  G_ASSERTF(ri_offset >= TERRAIN_RI_OFFSET && ri_offset < max_ri_offset, "ri_offset = %d", ri_offset);
  return getLandscape2uv(getMipPosition(ri_offset).offset,
    ri_offset == TERRAIN_RI_OFFSET ? currentContext->LRUPos.getZoom() : RI_LANDCLASS_FIXED_ZOOM);
}

Point4 ClipmapImpl::getUV2landacape(const IPoint2 &offset, float zoom) const
{
  Point4 landscape2uv = getLandscape2uv(offset, zoom);

  // uv = pos*A+B
  // pos = (uv-B)/A = uv*iA - B*iA
  Point4 uv2landscape;
  uv2landscape.x = 1.0f / landscape2uv.x;
  uv2landscape.y = 1.0f / landscape2uv.y;
  uv2landscape.z = -landscape2uv.z * uv2landscape.x;
  uv2landscape.w = -landscape2uv.w * uv2landscape.y;
  return uv2landscape;
}

Point4 ClipmapImpl::getUV2landacape(int ri_offset) const
{
  Point4 landscape2uv = getLandscape2uv(ri_offset);

  // uv = pos*A+B
  // pos = (uv-B)/A = uv*iA - B*iA
  Point4 uv2landscape;
  uv2landscape.x = 1.0f / landscape2uv.x;
  uv2landscape.y = 1.0f / landscape2uv.y;
  uv2landscape.z = -landscape2uv.z * uv2landscape.x;
  uv2landscape.w = -landscape2uv.w * uv2landscape.y;
  return uv2landscape;
}

const MipPosition &ClipmapImpl::getMipPosition(int ri_offset) const
{
  G_ASSERT_RETURN(isRiOffsetValid(ri_offset), currentContext->LRUPos);
  if (!use_ri_vtex || ri_offset == TERRAIN_RI_OFFSET)
    return currentContext->LRUPos;

  return riPos[ri_offset - 1];
}

bool ClipmapImpl::isRiOffsetValid(int ri_offset) const
{
  if (!use_ri_vtex || ri_offset == TERRAIN_RI_OFFSET)
    return true;

  return ri_offset > 0 && riIndices[ri_offset - 1] != RI_INVALID_ID;
}

bool Clipmap::isCompressionAvailable(uint32_t format) { return BcCompressor::isAvailable((BcCompressor::ECompressionType)format); }

FeedbackProperties Clipmap::getSuggestedOversampledFeedbackProperties(IPoint2 target_res)
{
  // Compare storing 2x2 vs 3x3 per feedback texel
  int feedbackOversampling = 1;
  int feedbackRegion = -1;
  {
    while (feedbackRegion < 0)
    {
      if (
        div_ceil(target_res.x, 2 * feedbackOversampling) * div_ceil(target_res.y, 2 * feedbackOversampling) <= MAX_FEEDBACK_RESOLUTION)
        feedbackRegion = 2;
      else if (
        div_ceil(target_res.x, 3 * feedbackOversampling) * div_ceil(target_res.y, 3 * feedbackOversampling) <= MAX_FEEDBACK_RESOLUTION)
        feedbackRegion = 3;
      else
        feedbackOversampling *= 2;
    }

    G_ASSERT(feedbackOversampling >= 1 && is_pow_of2(feedbackOversampling) && (feedbackRegion == 2 || feedbackRegion == 3));
  }

  FeedbackProperties feedbackProperties;
  feedbackProperties.feedbackSize = IPoint2(div_ceil(target_res.x, feedbackOversampling * feedbackRegion),
    div_ceil(target_res.y, feedbackOversampling * feedbackRegion));
  feedbackProperties.feedbackOversampling = feedbackOversampling;
  feedbackProperties.useFeedbackTexFilter = false;

  return feedbackProperties;
}

IPoint2 ClipmapImpl::centerOffset(const Point3 &position, float zoom) const
{
  float pRatio = zoom * pixelRatio;
  int xofs = (int)floor(safediv(position.x, pRatio) / texTileInnerSize);
  int yofs = (int)floor(safediv(position.z, pRatio) / texTileInnerSize);

  // Round xofs, yofs. Distance between round points is 2^(ALIGNED_MIPS - 1) tiles.
  IPoint2 ipos;
  ipos.x = (xofs + (1 << (ALIGNED_MIPS - 2))) & (~((1 << (ALIGNED_MIPS - 1)) - 1));
  ipos.y = (yofs + (1 << (ALIGNED_MIPS - 2))) & (~((1 << (ALIGNED_MIPS - 1)) - 1));
  return ipos;
}

// TODO: resurrect it, it's broken now
void ClipmapImpl::checkConsistency()
{
  // NOTE: comment out for the validation
  return;

#if 0
  int lruDimX = cacheDimX / texTileSize;
  int lruDimY = cacheDimY / texTileSize;
  Tab<TexTileInfo> CACHE(tmpmem);
  CACHE.resize(lruDimX * lruDimY);

  // clear
  for ( int y=0; y<lruDimY; ++y )
    for ( int x=0; x<lruDimX; ++x )
      CACHE[x + y*lruDimX] = TexTileInfo(0xffff, 0xffff, 0xffff, 0xffff);

  // fill with LRU, check with indirection
  for ( int i = 0; i < LRU.size(); ++i )
  {
    const TexLRU & lru = LRU[i];
    if ( lru.mip < texMips )
    {
      int m = lru.mip;
      G_ASSERT ( m>=0 && m<texMips && "Mip out of range" );
      TexTileInfo & dl = CACHE[lru.x + lru.y*lruDimX];
      if ( dl.mip < texMips )
      {
        G_ASSERT( dl.x==lru.tx && dl.y==lru.ty && dl.mip==m && "Two different LRU's are pointing at the same tile" );
      }
      dl = TexTileInfo(lru.tx, lru.ty, m, 0);
    }
  }
  for ( int m=0; m<texMips; ++m )
  {
    for ( int x=0; x<TILE_WIDTH; ++x )
    {
      for ( int y=0; y<TILE_WIDTH; ++y )
      {
        const TexTileIndirection & ind = getTile(m, x, y);
        int mip = ind.mip;
        if ( mip < texMips )
        {
          int lrux = (x>>mip)<<mip;
          int lruy = (y>>mip)<<mip;
          const TexTileInfo & cd = CACHE[ind.x + ind.y*lruDimX];
          G_ASSERT ( cd.x==lrux || cd.y==lruy && "Indirection points to element of cache, which does not represent the tile" );
        }
      }
    }
  }

  // clear
  for ( int y=0; y<lruDimY; ++y )
    for ( int x=0; x<lruDimX; ++x )
      CACHE[x + y*lruDimX] = TexTileInfo(0xffff, 0xffff, 0xffff, 0xffff);

  // fill with indirection, check with LRU
  for ( int m=0; m<texMips; ++m )
  {
    for ( int x=0; x<TILE_WIDTH; ++x )
    {
      for ( int y=0; y<TILE_WIDTH; ++y )
      {
        const TexTileIndirection & ind = getTile(m, x, y);
        int mip = ind.mip;
        if ( mip < texMips )
        {
          int lrux = (x>>mip)<<mip;
          int lruy = (y>>mip)<<mip;
          TexTileInfo & cd = CACHE[ind.x + ind.y*lruDimX];
          if ( cd.count==0xffff )
          {
            cd = TexTileInfo ( lrux, lruy, mip, 0 );
          }
          else
          {
            G_ASSERT ( cd.mip==mip && cd.x==lrux && cd.y==lruy
              && "Two different indirections point to the same tile, but expect different values" );
          }
        }
      }
    }
  }
  for ( int i = 0; i < LRU.size(); ++i )
  {
    const TexLRU & lru = LRU[i];
    if ( lru.mip < texMips )
    {
      int m = lru.mip;
      G_ASSERT ( m>=0 && m<texMips && "Mip out of range" );
      TexTileInfo & cd = CACHE[lru.x + lru.y*lruDimX];
      G_ASSERT ( cd.count!=0xffff && "LRU thinks there should be data here" );
      G_ASSERT ( cd.mip==m && cd.x==lru.tx && cd.y==lru.ty && "LRU thinks indirection should point elsewhere" );
      cd.count = 0xffff;
    }
  }
  for ( int y=0; y<lruDimY; ++y )
  {
    for ( int x=0; x<lruDimX; ++x )
    {
      G_ASSERT(CACHE[x + y*lruDimX].count==0xffff && "There is some data hanging in there somehow");
    }
  }

  for ( int m=0; m<texMips; ++m )
  {
    for ( int x=0; x<TILE_WIDTH; ++x )
    {
      for ( int y=0; y<TILE_WIDTH; ++y )
      {
        const TexTileIndirection & ind = getTile(m, x, y);
        G_ASSERT(ind.tag==0xff && "tag must stay intact");
      }
    }
  }
#endif
}

void ClipmapImpl::beginUpdateTiles(ClipmapRenderer &renderer)
{
  d3d::set_render_target();
  for (int i = 0; i < bindedBufferTex.size(); ++i)
    d3d::set_render_target(i, bindedBufferTex[i], 0);
  renderer.startRenderTiles(Point2::xz(viewerPosition));
}

void ClipmapImpl::endUpdateTiles(ClipmapRenderer &renderer) { renderer.endRenderTiles(); }

void ClipmapImpl::finalizeCompressionQueue()
{
  TIME_D3D_PROFILE(compressAndCopyQueue);
  if (!compressionQueue.size())
    return;

  G_ASSERT(bindedBufferTex.size());

  for (int ci = 0; ci < bindedBufferTex.size(); ++ci)
  {
    if (mipCnt > 1)
      bindedBufferTex[ci]->generateMips();

    d3d::resource_barrier({bindedBufferTex[ci], RB_RO_SRV | RB_STAGE_PIXEL, 0, unsigned(mipCnt)});
    // "compressor"'s shaders can use components from different attachments. We should bind them all beforehand.
    ShaderGlobal::set_texture(bufferTexVarId[ci], bindedBufferTex[ci]);
  }

  bool hasUncompressed = false;
  for (int ci = 0; ci < cacheCnt; ++ci)
  {
    if (compressor[ci] && compressor[ci]->isValid())
    {
      TIME_D3D_PROFILE(compressBuffer);
      for (int mipNo = 0; mipNo < mipCnt; mipNo++)
      {
        ShaderGlobal::set_int(var::clipmap_compression_gather4_allowed, mipNo == 0); // gathering on mips != 0 results in artifacts.
        compressor[ci]->updateFromMip(BAD_TEXTUREID, mipNo, mipNo, compressionQueue.size(),
          !useOwnBuffers ? bindedCompressorTex[ci] : nullptr);
      }
    }
    else
    {
      hasUncompressed = true;
    }
  }

  // Get compression error of cache mip0 if such debug option has been enabled
  for (int ci = 0; ci < cacheCnt && recordTilesError; ++ci)
  {
    if (compressor[ci] && compressor[ci]->isValid())
    {
      for (int i = 0; i < compressionQueue.size(); ++i)
      {
        BcCompressor::ErrorStats stats =
          compressor[ci]->getErrorStats(BAD_TEXTUREID, 0, 0, 0, i, !useOwnBuffers ? bindedCompressorTex[ci] : nullptr);
        tilesError[compressionQueue[i]][ci] = stats;
      }
    }
  }

  G_ASSERT(compressionQueue.size() <= COMPRESS_QUEUE_SIZE);
  // fixme: actually we need compressors for each format, not for each texture!
  for (int ci = 0; ci < cacheCnt; ++ci)
  {
    if (compressor[ci] && compressor[ci]->isValid())
    {
      TIME_D3D_PROFILE(copyBufferToCache);
      for (int mipNo = 0; mipNo < mipCnt; mipNo++)
        for (int i = 0; i < compressionQueue.size(); ++i)
          compressor[ci]->copyToMip(cache[ci].getTex2D(), mipNo, (compressionQueue[i].x * texTileSize) >> mipNo,
            (compressionQueue[i].y * texTileSize) >> mipNo, mipNo, (i * texTileSize) >> mipNo, 0, texTileSize >> mipNo,
            texTileSize >> mipNo, bindedCompressorTex[ci]);
    }
  }

  G_ASSERT(!hasUncompressed || directUncompressElem);
  if (hasUncompressed && directUncompressElem)
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target();
    d3d::setvsrc(0, 0, 0);
    StaticTab<Point4, COMPRESS_QUEUE_SIZE> quads;
    quads.resize(compressionQueue.size());
    d3d::set_vs_const1(50, 1.f / COMPRESS_QUEUE_SIZE, 1, float(texTileSize) / cacheDimX, float(texTileSize) / cacheDimY);
    for (int i = 0; i < compressionQueue.size(); ++i)
    {
      quads[i].x = compressionQueue[i].x;
      quads[i].y = compressionQueue[i].y;
      quads[i].z = float(i) / COMPRESS_QUEUE_SIZE;
      quads[i].w = 0;
    }

    for (int mipNo = 0; mipNo < mipCnt; mipNo++)
    {
      ShaderGlobal::set_float(var::src_mip, float(mipNo));
      if (VariableMap::isVariablePresent(var::bypass_cache_no))
      {
        for (int ci = 0; ci < cacheCnt; ++ci)
        {
          TextureInfo texInfo;
          cache[ci].getTex2D()->getinfo(texInfo);
          if (texInfo.cflg & TEXCF_RTARGET)
          {
            d3d::set_render_target(cache[ci].getTex2D(), mipNo);
            ShaderGlobal::set_int(var::bypass_cache_no, ci);
            directUncompressElem->setStates(0, true);
            d3d::set_vs_const1(51, mipNo, 0, 0, 0);
            d3d::set_vs_const(52, &quads[0].x, quads.size());
            d3d::draw(PRIM_TRILIST, 0, quads.size() * 2);
          }
        }
      }
      else
      {
        for (int ci = 0; ci < cacheCnt; ++ci)
        {
          TextureInfo texInfo;
          cache[ci].getTex2D()->getinfo(texInfo);
          d3d::set_render_target(ci, (texInfo.cflg & TEXCF_RTARGET) ? cache[ci].getTex2D() : nullptr, mipNo);
        }
        directUncompressElem->setStates(0, true);
        d3d::set_vs_const1(51, mipNo, 0, 0, 0);
        d3d::set_vs_const(52, &quads[0].x, quads.size());
        d3d::draw(PRIM_TRILIST, 0, quads.size() * 2);
      }
    }

    for (int ci = 0; ci < cacheCnt; ++ci)
    {
      d3d::resource_barrier({cache[ci].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, (unsigned)mipCnt});
    }
  }

  for (int ci = 0; ci < maxBufferCnt; ++ci)
    ShaderGlobal::set_texture(bufferTexVarId[ci], BAD_TEXTUREID);

  compressionQueue.clear();
}

void ClipmapImpl::addCompressionQueue(const CacheCoords &cacheCoords)
{
  if (compressionQueue.size() >= COMPRESS_QUEUE_SIZE)
    finalizeCompressionQueue();
  compressionQueue.push_back(cacheCoords);
}

void ClipmapImpl::updateTileBlockRaw(ClipmapRenderer &renderer, const CacheCoords &cacheCoords, const BBox2 &region)
{
  addCompressionQueue(cacheCoords);
  const bool groupCompress = COMPRESS_QUEUE_SIZE != 1;
  const bool groupClear = groupCompress && compressionQueue.size() == 1;
  if (groupClear)
  {
    // clear entire compression group texture before first tile
    int w, h;
    d3d::get_target_size(w, h);
    d3d::setview(0, 0, w, h, 0, 1);
  }

  if (!groupCompress || groupClear) //-V560
  {
    TIME_D3D_PROFILE(clearTilesTarget);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
  }

  if (COMPRESS_QUEUE_SIZE != 1)
    d3d::setview((compressionQueue.size() - 1) * texTileSize, 0, texTileSize, texTileSize, 0, 1);

  {
    TIME_D3D_PROFILE(renderTile);
    renderer.renderTile(region);
  }
}

void ClipmapImpl::updateTileBlock(ClipmapRenderer &renderer, const CacheCoords &cacheCoords, const TexTilePos &tilePos)
{
  Point4 uv2landscape = getUV2landacape(tilePos.ri_offset);
  IBBox2 tileBox = tilePos.projectOnLowerMip(getMipPosition(tilePos.ri_offset), 0);

  float brd = float(texTileBorder << tilePos.mip) / float(texTileInnerSize);
  Point2 t0 = (Point2(tileBox.lim[0]) - Point2(brd, brd)) / TILE_WIDTH_F;
  Point2 t1 = (Point2(tileBox.lim[1] + IPoint2::ONE) + Point2(brd, brd)) / TILE_WIDTH_F;
  BBox2 region;
  region[0].x = t0.x * uv2landscape.x + uv2landscape.z;
  region[0].y = t0.y * uv2landscape.y + uv2landscape.w;
  region[1].x = t1.x * uv2landscape.x + uv2landscape.z;
  region[1].y = t1.y * uv2landscape.y + uv2landscape.w;
  updateTileBlockRaw(renderer, cacheCoords, region);
}

////////////// END OF VIRTUAL TEXTURE /////////////


void ClipmapImpl::close()
{
  closeVirtualTexture();
  // END OF THE VIRTUAL TEXTURE
}

void ClipmapImpl::init(float texel_size, uint32_t feedback_type, FeedbackProperties feedback_properties, int tex_mips,
  bool use_own_buffers)
{
  finalizeFeedback();

  if (feedbackType != feedback_type || feedbackSize != feedback_properties.feedbackSize ||
      feedbackOversampling != feedback_properties.feedbackOversampling ||
      useFeedbackTexFilter != feedback_properties.useFeedbackTexFilter)
    feedbackPendingChanges = true;
  feedbackType = feedback_type;
  feedbackSize = feedback_properties.feedbackSize;
  feedbackOversampling = feedback_properties.feedbackOversampling;
  moreThanFeedback = feedbackSize.x * feedbackSize.y + 1;
  moreThanFeedbackInv = 0xFFFF - moreThanFeedback;
  useFeedbackTexFilter = feedback_properties.useFeedbackTexFilter;
  useOwnBuffers = use_own_buffers;

  G_ASSERT(feedbackSize.x * feedbackSize.y <= MAX_FEEDBACK_RESOLUTION);
  G_ASSERT((feedbackOversampling == 1 || use_uav_feedback) && is_pow_of2(feedbackOversampling));

  mem_set_0(changedRiIndices);
  riIndices = RiIndices();

  pixelRatio = texel_size;
  setMaxTexMips(tex_mips);
  eastl::fill(swFeedbackTilesPerMip.begin(), swFeedbackTilesPerMip.end(), -1);

  if (use_uav_feedback)
  {
    clearHistogramCs.reset(new_compute_shader("clipmap_clear_histogram_cs"));
    processFeedbackCs.reset(new_compute_shader("clipmap_process_feedback_cs"));
    buildTileInfoCs.reset(new_compute_shader("clipmap_build_tile_info_cs"));
  }

  if (ShaderGlobal::has_associated_interval(var::clipmap_use_ri_vtex) && ShaderGlobal::is_var_assumed(var::clipmap_use_ri_vtex))
    use_ri_vtex = ShaderGlobal::get_interval_assumed_value(var::clipmap_use_ri_vtex);

  debug("clipmap: clipmap_use_ri_vtex = %d", use_ri_vtex);
  packed_tile_size = use_ri_vtex ? sizeof(PackedTile<true>) : sizeof(PackedTile<false>);
  max_ri_offset = use_ri_vtex ? MAX_VTEX_CNT : 1;
  ri_offset_bits = use_ri_vtex ? MAX_RI_VTEX_CNT_BITS : 0;
  clipmap_hist_total_elements = use_ri_vtex ? CLIPMAP_HIST_WITH_RI_VTEX_ELEMENTS : CLIPMAP_HIST_NO_RI_VTEX_ELEMENTS;

  dynamicDDScaleParams.targetAtlasUsageRatio = 0.f;

  if (use_ri_vtex && !riLandclassDataManager)
    riLandclassDataManager = RiLandclassDataManager();

  ShaderGlobal::set_int(var::feedback_downsample_dim, feedbackOversampling);
  ShaderGlobal::set_int(var::clipmap_feedback_stride, feedbackSize.x * feedbackOversampling);
  ShaderGlobal::set_int(var::clipmap_feedback_readback_stride, feedbackSize.x);
  ShaderGlobal::set_int(var::clipmap_feedback_readback_size, feedbackSize.x * feedbackSize.y);
}

void ClipmapImpl::invalidate(bool force_redraw)
{
  finalizeFeedback();
  if (currentContext)
  {
    currentContext->reset(cacheDimX, cacheDimY, force_redraw, texTileSize, feedbackType);
  }
  if (force_redraw)
    currentLRUBox.setempty();
}

void ClipmapImpl::invalidatePointi(const IPoint2 &ipos, int lastMip)
{
  if (!(getMaximumIBBox(TERRAIN_RI_OFFSET) & ipos))
    return;

  threadpool::wait(&feedbackJob);
  if (currentContext)
  {
    TexLRUList &LRU = currentContext->LRU;
    for (auto &ti : LRU)
    {
      if (ti.tilePos.mip >= lastMip)
        continue;

      if (ti.tilePos.ri_offset != TERRAIN_RI_OFFSET)
        continue;

      IBBox2 tiProj = ti.tilePos.projectOnLowerMip(getMipPosition(ti.tilePos.ri_offset), 0);
      if (tiProj & ipos)
        ti.flags |= TexLRU::NEED_UPDATE;
    }
  }
}

void ClipmapImpl::invalidatePoint(const Point2 &world_xz)
{
  Point4 landscape2uv = getLandscape2uv(TERRAIN_RI_OFFSET); // TODO: maybe add support for RI invalidation
  Point2 xz = Point2(landscape2uv.x * world_xz.x, landscape2uv.y * world_xz.y) + Point2(landscape2uv.z, landscape2uv.w);
  invalidatePointi(IPoint2(floor(xz.x * TILE_WIDTH), floor(xz.y * TILE_WIDTH)), 0xFF);
}

void ClipmapImpl::invalidateBox(const BBox2 &world_xz)
{
  Point4 landscape2uv = getLandscape2uv(TERRAIN_RI_OFFSET); // TODO: maybe add support for RI invalidation

  DECL_ALIGN16(Point4, txBox) =
    Point4(landscape2uv.x * world_xz[0].x + landscape2uv.z, landscape2uv.y * world_xz[0].y + landscape2uv.w,
      landscape2uv.x * world_xz[1].x + landscape2uv.z, landscape2uv.y * world_xz[1].y + landscape2uv.w);
  const IPoint2 t0 = {(int)floor(txBox.x * TILE_WIDTH_F), (int)floor(txBox.y * TILE_WIDTH_F)};
  const IPoint2 t1 = {(int)floor(txBox.z * TILE_WIDTH_F), (int)floor(txBox.w * TILE_WIDTH_F)};
  const IBBox2 tBox = {t0, t1};
  if (!(getMaximumIBBox(TERRAIN_RI_OFFSET) & tBox))
    return;

  IPoint2 texelSize =
    IPoint2((txBox.z - txBox.x) * (texTileInnerSize * TILE_WIDTH), (txBox.w - txBox.y) * (texTileInnerSize * TILE_WIDTH));
  int lastMip = 0;
  for (; lastMip < currentContext->currentTexMipCnt; ++lastMip, texelSize.x >>= 1, texelSize.y >>= 1)
    if (texelSize.x == 0 && texelSize.y == 0)
      break;

  if (!lastMip)
    return;
  if (t0 == t1)
  {
    invalidatePointi(t0, lastMip);
    return;
  }

  threadpool::wait(&feedbackJob);

  if (currentContext)
  {
    TexLRUList &LRU = currentContext->LRU;
    for (auto &ti : LRU)
    {
      if (ti.tilePos.mip >= lastMip)
        continue;

      if (ti.tilePos.ri_offset != TERRAIN_RI_OFFSET)
        continue;

      IBBox2 tiProj = ti.tilePos.projectOnLowerMip(getMipPosition(ti.tilePos.ri_offset), 0);
      if (tiProj & tBox)
        ti.flags |= TexLRU::NEED_UPDATE;
    }
  }
}

void ClipmapImpl::prepareSoftwarePoi(const int vx, const int vy, const int mip, const int mipsize,
  carray<uint64_t, (TEX_MIPS * TILE_WIDTH * TILE_WIDTH + 63) / 64> &bitarrayUsed, const Point4 &uv2landscapetile,
  const vec4f *planes2d, int planes2dCnt)
{
  G_ASSERTF(!use_ri_vtex, "software feedback is not supported with RI clipmap indirection"); // TODO: implement it
  static constexpr int ri_offset = 0;
  IPoint2 uv = TexTilePos::getHigherMipProjectedPos(IPoint2(vx, vy), 0, getMipPosition(ri_offset), mip);
  vec4f width = v_make_vec4f(uv2landscapetile.x * (1 << mip), 0, 0, 0);
  vec4f height = v_make_vec4f(0, 0, uv2landscapetile.y * (1 << mip), 0);
  for (int y = -mipsize; y <= mipsize; ++y)
  {
    int cy = uv.y + y; //
    if (!(cy >= -HALF_TILE_WIDTH && cy < HALF_TILE_WIDTH))
      continue;
    for (int x = -mipsize; x <= mipsize; ++x)
    {
      int cx = uv.x + x;
      if (!(cx >= -HALF_TILE_WIDTH && cx < HALF_TILE_WIDTH))
        continue;
      const int type_size = sizeof(bitarrayUsed[0]) * 8;
      const int type_bits_ofs = 6;
      G_STATIC_ASSERT((1 << type_bits_ofs) == type_size);
      uint32_t elementFullAddr =
        (mip << (TILE_WIDTH_BITS + TILE_WIDTH_BITS)) + ((cy + HALF_TILE_WIDTH) << TILE_WIDTH_BITS) + (cx + HALF_TILE_WIDTH);
      uint32_t elementAddr = elementFullAddr >> type_bits_ofs;
      uint64_t bitAddr = uint64_t(1ULL << uint64_t(elementFullAddr & (type_size - 1)));
      if (bitarrayUsed[elementAddr] & bitAddr)
        continue; // fixme: not used in first poi
      // total++;
      if (planes2dCnt > 0)
      {
        TexTilePos tile{cx, cy, mip, ri_offset};
        IPoint2 worldCi = tile.projectOnLowerMip(getMipPosition(ri_offset), 0).lim[0];
        vec4f pos =
          v_make_vec4f(uv2landscapetile.x * worldCi.x + uv2landscapetile.z, 0, uv2landscapetile.y * worldCi.y + uv2landscapetile.w, 0);
        if (!is_2d_frustum_visible(pos, width, height, planes2d, planes2dCnt))
          continue;
      }
      tileInfo[tileInfoSize++] = TexTileInfoOrder(cx, cy, mip, ri_offset, 0xffff);
      bitarrayUsed[elementAddr] |= bitAddr; // fixme: not used in second poi
    }
  }
}

void ClipmapImpl::prepareSoftwareFeedback(int nextZoom, const Point3 &center, const TMatrix &view_itm, const TMatrix4 &globtm,
  float approx_height_above_land, float maxClipMoveDist0, float maxClipMoveDist1, bool force_update)
{
  G_ASSERTF(!use_ri_vtex, "software feedback is not supported with RI clipmap indirection"); // TODO: implement it

  G_UNREFERENCED(nextZoom);
  G_ASSERT(currentContext);

  tileInfoSize = 0;
  // todo: move to thread
  carray<plane3f, 12> planes2d; // fixme 6 planes should be enough, but it is not
  int planes2dCnt = 0;
  Point2 center0;
  Point2 center1;

  if (maxClipMoveDist0 > 0)
  {
    mat44f v_globtm;
    v_globtm.col0 = v_ldu(globtm.m[0]);
    v_globtm.col1 = v_ldu(globtm.m[1]);
    v_globtm.col2 = v_ldu(globtm.m[2]);
    v_globtm.col3 = v_ldu(globtm.m[3]);
    v_mat44_transpose(v_globtm, v_globtm);

    float landPlaneHeight = center.y - approx_height_above_land;
    Point3_vec4 origCenterS = Point3(center.x, landPlaneHeight, center.z);
    vec4f origCenter = v_ldu(&center.x);
    plane3f camPlanes[7];
    calc_frustum_planes(camPlanes, v_globtm);
    camPlanes[6] = v_make_vec4f(0, 1, 0, -landPlaneHeight);
    int planeSign[6];
    calc_planes_ground_signs(planeSign, camPlanes);

    vec3f points[8];

    calc_frustum_points(points, camPlanes);

    /*vec4f groundfarpoints[4]; vec4f behind;
    vec4f groundplane = v_make_vec4f(0,1,0, 0);
    groundfarpoints[0] = v_ray_intersect_plane(points[4], points[0], groundplane, behind, t);
    groundfarpoints[0] = v_sel(groundfarpoints[0], points[0], v_or(behind, v_cmp_gt(t, V_C_ONE)));
    groundfarpoints[1] = v_ray_intersect_plane(points[5], points[1], groundplane, behind, t);
    groundfarpoints[1] = v_sel(groundfarpoints[1], points[1], v_or(behind, v_cmp_gt(t, V_C_ONE)));
    groundfarpoints[2] = v_ray_intersect_plane(points[6], points[2], groundplane, behind, t);
    groundfarpoints[2] = v_sel(groundfarpoints[2], points[2], v_or(behind, v_cmp_gt(t, V_C_ONE)));
    groundfarpoints[3] = v_ray_intersect_plane(points[7], points[3], groundplane, behind, t);
    groundfarpoints[3] = v_sel(groundfarpoints[3], points[3], v_or(behind, v_cmp_gt(t, V_C_ONE)));*/
    // memcpy(groundfarpoints, points, sizeof(groundfarpoints));
    // planes2dCnt = 0;//fixme: no culling
    prepare_2d_frustum(points, points, planeSign, planes2d.data(), planes2dCnt);

    vec4f invalid;
    vec4f t;


    // for (int i = 0; i < planes2dCnt; ++i)
    //   planes2d[i] = v_norm3(planes2d[i]);

    vec4f invalidCenter, lookCenter, half = V_C_HALF;
    vec3f nearC = v_mul(v_add(points[4], points[7]), half);
    vec3f farC = v_mul(v_add(points[0], points[3]), half);
    vec3f lookDir = v_sub(farC, nearC);
    lookCenter = v_ray_intersect_plane(nearC, farC, camPlanes[6], invalidCenter, t);

    vec4f invalids[4];
    points[0] = v_ray_intersect_plane(points[4], points[0], camPlanes[6], invalids[0], t);
    points[1] = v_ray_intersect_plane(points[5], points[1], camPlanes[6], invalids[1], t);
    points[2] = v_ray_intersect_plane(points[6], points[2], camPlanes[6], invalids[2], t);
    points[3] = v_ray_intersect_plane(points[7], points[3], camPlanes[6], invalids[3], t);
    vec3f closePoints[4];
    vec4f invalid_cp[4];
    closePoints[0] = closest_point_on_segment(origCenter, points[1], points[0]);
    closePoints[1] = closest_point_on_segment(origCenter, points[2], points[0]);
    closePoints[2] = closest_point_on_segment(origCenter, points[3], points[1]);
    closePoints[3] = closest_point_on_segment(origCenter, points[3], points[2]);
    invalid_cp[0] = v_or(invalids[0], invalids[1]);
    invalid_cp[1] = v_or(invalids[0], invalids[2]);
    invalid_cp[2] = v_or(invalids[3], invalids[1]);
    invalid_cp[3] = v_or(invalids[3], invalids[2]);


    vec4f distances[4];
    distances[0] = v_length3_sq(v_sub(closePoints[0], origCenter));
    distances[1] = v_length3_sq(v_sub(closePoints[1], origCenter));
    distances[2] = v_length3_sq(v_sub(closePoints[2], origCenter));
    distances[3] = v_length3_sq(v_sub(closePoints[3], origCenter));
    vec4f minDist01 = v_min(v_sel(distances[0], distances[1], invalid_cp[0]), v_sel(distances[1], distances[0], invalid_cp[1]));
    vec4f minDist23 = v_min(v_sel(distances[2], distances[3], invalid_cp[2]), v_sel(distances[3], distances[2], invalid_cp[3]));
    vec4f invalid01 = v_and(invalid_cp[0], invalid_cp[1]), invalid23 = v_and(invalid_cp[2], invalid_cp[3]);
    vec4f invalidAny = v_or(v_or(invalid_cp[0], invalid_cp[1]), v_or(invalid_cp[2], invalid_cp[3]));
    vec4f minDist = v_min(v_sel(minDist01, minDist23, invalid01), v_sel(minDist23, minDist01, invalid23));
    vec4f center = origCenter;
    center = v_sel(closePoints[0], center, v_or(invalid_cp[0], v_cmp_gt(distances[0], minDist)));
    center = v_sel(closePoints[1], center, v_or(invalid_cp[1], v_cmp_gt(distances[1], minDist)));
    center = v_sel(closePoints[2], center, v_or(invalid_cp[2], v_cmp_gt(distances[2], minDist)));
    center = v_sel(closePoints[3], center, v_or(invalid_cp[3], v_cmp_gt(distances[3], minDist)));
    // center is centered on most closest plane to viewer

    // lerp with center of frustum, based on direction of view, but only if whole plane is inside frustum
    vec4f lerpParam = v_max(v_neg(v_splats(view_itm.m[2][1])), v_zero());
    lookCenter = v_sel(lookCenter, center, invalidCenter);
    lookCenter = v_madd(v_sub(lookCenter, center), lerpParam, center);
    center = v_sel(lookCenter, center, invalidAny);

    // now limit maximum center movement to FRUSTUM_MOVE_CLIPMAP_DIST
    vec3f centerMovDir = v_sub(center, origCenter);
    vec4f movDist = v_length3(centerMovDir);
    // if center is in the same place do not divide by zero
    centerMovDir = v_sel(centerMovDir, v_div(centerMovDir, movDist), v_cmp_gt(movDist, v_zero()));

    vec4f movDist0 = v_min(movDist, v_splats(maxClipMoveDist0));
    center = v_add(origCenter, v_mul(centerMovDir, movDist0));
    Point3_vec4 centerRes;
    v_st(&centerRes.x, center);
    center0.x = centerRes.x;
    center0.y = centerRes.z;

    if (maxClipMoveDist0 < maxClipMoveDist1)
    {
      vec4f movDist1 = v_min(movDist, v_splats(maxClipMoveDist1));
      center = v_add(origCenter, v_mul(centerMovDir, movDist1));
      Point3_vec4 centerRes;
      v_st(&centerRes.x, center);
      center1.x = centerRes.x;
      center1.y = centerRes.z;
    }
  }
  else
  {
    center0 = Point2::xz(center);
    prepare_2d_frustum(globtm, planes2d.data(), planes2dCnt);
  }

  Point4 landscape2uv = getLandscape2uv(TERRAIN_RI_OFFSET);
  Point4 uv2landscape = getUV2landacape(TERRAIN_RI_OFFSET);
  Point4 uv2landscapetile(uv2landscape.x / TILE_WIDTH, uv2landscape.y / TILE_WIDTH, uv2landscape.z, uv2landscape.w);

  int vx = floor((center0.x * landscape2uv.x + landscape2uv.z) * TILE_WIDTH_F);
  int vy = floor((center0.y * landscape2uv.y + landscape2uv.w) * TILE_WIDTH_F);
  // G_STATIC_ASSERT(sizeof(uint64_t)*8 == TILE_WIDTH);
  carray<uint64_t, (TEX_MIPS * TILE_WIDTH * TILE_WIDTH + 63) / 64> bitarrayUsed;
  mem_set_0(bitarrayUsed);
  for (int mip = 0; mip < currentContext->currentTexMipCnt; ++mip)
  {
    int mipsize = (mip >= currentContext->currentTexMipCnt - 1) ? swFeedbackOuterMipTiles : swFeedbackInnerMipTiles;
    if (swFeedbackTilesPerMip[mip] >= 0)
    {
      mipsize = swFeedbackTilesPerMip[mip];
    }
    prepareSoftwarePoi(vx, vy, mip, mipsize, bitarrayUsed, uv2landscapetile, planes2d.data(), planes2dCnt);
  }

  if (maxClipMoveDist0 > 0 && maxClipMoveDist0 < maxClipMoveDist1)
  {
    int vx = floor((center1.x * landscape2uv.x + landscape2uv.z) * TILE_WIDTH_F);
    int vy = floor((center1.y * landscape2uv.y + landscape2uv.w) * TILE_WIDTH_F);
    for (int mip = 2; mip < currentContext->currentTexMipCnt; ++mip)
    {
      const int mipsize = 2;
      prepareSoftwarePoi(vx, vy, mip, mipsize, bitarrayUsed, uv2landscapetile, planes2d.data(), planes2dCnt);
    }
  }
  // debug("total = %d invisible = %d", total, totalInvisible);
  // debug("tileInfoSize =%d",tileInfoSize);

  if (tileInfoSize)
  {
    feedbackJob.initFromSoftware(*this, force_update);
    threadpool::add(&feedbackJob);
  }
}

void ClipmapImpl::setDDScale()
{
  G_ASSERT(targetSize.x > 0 && targetSize.y > 0);

  float ddw = float(feedbackSize.x * feedbackOversampling) / float(targetSize.x);
  float ddh = float(feedbackSize.y * feedbackOversampling) / float(targetSize.y);

  if (dynamicDDScaleParams.targetAtlasUsageRatio > 0.f)
  {
    const int tilesTotal = (cacheDimX / texTileSize) * (cacheDimY / texTileSize);
    const float requestedTilesRatio = float(tileInfoSize) / tilesTotal;
    const float ratioDiff = requestedTilesRatio - dynamicDDScaleParams.targetAtlasUsageRatio;
    const bool outsideHysteresisRange = requestedTilesRatio < dynamicDDScaleParams.atlasUsageRatioHysteresisLow ||
                                        requestedTilesRatio > dynamicDDScaleParams.atlasUsageRatioHysteresisHigh;

    constexpr float changeThreshold = 0.01f;
    if (outsideHysteresisRange && (abs(ratioDiff) > changeThreshold))
    {
      const float newDDScale = dynamicDDScale + dynamicDDScaleParams.proportionalTerm * ratioDiff;
      const float minDDScale = exp2f(targetMipBias);
      dynamicDDScale = clamp(newDDScale, minDDScale, 8.f);
    }
    pixelDerivativesScale = dynamicDDScale;
  }
  else
  {
    float targetResolution = float(targetSize.x * targetSize.y);
    float effectiveTargetResolution = targetResolution * exp2f(-2 * targetMipBias);
    pixelDerivativesScale = sqrtf(targetResolution / min(effectiveTargetResolution, maxEffectiveTargetResolution));
  }
  ShaderGlobal::set_float4(var::world_dd_scale, Color4(ddw, ddh, pixelDerivativesScale, pixelDerivativesScale));
}

int ClipmapImpl::getZoomLimit(int texMips) const
{
  static constexpr float zoomLimitBias = 1.75f; // log2(1.75) = 0.81 additional zoom levels than the one that includes maxTexelSize.
  return eastl::clamp<int>(get_bigger_pow2(zoomLimitBias * maxTexelSize / ((1 << (texMips - 1)) * pixelRatio)), 1, TEX_MAX_ZOOM);
}

void ClipmapImpl::prepareFeedback(const Point3 &center, const TMatrix4 &globtm, const TMatrix4 *mirror_globtm_opt,
  const ZoomFeeedbackParams &zoom_params, const SoftwareFeedbackParams &software_fb, bool force_update,
  UpdateFeedbackThreadPolicy thread_policy)
{
  if (use_ri_vtex)
    prepareFeedback<true>(center, globtm, mirror_globtm_opt, zoom_params, software_fb, force_update, thread_policy);
  else
    prepareFeedback<false>(center, globtm, mirror_globtm_opt, zoom_params, software_fb, force_update, thread_policy);
}

template <bool ri_vtex>
void ClipmapImpl::prepareFeedback(const Point3 &center, const TMatrix4 &globtm, const TMatrix4 *mirror_globtm_opt,
  const ZoomFeeedbackParams &zoom_params, const SoftwareFeedbackParams &software_fb, bool force_update,
  UpdateFeedbackThreadPolicy thread_policy)
{
  using PackedTile = PackedTile<ri_vtex>;
  TIME_PROFILE(prepareFeedback);

#if DAGOR_DBGLEVEL > 0
  InvalidatePointFn invalidatePointFn = [this](const Point2 &world_xz) { this->invalidatePoint(world_xz); };
  ::clipmap_debug_before_prepare_feedback(invalidatePointFn);
#endif

  G_ASSERT(!feedbackPendingChanges);
  G_ASSERT(currentContext);
  viewerPosition = center;

  // Clamping wk
  float effectiveWidth = (float)targetSize.x / pixelDerivativesScale;

  int centerOffsetRoundingError = (1 << max(ALIGNED_MIPS - 2, 0));
  int minTilesDistFromMip0IndirectionEdge = HALF_TILE_WIDTH - centerOffsetRoundingError;

  // When perspective wk is increased (when zooming for example), further regions will start using tiles of lower mips.
  // However at some point indirection texture won't be large enough to support that (thus log2(tcSize) + 2 ... trick of shaders)
  // We can estimate wk limit such that at the edges of mip0 indirection tex at least mip1 is used by solving equation:
  // mip1_pixelSize >= 2*distance / (effectiveWidth * wk)
  // Where distance = minTilesDistFromMip0IndirectionEdge*texTileInnerSize*pixelRatio and mip1_pixelSize = 2*pixelRatio
  // Notice that the resulting inequation is not influenced by pixelRatio.
  float maxWk = (float)(minTilesDistFromMip0IndirectionEdge * texTileInnerSize) / effectiveWidth;

  static bool highWkReported = false;
  if ((always_report_high_wk || !highWkReported) && maxWk < zoom_params.wk)
  {
    logwarn("clipmap: Perspective wk is too big. Clipmap quality is probably decreased. (report once)");
    highWkReported = true;
  }
  float wk = min(maxWk, zoom_params.wk);

  // Prepare mipsCutoff parameters
  int cutoffTexMips = maxTexMipCnt;
  int cutoffZoom = getZoomLimit(maxTexMipCnt);
  if (zoom_params.mipsCutoff.isSet())
  {
    // Abjusting mipsCutoff parameters to satisfy getZoomLimit.
    G_ASSERT(is_pow_of2(zoom_params.mipsCutoff.zoom));
    cutoffTexMips = min(zoom_params.mipsCutoff.texMips, maxTexMipCnt);
    int zoomCutoffClamp = max(zoom_params.mipsCutoff.zoom / getZoomLimit(cutoffTexMips), 1);
    cutoffZoom = zoom_params.mipsCutoff.zoom / zoomCutoffClamp;
  }
  G_ASSERT(cutoffZoom <= getZoomLimit(cutoffTexMips));

  // Finding zoom level.
  float groundPixelSize = 2.f * zoom_params.height / (effectiveWidth * wk);
  float zoomf = max(zoom_pixel_size_scale * groundPixelSize / pixelRatio, 1.f);

  int nextZoom = currentContext->LRUPos.getZoom();
  if (zoomf > currentContext->LRUPos.getZoom() * 2.1f)
    nextZoom = currentContext->LRUPos.getZoom() * 2;
  else if (zoomf < currentContext->LRUPos.getZoom())
    nextZoom = currentContext->LRUPos.getZoom() / 2;

  nextZoom = clamp(nextZoom, get_bigger_pow2(zoom_params.minimumZoom), getZoomLimit(cutoffTexMips));

  if (fixed_zoom > 0)
    nextZoom = get_bigger_pow2(fixed_zoom);

  // Adjusting mips count to cutoffZoom, cutoffTexMips
  int newTexMipCnt = min(cutoffTexMips + (int)get_log2i_of_pow2w(max(cutoffZoom / nextZoom, 1)), maxTexMipCnt);

  IPoint2 nextOffset = centerOffset(center, nextZoom);
  if (currentContext->LRUPos.getZoom() != nextZoom || currentContext->LRUPos.offset != nextOffset ||
      currentContext->currentTexMipCnt != newTexMipCnt)
  {
    MipPosition oldMipPosition = currentContext->LRUPos;

    int newZoomShift = get_log2i_of_pow2w(nextZoom);
    currentContext->LRUPos.tileZoomShift = newZoomShift;
    currentContext->LRUPos.offset = nextOffset;
    currentContext->currentTexMipCnt = newTexMipCnt;

    migrateLRU(oldMipPosition, currentContext->LRUPos);
  }

  updateRendinstLandclassParams();

  if (feedbackType == SOFTWARE_FEEDBACK)
  {
    prepareSoftwareFeedback(nextZoom, center, software_fb.viewItm, globtm, software_fb.approxHt, software_fb.maxDist0,
      software_fb.maxDist1, force_update);
    return;
  }

  ////////////////////////
  // prepare feedback here

  int newestCaptureTarget = currentContext->captureTargetCounter - 1;

  if (currentContext->captureTargetCounter >= MAX_FRAMES_AHEAD)
  {
    TIME_PROFILE(retrieveFeedback);

    int captureTargetReadbackIdx = -1;
    int lockidx = newestCaptureTarget;
    for (; lockidx >= currentContext->oldestCaptureTargetCounter; lockidx--)
    {
      TIME_D3D_PROFILE(query_status)
      if (d3d::get_event_query_status(currentContext->captureTexFence[lockidx % MAX_FRAMES_AHEAD].get(), false))
      {
        captureTargetReadbackIdx = lockidx % MAX_FRAMES_AHEAD;
        currentContext->oldestCaptureTargetCounter = lockidx; // in case we skipped a capture target for a newer.
        break;
      }
    }
    if (captureTargetReadbackIdx < 0 && ++failLockCount >= 6)
    {
      logwarn("can not lock target in 5 attempts! lock anyway, can cause stall");
      captureTargetReadbackIdx = currentContext->oldestCaptureTargetCounter % MAX_FRAMES_AHEAD;
    }
    // debug("frames %d captureTargetReadbackIdx = %d(%d)", currentContext->captureTarget, captureTargetReadbackIdx, lockidx);
    if (captureTargetReadbackIdx >= 0 && !freeze_feedback)
    {
      failLockCount = 0;

      // Invalidate capture metadata.
      currentContext->lockedCaptureMetadata = CaptureMetadata();

      CaptureMetadata captureMetadata = currentContext->captureMetadata[captureTargetReadbackIdx];

      {
        bool validLock = false;
        bool validData = captureMetadata.isValid();
        if (!use_uav_feedback)
        {
          TIME_PROFILE_DEV(pixelLock);

          currentContext->lockedPixelsBytesStride = 0;

          int strideInBytes;
          uint8_t *lockedPixelsPtr;
          if (currentContext->captureTexRt[captureTargetReadbackIdx]->lockimg(reinterpret_cast<void **>(&lockedPixelsPtr),
                strideInBytes, 0, TEXLOCK_READ | TEXLOCK_RAWDATA))
          {
            currentContext->lockedPixelsBytesStride = strideInBytes;
            G_ASSERT(currentContext->lockedPixelsBytesStride >= feedbackSize.x * sizeof(TexTileFeedback));
            validLock = true;

            const size_t feedbackSizeBytes = currentContext->lockedPixelsBytesStride * feedbackSize.y;
            if (!currentContext->lockedPixelsMemCopy || feedbackSizeBytes != currentContext->lockedPixelsMemCopySize)
            {
              free(currentContext->lockedPixelsMemCopy);
              currentContext->lockedPixelsMemCopy = (uint8_t *)malloc(feedbackSizeBytes);
              currentContext->lockedPixelsMemCopySize = feedbackSizeBytes;
            }

            if (validData)
            {
              TIME_PROFILE(pixelLock_copyPixels);
              memcpy(currentContext->lockedPixelsMemCopy, lockedPixelsPtr, feedbackSizeBytes);
            }

            currentContext->captureTexRt[captureTargetReadbackIdx]->unlock();
          }
          else if (!d3d::is_in_device_reset_now())
            LOGERR_ONCE("Clipmap: Could not capture feedback texture target.");
        }
        else
        {
          TIME_PROFILE_DEV(tilesLock);

          void *lockedPtr = nullptr;
          if (currentContext->captureTileInfoBuf[captureTargetReadbackIdx]->lock(0, 0, &lockedPtr, VBLOCK_READONLY))
          {
            validLock = true;

            uint32_t lockedTileInfoCnt = *reinterpret_cast<uint32_t *>(lockedPtr);
            PackedTile *lockedTileInfoPtr = reinterpret_cast<PackedTile *>(lockedPtr);
            lockedTileInfoPtr++; // Actual tiles info start on &lockedTileInfoPtr[1];
            if (lockedTileInfoCnt > currentContext->maxTilesFeedback)
            {
              if (!::grs_draw_wire) // Wireframe can "bully" feedback. Too many tiles on feedback.
              {
                logerr("clipmap: lockedTileInfoCnt is out of bounds: %d > %d", lockedTileInfoCnt, currentContext->maxTilesFeedback);
              }
              lockedTileInfoCnt = currentContext->maxTilesFeedback;
            }

            {
              // Some GPU have slow CPU cached access, this is why we memcpy tiles into chunks and then we process them.
              TIME_PROFILE(tilesLock_copyTiles);
              preparedTilesFeedback.clear();

              eastl::array<PackedTile, 1024> tempMemCopy; // that's 8KiB in stack on the worst case (MAX_RI_VTEX_CNT_BITS > 0)
              for (int i = 0; i < div_ceil<uint32_t>(lockedTileInfoCnt, tempMemCopy.size()) && validData; i++)
              {
                int tilesDone = i * tempMemCopy.size();
                int tilesRemaining = lockedTileInfoCnt - tilesDone;
                int tilesCopying = min<int>(tilesRemaining, tempMemCopy.size());
                G_ASSERT_CONTINUE(tilesRemaining > 0);

                // Some GPU have slow CPU cached access, this is why we memcpy tiles into chunks and then we process them.
                memcpy(tempMemCopy.data(), lockedTileInfoPtr + tilesDone, tilesCopying * sizeof(PackedTile));

                for (const PackedTile &packedTile : dag::Span(tempMemCopy.data(), tilesCopying))
                {
                  uint32_t ri_offset;
                  uint4 fb = unpackTileInfo(packedTile, ri_offset);

                  // TODO add validity checks because we want to catch some rare feedback issues.
                  TexTileInfo tile((int)fb.x - HALF_TILE_WIDTH, (int)fb.y - HALF_TILE_WIDTH, fb.z, ri_offset, fb.w);
                  if (DAGOR_UNLIKELY(!tile.isValid() || tile.mip >= captureMetadata.texMipCnt))
                  {
                    // We have some rare cases of invalid GPU histogram. Report and prevent "poisoning" the rest of the implementation.
                    logerr("clipmap: tilesLock_unpackTiles: Invalid tile: x = %i, y = %i, mip = %i, ri = %i, count = %i,\n"
                           "  lockedTileInfoCnt = %i, tile idx = %i,\n"
                           "  lockIdx = %i, captureTargetCounter = %i, oldestCaptureTargetCounter = %i, texMipCnt = %i",
                      tile.x, tile.y, tile.mip, tile.ri_offset, tile.count, lockedTileInfoCnt, preparedTilesFeedback.size(), lockidx,
                      currentContext->captureTargetCounter, currentContext->oldestCaptureTargetCounter, captureMetadata.texMipCnt);

                    validData = false;
                    preparedTilesFeedback.clear();
                    break;
                  }
                  preparedTilesFeedback.emplace_back(tile);
                }
              }
            }

            currentContext->captureTileInfoBuf[captureTargetReadbackIdx]->unlock();
          }
          else if (!d3d::is_in_device_reset_now())
            LOGERR_ONCE("clipmap: Could not capture feedback tile buffer.");
        }

        if (validLock)
          currentContext->oldestCaptureTargetCounter++;

        if (!validLock || !validData)
          captureMetadata.texMipCnt = -1; // invalidating

        currentContext->lockedCaptureMetadata = captureMetadata;
      }
    }
    if (currentContext->lockedCaptureMetadata.isValid())
    {
      feedbackJob.initFromHWFeedback(currentContext->lockedCaptureMetadata, globtm, mirror_globtm_opt, *this, force_update);
      if (thread_policy == UpdateFeedbackThreadPolicy::SAME_THREAD)
        feedbackJob.doJob();
      else
        threadpool::add(&feedbackJob,
          (thread_policy == UpdateFeedbackThreadPolicy::PRIO_LOW) ? threadpool::PRIO_LOW : threadpool::PRIO_NORMAL);
    }
  }

  if (use_uav_feedback)
  {
    // 0 = invalid data, with dummy ri_offset==0 value (it can never occur in real feedback, only from clearing)
    d3d::zero_rwbufi(currentContext->captureStagingBuf.getBuf());
    resetUAVAtomicPrefix();
  }
}

void ClipmapImpl::renderFallbackFeedback(ClipmapRenderer &renderer, const TMatrix4 &globtm)
{
  if (feedbackType != CPU_HW_FEEDBACK || use_uav_feedback ||
      currentContext->captureTargetCounter - currentContext->oldestCaptureTargetCounter >= MAX_FRAMES_AHEAD)
    return;

  int captureTargetIdx = currentContext->captureTargetCounter % MAX_FRAMES_AHEAD;
  // debug("render to %d (%d), oldest = %d(%d)", captureTargetIdx, currentContext->captureTarget,
  //   currentContext->oldestCaptureTargetIdx%MAX_FRAMES_AHEAD, currentContext->oldestCaptureTargetIdx);

  TIME_D3D_PROFILE(renderFallbackFeedback);

  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;

  setDDScale();
  const bool shouldUseFeedbackTexFilter = useFeedbackTexFilter && currentContext->feedbackTexFilterRenderer;
  if (shouldUseFeedbackTexFilter)
    d3d::set_render_target(currentContext->captureTexRtUnfiltered.getTex2D(), 0);
  else
    d3d::set_render_target(currentContext->captureTexRt[captureTargetIdx].getTex2D(), 0);
  d3d::set_depth(feedbackDepthTex.getTex2D(), DepthAccess::RW);
  ShaderGlobal::set_float(var::rendering_feedback_view_scale, feedback_view_scale);
  TMatrix4 scaledGlobTm = globtm * TMatrix4(feedback_view_scale, 0, 0, 0, 0, feedback_view_scale, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
  d3d::settm(TM_PROJ, &scaledGlobTm);
  d3d::settm(TM_VIEW, TMatrix::IDENT);
  if (::grs_draw_wire)
    d3d::setwire(false);
  d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(255, 255, 255, 255), 0.0f, 0);
  renderer.renderFeedback(scaledGlobTm);
  if (shouldUseFeedbackTexFilter)
  {
    d3d::set_render_target(currentContext->captureTexRt[captureTargetIdx].getTex2D(), 0);
    d3d::set_depth(nullptr, DepthAccess::SampledRO);
    ShaderGlobal::set_texture(var::feedback_tex_unfiltered, currentContext->captureTexRtUnfiltered.getTexId());
    ShaderBlockSetter blockFrame(-1, ShaderGlobal::LAYER_FRAME);
    ShaderBlockSetter blockScene(-1, ShaderGlobal::LAYER_SCENE);
    ShaderBlockSetter blockObject(-1, ShaderGlobal::LAYER_OBJECT);
    currentContext->feedbackTexFilterRenderer->render();
  }
  if (::grs_draw_wire)
    d3d::setwire(true);

  // save_rt_image_as_tga(currentContext->captureTex[currentContext->captureTargetIdx], "feedback.tga");

  copyAndAdvanceCaptureTarget(captureTargetIdx);
}

void ClipmapImpl::copyAndAdvanceCaptureTarget(int captureTargetIdx)
{
  {
    TIME_D3D_PROFILE(copyFeedback);

    if (!use_uav_feedback)
    {
      TIME_D3D_PROFILE(copyImg);

      request_readback_nosyslock<TEXLOCK_RAWDATA>(currentContext->captureTexRt[captureTargetIdx].getTex2D(), 0);
    }
    else
    {
      TIME_D3D_PROFILE(copyTiles);

      if (currentContext->captureTileInfoBuf[captureTargetIdx]->lock(0, 0, (void **)nullptr, VBLOCK_READONLY))
        currentContext->captureTileInfoBuf[captureTargetIdx]->unlock();
    }
  }
  d3d::issue_event_query(currentContext->captureTexFence[captureTargetIdx].get());
  currentContext->captureTargetCounter++;
  currentContext->captureMetadata[captureTargetIdx].position = currentContext->bindedPos;
  currentContext->captureMetadata[captureTargetIdx].riIndices = currentContext->bindedRiIndices;
  currentContext->captureMetadata[captureTargetIdx].texMipCnt = currentContext->bindedTexMipCnt;
}

void ClipmapImpl::startUAVFeedback()
{
  if (feedbackType != CPU_HW_FEEDBACK || !use_uav_feedback)
    return;

  d3d::set_rwbuffer(STAGE_PS, uav_feedback_const_no, currentContext->captureStagingBuf.getBuf());
  var::uav_feedback_exists.set_int(1);
}

void ClipmapImpl::processUAVFeedback(int captureTargetIdx)
{
  G_ASSERT(use_uav_feedback && clearHistogramCs && processFeedbackCs && buildTileInfoCs);
  G_ASSERT(feedbackOversampling >= 1 && is_pow_of2(feedbackOversampling));
  TIME_D3D_PROFILE(clipmap_GPU_process_feedback);

  {
    TIME_D3D_PROFILE(clear_histogram);
    STATE_GUARD(ShaderGlobal::set_buffer(var::clipmap_tile_info_buf_rw, VALUE),
      currentContext->captureTileInfoBuf[captureTargetIdx].getBufId(), BAD_TEXTUREID);
    STATE_GUARD(ShaderGlobal::set_buffer(var::clipmap_histogram_buf_rw, VALUE), currentContext->intermediateHistBuf.getBufId(),
      BAD_TEXTUREID);
    clearHistogramCs->dispatchThreads(clipmap_hist_total_elements, 1, 1);
  }

  {
    TIME_D3D_PROFILE(process_feedback);

    d3d::resource_barrier({currentContext->captureStagingBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});

    d3d::resource_barrier({currentContext->intermediateHistBuf.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::resource_barrier(
      {currentContext->captureTileInfoBuf[captureTargetIdx].getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

    STATE_GUARD(ShaderGlobal::set_buffer(var::clipmap_feedback_buf, VALUE), currentContext->captureStagingBuf.getBufId(),
      BAD_TEXTUREID);
    STATE_GUARD(ShaderGlobal::set_buffer(var::clipmap_histogram_buf_rw, VALUE), currentContext->intermediateHistBuf.getBufId(),
      BAD_TEXTUREID);

    processFeedbackCs->dispatchThreads(feedbackSize.x * feedbackSize.y, 1, 1);
  }

  {
    TIME_D3D_PROFILE(build_tile_info);

    d3d::resource_barrier({currentContext->intermediateHistBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});

    ShaderGlobal::set_buffer(var::clipmap_histogram_buf, currentContext->intermediateHistBuf);
    STATE_GUARD(ShaderGlobal::set_buffer(var::clipmap_tile_info_buf_rw, VALUE),
      currentContext->captureTileInfoBuf[captureTargetIdx].getBufId(), BAD_TEXTUREID);
    buildTileInfoCs->dispatchThreads(TILE_WIDTH, TILE_WIDTH, 1);
  }

  // transfer texture to UAV state, to avoid slow autogenerated barrier inside driver
  // when binding this texture as RW_UAV later on in startUAVFeedback
  d3d::resource_barrier({currentContext->captureStagingBuf.getBuf(), RB_RW_UAV | RB_STAGE_PIXEL});
}

void ClipmapImpl::endUAVFeedback()
{
  if (feedbackType != CPU_HW_FEEDBACK || !use_uav_feedback)
    return;

  d3d::set_rwbuffer(STAGE_PS, uav_feedback_const_no, nullptr);
  var::uav_feedback_exists.set_int(0);
}

void ClipmapImpl::increaseUAVAtomicPrefix()
{
  if (feedbackType != CPU_HW_FEEDBACK || !use_uav_feedback)
    return;

  currentContext->feedbackUAVAtomicPrefix++;

  G_ASSERTF_RETURN(currentContext->feedbackUAVAtomicPrefix < 16, , "Clipmap, increaseUAVAtomicPrefix: atomic prefix exceeds limit.");
  var::clipmap_feedback_order_prefix.set_int(currentContext->feedbackUAVAtomicPrefix);
}

void ClipmapImpl::resetUAVAtomicPrefix()
{
  if (feedbackType != CPU_HW_FEEDBACK || !use_uav_feedback)
    return;

  currentContext->feedbackUAVAtomicPrefix = 0;

  var::clipmap_feedback_order_prefix.set_int(currentContext->feedbackUAVAtomicPrefix);
}

void ClipmapImpl::copyUAVFeedback()
{
  if (feedbackType != CPU_HW_FEEDBACK || !use_uav_feedback ||
      currentContext->captureTargetCounter - currentContext->oldestCaptureTargetCounter >= MAX_FRAMES_AHEAD)
    return;

  int captureTargetIdx = currentContext->captureTargetCounter % MAX_FRAMES_AHEAD;

  processUAVFeedback(captureTargetIdx);

  copyAndAdvanceCaptureTarget(captureTargetIdx);
}

void ClipmapImpl::finalizeFeedback()
{
  TIME_PROFILE(waitForFeedbackJob);
  threadpool::wait(&feedbackJob);
}

void ClipmapImpl::prepareRender(ClipmapRenderer &renderer, dag::Span<Texture *> buffer_tex, dag::Span<Texture *> compressor_tex,
  bool turn_off_decals_on_fallback)
{
  TIME_D3D_PROFILE(prepareTiles);

  G_ASSERT(bindedBufferTex.empty());
  G_ASSERT(bindedCompressorTex.empty());
  if (useOwnBuffers)
  {
    for (int ci = 0; ci < bufferCnt; ++ci)
      bindedBufferTex.emplace_back(internalBufferTex[ci].getTex2D());

    // Buffer is internally managed by BcCompressor class. bindedCompressorTex is set to nullptr.
    for (int ci = 0; ci < maxCacheCnt; ++ci)
      bindedCompressorTex.emplace_back(nullptr);
  }
  else
  {
    G_ASSERT(buffer_tex.size());
    eastl::copy(buffer_tex.begin(), buffer_tex.end(), eastl::back_inserter(bindedBufferTex));
    G_ASSERT(bindedBufferTex.size() == bufferCnt);

    G_ASSERT(compressor_tex.size());
    eastl::copy(compressor_tex.begin(), compressor_tex.end(), eastl::back_inserter(bindedCompressorTex));
    G_ASSERT(bindedCompressorTex.size() == cacheCnt);
  }

  update(renderer, turn_off_decals_on_fallback);
  updateRenderStates();

  bindedBufferTex.clear();
  bindedCompressorTex.clear();
}


struct SortRendinstBySortOrder
{
  bool operator()(const RendinstLandclassData &a, const RendinstLandclassData &b) const { return a.sortOrder < b.sortOrder; }
};

static void sort_rendinst_info_list(TempRiLandclassVec &ri_landclasses, const Point3 &camera_pos)
{
  for (int i = 0; i < ri_landclasses.size(); i++)
    ri_landclasses[i].sortOrder = calc_landclass_sort_order(ri_landclasses[i].matrixInv, ri_landclasses[i].radius, camera_pos);
  stlsort::sort(ri_landclasses.data(), ri_landclasses.data() + ri_landclasses.size(), SortRendinstBySortOrder());
}


static Point4 transform_mapping(Point4 mapping)
{
  // TODO: fix this lunacy
  // first apply a magic transform
  mapping.w = -mapping.w;
  // then remove last_clip_tex mirroring from shader code
  mapping.w = 1.0f - mapping.w;
  mapping.y = -mapping.y;
  return mapping;
}

void ClipmapImpl::updateRendinstLandclassRenderState()
{
  currentContext->bindedRiIndices = riIndices;

  if (!use_ri_vtex)
    return;

  int changedRiIndexBits = 0;
  for (int i = 0; i < changedRiIndices.size(); ++i)
    if (changedRiIndices[i])
      changedRiIndexBits |= 1 << i;
  ShaderGlobal::set_int(var::ri_landclass_changed_indicex_bits, changedRiIndexBits);

  if (auto dataManager = getRiLandclassDataManager())
  {
    dataManager->updateLandclass2uv(
      riIndices, [this](int ri_offset) { return this->getLandscape2uv(ri_offset); },
      [this](int ri_offset) { return this->getMipPosition(ri_offset); });
    dataManager->updateClosestIndices(riIndices);
  }
}

static void update_rendinst_landclass_subset(dag::Span<RendinstLandclassData> rendinst_landclass_data,
  dag::ConstSpan<RendinstLandclassData> prev_rendinst_landclass_data, const Point3 &camera_pos)
{
  TempRiLandclassVec rendinstLandclassesCandidate;
  if (get_rendinst_land_class_data)
    get_rendinst_land_class_data(rendinstLandclassesCandidate);

  sort_rendinst_info_list(rendinstLandclassesCandidate, camera_pos);

  int riLandclassCount = min<int>(rendinstLandclassesCandidate.size(), rendinst_landclass_data.size());

  // stable sorting, so feedback result don't need special invalidation:
  rendinstLandclassesCandidate.resize(riLandclassCount);
  TempRiLandclassVec rendinstLandclassesNewElements;
  eastl::bitset<MAX_RI_VTEX_CNT> prevMatchingIndices;
  for (const auto &candidate : rendinstLandclassesCandidate)
  {
    auto it = eastl::find_if(prev_rendinst_landclass_data.begin(), prev_rendinst_landclass_data.end(),
      [&candidate](const auto &prevData) { return prevData.index >= 0 && candidate.index == prevData.index; });
    int offset;
    if (it != prev_rendinst_landclass_data.end() &&
        (offset = eastl::distance(prev_rendinst_landclass_data.begin(), it)) < riLandclassCount)
      prevMatchingIndices.set(offset);
    else
      rendinstLandclassesNewElements.push_back(candidate);
  }

  for (int i = 0, insertedId = 0; i < riLandclassCount; ++i)
    if (prevMatchingIndices[i])
      rendinst_landclass_data[i] = prev_rendinst_landclass_data[i];
    else
    {
      rendinst_landclass_data[i] = rendinstLandclassesNewElements[insertedId++];
      rendinst_landclass_data[i].mapping = transform_mapping(rendinst_landclass_data[i].mapping);
    }

  for (int i = riLandclassCount; i < rendinst_landclass_data.size(); ++i)
    rendinst_landclass_data[i].index = RI_INVALID_ID;
}

static bool is_approx_equal(const Point3 &a, const Point3 &b, float epsilon = 0.0001f)
{
  return (a - b).lengthSq() < epsilon * epsilon;
}

static RendinstLandclassData make_secondary_terrain_vtex_landclass_data()
{
  RendinstLandclassData secondaryTerrainVtex;
  secondaryTerrainVtex.index = ClipmapImpl::UNIQUE_SECONDARY_TERRAIN_VTEX_INDEX;
  secondaryTerrainVtex.matrixInv = TMatrix::IDENT; // unused
  secondaryTerrainVtex.mapping = Point4(0, 0, 1, 1);
  secondaryTerrainVtex.bbox = bbox3f(); // unused
  secondaryTerrainVtex.radius = 0;      // unused
  return secondaryTerrainVtex;
}

void ClipmapImpl::updateRendinstLandclassParams()
{
  if (!use_ri_vtex)
    return;

  RiLandclasses prevRendinstLandclasses = rendinstLandclasses;
  RiIndices prevRiIndices = riIndices;

  const bool secondaryFeedbackCenterChanged =
    !!secondaryFeedbackCenter != !!nextSecondaryFeedbackCenter ||
    (secondaryFeedbackCenter && !is_approx_equal(*secondaryFeedbackCenter, *nextSecondaryFeedbackCenter));
  secondaryFeedbackCenter = nextSecondaryFeedbackCenter;

  auto updateRendinstLandclassSubset = [&](const Point3 &camera_pos, int start, int end) -> void {
    G_ASSERT(start < end && start >= 0 && end <= MAX_RI_VTEX_CNT);
    update_rendinst_landclass_subset(make_span(rendinstLandclasses.begin() + start, end - start),
      make_span_const(prevRendinstLandclasses.begin() + start, end - start), camera_pos);
  };

  updateRendinstLandclassSubset(viewerPosition, 0, SECONDARY_OFFSET_START_NO_TERRAIN);
  if (secondaryFeedbackCenter)
  {
    rendinstLandclasses[SECONDARY_OFFSET_START_NO_TERRAIN] = make_secondary_terrain_vtex_landclass_data();
    updateRendinstLandclassSubset(*secondaryFeedbackCenter, SECONDARY_OFFSET_START_NO_TERRAIN + 1, MAX_RI_VTEX_CNT);
  }
  else
  {
    for (int i = SECONDARY_OFFSET_START_NO_TERRAIN; i < MAX_RI_VTEX_CNT; ++i)
      rendinstLandclasses[i].index = RI_INVALID_ID;
  }

  for (int i = 0; i < MAX_RI_VTEX_CNT; ++i)
  {
    riIndices[i] = rendinstLandclasses[i].index;
    if (rendinstLandclasses[i].index == RI_INVALID_ID || rendinstLandclasses[i].index != prevRendinstLandclasses[i].index)
      changedRiIndices[i] = ri_invalid_frame_cnt;
  }

  eastl::fixed_vector<CacheCoords, MAX_CACHE_TILES, false> removedLRUs;
  const RiIndices::CompareBitset equalRiIndices = riIndices.compareEqual(prevRiIndices);
  auto validLRUsEndIt = eastl::remove_if(currentContext->LRU.begin(), currentContext->LRU.end(),
    [this, &equalRiIndices, &removedLRUs, secondaryFeedbackCenterChanged](const TexLRU &ti) {
      if (ti.tilePos.ri_offset != TERRAIN_RI_OFFSET && !equalRiIndices[ti.tilePos.ri_offset - 1] ||
          (ti.tilePos.ri_offset >= RI_VTEX_SECONDARY_OFFSET_START && secondaryFeedbackCenterChanged))
      {
        uint32_t addr = ti.tilePos.getTileBitIndex();
        G_ASSERT(currentContext->inCacheBitmap[addr]);
        currentContext->inCacheBitmap[addr] = false;
        removedLRUs.emplace_back(ti.cacheCoords);
        return true;
      }
      else
      {
        return false;
      }
    });

  if (removedLRUs.size())
  {
    // Make stealSuperfluousLRU pick them first. We use removedLRUs + remove_if to keep FIFO-like order of existing superfluous LRUs.
    G_ASSERT(validLRUsEndIt + removedLRUs.size() == currentContext->LRU.end());
    eastl::copy(removedLRUs.begin(), removedLRUs.begin() + removedLRUs.size(), validLRUsEndIt);
    currentContext->invalidateIndirection = true;
  }

  for (int i = 0; i < MAX_RI_VTEX_CNT; ++i)
  {
    if (riIndices[i] != RI_INVALID_ID)
    {
      IPoint2 offset = centerOffset(getMappedRiPos(i + 1), RI_LANDCLASS_FIXED_ZOOM);
      riPos[i] = {offset, RI_LANDCLASS_FIXED_ZOOM_SHIFT};
    }
    else
    {
      riPos[i] = MipPosition{};
    }
  }

  if (log_ri_indirection_change)
  {
    eastl::string txt;
    txt += "{";
    bool hasIt = false;
    for (int j = 0; j < rendinstLandclasses.size(); ++j)
    {
      if (rendinstLandclasses[j].index != prevRendinstLandclasses[j].index)
      {
        txt += eastl::to_string(j);
        txt += " ";
        hasIt = true;
      }
    }
    txt += "}";
    if (hasIt)
    {
      txt += " {";
      for (int j = 0; j < prevRendinstLandclasses.size(); ++j)
        txt += eastl::to_string(prevRendinstLandclasses[j].index) + " ";
      txt += "} {";
      for (int j = 0; j < rendinstLandclasses.size(); ++j)
        txt += eastl::to_string(rendinstLandclasses[j].index) + " ";
      txt += "}";
      logerr("RI indirection changed: %s", txt);
    }
  }
}

void ClipmapImpl::updateRenderStates()
{
  G_ASSERT(currentContext);

  for (int ci = 0; ci < cacheCnt; ++ci)
    cache[ci].setVar(); // new way

  ShaderGlobal::set_texture(var::indirection_tex, currentContext->indirection.getTexId());
  setDDScale();

  ShaderGlobal::set_int(var::clipmap_tex_mips, currentContext->currentTexMipCnt);

  Point4 landscape2uv = getLandscape2uv(TERRAIN_RI_OFFSET);
  ShaderGlobal::set_float4(var::landscape2uv, landscape2uv);
  IPoint2 offset = getMipPosition(TERRAIN_RI_OFFSET).offset;
  ShaderGlobal::set_int4(var::mippos_offset, IPoint4(offset.x, offset.y, 0, 0));

  currentContext->bindedPos = currentContext->LRUPos;
  currentContext->bindedTexMipCnt = currentContext->currentTexMipCnt;

  if (VariableMap::isGlobVariablePresent(var::fallback_info0))
  {
    Point3 fallback0(0, 2, 2);
    Point2 fallback1(0, 0);
    Point2 gradScale(1, 1);
    if (currentContext->fallback.isValid())
    {
      const float scale = 1.f / currentContext->fallback.getSize();
      const Point2 ofs = -scale * currentContext->fallback.getOrigin() + Point2(0.5, 0.5);
      fallback0 = Point3(scale, ofs.x, ofs.y);
      const float texels = currentContext->fallback.getTexelsSize();
      fallback1 = Point2(texels / float(cacheDimX), texels / float(cacheDimY));
      gradScale = fallback1 * scale;
    }
    ShaderGlobal::set_float4(var::fallback_info0, fallback0.x, fallback0.y, fallback0.z, 0);
    ShaderGlobal::set_float4(var::fallback_info1, fallback1.x, fallback1.y, gradScale.x, gradScale.y);
  }

  updateRendinstLandclassRenderState();
}


bool ClipmapImpl::getBBox(BBox2 &ret) const
{
  if (currentLRUBox.isempty())
    return false;
  ret = currentLRUBox;
  return true;
}

IBBox2 ClipmapImpl::getMaximumIBBox(int ri_offset) const
{
  const int texMips = getCurrentTexMips(ri_offset);
  return {
    TexTilePos{-HALF_TILE_WIDTH, -HALF_TILE_WIDTH, texMips - 1, ri_offset}.projectOnLowerMip(getMipPosition(ri_offset), 0).lim[0],
    TexTilePos{HALF_TILE_WIDTH - 1, HALF_TILE_WIDTH - 1, texMips - 1, ri_offset}
      .projectOnLowerMip(getMipPosition(ri_offset), 0)
      .lim[1]};
}

bool ClipmapImpl::getMaximumBBox(BBox2 &ret) const
{
  constexpr int riOffset = TERRAIN_RI_OFFSET;

  IBBox2 tilesRegion = getMaximumIBBox(riOffset);
  Point4 uv2l = getUV2landacape(riOffset);
  Point2 tilesDimensions = {uv2l.x / TILE_WIDTH, uv2l.y / TILE_WIDTH};
  Point2 originOffset = {uv2l.z, uv2l.w};

  ret = {
    mul((Point2)tilesRegion[0], tilesDimensions) + originOffset,
    mul((Point2)tilesRegion[1] + IPoint2::ONE, tilesDimensions) + originOffset, // + IPoint2::ONE to make box inclusive.
  };

  return true;
}

BcCompressor *ClipmapImpl::createCacheCompressor(int cache_id, uint32_t format, int compress_buf_width, int compress_buf_height)
{
  auto compressionType = get_texture_compression_type(format);
  if (compressionType == BcCompressor::COMPRESSION_ERR)
    return nullptr;

  const String compressionShader = [compressionType, cache_id]() {
    switch (compressionType)
    {
      case BcCompressor::COMPRESSION_ETC2_RGBA:
      case BcCompressor::COMPRESSION_ETC2_RG: return String(64, "cache%d_etc2_compressor", cache_id);
      case BcCompressor::COMPRESSION_ASTC4: return String(64, "cache%d_astc4_compressor", cache_id);
      default: return String(64, "cache%d_compressor", cache_id);
    }
  }();

  return new BcCompressor(compressionType, mipCnt, compress_buf_width, compress_buf_height, COMPRESS_QUEUE_SIZE, compressionShader,
    useOwnBuffers);
}

String ClipmapImpl::getCacheBufferName(int idx) { return String(30, "cache_buffer_tex_%d", idx); }

IPoint2 ClipmapImpl::getCacheBufferDim() { return bufferTexDim; }

dag::ConstSpan<uint32_t> ClipmapImpl::getCacheBufferFlags() { return {bufferTexFlags.data(), bufferCnt}; }

dag::ConstSpan<uint32_t> ClipmapImpl::getCacheCompressorFlags() { return {compressorTexFlags.data(), cacheCnt}; }

int ClipmapImpl::getCacheBufferMips() { return mipCnt; }

void ClipmapImpl::createCaches(const uint32_t *formats, const uint32_t *uncompressed_formats, uint32_t cnt,
  const uint32_t *buffer_formats, uint32_t buffer_cnt)
{
  cacheCnt = min(cnt, (uint32_t)cache.size());
  bufferCnt = min(buffer_cnt, (uint32_t)Clipmap::MAX_TEXTURES);

  IPoint2 newBufferTexDim = {COMPRESS_QUEUE_SIZE * texTileSize, texTileSize};
  bool bufDimensionsChanged = newBufferTexDim != bufferTexDim;
  bufferTexDim = newBufferTexDim;

  const uint32_t mipGenFlag = mipCnt > 1 ? TEXCF_GENERATEMIPS : 0;
  carray<bool, Clipmap::MAX_TEXTURES> bufferFlagsChanged;
  for (int ci = 0; ci < bufferCnt; ++ci)
  {
    uint32_t newBufferTexFlags = buffer_formats[ci] | TEXCF_RTARGET | mipGenFlag;
    bufferFlagsChanged[ci] = newBufferTexFlags != bufferTexFlags[ci];
    bufferTexFlags[ci] = newBufferTexFlags;
  }
  for (int ci = bufferCnt; ci < bufferTexFlags.size(); ++ci)
  {
    bufferFlagsChanged[ci] = true;
    bufferTexFlags[ci] = Clipmap::BAD_TEX_FLAGS;
  }

  int ownBufferCnt = useOwnBuffers ? bufferCnt : 0;

  // In case that cacheCnt or bufferCnt was decreased, close unneeded textures.
  for (int ci = ownBufferCnt; ci < internalBufferTex.size(); ++ci)
  {
    internalBufferTex[ci].close();
  }
  for (int ci = cacheCnt; ci < cache.size(); ++ci)
  {
    del_it(compressor[ci]);
    cache[ci].close();
  }

  // Create internal preperation buffers.
  for (int ci = 0; ci < ownBufferCnt; ++ci)
  {
    if (internalBufferTex[ci].getTex2D() && !bufDimensionsChanged && !bufferFlagsChanged[ci] &&
        internalBufferTex[ci].getTex2D()->level_count() == mipCnt)
      continue;

    internalBufferTex[ci].close();
    internalBufferTex[ci] =
      dag::create_tex(nullptr, bufferTexDim.x, bufferTexDim.y, bufferTexFlags[ci], mipCnt, getCacheBufferName(ci), RESTAG_LAND);
    d3d_err(internalBufferTex[ci].getTex2D());
  }

  // Create caches and compressors.
  for (int ci = 0; ci < cacheCnt; ++ci)
  {
    bool shouldCreateCacheTex = true;
    if (cache[ci].getTex2D())
    {
      TextureInfo tinfo;
      cache[ci].getTex2D()->getinfo(tinfo);
      if ((tinfo.cflg & TEXFMT_MASK) == (formats[ci] & TEXFMT_MASK) &&
          (tinfo.cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == (formats[ci] & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) &&
          (tinfo.mipLevels == mipCnt))
      {
        shouldCreateCacheTex = false;
      }
    }

    if (bufDimensionsChanged || shouldCreateCacheTex)
    {
      del_it(compressor[ci]);
      compressor[ci] = createCacheCompressor(ci, formats[ci], bufferTexDim.x, bufferTexDim.y);
    }

    if (!shouldCreateCacheTex)
    {
      continue;
    }

    String cacheTexName(30, "cache_tex%d", ci);
    cache[ci].close();
    bool hasCompressor = compressor[ci] && compressor[ci]->isValid();
    uint32_t texFormat =
      (hasCompressor ? (formats[ci] | TEXCF_UPDATE_DESTINATION) : (uncompressed_formats[ci] | TEXCF_RTARGET)) | TEXCF_CLEAR_ON_CREATE;
    cache[ci] = dag::create_tex(nullptr, cacheDimX, cacheDimY, texFormat, max(mipCnt, 1 + PS4_ANISO_WA), cacheTexName, RESTAG_LAND);
    d3d_err(cache[ci].getTex2D());
#if PS4_ANISO_WA
    cache[ci].getTex2D()->texmiplevel(0, 0);
#endif
  }

  // Collect compressors target flags.
  eastl::fill(compressorTexFlags.begin(), compressorTexFlags.end(), Clipmap::BAD_TEX_FLAGS);
  for (int ci = 0; ci < cacheCnt; ++ci)
  {
    if (compressor[ci] && compressor[ci]->isValid())
      compressorTexFlags[ci] = compressor[ci]->getBufferFlags();
  }

  // we have to set here, since it is not guaranteed that it was re-created
  setTileSizeVars();
  ShaderGlobal::set_int(::get_shader_variable_id("cache_anisotropy"), cacheAnisotropy);
  ShaderGlobal::set_int(var::clipmap_cache_output_count, cacheCnt);
  maxCacheCnt = cacheCnt;
  maxBufferCnt = bufferCnt;

  invalidate(true);
}

void ClipmapImpl::setCacheBufferCount(int cache_count, int buffer_count)
{
  G_ASSERT(cache_count <= maxCacheCnt);
  G_ASSERT(buffer_count <= maxBufferCnt);
  cache_count = cache_count < 0 ? maxCacheCnt : clamp(cache_count, 1, maxCacheCnt);
  buffer_count = buffer_count < 0 ? maxBufferCnt : clamp(buffer_count, 1, maxBufferCnt);
  const bool needInvalidate = cacheCnt < cache_count || bufferCnt < buffer_count;
  cacheCnt = cache_count;
  bufferCnt = buffer_count;
  ShaderGlobal::set_int(var::clipmap_cache_output_count, cacheCnt);
  debug("clipmap: setCacheBufferCount: cache: %d, buffer: %d, invalidate: %d", cacheCnt, bufferCnt, needInvalidate);
  if (needInvalidate)
    invalidate(true);
}

void ClipmapImpl::setDynamicDDScale(const DynamicDDScaleParams &params) { dynamicDDScaleParams = params; }

void ClipmapImpl::setAnisotropyImpl(int anisotropy)
{
  cacheAnisotropy = clamp(anisotropy, 1, (int)MAX_TEX_TILE_BORDER);
  texTileBorder = max((int)MIN_TEX_TILE_BORDER, cacheAnisotropy);
  texTileInnerSize = texTileSize - texTileBorder * 2;
  debug("clipmap: setAnisotropyImpl texTileBorder = %d, cacheAnisotropy = %d", texTileBorder, cacheAnisotropy);
  ShaderGlobal::set_int(::get_shader_variable_id("cache_anisotropy"), cacheAnisotropy);
}

bool ClipmapImpl::setAnisotropy(int anisotropy)
{
  if (anisotropy == cacheAnisotropy)
    return true;

  // Change in anisotropy may lead to the change in required mipCnt
  // In such case it is better to do a full clipmap reinit
  if (mipCnt != get_platform_adjusted_mip_cnt(mipCnt, anisotropy))
    return false;

  finalizeFeedback();
  setAnisotropyImpl(anisotropy);
  setTileSizeVars();
  invalidate(true);
  return true;
}

void ClipmapImpl::beforeReset()
{
  if (riLandclassDataManager)
    riLandclassDataManager->clear();
  finalizeFeedback();
  closeHWFeedback();
}

void ClipmapImpl::afterReset()
{
  invalidate(true);

  if (feedbackType == CPU_HW_FEEDBACK)
    initHWFeedback();

  for (int ci = 0; ci < cache.size(); ++ci)
  {
    if (!cache[ci].getTex2D())
      continue;
    TextureInfo texInfo;
    cache[ci].getTex2D()->getinfo(texInfo);
    if (get_texture_compression_type(texInfo.cflg) == BcCompressor::COMPRESSION_ERR)
      continue;
    del_it(compressor[ci]);
    const int compressBufW = COMPRESS_QUEUE_SIZE * texTileSize;
    const int compressBufH = texTileSize;
    compressor[ci] = createCacheCompressor(ci, texInfo.cflg, compressBufW, compressBufH);
  }

  if (riLandclassDataManager)
    riLandclassDataManager->init();
}

bool ClipmapImpl::is_uav_supported()
{
  auto &drvDesc = d3d::get_driver_desc();
  if (drvDesc.shaderModel < 5.0_sm)
    return false; // Pre-5.0 Shader Model binding UAV on Pixel Shader was not a thing.
  if (!drvDesc.caps.hasProperUAVSupport && (var::requires_uav < 0 || ShaderGlobal::get_int(var::requires_uav) == 0))
    return false; // Some GPU drivers have broken UAV support, BUT on some games we "requires_uav" and do a best-effort eitherway.
  if (ShaderGlobal::has_associated_interval(var::supports_uav) && ShaderGlobal::is_var_assumed(var::supports_uav) &&
      ShaderGlobal::get_interval_assumed_value(var::supports_uav) == 0)
    return false; // If shader has been compiled with "supports_uav = 0" assumption we cannot bind UAV on PS.
  return true;
}

void ClipmapImpl::updateLandclassData()
{
  if (!riLandclassDataManager)
    return;

  dag::Vector<int> localRiIndices;
  getRiLandclassIndices(localRiIndices);

  LandclassQueryData queryData;
  queryData.indices = localRiIndices;
  for (int ri_index = 0; ri_index < localRiIndices.size(); ++ri_index)
  {
    float distToGround;
    int riOffset = ri_index + 1;
    riLandclassDataManager->updateLandclassData(ri_index, rendinstLandclasses[ri_index]);

    Point3 landclassY = rendinstLandclasses[ri_index].matrix.getcol(1);
    Point3 landclassPos = rendinstLandclasses[ri_index].matrix.getcol(3);
    float radius = rendinstLandclasses[ri_index].radius;
    queryData.landclass_gravs.emplace_back(-landclassY);
    queryData.pos_radiusSqr.emplace_back(landclassPos.x, landclassPos.y, landclassPos.z, radius * radius);
  }

  riLandclassDataManager->updateGpuData();
  heightmap_query::update_landclass_data(queryData);
}
