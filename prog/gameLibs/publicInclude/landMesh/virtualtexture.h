//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_color.h>
#include <math/dag_bounds2.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_eventQueryHolder.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <util/dag_simpleString.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <3d/dag_drv3d.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <EASTL/bitset.h>


class ClipmapImpl;


enum class HWFeedbackMode
{
  INVALID,
  DEFAULT,
  DEFAULT_OPTIMIZED,
  ZOOM_CHANGED,
  OFFSET_CHANGED,
  DEBUG_COMPARE,
};

enum
{
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
};

struct TexTileIndirection
{
  uint8_t x, y;
  uint8_t mip;
  uint8_t tag;
  TexTileIndirection() : x(0), y(0), mip(0), tag(0) {}
  TexTileIndirection(int X, int Y, int MIP, int TAG)
  {
    x = X;
    y = Y;
    mip = MIP;
    tag = TAG;
  }
};


class ClipmapRenderer
{
public:
  virtual void startRenderTiles(const Point2 &center) = 0;
  virtual void renderTile(const BBox2 &region) = 0;
  virtual void endRenderTiles() = 0;
  virtual void renderFeedback(const TMatrix4 &) { G_ASSERT(0); }
};

class Clipmap
{
public:
  enum
  {
    SOFTWARE_FEEDBACK,
    CPU_HW_FEEDBACK
  };

  static const IPoint2 FEEDBACK_SIZE;
  static const int MAX_TEX_MIP_CNT;

  void initVirtualTexture(int cacheDimX, int cacheDimY, float maxEffectiveTargetResolution);
  void closeVirtualTexture();
  bool updateOrigin(const Point3 &cameraPosition, bool update_snap);

  static bool isCompressionAvailable(uint32_t format);

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
  void update(bool force_update);
  void updateMip(HWFeedbackMode hw_feedback_mode, bool force_update);
  void updateCache(bool force_update);
  void checkConsistency();

  void restoreIndirectionFromLRU(SmallTab<TexTileIndirection, MidmemAlloc> *tiles); // debug
  void GPUrestoreIndirectionFromLRUFull();                                          // restore completely
  void GPUrestoreIndirectionFromLRU();
  int getTexMips() const;
  int getAlignMips() const;
  void setMaxTileUpdateCount(int count);
  int getZoom() const;
  float getPixelRatio() const;

  TexTileIndirection &atTile(SmallTab<TexTileIndirection, MidmemAlloc> *tiles, int mip, int addr);
  void setTexMips(int tex_mips);
  Clipmap(const char *in_postfix = NULL, bool in_use_uav_feedback = true);
  ~Clipmap();
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
  void setMaxTexelSize(float max_texel_size);

  float getStartTexelSize() const;
  void setStartTexelSize(float st_texel_size);

  void setTargetSize(int w, int h, float mip_bias);
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
  void afterReset();

  static bool is_uav_supported();

private:
  eastl::unique_ptr<ClipmapImpl> clipmapImpl;
};
