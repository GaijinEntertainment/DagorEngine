//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <util/dag_baseDef.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_eventQueryHolder.h>
#include <util/dag_stdint.h>
#include <generic/dag_tab.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/toroidalHelper.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <shaders/dag_overrideStateId.h>
#include <math/dag_hlsl_floatx.h>
#include <webui/editVarHolder.h>
#include "grassInstance.hlsli"
#include <generic/dag_fixedMoveOnlyFunction.h>

class ComputeShaderElement;
class Sbuffer;
class IEditableVariablesNotifications;

struct IRandomGrassRenderHelper
{
  virtual bool beginRender(const Point3 &center_pos, const BBox3 &box, const TMatrix4 &glob_tm, const TMatrix4 &proj_tm) = 0;
  virtual void endRender() = 0;
  // virtual void renderHeight( float min_height, float max_height ) = 0;
  virtual void renderColor() = 0;
  virtual void renderMask() = 0;
  virtual bool getMinMaxHt(const BBox2 & /*box*/, float & /*min_ht*/, float & /*max_ht*/)
  {
    return false;
  } // min should be update to min, and max to max
  // virtual void renderExplosions() = 0;
  // virtual bool getHeightmapAtPoint( float x, float y, float &out ) = 0;
};

enum GrassQuality
{
  GRASS_QUALITY_MINIMUM,
  GRASS_QUALITY_LOW,
  GRASS_QUALITY_HIGH
};
class GPUGrassBase
{
public:
  GPUGrassBase();

  ~GPUGrassBase();
  void close();
  void init(const DataBlock &grass_settings);
  bool setQuality(GrassQuality quality);
  void applyAnisotropy();
  typedef eastl::function<bool(const BBox2 &, float &min_ht, float &max_ht)> frustum_heights_cb_t;
  using RenderGrassLodCallback =
    dag::FixedMoveOnlyFunction<64, void(const float currentGridSize, const float currentGrassDistance) const>;

  void renderGrassLods(RenderGrassLodCallback renderGrassLod) const;
  void generate(const Point3 &pos, const Point3 &view_dir, const frustum_heights_cb_t &cb);

  enum RenderType
  {
    GRASS_NO_PREPASS = 0,
    GRASS_DEPTH_PREPASS = 1,
    GRASS_AFTER_PREPASS = 2
  };
  void render(RenderType rtype);

  IEditableVariablesNotifications *varNotification = 0;

  void driverReset();

  float getGrassDistance() const { return grassDistance; }
  float getMaxGrassHeight() const { return maxGrassHeightSize; }
  float getMaxGrassHorSize() const { return maxGrassHorSize; }
  float getDistanceEffective() const;
  float getGridSizeEffective() const;
  float getDistanceWithBorder() const;
  float alignTo() const;
  bool isInited() const { return isInitialized; }

  void invalidate();

protected:
  void loadGrassTypes(const DataBlock &grassSettings, const eastl::hash_map<eastl::string, int> &grassTypesUsed);
  void updateGrassColors();
  void updateGrassTypes();
  void generateGrass(const Point2 &next_pos, const Point3 &view_dir, float min_ht, float max_ht, const frustum_heights_cb_t &cb);

  eastl::unique_ptr<ComputeShaderElement> createIndirect;
  UniqueBuf grassInstancesIndirect;
  UniqueBufHolder grassInstances;
  UniqueBufHolder grassInstancesCountRB;
  EventQueryHolder readbackQuery;

  UniqueBufHolder grassRandomTypesCB, grassColorsVSCB;

  eastl::unique_ptr<ComputeShaderElement> grassGenerator;
  // PostFxRenderer grassGenerator;
  DynamicShaderHelper grassRenderer;

  float grassGridSize = 1, grassDistance = 1;
  int maxInstanceCount = 0;
  int allocatedInstances = 0;
  bool generated = false;
  bool readbackQueryIssued = false;
  bool isInitialized = false;
#if _TARGET_IOS || _TARGET_C3
  static constexpr int MAX_GRASS = 100000;
#else
  static constexpr int MAX_GRASS = 1000000;
#endif
  GrassQuality grassQuality = GRASS_QUALITY_HIGH;
  float hor_size_mul = 1.0f;
  float height_mul = 1.0f;
  struct GrassTypeDesc
  {
    GrassTypeDesc() = default;
    GrassTypeDesc(const GrassTypeDesc &) = delete;
    GrassTypeDesc(GrassTypeDesc &&) { G_ASSERT(false); }
    GrassTypeDesc &operator=(const GrassTypeDesc &) = delete;
    GrassTypeDesc &operator=(GrassTypeDesc &&) = delete;

    eastl::string name;
    bool isValid = false;
    bool hasTexture = false;
    bool isHorizontal = false;
    bool isUnderwater = false;

