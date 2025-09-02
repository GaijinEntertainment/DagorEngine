// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_camera_elem.h>
#include <de3_dynRenderService.h>
#include <ecs/core/entityManager.h>
#include <ecs/render/updateStageRender.h>
#include <shaders/dag_shaderBlock.h>

namespace dng_based_render
{
Tab<IRenderingService *> rendSrv(tmpmem);

struct PreserveShaderBlocksAndAllowAutoChange
{
  int prevF, prevS, prevO;
  bool prevAutoChange;

  PreserveShaderBlocksAndAllowAutoChange()
  {
    prevF = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    prevS = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
    prevO = ShaderGlobal::getBlock(ShaderGlobal::LAYER_OBJECT);
    prevAutoChange = ShaderGlobal::enableAutoBlockChange(true);
  }
  ~PreserveShaderBlocksAndAllowAutoChange()
  {
    ShaderGlobal::enableAutoBlockChange(prevAutoChange);
    ShaderGlobal::setBlock(prevF, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(prevS, ShaderGlobal::LAYER_SCENE);
    ShaderGlobal::setBlock(prevO, ShaderGlobal::LAYER_OBJECT);
  }
};
}; // namespace dng_based_render
using dng_based_render::PreserveShaderBlocksAndAllowAutoChange;
using dng_based_render::rendSrv;


ECS_TAG(render)
static void render_services_opaque_es(const UpdateStageInfoRender &)
{
  PreserveShaderBlocksAndAllowAutoChange shBlkPreserve;
  for (auto *srv : rendSrv)
    srv->renderGeometry(IRenderingService::STG_RENDER_STATIC_OPAQUE);
  for (auto *srv : rendSrv)
    srv->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_OPAQUE);
}

ECS_TAG(render)
static void render_services_transp_es(const UpdateStageInfoRenderTrans &)
{
  PreserveShaderBlocksAndAllowAutoChange shBlkPreserve;
  for (auto *srv : rendSrv)
    srv->renderGeometry(IRenderingService::STG_RENDER_STATIC_TRANS);
  for (auto *srv : rendSrv)
    srv->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_TRANS);
}

ECS_NO_ORDER
ECS_TAG(render)
static void render_services_debug_es(const UpdateStageInfoRenderDebug &)
{
  if (IDynRenderService *rs = EDITORCORE->queryEditorInterface<IDynRenderService>())
    if (rs->getRenderType() != rs->RTYPE_DNG_BASED)
      return;
  PreserveShaderBlocksAndAllowAutoChange shBlkPreserve;
  IEditorCoreEngine::get()->renderObjects();
  IEditorCoreEngine::get()->renderTransObjects();

  ec_camera_elem::freeCameraElem->render();
  ec_camera_elem::maxCameraElem->render();
  ec_camera_elem::fpsCameraElem->render();
  ec_camera_elem::tpsCameraElem->render();
  ec_camera_elem::carCameraElem->render();
}
