// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/voxelShadows.h>
#include <drv/3d/dag_draw.h>
#include <perfMon/dag_statDrv.h>
#include <math/random/dag_random.h>
#include <math/integer/dag_IBBox3.h>
#include <util/dag_convar.h>
#include <generic/dag_carray.h>
#include <render/spheres_consts.hlsli>

namespace convar
{
CONSOLE_BOOL_VAL("voxel_shadows", force_full_update, false);
CONSOLE_BOOL_VAL("voxel_shadows", pause_origin_update, false);
CONSOLE_BOOL_VAL("voxel_shadows", smooth_cascade_transition, true);
CONSOLE_BOOL_VAL("voxel_shadows", debug_voxels, false);
CONSOLE_BOOL_VAL("voxel_shadows", debug_probes, false);
CONSOLE_BOOL_VAL("voxel_shadows", debug_probes_all, false);
CONSOLE_INT_VAL("voxel_shadows", debug_probes_cascade, -1, -1, 3);
CONSOLE_FLOAT_VAL_MINMAX("voxel_shadows", debug_probes_threshold, 0.5, 0.f, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("voxel_shadows", height_offset_ratio, 0.5, 0.f, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("voxel_shadows", temporal_speed_mul, 1.f, 0.f, 100.f);
} // namespace convar

void validate_warp_size(ComputeShaderElement *e, const VoxelShadows::Settings &cfg)
{
  eastl::array<uint16_t, 3> tgSize = e->getThreadGroupSizes();
  G_ASSERT((cfg.xzDim % tgSize[0]) == 0 && (cfg.xzDim % tgSize[1]) == 0 && (cfg.yDim % tgSize[2]) == 0);
  G_UNUSED(cfg);
  G_UNUSED(tgSize);
}

VoxelShadows::VoxelShadows(const Settings &c) :
  cfg(c),
  voxel_shadows_rndVarId("voxel_shadows_rnd"),
  voxel_shadows_cascade_countVarId("voxel_shadows_cascade_count"),
  voxel_shadows_cascadeIdxVarId("voxel_shadows_cascade_idx"),
  voxel_shadows_originVarId("voxel_shadows_origin"),
  voxel_shadows_origin_in_tc_spaceVarId("voxel_shadows_origin_in_tc_space"),
  voxel_shadows_wvp_in_tc_spaceVarId("voxel_shadows_wvp_in_tc_space"),
  voxel_shadows_voxel_sizeVarId("voxel_shadows_voxel_size"),
  voxel_shadows_world_to_tcVarId("voxel_shadows_world_to_tc"),
  voxel_shadows_update_box_ofsVarId("voxel_shadows_update_box_ofs"),
  voxel_shadows_update_box_sizeVarId("voxel_shadows_update_box_size"),
  voxel_shadows_sparse_divVarId("voxel_shadows_sparse_div"),
  voxel_shadows_allow_dynamicVarId("voxel_shadows_allow_dynamic"),
  voxel_shadows_debug_typeVarId("voxel_shadows_debug_type", true),
  voxel_shadows_debug_thresholdVarId("voxel_shadows_debug_threshold", true),
  voxel_shadows_debug_cascadeVarId("voxel_shadows_debug_cascade", true)
{
  G_ASSERT(c.cascadeCount <= maxCascadeCount);

  rndSeed = (int)reinterpret_cast<uintptr_t>(this);

  const uint32_t flags = TEXFMT_R8 | TEXCF_CLEAR_ON_CREATE | TEXCF_UNORDERED;

  // +2z for toroidal trilinear filtering
  volTexAtlas =
    dag::create_voltex(cfg.xzDim, cfg.xzDim, (cfg.yDim + 2) * cfg.cascadeCount, flags, 1, "voxel_shadows_tex", RESTAG_SHADOW);

  G_ASSERT(c.cascades.empty() || c.cascades.size() == c.cascadeCount);
  G_ASSERT(c.voxelSize > 0.f && c.voxelSizeMul > 0.f);

  for (int i = 0; i < c.cascadeCount; ++i)
  {
    CascadeSettings cs = c.cascades.empty() ? CascadeSettings() : c.cascades[i];

    ToroidalVoxelGrid::Settings gs = {
      .xzDim = c.xzDim,
      .yDim = c.yDim,
      .moveThreshold = cs.moveThreshold > 0 ? cs.moveThreshold : c.moveThreshold,
      .voxelSize = cs.voxelSize > 0 ? cs.voxelSize : c.voxelSize * powf(c.voxelSizeMul, i),
      .heightOffsetRatio = c.heightOffsetRatio,
    };
    grids.emplace_back(gs);
    gridStates.push_back(ToroidalVoxelGrid::UpdateResult::NONE);
    computeOrderLastIdx = max(computeOrderLastIdx, cs.computeGroupOrder);
    if (cs.sparseDiv > 0)
      haveSparse = true;

    G_ASSERT(cs.sparseDiv == 0 || (c.xzDim % cs.sparseDiv) == 0 && (c.yDim % cs.sparseDiv) == 0);
    G_ASSERT(cs.computeGroupOrder < cfg.cascadeCount);

    sparseProfilerNames[i].append_sprintf("voxel_shadows_sparse_%d", i);
    toroidalProfilerNames[i].append_sprintf("voxel_shadows_toroidal_%d", i);
  }
  computeOrderLastIdx++;

  fullUpdateCs.reset(new_compute_shader("voxel_shadows_full_update"));
  partialUpdateCs.reset(new_compute_shader("voxel_shadows_partial_update"));
  sparseFullUpdateCs.reset(new_compute_shader("voxel_shadows_sparse_full_update"));
  copyPaddingCs.reset(new_compute_shader("voxel_shadows_copy_padding"));

  G_ASSERT_RETURN(fullUpdateCs && partialUpdateCs && sparseFullUpdateCs && copyPaddingCs, );
  validate_warp_size(fullUpdateCs.get(), cfg);
  validate_warp_size(partialUpdateCs.get(), cfg);
  validate_warp_size(sparseFullUpdateCs.get(), cfg);
  validate_warp_size(copyPaddingCs.get(), cfg);

  d3d::SamplerInfo smpInfo;
  smpInfo.filter_mode = d3d::FilterMode::Linear;
  smpInfo.mip_map_mode = d3d::MipMapMode::Linear;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
  ShaderGlobal::set_sampler(::get_shader_variable_id("voxel_shadows_tex_samplerstate"), d3d::request_sampler(smpInfo));

  const ToroidalVoxelGrid::ShaderVars &gv = grids[0].getShaderVars();
  voxel_shadows_cascade_countVarId.set_int(cfg.cascadeCount);
  ShaderVariableInfo("voxel_shadows_tex_dim").set_int4(gv.texDim);
  ShaderVariableInfo("voxel_shadows_vignette_scale").set_float4(gv.vignetteTcScale.x, gv.vignetteTcScale.y, 0, 0);

  convar::height_offset_ratio.set(cfg.heightOffsetRatio);
}

VoxelShadows::~VoxelShadows() { voxel_shadows_cascade_countVarId.set_int(0); }

void VoxelShadows::updateOrigin(const Point3 &pos)
{
#if DAGOR_DBGLEVEL > 0
  if (convar::height_offset_ratio.pullValueChange() || convar::smooth_cascade_transition.pullValueChange())
  {
    for (ToroidalVoxelGrid &g : grids)
    {
      ToroidalVoxelGrid::Settings c = g.getSettings();
      c.heightOffsetRatio = convar::height_offset_ratio;
      c.useSmoothCascadeTransition = convar::smooth_cascade_transition;
      g.changeSettings(c);
    }
  }

  if (convar::pause_origin_update)
    return;

  if (cfg.oneToroidalUpdatePerFrame)
  {
    for (ToroidalVoxelGrid::UpdateResult r : gridStates)
      G_ASSERT(r == ToroidalVoxelGrid::UpdateResult::NONE);
  }
#endif

  lastViewPos = pos;

  for (int i = 0; i < cfg.cascadeCount; ++i)
  {
    // we need offset in case of sparse update to guarantee of update for all cascades
    uint32_t idx = (i + originUpdateCascadeIdx) % cfg.cascadeCount;
    ToroidalVoxelGrid::UpdateResult r = grids[idx].updateOrigin(pos, ToroidalVoxelGrid::UpdateType::ONE_DIRECTION);
    if (r != ToroidalVoxelGrid::UpdateResult::NONE)
    {
      gridStates[idx] = r;
      if (cfg.oneToroidalUpdatePerFrame && !is_force_full_update())
      {
        originUpdateCascadeIdx++;
        break;
      }
    }
  }
}

void VoxelShadows::calculateVolumes()
{
  TIME_D3D_PROFILE(voxel_shadows_calculate_volumes);

  uint32_t needToroidalUpdate = 0;
  for (int i = 0; i < cfg.cascadeCount; ++i)
    needToroidalUpdate |= (gridStates[i] != ToroidalVoxelGrid::UpdateResult::NONE ? 1 : 0);

  if (!needToroidalUpdate && !haveSparse && !is_force_full_update())
    return;

  carray<IPoint4, maxCascadeCount> origins;
  carray<Color4, maxCascadeCount> originsTc;
  carray<Color4, maxCascadeCount> voxelSizes;
  carray<Color4, maxCascadeCount> worldToTcs;
  carray<IPoint4, maxCascadeCount> sparseDivs;
  carray<Color4, maxCascadeCount> wvpInTcSpaces;

  // technically in case of one compute at the time we only need to update 1 cascade data
  // but this can lead to trash data (which should be unused) in vars, maybe do it later, after all other issues are resolved
  for (int i = 0; i < cfg.cascadeCount; ++i)
  {
    grids[i].updateWorldViewPos(lastViewPos);

    const ToroidalVoxelGrid::ShaderVars &v = grids[i].getShaderVars();
    origins[i] = v.origin;
    voxelSizes[i] = Color4(v.voxelSize.x, v.voxelSize.y, 0, 0);
    worldToTcs[i] = Color4(v.worldToTc.x, v.worldToTc.y, 0, 0);
    originsTc[i] = Color4(v.originInTcSpace.x, v.originInTcSpace.y, v.originInTcSpace.z, 0);
    wvpInTcSpaces[i] = Color4(v.wvpInTcSpaceClamped.x, v.wvpInTcSpaceClamped.y, v.wvpInTcSpaceClamped.z, 0);

    float temporal = convar::temporal_speed_mul * cfg.cascades[i].temporalSpeed;
    sparseDivs[i] = IPoint4(cfg.cascades[i].sparseDiv, *reinterpret_cast<int *>(&temporal), 0, 0);
  }

  voxel_shadows_originVarId.set_int4_array(origins.data(), cfg.cascadeCount);
  voxel_shadows_origin_in_tc_spaceVarId.set_float4_array(originsTc.data(), cfg.cascadeCount);
  voxel_shadows_wvp_in_tc_spaceVarId.set_float4_array(wvpInTcSpaces.data(), cfg.cascadeCount);
  voxel_shadows_voxel_sizeVarId.set_float4_array(voxelSizes.data(), cfg.cascadeCount);
  voxel_shadows_world_to_tcVarId.set_float4_array(worldToTcs.data(), cfg.cascadeCount);
  voxel_shadows_rndVarId.set_int4(_rnd(rndSeed), _rnd(rndSeed), _rnd(rndSeed), _rnd(rndSeed));
  voxel_shadows_sparse_divVarId.set_int4_array(sparseDivs.data(), cfg.cascadeCount);
  volTexAtlas.setVar();

  int updated = 0;
  if (needToroidalUpdate || is_force_full_update())
  {
    for (int i = 0; i < cfg.cascadeCount; ++i)
    {
      if (gridStates[i] == ToroidalVoxelGrid::UpdateResult::NONE && !is_force_full_update())
        continue;

      DA_PROFILE_NAMED_GPU_EVENT(toroidalProfilerNames[i].c_str());

      voxel_shadows_cascadeIdxVarId.set_int(i);
      voxel_shadows_allow_dynamicVarId.set_int(cfg.cascades[i].allowDynamic ? 1 : 0);
      if (gridStates[i] == ToroidalVoxelGrid::UpdateResult::FULL_UPDATE || is_force_full_update())
      {
        fullUpdateCs->dispatchThreads(cfg.xzDim, cfg.xzDim, cfg.yDim);
      }
      else if (gridStates[i] == ToroidalVoxelGrid::UpdateResult::PARTIAL_UPDATE)
      {
        const dag::Vector<ToroidalVoxelGrid::Region> &regs = grids[i].getRegions();
        for (int ri = 0; ri < regs.size(); ++ri)
        {
          const ToroidalVoxelGrid::Region &r = regs[ri];
          voxel_shadows_update_box_ofsVarId.set_int4(r.offs.x, r.offs.y, r.offs.z, 0);
          voxel_shadows_update_box_sizeVarId.set_int4(r.size.x, r.size.y, r.size.z, 0);
          partialUpdateCs->dispatchThreads(r.size.x, r.size.z, r.size.y);
        }
      }
      else
      {
        G_ASSERT(false);
      }

      // TODO: we only need copy padding if we was updating top or bottom layer of cascade
      d3d::resource_barrier({volTexAtlas.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
      copyPaddingCs->dispatchThreads(cfg.xzDim, cfg.xzDim, 1);

      gridStates[i] = ToroidalVoxelGrid::UpdateResult::NONE;
      updated++;
    }
  }

  G_ASSERT(!cfg.oneToroidalUpdatePerFrame || updated <= 1 || is_force_full_update());

  if (cfg.allowSparseAfterToroidal || updated == 0)
  {
    updated = 0;
    for (int i = 0; i < cfg.cascadeCount; ++i)
    {
      int sparseDiv = cfg.cascades[i].sparseDiv;
      if (sparseDiv == 0 || cfg.cascades[i].computeGroupOrder != computeOrderCascadeIdx)
        continue;

      DA_PROFILE_NAMED_GPU_EVENT(sparseProfilerNames[i].c_str());

      voxel_shadows_cascadeIdxVarId.set_int(i);
      voxel_shadows_allow_dynamicVarId.set_int(cfg.cascades[i].allowDynamic ? 1 : 0);

      sparseFullUpdateCs->dispatchThreads(cfg.xzDim / sparseDiv, cfg.xzDim / sparseDiv, cfg.yDim / sparseDiv);

      d3d::resource_barrier({volTexAtlas.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
      copyPaddingCs->dispatchThreads(cfg.xzDim, cfg.xzDim, 1);

      updated++;
    }

    computeOrderCascadeIdx = (computeOrderCascadeIdx + 1) % computeOrderLastIdx;
  }

  G_ASSERT(updated > 0);

  d3d::resource_barrier({volTexAtlas.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
  invalidatedLastFrame = false;
}

void VoxelShadows::invalidate()
{
  for (ToroidalVoxelGrid &g : grids)
    g.invalidate();
  invalidatedLastFrame = true;
}

bool VoxelShadows::is_force_full_update() const { return convar::force_full_update || invalidatedLastFrame; }

void VoxelShadows::renderDebugOpt()
{
  if (convar::debug_voxels)
  {
    if (!voxelDebugRenderer)
    {
      voxelDebugRenderer.reset(new PostFxRenderer());
      voxelDebugRenderer->init("voxel_shadows_debug_voxels");
    }
    voxelDebugRenderer->render();
  }

  if (convar::debug_probes || convar::debug_probes_all || convar::debug_probes_cascade >= 0)
  {
    if (!probesDebugShader)
    {
      probesDebugShader.reset(new DynamicShaderHelper());
      probesDebugShader->init("voxel_shadows_probe_debug", NULL, 0, "voxel_shadows_probe_debug");
    }

    if (probesDebugShader->shader)
    {
      voxel_shadows_debug_thresholdVarId.set_float(convar::debug_probes_threshold);
      int type = -1;
      if (convar::debug_probes)
        type = 0;
      else if (convar::debug_probes_all)
        type = 1;
      else if (convar::debug_probes_cascade >= 0)
        type = 2;
      else
        G_ASSERT(false);
      voxel_shadows_debug_typeVarId.set_int(type);

      d3d::setvsrc(0, 0, 0);
      bool oneCascade = convar::debug_probes_cascade >= 0;
      for (int i = 0; i < cfg.cascadeCount; ++i)
      {
        int c = oneCascade ? convar::debug_probes_cascade : cfg.cascadeCount - i - 1;
        voxel_shadows_debug_cascadeVarId.set_int(c);
        probesDebugShader->shader->setStates(0, true);
        d3d::draw_instanced(PRIM_TRILIST, 0, LOW_SPHERES_INDICES_TO_DRAW, cfg.xzDim * cfg.xzDim * cfg.yDim);
        if (oneCascade)
          break;
      }
    }
  }
}
// TODO: add invalid args!!
