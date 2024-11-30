// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "depthAOAbove.h"

#include <fx/dag_leavesWind.h>
#include <util/dag_threadPool.h>
#include <scene/dag_visibility.h>
#include <streaming/dag_streamingBase.h>
#include <landMesh/lmeshRenderer.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_delayedAction.h>
#include <osApiWrappers/dag_miscApi.h>
#include <shaders/dag_shaderBlock.h>
#include <rendInst/visibility.h>
#include <rendInst/rendInstGenRender.h>

#define INSIDE_RENDERER 1
#include "private_worldRenderer.h"
#include "lmesh_modes.h"

extern int rendinstDepthSceneBlockId;
extern int rendinstTransSceneBlockId;
extern int globalFrameBlockId;

class RenderDepthAOCB : public IRenderDepthAOCB
{
  DepthAOAboveContext &ctx;
  WorldRenderer &wr;
  TMatrix viewItm;

public:
  RenderDepthAOCB(DepthAOAboveContext &ctx, WorldRenderer &wr, const TMatrix view_itm) : ctx(ctx), wr(wr), viewItm(view_itm) {}

  void renderDepthAO(
    const Point3 &origin, mat44f_cref culling_view_proj, const float distace_around, int reg, RenderDepthAOType type, int)
  {
    G_ASSERT_RETURN(type > 0, );
    G_ASSERT_RETURN(reg < ctx.cullJobs.size() && ctx.cullJobs[reg].visibility, );
    // just to be sure that martrix is the same as prepared
    G_ASSERT(memcmp(&culling_view_proj, &ctx.cullJobs[reg].cullTm, sizeof(TMatrix4_vec4)) == 0);
    G_UNUSED(culling_view_proj);

    LeavesWindEffect::setNoAnimShaderVars(Point3(1, 0, 0), Point3(0, 0, 1), Point3(0, 1, 0));
    threadpool::wait(&ctx.cullJobs[reg]);
    if (type & RenderDepthAOType::Opaque)
    {
      SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
      rendinst::render::renderRIGen(rendinst::RenderPass::Depth, ctx.cullJobs[reg].visibility, viewItm,
        rendinst::LayerFlag::Opaque | rendinst::LayerFlag::NotExtra, rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::NoAtest,
        nullptr, wr.currentTexCtx);
    }

    if (type & RenderDepthAOType::Transparent)
    {
      SCENE_LAYER_GUARD(rendinstTransSceneBlockId);
      rendinst::render::renderRIGen(rendinst::RenderPass::Depth, ctx.cullJobs[reg].visibility, viewItm,
        rendinst::LayerFlag::Transparent, rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::NoAtest, nullptr,
        wr.currentTexCtx);
    }

    if (wr.binScene && (type & RenderDepthAOType::Opaque))
    {
      VisibilityFinder vf;
      // fixme: use same object_to_sceen_ratio as in main scene for csm shadows and main
      vf.set(v_ldu(&origin.x), ctx.cullJobs[reg].cullingFrustum, 0.0f, 0.0f, 1.0f, 1.0f, nullptr);
      wr.binScene->render(vf, 0, 0xFFFFFFFF);
    }

    if (wr.lmeshMgr && (type & RenderDepthAOType::Terrain))
    {
      set_lmesh_rendering_mode(LMeshRenderingMode::RENDERING_HEIGHTMAP);

      const float oldInvGeomDist = wr.lmeshRenderer->getInvGeomLodDist();
      const float heightmap_size = 4096;
      wr.lmeshRenderer->setInvGeomLodDist(0.5 / heightmap_size);

      BBox3 box(Point3::xVz(origin, 0), 2 * distace_around);
      wr.lmeshRenderer->prepare(*wr.lmeshMgr, Point3::xVz(origin, 0), 0);

      wr.lmeshRenderer->setRenderInBBox(box);
      shaders::overrides::set(wr.depthClipState);
      wr.lmeshRenderer->render(culling_view_proj, Frustum{culling_view_proj}, *wr.lmeshMgr, LandMeshRenderer::RENDER_ONE_SHADER,
        origin);
      shaders::overrides::reset();
      wr.lmeshRenderer->setRenderInBBox(BBox3());

      wr.lmeshRenderer->setInvGeomLodDist(oldInvGeomDist);
    }
  }

  void getMinMaxZ(float &min_z, float &max_z) { wr.getMinMaxZ(min_z, max_z); }
};

void DepthAOAboveContext::AsyncVisiblityJob::doJob()
{
  TIME_D3D_PROFILE(depth_ao_above_cull);

  rendinst::prepareRIGenExtraVisibility((mat44f_cref)cullTm, viewPos, *visibility, false, NULL);

  rendinst::prepareRIGenVisibility(cullingFrustum, viewPos, visibility, false, NULL);
}

DepthAOAboveContext::DepthAOAboveContext(int tex_size, float depth_around_distance, bool render_transparent) :
  renderer(tex_size, depth_around_distance, render_transparent)
{}

DepthAOAboveContext::~DepthAOAboveContext()
{
  for (AsyncVisiblityJob &job : cullJobs)
  {
    threadpool::wait(&job);
    if (job.visibility)
      rendinst::destroyRIGenVisibility(job.visibility);
  }
}

bool DepthAOAboveContext::prepare(const Point3 &view_pos, float scene_min_z, float scene_max_z)
{
  renderer.prepareRenderRegions(view_pos, scene_min_z, scene_max_z);
  auto regs = renderer.getRegionsToRender();
  G_ASSERT_RETURN(regs.size() < cullJobs.size(), false);

  if (regs.empty())
    return false;

  bool tpJobsAdded = false;
  for (int i = 0; i < regs.size(); ++i)
  {
    auto &r = regs[i];
    if (r.reg.wd.x <= 0 || r.reg.wd.y <= 0)
      continue;

    AsyncVisiblityJob &job = cullJobs[i];
    job.cullTm = r.cullViewProj;
    job.cullingFrustum.construct(r.cullViewProj);
    job.viewPos = view_pos;
    if (!job.visibility)
    {
      job.visibility = rendinst::createRIGenVisibility(midmem);
      rendinst::setRIGenVisibilityMinLod(job.visibility, 0, 1);
      rendinst::setRIGenVisibilityAtestSkip(job.visibility, true, false);
      rendinst::setRIGenVisibilityRendering(job.visibility, rendinst::VisibilityRenderingFlag::Static); // Do not render falling trees.
    }

    threadpool::add(&job, threadpool::PRIO_LOW, /*wake*/ false); // Low prio since it's waited on in the end of the frame
    tpJobsAdded = true;
  }
  return tpJobsAdded;
}

void DepthAOAboveContext::waitCullJobs()
{
  for (auto &j : cullJobs)
    threadpool::wait(&j);
}

void DepthAOAboveContext::render(WorldRenderer &wr, const TMatrix &view_itm)
{
  FRAME_LAYER_GUARD(globalFrameBlockId);
  RenderDepthAOCB depthAoCb(*this, wr, view_itm);
  renderer.renderPreparedRegions(depthAoCb);
}
