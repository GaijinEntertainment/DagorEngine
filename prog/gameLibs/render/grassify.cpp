// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/grassify.h>

#include <EASTL/tuple.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/visibility.h>
#include <vecmath/dag_vecMath_common.h>
#include <math/dag_bounds3.h>
#include <math/dag_mathUtils.h>
#include <perfMon/dag_statDrv.h>
#include <render/gpuGrass.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <shaders/dag_overrideStates.h>

static int globalFrameBlockId = -1;

#define GLOBAL_VARS_LIST                             \
  VAR(world_to_grass_slice)                          \
  VAR(rendinst_landscape_area_left_top_right_bottom) \
  VAR(grass_grid_params)                             \
  VAR_OPT(grass_average_ht__ht_extent__avg_hor__hor_extent)

#define VAR_OPT(a) static int a##VarId = -1;
#define VAR(a)     static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR
#undef VAR_OPT

struct GrassMaskSliceHelper
{
  GrassMaskSliceHelper(float texelSize, IPoint2 maskSize, const IPoint2 &offset) :
    texelSize(texelSize), offset(offset), maskSize(maskSize)
  {
    copy_grass_decals.init("copy_grass_decals");

    shaders::OverrideState state;
    state.set(shaders::OverrideState::FLIP_CULL);
    flipCullStateId = shaders::overrides::create(state);

    maskTex = dag::create_tex(nullptr, maskSize.x, maskSize.y, TEXCF_RTARGET, 1, "grass_mask_islands_tex");
    ShaderGlobal::set_sampler(get_shader_variable_id("grass_mask_islands_tex_samplerstate", true), d3d::request_sampler({}));
    colorTex =
      dag::create_tex(nullptr, maskSize.x, maskSize.y, TEXCF_SRGBREAD | TEXCF_SRGBWRITE | TEXCF_RTARGET, 1, "grass_color_islands_tex");
    ShaderGlobal::set_sampler(get_shader_variable_id("grass_color_islands_tex_samplerstate", true), d3d::request_sampler({}));
  };

  void renderMask(IRandomGrassRenderHelper &grassRenderHelper);

  float texelSize;
  IPoint2 maskSize;
  IPoint2 offset;
  UniqueTexHolder maskTex, colorTex;
  shaders::UniqueOverrideStateId flipCullStateId;
  PostFxRenderer copy_grass_decals;
};

struct GrassGenerateHelper
{
  GrassGenerateHelper(float maxDistance) : maxDistance(maxDistance) {}

  void initGrassifyRendinst()
  {
    if (!globalVis)
      globalVis = rendinst::createRIGenVisibility(midmem);

    if (!frameVis)
      frameVis = rendinst::createRIGenVisibility(midmem);

    rendinst::setRIGenVisibilityMinLod(globalVis, 0, 2);
    rendinst::setRIGenVisibilityRendering(globalVis, rendinst::VisibilityRenderingFlag::Static);
    const real max = 1e10;
    bbox3f boxCull = v_ldu_bbox3(BBox3(Point3(-max, -max, -max), Point3(max, max, max)));
    rendinst::prepareRIGenExtraVisibilityForGrassifyBox(boxCull, 3, 0, 0, *globalVis);
  }

  ~GrassGenerateHelper()
  {
    rendinst::destroyRIGenVisibility(globalVis);
    rendinst::destroyRIGenVisibility(frameVis);
  }

  void generate(const GrassView view, const Point3 &position, const IPoint2 &grassOffset, const IPoint2 &grassMaskSize,
    float grassMaskTexelSize, const TMatrix &view_itm, Driver3dPerspective perspective, const GPUGrassBase &gpuGrassBase);
  void render(const Point3 &position, const TMatrix &view_itm, const GPUGrassBase &gpuGrassBase);

private:
  TMatrix getViewMatrix() const;
  TMatrix4 getProjectionMatrix(const Point3 &position, float distance) const;

private:
  float maxDistance;
  RiGenVisibility *globalVis = nullptr;
  RiGenVisibility *frameVis = nullptr;
  int rendinstGrassifySceneBlockId = ShaderGlobal::getBlockId("rendinst_grassify_scene");
};

