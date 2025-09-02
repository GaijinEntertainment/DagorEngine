// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rendInstHeightmap.h"
#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <scene/dag_visibility.h>
#include <math/dag_frustum.h>
#include <math/dag_bounds2.h>
#include <perfMon/dag_perfTimer2.h>
#include <rendInst/visibility.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstExtra.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_threadPool.h>

#include <render/world/wrDispatcher.h>
#include <streaming/dag_streamingBase.h>


namespace var
{
static ShaderVariableInfo prefab_render_pass_is_depth("prefab_render_pass_is_depth", true);
}


#define GLOBAL_VARS_LIST                      \
  VAR(rendinst_clipmap_world_to_hmap_tex_ofs) \
  VAR(rendinst_clipmap_world_to_hmap_ofs)     \
  VAR(rendinst_clipmap)                       \
  VAR(rendinst_clipmap_zn_zf)                 \
  VAR(rendinst_clipmap_samplerstate)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR
static int is_rendinst_clipmapVarId = -1; // not global shader var
static int is_prefab_clipmapVarId = -1;   // not global shader var

extern ShaderBlockIdHolder rendinstDepthSceneBlockId;

static const float MIN_UPDATE_THRESHOLD = 30.f;

RendInstHeightmap::~RendInstHeightmap()
{
  for (RiGenVisibility *riHmapVisibility : rendinstHeightmapVisibilities)
    rendinst::destroyRIGenVisibility(riHmapVisibility);
  rendinstHeightmapVisibilities.clear();
}

bool RendInstHeightmap::has_heightmap_objects_on_scene()
{
  if (rendinst::hasRIClipmapPools() != rendinst::HasRIClipmap::NO)
    return true;

  auto *binScene = WRDispatcher::getBinScene();
  if (binScene && binScene->hasClipmap())
    return true;

  return false;
}

RiGenVisibility *create_rendinst_heightmap_visibility()
{
  RiGenVisibility *rendinstHeightmapVisibility = rendinst::createRIGenVisibility(midmem);
  rendinst::setRIGenVisibilityMinLod(rendinstHeightmapVisibility, 0, 0);
  rendinst::setRIGenVisibilityAtestSkip(rendinstHeightmapVisibility, true, false);
  return rendinstHeightmapVisibility;
}

