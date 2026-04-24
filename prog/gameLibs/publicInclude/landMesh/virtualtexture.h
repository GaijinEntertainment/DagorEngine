//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/unique_ptr.h>

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
  IPoint2 feedbackSize;      // = Clipmap::FEEDBACK_DEFAULT_SIZE
  int feedbackOversampling;  // = 1
  bool useFeedbackTexFilter; // = false
};

struct ZoomFeeedbackParams
{
  float height = 1.f;
  float wk = 1.f;
  int minimumZoom = 1;

  // mipsCutoff parameters allow us to decrease mip levels to mipsCutoff.texMips on zoom levels equal or greater than mipsCutoff.zoom.
  struct
  {
    int zoom = -1;
    int texMips = -1;

    bool isSet() const { return zoom > 0 && texMips > 0; }
  } mipsCutoff;
};

struct SoftwareFeedbackParams
{
  TMatrix viewItm = TMatrix::IDENT;
  float maxDist0 = 0.f;
  float maxDist1 = 0.f;
  float approxHt = 0.f;
};

struct DynamicDDScaleParams
{
  float targetAtlasUsageRatio = 0.0f;
  float atlasUsageRatioHysteresisLow = 0.3f;
  float atlasUsageRatioHysteresisHigh = 0.9f;
  float proportionalTerm = 0.1f;
};

class Clipmap
{
public:
  enum
  {
    SOFTWARE_FEEDBACK,
    CPU_HW_FEEDBACK
  };

  static constexpr int MAX_TEXTURES = 4;
  static constexpr uint32_t BAD_TEX_FLAGS = -1;

  static const IPoint2 FEEDBACK_DEFAULT_SIZE;
  static const int MAX_TEX_MIP_CNT;

  static FeedbackProperties getSuggestedOversampledFeedbackProperties(IPoint2 target_res);
  static FeedbackProperties getDefaultFeedbackProperties() { return {FEEDBACK_DEFAULT_SIZE, 1, false}; };

  static bool isCompressionAvailable(uint32_t format);
  static bool is_uav_supported();

  void initVirtualTexture(int cacheDimX, int cacheDimY, int tex_tile_size, int virtual_texture_mip_cnt, int virtual_texture_anisotropy,
    float maxEffectiveTargetResolution);
  void closeVirtualTexture();

  // fallbackTexelSize is fallbackPage size in meters
  // can be set to something like max(4, (1<<(getMaxTexMips()-1))*getPixelRatio*8) - 8 times bigger texel than last clip, but
  // not less than 4 meters if we allocate two pages, it results in minimum 4*256*2 == 1024meters (512 meters radius) of fallback
  // pages_dim*pages_dim is amount of pages allocated for fallback
  void initFallbackPage(int pages_dim, float fallback_texel_size);

  int getMaxTexMips() const;
  int getCurrentTexMips() const;
  void setMaxTileUpdateCount(int count);
  int getZoom() const;
  float getPixelRatio() const;

  void setMaxTexMips(int tex_mips);

  Clipmap(bool use_uav_feedback = true);
  ~Clipmap();

  // sz - texture size of each clip, clip_count - number of clipmap stack. multiplayer - size of next clip
  // size in meters of latest clip, virtual_texture_mip_cnt - amount of mips(lods) of virtual texture containing tiles
  void init(float st_texel_size, uint32_t feedbackType, FeedbackProperties feedback_properties = getDefaultFeedbackProperties(),
    int texMips = 6, bool use_own_buffers = true);
  void close();

  // returns bits of rendered clip, or 0 if no clip was rendered
  void setMaxTexelSize(float max_texel_size);

  float getStartTexelSize() const;
  void setStartTexelSize(float st_texel_size);

  void setTargetSize(int w, int h, float mip_bias);
  void prepareRender(ClipmapRenderer &render, dag::Span<Texture *> buffer_tex = {}, dag::Span<Texture *> compressor_tex = {},
    bool turn_off_decals_on_fallback = false);
  void prepareFeedback(const Point3 &viewer_pos, const TMatrix4 &globtm, const TMatrix4 *mirror_globtm_opt,
    const ZoomFeeedbackParams &zoom_params, const SoftwareFeedbackParams &software_fb = {}, bool force_update = false,
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
  uint32_t getFeedbackType() const;
  void setSoftwareFeedbackRadius(int inner_tiles, int outer_tiles);
  void setSoftwareFeedbackMipTiles(int mip, int tiles_for_mip);

  void startUAVFeedback();
  void endUAVFeedback();
  void increaseUAVAtomicPrefix(); // By increasing UAV atomic prefix following UAV feedback operations can overdraw feedback.
  void resetUAVAtomicPrefix();

  void copyUAVFeedback();
  void createCaches(const uint32_t *formats, const uint32_t *uncompressed_formats, uint32_t cnt, const uint32_t *buffer_formats,
    uint32_t buffer_cnt);

  String getCacheBufferName(int idx);
  IPoint2 getCacheBufferDim();
  dag::ConstSpan<uint32_t> getCacheBufferFlags();
  dag::ConstSpan<uint32_t> getCacheCompressorFlags();
  int getCacheBufferMips();

  // Used to temporarily reduce amount of caches/buffers used in baking without complete reinit
  // For example, when transitioned to a unit type where full clipmap is unnecessary (plane)
  // Negative arguments signal to restore original cache/buffer count, previously registered in createCaches
  void setCacheBufferCount(int cache_count, int buffer_count);

  // Gradients used for feedback/rendering mip resolution will be scaled to keep used/total tile ratio below
  // params.targetAtlasUsageRatio newDDScale = oldDDScale + proportionalTerm * (currentAtlasUsageRatio - params.targetAtlasUsageRatio)
  // The feature is enabled when params.targetAtlasUsageRatio > 0
  void setDynamicDDScale(const DynamicDDScaleParams &params);

  // This function attempts to set anisotropy level (used for sampling and internal calculations) with minimal changes
  // Returns true on success
  // Returned false means that changing anisotropy without complete clipmap reinit (init + createCaches) is not possible
  bool setAnisotropy(int anisotropy);

  void beforeReset();
  void afterReset();

  int getLastUpdatedTileCount() const;

  // debug
  void toggleManualUpdate(bool manual_update);
  void requestManualUpdates(int count);

  void getRiLandclassIndices(dag::Vector<int> &ri_landclass_indices);
  Point3 getMappedRelativeRiPos(int ri_offset, const Point3 &viewer_position, float &dist_to_ground) const;

  void resetSecondaryFeedbackCenter();
  void setSecondaryFeedbackCenter(const Point3 &center);

  void startSecondaryFeedback();
  void endSecondaryFeedback();

  void setHmapOfsAndTexOfs(int idx, const Point4 &hamp_ofs, const Point4 &hmap_tex_ofs);
  void updateLandclassData();

  enum class DebugMode : int
  {
    Disable,
    FeedbackResponse,
    TilesColoring,
  };
  void setDebugColorMode(DebugMode mode);

private:
  eastl::unique_ptr<ClipmapImpl> clipmapImpl;
};
