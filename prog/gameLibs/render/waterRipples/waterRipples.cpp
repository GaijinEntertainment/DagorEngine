// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/waterRipples.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_shaders.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>

static const float STEP_DT = 1.0f / 60.0f;
static const float STEP_HYSTERESIS = 0.75f;
static const float STEP_0_HYSTERESIS = 0.9f;
static const float STEP_1_HYSTERESIS = 0.75f;
static const float MAX_STEPS_NUM = 2.0f;
static const float MAX_DROPS_ALIVE_TIME = 20.0f;
static const int MAX_DROPS_PER_BATCH = 64;


WaterRipples::WaterRipples(float world_size, int simulation_tex_size, float water_displacement_max, bool use_flowmap) :
  worldSize(world_size), texSize(simulation_tex_size), steps(), texBuffers(), drops(midmem)
{
  drops.reserve(MAX_DROPS_PER_BATCH);
  texBuffers[0] = dag::create_tex(NULL, texSize, texSize, TEXFMT_R16F | TEXCF_RTARGET, 1, "water_ripples_0");
  texBuffers[1] = dag::create_tex(NULL, texSize, texSize, TEXFMT_R16F | TEXCF_RTARGET, 1, "water_ripples_1");
  texBuffers[2] = dag::create_tex(NULL, texSize, texSize, TEXFMT_R16F | TEXCF_RTARGET, 1, "water_ripples_2");

  // normal texture 2 times larger than solution texture
  heightNormalTexture =
    UniqueTexHolder(dag::create_tex(NULL, texSize * 2, texSize * 2, TEXFMT_A2R10G10B10 | TEXCF_RTARGET, 1, "water_ripples_normal"),
      get_shader_variable_id("water_ripples_normal", true));
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    ShaderGlobal::set_sampler(::get_shader_glob_var_id("water_ripples_normal_samplerstate", true), d3d::request_sampler(smpInfo));
  }

  dropsBuf = dag::buffers::create_one_frame_cb(MAX_DROPS_PER_BATCH, "water_ripples_drops_buf");

  lastStepOrigin[1] = Point2(0, 0);
  lastStepOrigin[0] = Point2(0, 0);

  updateRenderer.init("water_ripples_update");
  resolveRenderer.init("water_ripples_resolve");

  texelSize = world_size / texSize;

  useFlowmap = use_flowmap;

  waterRipplesOnVarId = ::get_shader_glob_var_id("water_ripples_on", true);
  ShaderGlobal::set_real(waterRipplesOnVarId, 0.0f);

  waterRipplesDropCountVarId = ::get_shader_glob_var_id("water_ripples_drop_count");
  waterRipplesOriginVarId = ::get_shader_glob_var_id("water_ripples_origin");
  waterRipplesOriginDeltaVarId = ::get_shader_glob_var_id("water_ripples_origin_delta");
  waterRipplesDisplaceMaxVarId = ::get_shader_glob_var_id("water_ripples_displace_max", true);

  waterRipplesT1VarId = ::get_shader_glob_var_id("water_ripples_t1");
  waterRipplesT1_samplerstateVarId = ::get_shader_glob_var_id("water_ripples_t1_samplerstate");
  waterRipplesT2VarId = ::get_shader_glob_var_id("water_ripples_t2");
  waterRipplesT2_samplerstateVarId = ::get_shader_glob_var_id("water_ripples_t2_samplerstate");
  waterRipplesDropsVarId = ::get_shader_glob_var_id("water_ripples_drops");

  waterRipplesFrameNoVarId = ::get_shader_glob_var_id("water_ripples_frame_no");
  waterRipplesFlowmapFrameCountVarId = ::get_shader_glob_var_id("water_ripples_flowmap_frame_count");
  waterRipplesFlowmapVarId = ::get_shader_glob_var_id("water_ripples_flowmap", true);

  ShaderGlobal::set_real(::get_shader_glob_var_id("water_ripples_pixel_size"), 1.0f / texSize);
  ShaderGlobal::set_real(::get_shader_glob_var_id("water_ripples_size"), worldSize);
  ShaderGlobal::set_real(waterRipplesDisplaceMaxVarId, water_displacement_max);
  ShaderGlobal::set_int(waterRipplesFlowmapFrameCountVarId, flowmapFrameCount);
  ShaderGlobal::set_int(waterRipplesFlowmapVarId, useFlowmap ? 1 : 0);

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    waterRipplesLinearSampler = d3d::request_sampler(smpInfo);
    smpInfo.filter_mode = d3d::FilterMode::Point;
    waterRipplesPointSampler = d3d::request_sampler(smpInfo);
  }
  ShaderGlobal::set_sampler(waterRipplesT1_samplerstateVarId, waterRipplesPointSampler);
  ShaderGlobal::set_sampler(waterRipplesT2_samplerstateVarId, waterRipplesPointSampler);
}