void GrassMaskSliceHelper::renderMask(IRandomGrassRenderHelper &grassRenderHelper)
{
  TIME_D3D_PROFILE(grassify_mask)

  MaskRenderCallback maskcb(texelSize, grassRenderHelper, maskTex.getTex2D(), colorTex.getTex2D(), &copy_grass_decals);

  shaders::overrides::set(flipCullStateId);

  maskcb.start(IPoint2(0, 0));
  maskcb.renderQuad(IPoint2(0, 0), maskSize, IPoint2(offset.x / texelSize, offset.y / texelSize));
  maskcb.end();
  shaders::overrides::reset();
}

TMatrix GrassGenerateHelper::getViewMatrix() const
{
  TMatrix vtm;
  vtm.setcol(0, 1, 0, 0);
  vtm.setcol(1, 0, 0, 1);
  vtm.setcol(2, 0, 1, 0);
  vtm.setcol(3, 0, 0, 0);
  return vtm;
}

TMatrix4 GrassGenerateHelper::getProjectionMatrix(const Point3 &position, float distance) const
{
  Point2 position2D = Point2(position.x, position.z);
  BBox2 box(position2D - Point2(distance, distance), position2D + Point2(distance, distance));
  BBox3 bbox3(Point3::xVy(box[0], -3000), Point3::xVy(box[1], 3000));
  return matrix_ortho_off_center_lh(bbox3[0].x, bbox3[1].x, bbox3[1].z, bbox3[0].z, bbox3[0].y, bbox3[1].y);
}

void GrassGenerateHelper::generate(const GrassView view, const Point3 &position, const IPoint2 &grassMaskOffset,
  const IPoint2 &grassMaskSize, float grassMaskTexelSize, const TMatrix &itm, Driver3dPerspective perspective,
  const GPUGrassBase &gpuGrassBase)
{
  if (!globalVis)
    return;

  TIME_D3D_PROFILE(grassify_generate);

  const float alignSize = gpuGrassBase.alignTo();
  Point2 position2D = floor(Point2(position.x, position.z) / (2 * alignSize) + Point2(0.5f, 0.5f)) * (2 * alignSize);
  Point3 alignedCenterPos = Point3(position2D.x, position.y, position2D.y);
  Point2 grassMaskDistance = Point2(grassMaskSize) * grassMaskTexelSize;

  TMatrix4 projTm;
  // Fix far to cull based on distance.
  perspective.zf = maxDistance;
  d3d::calcproj(perspective, projTm);
  Frustum frustum(TMatrix4(inverse(itm)) * projTm);
  rendinst::filterVisibility(*globalVis, *frameVis, [&frustum](vec4f min, vec4f max) { return frustum.testBox(min, max); });

  {
    SCOPE_VIEW_PROJ_MATRIX;
    const auto maxGrassHeightSize = gpuGrassBase.getMaxGrassHeight();
    const auto maxGrassHorSize = gpuGrassBase.getMaxGrassHorSize();

    TMatrix vtm = getViewMatrix();
    d3d::settm(TM_VIEW, vtm);

    auto renderGrassLod = [&](const float currentGridSize, float currentGrassDistance) {
      // CurrentGrassDistance is measured in world space,
      // but rendering islands are rotated for a top-down orthographic view (Y-up).
      // This rotation may push some points beyond the CurrentGrassDistance and outside the camera bounds.
      // To compensate this, we scale the camera bounds by a distance multiplier.
      const float distanceMultiplier = 2;
      currentGrassDistance *= distanceMultiplier;
      const int quadSize = currentGrassDistance / currentGridSize;
      ShaderGlobal::set_color4(grass_average_ht__ht_extent__avg_hor__hor_extentVarId,
        0.5 * maxGrassHeightSize,               // average grass Ht
        0.5 * maxGrassHeightSize,               // grass Ht extents
        currentGridSize * 0.5,                  // average grid extents
        currentGridSize * 0.5 + maxGrassHorSize // max hor extents
      );

      TMatrix4 proj = getProjectionMatrix(alignedCenterPos, currentGrassDistance);
      d3d::settm(TM_PROJ, &proj);
      d3d::setview(0, 0, min(quadSize * 2, grassMaskSize.x), min(quadSize * 2, grassMaskSize.y), 0, 1);

      ShaderGlobal::set_color4(grass_grid_paramsVarId, alignedCenterPos.x, alignedCenterPos.y, currentGridSize, quadSize);

      FRAME_LAYER_GUARD(globalFrameBlockId);
      SCENE_LAYER_GUARD(rendinstGrassifySceneBlockId);

      rendinst::render::renderRIGen(rendinst::RenderPass::Grassify, frameVis, orthonormalized_inverse(vtm),
        rendinst::LayerFlag::Opaque, rendinst::OptimizeDepthPass::No, 3);
    };

    ShaderGlobal::set_color4(world_to_grass_sliceVarId, 1.0f / grassMaskDistance.x, 1.0f / grassMaskDistance.y,
      -grassMaskOffset.x / grassMaskDistance.x, -grassMaskOffset.y / grassMaskDistance.y);

    // todo: generate lods in one pass
    gpuGrassBase.renderGrassLods(view, renderGrassLod);

    ShaderElement::invalidate_cached_state_block();
  }
}

