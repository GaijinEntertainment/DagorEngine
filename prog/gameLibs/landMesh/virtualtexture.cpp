#include <stdio.h>
#include <landMesh/clipMap.h>
#include <util/dag_globDef.h>
#include <3d/dag_drv3d.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_dynShaderBuf.h>
#include <3d/dag_ringDynBuf.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_Point4.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/dag_drv3dCmd.h>
#include <render/dxtcompress.h>
#include <math/dag_TMatrix4.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_adjpow2.h>
#include <math/integer/dag_IBBox2.h>
#include <3d/dag_render.h>
#include <debug/dag_debug3d.h>
#include <util/dag_stlqsort.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_console.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <render/bcCompressor.h>
#include <memory/dag_framemem.h>
#include <EASTL/vector_set.h>
#include <EASTL/vector_map.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include <workCycle/dag_workCycle.h>
#include <landMesh/lmeshRenderer.h>
#include <3d/dag_gpuConfig.h>
#include <math/dag_hlsl_floatx.h>
#include <util/dag_convar.h>

#include <3d/dag_lockTexture.h>
#include <3d/dag_lockSbuffer.h>

#include <math/dag_hlsl_floatx.h>
#include <rendInst/rendInstGen.h>


class BcCompressor;
#define USE_VIRTUAL_TEXTURE 1

#include "landMesh/vtex.hlsli"
static constexpr float TILE_WIDTH_F = TILE_WIDTH; // float version to avoid static analyzer warnings

static constexpr int TERRAIN_RI_INDEX = 0;

enum
{
  CLIPMAP_RENDER_STAGE_0 = 0x00000001,
  CLIPMAP_RENDER_STAGE_1 = 0x00000002,
  CLIPMAP_RENDER_STAGE_2 = 0x00000004,
  CLIPMAP_RENDER_ALL_STAGES = 0x00000007
};


class DxtCompressJob;

class ShaderMaterial;
class ShaderElement;
class DynShaderQuadBuf;
class RingDynamicVB;


struct TexTileFeedback
{
  uint8_t x, y;
  uint8_t mip;
#if MAX_RI_VTEX_CNT_BITS > 0
  uint8_t ri_index;
#else
  uint8_t _unused;
  static constexpr uint8_t ri_index = 0;
#endif
};
typedef Tab<TexTileFeedback> TexTileFeedbackVector;

struct TexTileInfo
{
  uint16_t x, y;
  uint8_t mip;
  uint8_t ri_index;
  uint16_t count;
  uint64_t sortOrder = 0;
  TexTileInfo() : x(0), y(0), mip(0), count(0), ri_index(0) {}
  __forceinline TexTileInfo(int X, int Y, int MIP, int COUNT, int RI_INDEX)
  {
    x = X;
    y = Y;
    mip = MIP;
    ri_index = RI_INDEX;
    count = COUNT;
  }
  __forceinline void set(int X, int Y, int MIP, int COUNT, int RI_INDEX)
  {
    x = X;
    y = Y;
    mip = MIP;
    ri_index = RI_INDEX;
    count = COUNT;
  }

  // for debugging only
  bool operator==(const TexTileInfo &other) const
  {
    return ri_index == other.ri_index && x == other.x && y == other.y && mip == other.mip && count == other.count &&
           sortOrder == other.sortOrder;
  }
  bool operator!=(const TexTileInfo &other) const { return !(*this == other); }
};
typedef Tab<TexTileInfo> TexTileInfoVector;


struct TexLRU
{
  enum
  {
    NEED_UPDATE = (1 << 0),
    IS_USED = (1 << 1), // is visible now
    // has different (previous) tile in atlas. This atals cache can not be used, since it has old data in atlas but refers to new
    // position already. alternatively, can be fixed by having two sets of (x,y,mip) - current, and future. Current should be replaced
    // with future after updating tile
    IS_INVALID = (1 << 2),
  };

  __forceinline TexLRU() : x(0), y(0), tx(0), ty(0), mip(0xff), ri_index(0), flags(0) {}
  __forceinline TexLRU(int X, int Y) : x(X), y(Y), tx(0xff), ty(0xff), mip(0xff), ri_index(0), flags(0) {}
  __forceinline TexLRU(int X, int Y, int TX, int TY, int MIP, int RI_INDEX) :
    x(X), y(Y), tx(TX), ty(TY), mip(MIP), ri_index(RI_INDEX), flags(0)
  {}
  __forceinline TexLRU &Update()
  {
    flags |= NEED_UPDATE;
    return *this;
  }
  __forceinline TexLRU &setFlags(uint8_t flag)
  {
    flags = flag;
    return *this;
  }

  uint8_t tx, ty;
  uint8_t ri_index;
  uint8_t x, y, mip, flags; // cache coordinates
};

typedef Tab<TexLRU> TexLRUList;

#if _TARGET_C1 | _TARGET_C2

#else
static constexpr int MAX_FRAMES_AHEAD = 3;
#endif

struct MipContext
{
  TexLRUList LRU;
  int xOffset, yOffset, tileZoom;
  static constexpr int MAX_TEXTURES = 4;

  UniqueTex indirection;
  uint32_t captureTarget;
  int oldestCaptureTargetIdx;
  int texMips;

  bool invalidateIndirection;

  using InCacheBitmap = eastl::bitset<TEX_TOTAL_ELEMENTS>;
  InCacheBitmap inCacheBitmap;

  carray<EventQueryHolder, MAX_FRAMES_AHEAD> captureTexFence;
  carray<UniqueTex, MAX_FRAMES_AHEAD> captureTex;
  carray<UniqueBuf, MAX_FRAMES_AHEAD> captureTileInfoBuf;
  UniqueBuf intermediateHistBuf;

  carray<carray<int, 3>, MAX_FRAMES_AHEAD> captureWorldOffset;
  carray<Point4, MAX_FRAMES_AHEAD> captureUV2landscape;
  carray<int, MAX_FRAMES_AHEAD> debugHWFeedbackConvar;

  bool use_uav_feedback;

  void init(bool in_use_uav_feedback);
  void initIndirection(int tex_mips);
  void closeIndirection();
  void closeHWFeedback();
  void initHWFeedback();
  void reset(int cacheDimX, int cacheDimY, bool force_redraw, int tex_tile_size, int feedback_type);
  inline void resetInCacheBitmap();

  struct Fallback
  {
    int pagesDim = 0;
    float texelSize = 2.0f;
    int texTileSize = 0;
    IPoint2 originXZ = {-1000, 10000};
    bool sameSettings(int dim, float texel) const { return pagesDim == dim && texelSize == texel; }
    const Point2 getOrigin() const { return Point2::xy(originXZ) * texelSize; }
    const int getTexelsSize() const { return (pagesDim * texTileSize); }
    const float getSize() const { return getTexelsSize() * texelSize; }
    const bool isValid() const { return pagesDim > 0; }
  } fallback;
  bool allocateFallback(int dim, int tex_tile_size); // return true if changed indirection

private:
  bool canUseUAVFeedback() const;
};

static constexpr int calc_squared_area_around(int tile_width) { return SQR(tile_width * 2 + 1); }

class ClipmapImpl
{
public:
#if MAX_RI_VTEX_CNT_BITS > 0
  using PackedTile = uint2;
#else
  using PackedTile = uint32_t; // not only packing assumes it, but cnt resides in the first entry of the same buffer
#endif

  static constexpr int NUM_TILES_AROUND_RI_FIXED_MIP = 0;
  static constexpr int NUM_TILES_AROUND_RI_FALLBACK = 1;

  static constexpr int NUM_TILES_AROUND = 2;

  static constexpr int MAX_FAKE_TILE_CNT = // based on processTilesAroundCamera
    (calc_squared_area_around(NUM_TILES_AROUND) + calc_squared_area_around(NUM_TILES_AROUND / 2)) * MAX_VTEX_CNT +
    (calc_squared_area_around(NUM_TILES_AROUND_RI_FIXED_MIP) + calc_squared_area_around(NUM_TILES_AROUND_RI_FIXED_MIP)) *
      MAX_RI_VTEX_CNT;

  static constexpr int MAX_CLIPMAP = 9;
  enum
  {
    SOFTWARE_FEEDBACK,
    CPU_HW_FEEDBACK
  };

  static constexpr uint32_t MAX_TILE_CAPTURE_INFO_BUF_ELEMENTS =
    min(TILE_WIDTH * TILE_WIDTH * MAX_VTEX_CNT, FEEDBACK_WIDTH *FEEDBACK_HEIGHT);
  static constexpr uint32_t MAX_TILE_INFO_ELEMENT_CNT = MAX_TILE_CAPTURE_INFO_BUF_ELEMENTS * 2 + MAX_FAKE_TILE_CNT;

  void initVirtualTexture(int cacheDimX, int cacheDimY);
  void closeVirtualTexture();
  bool updateOrigin(const Point3 &cameraPosition, bool update_snap);

  // fallbackTexelSize is fallbackPage size in meters
  // can be set to something like max(4, (1<<(getAlignMips()-1))*getPixelRatio*8) - 8 times bigger texel than last aligned clip, but
  // not less than 4 meters if we allocate two pages, it results in minimum 4*256*2 == 1024meters (512 meters radius) of fallback
  // pages_dim*pages_dim is amount of pages allocated for fallback
  void initFallbackPage(int pages_dim, float fallback_texel_size);

  bool canUseOptimizedReadback(int captureTargetIdx) const;
  HWFeedbackMode getHWFeedbackMode(int captureTargetIdx) const;

  void updateFromFeedback(HWFeedbackMode hw_feedback_mode, TMatrix4 &globtm, char *feedbackCharPtr, char *feedbackTilePtr,
    int feedbackTileCnt, int oldXOffset, int oldYOffset, int oldZoom, const Point4 &uv2l, bool force_update);
  void prepareTileInfoFromTextureFeedback(HWFeedbackMode hw_feedback_mode, TMatrix4 &globtm, char *feedbackCharPtr, int oldXOffset,
    int oldYOffset, int oldZoom, const Point4 &uv2l, bool force_update);

  bool changeXYOffset(int nx, int ny);                  // return true if chaned
  bool changeZoom(int newZoom, const Point3 &position); // return true if chaned
  void centerOffset(const Point3 &position, int &xofs, int &yofs, float zoom) const;
  void update(bool force_update);
  void updateMip(HWFeedbackMode hw_feedback_mode, bool force_update);
  void updateCache(bool force_update);
  void checkConsistency();


  Point3 getMappedRiPos(int ri_index) const;

  Point3 getMappedRelativeRiPos(int ri_index, const Point3 &viewer_position, float &dist_to_ground) const;

  Point4 getLandscape2uv(const Point2 &offset, float zoom) const;
  Point4 getLandscape2uv(int ri_index) const;
  Point4 getUV2landacape(const Point2 &offset, float zoom) const;
  Point4 getUV2landacape(int ri_index) const;

  IPoint2 getTileOffset(int ri_index) const;

  Point4 getFallbackLandscape2uv() const;

  void restoreIndirectionFromLRU(SmallTab<TexTileIndirection, MidmemAlloc> *tiles); // debug
  void GPUrestoreIndirectionFromLRUFull();                                          // restore completely
  void GPUrestoreIndirectionFromLRU();
  int getTexMips() const { return texMips; }
  int getAlignMips() const { return alignMips; }
  void setMaxTileUpdateCount(int count) { maxTileUpdateCount = count; }
  __forceinline int getZoom() const { return currentContext->tileZoom; }
  __forceinline float getPixelRatio() const { return currentContext->tileZoom * pixelRatio; }

  __forceinline TexTileIndirection &atTile(SmallTab<TexTileIndirection, MidmemAlloc> *tiles, int mip, int addr)
  {
    // G_ASSERTF(currentContext, "current context");
    return tiles[mip][addr];
  }
  void setTexMips(int tex_mips);
  ClipmapImpl(const char *in_postfix = NULL, bool in_use_uav_feedback = true) : // same as TEXFMT_A8R8G8B8
    postfix(in_postfix),
    viewerPosition(0.f, 0.f, 0.f),
    tileInfoSize(0),
    mipCnt(1),
    cacheWidth(0),
    cacheHeight(0),
    cacheDimX(0),
    cacheDimY(0),
    tileLimit(0),
    maxTileUpdateCount(1024),
    use_uav_feedback(in_use_uav_feedback)
  {
    G_STATIC_ASSERT((1 << TILE_WIDTH_BITS) == TILE_WIDTH);

    mem_set_0(compressor);
    texTileBorder = 1; // bilinear only
    texTileInnerSize = texTileSize - 2 * texTileBorder;
    texMips = 0;
    pixelRatio = 1.0f;
    maxTexelSize = 32.0f;
    pClipmapRenderer = NULL;
    feedbackType = SOFTWARE_FEEDBACK;
    feedbackDepthTex.close();
    changedLRUscount = 0;
    failLockCount = 0;
    // END OF VIRTUAL TEXTURE INITIALIZATION

    mem_set_0(updatedClipmaps);
    mem_set_0(tileInfo);

#if MAX_RI_VTEX_CNT_BITS > 0
    mem_set_0(riInvMatrices);
    mem_set_0(riMappings);
    mem_set_0(changedRiIndices);
#endif
  }
  ~ClipmapImpl() { close(); }
  // sz - texture size of each clip, clip_count - number of clipmap stack. multiplayer - size of next clip
  // size in meters of latest clip, virtual_texture_mip_cnt - amount of mips(lods) of virtual texture containing tiles
  void init(float st_texel_size, uint32_t feedbackType, int texMips = 6,
#if _TARGET_IOS || _TARGET_ANDROID || _TARGET_C3
    int tex_tile_size = 128
#else
    int tex_tile_size = 256
#endif
    ,
    int virtual_texture_mip_cnt = 1, int virtual_texture_anisotropy = ::dgs_tex_anisotropy, int min_tex_tile_border = 1);
  void close();

  // returns bits of rendered clip, or 0 if no clip was rendered
  void setMaxTexelSize(float max_texel_size) { maxTexelSize = max_texel_size; }

  float getStartTexelSize() const { return pixelRatio; }
  void setStartTexelSize(float st_texel_size) // same as in init
  {
    pixelRatio = st_texel_size;
    invalidate(true);
  }

  void setTargetSize(int w, int h) { targetSize = IPoint2(w, h); };

  void prepareRender(ClipmapRenderer &render, bool force_update = false, bool turn_off_decals_on_fallback = false);
  void prepareFeedback(ClipmapRenderer &render, const Point3 &viewer_pos, const TMatrix &view_itm, const TMatrix4 &globtm,
    float height, float maxDist0 = 0.f, float maxDist1 = 0.f, float approx_ht = 0.f, bool force_update = false,
    bool low_tpool_prio = true); // for software feeback only
  void finalizeFeedback();

  void invalidatePointi(int tx, int ty, int lastMip = 0xFF); // not forces redraw!  (int tile coords)
  void invalidatePoint(const Point2 &world_xz);              // not forces redraw!
  void invalidateBox(const BBox2 &world_xz);                 // not forces redraw!
  void invalidate(bool force_redraw = true);
  void dump();

  bool getBBox(BBox2 &ret) const;
  bool getMaximumBBox(BBox2 &ret) const;
  void setFeedbackType(uint32_t ftp);
  uint32_t getFeedbackType() const { return feedbackType; }
  void setSoftwareFeedbackRadius(int inner_tiles, int outer_tiles);
  void setSoftwareFeedbackMipTiles(int mip, int tiles_for_mip);

  void enableUAVOutput(bool enable); // default is enabled
  void startUAVFeedback(int reg_no = 6);
  void endUAVFeedback(int reg_no = 6);
  void copyUAVFeedback();
  const UniqueTexHolder &getCache(int at) { return cache[at]; }                   // for debug
  const UniqueTex &getIndirection() const { return currentContext->indirection; } // for debug
  void createCaches(const uint32_t *formats, uint32_t cnt, const uint32_t *buffer_formats, uint32_t buffer_cnt);
  void afterReset();

  static bool is_uav_supported();

private:
  using TileInfoArr = carray<TexTileInfo, MAX_TILE_INFO_ELEMENT_CNT>;

  void beginUpdateTiles();
  void updateTileBlock(int mip, int cx, int cy, int x, int y, int ri_index);
  void updateTileBlockRaw(int cx, int cy, const BBox2 &region);
  void endUpdateTiles();
  void setDDScale();

  bool canUseUAVFeedback() const;

  void dispatchTileFeedback(int captureTargetIdx);

  void copyAndAdvanceCaptureTarget(int captureTargetIdx);
  void prepareSoftwareFeedback(int zoomPow2, const Point3 &viewer_pos, const TMatrix &view_itm, const TMatrix4 &globtm,
    float approx_height, float maxDist0, float maxDist1, bool force_update);
  void prepareSoftwarePoi(const int vx, const int vy, const int mip, const int mipsize,
    carray<uint64_t, (TEX_MIPS * TILE_WIDTH * TILE_WIDTH + 63) / 64> &bitarrayUsed, const Point4 &uv2landscapetile,
    const vec4f *planes2d, int planes2dCnt);
  BBox2 currentLRUBox;
  void checkUpload();
  uint32_t feedbackType;
  carray<bool, MAX_CLIPMAP> updatedClipmaps;
  static constexpr int MAX_CACHE_TILES = 1024; // 8192x8192
  carray<TexLRU, MAX_CACHE_TILES * 2> changedLRUs;
  int changedLRUscount;
  int maxTileUpdateCount;

  SmallTab<TexTileFeedback, TmpmemAlloc> lockedPixels;
  SmallTab<PackedTile, TmpmemAlloc> lockedTileInfo;
  uint32_t lockedTileInfoCnt = 0;

  friend class PrepareFeedbackJob;
  int swFeedbackInnerMipTiles = 2;
  int swFeedbackOuterMipTiles = 3;

  ShaderMaterial *constantFillerMat = 0;
  ShaderElement *constantFillerElement = 0;
  ShaderMaterial *directUncompressMat = 0;
  ShaderElement *directUncompressElem = 0;
  DynShaderQuadBuf *constantFillerBuf = 0;
  RingDynamicVB *constantFillerVBuf = 0;
  bool indirectionChanged = false;

  int cacheAnisotropy = 1;
  static constexpr int MAX_TEXTURES = MipContext::MAX_TEXTURES;

  carray<UniqueTexHolder, MAX_TEXTURES> bufferTex;
  int bufferCnt = 0;

  carray<BcCompressor *, MAX_TEXTURES> compressor;
  carray<UniqueTexHolder, MAX_TEXTURES> cache;
  int cacheCnt = 0;
  int mipCnt = 1;
  int fallbackFramesUpdate = 0;

  void changeFallback(const Point3 &viewer, bool turn_off_decals_on_fallback);

  void updatePyramidTex(); // internal! do not saves/restores any render states
  void initHWFeedback();
  void closeHWFeedback();
  void updateRenderStates();
  void finalizeCompressionQueue();
  void addCompressionQueue(int cx, int cy);

  void updateRendinstLandclassParams();

  void processTileFeedback(TileInfoArr &result_arr, int &result_size, char *feedback_tile_ptr, int feedback_tile_cnt,
    const TMatrix4 &globtm, const Point4 &uv2l);

private:
  SimpleString postfix;
  vec4f align_hist; /// aligns histogramm to 128 bit!
  carray<uint16_t, TEX_TOTAL_ELEMENTS> hist;
  ClipmapRenderer *pClipmapRenderer = 0;
  UniqueTex feedbackDepthTex;
  eastl::unique_ptr<MipContext> currentContext;

#if MAX_RI_VTEX_CNT_BITS > 0
  int riLandclassCount = 0;
  carray<TMatrix, MAX_RI_VTEX_CNT> riInvMatrices;
  carray<Point4, MAX_RI_VTEX_CNT> riMappings;
  dag::Vector<rendinst::RendinstLandclassData> prevRendinstLandclasses;
  eastl::array<int, MAX_RI_VTEX_CNT> changedRiIndices;
#endif

  PostFxRenderer clearHistogramPs;
  PostFxRenderer fillHistogramPs;
  PostFxRenderer buildTileInfoPs;

  eastl::unique_ptr<ComputeShaderElement> clearHistogramCs;
  eastl::unique_ptr<ComputeShaderElement> fillHistogramCs;
  eastl::unique_ptr<ComputeShaderElement> buildTileInfoCs;

