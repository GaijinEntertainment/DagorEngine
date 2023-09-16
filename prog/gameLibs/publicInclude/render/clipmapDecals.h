//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_globDef.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <shaders/dag_shaders.h>
#include <math/dag_Point4.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <util/dag_simpleString.h>
#include <math/dag_bounds2.h>
#include <3d/dag_resPtr.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/string.h>

struct Frustum;
class DataBlock;

class ClipmapDecals
{
protected:
  enum
  {
    MAX_DECALS_PER_TYPE = 10000,
  };

  struct DisplacementMask
  {
    uint32_t lowBits;
    uint32_t highBits;
  };

  struct ClipmapDecal
  {
    Point2 localX;
    Point2 localY;
    Point2 pos;
    Point4 tc;
    float lifetime = -1;
  };

  struct ClipmapDecalSubType
  {
    Point4 tc;
    int variants;
  };

  struct ClipmapDecalType
  {
    TEXTUREID dtex = BAD_TEXTUREID;
    TEXTUREID ntex = BAD_TEXTUREID;
    TEXTUREID mtex = BAD_TEXTUREID;
    int newDecal = 0;
    int activeCount = 0;
    Tab<ClipmapDecalSubType> subtypes;
    Tab<ClipmapDecal> decals;
    DynamicShaderHelper material;
    UniqueBuf decalDataVS;

    // store texnames for array recreation
    Tab<String> diffuseList, normalList;

    SharedTex diffuseArray, normalArray;
    SharedTex diffuseTex, normalTex, materialTex;
    Tab<DisplacementMask> displacementMasks;
    Tab<uint8_t> displacementMaskSizes;
    UniqueBuf displacementMaskBuf;

    bool arrayTextures;
    bool needUpdate;

    // shader parameters
    float alphaThreshold = 1.0;
    float displacementMin = 0.0;
    float displacementMax = 0.0;
    float displacementScale = 1.0;
    float displacementFalloffRadius = -1.0;
    float reflectance = 0.5;
    float ao = 1.0;
    float microdetail = 0.0;
    float sizeScale = 1.0;

    ClipmapDecalType(int decals_count)
    {
      decals.resize(decals_count);
      mem_set_0(decals);
      arrayTextures = false;
      needUpdate = false;
    }

    shaders::UniqueOverrideStateId decalOverride;
  };

private:
  SimpleString defaultMaterial;
  Tab<ClipmapDecalType *> decalTypes;
  ska::flat_hash_map<eastl::string, int> decalTypesByName;
  int typesCount = 0;
  bool stubMode = false;
  bool isRenderingSetup = false;

  int alphaHeightScaleVarId = -1;
  int decalParametersVarId = -1;
  int useDisplacementMaskVarId = -1;
  int displacementMaskVarId = -1;
  int displacementFalloffRadiusVarId = -1;

  struct InstData
  {
    Point2 localX;
    Point2 localY;
    Point4 tc;
    Point4 pos;
  };
  carray<InstData, 100> instData;

  bool inited = false;

  enum
  {
    RENDER_TO_CLIPMAP = 0,
    RENDER_TO_DISPLACEMENT = 1,
    RENDER_TO_GRASS_MASK = 2
  };

  void renderByCount(int count);
  void updateBuffers(int type_no);

  int maxDelayedRegionsCount = 0;
  float delayedRegionSizeFactor = 5;
  bool useDelayedRegions = false;
  Point3 cameraPosition;

public:
  ClipmapDecals();
  ~ClipmapDecals();
  // init system. load settings from blk-file
  void init(bool stub_render_mode, const char *shader_name);

  // release system
  void release();

  // remove all
  void clear();

  void setDelayedRegionsParams(bool use_delayed_regions, int max_count, float size_factor)
  {
    maxDelayedRegionsCount = max_count;
    delayedRegionSizeFactor = size_factor;
    useDelayedRegions = use_delayed_regions;
  }

  void setCameraPosition(Point3 cam_pos) { cameraPosition = cam_pos; }

  // render decals
  void render(bool override_writemask);

  void afterReset();

  void setupRendering();
  bool checkDecalTexturesLoaded();

  void initDecalTypes(const DataBlock &blk);
  int getDecalTypeIdByName(const char *decal_type_name);
  int createDecalType(TEXTUREID d_tex_id, TEXTUREID n_tex_id, TEXTUREID m_tex_id, int decals_count, const char *shader_name,
    bool use_array_textures, int existing_decal_type = -1);
  int createDecalSubType(int decal_type, Point4 tc, int random_count, const char *diffuse_name, const char *normal_name,
    const char *material_name);

  void createDecal(int decalType, const Point2 &pos, float rot, const Point2 &size, int subtype, bool mirror,
    uint64_t displacement_mask, int displacement_mask_size);

  Tab<BBox2> updated_regions;
  Tab<BBox2> delayed_regions;

  const Tab<BBox2> &get_updated_regions();
  void clear_updated_regions();
};

namespace clipmap_decals_mgr
{
void createDecal(int decalType, const Point2 &pos, float rot, const Point2 &size, int subtype = 0, bool mirror = false,
  uint64_t displacement_mask = 0, int displacement_mask_size = 0);

// init system
void init();

// release system
void release();

// remove all decals from map
void clear();

void after_reset();

bool check_decal_textures_loaded();

int getDecalTypeIdByName(const char *decal_type_name);
int createDecalType(TEXTUREID d_tex_id, TEXTUREID n_tex_id, TEXTUREID m_tex_id = BAD_TEXTUREID, int decals_count = 1000,
  const char *shader_name = NULL, bool use_array = false);
int createDecalSubType(int decal_type, Point4 tc, int random_count, const char *diffuse_name = NULL, const char *normal_name = NULL,
  const char *material_name = NULL);

void render(bool override_writemask);

void setup_rendering();

const Tab<BBox2> &get_updated_regions();
void clear_updated_regions();

void set_delayed_regions_params(bool use_delayed_regions, int max_count, float size_factor);
void set_camera_position(Point3 pos);
} // namespace clipmap_decals_mgr