Grassify::~Grassify() {}

Grassify::Grassify(const DataBlock &grassifyBlock, float grassDistance)
{
#define VAR(a)     a##VarId = get_shader_variable_id(#a);
#define VAR_OPT(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef VAR_OPT

  globalFrameBlockId = ShaderGlobal::getBlockId("global_frame");
  int grassifyMaskResolution = grassifyBlock.getInt("grassifyMaskResolution", 512);
  grassifyMaskSize = IPoint2(grassifyMaskResolution, grassifyMaskResolution);

  grassGenHelper = eastl::make_unique<GrassGenerateHelper>(grassDistance);
}

void Grassify::initGrassifyRendinst() { grassGenHelper->initGrassifyRendinst(); }

void Grassify::generate(GrassView view, const Point3 &pos, const TMatrix &view_itm, const Driver3dPerspective &perspective,
  Texture *grass_mask, IRandomGrassRenderHelper &grassRenderHelper, const GPUGrassBase &gpuGrassBase)
{
  if (!grassMaskHelper)
    generateGrassMask(grassRenderHelper);

  // There is no any render to this target, but we need any target to set as a render target,
  // and we need to keep the same texel size between regular grass and grassify, so we use same targets.
  d3d::set_render_target(grass_mask, 0);

  grassMaskHelper->maskTex.setVar();
  grassMaskHelper->colorTex.setVar();

  grassGenHelper->generate(view, pos, grassMaskHelper->offset, grassMaskHelper->maskSize, grassMaskHelper->texelSize, view_itm,
    perspective, gpuGrassBase);
}

void Grassify::generateGrassMask(IRandomGrassRenderHelper &grassRenderHelper)
{
  Color4 rendinst_landscape_area_left_top_right_bottom = ShaderGlobal::get_color4(rendinst_landscape_area_left_top_right_bottomVarId);

  IPoint2 grassifyMaskDistance =
    IPoint2(rendinst_landscape_area_left_top_right_bottom.b - rendinst_landscape_area_left_top_right_bottom.r,
      rendinst_landscape_area_left_top_right_bottom.a - rendinst_landscape_area_left_top_right_bottom.g);

  IPoint2 grassifyMaskOffset =
    IPoint2(min(rendinst_landscape_area_left_top_right_bottom.b, rendinst_landscape_area_left_top_right_bottom.r),
      min(rendinst_landscape_area_left_top_right_bottom.a, rendinst_landscape_area_left_top_right_bottom.g));

  grassifyMaskDistance = abs(grassifyMaskDistance);
  float texelSize = min(grassifyMaskDistance.x / (float)grassifyMaskSize.x, grassifyMaskDistance.x / (float)grassifyMaskSize.y);

  grassMaskHelper = eastl::make_unique<GrassMaskSliceHelper>(texelSize, grassifyMaskSize, grassifyMaskOffset);
  grassMaskHelper->renderMask(grassRenderHelper);
}