  static constexpr int COMPRESS_QUEUE_SIZE = 4;
  StaticTab<IPoint2, COMPRESS_QUEUE_SIZE> compressionQueue;
  int texMips = 0, mostLeft = 0, mostRight = 0, alignMips = 6;
  int texTileBorder, texTileInnerSize;
  void setTileSizeVars();
  int failLockCount = 0;

  int cacheWidth = 0, cacheHeight = 0, cacheDimX = 0, cacheDimY = 0;
  TileInfoArr tileInfo;
  int tileInfoSize = 0;

  IPoint2 targetSize = {-1, -1};

  // for debug comparison
  TileInfoArr debugTileInfo;
  int debugTileInfoSize = 0;

  float pixelRatio = 0, maxTexelSize = 0;
  int tileLimit = 0, maxTileLimit = 0, zoomTileLimit = 0;
  int texTileSize = 256;
  int minTexTileBorder = 1;
  eastl::array<int, TEX_MIPS> swFeedbackTilesPerMip;

  int fallbackPages = 0;
  float fallbackTexelSize = 1;

  Point3 viewerPosition;

  int feedback_landscape2uvVarId = -1;
  int indirectionTexVarId = -1;
  int landscape2uvVarId = -1;
  int worldDDScaleVarId = -1;
  int vTexDimVarId = -1;
  int tileSizeVarId = -1;
  int supports_uavVarId = -1;
  int g_cache2uvVarId = -1;
  int fallback_info0VarId = -1, fallback_info1VarId = -1;

  bool use_uav_feedback = true;

  void initCurrentContext();
  void freeCurrentContext();
};


const IPoint2 Clipmap::FEEDBACK_SIZE = IPoint2(FEEDBACK_WIDTH, FEEDBACK_HEIGHT);
const int Clipmap::MAX_TEX_MIP_CNT = TEX_MIPS;

void Clipmap::initVirtualTexture(int cacheDimX, int cacheDimY) { clipmapImpl->initVirtualTexture(cacheDimX, cacheDimY); }
void Clipmap::closeVirtualTexture() { clipmapImpl->closeVirtualTexture(); }
bool Clipmap::updateOrigin(const Point3 &cameraPosition, bool update_snap)
{
  return clipmapImpl->updateOrigin(cameraPosition, update_snap);
}

void Clipmap::initFallbackPage(int pages_dim, float fallback_texel_size)
{
  clipmapImpl->initFallbackPage(pages_dim, fallback_texel_size);
}

bool Clipmap::canUseOptimizedReadback(int captureTargetIdx) const { return clipmapImpl->canUseOptimizedReadback(captureTargetIdx); }
HWFeedbackMode Clipmap::getHWFeedbackMode(int captureTargetIdx) const { return clipmapImpl->getHWFeedbackMode(captureTargetIdx); }

void Clipmap::updateFromFeedback(HWFeedbackMode hw_feedback_mode, TMatrix4 &globtm, char *feedbackCharPtr, char *feedbackTilePtr,
  int feedbackTileCnt, int oldXOffset, int oldYOffset, int oldZoom, const Point4 &uv2l, bool force_update)
{
  clipmapImpl->updateFromFeedback(hw_feedback_mode, globtm, feedbackCharPtr, feedbackTilePtr, feedbackTileCnt, oldXOffset, oldYOffset,
    oldZoom, uv2l, force_update);
}
void Clipmap::prepareTileInfoFromTextureFeedback(HWFeedbackMode hw_feedback_mode, TMatrix4 &globtm, char *feedbackCharPtr,
  int oldXOffset, int oldYOffset, int oldZoom, const Point4 &uv2l, bool force_update)
{
  clipmapImpl->prepareTileInfoFromTextureFeedback(hw_feedback_mode, globtm, feedbackCharPtr, oldXOffset, oldYOffset, oldZoom, uv2l,
    force_update);
}

bool Clipmap::changeXYOffset(int nx, int ny) { return clipmapImpl->changeXYOffset(nx, ny); }
bool Clipmap::changeZoom(int newZoom, const Point3 &position) { return clipmapImpl->changeZoom(newZoom, position); }
void Clipmap::update(bool force_update) { clipmapImpl->update(force_update); }
void Clipmap::updateMip(HWFeedbackMode hw_feedback_mode, bool force_update) { clipmapImpl->updateMip(hw_feedback_mode, force_update); }
void Clipmap::updateCache(bool force_update) { clipmapImpl->updateCache(force_update); }
void Clipmap::checkConsistency() { clipmapImpl->checkConsistency(); }
void Clipmap::restoreIndirectionFromLRU(SmallTab<TexTileIndirection, MidmemAlloc> *tiles)
{
  clipmapImpl->restoreIndirectionFromLRU(tiles);
}
void Clipmap::GPUrestoreIndirectionFromLRUFull() { clipmapImpl->GPUrestoreIndirectionFromLRUFull(); }
void Clipmap::GPUrestoreIndirectionFromLRU() { clipmapImpl->GPUrestoreIndirectionFromLRU(); }
int Clipmap::getTexMips() const { return clipmapImpl->getTexMips(); }
int Clipmap::getAlignMips() const { return clipmapImpl->getAlignMips(); }
void Clipmap::setMaxTileUpdateCount(int count) { clipmapImpl->setMaxTileUpdateCount(count); }
int Clipmap::getZoom() const { return clipmapImpl->getZoom(); }
float Clipmap::getPixelRatio() const { return clipmapImpl->getPixelRatio(); }

TexTileIndirection &Clipmap::atTile(SmallTab<TexTileIndirection, MidmemAlloc> *tiles, int mip, int addr)
{
  return clipmapImpl->atTile(tiles, mip, addr);
}
void Clipmap::setTexMips(int tex_mips) { clipmapImpl->setTexMips(tex_mips); }

Clipmap::Clipmap(const char *in_postfix, bool in_use_uav_feedback) :
  clipmapImpl(eastl::make_unique<ClipmapImpl>(in_postfix, in_use_uav_feedback))
{}
Clipmap::~Clipmap() { close(); }


void Clipmap::init(float st_texel_size, uint32_t feedbackType, int texMips, int tex_tile_size, int virtual_texture_mip_cnt,
  int virtual_texture_anisotropy, int min_tex_tile_border)
{
  clipmapImpl->init(st_texel_size, feedbackType, texMips, tex_tile_size, virtual_texture_mip_cnt, virtual_texture_anisotropy,
    min_tex_tile_border);
}

void Clipmap::close() { clipmapImpl->close(); }


void Clipmap::setMaxTexelSize(float max_texel_size) { clipmapImpl->setMaxTexelSize(max_texel_size); }
float Clipmap::getStartTexelSize() const { return clipmapImpl->getStartTexelSize(); }
void Clipmap::setStartTexelSize(float st_texel_size) { clipmapImpl->setStartTexelSize(st_texel_size); }
void Clipmap::setTargetSize(int w, int h) { clipmapImpl->setTargetSize(w, h); }
void Clipmap::prepareRender(ClipmapRenderer &render, bool force_update, bool turn_off_decals_on_fallback)
{
  clipmapImpl->prepareRender(render, force_update, turn_off_decals_on_fallback);
}
void Clipmap::prepareFeedback(ClipmapRenderer &render, const Point3 &viewer_pos, const TMatrix &view_itm, const TMatrix4 &globtm,
  float height, float maxDist0, float maxDist1, float approx_ht, bool force_update, bool low_tpool_prio)
{
  clipmapImpl->prepareFeedback(render, viewer_pos, view_itm, globtm, height, maxDist0, maxDist1, approx_ht, force_update,
    low_tpool_prio);
}
void Clipmap::finalizeFeedback() { clipmapImpl->finalizeFeedback(); }
void Clipmap::invalidatePointi(int tx, int ty, int lastMip) { clipmapImpl->invalidatePointi(tx, ty, lastMip); }
void Clipmap::invalidatePoint(const Point2 &world_xz) { clipmapImpl->invalidatePoint(world_xz); }
void Clipmap::invalidateBox(const BBox2 &world_xz) { clipmapImpl->invalidateBox(world_xz); }
void Clipmap::invalidate(bool force_redraw) { clipmapImpl->invalidate(force_redraw); }
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
void Clipmap::enableUAVOutput(bool enable) { clipmapImpl->enableUAVOutput(enable); }
void Clipmap::startUAVFeedback(int reg_no) { clipmapImpl->startUAVFeedback(reg_no); }
void Clipmap::endUAVFeedback(int reg_no) { clipmapImpl->endUAVFeedback(reg_no); }
void Clipmap::copyUAVFeedback() { clipmapImpl->copyUAVFeedback(); }
const UniqueTexHolder &Clipmap::getCache(int at) { return clipmapImpl->getCache(at); }
const UniqueTex &Clipmap::getIndirection() const { return clipmapImpl->getIndirection(); }
void Clipmap::createCaches(const uint32_t *formats, uint32_t cnt, const uint32_t *buffer_formats, uint32_t buffer_cnt)
{
  clipmapImpl->createCaches(formats, cnt, buffer_formats, buffer_cnt);
}
void Clipmap::afterReset() { clipmapImpl->afterReset(); }

bool Clipmap::is_uav_supported() { return ClipmapImpl::is_uav_supported(); }


#define VTEX_VARS_LIST                               \
  VAR(clipmap_feedback_tex)                          \
  VAR(clipmap_tex_mips)                              \
  VAR(clipmap_ri_landscape2uv_arr)                   \
  VAR(ri_landclass_closest_indices_0)                \
  VAR(ri_landclass_closest_indices_1)                \
  VAR(ri_landclass_changed_indicex_bits)             \
  VAR(tex_hmap_inv_sizes)                            \
  VAR(rendinst_landscape_area_left_top_right_bottom) \
  VAR(clipmap_histogram_buf)
#define VAR(a) static int a##VarId = -1;
VTEX_VARS_LIST
#undef VAR

#define VTEX_CONST_LIST               \
  VAR(clipmap_clear_histogram_buf_rw) \
  VAR(clipmap_clear_tile_info_buf_rw) \
  VAR(clipmap_histogram_buf_rw)       \
  VAR(clipmap_tile_info_cnt_rw)       \
  VAR(clipmap_tile_info_buf_rw)
#define VAR(a) static int a##_const_no = -1;
VTEX_CONST_LIST
#undef VAR

static void init_shader_vars()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  VTEX_VARS_LIST
#undef VAR

#define VAR(a)                                              \
  {                                                         \
    int tmp = get_shader_variable_id(#a "_const_no", true); \
    if (tmp >= 0)                                           \
      a##_const_no = ShaderGlobal::get_int_fast(tmp);       \
  }
  VTEX_CONST_LIST
#undef VAR
}


CONSOLE_INT_VAL("clipmap", ri_invalid_frame_cnt, 6, 0, 20);
CONSOLE_FLOAT_VAL_MINMAX("clipmap", ri_fake_tile_max_height, 10.0f, 1.0f, 100.0f);

CONSOLE_BOOL_VAL("clipmap", disable_fake_tiles, false);
CONSOLE_FLOAT_VAL_MINMAX("clipmap", fixed_zoom, -1, -1.0f, 64.0f);
CONSOLE_BOOL_VAL("clipmap", disable_feedback, false);
CONSOLE_BOOL_VAL("clipmap", log_ri_indirection_change, false);


static constexpr float RI_LANDCLASS_FIXED_ZOOM = 1;


// CS is a bit faster when isolated, but PS can interleave with neighboring calls, making it overall faster
// BUT: PS is broken on Vulkan for some reason // TODO: fix it
CONSOLE_BOOL_VAL("clipmap", use_cs_for_hw_feedback_process, true);

#if _TARGET_ANDROID
// some Adreno devices go mad about this
ConVarT<int, true> hw_feedback_target_mode("clipmap.hw_feedback_target_mode", 0, 0, 2,
  "0 = original, 1 = optimized, 2 = debug compare");
#else
ConVarT<int, true> hw_feedback_target_mode("clipmap.hw_feedback_target_mode", 1, 0, 2,
  "0 = original, 1 = optimized, 2 = debug compare");
#endif


template <typename T, size_t N>
using TempSet =
  eastl::vector_set<T, eastl::less<T>, framemem_allocator, eastl::fixed_vector<T, N, /*overflow*/ true, framemem_allocator>>;

template <typename K, typename T, size_t N>
using TempMap = eastl::vector_map<K, T, eastl::less<K>, framemem_allocator,
  eastl::fixed_vector<eastl::pair<K, T>, N, /*overflow*/ true, framemem_allocator>>;

#define CHECK_MEMGUARD 0

#define MAX_ZOOM_TO_ALLOW_PARALLAX           8
#define MAX_ZOOM_TO_ALLOW_FAKE_AROUND_CAMERA 64 // all tiles!
#define MIP_AROUND_CAMERA                    2
#define FRAME_FRACTION_TO_SPEND_IN_UPDATE    0.06 // limit update time to 1 msec at 60 FPS

#define RESTORE_IN_INDIRECTION_ONLY_USED_LRU \
  0 // change this to 1, if we need only used LRU tiles in indirection. However, on SLI/Crossfire we always restore only used

static constexpr uint16_t MORE_THAN_FEEDBACK = FEEDBACK_WIDTH * FEEDBACK_HEIGHT + 1;
static constexpr uint16_t MORE_THAN_FEEDBACK_INV = 0xFFFF - MORE_THAN_FEEDBACK;


static const uint4 TILE_PACKED_BITS = uint4(TILE_PACKED_X_BITS, TILE_PACKED_Y_BITS, TILE_PACKED_MIP_BITS, TILE_PACKED_COUNT_BITS);
static const uint4 TILE_PACKED_SHIFT =
  uint4(0, TILE_PACKED_BITS.x, TILE_PACKED_BITS.x + TILE_PACKED_BITS.y, TILE_PACKED_BITS.x + TILE_PACKED_BITS.y + TILE_PACKED_BITS.z);
static const uint4 TILE_PACKED_MASK =
  uint4((1 << TILE_PACKED_BITS.x) - 1, (1 << TILE_PACKED_BITS.y) - 1, (1 << TILE_PACKED_BITS.z) - 1, (1 << TILE_PACKED_BITS.w) - 1);
static const uint16_t MAX_GPU_FEEDBACK_COUNT = TILE_PACKED_MASK.w;


static bool checkTextureFeedbackUsage(HWFeedbackMode feedback_mode) { return feedback_mode != HWFeedbackMode::DEFAULT_OPTIMIZED; }

static bool checkTileFeedbackUsage(HWFeedbackMode feedback_mode)
{
  return feedback_mode == HWFeedbackMode::DEFAULT_OPTIMIZED || feedback_mode == HWFeedbackMode::DEBUG_COMPARE;
}


// this scales feedback view and so WASTES cache a lot
// but it helps to fight with latency on feedback
static const float feedback_view_scale = 0.95;


static uint4 unpackTileInfo(ClipmapImpl::PackedTile packed_data, uint32_t &ri_index)
{
  uint4 data;
#if MAX_RI_VTEX_CNT_BITS > 0
  for (int i = 0; i < 4; ++i)
    data[i] = (packed_data.x >> TILE_PACKED_SHIFT[i]) & TILE_PACKED_MASK[i];
  ri_index = packed_data.y;
  G_FAST_ASSERT(ri_index < MAX_VTEX_CNT);
#else
  ri_index = 0;
  for (int i = 0; i < 4; ++i)
    data[i] = (packed_data >> TILE_PACKED_SHIFT[i]) & TILE_PACKED_MASK[i];
#endif
  return data;
}

__forceinline uint32_t TagOfTile(uint32_t mip, uint32_t x, uint32_t y, uint32_t ri_index)
{
  G_FAST_ASSERT(x < TILE_WIDTH);
  G_FAST_ASSERT(y < TILE_WIDTH);
  G_FAST_ASSERT(mip < TEX_MIPS);
  G_FAST_ASSERT(ri_index < MAX_VTEX_CNT);
  return (mip << (TILE_WIDTH_BITS + TILE_WIDTH_BITS + MAX_RI_VTEX_CNT_BITS)) | (ri_index << (TILE_WIDTH_BITS + TILE_WIDTH_BITS)) |
         (y << TILE_WIDTH_BITS) | (x);
}
__forceinline uint32_t IndexOfTileInBit(uint32_t mip, uint32_t x, uint32_t y, uint32_t ri_index)
{
  return TagOfTile(mip, x, y, ri_index);
}

struct SortBySortOrder
{
  bool operator()(const TexTileInfo &a, const TexTileInfo &b) const { return a.sortOrder < b.sortOrder; }
};
static uint64_t calc_tile_sort_order(const TexTileInfo &tile)
{
  uint32_t count = tile.count * (TEX_MIPS - tile.mip);
  uint32_t index = TagOfTile(tile.mip, tile.x, tile.y, tile.ri_index);
  return ~(uint64_t)count << 32 | index;
}

void MipContext::init(bool in_use_uav_feedback)
{
  use_uav_feedback = in_use_uav_feedback;

  for (int i = 0; i < MAX_FRAMES_AHEAD; ++i)
  {
    captureTex[i].close();
    captureTileInfoBuf[i].close();
    captureWorldOffset[i][0] = 0;
    captureWorldOffset[i][1] = 0;
    captureWorldOffset[i][2] = 1;
    captureUV2landscape[i] = Point4(1, 1, 0, 0);
    debugHWFeedbackConvar[i] = 0;
  }
  intermediateHistBuf.close();
  captureTarget = oldestCaptureTargetIdx = 0;
}

void MipContext::resetInCacheBitmap() { inCacheBitmap.reset(); }

