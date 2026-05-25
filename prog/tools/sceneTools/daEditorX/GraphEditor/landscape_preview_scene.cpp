// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "landscape_preview_scene.h"

#include "pluginService/landscape_preview_service.h"
#include "pluginService/graph_tex_gen_service.h"

#include <de3_interface.h>

#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix4.h>
#include <util/dag_flyMode.h>

namespace
{
// Matches IFreeCameraDriver::turboScale default used by prog/tools/graphEditor/app.cpp.
constexpr float TURBO_SCALE = 3.0f;

const char *SERVICE_NAME_UNAVAILABLE = "Landscape preview service not registered in this daEditorX build.";
} // namespace

LandscapePreviewScene::LandscapePreviewScene()
{
  // Replicates prog/tools/graphEditor/app.cpp camera setup: create_mskbd_free_camera()
  // uses FlyModeBlackBox internally, then scaleSensitivity(1300, 0.5) and an initial
  // matrix_look_at_lh pose. Input is fed from the ImGui panel instead of HumanInput.
  flyMode = create_flymode_bb();
  flyMode->setViewMatrix(inverse(tmatrix(matrix_look_at_lh(Point3(0, 3800, -6000), Point3(0, 0, -1500), Point3(0, 1, 0)))));
  flyMode->turboSpd = flyMode->moveSpd * 20.0f;
  flyMode->angSpdScaleH *= 0.5f;
  flyMode->angSpdScaleV *= 0.5f;
  flyMode->moveSpd *= 1300.0f;
  flyMode->turboSpd *= 1300.0f;

  TMatrix itm;
  flyMode->getInvViewMatrix(itm);
  cameraPos = itm.getcol(3);
  cameraForward = itm.getcol(2);
  viewTm = orthonormalized_inverse(itm);
}

LandscapePreviewScene::~LandscapePreviewScene()
{
  delete flyMode;
  flyMode = nullptr;
}

bool LandscapePreviewScene::init()
{
  if (serviceLookedUp)
  {
    return service != nullptr;
  }
  serviceLookedUp = true;

  if (IEditorService *svc = IDaEditor3Engine::get().findService("landscape-preview"))
  {
    service = svc->queryInterface<ILandscapePreviewService>();
  }

  return service != nullptr;
}

bool LandscapePreviewScene::isServiceAvailable() const
{
  if (!service)
  {
    return false;
  }

  return service->isAvailable();
}

const char *LandscapePreviewScene::getUnavailableReason() const
{
  if (!service)
  {
    return SERVICE_NAME_UNAVAILABLE;
  }

  return service->getUnavailableReason();
}

void LandscapePreviewScene::updateHeightmapParams(IGraphTexGenService &texgen)
{
  if (!service)
  {
    return;
  }

  service->setHeightmapParams(texgen.getHeightmapScale(), texgen.getHeightmapMin(), texgen.getHeightmapCellSize());
  service->setHeightmapTexture(texgen.getHeightmapTextureId());
}

void LandscapePreviewScene::actCamera(float dt, const CameraInput &in)
{
  flyMode->keys.fwd = in.fwd;
  flyMode->keys.back = in.back;
  flyMode->keys.left = in.left;
  flyMode->keys.right = in.right;
  flyMode->keys.worldUp = in.worldUp;
  flyMode->keys.worldDown = in.worldDown;
  flyMode->keys.turbo = in.turbo;

  // Turbo acceleration ramp, identical to MouseKbdFreeCameraDriver::act
  // (prog/samples/commonFramework/de3_freeCam_mk.cpp lines 82-87).
  const bool moving = in.fwd || in.back || in.left || in.right;
  accelTime = (in.turbo && moving) ? (accelTime + dt) : 0.0f;
  flyMode->turboSpd = flyMode->moveSpd * TURBO_SCALE * (accelTime * 4.0f + 1.0f);

  flyMode->integrate(dt, in.mouseDx, in.mouseDy);

  TMatrix itm;
  flyMode->getInvViewMatrix(itm);
  cameraPos = itm.getcol(3);
  cameraForward = itm.getcol(2);
  viewTm = orthonormalized_inverse(itm);
}

void LandscapePreviewScene::setRenderParams(float dt)
{
  if (!service)
  {
    return;
  }

  service->setCameraTransform(viewTm, cameraPos);
  service->setRenderParams(dt);
}

void LandscapePreviewScene::executeRender()
{
  if (!service)
  {
    return;
  }

  service->executeRender();
}

BaseTexture *LandscapePreviewScene::getOutputTexture()
{
  if (!service)
  {
    return nullptr;
  }

  return service->getOutputTexture();
}

IPoint2 LandscapePreviewScene::getOutputAllocSize() const
{
  if (!service)
  {
    return IPoint2(1, 1);
  }

  return service->getOutputAllocSize();
}
