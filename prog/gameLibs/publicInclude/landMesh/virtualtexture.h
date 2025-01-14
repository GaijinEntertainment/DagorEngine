//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint2.h>

class ClipmapImpl;
class TMatrix4;
class Point4;
class Point3;
class Point2;
class BBox2;

enum class HWFeedbackMode
{
  DEFAULT = 0,
  OPTIMIZED = 1,
  DEBUG_COMPARE = 2,
  INVALID = -1,
};

enum class UpdateFeedbackThreadPolicy
{
  SAME_THREAD = 0, // Prevent running update job in a separate worker thread.
  PRIO_NORMAL = 1,
  PRIO_LOW = 2
};

enum
{
  MIN_TEX_TILE_BORDER = 1,
  MAX_TEX_TILE_BORDER = 16, // no sense to have more than 16
                            // TEX_TILE_BORDER = 16,//for parallax shadow we need 16, if no parallax - 8 is enough (for 8x anisotropy,
                            // which is already slow)! TEX_TILE_INNER_SIZE = TEX_TILE_SIZE - 2*TEX_TILE_BORDER, TILE_WIDTH =
                            // 1<<TILE_WIDTH_BITS, TEX_WIDTH = TILE_WIDTH * TEX_TILE_INNER_SIZE, TEX_HEIGHT = TILE_HEIGHT *
                            // TEX_TILE_INNER_SIZE,
  TEX_DEFAULT_CACHE_DIM = 4 * 1024,
  TEX_DEFAULT_LIMITER = 48,
  TEX_DEFAULT_CLIPMAP_UPDATES_PER_FRAME = 8,
  TEX_DEFAULT_CLIPMAP_UPDATES_PER_FRAME_MOBILE = 4,
  TEX_ZOOM_LIMITER = TEX_DEFAULT_LIMITER + TEX_DEFAULT_LIMITER / 2,
  TEX_ORIGIN_LIMITER = TEX_DEFAULT_LIMITER + (TEX_DEFAULT_LIMITER >> 3),
  TEX_MAX_ZOOM = 64,
  MAX_FEEDBACK_RESOLUTION = 0xFFFF - 1,
  FEEDBACK_DEFAULT_WIDTH = 320,
  FEEDBACK_DEFAULT_HEIGHT = 192
};

class ClipmapRenderer
{
public:
  virtual void startRenderTiles(const Point2 &center) = 0;
  virtual void renderTile(const BBox2 &region) = 0;
  virtual void endRenderTiles() = 0;
  virtual void renderFeedback(const TMatrix4 &) { G_ASSERT(0); }
};

struct FeedbackProperties
{
  IPoint2 feedbackSize;     // = Clipmap::FEEDBACK_DEFAULT_SIZE
  int feedbackOversampling; // = 1
};

class Clipmap
{
public:
  enum
  {
    SOFTWARE_FEEDBACK,
    CPU_HW_FEEDBACK
  };

  static const IPoint2 FEEDBACK_DEFAULT_SIZE;
  static const int MAX_TEX_MIP_CNT;

  static FeedbackProperties getSuggestedOversampledFeedbackProperties(IPoint2 target_res);
  static FeedbackProperties getDefaultFeedbackProperties() { return {FEEDBACK_DEFAULT_SIZE, 1}; };

  void initVirtualTexture(int cacheDimX, int cacheDimY, float maxEffectiveTargetResolution);
  void closeVirtualTexture();
  bool updateOrigin(const Point3 &cameraPosition, bool update_snap);

  static bool isCompressionAvailable(uint32_t format);

  // fallbackTexelSize is fallbackPage size in meters
  // can be set to something like max(4, (1<<(getAlignMips()-1))*getPixelRatio*8) - 8 times bigger texel than last aligned clip, but
  // not less than 4 meters if we allocate two pages, it results in minimum 4*256*2 == 1024meters (512 meters radius) of fallback
  // pages_dim*pages_dim is amount of pages allocated for fallback
  void initFallbackPage(int pages_dim, float fallback_texel_size);

  bool changeXYOffset(int nx, int ny);                  // return true if chaned
  bool changeZoom(int newZoom, const Point3 &position); // return true if chaned
  void update();
  void updateCache();
  void checkConsistency();
  bool checkClipmapInited();