void MipContext::reset(int cacheDimX, int cacheDimY, bool force_redraw, int tex_tile_size, int feedback_type)
{
  if (force_redraw && feedback_type == ClipmapImpl::CPU_HW_FEEDBACK)
    initHWFeedback();
  invalidateIndirection = true;
  fallback.originXZ = IPoint2(-100000, 100000);
  if (force_redraw)
  {
    int lruDimX = cacheDimX / tex_tile_size;
    int lruDimY = cacheDimY / tex_tile_size;
    LRU.clear();
    LRU.reserve(lruDimY * lruDimX);
    for (int y = 0; y < lruDimY; ++y)
      for (int x = (y < fallback.pagesDim ? fallback.pagesDim : 0); x < lruDimX; ++x)
        LRU.push_back(TexLRU(x, y));
    resetInCacheBitmap();
  }
  else
  {
    for (int i = 0; i < LRU.size(); ++i)
      if (LRU[i].mip != 0xFF) // && (LRU[i].flags & LRU[i].IS_USED))
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
static void sort_tile_info_list(T &tileInfo, int tileInfoSize)
{
  for (int tileNo = 0; tileNo < tileInfoSize; tileNo++)
    tileInfo[tileNo].sortOrder = calc_tile_sort_order(tileInfo[tileNo]);
  stlsort::sort(tileInfo.data(), tileInfo.data() + tileInfoSize, SortBySortOrder());
}

static bool compare_tile_info_results(carray<TexTileInfo, ClipmapImpl::MAX_TILE_INFO_ELEMENT_CNT> &tileInfo0, int tileInfoSize0,
  carray<TexTileInfo, ClipmapImpl::MAX_TILE_INFO_ELEMENT_CNT> &tileInfo1, int tileInfoSize1, int &firstMismatchId)
{
  firstMismatchId = -1;
  if (tileInfoSize0 != tileInfoSize1)
    return false;
  if (tileInfoSize0 == 0)
    return true;
  for (int i = 0; i < tileInfoSize0; ++i)
    if (tileInfo0[i] != tileInfo1[i])
    {
      firstMismatchId = i;
      return false;
    }
  return true;
}


template <typename Funct>
static void processTilesAroundCamera(int tex_mips, const TMatrix4 &globtm, const Point4 &uv2l, const Point4 &l2uv,
  const Point3 &viewerPosition, const Point4 &l2uv0, int ri_index, float dist_to_ground, Funct funct)
{
  auto insertTilesTerrain = [&globtm, uv2l, funct](IPoint2 tile_center, int mipToAdd, int tilesAround) {
    static constexpr int ri_index = 0;
    carray<plane3f, 12> planes2d;
    int planes2dCnt = 0;
    prepare_2d_frustum(globtm, planes2d.data(), planes2dCnt);
    Point4 uv2landscapetile(uv2l.x / TILE_WIDTH, uv2l.y / TILE_WIDTH, uv2l.z, uv2l.w);
    vec4f width = v_make_vec4f(uv2landscapetile.x * (1 << mipToAdd), 0, 0, 0);
    vec4f height = v_make_vec4f(0, 0, uv2landscapetile.y * (1 << mipToAdd), 0);
    int uvy = (tile_center.y >> mipToAdd) + TILE_WIDTH / 2, uvx = (tile_center.x >> mipToAdd) + TILE_WIDTH / 2;
    int hist_base_ofs = TagOfTile(mipToAdd, 0, 0, ri_index);
    for (int y = -tilesAround; y <= tilesAround; ++y)
    {
      int cy = uvy + y;
      if (cy >= 0 && cy < TILE_WIDTH)
      {
        int hist_ofs_y = hist_base_ofs | TagOfTile(0, 0, cy, 0);
        for (int x = -tilesAround; x <= tilesAround; ++x)
        {
          int cx = uvx + x;
          if (cx >= 0 && cx < TILE_WIDTH)
          {
            if (planes2dCnt > 0)
            {
              // software 2d culling to save some cache
              int worldCx = ((cx - TILE_WIDTH / 2) << mipToAdd) + TILE_WIDTH / 2,
                  worldCy = ((cy - TILE_WIDTH / 2) << mipToAdd) + TILE_WIDTH / 2;
              vec4f pos = v_make_vec4f(uv2landscapetile.x * worldCx + uv2landscapetile.z, 0,
                uv2landscapetile.y * worldCy + uv2landscapetile.w, 0);
              if (!is_2d_frustum_visible(pos, width, height, planes2d.data(), planes2dCnt))
                continue;
            }
            int histOfs = hist_ofs_y | TagOfTile(0, cx, 0, 0);
            funct(cx, cy, mipToAdd, ri_index, histOfs);
          }
        }
      }
    }
  };
  auto insertTilesRI = [ri_index, funct](IPoint2 tile_center, int mipToAdd, int tilesAround) {
    G_ASSERT(ri_index != 0);
    int uvy = (tile_center.y >> mipToAdd) + TILE_WIDTH / 2, uvx = (tile_center.x >> mipToAdd) + TILE_WIDTH / 2;
    int hist_base_ofs = TagOfTile(mipToAdd, 0, 0, ri_index);
    for (int y = -tilesAround; y <= tilesAround; ++y)
    {
      int cy = uvy + y;
      if (cy >= 0 && cy < TILE_WIDTH)
      {
        int hist_ofs_y = hist_base_ofs | TagOfTile(0, 0, cy, 0);
        for (int x = -tilesAround; x <= tilesAround; ++x)
        {
          int cx = uvx + x;
          if (cx >= 0 && cx < TILE_WIDTH)
          {
            // TODO: bbox culling for RI
            int histOfs = hist_ofs_y | TagOfTile(0, cx, 0, 0);
            funct(cx, cy, mipToAdd, ri_index, histOfs);
          }
        }
      }
    }
  };
  auto calcTileCenter = [viewerPosition](const Point4 &l2uv) {
    Point2 viewerUV = Point2(viewerPosition.x * l2uv.x + l2uv.z, viewerPosition.z * l2uv.y + l2uv.w);
    return IPoint2((int)(viewerUV.x * TILE_WIDTH) - (TILE_WIDTH / 2), (int)(viewerUV.y * TILE_WIDTH) - (TILE_WIDTH / 2));
  };

  IPoint2 tileCenter0 = calcTileCenter(l2uv0);
  if (ri_index != TERRAIN_RI_INDEX)
  {
    IPoint2 tileCenter = calcTileCenter(l2uv);
    tileCenter.x += (TILE_WIDTH / 2);
    tileCenter.y += (TILE_WIDTH / 2);
    int lastMip = tex_mips - 1;

    // insert fallback tiles to terrain indirection to make the transition smoother
    // Basically even when we render and have feedback from ri indirection, we also store the least detailed mip of the terrain
    // indirection for them, so when we switch, there is already a tile in the cache, and we don't need to use the terrain fallback
    // (which can be extremely blurry for ri tiles).
    insertTilesTerrain(tileCenter0, lastMip, ClipmapImpl::NUM_TILES_AROUND_RI_FIXED_MIP);

    // insert the least detailed tiles to RI indirection (so we never need to use the terrain fallback)
    insertTilesRI(IPoint2::ZERO, lastMip, ClipmapImpl::NUM_TILES_AROUND_RI_FALLBACK);

    // insert tiles close to camera to RI indirection
    if (dist_to_ground < ri_fake_tile_max_height)
      insertTilesRI(tileCenter, MIP_AROUND_CAMERA, 1);
  }
  else
  {
    for (int mipToAdd = MIP_AROUND_CAMERA, tilesAround = ClipmapImpl::NUM_TILES_AROUND; tilesAround; tilesAround /= 2, mipToAdd += 3)
      insertTilesTerrain(tileCenter0, mipToAdd, tilesAround);
  }
}

void ClipmapImpl::updateFromFeedback(HWFeedbackMode hw_feedback_mode, TMatrix4 &globtm, char *feedbackCharPtr, char *feedbackTilePtr,
  int feedbackTileCnt, int oldXOffset, int oldYOffset, int oldZoom, const Point4 &uv2l, bool force_update)
{
  {
    TIME_PROFILE_DEV(clipmap_updateFromFeedback);
    if (hw_feedback_mode == HWFeedbackMode::DEBUG_COMPARE)
    {
      processTileFeedback(debugTileInfo, debugTileInfoSize, feedbackTilePtr, feedbackTileCnt, globtm, uv2l);
      prepareTileInfoFromTextureFeedback(hw_feedback_mode, globtm, feedbackCharPtr, oldXOffset, oldYOffset, oldZoom, uv2l,
        force_update);
    }
    else if (hw_feedback_mode == HWFeedbackMode::DEFAULT_OPTIMIZED)
    {
      processTileFeedback(tileInfo, tileInfoSize, feedbackTilePtr, feedbackTileCnt, globtm, uv2l);
    }
    else
    {
      prepareTileInfoFromTextureFeedback(hw_feedback_mode, globtm, feedbackCharPtr, oldXOffset, oldYOffset, oldZoom, uv2l,
        force_update);
    }
  }

  if (tileInfoSize)
  {
    tileLimit = max(maxTileLimit, zoomTileLimit);
    updateMip(hw_feedback_mode, force_update);
  }
}

void ClipmapImpl::prepareTileInfoFromTextureFeedback(HWFeedbackMode hw_feedback_mode, TMatrix4 &globtm, char *feedbackCharPtr,
  int oldXOffset, int oldYOffset, int oldZoom, const Point4 &uv2l, bool force_update)
{
  // we do not have correct feedback for a stub driver, and random data
  // will cause the memory corruption
  if (d3d::is_stub_driver())
    return;

  TIME_PROFILE_DEV(clipmap_updateFromReadPixels);

  const TexTileFeedback *feedbackPtr = (const TexTileFeedback *)feedbackCharPtr;
  int width = FEEDBACK_WIDTH, height = FEEDBACK_HEIGHT;
  int total = width * height;

  if (disable_feedback)
    total = 0;

  tileInfoSize = 0;
  mem_set_0(hist);

  auto appendHist = [this](int x, int y, int mip, int ri_index) {
    int hist_ofs = TagOfTile(mip, x, y, ri_index);
    G_ASSERTF(hist_ofs < TEX_TOTAL_ELEMENTS, "%d, %d, %d, %d", x, y, mip, ri_index);
#if CHECK_MEMGUARD
    hist[hist_ofs]++;
#else
    hist.data()[hist_ofs]++;
#endif
  };

  if (hw_feedback_mode == HWFeedbackMode::ZOOM_CHANGED)
  {
    int newZoom = currentContext->tileZoom;
    if (!force_update)
    {
      if (newZoom < oldZoom - 1)
        return; // too big change
      if (newZoom > oldZoom + 1)
        return; // too big change
    }
    int oldZoomShift = get_log2i_of_pow2w(oldZoom);
    int newZoomShift = get_log2i_of_pow2w(newZoom);

    for (const TexTileFeedback *feedbackPtrEnd = feedbackPtr + total; feedbackPtr != feedbackPtrEnd; ++feedbackPtr)
    {
      const TexTileFeedback &fb = *feedbackPtr;

      if (fb.ri_index != TERRAIN_RI_INDEX)
      {
        if (fb.mip < texMips)
          appendHist(fb.x, fb.y, fb.mip, fb.ri_index);
      }
      else
      {
        int newMip = oldZoom > newZoom ? (fb.mip + 1) : max(0, fb.mip - 1);
        if (newMip >= texMips)
          continue;

        int gx = ((((int)fb.x - (TILE_WIDTH / 2)) << fb.mip) + (TILE_WIDTH / 2) + oldXOffset) << oldZoomShift;
        int gy = ((((int)fb.y - (TILE_WIDTH / 2)) << fb.mip) + (TILE_WIDTH / 2) + oldYOffset) << oldZoomShift;
        int nx = (((gx >> newZoomShift) - currentContext->xOffset - (TILE_WIDTH / 2)) >> newMip) + (TILE_WIDTH / 2);
        int ny = (((gy >> newZoomShift) - currentContext->yOffset - (TILE_WIDTH / 2)) >> newMip) + (TILE_WIDTH / 2);

        if (nx >= 0 && nx < TILE_WIDTH && ny >= 0 && ny < TILE_WIDTH)
          appendHist(nx, ny, newMip, 0);
      }
    }
  }
  else if (hw_feedback_mode == HWFeedbackMode::OFFSET_CHANGED)
  {
    // this is fairly rare case for when we changed origin and collecting old feedback
    int offsetDeltaX = oldXOffset - currentContext->xOffset;
    int offsetDeltaY = oldYOffset - currentContext->yOffset;
    for (const TexTileFeedback *feedbackPtrEnd = feedbackPtr + total; feedbackPtr != feedbackPtrEnd; ++feedbackPtr)
    {
      const TexTileFeedback &fb = *feedbackPtr;
      if (fb.mip < texMips)
      {
        if (fb.ri_index != TERRAIN_RI_INDEX)
        {
          appendHist(fb.x, fb.y, fb.mip, fb.ri_index);
        }
        else
        {
          // G_ASSERTF(fb.y < (TILE_WIDTH>>fb.mip), "fb = %dx%d mip = %d", fb.x, fb.y, fb.mip);
          // G_ASSERTF(fb.x < (TILE_WIDTH>>fb.mip), "fb = %dx%d mip = %d", fb.x, fb.y, fb.mip);
          // int mip = min(uint8_t(texMips-1),fb.mip);
          int mip = fb.mip;
          int newX = int(fb.x) + (offsetDeltaX >> mip);
          int newY = int(fb.y) + (offsetDeltaY >> mip);
          if (newX >= 0 && newX < TILE_WIDTH && newY >= 0 && newY < TILE_WIDTH)
            appendHist(newX, newY, mip, 0);
        }
      }
    }
  }
  else
  {
    for (const TexTileFeedback *feedbackPtrEnd = feedbackPtr + total; feedbackPtr != feedbackPtrEnd; ++feedbackPtr)
    {
      const TexTileFeedback &fb = *feedbackPtr;
      if (fb.mip < texMips)
      {
        G_FAST_ASSERT(fb.y < TILE_WIDTH && fb.x < TILE_WIDTH);
        appendHist(fb.x, fb.y, fb.mip, fb.ri_index);
      }
    }
  }

  if (hw_feedback_mode == HWFeedbackMode::DEBUG_COMPARE)
  {
    // For the optimized version (tile GPU readback), count is stored in 15 bits, which is not enough in extreme cases,
    // but the first N in the sorted tiles will still be the same, so it doesn't matter
    // To make the comparison fair and avoid false-positive errors, the original version must be clamped to simulate the same behavior.
    for (int i = 0; i < hist.size(); ++i)
      hist[i] = min(hist[i], MAX_GPU_FEEDBACK_COUNT);
  }

  if (!disable_fake_tiles && currentContext->tileZoom <= MAX_ZOOM_TO_ALLOW_FAKE_AROUND_CAMERA)
  {
    auto funct = [this](int x, int y, int mip, int ri_index, int hist_ofs) {
      if (hist[hist_ofs] >= MORE_THAN_FEEDBACK) // only apply fake tiles once (overlapping can cause issues otherwise)
        return;
#if CHECK_MEMGUARD
      hist[hist_ofs] = min(hist[hist_ofs], MORE_THAN_FEEDBACK_INV) + MORE_THAN_FEEDBACK;
#else
      hist.data()[hist_ofs] = min(hist.data()[hist_ofs], MORE_THAN_FEEDBACK_INV) + MORE_THAN_FEEDBACK;
#endif
    };
    for (int riIndex = 0; riIndex < MAX_VTEX_CNT; ++riIndex) // -V1008
    {
      float dist_to_ground;
      Point3 camPos = getMappedRelativeRiPos(riIndex, viewerPosition, dist_to_ground);
      processTilesAroundCamera(texMips, globtm, riIndex == 0 ? uv2l : getUV2landacape(riIndex), getLandscape2uv(riIndex), //-V547
        camPos, getLandscape2uv(TERRAIN_RI_INDEX), riIndex, dist_to_ground, funct);
    }
  }

  {
    uint16_t *__restrict pHist = static_cast<uint16_t *>(hist.data());
#if _TARGET_SIMD_SSE
    G_ASSERT(!(((intptr_t)pHist) & 0xF));
    G_STATIC_ASSERT(!(TILE_WIDTH & 7));
    vec4f *__restrict pHistVec = (vec4f *)pHist;
    vec4f zero = v_zero();
    for (uint32_t mip = 0; mip < texMips && tileInfoSize + 8 < tileInfo.size(); mip++)
      for (uint32_t ri_index = 0; ri_index < MAX_VTEX_CNT; ri_index++) // -V1008
      {
        static constexpr int X_MASK = (1 << TILE_WIDTH_BITS) - 1;
        for (uint32_t i = 0; i < (TILE_WIDTH * TILE_WIDTH) && tileInfoSize + 8 < tileInfo.size(); i += 8, pHistVec++)
        {
          vec4i eq = _mm_cmpeq_epi32(v_cast_vec4i(*pHistVec), v_cast_vec4i(zero));
          if (_mm_movemask_ps(v_cast_vec4f(eq)) == 0xF)
            continue;
          uint32_t y = (i >> TILE_WIDTH_BITS) & X_MASK;
          uint32_t x = i & X_MASK;
          uint32_t *chist = ((uint32_t *)pHistVec);
          uint32_t count2;

          count2 = chist[0];
          if (uint32_t count = (count2 & 0xffff))
            tileInfo[tileInfoSize++].set(x, y, mip, count, ri_index);
          if (uint32_t count = (count2 >> 16))
            tileInfo[tileInfoSize++].set(x + 1, y, mip, count, ri_index);

          count2 = chist[1];
          if (uint32_t count = (count2 & 0xffff))
            tileInfo[tileInfoSize++].set(x + 2, y, mip, count, ri_index);
          if (uint32_t count = (count2 >> 16))
            tileInfo[tileInfoSize++].set(x + 3, y, mip, count, ri_index);

          count2 = chist[2];
          if (uint32_t count = (count2 & 0xffff))
            tileInfo[tileInfoSize++].set(x + 4, y, mip, count, ri_index);
          if (uint32_t count = (count2 >> 16))
            tileInfo[tileInfoSize++].set(x + 5, y, mip, count, ri_index);

          count2 = chist[3];
          if (uint32_t count = (count2 & 0xffff))
            tileInfo[tileInfoSize++].set(x + 6, y, mip, count, ri_index);
          if (uint32_t count = (count2 >> 16))
            tileInfo[tileInfoSize++].set(x + 7, y, mip, count, ri_index);
        }
      }
#elif _TARGET_64BIT
    uint64_t *__restrict pHist64 = (uint64_t *)pHist;
    for (uint32_t mip = 0; mip < texMips && tileInfoSize + 8 < tileInfo.size(); mip++)
      for (uint32_t ri_index = 0; ri_index < MAX_VTEX_CNT; ri_index++)
      {
        const uint32_t x_bits = TILE_WIDTH_BITS;
        const uint32_t x_mask = (1 << TILE_WIDTH_BITS) - 1;
        for (uint32_t i = 0; i < (TILE_WIDTH * TILE_WIDTH) && tileInfoSize + 8 < tileInfo.size(); i += 4, pHist64++)
        {
          if (uint64_t count2 = *pHist64)
          {
            // _mm_prefetch( (const char *) out, _MM_HINT_NTA);
            uint32_t y = (i >> x_bits) & x_mask;
            uint32_t x = i & x_mask;
            if (uint32_t count = (count2 & 0xffff))
              tileInfo[tileInfoSize++].set(x, y, mip, count, ri_index);
            if (uint32_t count = ((count2 >> 16) & 0xffff))
              tileInfo[tileInfoSize++].set(x + 1, y, mip, count, ri_index);
            if (uint32_t count = ((count2 >> 32) & 0xffff))
              tileInfo[tileInfoSize++].set(x + 2, y, mip, count, ri_index);
            if (uint32_t count = ((count2 >> 48) & 0xffff))
              tileInfo[tileInfoSize++].set(x + 3, y, mip, count, ri_index);
          }
        }
      }
#else
    uint32_t *__restrict pHist32 = (uint32_t *)pHist;
    for (uint32_t mip = 0; mip < texMips && tileInfoSize + 8 < tileInfo.size(); mip++)
      for (uint32_t ri_index = 0; ri_index < MAX_VTEX_CNT; ri_index++)
      {
        const uint32_t x_bits = TILE_WIDTH_BITS;
        const uint32_t x_mask = (1 << TILE_WIDTH_BITS) - 1;
        for (uint32_t i = 0; i < (TILE_WIDTH * TILE_WIDTH) && tileInfoSize + 8 < tileInfo.size(); i += 4, pHist32++)
        {
          if (uint32_t count2 = *pHist32)
          {
            // _mm_prefetch( (const char *) out, _MM_HINT_NTA);
            uint32_t y = (i >> x_bits) & x_mask;
            uint32_t x = i & x_mask;
            if (uint32_t count = (count2 & 0xffff))
              tileInfo[tileInfoSize++].set(x, y, mip, count, ri_index);
            if (uint32_t count = (count2 >> 16))
              tileInfo[tileInfoSize++].set(x + 1, y, mip, count, ri_index);
          }
        }
      }
#endif
  }
  // G_ASSERT(tileInfoSize <= tileInfo.size());
  // for (int i = 0; i < tileInfoSize; ++i)
  //{
  //   G_ASSERTF(tileInfo[i].x < (TILE_WIDTH), "mip=%d x = %d y = %d", tileInfo[i].mip, tileInfo[i].x, tileInfo[i].y);
  //   G_ASSERTF(tileInfo[i].y < (TILE_WIDTH), "mip=%d x = %d y = %d", tileInfo[i].mip, tileInfo[i].x, tileInfo[i].y);
  // }

  // debug("vx = %i, vy = %i", vx, vy );

  // this ugly hack should only be applied
  // when we know we need to apply it
}

void ClipmapImpl::processTileFeedback(TileInfoArr &result_arr, int &result_size, char *feedback_tile_ptr, int feedback_tile_cnt,
  const TMatrix4 &globtm, const Point4 &uv2l)
{
  TempMap<uint32_t, TexTileInfo, MAX_FAKE_TILE_CNT> aroundCameraTileIndexMap;
  if (!disable_fake_tiles && currentContext->tileZoom <= MAX_ZOOM_TO_ALLOW_FAKE_AROUND_CAMERA)
  {
    auto funct = [&aroundCameraTileIndexMap](int x, int y, int mip, int ri_index, int hist_ofs) {
      TexTileInfo tile = TexTileInfo(x, y, mip, MORE_THAN_FEEDBACK, ri_index);
      aroundCameraTileIndexMap.insert(eastl::pair(hist_ofs, tile));
    };
    for (int riIndex = 0; riIndex < MAX_VTEX_CNT; ++riIndex) // -V1008
    {
      float dist_to_ground;
      Point3 camPos = getMappedRelativeRiPos(riIndex, viewerPosition, dist_to_ground);
      processTilesAroundCamera(texMips, globtm, riIndex == 0 ? uv2l : getUV2landacape(riIndex), getLandscape2uv(riIndex), //-V547
        camPos, getLandscape2uv(TERRAIN_RI_INDEX), riIndex, dist_to_ground, funct);
    }
  }

  if (disable_feedback)
    feedback_tile_cnt = 0;

  {
    if (feedback_tile_cnt > result_arr.size())
    {
      logerr("feedback_tile_cnt is out of bounds: %d > %d", feedback_tile_cnt, result_arr.size());
      feedback_tile_cnt = 0;
    }
    const PackedTile *feedbackPtr = (const PackedTile *)feedback_tile_ptr;
    result_size = 0;
    for (int i = 0; i < feedback_tile_cnt; ++i)
    {
      uint32_t ri_index;
      uint4 fb = unpackTileInfo(feedbackPtr[i], ri_index);

      TexTileInfo tile(fb.x, fb.y, fb.z, fb.w, ri_index);
      uint32_t tag = TagOfTile(tile.mip, tile.x, tile.y, ri_index);

      auto it = aroundCameraTileIndexMap.find(tag);
      if (it != aroundCameraTileIndexMap.end())
      {
        TexTileInfo &taggedTileRef = it->second;
        taggedTileRef.count = min(tile.count, MORE_THAN_FEEDBACK_INV) + MORE_THAN_FEEDBACK;
      }
      else
      {
        result_arr[result_size++] = tile;
      }
    }
  }

  for (const auto &pair : aroundCameraTileIndexMap)
    result_arr[result_size++] = pair.second;

  sort_tile_info_list(result_arr, result_size);
}


enum
{
  MAX_USED_TILES = 8,
  MAX_WITH_USED_TILES = 16,
  MAX_UNUSED_TILES = 1,
  MAX_WITH_UNUSED_TILES = 4
};
enum
{
  PRIO_INVALID = 0,
  PRIO_VISIBLE = 1,
  PRIO_INVISIBLE = 2,
  PRIO_COUNT
};

static const int max_tiles_count[PRIO_COUNT] = {TEX_ZOOM_LIMITER, MAX_USED_TILES, MAX_UNUSED_TILES};
static const int max_total_tiles_count[PRIO_COUNT] = {TEX_ZOOM_LIMITER, MAX_WITH_USED_TILES, MAX_WITH_UNUSED_TILES};

void ClipmapImpl::updateCache(bool force_update)
{
  TIME_D3D_PROFILE(updateCache);

  Driver3dRenderTarget prevRt;
  bool updateBegan = false;

  TexLRUList &LRU = currentContext->LRU;
  int maxTimeToSpendInUpdateUsec = max<int>(0, ::dagor_game_act_time * FRAME_FRACTION_TO_SPEND_IN_UPDATE * 1e6);
  int64_t reft = ref_time_ticks();
  carray<StaticTab<uint16_t, TEX_ZOOM_LIMITER>, PRIO_COUNT> updateLRU;
  carray<StaticTab<uint16_t, TEX_ZOOM_LIMITER>, TEX_MIPS> updateLRUInvalid;

  for (int i = 0; i < LRU.size() && updateLRU[0].size() < TEX_ZOOM_LIMITER; ++i)
  {
    TexLRU &ti = LRU[i];

#if MAX_RI_VTEX_CNT_BITS > 0
    // invalidate the recently changed RI-based tiles which are not used (fake tiles around camera)
    if (ti.ri_index != TERRAIN_RI_INDEX && ti.ri_index < MAX_VTEX_CNT && changedRiIndices[ti.ri_index - 1] &&
        !(ti.flags & TexLRU::IS_USED))
      ti.flags |= TexLRU::NEED_UPDATE | TexLRU::IS_INVALID;
    else
#endif
      if (!(ti.flags & TexLRU::NEED_UPDATE))
      continue;

    if (ti.flags & TexLRU::IS_INVALID)
    {
      if (updateLRUInvalid[ti.mip].size() < max_tiles_count[0])
        updateLRUInvalid[ti.mip].push_back(i);
    }
    else
    {
      int index = (ti.flags & TexLRU::IS_USED) ? PRIO_VISIBLE : PRIO_INVISIBLE;
      if (updateLRU[index].size() < max_tiles_count[index])
        updateLRU[index].push_back(i);
    }
  }

#if MAX_RI_VTEX_CNT_BITS > 0
  for (int i = 0; i < changedRiIndices.size(); ++i)
    changedRiIndices[i] = max<int>(changedRiIndices[i] - 1, 0);
#endif

  for (int i = TEX_MIPS - 1; i >= 0 && updateLRU[PRIO_INVALID].size() < max_total_tiles_count[PRIO_INVALID]; --i) // can be also sort
                                                                                                                  // with count
  {
    if (updateLRUInvalid[i].size())
    {
      append_items(updateLRU[PRIO_INVALID],
        min(updateLRUInvalid[i].size(), max_total_tiles_count[PRIO_INVALID] - updateLRU[PRIO_INVALID].size()),
        &updateLRUInvalid[i][0]);
    }
  }

  if (updateLRU[PRIO_INVALID].size()) // if we have invalid tile block, than indirection has changed, because it was not rendered into
                                      // it
    indirectionChanged = true;
  G_STATIC_ASSERT(PRIO_INVALID == 0);
  for (int prio = PRIO_VISIBLE; prio < PRIO_COUNT && updateLRU[PRIO_INVALID].size() < max_total_tiles_count[PRIO_INVALID]; ++prio)
  {
    if (updateLRU[prio].size() && max_total_tiles_count[prio] > updateLRU[PRIO_INVALID].size()) // check before to send num>0 to append
      append_items(updateLRU[PRIO_INVALID], min(updateLRU[prio].size(), max_total_tiles_count[prio] - updateLRU[PRIO_INVALID].size()),
        &updateLRU[prio][0]);
  }

  if (!updateLRU[PRIO_INVALID].size())
    return;

  {
    TIME_D3D_PROFILE(updateTiles);
    int limitByCount = maxTileUpdateCount;

    int tilesUpdated = 0;
    for (int i = 0; i < updateLRU[0].size(); ++i)
    {
      TexLRU &ti = LRU[updateLRU[0][i]];
      G_ASSERT(ti.flags & TexLRU::NEED_UPDATE);
      if (!updateBegan)
      {
        d3d::get_render_target(prevRt);
        beginUpdateTiles();
        updateBegan = true;
      }
      ti.flags &= ~(TexLRU::NEED_UPDATE | TexLRU::IS_INVALID);
      updateTileBlock(ti.mip, ti.x, ti.y, ti.tx, ti.ty, ti.ri_index);
      tilesUpdated++;

      if (tilesUpdated > limitByCount && !force_update)
      {
        break;
      }

      if ((tilesUpdated & 3) == 0 && !force_update) // check each fourth tile
      {
        int timePassed = get_time_usec(reft);
        if (timePassed > maxTimeToSpendInUpdateUsec)
          break;
      }
    }
  }
  if (updateBegan)
  {
    TIME_D3D_PROFILE(finalizeUpdate);
    finalizeCompressionQueue();
    endUpdateTiles();
    d3d::set_render_target(prevRt);
  }
}

void ClipmapImpl::updateMip(HWFeedbackMode hw_feedback_mode, bool force_update)
{
  TIME_PROFILE_DEV(clipmap_updateMip);

  if (feedbackType != SOFTWARE_FEEDBACK) // no reasonable count
  {
    sort_tile_info_list(tileInfo, tileInfoSize);

    int firstMismatchId;
    if (hw_feedback_mode == HWFeedbackMode::DEBUG_COMPARE &&
        getHWFeedbackMode(currentContext->captureTarget % MAX_FRAMES_AHEAD) == HWFeedbackMode::DEBUG_COMPARE &&
        !compare_tile_info_results(tileInfo, tileInfoSize, debugTileInfo, debugTileInfoSize, firstMismatchId))
    {
      if (firstMismatchId >= 0)
      {
        const TexTileInfo &a = tileInfo[firstMismatchId];      // tile feedback
        const TexTileInfo &b = debugTileInfo[firstMismatchId]; // texture feedback
        logerr("clipmap tile feedback mismatch (%d ; %d), {%d,%d, %d,%d,%d, %d} != {%d,%d, %d,%d,%d, %d}", tileInfoSize,
          debugTileInfoSize, a.x, a.y, a.ri_index, a.mip, a.count, a.sortOrder, b.x, b.y, b.ri_index, b.mip, b.count, b.sortOrder);
      }
      else
      {
        logerr("clipmap tile feedback mismatch (%d ; %d)", tileInfoSize, debugTileInfoSize);
      }
    }
  }

  int lruDimX = cacheDimX / texTileSize;
  int lruDimY = cacheDimY / texTileSize;
  int limit = lruDimX * lruDimY - currentContext->fallback.pagesDim * currentContext->fallback.pagesDim; // / TEX_MIPS;
  int newSize = min(tileInfoSize, limit);

#if DAGOR_DBGLEVEL > 0
  if (false)
  {
    TexLRUList &LRU = currentContext->LRU;
    MipContext::InCacheBitmap inCacheBitmapCheck;
    for (int i = 0; i < LRU.size(); ++i)
    {
      const TexLRU &ti = LRU[i];
      if (ti.mip < texMips)
      {
        // inCache.insert(IndexOfTile(ti.mip,ti.tx,ti.ty));
        uint32_t addr = IndexOfTileInBit(ti.mip, ti.tx, ti.ty, ti.ri_index);
        G_ASSERT(currentContext->inCacheBitmap[addr]);
        inCacheBitmapCheck[addr] = true;
      }
    }
    G_ASSERT(currentContext->inCacheBitmap == inCacheBitmapCheck);
  }
#endif
  G_ASSERT(limit <= MAX_CACHE_TILES);

  // fixme: try with bitarray
  TempSet<uint32_t, 256> inTag;
  TempSet<uint32_t, 256> fakeTag;
  bool hide_invisible_tiles = feedbackType != SOFTWARE_FEEDBACK;
  for (int fbi = 0; fbi < newSize; ++fbi)
  {
    TexTileInfo &fb = tileInfo[fbi];
    uint32_t addr = IndexOfTileInBit(fb.mip, fb.x, fb.y, fb.ri_index);
    if (currentContext->inCacheBitmap[addr])
    {
      inTag.insert(TagOfTile(fb.mip, fb.x, fb.y, fb.ri_index));
      if (hide_invisible_tiles)
      {
        if (fb.count == MORE_THAN_FEEDBACK)
          fakeTag.insert(TagOfTile(fb.mip, fb.x, fb.y, fb.ri_index));
      }
    }
  }

  carray<TexLRU, MAX_CACHE_TILES> unusedLRU;
  int unusedLRUCount = 0;
  if (true)
  {
    TexLRUList &LRU = currentContext->LRU;
    carray<TexLRU, MAX_CACHE_TILES> usedLRU;
    int usedLRUcount = 0;
    for (int i = 0; i < LRU.size(); ++i)
    {
      TexLRU &ti = LRU[i];
      uint32_t index;
      if (ti.mip >= texMips || inTag.find(index = TagOfTile(ti.mip, ti.tx, ti.ty, ti.ri_index)) == inTag.end())
      {
        ti.flags &= ~ti.IS_USED;
        unusedLRU[unusedLRUCount++] = ti;
      }
      else
      {
        if (fakeTag.find(index) == fakeTag.end())
          ti.flags |= ti.IS_USED;
        else
          ti.flags &= ~ti.IS_USED; // it is actually NOT used, just added around
        usedLRU[usedLRUcount++] = ti;
      }
    }
    LRU.resize(usedLRUcount);
    memcpy(LRU.data(), usedLRU.data(), data_size(LRU));
  }
  int unusedLRUutilized = unusedLRUCount - 1;

  TexLRUList &LRU = currentContext->LRU;
  changedLRUscount = 0;
  carray<TexLRU, MAX_CACHE_TILES> newLRUchanged;
  int newLRUchangedCount = 0;
  for (int fbi = 0; fbi < newSize; ++fbi)
  {
    TexTileInfo &fb = tileInfo[fbi];
    uint32_t addr = IndexOfTileInBit(fb.mip, fb.x, fb.y, fb.ri_index);
    if (currentContext->inCacheBitmap[addr])
      continue;

    int mip = fb.mip;
    int fbx = fb.x;
    int fby = fb.y;
    int ri_index = fb.ri_index;

    // mark all the old ones as void
    // TexLRU ti = *it;
    G_ASSERTF_CONTINUE(unusedLRUutilized >= 0, "unusedLRUutilized=%d >= 0, unusedLRUCount=%d limit = %d, lru = %d, newSize = %d",
      unusedLRUutilized, unusedLRUCount, limit, LRU.size(), newSize);
    const TexLRU &ti = unusedLRU[unusedLRUutilized];
    unusedLRUutilized--;
    // LRU.erase(it);
    if (ti.mip < texMips)
    {
      changedLRUs[changedLRUscount] = ti;
      changedLRUs[changedLRUscount].setFlags(0);
      changedLRUscount++;
      uint32_t addr2 = IndexOfTileInBit(ti.mip, ti.tx, ti.ty, ti.ri_index);
      G_ASSERT(currentContext->inCacheBitmap[addr2]);
      currentContext->inCacheBitmap[addr2] = false;
    }
    TexLRU newLru(ti.x, ti.y, fbx, fby, mip, ri_index);
    newLru.setFlags(ti.IS_USED | ti.NEED_UPDATE | ti.IS_INVALID);
    newLRUchanged[newLRUchangedCount++] = newLru;
    LRU.push_back(newLru);
    indirectionChanged = true;
    currentContext->inCacheBitmap[addr] = true;

    tileLimit -= (mip + 1); // linear weight (1-2-3-4...)
    if (tileLimit <= 0 && !force_update)
      break;
  }
  G_ASSERT(changedLRUscount + newLRUchangedCount <= changedLRUs.size());
  memcpy(&changedLRUs[changedLRUscount], newLRUchanged.data(), newLRUchangedCount * elem_size(changedLRUs));
  changedLRUscount += newLRUchangedCount;
  append_items(LRU, unusedLRUutilized + 1, unusedLRU.data());
}

void ClipmapImpl::setFeedbackType(uint32_t ftp)
{
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
  swFeedbackInnerMipTiles = inner_tiles;
  swFeedbackOuterMipTiles = outer_tiles;
}
void ClipmapImpl::setSoftwareFeedbackMipTiles(int mip, int tiles_for_mip)
{
  if (mip >= 0 && mip < TEX_MIPS)
  {
    swFeedbackTilesPerMip[mip] = tiles_for_mip;
  }
  else
  {
    logerr("ClipmapImpl::setSoftwareFeedbackMipTiles: mip %d is out of bounds", mip);
  }
}

static bool checkUAVSupport() { return d3d::get_driver_desc().shaderModel >= 5.0_sm; }

bool ClipmapImpl::canUseUAVFeedback() const { return checkUAVSupport() && use_uav_feedback; }

bool MipContext::canUseUAVFeedback() const { return checkUAVSupport() && use_uav_feedback; }

void MipContext::initHWFeedback()
{
  closeHWFeedback();

  captureTarget = 0;
  oldestCaptureTargetIdx = 0;

  for (int i = 0; i < MAX_FRAMES_AHEAD; ++i)
  {
    captureTexFence[i].reset(d3d::create_event_query());
    String captureTexName(128, "capture_tex%p_%d", this, i);
    uint32_t uavFlag = canUseUAVFeedback() ? (TEXCF_UNORDERED | TEXFMT_R8G8B8A8) : (TEXCF_RTARGET | TEXFMT_A8R8G8B8);

    captureTex[i] = dag::create_tex(nullptr, FEEDBACK_WIDTH, FEEDBACK_HEIGHT,
      TEXCF_CPU_CACHED_MEMORY | TEXCF_LINEAR_LAYOUT | // PS4
        uavFlag,
      1, captureTexName.str());
    d3d_err(captureTex[i].getTex2D());

    String tileInfoName(128, "clipmap_tile_info_buf%p_%d", this, i);
    static constexpr uint32_t subElementCnt = sizeof(ClipmapImpl::PackedTile) / d3d::buffers::BYTE_ADDRESS_ELEMENT_SIZE;
    G_STATIC_ASSERT(subElementCnt > 0);
    G_STATIC_ASSERT(sizeof(ClipmapImpl::PackedTile) % sizeof(uint32_t) == 0);
    captureTileInfoBuf[i] = dag::buffers::create_ua_byte_address_readback(
      subElementCnt * (ClipmapImpl::MAX_TILE_CAPTURE_INFO_BUF_ELEMENTS + 1u), tileInfoName.str());
    d3d_err(captureTileInfoBuf[i].getBuf());
  }
  intermediateHistBuf = dag::buffers::create_ua_sr_byte_address(TEX_TOTAL_ELEMENTS, "clipmap_histogram_buf");
  d3d_err(intermediateHistBuf.getBuf());
}

void MipContext::closeHWFeedback()
{
  for (int i = 0; i < MAX_FRAMES_AHEAD; ++i)
  {
    captureTex[i].close();
    captureTileInfoBuf[i].close();
    captureTexFence[i].reset();
  }
  intermediateHistBuf.close();
}


void ClipmapImpl::initHWFeedback()
{
  String texName(30, "feedback_depth_tex_%p", this);
  feedbackDepthTex =
    dag::create_tex(nullptr, FEEDBACK_WIDTH, FEEDBACK_HEIGHT, TEXFMT_DEPTH32 | TEXCF_RTARGET | TEXCF_TC_COMPATIBLE, 1, texName);
  d3d_err(feedbackDepthTex.getTex2D());
  if (currentContext)
    currentContext->initHWFeedback();
}

void ClipmapImpl::closeHWFeedback()
{
  if (currentContext)
    currentContext->closeHWFeedback();
  feedbackDepthTex.close();
}

void ClipmapImpl::setTexMips(int tex_mips)
{
  if (tex_mips == texMips)
    return;
  G_ASSERTF(tex_mips > 3 && tex_mips <= TEX_MIPS, "tex_mips is %d, and should be 4..%d", tex_mips, TEX_MIPS);
  texMips = clamp(tex_mips, (int)4, (int)TEX_MIPS);
  alignMips = min<int>(get_log2w(TILE_WIDTH), texMips);
  debug("tex_mips now = %d, align = %d", tex_mips, alignMips);
  mostLeft = ((-TILE_WIDTH / 2) << (texMips - 1)) + TILE_WIDTH / 2;
  mostRight = ((TILE_WIDTH / 2) << (texMips - 1)) + TILE_WIDTH / 2;
  String varName;
  varName.printf(0, "clipmapTexMips%s", postfix);
  ShaderGlobal::set_int(get_shader_variable_id(varName.str(), true), texMips);
  if (currentContext)
  {
    currentContext->closeIndirection();
    currentContext->initIndirection(texMips);

    invalidate(true);
  }
}

void ClipmapImpl::setTileSizeVars()
{
  texTileInnerSize = texTileSize - texTileBorder * 2;
  ShaderGlobal::set_color4(vTexDimVarId, Color4(TILE_WIDTH * texTileInnerSize, TILE_WIDTH * texTileInnerSize, 0, 0));
  ShaderGlobal::set_color4(tileSizeVarId,
    Color4(texTileInnerSize, 1.0f / texTileInnerSize, texTileSize,
      get_log2w(clamp(cacheAnisotropy, minTexTileBorder, canUseUAVFeedback() ? (int)16 : (int)8) * 2.f)));
  ShaderGlobal::set_color4(tileSizeVarId, Color4(texTileInnerSize, 1.0f / texTileInnerSize, texTileSize,
                                            get_log2w(clamp(cacheAnisotropy, minTexTileBorder, (int)16) * 2.f)));
  float brd = texTileBorder;
  float cpx = 1.0f / cacheDimX;
  float cpy = 1.0f / cacheDimY;
  ShaderGlobal::set_color4(g_cache2uvVarId, Color4(cpx, cpy, (brd + HALF_TEXEL_OFSF) * cpx, (brd + HALF_TEXEL_OFSF) * cpy));
}

void ClipmapImpl::initVirtualTexture(int argCacheDimX, int argCacheDimY)
{
  if (!is_uav_supported())
    use_uav_feedback = false;
  debug("use_uav_feedback=%d", (int)use_uav_feedback);

  argCacheDimX = min(argCacheDimX, d3d::get_driver_desc().maxtexw);
  argCacheDimY = min(argCacheDimY, d3d::get_driver_desc().maxtexh);

  if (cacheDimX == argCacheDimX && cacheDimY == argCacheDimY) // already inited
    return;
  closeVirtualTexture();

  cacheDimX = argCacheDimX;
  cacheDimY = argCacheDimY;
  tileInfoSize = 0;
  indirectionChanged = false;

  tileLimit = 0;
  maxTileLimit = TEX_DEFAULT_LIMITER;
  zoomTileLimit = 0;

  G_ASSERTF(((cacheDimX % texTileSize) == 0), "Cache X size must be evenly divisible by TileSize");
  G_ASSERTF(((cacheDimY % texTileSize) == 0), "Cache Y size must be evenly divisible by TileSize");
  G_ASSERT(cacheDimX * cacheDimY / (texTileSize * texTileSize) <= MAX_CACHE_TILES);

  float wp = 1.0f / (texMips * TILE_WIDTH_F);
  float hp = 1.0f / (TILE_WIDTH_F);

  String varName;
  varName.printf(0, "fallback_info0%s", postfix);
  fallback_info0VarId = get_shader_variable_id(varName.str(), true);
  varName.printf(0, "fallback_info1%s", postfix);
  fallback_info1VarId = get_shader_variable_id(varName.str(), true);
  supports_uavVarId = get_shader_variable_id("supports_uav", true);
  varName.printf(0, "g_cache2uv%s", postfix);
  g_cache2uvVarId = ::get_shader_glob_var_id(varName.str());

  varName.printf(0, "feedback_landscape2uv%s", postfix);
  feedback_landscape2uvVarId = ::get_shader_glob_var_id(varName.str(), true);

  varName.printf(0, "landscape2uv%s", postfix);
  landscape2uvVarId = ::get_shader_glob_var_id(varName.str(), true);

  varName.printf(0, "world_dd_scale%s", postfix);
  worldDDScaleVarId = ::get_shader_glob_var_id(varName.str(), true);

  varName.printf(0, "g_VTexDim%s", postfix);
  vTexDimVarId = ::get_shader_glob_var_id(varName.str(), true);
  varName.printf(0, "g_TileSize%s", postfix);
  tileSizeVarId = ::get_shader_glob_var_id(varName.str(), true);

  varName.printf(0, "indirection_tex%s", postfix);
  indirectionTexVarId = ::get_shader_glob_var_id(varName.str(), true);
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
  constantFillerVBuf->init(maxQuads * TEX_MIPS * 4 * 2, sizeof(E3DCOLOR) * 2); // allow up to 4 frames ahead with no lock
  constantFillerBuf->setRingBuf(constantFillerVBuf);
  constantFillerBuf->init(make_span_const(channels, 2), maxQuads); // allow up to 4 frames ahead with no lock
  constantFillerBuf->setCurrentShader(constantFillerElement);

  currentContext.reset(new MipContext());
  currentContext->init(use_uav_feedback);
  currentContext->xOffset = currentContext->yOffset = 0;
  currentContext->tileZoom = 1;
  currentContext->initIndirection(texMips);
  currentContext->reset(cacheDimX, cacheDimY, true, texTileSize, feedbackType);

  if (feedbackType == CPU_HW_FEEDBACK)
    initHWFeedback();

  clear_and_resize(lockedPixels, FEEDBACK_HEIGHT * FEEDBACK_WIDTH);
  clear_and_resize(lockedTileInfo, ClipmapImpl::MAX_TILE_CAPTURE_INFO_BUF_ELEMENTS);

  setTileSizeVars();
}

void MipContext::initIndirection(int tex_mips)
{
  texMips = tex_mips;
  String ind_tex(30, "ind_tex_%p", this);
  unsigned indFlags = TEXFMT_A8R8G8B8 | TEXCF_RTARGET;

  indirection = dag::create_tex(nullptr, TILE_WIDTH * texMips * MAX_VTEX_CNT, TILE_WIDTH, indFlags, 1, ind_tex);
  indirection.getTex2D()->texaddr(TEXADDR_BORDER);
  indirection.getTex2D()->texbordercolor(0xFFFFFFFF);
  indirection.getTex2D()->texfilter(TEXFILTER_POINT);
}

void MipContext::closeIndirection() { indirection.close(); }

static class PrepareFeedbackJob final : public cpujobs::IJob
{
public:
  ClipmapImpl *clipmap = 0;
  int oldXOffset = 0, oldYOffset = 0, oldZoom = -1;
  Point4 uv2l;
  TMatrix4 globtm;
  bool forceUpdate;
  HWFeedbackMode hwFeedbackMode = HWFeedbackMode::INVALID;

  enum class FeedbackType
  {
    UPDATE_FROM_READPIXELS,
    UPDATE_MIP,
    UPDATE_INVALID,
  } updateFeedbackType;

  PrepareFeedbackJob()
  {
    updateFeedbackType = FeedbackType::UPDATE_INVALID;
    forceUpdate = false;
  }
  ~PrepareFeedbackJob() {}
  void initFromHWFeedback(HWFeedbackMode hw_feedback_mode, const TMatrix4 &gtm, ClipmapImpl &clip_, int oldxo, int oldyo, int oldz,
    const Point4 &olduv2l, bool force_update)
  {
    hwFeedbackMode = hw_feedback_mode;
    globtm = gtm;
    clipmap = &clip_;
    oldXOffset = oldxo;
    oldYOffset = oldyo;
    oldZoom = oldz;
    uv2l = olduv2l;
    updateFeedbackType = FeedbackType::UPDATE_FROM_READPIXELS;
    forceUpdate = force_update;
  }
  void initFromSoftware(ClipmapImpl &clip_, bool force_update)
  {
    clipmap = &clip_;
    hwFeedbackMode = HWFeedbackMode::INVALID;
    updateFeedbackType = FeedbackType::UPDATE_MIP;
    forceUpdate = force_update;
  }

  virtual void doJob() override
  {
    if (updateFeedbackType == FeedbackType::UPDATE_FROM_READPIXELS)
    {
      G_ASSERT(clipmap->lockedTileInfoCnt <= ClipmapImpl::MAX_TILE_CAPTURE_INFO_BUF_ELEMENTS); // should never happen
      clipmap->updateFromFeedback(hwFeedbackMode, globtm, (char *)clipmap->lockedPixels.data(), (char *)clipmap->lockedTileInfo.data(),
        (int)clipmap->lockedTileInfoCnt, oldXOffset, oldYOffset, oldZoom, uv2l, forceUpdate);
    }
    else if (updateFeedbackType == FeedbackType::UPDATE_MIP)
    {
      clipmap->updateMip(hwFeedbackMode, forceUpdate);
    }
  }
} feedbackJob;

void ClipmapImpl::update(bool force_update)
{
  updateCache(force_update);
  checkConsistency();
  if (indirectionChanged || currentContext->invalidateIndirection)
    GPUrestoreIndirectionFromLRU();
  changedLRUscount = 0;
}


void ClipmapImpl::closeVirtualTexture()
{
  threadpool::wait(&feedbackJob);
  ShaderGlobal::set_texture(indirectionTexVarId, BAD_TEXTUREID);

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

  currentContext.reset();
}

bool ClipmapImpl::changeZoom(int newZoom, const Point3 &position)
{
  if (currentContext->tileZoom == newZoom)
    return false;
  newZoom = max(newZoom, 1);
  newZoom = min(newZoom, int(TEX_MAX_ZOOM));
  if (pixelRatio > 0)
  {
    int max_zoom = 1.75 * maxTexelSize / ((1 << (texMips - 1)) * pixelRatio);
    newZoom = max(min(newZoom, get_bigger_pow2(max_zoom)), 1);
  }

  if (currentContext->tileZoom == newZoom)
    return false;

  int oldZoom = currentContext->tileZoom;
  int oldXOffset = currentContext->xOffset;
  int oldYOffset = currentContext->yOffset;

  currentContext->tileZoom = newZoom;
  int newXOffset, newYOffset;
  centerOffset(position, newXOffset, newYOffset, currentContext->tileZoom);
  // debug("centerOffset = %gx%g pr=%g(%g) %dx%d",P2D(Point2::xz(position)), getPixelRatio(), pixelRatio, newXOffset, newYOffset);
  newXOffset -= (TILE_WIDTH / 2);
  newYOffset -= (TILE_WIDTH / 2);
  currentContext->xOffset = newXOffset;
  currentContext->yOffset = newYOffset;
  // debug("chzoom %d->%d ofs = %dx%d -> %dx%d", oldZoom, newZoom, oldXOffset, oldYOffset, newXOffset, newYOffset);

  /*
   l2uv = 1 / Zoom
   uv2l = Zoom
   */

  int dzoom = newZoom > oldZoom ? -1 : 1;

  carray<TexLRU, MAX_CACHE_TILES> newLRU;
  int newLRUUsed = 0;
  TexLRUList &LRU = currentContext->LRU;
  currentContext->resetInCacheBitmap();
  indirectionChanged = true;
  currentContext->invalidateIndirection = true;
  for (int i = 0; i < LRU.size(); ++i)
  {
    TexLRU &ti = LRU[i];
    bool discard = true;
    if (ti.mip < texMips)
    {
      if (ti.ri_index != TERRAIN_RI_INDEX)
      {
        // RI indirection has fixed zoom
        discard = false;
        newLRU[newLRUUsed++] = ti;
        currentContext->inCacheBitmap[IndexOfTileInBit(ti.mip, ti.tx, ti.ty, ti.ri_index)] = true;
      }
      else
      {
        int newMip = ti.mip + dzoom;
        if (newMip >= 0 && newMip < texMips)
        {
          int gx = ((((int)ti.tx - (TILE_WIDTH / 2)) << ti.mip) + (TILE_WIDTH / 2) + oldXOffset) * oldZoom;
          int gy = ((((int)ti.ty - (TILE_WIDTH / 2)) << ti.mip) + (TILE_WIDTH / 2) + oldYOffset) * oldZoom;
          int nx = (((gx / newZoom) - newXOffset - (TILE_WIDTH / 2)) >> newMip) + (TILE_WIDTH / 2);
          int ny = (((gy / newZoom) - newYOffset - (TILE_WIDTH / 2)) >> newMip) + (TILE_WIDTH / 2);

          if (nx >= 0 && nx < TILE_WIDTH && ny >= 0 && ny < TILE_WIDTH)
          {
            int GGX = (((nx - (TILE_WIDTH / 2)) << newMip) + newXOffset + (TILE_WIDTH / 2)) * newZoom;
            int GGY = (((ny - (TILE_WIDTH / 2)) << newMip) + newYOffset + (TILE_WIDTH / 2)) * newZoom;
            if (GGX == gx && GGY == gy)
            {
              discard = false;
              uint32_t addr = IndexOfTileInBit(newMip, nx, ny, ti.ri_index);
              currentContext->inCacheBitmap[addr] = true;
              newLRU[newLRUUsed++] = TexLRU(ti.x, ti.y, nx, ny, newMip, ti.ri_index).setFlags(ti.flags);
            }
          }
        }
      }
    }
    if (discard)
      newLRU[newLRUUsed++] = TexLRU(ti.x, ti.y);
  }
  G_ASSERT(newLRUUsed == LRU.size());
  LRU.resize(newLRUUsed);
  memcpy(LRU.data(), newLRU.data(), data_size(LRU));
  return true;
}

bool ClipmapImpl::changeXYOffset(int nx, int ny)
{
  if (currentContext->xOffset == nx && currentContext->yOffset == ny)
    return false;
  // debug("changeXYOffset = %dx%d -> %dx%d", currentContext->xOffset, currentContext->yOffset, nx, ny);
  carray<TexLRU, MAX_CACHE_TILES> newLRU;
  int newLRUUsed = 0;
  TexLRUList &LRU = currentContext->LRU;
  currentContext->resetInCacheBitmap();
  indirectionChanged = true;
  currentContext->invalidateIndirection = true;
  for (int i = 0; i < LRU.size(); ++i)
  {
    TexLRU &ti = LRU[i];
    if (ti.mip < alignMips)
    {
      if (ti.ri_index != TERRAIN_RI_INDEX)
      {
        // RI indirection has fixed offset (the virtual camera is always at the UV center)
        newLRU[newLRUUsed++] = ti;
        currentContext->inCacheBitmap[IndexOfTileInBit(ti.mip, ti.tx, ti.ty, ti.ri_index)] = true;
      }
      else
      {
        IPoint2 diff = IPoint2(currentContext->xOffset - nx, currentContext->yOffset - ny);
        int newX = (int)ti.tx + (diff.x >> ti.mip);
        int newY = (int)ti.ty + (diff.y >> ti.mip);
        if (newX >= 0 && newX < TILE_WIDTH && newY >= 0 && newY < TILE_WIDTH)
        {
          newLRU[newLRUUsed++] = TexLRU(ti.x, ti.y, newX, newY, ti.mip, ti.ri_index).setFlags(ti.flags);
          uint32_t addr = IndexOfTileInBit(ti.mip, newX, newY, ti.ri_index);
          currentContext->inCacheBitmap[addr] = true;
        }
        else
          newLRU[newLRUUsed++] = TexLRU(ti.x, ti.y);
      }
    }
    else
      newLRU[newLRUUsed++] = TexLRU(ti.x, ti.y);
  }
  G_ASSERT(newLRUUsed == LRU.size());
  LRU.resize(newLRUUsed);
  memcpy(LRU.data(), newLRU.data(), data_size(LRU));
  currentContext->xOffset = nx;
  currentContext->yOffset = ny;
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
        LRU.push_back(TexLRU(x, y));
    return false;
  }
  bool changed = false;
  for (auto it = LRU.begin(), end = LRU.end(); it != end;)
  {
    if (it->x >= ndim || it->y >= ndim)
    {
      ++it;
      continue;
    }

    // remove it if it used
    if (it->mip < texMips)
    {
      uint32_t addr = IndexOfTileInBit(it->mip, it->tx, it->ty, it->ri_index);
      inCacheBitmap[addr] = false;
      changed = true;
    }

    eastl::swap(*it, *--end); // fast erase
    LRU.pop_back();
  }

  invalidateIndirection |= changed;
  return changed;
}

void ClipmapImpl::initFallbackPage(int pages_dim, float texel_size)
{
  if (!VariableMap::isGlobVariablePresent(fallback_info0VarId) || !VariableMap::isGlobVariablePresent(fallback_info1VarId))
    return;
  fallbackPages = clamp(pages_dim, (int)0, (int)4);
  fallbackTexelSize = clamp(texel_size, 0.0001f, 1e4f);
}

void ClipmapImpl::changeFallback(const Point3 &viewer, bool turn_off_decals_on_fallback)
{
  if (currentContext->fallback.pagesDim == fallbackPages && fallbackPages == 0)
    return;
  static constexpr int BLOCK_SZ = 8; // 4 texel align is minimum (block size) alignment, we keep aligned better than that for
                                     // supersampling if ever
  const IPoint2 newOrigin = ipoint2(floor(Point2::xz(viewer) / (BLOCK_SZ * fallbackTexelSize) + Point2(0.5, 0.5))) * BLOCK_SZ;
  if (!currentContext->fallback.sameSettings(fallbackPages, fallbackTexelSize))
  {
    currentContext->fallback.texelSize = fallbackTexelSize;
    currentContext->fallback.originXZ = newOrigin + IPoint2(-10000, 10000);
  }
  indirectionChanged |= currentContext->allocateFallback(fallbackPages, texTileSize);

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
    beginUpdateTiles();
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
        updateTileBlockRaw(x, y, BBox2(pageLt, pageLt + Point2(pageMeters, pageMeters)));
      }
    finalizeCompressionQueue();
    endUpdateTiles();
    LandMeshRenderer::lmesh_render_flags = oldFlags;
    viewerPosition = saveViewerPos;
  }
}

