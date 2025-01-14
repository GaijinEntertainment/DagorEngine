//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_TMatrix.h>
#include <math/dag_frustum.h>
#include <generic/dag_tab.h>
#include <util/dag_stdint.h>
// #include <GuillotineBinPack.h>
#include <PowerOfTwoBinPack.h>
#include <3d/dag_resPtr.h>
#include <EASTL/vector_set.h>
#include <EASTL/string.h>
#include <shaders/dag_overrideStateId.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/integer/dag_IPoint2.h>
#include <render/dynamicShadowRender.h>
#include <render/dynamicShadowRenderExtensions.h>
#include <render/shadowCastersFlags.h>

struct TraceLightInfo;

class ShadowSystem
{
public:
  void changeResolution(int atlasWidth, int max_shadow_size, int min_shadow_size, int shadow_size_step, bool dynamic_shadow_32bit);

  void setOverrideState(const shaders::OverrideState &baseState);
  void close();
  ShadowSystem(const char *name_suffix = "");
  ~ShadowSystem() { close(); }
  static const int MAX_PRIORITY = 15; // 15 times more important than anything else
  // priority - the higher, the better. keep in mind, that with very high value you can steal all updates from other volumes
  // hint_dyamic - light is typically moving, will be rendered each frame
  // only_static_casters - light will not cast shadows from dynamic objects
  // quality - the higher the better. It is the speed of going from lowest mip (min_shadow_size) to high mip
  // (max_shadow_size>>shadow_size_srl).
  //  shadow_size_srl - maximum size degradation (shft right bits count for max shadow. If shadow is 256 maximum, and srl is 2, than
  //  maximum size will be 64)
  // render_gpu_objects - Whether or not the volume should render GPU objects
  int allocateVolume(ShadowCastersFlag casters, bool hint_dynamic, uint16_t quality, uint8_t priority, uint8_t shadow_size_srl,
    DynamicShadowRenderGPUObjects render_gpu_objects);
  // if obb is not full world, it is additional limitation to shadow size. Typically, it is OBB from light
  // if invalidate is on, previous content of shadow volume will be declared invalid. Note, that most of the times, even wrong content
  // is better, than none. however, currently we store the final worldToTexture matrix, and so, shadow volume has to always be "bigger"
  // than light (enclose the light). may be it makes sense to separate atlas position from shadow projection matrix, but will require
  // additional constant in constant buffer (atlas position and scale)
  void setShadowVolume(uint32_t id, mat44f_cref invView, float zn, float zf, float wk, bbox3f_cref box);
  void setOctahedralShadowVolume(uint32_t id, vec3f pos, float zn, float zf, bbox3f_cref box);
  void setShadowVolumeContentChanged(uint32_t id, bool invalidate); // all content changed, but volume itself has not
  void setShadowVolumeDynamicContentChanged(uint32_t id);           // dynamic content changed, everything else is same
  void invalidateVolumeShadow(uint32_t id);                         // if something has changed in scene for a light
  void destroyVolume(uint32_t id);
  bool getShadowProperties(uint32_t id, ShadowCastersFlag &casters, bool &hint_dynamic, uint16_t &quality, uint8_t &priority,
    uint8_t &shadow_size_srl, DynamicShadowRenderGPUObjects &render_gpu_objects) const;
  Point2 getShadowUvSize(uint32_t id) const;
  Point4 getShadowUvMinMax(uint32_t id) const;

  void startPrepareShadows(); // increases currentFrame
  // useShadowOnFrame should be called for any light which should be called
  void useShadowOnFrame(int id); // hint that we need shadow now,in this frame

  // should be called before endPrepareShadows, but after all visible/used casters are added
  void setDynamicObjectsContent(const bbox3f *boxes, int count); // dynamic content within those boxes
  void invalidateStaticObjects(bbox3f_cref box);                 // invalidate static content within box

  // will dynamically change shadow resolution, etc.
  // prepares volumesToRender list
  void endPrepareShadows(dynamic_shadow_render::VolumesVector &volumesToRender, int max_shadow_volumes_to_update,
    float max_area_part_to_update, const Point3 &viewPos, float hk, mat44f_cref viewproj);

  const Frustum &getVolumeFrustum(uint16_t id) const { return volumesFrustum[id]; }
  const mat44f &getVolumeTexMatrix(uint16_t id) const { return volumesTexTM[id]; }
  const Point4 &getOctahedralVolumeTexData(uint16_t id) const { return volumesOctahedralData[id]; }
  const bbox3f &getVolumeBox(uint16_t id) const { return volumesBox[id]; }
  void startRenderVolumes(const dag::ConstSpan<uint16_t> &volumesToRender);

  enum RenderFlags
  {
    RENDER_NONE = 0,
    RENDER_STATIC = 1,
    RENDER_DYNAMIC = 2,
    RENDER_ALL = RENDER_DYNAMIC | RENDER_STATIC
  };

  void getVolumeUpdateData(uint32_t id, dynamic_shadow_render::FrameUpdate &result);
  RenderFlags getVolumeRenderFlags(uint32_t id);

