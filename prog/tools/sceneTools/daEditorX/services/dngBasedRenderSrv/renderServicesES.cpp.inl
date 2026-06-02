// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_camera_elem.h>
#include <de3_dynRenderService.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/updateStageRender.h>
#include <shaders/dag_shaderBlock.h>
#include "render/renderEvent.h"

extern bool dynmodel_mgr_has_active_entities();

static ShaderVariableInfo forceInEditorVarInfo("force_in_editor");

namespace dng_based_render
{
Tab<IRenderingService *> rendSrv(tmpmem);

struct RenderServiceBlock
{
  int prevF, prevS, prevO;
  bool prevAutoChange;

  RenderServiceBlock()
  {
    forceInEditorVarInfo.set_int(1);
    prevF = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    prevS = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
    prevO = ShaderGlobal::getBlock(ShaderGlobal::LAYER_OBJECT);
    prevAutoChange = ShaderGlobal::enableAutoBlockChange(true);
  }
  ~RenderServiceBlock()
  {
    ShaderGlobal::enableAutoBlockChange(prevAutoChange);
    ShaderGlobal::setBlock(prevF, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(prevS, ShaderGlobal::LAYER_SCENE);
    ShaderGlobal::setBlock(prevO, ShaderGlobal::LAYER_OBJECT);
    forceInEditorVarInfo.set_int(0);
  }
};
}; // namespace dng_based_render
using dng_based_render::RenderServiceBlock;
using dng_based_render::rendSrv;


ECS_TAG(render)
static void render_services_opaque_es(const UpdateStageInfoRender &)
{
  TIME_D3D_PROFILE(render_services_opaque);
  RenderServiceBlock renderBlk;
  for (auto *srv : rendSrv)
    srv->renderGeometry(IRenderingService::STG_RENDER_STATIC_OPAQUE);
  for (auto *srv : rendSrv)
    srv->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_OPAQUE);
}

ECS_TAG(render)
static void render_services_transp_es(const UpdateStageInfoRenderTrans &)
{
  TIME_D3D_PROFILE(render_services_transp);
  RenderServiceBlock renderBlk;
  for (auto *srv : rendSrv)
    srv->renderGeometry(IRenderingService::STG_RENDER_STATIC_TRANS);
  for (auto *srv : rendSrv)
    srv->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_TRANS);
}

ECS_TAG(render)
ECS_NO_ORDER
static void render_services_has_any_visible_distortion_es(UpdateStageInfoNeedDistortion &e)
{
  e.needed = e.needed || dynmodel_mgr_has_active_entities();
}

ECS_TAG(render)
static void render_services_distortion_es(const UpdateStageInfoRenderDistortion &)
{
  TIME_D3D_PROFILE(render_services_distortion);
  RenderServiceBlock renderBlk;
  for (auto *srv : rendSrv)
    srv->renderGeometry(IRenderingService::STG_RENDER_STATIC_DISTORTION);
  for (auto *srv : rendSrv)
    srv->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_DISTORTION);
}

ECS_NO_ORDER
ECS_TAG(render)
static void render_services_debug_es(const UpdateStageInfoRenderDebug &)
{
  TIME_D3D_PROFILE(render_services_debug);
  if (IDynRenderService *rs = EDITORCORE->queryEditorInterface<IDynRenderService>())
    if (rs->getRenderType() != rs->RTYPE_DNG_BASED)
      return;
  RenderServiceBlock renderBlk;
  IEditorCoreEngine::get()->renderObjects();
  IEditorCoreEngine::get()->renderTransObjects();

  ec_camera_elem::freeCameraElem->render();
  ec_camera_elem::maxCameraElem->render();
  ec_camera_elem::fpsCameraElem->render();
  ec_camera_elem::tpsCameraElem->render();
  ec_camera_elem::carCameraElem->render();
}
