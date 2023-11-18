// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#pragma once

/** \addtogroup de3Common
  @{
*/

#include <3d/dag_texMgr.h>
#include <workCycle/dag_gameSceneRenderer.h>
#include <render/variance.h>
#include <render/renderType.h>
#include <render/waterRender.h>
#include <render/waterObjects.h>
#include <render/rain.h>
#include <fx/dag_leavesWind.h>
#include "de3_envi.h"
#include <shaders/dag_overrideStateId.h>

class BaseTexture;
typedef BaseTexture Texture;
class DataBlock;
class PostFx;

/**
World renderer class.
*/

class WorldRenderer : public IDagorGameSceneRenderer
{
public:
  class IWorld
  {
  public:
    virtual void beforeRender() = 0;
    virtual void renderGeomEnvi() = 0;
    virtual void renderGeomOpaque() = 0;
    virtual void renderGeomTrans() = 0;
    virtual void renderGeomForShadows(int cascade_id) = 0;
    virtual void renderUi() = 0;
  };

  /**
  \brief Constructor

  \param[in] render_world to render.
  \param[in] gameParamsBlk data block with game parameters.
  */
  WorldRenderer(IWorld *render_world, const DataBlock &gameParamsBlk);
  /**
  \brief Destructor
  */
  ~WorldRenderer() { close(); }

  /**
  \brief Draws game scene.

  \param[in] scn game scene to draw. See #DagorGameScene.

  @see DagorGameScene
  */
  virtual void render(DagorGameScene &scn);

  void restartPostfx(DataBlock *postfxBlkTemp);

  void update(float dt);

public:
  SceneEnviSettings envSetts;

  WaterRender reflectionRender, waterRender;
  RainDroplets droplets;
  LeavesWindEffect windEffect;

protected:
  DataBlock *gameParamsBlk;
  IWorld *renderWorld;
  PostFx *postFx;
  int targetW, targetH, sceneFmt;
  Texture *sceneRt, *postfxRt;
  TEXTUREID sceneRtId, postfxRtId;
  Texture *sceneDepth;
  bool allowDynamicRender, useDynamicRender;

  void close();

  void prepareWaterReflection();
  void beforeRender();
  void classicRender();
  void classicRenderTrans();

  void renderGeomOpaque();
  void renderGeomEnvi();

protected:
  static bool renderWater, renderShadow, renderShadowVsm, renderEnvironmentFirst, enviReqSceneBlk;
  static int shadowQuality;
  shaders::UniqueOverrideStateId colorOnlyOverride;
};
/** @} */
