//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_texMgr.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>

#include <math/dag_color.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <util/dag_string.h>
#include <gameRes/dag_gameResources.h>
#include <shaders/dag_shaderMesh.h>
#include <render/toroidalHelper.h>
#include <render/toroidal_update.h>
#include <shaders/dag_overrideStateId.h>
#include <math/dag_hlsl_floatx.h>
#include "randomGrassParams.hlsli"
#include <render/atlasTexManager.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <EASTL/unique_ptr.h>
#include <webui/editVarHolder.h>
#include <webui/editVarPlugin.h>

#define GRID_SIZE              10
#define VB_GRID_SIZE           8
#define MAX_VERTICES_IN_CELL   65534
#define INSTANCES_IN_SINGLE_VB 100
#define MAX_COLOR_MASKS        6


class BaseTexture;
typedef BaseTexture Texture;
class DataBlock;
class RenderableInstanceLodsResource;
class LandMeshManager;
class LandMeshRenderer;
class PostFxRenderer;
class BBox2;
class Occlusion;
class LandMask;
class ComputeShaderElement;
class IEditableVariablesNotifications;
struct BVHConnection;

struct ColorRange
{
  Color4 start;
  Color4 end;
};

enum
{
  CHANNEL_RED = 0,
  CHANNEL_GREEN = 1,
  CHANNEL_BLUE = 2,
  CHANNEL_COUNT
};

struct GrassLayerInfo
{
  String resName; // SimpleString
  uint64_t bitMask;

  float crossBlend;
  float windMul;
  float radiusMul;

  Point2 minScale;
  Point2 maxScale;

  float density;
  ColorRange colors[CHANNEL_COUNT];

  bool resetLayerVB;
};

struct GrassLodCombined
{
  UniqueBuf lodVb;
  UniqueBuf lodIb;

  UniqueBuf grassInstancesIndirect; // draw count and startindex locations for indirect drawing
  UniqueBuf grassInstances;         // instance data

  UniqueBuf grassGenerateIndirect; // count of tread groups for dispatch_indirect
  UniqueBuf grassIndirectParams;   // dispatch params for indirect instance generation (xy size of thread group)
  UniqueBuf grassDispatchCount;    // count of thread groups for each layer
  UniqueBuf grassInstancesStride;  // offset for different layers in grassInstances buffer
  UniqueBuf grassLayersData;       // layer params for culling and indirect generation

  int vdecl = 0;
  int instancesCount = 0; // Total instances on this lod
};

struct GrassLod
{
  UniqueBuf cellVb;
  UniqueBuf cellIb;
  ShaderMesh *mesh;
  int vdecl;
  int numv;
  int numf;
  int numfperinst;
  int numvperinst;

  unsigned int numInstancesInCell;

  // as instance count in cell ing GPU grass is constant
  // we use different cell size dependant from density
  float gpuCellSize;
  int warpSize;

  float minRadius;
  float maxRadius;

  float fadeIn;
  float fadeOut;

  float maxInstanceSize;

  float density;
  bool singleVb;

  Tab<uint8_t> gridOpaqueIndices;
  Tab<uint8_t> gridTranspIndices;

  int maxInstancesCount = 0;
  int startIndexlocation = 0;

  int lodRndSeed;

  Texture *diffuseTex;
  Texture *alphaTex;
  d3d::SamplerHandle grassTexSmp;

  TEXTUREID diffuseTexId;
  TEXTUREID alphaTexId;


  GrassLod() : mesh(NULL), gridOpaqueIndices(midmem), gridTranspIndices(midmem) {}
};

struct GrassLayer
{
  GrassLayerInfo info;

  Tab<GrassLod> lods;
  RenderableInstanceLodsResource *resource;

  GrassLayer() : resource(NULL), lods(midmem) {}
};


struct IRandomGrassHelper
{
  virtual bool getHeightmapAtPoint(float x, float y, float &out) = 0;
  virtual bool isValid() const = 0;
};

class RandomGrass
{
public:
  RandomGrass(const DataBlock &level_grass_blk, const DataBlock &params_blk);
  ~RandomGrass();