  // returns the number of sub-views required
  uint32_t startRenderVolume(uint32_t id, mat44f &proj, RenderFlags &render_flags);
  void startRenderVolumeView(uint32_t id, uint32_t view_id, mat44f &viewItm, mat44f &view, RenderFlags render_flags,
    RenderFlags current_render_type);
  void startRenderTempShadow();
  void endRenderTempShadow() {}
  void startRenderStatic(uint32_t id, uint32_t view_id);
  void endRenderStatic(uint32_t id);

  void startRenderDynamic(uint32_t id);
  void endRenderVolumeView(uint32_t id, uint32_t view_id);
  void endRenderVolume(uint32_t id);
  void endRenderVolumes();

  void pruneFreeVolumes();
  void validateConsistency(); // in dev build only
  void invalidateAllVolumes();
  int getAtlasWidth() const { return atlasWidth; }
  int getAtlasHeight() const { return atlasHeight; }
  void debugValidate();
  bool getVolumeInfo(uint32_t id, float &wk, float &zn, float &zf) const
  {
    if (id >= volumes.size())
      return false;
    wk = volumes[id].wk;
    zn = volumes[id].zn;
    zf = volumes[id].zf;
    return true;
  }
  Texture *getTempShadowTexture() const { return tempCopy.getTex2D(); }
  void afterReset();

  // dynamic objects only need to be rendered inside this volume
  bbox3f getActiveShadowVolume() const { return activeShadowVolume; }

protected:
  // 8tap sampling when using PCF requires 3 pixels
  // simple PCF sampling requires 1 pixel
  static constexpr int OMNI_SHADOW_MARGIN = 3;

  // dag::ConstSpan<uint16_t> getShadowVolumesToUpdate() const {return volumesToUpdate;}

  rbp::PowerOfTwoBinPack atlas;
  UniqueTex dynamic_light_shadows;
  UniqueTex octahedral_temp_shadow;
  PostFxRenderer octahedralPacker;
  // temp Copy is used for static scene shadow caching. When we have static scene
  // should not be used on PS4/Vulkan, where we can alias
  // it is also used to store shadow map with static geometry only to find maximum range of light
  UniqueTex tempCopy;
  PostFxRenderer *copyDepth = 0;
  PostFxRenderer trace_shadow_depth_region;
  shaders::UniqueOverrideStateId cmpfAlwaysStateId, cmpfAlwaysNoBiasStateId;
  void copyAtlasRegion(int src_x, int src_y, int dst_x, int dst_y, int w, int h);

  struct AtlasRect : public rbp::Rect
  {
    AtlasRect(const rbp::Rect &r) : rbp::Rect(r) {}
    AtlasRect &operator=(const rbp::Rect &r)
    {
      (rbp::Rect &)*this = r;
      return *this;
    }
    void reset() { width = height = 0; }
    bool isEmpty() const { return height == 0; }
    AtlasRect() { reset(); }
  };
  struct Volume
  {
    mat44f invTm; //-V730_NOINIT
    float zn = 0.001, zf = 0.001, wk = 0.001;
    int16_t quality = 0;
    uint8_t sizeShift = 0;
    uint8_t priority = 0;
    enum
    {
      DYNAMIC_LIGHT = 1 << 0,
      ONLY_STATIC = 1 << 1,
      OCTAHEDRAL = 1 << 2,
      APPROXIMATE_STATIC = 1 << 3,
    };
    uint8_t flags = DYNAMIC_LIGHT;
    bool valid = false;
    AtlasRect shadow;
    AtlasRect dynamicShadow; // this is only not empty for static lights
    uint32_t lastFrameHasDynamicContent = 1;
    uint32_t lastFrameChanged = 1;
    uint32_t lastFrameUsed = 1;
    uint32_t lastFrameUpdated = 1;
    DynamicShadowRenderGPUObjects renderGPUObjects = DynamicShadowRenderGPUObjects::NO;
    uint32_t getNumViews() const;
    void invalidate();
    void buildProj(mat44f &proj) const;
    void buildView(mat44f &view, mat44f &invVIew, uint32_t view_id) const;
    void buildViewProj(mat44f &viewproj, uint32_t view_id) const;
    bool isOctahedral() const { return flags & OCTAHEDRAL; }

    // hasOnlyStaticCasters() should be in practical ways behave equal as isDynamic() (except no dynamic data should be rendered to)
    bool hasOnlyStaticCasters() const { return flags & ONLY_STATIC; }
    bool isDynamic() const { return flags & DYNAMIC_LIGHT; }
    bool isDynamicOrOnlyStaticCasters() const { return isDynamic() || hasOnlyStaticCasters(); }
    bool isApproximatelyTracedStaticCasters() const { return flags & APPROXIMATE_STATIC; }

    bool isDestroyed() const { return lastFrameUsed == 0; }
    bool isValidContent() const { return !shadow.isEmpty() && lastFrameChanged < lastFrameUpdated; }
    bool isStaticLightWithDynamicContent(uint32_t frame) const
    {
      return !isDynamicOrOnlyStaticCasters() && frame == lastFrameHasDynamicContent;
    }

