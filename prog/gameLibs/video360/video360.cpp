// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ioSys/dag_dataBlock.h"
#include <3d/dag_render.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_shaders.h>
#include "shaders/dag_shaderBlock.h"
#include <startup/dag_globalSettings.h>
#include <util/dag_console.h>
#include "video360/video360.h"
#include <drv/3d/dag_commands.h>
#include <generic/dag_enumerate.h>
#include <math/dag_cube_matrix.h>

static ShaderVariableInfo copySourceVarId{"video360_copy_source"};

Video360::Video360(int cube_size, int convergence_frames, float z_near, float z_far, TMatrix view_itm) :
  cubemapFace(0),
  stabilityFrameIndex(0),
  copyTargetRenderer("copy_texture"),
  cubemapToSphericalProjectionRenderer("cubemap_to_spherical_projection"),
  zNear(z_near),
  zFar(z_far),
  cubeSize(cube_size),
  convergenceFrames(convergence_frames)
{
  // align camera position to be horizontal, and rotate with -90, so the
  // forward direction appears on the middle of the panoramic shot
  Point3 forward = view_itm.getcol(2);
  forward.y = 0;
  forward.normalize();
  savedCameraTm.makeTM(Point3(0, 1, 0), atan2f(forward.x, forward.z) + DegToRad(-90));
  savedCameraTm.setcol(3, view_itm.getcol(3));

  uint32_t fmt = TEXFMT_R11G11B10F;
  if (!(d3d::get_texformat_usage(fmt) & d3d::USAGE_RTARGET))
    fmt = TEXCF_SRGBREAD | TEXCF_SRGBWRITE;

  envCubeTex = dag::create_cubetex(cubeSize, fmt | TEXCF_RTARGET, 1, "video360_screen_env");
  for (auto [i, face] : enumerate(faces))
  {
    String name = String(0, "video360_%i", i);
    face = dag::create_tex(nullptr, cubeSize, cubeSize, fmt | TEXCF_RTARGET, 1, name);
  }
}

void Video360::update(ManagedTexView tex)
{
  G_ASSERT(!isFinished());
  stabilityFrameIndex++;
  if (stabilityFrameIndex > convergenceFrames)
  {
    d3d::stretch_rect(tex.getTex2D(), faces[cubemapFace].getTex2D());
    copyFrame(tex.getTexId(), cubemapFace);
    cubemapFace++;
    stabilityFrameIndex = 0;
  }
}

bool Video360::isFinished() { return cubemapFace > 5; }

eastl::optional<CameraSetupPerspPair> Video360::getCamera() const
{
  G_ASSERT_RETURN(cubemapFace >= 0 && cubemapFace <= 5, eastl::nullopt);

  TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, cubemapFace);
  cameraMatrix = savedCameraTm * cameraMatrix;

  CameraSetup cam;
  cam.transform = cameraMatrix;
  cam.accuratePos = dpoint3(cameraMatrix.getcol(3));
  cam.fov = 90;
  cam.znear = zNear;
  cam.zfar = zFar;

  Driver3dPerspective persp;
  persp.wk = 1.f;
  persp.hk = 1.f;
  persp.zn = zNear;
  persp.zf = zFar;

  return CameraSetupPerspPair{cam, persp};
}

dag::Span<UniqueTex> Video360::getFaces() { return dag::Span<UniqueTex>(faces, countof(faces)); }

int Video360::getCubeSize() { return cubeSize; }


void Video360::copyFrame(TEXTUREID renderTargetTexId, int cube_face)
{
  G_ASSERT_RETURN(cubemapFace >= 0 && cubemapFace <= 5, );

  ShaderGlobal::set_texture(copySourceVarId, renderTargetTexId);

  SCOPE_RENDER_TARGET;
  d3d::set_render_target(0, envCubeTex.getCubeTex(), cube_face, 0);
  copyTargetRenderer.render();
}

void Video360::renderSphericalProjection() { cubemapToSphericalProjectionRenderer.render(); }