#include "ioSys/dag_dataBlock.h"
#include <3d/dag_render.h>
#include <3d/dag_drv3d.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include "shaders/dag_shaderBlock.h"
#include <startup/dag_globalSettings.h>

#include "video360/video360.h"

static int global_frame_block_id = -1;

Video360::Video360() :
  enabled(false),
  captureFrame(false),
  frameIndex(-1),
  copyTargetRenderer(nullptr),
  cubemapToSphericalProjectionRenderer(nullptr),
  fixedFramerate(30),
  zNear(0.1f),
  zFar(1000.f),
  cubeSize(2048),
  renderTargetTex(nullptr),
  renderTargetTexId(-1)
{}

void Video360::init(int cube_size)
{
  cubeSize = cube_size;
  global_frame_block_id = ShaderGlobal::getBlockId("global_frame");

  // parameters
  const DataBlock *video360Blk = ::dgs_get_game_params()->getBlockByNameEx("video360");
  fixedFramerate = video360Blk->getInt("fixedFramerate", 30);

  savedCameraTm.identity();

  // shaders
  copyTargetRenderer = eastl::make_unique<PostFxRenderer>();
  copyTargetRenderer->init("copy_texture", nullptr, false);

  cubemapToSphericalProjectionRenderer = eastl::make_unique<PostFxRenderer>();
  cubemapToSphericalProjectionRenderer->init("cubemap_to_spherical_projection", nullptr, false);

  // create cubemat to capture frame
  if (copyTargetRenderer->getElem() && cubemapToSphericalProjectionRenderer->getElem())
  {
    uint32_t fmt = TEXFMT_R11G11B10F;
    if (!(d3d::get_texformat_usage(fmt) & d3d::USAGE_RTARGET))
      fmt = TEXCF_SRGBREAD | TEXCF_SRGBWRITE;

    envCubeTex.set(d3d::create_cubetex(cubeSize, fmt | TEXCF_RTARGET, 1, "video360_screen_env"), "video360_screen_env");
  }
}

void Video360::enable(bool enable_video360)
{
  enabled = enable_video360 && envCubeTex.getCubeTex() != nullptr;
  frameIndex = -1;
  captureFrame = false;
}

bool Video360::isEnabled() { return enabled; }

void Video360::activate(float z_near, float z_far, Texture *render_target_tex, TEXTUREID render_target_tex_id)
{
  captureFrame = true;

  zNear = z_near;
  zFar = z_far;

  renderTargetTex = render_target_tex;
  renderTargetTexId = render_target_tex_id;

  // align camera position to be horizontal, and rotate with -90, so the
  // forward direction appears on the middle of the panoramic shot
  Point3 forward = ::grs_cur_view.itm.getcol(2);
  forward.y = 0;
  forward.normalize();
  savedCameraTm.makeTM(Point3(0, 1, 0), atan2f(forward.x, forward.z) + DegToRad(-90));
  savedCameraTm.setcol(3, ::grs_cur_view.itm.getcol(3));
}

bool Video360::isActive() { return enabled && captureFrame; }

bool Video360::useFixedDt() { return fixedFramerate > 0; }

float Video360::getFixedDt() { return 1.f / (float)(max(fixedFramerate, 1)); }

int Video360::getCubeSize() { return cubeSize; }