  void loadParams(const DataBlock &level_grass_blk, const DataBlock &params_blk);
  void onReset();
  void replaceLayerVdecl(GrassLayer &layer);
  void replaceVdecls();

  void setWaterLevel(float v);

  float getCellSize() const { return cellSize; }

  void generateGPUGrass(const LandMask &land_mask, const Frustum &frustum, const Point3 &view_dir, TMatrix4 viewTm, TMatrix4 projTm,
    Point3 view_pos);
  void beforeRender(const Point3 &center_pos, IRandomGrassHelper &render_helper, const LandMask &land_mask,
    Occlusion *occlusion = NULL);

  void renderDepth(const LandMask &land_mask);
  void renderDepthOptPrepass(const LandMask &land_mask);
  void renderTrans(const LandMask &land_mask);
  void renderOpaque(const LandMask &land_mask);
  void renderDebug();

  void fillLayers(float grass_mul, const DataBlock &params_blk, float min_far_lod_radius = -1.f);
  BBox2 getRenderBbox() const;
  void resetLayersVB();
  void deleteBuffers();
  int addLayer(GrassLayerInfo &layer_info);
  void setLayerRes(GrassLayer *layer);
  GameResource *loadLayerResource(const char *resName);

#if DAGOR_DBGLEVEL > 0

  struct RandomGrassTypeDesc
  {
    E3DCOLOR colors[MAX_COLOR_MASKS];
  };
  Tab<RandomGrassTypeDesc> grassDescs;
  eastl::unique_ptr<IEditableVariablesNotifications> varNotification;


  void updateGrassColorLayer(Tab<carray<float4, MAX_COLOR_MASKS>> colors);
  void initWebTools();
#endif

  GrassLayer *getGrassLayerAt(int idx) { return layers[idx]; }
  int getGrassLayerCount() const { return layers.size(); }

  static void reset_mask_number();

  void setAlphaToCoverage(bool alpha_to_coverage);

  static void setBVHConnection(BVHConnection *bvh_connection) { bvhConnection = bvh_connection; }

  bool isDissolve;
  bool isColorDebug;

  float globalDensityMul;

protected:
  void resetLayer(GrassLayer &layer);
  void resetLayers();
  void createCombinedBuffers();
  void fillLayerLod(GrassLayer &layer, unsigned int lod_idx, int rnd_seed);
  void fillLodVB(GrassLayer &layer, int lod_idx);

  void beginRender(const LandMask &land_mask);
  void endRender();
  void draw(const LandMask &land_mask, bool opaque, int startLod, int endLod); // endLod = lastLodToRender+1
  void setLodStates(const GrassLod &lod);
  void closeTextures();

  UniqueBufHolder layerDataVS, layerDataColor;

  float grassRadiusMul;
  float grassDensityMul;
  bool useSorting;
  bool vboForMaxElements;

  float cellSize;
  unsigned int baseNumInstances;
  int maxLodCount;
  int maxLayerCount;

  Point3 centerPos;
  IPoint2 squareCenter;

  float curMaxRadius;
  float refMaxRadius;

  float curFadeDelta;
  float refFadeDelta;

  float minDensity;
  float maxDensity;


  float lodFadeDelta;
  float minFarLodRadius;

  float waterLevel;
  bool shouldRender;
  bool prepassIsValid = false;

  bool alphaToCoverage = false;

  Tab<uint8_t> gridIndices;
  Tab<GrassLayer *> layers;
  carray<BBox3, GRID_SIZE * GRID_SIZE> gridBoxes;
  carray<Point3, (GRID_SIZE + 1) * (GRID_SIZE + 1)> lmeshPoints;

  ComputeShaderElement *randomGrassGenerator;
  ComputeShaderElement *randomGrassIndirect;
  DynamicShaderHelper randomGrassShader;

  bool grassBufferGenerated;

  Tab<GrassLodCombined> combinedLods;

  Tab<int> shaderVars;
  shaders::UniqueOverrideStateId afterPrepassOverride;

  static BVHConnection *bvhConnection;
};