WaterRipples::~WaterRipples() { ShaderGlobal::set_real(waterRipplesOnVarId, 0.0f); }

void WaterRipples::placeDrop(const Point2 &pos, float strength, float radius)
{
  G_ASSERT_RETURN(radius > 0.0f, );

  Point4 &drop = drops.push_back();
  drop = Point4(pos.x, pos.y, radius / texSize, strength);
  addDropInst(1);
}

void WaterRipples::placeSolidBox(const Point2 &pos, const Point2 &size, const Point2 &local_x, const Point2 &local_y, float strength,
  float radius)
{
  G_ASSERT(radius > 0.0f);

  Point4 &drop = drops.push_back();
  drop = Point4(pos.x, pos.y, -radius / texSize, strength);
  Point4 &localXY = drops.push_back();
  localXY = Point4(local_x.x * size.x, local_x.y * size.x, local_y.x * size.y, local_y.y * size.y) * 0.5f / worldSize;
  addDropInst(2);
}

// Corrected means the radius should be divided by the world size, instead of the texture size.
// Cant change this in the original functions as that would affect WT, and as the values were
// set up with this error in mind, it would mess up WT ripples.
// This should be fixen in WT later.
void WaterRipples::placeDropCorrected(const Point2 &pos, float strength, float radius)
{
  placeDrop(pos, strength, (radius / worldSize) * texSize);
}

void WaterRipples::placeSolidBoxCorrected(const Point2 &pos, const Point2 &size, const Point2 &local_x, const Point2 &local_y,
  float strength, float radius)
{
  placeSolidBox(pos, size, local_x, local_y, strength, (radius / worldSize) * texSize);
}

void WaterRipples::reset()
{
  firstStep = true;
  dropsAliveTime = 0;
  curBuffer = 0;
  mem_set_0(steps);
  curStepNo = 0;
  accumStep = 0.0f;
  curNumSteps = 1;
}

void WaterRipples::updateSteps(float dt)
{
  if (inSleep)
  {
    curNumSteps = 1;
    nextNumSteps = 1;
    accumStep = 0.0f;
    curStepNo = 0;
    return;
  }

  steps[(curStepNo++) % steps.size()] = dt;
  float stepAvg = 0.0f;
  for (int stepNo = 0; stepNo < steps.size(); ++stepNo)
    stepAvg += steps[stepNo];
  stepAvg /= steps.size();

  float fNumSteps = min(stepAvg / STEP_DT, MAX_STEPS_NUM);
  if (curNumSteps == 0 && fNumSteps > STEP_0_HYSTERESIS)
  {
    curNumSteps = 1;
    accumStep = 0.0f;
  }
  else if (curNumSteps == 1 && fNumSteps < STEP_1_HYSTERESIS)
  {
    curNumSteps = 0;
    accumStep = 1.0f;
  }
  else if (fabsf((float)curNumSteps - fNumSteps) > STEP_HYSTERESIS)
  {
    curNumSteps = floorf(fNumSteps + 0.5f);
    accumStep = 0.0f;
  }

  nextNumSteps = curNumSteps;
  if (nextNumSteps == 0)
  {
    accumStep += fNumSteps;
    nextNumSteps = floorf(accumStep);
    accumStep -= nextNumSteps;
  }
}

inline int get_buf_idx(int current, int layer) { return (current + (3 - layer)) % 3; }