    bool hasDynamicContentAt(uint32_t frame) const { return frame == lastFrameHasDynamicContent; }
    bool hasDynamicContent(uint32_t frame) const { return isDynamic() || hasDynamicContentAt(frame); }

    bool shouldBeRendered(uint32_t frame) const
    {
      if (shadow.isEmpty())
        return false;
      if (!isValidContent())
        return true;
      if (isDynamic() && (hasDynamicContentAt(frame) || hasDynamicContentAt(frame - 1))) // if last frame it had dynamic content we
                                                                                         // still need to invalidate
        return true;
      if (dynamicShadow.isEmpty() || !isStaticLightWithDynamicContent(frame))
        return false;
      return true;
    }
  };
  void copyStaticToDynamic(const Volume &);

  // volumes SoA
  Tab<bbox3f> volumesBox;
  Tab<vec4f> volumesSphere;
  Tab<Frustum> volumesFrustum;
  Tab<mat44f> volumesTexTM;
  Tab<Point4> volumesOctahedralData;
  Tab<Volume> volumes;
  // SoA-
  Tab<uint16_t> freeVolumes;
  eastl::vector_set<uint16_t> volumesToUpdate;
  bbox3f activeShadowVolume;

  uint16_t atlasWidth = 0, atlasHeight = 0;
  bool uses32bitShadow = false;

  uint32_t currentFrame = 2;
  int shadowStep = 0, maxShadow = 0, minShadow = 0, maxOctahedralTempShadow = 0;
  enum
  {
    UPDATE_ENDED,
    UPDATE_STARTED
  } currentState = UPDATE_ENDED;
  float notEnoughSpaceMul = 1.0f;
  uint32_t lastFrameQualityUpgraded = 1, lastFrameQualityDegraded = 1;

  eastl::string nameSuffix;

  // append nameSuffix to this name to get unique d3d names
  eastl::string getResName(const char *name) const
  {
    return eastl::string(eastl::string::CtorSprintf{}, "%s%s", name, nameSuffix.c_str());
  }

  int appendVolume();
  void resizeVolumes(int cnt);
  // return true if success. will try to free least recent used, if no space
  bool insertToAtlas(uint16_t id, uint16_t targetShadowSize);
  void packOctahedral(uint16_t id, bool dynamic_content);
  TraceLightInfo prepareApproximateStaticCasters(const Volume &volume, uint16_t atlas_w, uint16_t atlas_h);

  // return rect if there is one 100% fit. otherwise just free unused rects. call with big number to just free unused
  AtlasRect replaceUnused(uint16_t targetShadowSize); //
  bool atlasIsAllocated(AtlasRect r)
  {
    if (!r.width)
      return false;
    r.x /= minShadow;
    r.y /= minShadow;
    r.width /= minShadow;
    r.height /= minShadow;
    return atlas.isAllocated(r);
  }
  void atlasFree(AtlasRect r)
  {
    if (r.width == 0)
      return;
    G_ASSERT((r.x % minShadow) == 0 && (r.y % minShadow) == 0 && (r.width % minShadow) == 0 && r.width == r.height);
    r.x /= minShadow;
    r.y /= minShadow;
    r.width /= minShadow;
    r.height /= minShadow;
    atlas.freeUsedRect(r);
  }
  AtlasRect atlasInsert(uint32_t w)
  {
    AtlasRect r = atlas.insert(w / minShadow);
    G_ASSERT(r.x < (atlasWidth / minShadow) && r.y < (atlasHeight / minShadow));
    r.x *= minShadow;
    r.y *= minShadow;
    r.width *= minShadow;
    r.height *= minShadow;
    return r;
  }
  bool atlasIntersectsWithFree(rbp::Rect r, rbp::Rect &w) const
  {
    G_ASSERT(r.x % minShadow == 0 && r.y % minShadow == 0 && r.width % minShadow == 0 && r.width == r.height);
    r.x /= minShadow;
    r.y /= minShadow;
    r.width /= minShadow;
    r.height /= minShadow;
    w.x /= minShadow;
    w.y /= minShadow;
    w.width /= minShadow;
    w.height /= minShadow;
    bool ret = atlas.intersectsWithFree(r, w);
    w.x *= minShadow;
    w.y *= minShadow;
    w.width *= minShadow;
    w.height *= minShadow;
    return ret;
  }
  float atlasOccupancy() const { return atlas.occupancy((atlasWidth / minShadow) * (atlasHeight / minShadow)); }

  IPoint2 getOctahedralTempShadowExtent(const Volume &volume) const;

  // update on a need-now basis lru
  struct LRUEntry
  {
    uint16_t id, age;
    LRUEntry(uint16_t id_, uint16_t age_) : id(id_), age(age_) {}
  };
  struct LRUEntryCompare
  {
    bool operator()(const LRUEntry a, const LRUEntry b) const { return a.age > b.age; }
  };


  Tab<LRUEntry> lruList; // only valid for lruListFrame
  uint32_t lruListFrame = 0;
  void buildLRU();

  bool freeUnusedDynamicDataForStaticLights();
  uint32_t lastFrameStaticLightsOptimized = 0;
  bool degradeQuality();
  bool upgradeQuality();

  d3d::SamplerHandle shadowSampler;
};