void ClipmapImpl::restoreIndirectionFromLRU(SmallTab<TexTileIndirection, MidmemAlloc> *tiles) // debug
{
  // TIME_PROFILE(restoreIndirectionFromLRU);
  for (int mip = 0; mip < texMips; ++mip)
  {
    clear_and_resize(tiles[mip], TILE_WIDTH * TILE_WIDTH);
    mem_set_ff(tiles[mip]);
  }
  TexLRUList &LRU = currentContext->LRU;
  for (int i = 0; i < LRU.size(); ++i)
  {
    TexLRU &ti = LRU[i];
    // if (!(ti.flags&ti.IS_USED))//uncomment this if you want to use only really used in frame LRUs
    //   continue;
    if (ti.mip >= texMips || (ti.flags & ti.IS_INVALID))
      continue;
    TexTileIndirection stti(ti.x, ti.y, ti.mip, 0xff);
    int addr = ti.ty * TILE_WIDTH + ti.tx;
    int lruBlock = 1 << ti.mip;
    int stride_offs = TILE_WIDTH - lruBlock;
    for (int by = 0; by < lruBlock; ++by, addr += stride_offs)
      for (int bx = 0; bx < lruBlock; ++bx, ++addr)
      {
        atTile(tiles, ti.mip, addr) = stti;

        // all the previous get it, if they have nada, or this one is higher res
        for (int m = 0; m < stti.mip; ++m)
        {
          TexTileIndirection &tile = atTile(tiles, m, addr);
          if (tile.mip > stti.mip)
            tile = stti;
        }
        // all the ones after get it, if they have nada, or this one is lower res
        for (int m = stti.mip + 1; m < texMips; ++m)
        {
          TexTileIndirection &tile = atTile(tiles, m, addr);
          // if ( tile.mip==0xff || tile.mip<stti.mip )
          if (tile.mip < stti.mip || tile.mip > m)
            tile = stti;
        }
      }
  }
}

