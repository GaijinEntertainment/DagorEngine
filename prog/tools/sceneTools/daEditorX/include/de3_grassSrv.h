//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EditorCore/ec_interface.h>
#include <oldEditor/de_common_interface.h>

#include <math/dag_Point3.h>
#include <render/editorGrass.h>

class LandMeshRenderer;
class IGrassService : public IRenderingService
{
public:
  static constexpr unsigned HUID = 0x7B96305Bu; // IGrassService

  virtual void init() = 0;
  virtual void create_grass(DataBlock &grassBlk) = 0;
  virtual DataBlock *create_default_grass() = 0;
  virtual void enableGrass(bool flag) = 0;

  virtual void beforeRender(Stage stage) = 0;
  virtual void renderGeometry(Stage stage) = 0;
  virtual BBox3 *getGrassBbox() = 0;

  virtual int addDefaultLayer() = 0;
  virtual bool removeLayer(int layer_i) = 0;
  virtual void reloadAll(const DataBlock &grass_blk, const DataBlock &params_blk) = 0;

  virtual int getGrassLayersCount() = 0;
  virtual GrassLayerInfo *getLayerInfo(int layer_i) = 0;

  virtual bool changeLayerResource(int layer_i, const char *resName) = 0;
  virtual void setLayerDensity(int layer_i, float density) = 0;
  virtual void updateLayerVbo(int layer_i) = 0;
  virtual void forceUpdate() = 0;
  virtual const char *getResName(TEXTUREID id) const = 0;
  virtual void resetGrassMask(LandMeshRenderer &landMeshRenderer, int index, const DataBlock &grassBlk, const String &colorMapId,
    const String &maskName) const = 0;
  virtual void resetLayersVB() = 0;
};