void WaterRipples::advance(float dt, const Point2 &origin_)
{
  Point2 origin = origin_;
  origin.x = texelSize * floor(origin.x / texelSize);
  origin.y = texelSize * floor(origin.y / texelSize);

  const bool cleared = firstStep;
  if (firstStep)
  {
    firstStep = false;
    clearRts();
  }

  dropsAliveTime = drops.size() > 0 ? MAX_DROPS_ALIVE_TIME : dropsAliveTime;
  if (dropsAliveTime <= 0.0f)
  {
    if (!inSleep)
    {
      inSleep = true;
      if (!cleared)
        clearRts();
      dropsAliveTime = 0.0f;
      ShaderGlobal::set_real(waterRipplesOnVarId, 0.0f);
    }
    return;
  }
  else if (inSleep)
  {
    inSleep = false;
    ShaderGlobal::set_real(waterRipplesOnVarId, 1.0f);
  }
  dropsAliveTime -= dt;

  heightNormalTexture.setVar();

  SCOPE_RENDER_TARGET;

  curBuffer = get_buf_idx(curBuffer, 0);

  for (int stepNo = 0; stepNo < nextNumSteps; ++stepNo)
  {
    Point2 originDelta1(0.0f, 0.0f);
    Point2 originDelta2(0.0f, 0.0f);
    {
      originDelta1 = (origin - lastStepOrigin[0]) / worldSize;
      originDelta2 = (origin - lastStepOrigin[1]) / worldSize;
      lastStepOrigin[1] = lastStepOrigin[0];
      lastStepOrigin[0] = origin;
    }

    if (dropsInst.size() > 0)
    {
      dropsBuf->updateData(0, dropsInst[0] * sizeof(Point4), drops.data(), VBLOCK_DISCARD);
      ShaderGlobal::set_buffer(waterRipplesDropsVarId, dropsBuf.getBufId());
      ShaderGlobal::set_int(waterRipplesDropCountVarId, dropsInst[0]);
      erase_items(drops, 0, dropsInst[0]);
      erase_items(dropsInst, 0, 1);
    }
    else
    {
      ShaderGlobal::set_buffer(waterRipplesDropsVarId, BAD_D3DRESID);
      ShaderGlobal::set_int(waterRipplesDropCountVarId, 0);
    }

    ShaderGlobal::set_int(waterRipplesFrameNoVarId, frameNo);

    ShaderGlobal::set_color4(waterRipplesOriginVarId, Color4(origin.x, 0.0f, origin.y, 0.0f));
    ShaderGlobal::set_color4(waterRipplesOriginDeltaVarId, originDelta1.x, originDelta1.y, originDelta2.x, originDelta2.y);

    // if we using flowmap, on some steps we need smooth filtering of texture buffers
    if (useFlowmap)
    {
      // N'th step has only reading with linear filtering
      if (frameNo == 0)
        ShaderGlobal::set_sampler(waterRipplesT1_samplerstateVarId, waterRipplesLinearSampler);
      else
        ShaderGlobal::set_sampler(waterRipplesT1_samplerstateVarId, waterRipplesPointSampler);

      // (N-1)'th step has two readings with linear filtering
      if (frameNo <= 1)
        ShaderGlobal::set_sampler(waterRipplesT2_samplerstateVarId, waterRipplesLinearSampler);
      else
        ShaderGlobal::set_sampler(waterRipplesT2_samplerstateVarId, waterRipplesPointSampler);
    }

    ShaderGlobal::set_texture(waterRipplesT1VarId, texBuffers[get_buf_idx(curBuffer, 1)].getTexId());
    ShaderGlobal::set_texture(waterRipplesT2VarId, texBuffers[get_buf_idx(curBuffer, 2)].getTexId());

    d3d::set_render_target(texBuffers[curBuffer].getTex2D(), 0);
    updateRenderer.render();
    d3d::resource_barrier({texBuffers[curBuffer].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    if (stepNo == 0) // update normal texture only once per update cycle
    {
      d3d::set_render_target(heightNormalTexture.getTex2D(), 0);
      ShaderGlobal::set_texture(waterRipplesT1VarId, texBuffers[get_buf_idx(curBuffer, 0)].getTexId());
      ShaderGlobal::set_sampler(waterRipplesT1_samplerstateVarId, waterRipplesLinearSampler);
      resolveRenderer.render();

      d3d::resource_barrier({heightNormalTexture.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }

    curBuffer = get_buf_idx(curBuffer + 1, 0);
    frameNo = (frameNo + 1) % flowmapFrameCount;
  }
}

void WaterRipples::addDropInst(int reg_num)
{
  if (dropsInst.size() == 0 || (dropsInst.back() + reg_num) > MAX_DROPS_PER_BATCH)
    dropsInst.push_back(0);
  dropsInst.back() += reg_num;
}

void WaterRipples::clearRts()
{
  SCOPE_RENDER_TARGET;
  d3d::set_render_target();
  d3d::set_render_target(0, texBuffers[0].getTex2D(), 0);
  d3d::set_render_target(1, texBuffers[1].getTex2D(), 0);
  d3d::set_render_target(2, texBuffers[2].getTex2D(), 0);
  d3d::clearview(CLEAR_TARGET, 0, 0, 0);
  d3d::set_render_target(heightNormalTexture.getTex2D(), 0);
  d3d::clearview(CLEAR_TARGET, E3DCOLOR(127, 127, 127), 0, 0);

  d3d::resource_barrier({texBuffers[0].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({texBuffers[1].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({texBuffers[2].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({heightNormalTexture.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}