struct TwoInt16
{
  TwoInt16(int16_t x_, int16_t y_) : x(x_), y(y_) {}
  int16_t x, y;
};
typedef TwoInt16 int16_tx2;

struct ConstantVert
{
  int16_tx2 pos_wh;
  TexTileIndirection tti; // instead of tag, use target mip
};

void ClipmapImpl::checkUpload()
{
#define CHECK_GPU_LOAD 0
#if CHECK_GPU_LOAD
  SmallTab<TexTileIndirection, MidmemAlloc> tiles[TEX_MIPS];
  restoreIndirectionFromLRU(tiles);
  uint8_t *data;
  int stride;
  currentContext->indirectionTex->lockimg((void **)&data, stride, 0, TEXLOCK_READ);
  for (int mip = 0; mip < texMips; ++mip)
  {
    int ofsx = mip * TILE_WIDTH;

    for (int y = 0; y < TILE_WIDTH; ++y)
    {
      void *dest = ((char *)data) + ofsx * sizeof(TexTileIndirection) + y * stride;
      void *src = &tiles[mip][y * TILE_WIDTH];
      if (memcmp(dest, src, TILE_WIDTH * sizeof(TexTileIndirection)))
      {
        for (int x = 0; x < TILE_WIDTH; ++x)
          if (((uint32_t *)dest)[x] != ((uint32_t *)src)[x])
            debug("mip = %d, y = %d, x = %d GPU %X CPU %X", mip, y, x, ((uint32_t *)dest)[x], ((uint32_t *)src)[x]);
        save_rt_image_as_tga(currentContext->indirectionTex, "indirection.tga");
        G_ASSERT(0);
      }
    }
  }
  currentContext->indirectionTex->unlockimg();
#endif
}


