#include "PixelPerfectEntitySelector.h"
#include <3d/dag_lockTexture.h>
#include <EditorCore/ec_interface.h>
#include <image/dag_texPixel.h>
#include <rendInst/rendInstExtraRender.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_shaderBlock.h>
#include <render/dynModelRenderer.h>

int PixelPerfectEntitySelector::global_frame_block_id = -1;
int PixelPerfectEntitySelector::rendinst_scene_block_id = -1;

void PixelPerfectEntitySelector::init()
{
  global_frame_block_id = ShaderGlobal::getBlockId("global_frame");
  rendinst_scene_block_id = ShaderGlobal::getBlockId("rendinst_scene");

  rendinstMatrixBuffer.reset(d3d::create_sbuffer(sizeof(Point4), 4U, SBCF_MAYBELOST | SBCF_BIND_SHADER_RES, TEXFMT_A32B32G32R32F,
    "simple_selection_matrix_buffer"));

  depthRt.set(d3d::create_tex(nullptr, 1, 1, TEXCF_RTARGET | TEXFMT_DEPTH32, 1, "simple_selection_depth_rt"),
    "simple_selection_depth_rt");
  depthRt.getTex2D()->texfilter(TEXFILTER_POINT);
}

TMatrix4 PixelPerfectEntitySelector::makeProjectionMatrixForViewRegion(int viewWidth, int viewHeight, float fov, float zNear,
  float zFar, int regionLeft, int regionTop, int regionWidth, int regionHeight)
{
  // Normal frustum.
  const float verticalFov = 2.f * atanf(tanf(0.5f * fov) * (static_cast<float>(viewHeight) / viewWidth));
  const float aspect = static_cast<float>(viewWidth) / viewHeight;
  const float top = tanf(verticalFov * 0.5f) * zNear;
  const float bottom = -top;
  const float left = aspect * bottom;
  const float right = aspect * top;
  const float width = fabs(right - left);
  const float height = fabs(top - bottom);

  // Frustum that covers the required region.
  const float subLeft = left + ((regionLeft * width) / viewWidth);
  const float subTop = top - ((regionTop * height) / viewHeight);
  const float subWidth = (width * regionWidth) / viewWidth;
  const float subHeight = (height * regionHeight) / viewHeight;
  return matrix_perspective_off_center_reverse(subLeft, subLeft + subWidth, subTop - subHeight, subTop, zNear, zFar);
}

bool PixelPerfectEntitySelector::getHitFromDepthBuffer(float &hitZ)
{
  bool hasBeenHit = false;

  Texture *depthTexture = depthRt.getTex2D();
  if (depthTexture)
  {
    int stride = 0;
    LockedTexture<float> lockedTexture = lock_texture<float>(depthTexture, stride, 0, TEXLOCK_READ);
    if (lockedTexture)
    {
      hitZ = lockedTexture[0];
      hasBeenHit = hitZ != 0.0f;
    }
  }

  return hasBeenHit;
}

void PixelPerfectEntitySelector::getHitsAt(IGenViewportWnd &wnd, int pickX, int pickY, dag::Vector<Hit> &hits)
{
  if (hits.empty())
    return;

  if (!isInited())
    init();

  Driver3dRenderTarget prevRT;
  d3d::get_render_target(prevRT);

  int prevViewX, prevViewY, prevViewWidth, prevViewHeight;
  float prevViewMinZ, prevViewMaxZ;
  d3d::getview(prevViewX, prevViewY, prevViewWidth, prevViewHeight, prevViewMinZ, prevViewMaxZ);

  TMatrix prevProj;
  d3d::gettm(TM_PROJ, prevProj);

  int viewportWidth, viewportHeight;
  wnd.getViewportSize(viewportWidth, viewportHeight);

  float zNear;
  float zFar;
  wnd.getZnearZfar(zNear, zFar);

  const float fov = wnd.getFov();
  const TMatrix4 projMatrix = makeProjectionMatrixForViewRegion(viewportWidth, viewportHeight, fov, zNear, zFar, pickX, pickY, 1, 1);

  d3d::set_render_target(nullptr, 0); // We only use the depth buffer.
  d3d::set_depth(depthRt.getTex2D(), DepthAccess::RW);
  d3d::setview(0, 0, 1, 1, prevViewMinZ, prevViewMaxZ);
  d3d::settm(TM_PROJ, &projMatrix);

  const int lastFrameBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(global_frame_block_id, ShaderGlobal::LAYER_FRAME);

  for (int i = 0; i < hits.size(); ++i)
  {
    Hit &hit = hits[i];

    d3d::clearview(CLEAR_ZBUFFER, 0, 0.0f, 0);

    if (hit.rendInstExtraResourceIndex >= 0)
    {
      Point4 data[4];
      data[0] = Point4(hit.transform.getcol(0).x, hit.transform.getcol(1).x, hit.transform.getcol(2).x, hit.transform.getcol(3).x),
      data[1] = Point4(hit.transform.getcol(0).y, hit.transform.getcol(1).y, hit.transform.getcol(2).y, hit.transform.getcol(3).y),
      data[2] = Point4(hit.transform.getcol(0).z, hit.transform.getcol(1).z, hit.transform.getcol(2).z, hit.transform.getcol(3).z);
      data[3] = Point4(0, 0, 0, 0);
      rendinstMatrixBuffer.get()->updateData(0, sizeof(Point4) * 4U, &data, 0);

      IPoint2 offsAndCnt(0, 1);
      uint16_t riIdx = hit.rendInstExtraResourceIndex;
      uint32_t zeroLodOffset = 0;
      SCENE_LAYER_GUARD(rendinst_scene_block_id);

      rendinst::render::ensureElemsRebuiltRIGenExtra(/*gpu_instancing = */ false);

      rendinst::render::renderRIGenExtraFromBuffer(rendinstMatrixBuffer.get(), dag::ConstSpan<IPoint2>(&offsAndCnt, 1),
        dag::ConstSpan<uint16_t>(&riIdx, 1), dag::ConstSpan<uint32_t>(&zeroLodOffset, 1), rendinst::RenderPass::Depth,
        rendinst::OptimizeDepthPass::Yes, rendinst::OptimizeDepthPrepass::No, rendinst::IgnoreOptimizationLimits::No,
        rendinst::LayerFlag::Opaque);
    }
    else
    {
      G_ASSERT(hit.sceneInstance);
      if (!dynrend::render_in_tools(hit.sceneInstance, dynrend::RenderMode::Opaque))
        hit.sceneInstance->render();
    }

    if (!getHitFromDepthBuffer(hit.z))
    {
      hits.erase_unsorted(&hit);
      --i;
    }
  }

  ShaderGlobal::setBlock(lastFrameBlockId, ShaderGlobal::LAYER_FRAME);

  d3d::settm(TM_PROJ, prevProj);
  d3d::setview(prevViewX, prevViewY, prevViewWidth, prevViewHeight, prevViewMinZ, prevViewMaxZ);
  d3d::set_render_target(prevRT);
}