    EditableFloatHolderSlider height = 0;
    EditableFloatHolderSlider size_lod_mul = 1.3f;
    EditableFloatHolderSlider size_lod_wide_mul = 1.f;
    EditableFloatHolderSlider ht_rnd_add = 0;
    EditableFloatHolderSlider hor_size = 0;
    EditableFloatHolderSlider hor_size_rnd_add = 0;
    EditableFloatHolderSlider height_from_weight_mul = 0.75f;
    EditableFloatHolderSlider height_from_weight_add = 0.25f;
    EditableFloatHolderSlider density_from_weight_mul = 0.999f;
    EditableFloatHolderSlider density_from_weight_add = 0;
    EditableFloatHolderSlider vertical_angle_add = -0.1f;
    EditableFloatHolderSlider vertical_angle_mul = 0.2f;
    EditableFloatHolderSlider tile_tc_x = 1.f;
    EditableFloatHolderSlider stiffness = 1.0f;
    EditableIntHolderSlider isHorizontalValue = 0;
    EditableIntHolderSlider isUnderwaterValue = 0;
    EditableE3dcolorHolderEditbox colors[6];
  };
  eastl::vector<GrassTypeDesc> grassDescriptions;
  SharedTexHolder grassTex, grassNTex, grassAlphaTex;
  d3d::SamplerInfo grassColorAlphaTexSampler;

  Tab<GrassChannel> grassChannels;
  Tab<GrassType> grasses;
  shaders::UniqueOverrideStateId grassAfterPrepassOverride;
  Tab<GrassColorVS> grassColors; // don't move this Tab in memory
  float maxGrassHeightSize = 0;
  float maxGrassHorSize = 0;
  bool indexBufferInited = false;
  bool grassIsSorted = false;
  bool alreadyLoaded = false;
  bool extraGrassInstances = false;
  // Color4 color_mask_r_from, color_mask_r_to, color_mask_g_from, color_mask_g_to, color_mask_b_from, color_mask_b_to;

  // TextureIDHolder grass;
  // ComputeShaderElement *cse;
};

struct MaskRenderCallback
{
  ViewProjMatrixContainer originalViewproj;
  TMatrix viewTm;

  float texelSize;
  IRandomGrassRenderHelper &cm;
  Texture *maskTex, *colorTex;
  PostFxRenderer *copy_grass_decals = 0;
  MaskRenderCallback(float texel, IRandomGrassRenderHelper &cb, Texture *m, Texture *c, PostFxRenderer *cgd) :
    texelSize(texel), cm(cb), maskTex(m), colorTex(c), copy_grass_decals(cgd)
  {
    viewTm.setcol(0, 1, 0, 0);
    viewTm.setcol(1, 0, 0, 1);
    viewTm.setcol(2, 0, 1, 0);
    viewTm.setcol(3, 0, 0, 0);
  }

  void start(const IPoint2 &);
  void renderQuad(const IPoint2 &lt, const IPoint2 &wd, const IPoint2 &texelsFrom);
  void end();
};


class GPUGrass
{
public:
  GPUGrassBase base;
  GPUGrass();

  ~GPUGrass();
  void close();
  void init(const DataBlock &grass_settings);
  void setQuality(GrassQuality quality);
  void applyAnisotropy();

  void generate(const Point3 &pos, const Point3 &view_dir, IRandomGrassRenderHelper &cb);

  enum RenderType
  {
    GRASS_NO_PREPASS = GPUGrassBase::GRASS_NO_PREPASS,
    GRASS_DEPTH_PREPASS = GPUGrassBase::GRASS_DEPTH_PREPASS,
    GRASS_AFTER_PREPASS = GPUGrassBase::GRASS_AFTER_PREPASS
  };
  void render(RenderType rtype);

  void driverReset();

  Texture *getMaskTex() const { return maskTex.getTex(); }
  float getGrassDistance() const { return base.getGrassDistance(); }
  int getMaskResolution() const { return maskResolution; }

  void invalidate(bool regenerate = true);
  void invalidateBoxes(const dag::ConstSpan<BBox2> &boxes);
  void invalidateBoxes(const dag::ConstSpan<BBox3> &boxes);

protected:
  void generateGrass(const Point2 &next_pos, const Point3 &view_dir, float min_ht, float max_ht, IRandomGrassRenderHelper &cb);

  PostFxRenderer copy_grass_decals;
  TextureIDHolder noiseTex;

  SharedTexHolder masksTex, typesTex; // both are for decals

  TextureIDHolderWithVar maskTex, colorTex;
  ToroidalHelper maskTexHelper;
  shaders::UniqueOverrideStateId flipCullStateId;
  eastl::vector<BBox2> boxesForInvalidation;
  Point2 lastGenPos = {100000, -10000};
  int maskResolution = 512;
};
GrassQuality str_to_grass_quality(const char *quality);