  void GPUrestoreIndirectionFromLRUFull(); // restore completely
  void GPUrestoreIndirectionFromLRU();
  int getTexMips() const;
  int getAlignMips() const;
  void setMaxTileUpdateCount(int count);
  int getZoom() const;
  float getPixelRatio() const;

  void setTexMips(int tex_mips);
  Clipmap(bool use_uav_feedback = true);
  ~Clipmap();
  // sz - texture size of each clip, clip_count - number of clipmap stack. multiplayer - size of next clip
  // size in meters of latest clip, virtual_texture_mip_cnt - amount of mips(lods) of virtual texture containing tiles
  void init(float st_texel_size, uint32_t feedbackType, FeedbackProperties feedback_properties = getDefaultFeedbackProperties(),
    int texMips = 6,
#if _TARGET_IOS || _TARGET_ANDROID || _TARGET_C3
    int tex_tile_size = 128
#else
    int tex_tile_size = 256
#endif
    ,
    int virtual_texture_mip_cnt = 1, int virtual_texture_anisotropy = ::dgs_tex_anisotropy);
  void close();

  // returns bits of rendered clip, or 0 if no clip was rendered
  void setMaxTexelSize(float max_texel_size);

  float getStartTexelSize() const;
  void setStartTexelSize(float st_texel_size);

  void setTargetSize(int w, int h, float mip_bias);
  void prepareRender(ClipmapRenderer &render, bool turn_off_decals_on_fallback = false);
  void prepareFeedback(const Point3 &viewer_pos, const TMatrix &view_itm, const TMatrix4 &globtm, float height, float maxDist0 = 0.f,
    float maxDist1 = 0.f, float approx_ht = 0.f, bool force_update = false,
    UpdateFeedbackThreadPolicy thread_policy = UpdateFeedbackThreadPolicy::PRIO_LOW); // for software feeback only
  void renderFallbackFeedback(ClipmapRenderer &renderer, const TMatrix4 &globtm);
  void finalizeFeedback();

  void invalidatePointi(int tx, int ty, int lastMip = 0xFF); // not forces redraw!  (int tile coords)
  void invalidatePoint(const Point2 &world_xz);              // not forces redraw!
  void invalidateBox(const BBox2 &world_xz);                 // not forces redraw!
  void invalidate(bool force_redraw = true);

  void recordCompressionErrorStats(bool enable);
  void dump();

  bool getBBox(BBox2 &ret) const;
  bool getMaximumBBox(BBox2 &ret) const;
  void setFeedbackType(uint32_t ftp);
  uint32_t getFeedbackType() const;
  void setSoftwareFeedbackRadius(int inner_tiles, int outer_tiles);
  void setSoftwareFeedbackMipTiles(int mip, int tiles_for_mip);

  void enableUAVOutput(bool enable);

  void startUAVFeedback(int reg_no = 6);
  void endUAVFeedback(int reg_no = 6);
  void copyUAVFeedback();
  const UniqueTexHolder &getCache(int at);
  const UniqueTex &getIndirection() const;
  void createCaches(const uint32_t *formats, const uint32_t *uncompressed_formats, uint32_t cnt, const uint32_t *buffer_formats,
    uint32_t buffer_cnt);
  // Used to temporarily reduce amount of caches/buffers used in baking without complete reinit
  // For example, when transitioned to a unit type where full clipmap is unnecessary (plane)
  // Negative arguments signal to restore original cache/buffer count, previously registered in createCaches
  void setCacheBufferCount(int cache_count, int buffer_count);
  void beforeReset();
  void afterReset();

  int getLastUpdatedTileCount() const;

  static bool is_uav_supported();

  // debug
  void toggleManualUpdate(bool manual_update);
  void requestManualUpdates(int count);

  void getRiLandclassIndices(dag::Vector<int> &ri_landclass_indices);
  Point3 getMappedRelativeRiPos(int ri_offset, const Point3 &viewer_position, float &dist_to_ground) const;

private:
  eastl::unique_ptr<ClipmapImpl> clipmapImpl;
};