void ClipmapImpl::GPUrestoreIndirectionFromLRUFull()
{
  TexLRUList &LRU = currentContext->LRU;
  carray<carray<uint16_t, MAX_CACHE_TILES>, TEX_MIPS> mippedLRU;
  carray<int, TEX_MIPS> mippedLRUCount;
  mem_set_0(mippedLRUCount);
  IBBox2 totalbox;
  int totalUpdated = 0;
  for (int i = 0; i < LRU.size(); ++i)
  {
    TexLRU &ti = LRU[i];
    if (ti.mip >= texMips || (ti.flags & ti.IS_INVALID))
      continue;
    mippedLRU[ti.mip][mippedLRUCount[ti.mip]++] = i;
    IPoint2 lefttop(((ti.tx - TILE_WIDTH / 2) << ti.mip), ((ti.ty - TILE_WIDTH / 2) << ti.mip));
    totalbox += lefttop;
    totalbox += lefttop + IPoint2((1 << ti.mip), (1 << ti.mip));
    totalUpdated++;
  }
  float ratio = texTileInnerSize * currentContext->tileZoom * pixelRatio;
  currentLRUBox[0] = Point2((totalbox[0].x + TILE_WIDTH / 2 + currentContext->xOffset) * ratio,
    (totalbox[0].y + TILE_WIDTH / 2 + currentContext->yOffset) * ratio);
  currentLRUBox[1] = Point2((totalbox[1].x + TILE_WIDTH / 2 + currentContext->xOffset) * ratio,
    (totalbox[1].y + TILE_WIDTH / 2 + currentContext->yOffset) * ratio);

  int maxQuadCount = 0;
  for (int i = 0; i < texMips; ++i)
    maxQuadCount += mippedLRUCount[i] * (i + 1);
  maxQuadCount++;

  ConstantVert *quad = (ConstantVert *)constantFillerBuf->lockData(maxQuadCount);
  if (!quad) // device lost?
  {
    currentContext->invalidateIndirection = true;
    return;
  }
  const ConstantVert *basequad = quad;
  TexTileIndirection tti(0xff, 0xff, 0xff, 0xff);
  quad[0].pos_wh = int16_tx2(0, 0);
  quad[0].tti = tti;
  quad[1].pos_wh = int16_tx2(TILE_WIDTH * TEX_MIPS * MAX_VTEX_CNT, 0);
  quad[1].tti = tti;
  quad[2].pos_wh = int16_tx2(TILE_WIDTH * TEX_MIPS * MAX_VTEX_CNT, TILE_WIDTH);
  quad[2].tti = tti;
  quad[3].pos_wh = int16_tx2(0, TILE_WIDTH);
  quad[3].tti = tti;
  quad += 4;
  for (int mipI = texMips - 1; mipI >= 0; --mipI)
  {
    for (int i = 0; i < mippedLRUCount[mipI]; ++i)
    {
      const TexLRU &ti = LRU[mippedLRU[mipI][i]];
      G_ASSERT(ti.mip == mipI);
      TexTileIndirection tti(ti.x, ti.y, min((uint8_t)ti.mip, (uint8_t)(texMips - 1)), 0xFF);
      for (int targI = 0; targI <= mipI; ++targI)
      {
        int mip_width = 1 << (mipI - targI);
        // int titx = clamp(ti.tx<<(mipI-targI), 0, TILE_WIDTH-1);
        // int tity = clamp(ti.ty<<(mipI-targI), 0, TILE_WIDTH-1);
        int titx = (ti.tx - TILE_WIDTH / 2) * mip_width + TILE_WIDTH / 2;
        int tity = (ti.ty - TILE_WIDTH / 2) * mip_width + TILE_WIDTH / 2;
        // skip drawing quads out of texture
        if (titx >= TILE_WIDTH || tity >= TILE_WIDTH || titx + mip_width < 0 || tity + mip_width < 0)
          continue;
        int pixel_width = min(mip_width, mip_width + titx);
        int pixel_height = min(mip_width, mip_width + tity);
        titx = max(0, titx);
        tity = max(0, tity);

        titx += ti.ri_index * TILE_WIDTH * texMips;
        titx += targI * TILE_WIDTH;
        tti.tag = targI;
        quad[0].pos_wh = int16_tx2(titx, tity);
        quad[0].tti = tti;
        quad[1].pos_wh = int16_tx2(titx + pixel_width, tity);
        quad[1].tti = tti;
        quad[2].pos_wh = int16_tx2(titx + pixel_width, tity + pixel_height);
        quad[2].tti = tti;
        quad[3].pos_wh = int16_tx2(titx, tity + pixel_height);
        quad[3].tti = tti;
        quad += 4;
      }
    }
  }
  Driver3dRenderTarget prevRt;
  d3d::get_render_target(prevRt);

  d3d::set_render_target(currentContext->indirection.getTex2D(), 0);
  d3d::clearview(CLEAR_DISCARD_TARGET, 0xFFFFFFFF, 1.f, 0);
  constantFillerBuf->unlockDataAndFlush((quad - basequad) / 4);

  currentContext->invalidateIndirection = false;
  d3d::resource_barrier({currentContext->indirection.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  // static int fullId = 0;
  // save_rt_image_as_tga(currentContext->indirectionTex,String(128, "indirection_full_%02d.tga", fullId));
  // fullId++;

  d3d::set_render_target(prevRt);
}

void ClipmapImpl::GPUrestoreIndirectionFromLRU()
{
  if (!indirectionChanged && !currentContext->invalidateIndirection)
    return;
  TIME_D3D_PROFILE(GPUrestoreIndirectionFromLRU);
  GPUrestoreIndirectionFromLRUFull();
  checkUpload();
  indirectionChanged = false;

  checkConsistency();
}

void ClipmapImpl::dump()
{
  int used = 0;
  if (currentContext)
  {
    debug("zoom = %d, xOffset = %d, yOffset = %d", currentContext->tileZoom, currentContext->xOffset, currentContext->yOffset);
    BBox2 box;
    const TexLRUList &LRU = currentContext->LRU;
    float ratio = texTileInnerSize * currentContext->tileZoom * pixelRatio;
    for (int i = 0; i < LRU.size(); ++i)
    {
      const TexLRU &ti = LRU[i];
      if (ti.mip == 0xFF)
      {
        debug("tile = %d", i);
        continue;
      }

      BBox2 lruBox;
      lruBox[1] = lruBox[0] = Point2((ti.tx << ti.mip) + currentContext->xOffset, (ti.ty << ti.mip) + currentContext->yOffset) * ratio;
      lruBox[1] += Point2((1 << ti.mip) * ratio, (1 << ti.mip) * ratio);
      box += lruBox;
      used++;
      debug("tile = %d x =%d y=%d mip= %d, flags = %d tx = %d ty = %d worldbbox = (%g %g) (%g %g)", i, ti.x, ti.y, ti.mip, ti.flags,
        ti.tx, ti.ty, P2D(lruBox[0]), P2D(lruBox[1]));
      // todo: make different priority BETTER_UPDATE and UPDATE_LATER (for not in frustum) and update only if previous priority has
      // been already updated
    }
    debug("total LRU Box = (%g %g) (%g %g)", P2D(box[0]), P2D(box[1]));
  }
  debug("used = %d", used);
}

IPoint2 ClipmapImpl::getTileOffset(int ri_index) const
{
  IPoint2 offset;
  if (ri_index == 0)
  {
    offset.x = currentContext->xOffset;
    offset.y = currentContext->yOffset;
  }
  else
  {
    centerOffset(getMappedRiPos(ri_index), offset.x, offset.y, RI_LANDCLASS_FIXED_ZOOM);
    offset.x -= (TILE_WIDTH / 2);
    offset.y -= (TILE_WIDTH / 2);
  }
  return offset;
}


Point3 ClipmapImpl::getMappedRiPos(int ri_index) const
{
  G_ASSERTF(ri_index > 0 && ri_index < MAX_VTEX_CNT, "ri_index = %d", ri_index);
  Point3 mappedCenter = Point3::ZERO;
#if MAX_RI_VTEX_CNT_BITS > 0
  int realRiIndex = ri_index - 1;

  Color4 rendinst_landscape_area_left_top_right_bottom = ShaderGlobal::get_color4(rendinst_landscape_area_left_top_right_bottomVarId);
  Point4 ri_landscape_tc_to_world =
    Point4(rendinst_landscape_area_left_top_right_bottom.b - rendinst_landscape_area_left_top_right_bottom.r,
      rendinst_landscape_area_left_top_right_bottom.a - rendinst_landscape_area_left_top_right_bottom.g,
      rendinst_landscape_area_left_top_right_bottom.r, rendinst_landscape_area_left_top_right_bottom.g);

  Point4 mapping = riMappings[realRiIndex];
  Point2 mappedCenterUV = Point2(mapping.z, mapping.w); // only correct if the UVs center is at 0,0
  mappedCenter = Point3(mappedCenterUV.x * ri_landscape_tc_to_world.x + ri_landscape_tc_to_world.z, 0,
    mappedCenterUV.y * ri_landscape_tc_to_world.y + ri_landscape_tc_to_world.w);
#endif
  return mappedCenter;
};


Point3 ClipmapImpl::getMappedRelativeRiPos(int ri_index, const Point3 &viewer_position, float &dist_to_ground) const
{
  dist_to_ground = 0;
#if MAX_RI_VTEX_CNT_BITS > 0
  if (ri_index != 0)
  {
    G_ASSERTF(ri_index > 0 && ri_index < MAX_VTEX_CNT, "ri_index = %d", ri_index);
    int realRiIndex = ri_index - 1;

    TMatrix lmeshTmInv = riInvMatrices[realRiIndex];
    Point4 uv2l = getUV2landacape(ri_index);
    Point3 mappedCenter = Point3(uv2l.z, 0, uv2l.w); // only correct if the UVs center is at 0,0
    Point3 relPos = lmeshTmInv * viewer_position;

    dist_to_ground = relPos.y; // TODO: use smarter distance calc, the same as for sorting keys

    return mappedCenter + relPos;
  }
#endif
  return viewer_position;
};

Point4 ClipmapImpl::getLandscape2uv(const Point2 &offset, float zoom) const
{
  float pRatio = zoom * pixelRatio;
  Point2 TO;
  TO.x = float(offset.x) / TILE_WIDTH_F;
  TO.y = float(offset.y) / TILE_WIDTH_F;
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

Point4 ClipmapImpl::getLandscape2uv(int ri_index) const
{
  G_ASSERTF(ri_index >= 0 && ri_index < MAX_VTEX_CNT, "ri_index = %d", ri_index);
  return getLandscape2uv((Point2)getTileOffset(ri_index), ri_index == 0 ? currentContext->tileZoom : RI_LANDCLASS_FIXED_ZOOM);
}

Point4 ClipmapImpl::getUV2landacape(const Point2 &offset, float zoom) const
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

Point4 ClipmapImpl::getUV2landacape(int ri_index) const
{
  Point4 landscape2uv = getLandscape2uv(ri_index);

  // uv = pos*A+B
  // pos = (uv-B)/A = uv*iA - B*iA
  Point4 uv2landscape;
  uv2landscape.x = 1.0f / landscape2uv.x;
  uv2landscape.y = 1.0f / landscape2uv.y;
  uv2landscape.z = -landscape2uv.z * uv2landscape.x;
  uv2landscape.w = -landscape2uv.w * uv2landscape.y;
  return uv2landscape;
}

Point4 ClipmapImpl::getFallbackLandscape2uv() const
{
  Point2 iLS;
  iLS.x = 1.0f / (TILE_WIDTH_F * texTileInnerSize);
  iLS.y = 1.0f / (TILE_WIDTH_F * texTileInnerSize);
  Point2 US;
  US.x = 1.0f / getPixelRatio();
  US.y = 1.0f / getPixelRatio();
  // this one is for the fallback texture
  Point4 landscape2uv_fallback;
  landscape2uv_fallback.x = iLS.x * US.x;
  landscape2uv_fallback.y = iLS.y * US.y;
  landscape2uv_fallback.z = 0.0f;
  landscape2uv_fallback.w = 0.0f;
  return landscape2uv_fallback;
}

bool ClipmapImpl::updateOrigin(const Point3 &cameraPosition, bool update_snap)
{
  int xofs, yofs;
  centerOffset(cameraPosition, xofs, yofs, currentContext->tileZoom);
  xofs = xofs - TILE_WIDTH / 2;
  yofs = yofs - TILE_WIDTH / 2;
  if (abs(xofs - currentContext->xOffset) >= 10 || abs(yofs - currentContext->yOffset) >= 10 ||
      (update_snap && (xofs != currentContext->xOffset || yofs != currentContext->yOffset)))
  {
    TIME_PROFILE(changeXYOffset);
    zoomTileLimit = max(zoomTileLimit, int(TEX_ORIGIN_LIMITER));
    return changeXYOffset(xofs, yofs);
  }
  return false;
}

bool Clipmap::isCompressionAvailable(uint32_t format) { return BcCompressor::isAvailable((BcCompressor::ECompressionType)format); }

void ClipmapImpl::centerOffset(const Point3 &position, int &xofs, int &yofs, float zoom) const
{
  float pRatio = zoom * pixelRatio;
  xofs = (int)floor(safediv(position.x, pRatio) / texTileInnerSize);
  yofs = (int)floor(safediv(position.z, pRatio) / texTileInnerSize);
  int useMips = alignMips;
  xofs = (xofs + (1 << (useMips - 2))) & (~((1 << (useMips - 1)) - 1));
  yofs = (yofs + (1 << (useMips - 2))) & (~((1 << (useMips - 1)) - 1));
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

void ClipmapImpl::beginUpdateTiles()
{
  d3d::set_render_target();
  for (int i = 0; i < bufferCnt; ++i)
    d3d::set_render_target(i, bufferTex[i].getTex2D(), 0);
  pClipmapRenderer->startRenderTiles(Point2::xz(viewerPosition));
}

void ClipmapImpl::endUpdateTiles() { pClipmapRenderer->endRenderTiles(); }

void ClipmapImpl::finalizeCompressionQueue()
{
  if (!compressionQueue.size())
    return;
  for (int ci = 0; ci < MAX_TEXTURES; ++ci)
  {
    if (bufferTex[ci].getTex2D())
    {
      if (mipCnt > 1)
      {
        bufferTex[ci].getTex2D()->generateMips();
      }
      d3d::resource_barrier({bufferTex[ci].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, unsigned(mipCnt)});
    }
    bufferTex[ci].setVar();
  }

  bool hasUncompressed = false;
  for (int ci = 0; ci < cacheCnt; ++ci)
    if (compressor[ci] && compressor[ci]->getCompressionType() != BcCompressor::COMPRESSION_ERR)
    {
      for (int mipNo = 0; mipNo < mipCnt; mipNo++)
        compressor[ci]->updateFromMip(bufferTex[ci].getTexId(), mipNo, mipNo, compressionQueue.size());
    }
    else
      hasUncompressed = true;

  G_ASSERT(compressionQueue.size() <= COMPRESS_QUEUE_SIZE);
  // fixme: actually we need compressors for each format, not for each texture!
  for (int ci = 0; ci < cacheCnt; ++ci)
    if (compressor[ci] && compressor[ci]->getCompressionType() != BcCompressor::COMPRESSION_ERR)
    {
      for (int mipNo = 0; mipNo < mipCnt; mipNo++)
        for (int i = 0; i < compressionQueue.size(); ++i)
          compressor[ci]->copyToMip(cache[ci].getTex2D(), mipNo, (compressionQueue[i].x * texTileSize) >> mipNo,
            (compressionQueue[i].y * texTileSize) >> mipNo, mipNo, (i * texTileSize) >> mipNo, 0, texTileSize >> mipNo,
            texTileSize >> mipNo);
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
      for (int ci = 0; ci < cacheCnt; ++ci)
        d3d::set_render_target(ci, compressor[ci] ? nullptr : cache[ci].getTex2D(), mipNo);
      directUncompressElem->setStates(0, true);
      d3d::set_vs_const1(51, mipNo, 0, 0, 0);
      d3d::set_vs_const(52, &quads[0].x, quads.size());
      d3d::draw(PRIM_TRILIST, 0, quads.size() * 2);
    }

    for (int ci = 0; ci < cacheCnt; ++ci)
    {
      for (int mipNo = 0; mipNo < mipCnt; mipNo++)
      {
        d3d::resource_barrier({cache[ci].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(mipNo), 1});
      }
    }
  }
  compressionQueue.clear();
}

void ClipmapImpl::addCompressionQueue(int cx, int cy)
{
  if (compressionQueue.size() >= COMPRESS_QUEUE_SIZE)
    finalizeCompressionQueue();
  compressionQueue.push_back(IPoint2(cx, cy));
}

void ClipmapImpl::updateTileBlockRaw(int cx, int cy, const BBox2 &region)
{
  addCompressionQueue(cx, cy);
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
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);

  if (COMPRESS_QUEUE_SIZE != 1)
    d3d::setview((compressionQueue.size() - 1) * texTileSize, 0, texTileSize, texTileSize, 0, 1);

  pClipmapRenderer->renderTile(region);
}

void ClipmapImpl::updateTileBlock(int mip, int cx, int cy, int x, int y, int ri_index)
{
  int block = 1 << mip;
  Point4 uv2landscape = getUV2landacape(ri_index);
  x = ((x - (TILE_WIDTH / 2)) << mip) + (TILE_WIDTH / 2);
  y = ((y - (TILE_WIDTH / 2)) << mip) + (TILE_WIDTH / 2);

  float brd = float(texTileBorder << mip) / float(texTileInnerSize);
  Point2 t0(float(x - brd) / TILE_WIDTH, float(y - brd) / TILE_WIDTH);
  Point2 t1(float(x + brd + block) / TILE_WIDTH, float(y + brd + block) / TILE_WIDTH);
  BBox2 region;
  region[0].x = t0.x * uv2landscape.x + uv2landscape.z;
  region[0].y = t0.y * uv2landscape.y + uv2landscape.w;
  region[1].x = t1.x * uv2landscape.x + uv2landscape.z;
  region[1].y = t1.y * uv2landscape.y + uv2landscape.w;
  updateTileBlockRaw(cx, cy, region);
}

////////////// END OF VIRTUAL TEXTURE /////////////


void ClipmapImpl::close()
{
  closeVirtualTexture();
  // END OF THE VIRTUAL TEXTURE
}

void ClipmapImpl::init(float texel_size, uint32_t feedback_type, int tex_mips, int tex_tile_size, int virtual_texture_mip_cnt,
  int virtual_texture_anisotropy, int min_tex_tile_border)
{
  init_shader_vars();

  texTileSize = tex_tile_size;
  minTexTileBorder = min_tex_tile_border;
  setTexMips(tex_mips);
  cacheAnisotropy = min(virtual_texture_anisotropy, (int)MAX_TEX_TILE_BORDER);
  eastl::fill(swFeedbackTilesPerMip.begin(), swFeedbackTilesPerMip.end(), -1);

  mipCnt = virtual_texture_mip_cnt;
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_APPLE    // MAC version of ATI cant do an anisotropy with mips properly
#define PS4_ANISO_WA 0 // and caused grids of black stripes.
#else
#define PS4_ANISO_WA 0
  if (cacheAnisotropy > 1)
  {
    if (d3d_get_gpu_cfg().primaryVendor == D3D_VENDOR_ATI)
    {
      mipCnt = max(mipCnt, 2); // But ATI on PC requires 2 valid mips for aniso without harsh moire.
    }
  }
#endif

  texTileBorder = max(minTexTileBorder, cacheAnisotropy); // bilinear is maximum
  texTileInnerSize = texTileSize - texTileBorder * 2;
  debug("init texTileBorder = %d", texTileBorder);

  pixelRatio = texel_size;
  feedbackType = feedback_type;

  if (shader_exists("clipmap_clear_histogram_ps"))
  {
    clearHistogramPs.init("clipmap_clear_histogram_ps");
    fillHistogramPs.init("clipmap_fill_histogram_ps");
    buildTileInfoPs.init("clipmap_build_tile_info_ps");
  }

  if (shader_exists("clipmap_clear_histogram_cs"))
  {
    clearHistogramCs.reset(new_compute_shader("clipmap_clear_histogram_cs", true));
    fillHistogramCs.reset(new_compute_shader("clipmap_fill_histogram_cs", true));
    buildTileInfoCs.reset(new_compute_shader("clipmap_build_tile_info_cs", true));
  }
}

void ClipmapImpl::invalidate(bool force_redraw)
{
  // TODO:
  //  support force_redraw
  threadpool::wait(&feedbackJob);
  if (currentContext)
  {
    currentContext->reset(cacheDimX, cacheDimY, force_redraw, texTileSize, feedbackType);
  }
  if (force_redraw)
    currentLRUBox.setempty();
}

// enum {MOST_LEFT = ((-TILE_WIDTH/2)<<(TEX_MIPS-1)) + TILE_WIDTH/2, MOST_RIGHT = ((TILE_WIDTH/2)<<(TEX_MIPS-1)) + TILE_WIDTH/2};

void ClipmapImpl::invalidatePointi(int tx, int ty, int lastMip)
{

  if (tx < mostLeft || tx >= mostRight || ty < mostLeft || ty >= mostRight)
    return;

  threadpool::wait(&feedbackJob);
  tx -= TILE_WIDTH / 2;
  ty -= TILE_WIDTH / 2;
  if (currentContext)
  {
    TexLRUList &LRU = currentContext->LRU;
    for (auto &ti : LRU)
    {
      if (ti.mip >= lastMip)
        continue;

      int tmipx = (tx >> ti.mip) + (TILE_WIDTH / 2), tmipy = (ty >> ti.mip) + (TILE_WIDTH / 2);
      if (ti.tx == tmipx && ti.ty == tmipy)
        ti.flags |= TexLRU::NEED_UPDATE; // same priority!
      // todo: make different priority BETTER_UPDATE and UPDATE_LATER (for not in frustum) and update only if previous priority has
      // been already updated
    }
  }
}

void ClipmapImpl::invalidatePoint(const Point2 &world_xz)
{
  Point4 landscape2uv = getLandscape2uv(TERRAIN_RI_INDEX); // TODO: maybe add support for RI invalidation
  Point2 xz = Point2(landscape2uv.x * world_xz.x, landscape2uv.y * world_xz.y) + Point2(landscape2uv.z, landscape2uv.w);
  invalidatePointi(xz.x * TILE_WIDTH, xz.y * TILE_WIDTH, 0xFF);
}

void ClipmapImpl::invalidateBox(const BBox2 &world_xz)
{
  Point4 landscape2uv = getLandscape2uv(TERRAIN_RI_INDEX); // TODO: maybe add support for RI invalidation
  DECL_ALIGN16(Point4, txBox) =
    Point4(landscape2uv.x * world_xz[0].x + landscape2uv.z, landscape2uv.y * world_xz[0].y + landscape2uv.w,
      landscape2uv.x * world_xz[1].x + landscape2uv.z, landscape2uv.y * world_xz[1].y + landscape2uv.w);
  int tx0 = txBox.x * TILE_WIDTH;
  int ty0 = txBox.y * TILE_WIDTH;
  int tx1 = txBox.z * TILE_WIDTH;
  int ty1 = txBox.w * TILE_WIDTH;
  if (tx1 < mostLeft || tx0 > mostRight || ty1 < mostLeft || ty0 >= mostRight)
    return;
  IPoint2 texelSize =
    IPoint2((txBox.z - txBox.x) * (texTileInnerSize * TILE_WIDTH), (txBox.w - txBox.y) * (texTileInnerSize * TILE_WIDTH));
  int lastMip = 0;
  for (; lastMip < texMips; ++lastMip, texelSize.x >>= 1, texelSize.y >>= 1)
    if (texelSize.x == 0 && texelSize.y == 0)
      break;

  if (!lastMip)
    return;
  if (tx1 == tx0 && ty1 == ty0)
  {
    invalidatePointi(tx0, ty0, lastMip);
    return;
  }
  tx0 -= TILE_WIDTH / 2;
  ty0 -= TILE_WIDTH / 2;
  tx1 -= TILE_WIDTH / 2;
  ty1 -= TILE_WIDTH / 2;

  threadpool::wait(&feedbackJob);

  if (currentContext)
  {
    TexLRUList &LRU = currentContext->LRU;
    for (auto &ti : LRU)
    {
      if (ti.mip >= lastMip)
        continue;

      int tmipx0 = (tx0 >> ti.mip) + (TILE_WIDTH / 2), tmipy0 = (ty0 >> ti.mip) + (TILE_WIDTH / 2);
      int tmipx1 = (tx1 >> ti.mip) + (TILE_WIDTH / 2), tmipy1 = (ty1 >> ti.mip) + (TILE_WIDTH / 2);
      if (ti.tx >= tmipx0 && ti.tx <= tmipx1 && ti.ty >= tmipy0 && ti.ty <= tmipy1)
        ti.flags |= TexLRU::NEED_UPDATE; // same priority!
      // todo: make different priority BETTER_UPDATE and UPDATE_LATER (for not in frustum) and update only if previous priority has
      // been already updated
    }
  }
}

void ClipmapImpl::prepareSoftwarePoi(const int vx, const int vy, const int mip, const int mipsize,
  carray<uint64_t, (TEX_MIPS * TILE_WIDTH * TILE_WIDTH + 63) / 64> &bitarrayUsed, const Point4 &uv2landscapetile,
  const vec4f *planes2d, int planes2dCnt)
{
  G_ASSERTF(MAX_RI_VTEX_CNT == 0, "software feedback is not supported with RI clipmap indirection"); // TODO: implement it
  int ri_index = 0;
  int uvy = ((vy - TILE_WIDTH / 2) >> mip) + TILE_WIDTH / 2, uvx = ((vx - TILE_WIDTH / 2) >> mip) + TILE_WIDTH / 2;
  vec4f width = v_make_vec4f(uv2landscapetile.x * (1 << mip), 0, 0, 0);
  vec4f height = v_make_vec4f(0, 0, uv2landscapetile.y * (1 << mip), 0);
  for (int y = -mipsize; y <= mipsize; ++y)
  {
    int cy = uvy + y; //
    if (cy < 0 || cy >= TILE_WIDTH)
      continue;
    for (int x = -mipsize; x <= mipsize; ++x)
    {
      int cx = uvx + x;
      if (cx < 0 || cx >= TILE_WIDTH)
        continue;
      const int type_size = sizeof(bitarrayUsed[0]) * 8;
      const int type_bits_ofs = 6;
      G_STATIC_ASSERT((1 << type_bits_ofs) == type_size);
      uint32_t elementFullAddr = (mip << (TILE_WIDTH_BITS + TILE_WIDTH_BITS)) + (cy << TILE_WIDTH_BITS) + cx;
      uint32_t elementAddr = elementFullAddr >> type_bits_ofs;
      uint64_t bitAddr = uint64_t(1ULL << uint64_t(elementFullAddr & (type_size - 1)));
      if (bitarrayUsed[elementAddr] & bitAddr)
        continue; // fixme: not used in first poi
      // total++;
      if (planes2dCnt > 0)
      {
        int worldCx = ((cx - TILE_WIDTH / 2) << mip) + TILE_WIDTH / 2, worldCy = ((cy - TILE_WIDTH / 2) << mip) + TILE_WIDTH / 2;
        vec4f pos =
          v_make_vec4f(uv2landscapetile.x * worldCx + uv2landscapetile.z, 0, uv2landscapetile.y * worldCy + uv2landscapetile.w, 0);
        if (!is_2d_frustum_visible(pos, width, height, planes2d, planes2dCnt))
          continue;
      }
      tileInfo[tileInfoSize++].set(cx, cy, mip, 0xffff, ri_index);
      bitarrayUsed[elementAddr] |= bitAddr; // fixme: not used in second poi
    }
  }
}

void ClipmapImpl::prepareSoftwareFeedback(int nextZoom, const Point3 &center, const TMatrix &view_itm, const TMatrix4 &globtm,
  float approx_height_above_land, float maxClipMoveDist0, float maxClipMoveDist1, bool force_update)
{
  G_ASSERTF(MAX_RI_VTEX_CNT == 0, "software feedback is not supported with RI clipmap indirection"); // TODO: implement it

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

  Point4 landscape2uv = getLandscape2uv(TERRAIN_RI_INDEX);
  Point4 uv2landscape = getUV2landacape(TERRAIN_RI_INDEX);
  Point4 uv2landscapetile(uv2landscape.x / TILE_WIDTH, uv2landscape.y / TILE_WIDTH, uv2landscape.z, uv2landscape.w);

  int vx = (center0.x * landscape2uv.x + landscape2uv.z) * TILE_WIDTH;
  int vy = (center0.y * landscape2uv.y + landscape2uv.w) * TILE_WIDTH;
  // G_STATIC_ASSERT(sizeof(uint64_t)*8 == TILE_WIDTH);
  carray<uint64_t, (TEX_MIPS * TILE_WIDTH * TILE_WIDTH + 63) / 64> bitarrayUsed;
  mem_set_0(bitarrayUsed);
  for (int mip = 0; mip < texMips; ++mip)
  {
    int uvy = ((vy - TILE_WIDTH / 2) >> mip) + TILE_WIDTH / 2, uvx = ((vx - TILE_WIDTH / 2) >> mip) + TILE_WIDTH / 2;
    int mipsize = (mip >= texMips - 1) ? swFeedbackOuterMipTiles : swFeedbackInnerMipTiles;
    if (swFeedbackTilesPerMip[mip] >= 0)
    {
      mipsize = swFeedbackTilesPerMip[mip];
    }
    prepareSoftwarePoi(vx, vy, mip, mipsize, bitarrayUsed, uv2landscapetile, planes2d.data(), planes2dCnt);
  }

  if (maxClipMoveDist0 > 0 && maxClipMoveDist0 < maxClipMoveDist1)
  {
    int vx = (center1.x * landscape2uv.x + landscape2uv.z) * TILE_WIDTH;
    int vy = (center1.y * landscape2uv.y + landscape2uv.w) * TILE_WIDTH;
    for (int mip = 2; mip < texMips; ++mip)
    {
      const int mipsize = 2;
      prepareSoftwarePoi(vx, vy, mip, mipsize, bitarrayUsed, uv2landscapetile, planes2d.data(), planes2dCnt);
    }
  }
  // debug("total = %d invisible = %d", total, totalInvisible);
  // debug("tileInfoSize =%d",tileInfoSize);

  if (tileInfoSize)
  {
    tileLimit = max(maxTileLimit, zoomTileLimit);
    feedbackJob.initFromSoftware(*this, force_update);
    threadpool::add(&feedbackJob);
  }
}

void ClipmapImpl::setDDScale()
{
  G_ASSERT(targetSize.x > 0 && targetSize.y > 0);

  float ddw = float(FEEDBACK_WIDTH) / float(targetSize.x);
  float ddh = float(FEEDBACK_HEIGHT) / float(targetSize.y);

  float sddh = (targetSize.y > 1200) ? sqrtf(float(targetSize.y) / 1080) : 1;
  ShaderGlobal::set_color4(worldDDScaleVarId, Color4(ddw, ddh, sddh, sddh));
}

void ClipmapImpl::prepareFeedback(ClipmapRenderer &renderer, const Point3 &center, const TMatrix &view_itm, const TMatrix4 &globtm,
  float zoom_height, float maxClipMoveDist0, float maxClipMoveDist1, float approx_height_above_land, bool force_update,
  bool low_tpool_prio)
{
  setDDScale();

  G_ASSERT(currentContext);
  // Frustum frustum(*globtm);
  //  i need world-space X_Z here
  Point3 oldViewerPosition = viewerPosition;
  viewerPosition = center;
  // debug("at %f %f pixel ratio %f, at tile %i %i", center2.x, center2.y, pixelRatio, xOffset, yOffset);

  // NOTE
  //  comment this out to avoid bad zoom artefacts
  float zoom_at_0m = 0.0f;
  float zoom_at_Bm = 2.0f;
  float bHeight = 20.f;
  float zoom_limit = TEX_MAX_ZOOM;
  float new_zoom;
  zoom_height /= bHeight;

  float b_zoom = zoom_at_0m;
  float k_zoom = (zoom_at_Bm - zoom_at_0m);

  new_zoom = zoom_height * k_zoom + b_zoom;

  if (fixed_zoom > 0)
    new_zoom = max<float>(fixed_zoom, 1);

  new_zoom = max(new_zoom, 1.0f);

  const float log2e = 1.44269504089f;
  const float ln2 = 0.6931471805599453f;
  if (fabsf(new_zoom - currentContext->tileZoom) < 1.f)
    new_zoom = currentContext->tileZoom;
  else
    new_zoom = exp(ln2 * floorf(log(new_zoom) * log2e)); // exp2(floor(log2(new_zoom)))
  new_zoom = min(new_zoom, zoom_limit);

  int nextZoom = currentContext->tileZoom;
  zoomTileLimit = (zoomTileLimit + TEX_DEFAULT_LIMITER) >> 1; // this way we will have few frames with gradually decreasing limiter
                                                              // after zoom, instead of one very slow frame
  if (new_zoom > currentContext->tileZoom * 1.5f)
    nextZoom = currentContext->tileZoom * 2, zoomTileLimit = TEX_ZOOM_LIMITER;
  else if (new_zoom < currentContext->tileZoom / 1.5f || (new_zoom < 2.0 && currentContext->tileZoom >= 2.0))
    nextZoom = currentContext->tileZoom / 2, zoomTileLimit = TEX_ZOOM_LIMITER;

  if (!changeZoom(nextZoom, viewerPosition))
    updateOrigin(viewerPosition, true);

  if (feedbackType == SOFTWARE_FEEDBACK)
  {
    prepareSoftwareFeedback(nextZoom, center, view_itm, globtm, approx_height_above_land, maxClipMoveDist0, maxClipMoveDist1,
      force_update);
    Point4 landscape2uv = getLandscape2uv(TERRAIN_RI_INDEX);
    ShaderGlobal::set_color4(feedback_landscape2uvVarId, Color4(&landscape2uv.x));
    return;
  }
  Point4 landscape2uv = getLandscape2uv(TERRAIN_RI_INDEX);
  ShaderGlobal::set_color4(feedback_landscape2uvVarId, Color4(&landscape2uv.x));

  pClipmapRenderer = &renderer;

  ////////////////////////
  // prepare feedback here

  int newestpossible = 1;
#if _TARGET_C1 | _TARGET_C2

#endif
  int newestCaptureTarget = currentContext->captureTarget - newestpossible;

  if (currentContext->captureTarget >= MAX_FRAMES_AHEAD)
  {
    int captureTargetIdx = -1;
    int lockidx = newestCaptureTarget;
#if !(_TARGET_C1 | _TARGET_C2)
    for (; lockidx >= currentContext->oldestCaptureTargetIdx; lockidx--)
    {
      TIME_D3D_PROFILE(query_status)
      if (d3d::get_event_query_status(currentContext->captureTexFence[lockidx % MAX_FRAMES_AHEAD].get(), false))
      {
        captureTargetIdx = lockidx % MAX_FRAMES_AHEAD;
        break;
      }
    }
    if (captureTargetIdx < 0 && ++failLockCount >= 6)
    {
      logwarn("can not lock target in 5 attempts! lock anyway, can cause stall");
      captureTargetIdx = currentContext->oldestCaptureTargetIdx % MAX_FRAMES_AHEAD;
    }
    if (captureTargetIdx < 0 && force_update)
      captureTargetIdx = currentContext->oldestCaptureTargetIdx % MAX_FRAMES_AHEAD;
#else

#endif
    // debug("frames %d captureTargetIdx = %d(%d)", currentContext->captureTarget, captureTargetIdx, lockidx);
    if (captureTargetIdx >= 0)
    {
      failLockCount = 0;

      HWFeedbackMode feedbackMode = getHWFeedbackMode(captureTargetIdx);
      bool canStartJob = false;
      {
        TIME_PROFILE(retrieveFeedback);

        if (checkTextureFeedbackUsage(feedbackMode))
        {
          TIME_D3D_PROFILE(lockAndCopyTex);
          int stride;
          if (auto lockedTex = lock_texture<uint8_t>(currentContext->captureTex[captureTargetIdx].getTex2D(), stride, 0,
                TEXLOCK_READ | TEXLOCK_RAWDATA))
          {
            const uint8_t *data = lockedTex.get();
            for (int y = 0; y < FEEDBACK_HEIGHT; ++y, data += stride)
              memcpy(lockedPixels.data() + y * FEEDBACK_WIDTH, data, FEEDBACK_WIDTH * sizeof(TexTileFeedback));
            canStartJob = true;
          }
        }

        if (checkTileFeedbackUsage(feedbackMode))
        {
          TIME_D3D_PROFILE(lockAndCopyBuf);

          lockedTileInfoCnt = 0;
          if (auto lockedBuf = lock_sbuffer<const PackedTile>(currentContext->captureTileInfoBuf[captureTargetIdx].getBuf(), 0, 0, 0))
          {
#if MAX_RI_VTEX_CNT_BITS > 0
            lockedTileInfoCnt = lockedBuf[0].x;
#else
            lockedTileInfoCnt = lockedBuf[0];
#endif
            if (lockedTileInfoCnt > MAX_TILE_CAPTURE_INFO_BUF_ELEMENTS)
            {
              logerr("lockedTileInfoCnt is out of bounds: %d > %d", lockedTileInfoCnt, MAX_TILE_CAPTURE_INFO_BUF_ELEMENTS);
              lockedTileInfoCnt = 0;
            }
            if (lockedTileInfoCnt > 0)
              memcpy(lockedTileInfo.data(), &lockedBuf[1], sizeof(PackedTile) * lockedTileInfoCnt);
          }
          canStartJob = true;
        }
      }

      if (canStartJob)
      {
        feedbackJob.initFromHWFeedback(feedbackMode, globtm, *this, currentContext->captureWorldOffset[captureTargetIdx][0],
          currentContext->captureWorldOffset[captureTargetIdx][1], currentContext->captureWorldOffset[captureTargetIdx][2],
          currentContext->captureUV2landscape[captureTargetIdx], force_update);
        if (force_update)
          feedbackJob.doJob();
        else
          threadpool::add(&feedbackJob, low_tpool_prio ? threadpool::PRIO_LOW : threadpool::PRIO_NORMAL);
        // currentContext->oldestCaptureTargetIdx = (lockidx+1)%MAX_FRAMES_AHEAD;
        currentContext->oldestCaptureTargetIdx = lockidx + 1;
      }
    }
  }

  enableUAVOutput(true);

  if (currentContext->captureTarget - currentContext->oldestCaptureTargetIdx < MAX_FRAMES_AHEAD)
  {
    int captureTargetIdx = (currentContext->captureTarget % MAX_FRAMES_AHEAD);
    if (canUseUAVFeedback())
    {
      Texture *tex = currentContext->captureTex[captureTargetIdx].getTex2D();
      float one[4] = {1.0, 1.0, 1.0, 1.0};
      d3d::clear_rwtexf(tex, one, 0, 0);
    }
    else
    {
      // debug("render to %d (%d), oldest = %d(%d)", captureTargetIdx, currentContext->captureTarget,
      //   currentContext->oldestCaptureTargetIdx%MAX_FRAMES_AHEAD, currentContext->oldestCaptureTargetIdx);
      Driver3dRenderTarget prevRt;
      d3d::get_render_target(prevRt);
      TIME_D3D_PROFILE(renderFeedback);
      mat44f oproj, ovtm;
      d3d::gettm(TM_PROJ, oproj);
      d3d::gettm(TM_VIEW, ovtm);
      Driver3dPerspective p;
      bool prevPerspValid = d3d::getpersp(p);

      d3d::set_render_target(currentContext->captureTex[captureTargetIdx].getTex2D(), 0);
      d3d::set_depth(feedbackDepthTex.getTex2D(), DepthAccess::RW);
      TMatrix4 scaledGlobTm = globtm * TMatrix4(feedback_view_scale, 0, 0, 0, 0, feedback_view_scale, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
      d3d::settm(TM_PROJ, &scaledGlobTm);
      d3d::settm(TM_VIEW, TMatrix::IDENT);
      d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(255, 255, 255, 255), 0.0f, 0);
      pClipmapRenderer->renderFeedback(scaledGlobTm);
      d3d::settm(TM_PROJ, oproj);
      d3d::settm(TM_VIEW, ovtm);
      if (prevPerspValid)
        d3d::setpersp(p);
      // save_rt_image_as_tga(currentContext->captureTex[currentContext->captureTargetIdx], "feedback.tga");
      copyAndAdvanceCaptureTarget(captureTargetIdx);
      d3d::set_render_target(prevRt);
    }
  }
}

bool ClipmapImpl::canUseOptimizedReadback(int captureTargetIdx) const
{
  return currentContext->debugHWFeedbackConvar[captureTargetIdx] == 1 || currentContext->debugHWFeedbackConvar[captureTargetIdx] == 2;
}
HWFeedbackMode ClipmapImpl::getHWFeedbackMode(int captureTargetIdx) const
{
  int oldXOffset = currentContext->captureWorldOffset[captureTargetIdx][0];
  int oldYOffset = currentContext->captureWorldOffset[captureTargetIdx][1];
  int oldZoom = currentContext->captureWorldOffset[captureTargetIdx][2];

  int hwFeedbackConvar = currentContext->debugHWFeedbackConvar[captureTargetIdx];
  bool debugCompare = hwFeedbackConvar == 2;
  bool useOptimization = hwFeedbackConvar == 1;

  if (debugCompare)
    return HWFeedbackMode::DEBUG_COMPARE;
  else if (oldZoom != currentContext->tileZoom)
    return HWFeedbackMode::ZOOM_CHANGED;
  else if (currentContext->xOffset != oldXOffset || currentContext->yOffset != oldYOffset)
    return HWFeedbackMode::OFFSET_CHANGED;
  else if (useOptimization)
    return HWFeedbackMode::DEFAULT_OPTIMIZED;
  else
    return HWFeedbackMode::DEFAULT;
}

void ClipmapImpl::copyAndAdvanceCaptureTarget(int captureTargetIdx)
{
  {
    TIME_PROFILE(copyFeedback);

    {
      TIME_PROFILE(copyImg);
      int stride;
      if (
        currentContext->captureTex[captureTargetIdx]->lockimg(nullptr, stride, 0, TEXLOCK_READ | TEXLOCK_NOSYSLOCK | TEXLOCK_RAWDATA))
        currentContext->captureTex[captureTargetIdx]->unlockimg();
    }

    if (canUseOptimizedReadback(captureTargetIdx))
    {
      if (currentContext->captureTileInfoBuf[captureTargetIdx]->lock(0, 0, (void **)nullptr, VBLOCK_READONLY))
        currentContext->captureTileInfoBuf[captureTargetIdx]->unlock();
    }
  }
  d3d::issue_event_query(currentContext->captureTexFence[captureTargetIdx].get());
  currentContext->captureTarget++;
  currentContext->captureWorldOffset[captureTargetIdx][0] = currentContext->xOffset;
  currentContext->captureWorldOffset[captureTargetIdx][1] = currentContext->yOffset;
  currentContext->captureWorldOffset[captureTargetIdx][2] = currentContext->tileZoom;
  currentContext->captureUV2landscape[captureTargetIdx] = getUV2landacape(TERRAIN_RI_INDEX);
}

void ClipmapImpl::enableUAVOutput(bool enable)
{
  int value = enable && canUseUAVFeedback() ? 1 : 0;
  ShaderGlobal::set_int(supports_uavVarId, value);
}

void ClipmapImpl::startUAVFeedback(int reg_no)
{
  if (feedbackType != CPU_HW_FEEDBACK || !canUseUAVFeedback())
    return;

  d3d::set_rwtex(STAGE_PS, reg_no, currentContext->captureTex[currentContext->captureTarget % MAX_FRAMES_AHEAD].getTex2D(), 0, 0);
}

void ClipmapImpl::dispatchTileFeedback(int captureTargetIdx)
{
  G_ASSERT(clearHistogramPs.getElem() && fillHistogramPs.getElem() && buildTileInfoPs.getElem()); // PS is mandatory

  TIME_D3D_PROFILE(clipmap_feedback_transform);

  bool useCS = use_cs_for_hw_feedback_process && fillHistogramCs && buildTileInfoCs;
  if (!useCS)
    use_cs_for_hw_feedback_process = false; // just so it's not a silent fallback

  ShaderGlobal::set_int(clipmap_tex_mipsVarId, texMips);
  SCOPE_VIEWPORT;
  {
    TIME_D3D_PROFILE(clear_histogram);
    int stage = useCS ? STAGE_CS : STAGE_PS;
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(stage, clipmap_clear_tile_info_buf_rw_const_no, VALUE),
      currentContext->captureTileInfoBuf[captureTargetIdx].getBuf());
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(stage, clipmap_clear_histogram_buf_rw_const_no, VALUE),
      currentContext->intermediateHistBuf.getBuf());
    if (useCS)
    {
      clearHistogramCs->dispatchThreads(TEX_TOTAL_ELEMENTS, 1, 1);
    }
    else
    {
      d3d::setview(0, 0, TILE_WIDTH * TEX_MIPS, TILE_WIDTH, 0, 1);
      clearHistogramPs.render();
    }
  }
  ResourceBarrier barrierStage = useCS ? RB_STAGE_COMPUTE : RB_STAGE_PIXEL;
  ResourceBarrier sourceStage = useCS ? RB_SOURCE_STAGE_COMPUTE : RB_SOURCE_STAGE_PIXEL;
  d3d::resource_barrier({currentContext->captureTex[captureTargetIdx].getTex2D(), RB_RO_SRV | barrierStage, 0, 0});
  d3d::resource_barrier({currentContext->intermediateHistBuf.getBuf(), RB_FLUSH_UAV | sourceStage | barrierStage});
  d3d::resource_barrier({currentContext->captureTileInfoBuf[captureTargetIdx].getBuf(), RB_FLUSH_UAV | sourceStage | barrierStage});
  {
    TIME_D3D_PROFILE(fill_histogram);
    int stage = useCS ? STAGE_CS : STAGE_PS;
    ShaderGlobal::set_texture(clipmap_feedback_texVarId, currentContext->captureTex[captureTargetIdx]);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(stage, clipmap_histogram_buf_rw_const_no, VALUE),
      currentContext->intermediateHistBuf.getBuf());
    if (useCS)
    {
      fillHistogramCs->dispatchThreads(FEEDBACK_WIDTH, FEEDBACK_HEIGHT, 1);
    }
    else
    {
      d3d::setview(0, 0, FEEDBACK_WIDTH, FEEDBACK_HEIGHT, 0, 1);
      fillHistogramPs.render();
    }
  }
  d3d::resource_barrier({currentContext->intermediateHistBuf.getBuf(), RB_RO_SRV | barrierStage});
  {
    TIME_D3D_PROFILE(build_tile_info);
    int stage = useCS ? STAGE_CS : STAGE_PS;
    ShaderGlobal::set_buffer(clipmap_histogram_bufVarId, currentContext->intermediateHistBuf);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(stage, clipmap_tile_info_buf_rw_const_no, VALUE),
      currentContext->captureTileInfoBuf[captureTargetIdx].getBuf());
    if (useCS)
    {
      buildTileInfoCs->dispatchThreads(TILE_WIDTH, TILE_WIDTH, 1);
    }
    else
    {
      d3d::setview(0, 0, TILE_WIDTH, TILE_WIDTH, 0, 1);
      buildTileInfoPs.render();
    }
  }
  // transfer texture to UAV state, to avoid slow autogenerated barrier inside driver
  // when binding this texture as RW_UAV later on in startUAVFeedback
  d3d::resource_barrier({currentContext->captureTex[captureTargetIdx].getTex2D(), RB_RW_UAV | RB_STAGE_PIXEL, 0, 0});
}