RendInstHeightmap::RendInstHeightmap(int tex_size, float rect_size, float land_height_min, float land_height_max) :
  enabled(false),
  texSize(tex_size + (int)MIN_UPDATE_THRESHOLD * 2), // note: NPOT texture
  rectSize(rect_size + MIN_UPDATE_THRESHOLD * 2.0f),
  texelSize(rect_size / tex_size),
  landHeightMin(land_height_min),
  landHeightMax(land_height_max)
{
  const char *texName = "rendinst_clipmap_depth_tex";
  depthTexture = dag::create_tex(nullptr, texSize, texSize, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, texName);

  invalidate();
  helper.texSize = texSize;

  vtm.setcol(0, 1, 0, 0);
  vtm.setcol(1, 0, 0, 1);
  vtm.setcol(2, 0, 1, 0);
  vtm.setcol(3, 0, 0, 0);

#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR

#define VAR(a) &&VariableMap::isGlobVariablePresent(a##VarId)
  enabled = true GLOBAL_VARS_LIST;
#undef VAR

  is_rendinst_clipmapVarId = VariableMap::getVariableId("is_rendinst_clipmap");
  is_prefab_clipmapVarId = VariableMap::getVariableId("is_prefab_clipmap");
  enabled =
    enabled && (VariableMap::isVariablePresent(is_rendinst_clipmapVarId) || VariableMap::isVariablePresent(is_prefab_clipmapVarId));

  if (!enabled)
    logerr("Rendinst heightmap shadervars not found. Rendinst heightmap will be disabled.");

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
    ShaderGlobal::set_sampler(rendinst_clipmap_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
}

static struct RendinstHeightmapVisibilityJob final : public cpujobs::IJob
{
  RendInstHeightmap *riHmap;
  const char *getJobName(bool &) const override { return "rendinst_heightmap_visibility"; }
  void doJob() override { riHmap->prepareRiVisibilityAsync(); };
} visibility_job;

void RendInstHeightmap::updatePos(const Point3 &cam_pos)
{
  if (!enabled || lengthSq(cam_pos - prevCamPos) < MIN_UPDATE_THRESHOLD * MIN_UPDATE_THRESHOLD)
    return;

  float maxStaticHeight = rendinst::getMaxRendinstHeight(is_rendinst_clipmapVarId);
  auto *binScene = WRDispatcher::getBinScene();
  if (binScene)
  {
    float minPrefabHeight, maxPrefabHeight;
    if (binScene->getClipmapMinMaxHeight(minPrefabHeight, maxPrefabHeight))
      maxStaticHeight = max(maxStaticHeight, maxPrefabHeight);
  }

  float zf = max(landHeightMax, maxStaticHeight);
  if (zf > maxHt)
  {
    invalidate();
    maxHt = zf;
  }

  IPoint2 newTexelsOrigin = ipoint2(floor(Point2::xz(cam_pos) / texelSize));

  regionsToUpdate.resize(0);
  ToroidalGatherCallback cb(regionsToUpdate);
  nextHelper = helper;
  int updated = toroidal_update(newTexelsOrigin, nextHelper, nextHelper.texSize / 2, cb);
  if (!updated)
    return;

  visibility_job.riHmap = this;
  // NOTE: intentionally don't wake up worker thread because there are a lot of other parallel jobs near this one
  // (most likely CSM jobs for animchars will wake up it), also we are waiting for result of this job only in next
  // frame, so priority can be minimal.
  threadpool::add(&visibility_job, threadpool::PRIO_LOW, /* wake */ false);

  prevCamPos = cam_pos;
}

void RendInstHeightmap::prepareRiVisibilityAsync()
{
  float zn = landHeightMin;
  float zf = maxHt;
  for (int i = 0; i < regionsToUpdate.size(); ++i)
  {
    const ToroidalQuadRegion &region = regionsToUpdate[i];
    G_ASSERT(region.wd.x > 0 && region.wd.y > 0);

    BBox2 box(point2(region.texelsFrom) * texelSize, point2(region.texelsFrom + region.wd) * texelSize);
    projTMs[i] = ::matrix_ortho_off_center_lh(box[0].x, box[1].x, box[0].y, box[1].y, zn, zf);
    G_ASSERTF(region.lt.x + region.wd.x <= helper.texSize && helper.texSize - region.lt.y <= helper.texSize,
      "update region exceeds texture size left_top = %@, width = %@ texSize = %@", region.lt, region.wd, helper.texSize);

    TMatrix4 globTm = TMatrix4(vtm) * projTMs[i];
    mat44f culling_view_proj;
    v_mat44_make_from_44cu(culling_view_proj, &globTm._11);

    if (rendinstHeightmapVisibilities.size() <= i)
      rendinstHeightmapVisibilities.push_back(create_rendinst_heightmap_visibility());

    rendinst::prepareRIGenExtraVisibility(culling_view_proj, Point3(box.center().x, (zf - zn), box.center().y),
      *rendinstHeightmapVisibilities[i], true, NULL, {}, rendinst::RiExtraCullIntention::MAIN, false, true);
  }
}

void RendInstHeightmap::updateToroidalTextureRegions(int globalFrameBlockId)
{
  TIME_D3D_PROFILE(rendinst_heightmap_update_texture);

  threadpool::wait(&visibility_job);

  if (regionsToUpdate.empty())
    return;
  for (int i = 0; i < regionsToUpdate.size(); ++i)
    if (!rendinst::isRiGenVisibilityForcedLodLoaded(rendinstHeightmapVisibilities[i]))
    {
      rendinst::riGenVisibilityScheduleForcedLodLoading(rendinstHeightmapVisibilities[i]);
      return;
    }
  helper = nextHelper;

  TMatrix itm = orthonormalized_inverse(vtm);

  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;
  FRAME_LAYER_GUARD(globalFrameBlockId);

  d3d::settm(TM_VIEW, vtm);
  d3d::set_render_target((Texture *)nullptr, 0);
  d3d::set_depth(depthTexture.getTex2D(), DepthAccess::RW);

  for (int i = 0; i < regionsToUpdate.size(); ++i)
  {
    const ToroidalQuadRegion &region = regionsToUpdate[i];
    d3d::setview(region.lt.x, helper.texSize - region.wd.y - region.lt.y, region.wd.x, region.wd.y, 0, 1);
    d3d::clearview(CLEAR_ZBUFFER, E3DCOLOR(), 0, 0);

    d3d::settm(TM_PROJ, &projTMs[i]);
    // globtm is changed by d3d::settm() so need to set this scene block for every region
    {
      TIME_D3D_PROFILE(render_ri_gen);

      SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
      rendinst::render::renderRIGen(rendinst::RenderPass::ToHeightMap, rendinstHeightmapVisibilities[i], itm,
        rendinst::LayerFlag::Opaque | rendinst::LayerFlag::RendinstClipmapBlend, rendinst::OptimizeDepthPass::No);
    }

    if (VariableMap::isGlobVariablePresent(var::prefab_render_pass_is_depth))
    {
      auto *binScene = WRDispatcher::getBinScene();
      if (binScene)
      {
        TIME_D3D_PROFILE(render_bin_scene);

        const auto &cameraPos = itm.getcol(3);
        TMatrix4 viewProj = TMatrix4(vtm) * projTMs[i];
        Frustum cullingFrustum = viewProj;

        VisibilityFinder vf;
        vf.set(v_ldu(&cameraPos.x), cullingFrustum, 0.0f, 0.0f, 1.0f, 1.0f, nullptr);

        ShaderGlobal::set_int(var::prefab_render_pass_is_depth, 1);
        binScene->render(vf, 0, 0xFFFFFFFF);
        ShaderGlobal::set_int(var::prefab_render_pass_is_depth, 0);
      }
    }
  }
  regionsToUpdate.resize(0);

  d3d::set_depth(nullptr, DepthAccess::RW);
  d3d::resource_barrier({depthTexture.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});

  setVar();
}

void RendInstHeightmap::driverReset()
{
  invalidate();
  d3d::resource_barrier({depthTexture.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
}

void RendInstHeightmap::invalidate()
{
  prevCamPos.x = prevCamPos.y = prevCamPos.z = eastl::numeric_limits<float>::max();
  helper.curOrigin = {-1000000, 100000};
}

void RendInstHeightmap::setVar()
{
  if (!enabled)
    return;
  ShaderGlobal::set_texture(rendinst_clipmapVarId, depthTexture);
  Point2 ofs = Point2((helper.mainOrigin - helper.curOrigin) % helper.texSize) / helper.texSize;
  ShaderGlobal::set_color4(rendinst_clipmap_world_to_hmap_tex_ofsVarId, ofs.x, -ofs.y, 0, 0);

  Point2 worldSpaceOrigin = Point2(helper.curOrigin) * texelSize;
  float worldArea = helper.texSize * texelSize;
  ShaderGlobal::set_color4(rendinst_clipmap_world_to_hmap_ofsVarId, 1.0f / worldArea, -1.0f / worldArea,
    -worldSpaceOrigin.x / worldArea + 0.5f, worldSpaceOrigin.y / worldArea + 0.5);

  ShaderGlobal::set_color4(rendinst_clipmap_zn_zfVarId, landHeightMin, maxHt, 0, 0);
}
