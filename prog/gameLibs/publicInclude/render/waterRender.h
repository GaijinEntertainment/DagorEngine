//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_DObject.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_plane3.h>
#include <math/dag_Point2.h>
#include <math/dag_TMatrix4.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_resPtr.h>
class ShaderMaterial;
class DataBlock;

#define WAVE_LAYERS 4 // Multiple modifications required to change this value.

class WaterRender
{
public:
  enum TexType
  {
    TEX_REFLECT = 0,
    _NUM_TEX
  };

  WaterRender();
  ~WaterRender();
  void init(const DataBlock &settings, bool occlusion_query_enabled);
  void close();

  void onBeforeReset3dDevice();
  void onPreWaterRender(const Plane3 &plane);
  void onPostWaterRender();
  // void renderTextures(IRenderWorld &world);
  void setGlobalVariables();
  void setShaderVariables(ShaderMaterial *material);

  bool startRenderingTextures();
  void endRenderingTextures();
  void prepareRenderReflection();

  static void calcReflectionView(TMatrix4 &out_view, const Plane3 &plane, const TMatrix4 &view)
  {
    TMatrix4 reflectionMatrix = matrix_reflect(plane);
    TMatrix4 flipMatrix = matrix_reflect(Plane3(Point3(0, 1, 0), 0.f));
    TMatrix4 reflectedView = reflectionMatrix * view;
    out_view = reflectedView * flipMatrix;
  }

protected:
  void setTexture(TexType tex_type, TEXTUREID tex_id);
  Texture *createWaterRTTex(TexType tex_type, int color_format);

  struct RenderCtx
  {
    Driver3dRenderTarget prevRendertarget;

    TMatrix4 viewMatrix, projMatrix;
    int viewportX, viewportY, viewportW, viewportH;
    float viewportMinZ, viewportMaxZ;
  } ctx;
  Plane3 reflectionPlane;
  void freeTextures();

  // Settings.

  Point2 layerSize[WAVE_LAYERS];
  Point2 layerSpeed[WAVE_LAYERS];

  Point2 waveSizeArray[WAVE_LAYERS];
  Point2 waveSpeedArray[WAVE_LAYERS];
  float waveDirArray[WAVE_LAYERS];

  Point2 foamSize;
  Point2 foamSpeed;
  float foamDir;
  float foamParallaxScale;

  Point2 amplitudeSize;
  Point2 amplitudeSpeed;
  float amplitudeDir;

  float enviBumpScale;
  float foamVisibilityThreshold;

  Point2 reflectionBumpScale;

  float objectToSceenRatio;
  float nearRatioOffset;
  float visibilityRangeMultiplier;

  bool isRefractionEnabled;
  bool occlusionQueryEnabled;


  Texture *hdrTex[_NUM_TEX];
  TEXTUREID hdrTexId[_NUM_TEX];
  UniqueTex depthTex;

  int reflectionTextureVarId;
  int reflectionPlaneVarId;
  int waveScaleOffsetVarIdArray[WAVE_LAYERS];

  int waveDirVarIdArray[WAVE_LAYERS];
  Point4 waveDirVectorsArray[WAVE_LAYERS];
  int waveOffsetVarIdArray[WAVE_LAYERS];

  int foamDirVarId;
  Point4 foamDirVectors;
  int foamOffsetVarId;
  int foamParallaxScaleVarId;

  int rendering_water_varId;

  int amplitudeDirVarId;
  Point4 amplitudeDirVectors;
  int amplitudeOffsetVarId;

  int normalToOffsetXVarId;
  int normalToOffsetYVarId;
  int enviBumpScaleVarId;
  int foamVisibilityThresholdVarId;
  int waterQualityVarId;

  bool isRenderingTextures_;
  bool isWaterVisible;

  Ptr<ShaderMaterial> waterMaterial;
  float waterLevel;

  void *occlusionQuery;
  bool isOcclusionQueryIssued;
  int prevPixelCount;
};