void ClipmapImpl::endUAVFeedback(int reg_no)
{
  if (feedbackType != CPU_HW_FEEDBACK || !canUseUAVFeedback())
    return;

  d3d::set_rwtex(STAGE_PS, reg_no, nullptr, 0, 0);
}

void ClipmapImpl::copyUAVFeedback()
{
  if (feedbackType != CPU_HW_FEEDBACK || !canUseUAVFeedback() ||
      currentContext->captureTarget - currentContext->oldestCaptureTargetIdx >= MAX_FRAMES_AHEAD)
    return;

  int captureTargetIdx = currentContext->captureTarget % MAX_FRAMES_AHEAD;

  int feedbackTargetMode = hw_feedback_target_mode;
  if (feedbackTargetMode != 0 && !(clearHistogramPs.getElem() && fillHistogramPs.getElem() && buildTileInfoPs.getElem()))
    feedbackTargetMode = hw_feedback_target_mode = 0; // just so it's not a silent fallback

  currentContext->debugHWFeedbackConvar[captureTargetIdx] = feedbackTargetMode;

  if (canUseOptimizedReadback(captureTargetIdx))
    dispatchTileFeedback(captureTargetIdx);

  copyAndAdvanceCaptureTarget(currentContext->captureTarget % MAX_FRAMES_AHEAD);
}

