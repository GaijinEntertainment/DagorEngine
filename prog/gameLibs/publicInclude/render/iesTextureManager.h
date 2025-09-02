//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_texMgr.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_string.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/integer/dag_IPoint3.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>

#include <math/dag_hlsl_floatx.h>

#include <render/ies_generator_shared.hlsli>


class IesEditor
{
public:
  IesEditor();

  void renderIesTexture();
  void clearControlPoints();
  void removeControlPoint(int index);
  void recalcCoefficients();
  int addPointWithUpdate(float theta, float intensity);
  int addPointWithoutUpdate(float theta, float intensity);
  float getLightIntensityAt(float theta) const;

  int getNumControlPoints() const { return sortedPhotometryControlPoints.size(); }
  const PhotometryControlPoint *getPhotometryControlPoints() const { return sortedPhotometryControlPoints.data(); }
  PhotometryControlPoint *getPhotometryControlPoints() { return sortedPhotometryControlPoints.data(); }

private:
  PostFxRenderer iesGenerator;
  IPoint3 currentResolution = IPoint3(0, 0, 0);
  UniqueTex dynamicIesTexArray;
  UniqueBufHolder sortedPhotometryControlPointsBuf;
  eastl::string originalTextureName;
  eastl::vector<PhotometryControlPoint> sortedPhotometryControlPoints;
  bool needsRender = true;

  static IPoint3 getTexResolution(BaseTexture *photometry_tex_array);

  void ensureTextureCreated(BaseTexture *photometry_tex_array);
};

class IesTextureCollection
{
public:
  static const char *EDITOR_TEXTURE_NAME;

  struct PhotometryData
  {
    float zoom = 1;
    bool rotated = false;
  };

  static IesTextureCollection *getSingleton();
  static IesTextureCollection *acquireRef();
  static void releaseRef();

  explicit IesTextureCollection(eastl::vector<String> &&textures);
  ~IesTextureCollection();

  IesTextureCollection(const IesTextureCollection &) = delete;
  IesTextureCollection &operator=(const IesTextureCollection &) = delete;

  // move semantics implicitly deleted
  // no need for its implementation

  int getTextureIdx(const char *name);
  void gatherAndReloadTextures();
  void reloadTextures();
  void addTexture(const char *textureName);
  void close();
  PhotometryData getTextureData(int tex_idx) const;
  TEXTUREID getTextureArrayId();

  IesEditor *requireEditor();

private:
  static IesTextureCollection *instance;
  eastl::unique_ptr<IesEditor> editor;
  uint32_t refCount = 0;

  eastl::vector<String> usedTextures;
  eastl::vector<PhotometryData> photometryData;
  TEXTUREID photometryTexId = BAD_TEXTUREID;
  FastNameMap textureNames;
};