static TMatrix build_cubemap_face_view_matrix(int face_index, const TMatrix &source_matrix)
{
  TMatrix cameraMatrix;
  switch (face_index)
  {
    case 0:
      cameraMatrix.setcol(0, -source_matrix.getcol(2));
      cameraMatrix.setcol(1, source_matrix.getcol(1));
      cameraMatrix.setcol(2, source_matrix.getcol(0));
      break;
    case 1:
      cameraMatrix.setcol(0, source_matrix.getcol(2));
      cameraMatrix.setcol(1, source_matrix.getcol(1));
      cameraMatrix.setcol(2, -source_matrix.getcol(0));
      break;
    case 3:
      cameraMatrix.setcol(0, source_matrix.getcol(0));
      cameraMatrix.setcol(1, -source_matrix.getcol(2));
      cameraMatrix.setcol(2, source_matrix.getcol(1));
      break;
    case 2:
      cameraMatrix.setcol(0, source_matrix.getcol(0));
      cameraMatrix.setcol(1, source_matrix.getcol(2));
      cameraMatrix.setcol(2, -source_matrix.getcol(1));
      break;
    case 4:
      cameraMatrix.setcol(0, source_matrix.getcol(0));
      cameraMatrix.setcol(1, source_matrix.getcol(1));
      cameraMatrix.setcol(2, source_matrix.getcol(2));
      break;
    case 5:
      cameraMatrix.setcol(0, -source_matrix.getcol(0));
      cameraMatrix.setcol(1, source_matrix.getcol(1));
      cameraMatrix.setcol(2, -source_matrix.getcol(2));
      break;
  }
  cameraMatrix.setcol(3, source_matrix.getcol(3));
  return cameraMatrix;
}

bool Video360::getCamera(DagorCurView &cur_view, Driver3dPerspective &persp)
{
  if (!enabled || frameIndex == -1)
    return false;

  int current_face_rendering = (frameIndex) % 6;

  TMatrix cameraMatrix = build_cubemap_face_view_matrix(current_face_rendering, TMatrix::IDENT);
  cameraMatrix = savedCameraTm * cameraMatrix;

  cur_view.itm = cameraMatrix;
  cur_view.tm = orthonormalized_inverse(::grs_cur_view.itm);
  cur_view.pos = grs_cur_view.itm.getcol(3);

  persp.wk = 1.f;
  persp.hk = 1.f;
  persp.zn = zNear;
  persp.zf = zFar;

  return true;
}

bool Video360::getCamera(CameraSetup &cam, Driver3dPerspective &persp)
{
  if (!enabled || frameIndex == -1)
    return false;

  int current_face_rendering = (frameIndex) % 6;

  TMatrix cameraMatrix = build_cubemap_face_view_matrix(current_face_rendering, TMatrix::IDENT);
  cameraMatrix = savedCameraTm * cameraMatrix;

  cam.transform = cameraMatrix;
  cam.accuratePos = dpoint3(cameraMatrix.getcol(3));
  cam.fov = 90;
  cam.znear = zNear;
  cam.zfar = zFar;

  persp.wk = 1.f;
  persp.hk = 1.f;
  persp.zn = zNear;
  persp.zf = zFar;

  return true;
}


void Video360::beginFrameRender(int frame_id)
{
  if (frame_id < 0 || frame_id > 5)
    return;

  frameIndex = frame_id;
}

void Video360::endFrameRender(int frame_id)
{
  if (frame_id < 0 || frame_id > 5)
    return;

  copyFrame(frame_id);
}

void Video360::renderResultOnScreen() { renderSphericalProjection(); }

void Video360::finishRendering()
{
  frameIndex = -1;
  captureFrame = false;
}

void Video360::copyFrame(int cube_face)
{
  if (!enabled || cube_face < 0 || cube_face > 5)
    return;

  static int copyTextureSourceVarId = get_shader_variable_id("video360_screen_env", true);
  ShaderGlobal::set_texture(copyTextureSourceVarId, renderTargetTexId);

  SCOPE_RENDER_TARGET;
  d3d::set_render_target(0, envCubeTex.getCubeTex(), cube_face, 0);
  ShaderGlobal::setBlock(global_frame_block_id, ShaderGlobal::LAYER_FRAME);
  copyTargetRenderer->render();
}

void Video360::renderSphericalProjection()
{
  // render spherical projection to screen
  envCubeTex.setVar();

  SCOPE_RENDER_TARGET;
  d3d::set_render_target(renderTargetTex, 0);
  ShaderGlobal::setBlock(global_frame_block_id, ShaderGlobal::LAYER_FRAME);
  cubemapToSphericalProjectionRenderer->render();
}