void ClipmapImpl::finalizeFeedback()
{
  TIME_PROFILE(waitForFeedbackJob);
  threadpool::wait(&feedbackJob);

  pClipmapRenderer = nullptr;
}

void ClipmapImpl::prepareRender(ClipmapRenderer &renderer, bool force_update, bool turn_off_decals_on_fallback)
{
  TIME_D3D_PROFILE(prepareTiles);

  pClipmapRenderer = &renderer;
  changeFallback(viewerPosition, turn_off_decals_on_fallback);
  update(force_update);
  updateRenderStates();

  pClipmapRenderer = nullptr;
}


struct SortRendinstBySortOrder
{
  bool operator()(const rendinst::RendinstLandclassData &a, const rendinst::RendinstLandclassData &b) const
  {
    return a.sortOrder < b.sortOrder;
  }
};

static float calc_rendinst_sort_order(const rendinst::RendinstLandclassData &ri_landclass_data, const Point3 &camera_pos)
{
  Point3 relPos = ri_landclass_data.matrixInv * camera_pos;

  if (relPos.y < 0)
    return FLT_MAX;

  float distSq = relPos.y * relPos.y;

  float radius = ri_landclass_data.radius;
  Point2 relPos2d = Point2(relPos.x, relPos.z);
  float dist2dSq = lengthSq(relPos2d);
  if (dist2dSq > radius * radius)
  {
    Point2 pos2d = relPos2d * (radius / sqrt(dist2dSq));
    Point3 edgePos = Point3(pos2d.x, 0, pos2d.y);
    distSq = lengthSq(relPos - edgePos);
  }
  return distSq;
}

static void sort_rendinst_info_list(rendinst::TempRiLandclassVec &ri_landclasses, const Point3 &camera_pos)
{
  for (int i = 0; i < ri_landclasses.size(); i++)
    ri_landclasses[i].sortOrder = calc_rendinst_sort_order(ri_landclasses[i], camera_pos);
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

void ClipmapImpl::updateRendinstLandclassParams()
{
#if MAX_RI_VTEX_CNT_BITS > 0

  rendinst::TempRiLandclassVec rendinstLandclassesCandidate;
  rendinst::getRendinstLandclassData(rendinstLandclassesCandidate);

  sort_rendinst_info_list(rendinstLandclassesCandidate, viewerPosition);

  riLandclassCount = min<int>(rendinstLandclassesCandidate.size(), MAX_RI_VTEX_CNT);

  // stable sorting, so feedback result don't need special invalidation:
  rendinstLandclassesCandidate.resize(riLandclassCount);
  rendinst::TempRiLandclassVec rendinstLandclassesNewElements;
  eastl::bitset<MAX_RI_VTEX_CNT> prevMatchingIndices;
  for (const auto &candidate : rendinstLandclassesCandidate)
  {
    auto it = eastl::find_if(prevRendinstLandclasses.begin(), prevRendinstLandclasses.end(),
      [&candidate](const auto &prevData) { return prevData.index >= 0 && candidate.index == prevData.index; });
    int index;
    if (it != prevRendinstLandclasses.end() && (index = eastl::distance(prevRendinstLandclasses.begin(), it)) < riLandclassCount)
      prevMatchingIndices.set(index);
    else
      rendinstLandclassesNewElements.push_back(candidate);
  }

  rendinst::TempRiLandclassVec rendinstLandclasses;
  rendinstLandclasses.resize(riLandclassCount);
  for (int i = 0, insertedId = 0; i < rendinstLandclasses.size(); ++i)
    if (prevMatchingIndices[i])
      rendinstLandclasses[i] = prevRendinstLandclasses[i];
    else
      rendinstLandclasses[i] = rendinstLandclassesNewElements[insertedId++];

  int minCnt = min<int>(prevRendinstLandclasses.size(), riLandclassCount);
  for (int i = 0; i < minCnt; ++i)
    if (rendinstLandclasses[i].index != prevRendinstLandclasses[i].index)
      changedRiIndices[i] = ri_invalid_frame_cnt;

  for (int i = riLandclassCount; i < prevRendinstLandclasses.size(); ++i)
    changedRiIndices[i] = ri_invalid_frame_cnt;

  if (log_ri_indirection_change)
  {
    eastl::string txt;
    txt += "{";
    bool hasIt = false;
    for (int j = 0; j < changedRiIndices.size(); ++j)
    {
      if (changedRiIndices[j])
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

  static constexpr int RI_INVALID_ID = -2; // -1 is invalid source RI, -2 is invalid target RI, they can't match

  prevRendinstLandclasses.resize(riLandclassCount);
  for (int i = 0; i < riLandclassCount; ++i)
    prevRendinstLandclasses[i] = rendinstLandclasses[i];

  carray<Point4, MAX_RI_VTEX_CNT> l2uvArr;
  carray<int, MAX_RI_VTEX_CNT> tmpRiIndices;
  for (int i = 0; i < riLandclassCount; ++i)
  {
    const rendinst::RendinstLandclassData &ri_landclass = rendinstLandclasses[i];
    tmpRiIndices[i] = ri_landclass.index;
    riInvMatrices[i] = ri_landclass.matrixInv;
    riMappings[i] = transform_mapping(ri_landclass.mapping);
    l2uvArr[i] = getLandscape2uv(i + 1) + Point4(0, 0, -0.5f, -0.5f); // same offset applied as in clipmap shader
  }
  for (int i = riLandclassCount; i < MAX_RI_VTEX_CNT; ++i)
    tmpRiIndices[i] = RI_INVALID_ID;

  int changedRiIndexBits = 0;
  for (int i = 0; i < changedRiIndices.size(); ++i)
    if (changedRiIndices[i])
      changedRiIndexBits |= 1 << i;

  riLandclassCount += 1; // adding terrain
  ShaderGlobal::set_color4_array(clipmap_ri_landscape2uv_arrVarId, l2uvArr.data(), MAX_RI_VTEX_CNT);

  ShaderGlobal::set_int(ri_landclass_changed_indicex_bitsVarId, changedRiIndexBits);

  G_STATIC_ASSERT(MAX_RI_VTEX_CNT <= 8);
  IPoint4 riIndices0 = IPoint4(RI_INVALID_ID, RI_INVALID_ID, RI_INVALID_ID, RI_INVALID_ID);
  IPoint4 riIndices1 = IPoint4(RI_INVALID_ID, RI_INVALID_ID, RI_INVALID_ID, RI_INVALID_ID);
  for (int i = 0; i < min(4, MAX_RI_VTEX_CNT); ++i)
    riIndices0[i] = tmpRiIndices[i];
  for (int i = 4; i < min(8, MAX_RI_VTEX_CNT); ++i)
    riIndices1[i - 4] = tmpRiIndices[i];
  ShaderGlobal::set_int4(ri_landclass_closest_indices_0VarId, riIndices0);
  ShaderGlobal::set_int4(ri_landclass_closest_indices_1VarId, riIndices1);

#endif
}

void ClipmapImpl::updateRenderStates()
{
  G_ASSERT(currentContext);

  for (int ci = 0; ci < cacheCnt; ++ci)
    cache[ci].setVar(); // new way

  ShaderGlobal::set_texture(indirectionTexVarId, currentContext->indirection.getTexId());
  setDDScale();

  Point4 landscape2uv = getLandscape2uv(TERRAIN_RI_INDEX);
  ShaderGlobal::set_color4(landscape2uvVarId, Color4(&landscape2uv.x));

  updateRendinstLandclassParams();

  if (VariableMap::isGlobVariablePresent(fallback_info0VarId))
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
    ShaderGlobal::set_color4(fallback_info0VarId, fallback0.x, fallback0.y, fallback0.z, 0);
    ShaderGlobal::set_color4(fallback_info1VarId, fallback1.x, fallback1.y, gradScale.x, gradScale.y);
  }
}


bool ClipmapImpl::getBBox(BBox2 &ret) const
{
  if (currentLRUBox.isempty())
    return false;
  ret = currentLRUBox;
  return true;
}


bool ClipmapImpl::getMaximumBBox(BBox2 &ret) const
{
  Point4 uv2landscape = getUV2landacape(TERRAIN_RI_INDEX);
  ret[0] = Point2(uv2landscape.z, uv2landscape.w);
  ret[1] = Point2(uv2landscape.z + uv2landscape.x, uv2landscape.w + uv2landscape.y);
  ret.scale(1 << (texMips - 1));
  return true;
}

void ClipmapImpl::createCaches(const uint32_t *formats, uint32_t cnt, const uint32_t *buffer_formats, uint32_t buffer_cnt)
{
  cacheCnt = min(cnt, (uint32_t)cache.size());
  bufferCnt = min(buffer_cnt, (uint32_t)bufferTex.size());

  for (int ci = bufferCnt; ci < bufferTex.size(); ++ci)
    bufferTex[ci].close();
  for (int ci = cacheCnt; ci < cache.size(); ++ci)
  {
    del_it(compressor[ci]);
    cache[ci].close();
  }

  const int compressBufW = COMPRESS_QUEUE_SIZE * texTileSize;
  const int compressBufH = texTileSize;
  for (int ci = 0; ci < MAX_TEXTURES; ++ci)
  {
    String bufferVarName(30, "cache_buffer_tex_%d", ci);
    if (ci >= bufferCnt)
    {
      bufferTex[ci].setVarId(::get_shader_glob_var_id(bufferVarName, true));
      continue;
    }

    if (bufferTex[ci].getTex2D())
    {
      TextureInfo tinfo;
      bufferTex[ci].getTex2D()->getinfo(tinfo);
      if ((tinfo.cflg & TEXFMT_MASK) == (buffer_formats[ci] & TEXFMT_MASK) && (tinfo.w == compressBufW) && (tinfo.h == compressBufH) &&
          (tinfo.cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == (buffer_formats[ci] & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) &&
          bufferTex[ci].getTex2D()->level_count() == mipCnt)
      {
        continue;
      }
    }
    String bufferTexName(30, "cache_buffer_tex_%d%s", ci, postfix.str());
    const uint32_t mipGenFlag = mipCnt > 1 ? TEXCF_GENERATEMIPS : 0;

    bufferTex[ci].close();
    bufferTex[ci] =
      dag::create_tex(nullptr, compressBufW, compressBufH, buffer_formats[ci] | TEXCF_RTARGET | mipGenFlag, mipCnt, bufferTexName);
    d3d_err(bufferTex[ci].getTex2D());
    bufferTex[ci].setVarId(::get_shader_glob_var_id(bufferVarName, true));
    bufferTex[ci].getTex2D()->texfilter(TEXFILTER_POINT);
  }

  for (int ci = 0; ci < cacheCnt; ++ci)
  {
    if (cache[ci].getTex2D())
    {
      TextureInfo tinfo;
      cache[ci].getTex2D()->getinfo(tinfo);
      if ((tinfo.cflg & TEXFMT_MASK) == (formats[ci] & TEXFMT_MASK) &&
          (tinfo.cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == (formats[ci] & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)))
      {
        cache[ci].getTex2D()->setAnisotropy(cacheAnisotropy);
        continue;
      }
    }
    del_it(compressor[ci]);

    auto compressionType = get_texture_compression_type(formats[ci]);
    if (compressionType != BcCompressor::COMPRESSION_ERR)
    {
      bool etc2 = compressionType == BcCompressor::COMPRESSION_ETC2_RGBA || compressionType == BcCompressor::COMPRESSION_ETC2_RG;
      compressor[ci] = new BcCompressor(compressionType, mipCnt, compressBufW, compressBufH, COMPRESS_QUEUE_SIZE,
        etc2 ? String(64, "cache%d_etc2_compressor", ci) : String(64, "cache%d_compressor", ci));
    }

    String cacheTexName(30, "cache_tex%d%s", ci, postfix.str());
    cache[ci] = dag::create_tex(nullptr, cacheDimX, cacheDimY,
      formats[ci] | (compressor[ci] ? TEXCF_UPDATE_DESTINATION : TEXCF_RTARGET) | TEXCF_CLEAR_ON_CREATE, max(mipCnt, 1 + PS4_ANISO_WA),
      cacheTexName);
    d3d_err(cache[ci].getTex2D());
    cache[ci].getTex2D()->setAnisotropy(cacheAnisotropy);
    if (cacheAnisotropy != ::dgs_tex_anisotropy)
      add_anisotropy_exception(cache[ci].getTexId());
#if PS4_ANISO_WA
    cache[ci].getTex2D()->texmiplevel(0, 0);
#endif
  }


  texTileBorder = max(minTexTileBorder, cacheAnisotropy);

  debug("parallax texTileBorder = %d cacheAnisotropy = %d", texTileBorder, cacheAnisotropy);

  // we have to set here, since it is not guaranteed that it was re-created
  setTileSizeVars();

  invalidate(true);
}

void ClipmapImpl::afterReset()
{
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
    compressor[ci] = new BcCompressor(get_texture_compression_type(texInfo.cflg), mipCnt, compressBufW, compressBufH,
      COMPRESS_QUEUE_SIZE, String(64, "cache%d_compressor", ci));
  }
}

bool ClipmapImpl::is_uav_supported()
{
#if _TARGET_IOS
  return false;
#endif
  const GpuUserConfig &gpuCfg = d3d_get_gpu_cfg();
  if (d3d::get_driver_desc().shaderModel < 5.0_sm || (gpuCfg.disableUav && !get_shader_variable_id("requires_uav", true)))
    return false;
#if _TARGET_PC_MACOSX
  if (!(d3d::get_texformat_usage(TEXFMT_R8G8B8A8) & d3d::USAGE_PIXREADWRITE) && d3d::guess_gpu_vendor() == D3D_VENDOR_INTEL)
    return false;
#endif
  return true;
}
