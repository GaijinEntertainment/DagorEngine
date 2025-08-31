// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <streaming/dag_streamingBase.h>
#include <render/dag_cur_view.h>
#include <scene/dag_frtdumpInline.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_asyncRead.h>
#include <3d/dag_picMgr.h>
#include <render/fx/dag_demonPostFx.h>
#include <math/dag_bounds2.h>
#include <math/dag_bezier.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_overrideStateId.h>
#include <osApiWrappers/dag_critSec.h>
#include <landMesh/lmeshManager.h>
#include <landMesh/landRayTracer.h>
#include <util/dag_oaHashNameMap.h>
#include <stdlib.h>
#include "gridRender.h"
#include <de3_collisionPreview.h>
#include <3d/dag_resPtr.h>

class DataBlock;
class LandMeshRenderer;
class RendInstShadowsManager;
class DynamicRenderableSceneInstance;
class AcesResource;
class Weather;
struct ObjectsToPlace;
class Clipmap;
class ToroidalHeightmap;
class IWaterService;


struct BBoxTreeElement
{
  static const int WIDTH = 16;
  BBox3 bbox;
  unsigned int indices[WIDTH];
};


class AcesScene final : public BaseStreamingSceneHolder
{
public:
  AcesScene();
  ~AcesScene() override;

  void setEnvironmentSettings(DataBlock &enviBlk);

  void loadLevel(const char *fname);
  void clear();

  bool bdlCustomLoad(unsigned bindump_id, int tag, IGenLoad &crd, dag::ConstSpan<TEXTUREID> texId) override;

  void delBinDump(unsigned bindump_id) override;

  void update(const Point3 &observer, float dt);
  void beforeRender();
  void render(bool render_rs);
  void renderTrans(bool render_rs);
  void renderWater(IRenderingService::Stage stage);
  void renderSplines();
  void renderSplinePoints(float max_dist);
  void renderShadows();
  void renderShadowsVsm();
  void renderHeight();

  void setZTransformPersp();
  void setZTransformPersp(float zn, float zf);
  void renderClipmaps();
  void invalidateClipmap(bool force_redraw);

  void prepareFixedClip(int texture_size);
  void setFixedClipToShader();
  void closeFixedClip();

  void setupShaderVars();

  bool getLandscapeVis();
  void setLandscapeVis(bool vis);

  bool getLandscapeMirroring() const { return useLandMirroring; }
  void setLandscapeMirroring(bool mirror) { useLandMirroring = mirror; }
  void setupMirroring();

  bool tracerayNormalizedStaticScene(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm);

  bool tracerayNormalizedHeightmap(const Point3 &p0, const Point3 &dir, float &t, Point3 *norm);
  bool getHeightmapAtPoint(float x, float z, float &out_height, Point3 *out_normal);
  bool isInited() { return intited; }

  LandMeshManager *lmeshMgr;
  FastRtDump frtDump;
  collisionpreview::Collision lrtCollision;
  bool skipEnviData;
  bool pendingBuildDf;
  IWaterService *waterService;

protected:
  LandMeshRenderer *lmeshRenderer;
  float cameraHeight;
  Clipmap *clipmap;
  ToroidalHeightmap *toroidalHeightmap;
  static AcesScene *self;
  shaders::UniqueOverrideStateId flipCullStateId;
  shaders::UniqueOverrideStateId blendOpMaxStateId;
  shaders::UniqueOverrideStateId zFuncLessStateId;

  static const int NUM_WATER_NORMAL_MAPS = 2;

  float dynamicZNear;
  float dynamicZFar;

  bool hasWater;
  bool useLandMirroring = true;

  TEXTUREID levelMapTexId;

  float skySphereFogDistInitial;
  float skySphereFogHeightInitial;
  float skySphereFogDist;
  float skySphereFogHeight;
  float waterLevel;

  Color3 skyColor, skyColorInitial;
  Point3 sunDir0, sunDir0Initial;
  Color3 sunColor0, sunColor0Initial;
  Point3 sunDir1, sunDir1Initial;
  Color3 sunColor1, sunColor1Initial;

  int landMeshOffset;

  Point2 zRange;

  DataBlock *hmapEnviBlk;

  TEXTUREID lightmapTexId;

  BinaryDump *bindumpHandle;

  Tab<ObjectsToPlace *> objectsToPlace;


  bool loadingAllRequiredResources;

  DataBlock levelSettingsBlk;

  FastNameMap requiredResources;
  bool intited;

  UniqueTexHolder last_clip;
  d3d::SamplerInfo last_clip_sampler;
  int lastClipTexSz;
  bool rebuildLastClip;

  void placeResObjects();

  struct SplineData
  {
    SimpleString name;
    BezierSpline3d spl;
  };
  Tab<SplineData> splines;

  void renderSplineCurve(const BezierSpline3d &spl, bool opaque);

  static bool __stdcall custom_get_height(Point3 &p, Point3 *n);
  static vec3f __stdcall custom_update_pregen_pos_y(vec4f pos, int16_t *dest_packed_y, float csz_y, float oy);
};
