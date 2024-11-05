// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_computeShaders.h>
#include <render/beamTracers.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResSystem.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <frustumCulling/frustumPlanes.h>

#define GLOBAL_VARS_LIST       \
  VAR(beam_trace_current_time) \
  VAR(beam_fade_dist)          \
  VAR(beam_min_intensity_cos)  \
  VAR(beam_max_intensity_cos)  \
  VAR(beam_min_intensity_mul)  \
  VAR(beam_max_intensity_mul)  \
  VAR(beam_fog_intensity_mul)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

void BeamTracerManager::updateAlive()
{
  int nextUsed = (1 - currentUsed) & 1;
  int count = 0;

  usedTracers[nextUsed].resize(usedTracers[currentUsed].size());
  for (int i = 0; i < usedTracers[currentUsed].size(); ++i)
  {
    int id = usedTracers[currentUsed][i];
    if (tracersVaporTime[id] > 0.0f)
    {
      tracersVaporTime[id] -= lastDt;
      usedTracers[nextUsed][count++] = id;
    }
    else
    {
#if DAGOR_DBGLEVEL > 0
      if (!tracersLeft.get(id))
      {
        // logerr("Can't auto remove beamTracer because it wasn't removed via leaveTracer()");
        tracersLeft.set(id); // update or remove will cause assert, so call stack will be shown. Otherwise it's leak
      }
      else
#endif
      // if (tracersLeft.get())//we can auto add dead tracers, since we we rely on them being updated each frame
      {
        freeTracers.push_back(id);
      }
    }
  }
  usedTracers[nextUsed].resize(count);
  currentUsed = nextUsed;
}

void BeamTracerManager::closeGPU()
{
  tailTex.close();
  beamTex.close();
  tracerBuffer.close();
  tracerVertsBuffer.close();
  tracerBufferDynamic.close();
  culledTracerTails.close();
  culledTracerHeads.close();
  drawIndirectBuffer.close();
}

void BeamTracerManager::initGPU(const DataBlock &settings)
{
  closeGPU();

#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR

  const char *texName = settings.getStr("tail_texture", "tracer_tail_density");
  tailTex = dag::get_tex_gameres(texName, "beam_tracer_tail_tex");

  beamTex = dag::get_tex_gameres("misc_ribbon_trail_1_tex_a", "beam_tracer_tex");
  // beamTex = dag::get_tex_gameres("soldier_teeth_tex_d", "beam_tracer_tex");
  if (beamTex)
    beamTex->texaddr(TEXADDR_WRAP);

  updateCommands_cs.reset(new_compute_shader("update_beam_tracer_commands_cs"));
  createCommands_cs.reset(new_compute_shader("create_beam_tracer_commands_cs"));
  cullTracers_cs.reset(new_compute_shader("cull_beam_tracers_cs"));
  clearIndirectBuffers.reset(new_compute_shader("clear_beam_tracers_indirect_buffers"));
  G_ASSERT(cullTracers_cs);
  G_ASSERT(createCommands_cs);
  G_ASSERT(updateCommands_cs);
  G_ASSERT(clearIndirectBuffers);

  tracerBuffer = dag::buffers::create_ua_sr_structured(sizeof(GPUBeamTracer), MAX_TRACERS, "beamTracers");
  tracerVertsBuffer =
    dag::buffers::create_ua_sr_structured(sizeof(GPUBeamTracerVertices), TRACER_SEGMENTS_COUNT * MAX_TRACERS, "beamTracerVerts");
  tracerBufferDynamic = dag::buffers::create_ua_sr_structured(sizeof(GPUBeamTracerDynamic), MAX_TRACERS, "beamTracersDynamic");
  culledTracerTails = dag::buffers::create_ua_sr_structured(sizeof(GPUBeamTracerTailRender), MAX_TRACERS, //*2
    "culledTracerTails");
  culledTracerHeads = dag::buffers::create_ua_sr_structured(sizeof(GPUBeamTracerHeadRender), MAX_TRACERS, "culledTracerHeads");
  drawIndirectBuffer = dag::buffers::create_ua_indirect(dag::buffers::Indirect::Draw, 2, "culledTracersIndirect");

  headRenderer.init("beam_tracers_head", NULL, 0, "beam_tracers_head");
  tailRenderer.init("beam_tracers_tail", NULL, 0, "beam_tracers_tail");

  initFillIndirectBuffer();

  mem_set_0(usedTracers[0]);
  mem_set_0(usedTracers[1]);
}

void BeamTracerManager::initFillIndirectBuffer()
{
  if (!drawIndirectBuffer)
    return;

  // struct args{   uint vertexCountPerInstance,InstanceCount, StartVertexLocation, StartInstanceLocation;}
  uint32_t *data;
  int ret = drawIndirectBuffer->lock(0, 0, (void **)&data, VBLOCK_WRITEONLY);
  d3d_err(ret);
  if (!ret || !data)
    return;
  data[0] = 14;
  data[1] = data[2] = data[3] = 0;

  data[4] = 2 * TRACER_RIBBON_SEGMENTS_COUNT;
  data[5] = data[6] = data[7] = 0;
  drawIndirectBuffer->unlock();
}

void BeamTracerManager::afterDeviceReset() { initFillIndirectBuffer(); }

void BeamTracerManager::leaveTracer(unsigned id)
{
  // bullshit. we have to continue updating tracers until they die
  G_ASSERT(id < tracers.size());
  if (id >= tracers.size())
    return;
  // tracersVaporTime[id] = tracers[id].vaporTimeInterval;
  /*
  #if DAGOR_DBGLEVEL>0
  G_ASSERT(!tracersLeft.get(id));
  tracersLeft.set(id);
  G_ASSERT(tracersLeft.get(id));
  #endif
  */
}

void BeamTracerManager::removeTracer(unsigned id)
{
  G_ASSERT(id < tracers.size());
  tracersVaporTime[id] = 0;
}

int BeamTracerManager::updateTracerPos(unsigned id, const Point3 &pos, const Point3 &start_pos)
{
  G_ASSERT(id < tracers.size());
  if (id >= tracers.size())
    return -1;
#if DAGOR_DBGLEVEL > 0
  // G_ASSERT(!tracersLeft.get(id));
  tracersLeft.reset(id);
#endif
  tracers[id].lastPos = pos;
  // tracersVaporTime[id] = tracers[id].vaporTimeInterval;
  updateCommands.push_back(BeamTracerUpdateCommand(id, pos, start_pos));
  return id;
}

int BeamTracerManager::createTracer(const Point3 &start_pos, const Point3 &ndir, float radius, const Color4 &smoke_color,
  const Color4 &head_color, float time_to_live, float fade_dist, float begin_fade_time, float end_fade_time, float scroll_speed,
  bool is_ray)
{
  if (DAGOR_UNLIKELY(!tracerVertsBuffer))
  {
    const DataBlock *tracersBlk = dgs_get_settings()->getBlockByNameEx("tracers");
    if (!tracersBlk)
      return -1;
    init(*tracersBlk);
  }

  G_UNUSED(smoke_color);
  if (!freeTracers.size())
  {
    debug("no free");
    return -1;
  }
  if (head_color.a < 0)
    logerr("negative tracer head ttl, head burnTime = %f", head_color.a);
  if (time_to_live < 0)
    logerr("negative tracer smoke ttl, smoke vaporTime = %f", time_to_live);
  int id = freeTracers.back();
  freeTracers.pop_back();
  tracers[id].startPos = tracers[id].lastPos = start_pos;
  tracers[id].vaporTimeInterval = time_to_live;
#if DAGOR_DBGLEVEL > 0
  tracersLeft.reset(id);
#endif
  tracersVaporTime[id] = time_to_live;
  float4 head_color_(head_color.r, head_color.g, head_color.b, head_color.a);
  createCommands.push_back(BeamTracerCreateCommand(id, start_pos, ndir * radius, time_to_live, head_color_, fade_dist,
    is_ray ? -begin_fade_time : begin_fade_time, end_fade_time, scroll_speed));
  // v_bbox3_init(tracersBox[id], v_ldu(&start_pos.x));
  // v_bbox3_add_pt(tracersBox[id], v_ldu(&next_pos.x));

  usedTracers[currentUsed].push_back(id);
  return id;
}
void BeamTracerManager::performGPUCommands()
{
  if (!createCommands_cs)
  {
    createCommands.clear();
    updateCommands.clear();
    return;
  }
  if (!createCommands.size() && !updateCommands.size())
    return;
  // ShaderGlobal::set_real(beam_trace_current_timeVarId, cTime);
  float time[4] = {cTime, lastDt, 0, 0};
  d3d::set_cs_const(2, (float *)time, 1);
  uint32_t v[4] = {0};

  d3d::set_rwbuffer(STAGE_CS, 0, tracerVertsBuffer.get());
  d3d::set_rwbuffer(STAGE_CS, 1, tracerBufferDynamic.get());
  if (createCommands.size())
  {
    TIME_D3D_PROFILE(create_commands);
    d3d::set_rwbuffer(STAGE_CS, 2, tracerBuffer.get());
    // debug("%d createCommands", createCommands.size());
    G_STATIC_ASSERT(sizeof(createCommands[0]) % sizeof(float4) == 0);
    G_STATIC_ASSERT(sizeof(createCommands[0]) == TRACER_BEAM_CREATE_COMMAND_SIZE * sizeof(float4));
    // it is rare we create more than 28 tracers each frame, so use common constant buffer
    const int command_size_in_consts = (elem_size(createCommands) + 15) / 16;
    const int req_size = 4 + createCommands.size() * command_size_in_consts;
    const int cbuffer_size = d3d::set_cs_constbuffer_size(req_size);
    // debug("cbuffer_size = %d req = %d", cbuffer_size, req_size);
    for (int i = 0; i < createCommands.size(); i += (cbuffer_size - 4) / command_size_in_consts)
    {
      int batch_size = min<int>(createCommands.size() - i, (cbuffer_size - 4) / command_size_in_consts);
      // debug("batch_size = %d", batch_size);
      v[0] = batch_size;
      d3d::set_cs_const(3, (float *)v, 1);
      d3d::set_cs_const(4, (float *)&createCommands[i], batch_size * command_size_in_consts);
      createCommands_cs->dispatch((batch_size + TRACER_COMMAND_WARP_SIZE - 1) / TRACER_COMMAND_WARP_SIZE, 1, 1);
    }
    d3d::set_cs_constbuffer_size(0);
    if (updateCommands.size())
    {
      d3d::resource_barrier({tracerVertsBuffer.get(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
      d3d::resource_barrier({tracerBufferDynamic.get(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    }
    else
    {
      d3d::resource_barrier({tracerVertsBuffer.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
      d3d::resource_barrier({tracerBufferDynamic.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
    }

    createCommands.clear();
    d3d::set_rwbuffer(STAGE_CS, 2, 0);
    d3d::resource_barrier({tracerBuffer.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
  }

  if (updateCommands.size())
  {
    d3d::set_buffer(STAGE_CS, 0, tracerBuffer.get());
    TIME_D3D_PROFILE(update_commands);
    // debug("%d updateCommands", updateCommands.size());
    G_STATIC_ASSERT(sizeof(updateCommands[0]) % sizeof(float4) == 0);
    G_STATIC_ASSERT(sizeof(updateCommands[0]) == TRACER_BEAM_UPDATE_COMMAND_SIZE * sizeof(float4));
    v[0] = updateCommands.size();
    const int command_size_in_consts = (elem_size(updateCommands) + 15) / 16;
    const int startFromReg = 4;
    const int req_size = startFromReg + updateCommands.size() * command_size_in_consts;
    const int cbuffer_size = d3d::set_cs_constbuffer_size(req_size);
    // debug("cbuffer_size = %d req = %d", cbuffer_size, req_size);

    for (int i = 0; i < updateCommands.size(); i += (cbuffer_size - startFromReg) / command_size_in_consts)
    {
      int batch_size = min<int>(updateCommands.size() - i, (cbuffer_size - startFromReg) / command_size_in_consts);
      v[0] = batch_size;
      d3d::set_cs_const(startFromReg - 1, (float *)v, 1);
      d3d::set_cs_const(startFromReg, (float *)&updateCommands[i], batch_size * command_size_in_consts);
      updateCommands_cs->dispatch((batch_size + TRACER_COMMAND_WARP_SIZE - 1) / TRACER_COMMAND_WARP_SIZE, 1, 1);
    }
    d3d::set_cs_constbuffer_size(0);
    updateCommands.clear();
    d3d::set_buffer(STAGE_CS, 0, 0);
    d3d::resource_barrier({tracerVertsBuffer.get(), RB_RO_SRV | RB_STAGE_VERTEX | RB_STAGE_COMPUTE});
  }
  d3d::set_rwbuffer(STAGE_CS, 0, 0);
  d3d::set_rwbuffer(STAGE_CS, 1, 0);
};

void BeamTracerManager::beforeRender(const Frustum &frustum, float exposure_time, float pixel_scale)
{
  if (!cullTracers_cs)
    return;
  performGPUCommands();
  if (!usedTracers[currentUsed].size())
    return;
  TIME_D3D_PROFILE(cull_tracers);
  // gpu culling
  d3d::resource_barrier({tracerBufferDynamic.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
  d3d::resource_barrier({drawIndirectBuffer.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
  d3d::set_buffer(STAGE_CS, 8, tracerBuffer.get());
  d3d::set_buffer(STAGE_CS, 9, tracerVertsBuffer.get());
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), culledTracerHeads.get());
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 1, VALUE), culledTracerTails.get());
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 2, VALUE), tracerBufferDynamic.get()); // only for updating firstVertex
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 3, VALUE), drawIndirectBuffer.get());

  clearIndirectBuffers->dispatch(1, 1, 1);
  d3d::resource_barrier({drawIndirectBuffer.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

  set_frustum_planes(frustum);
  uint v[4] = {0};
  v[0] = usedTracers[currentUsed].size();
  float time[4] = {cTime, lastDt, exposure_time, pixel_scale};
  static const int startFromReg = 14;
  d3d::set_cs_const(startFromReg - 2, (float *)time, 1);
  G_ASSERT(usedTracers[0].static_size % 16 == 0);
  const int tracersPerCommand = 16 / elem_size(usedTracers[currentUsed]);

  const int req_size = startFromReg + (data_size(usedTracers[currentUsed]) + 15) / 16;
  const int cbuffer_size = d3d::set_cs_constbuffer_size(req_size);
  for (int i = 0; i < usedTracers[currentUsed].size(); i += (cbuffer_size - startFromReg) * tracersPerCommand) // up to 4094 consts
                                                                                                               // i.e. up to 4094*8
                                                                                                               // tracers
  {
    int batch_size = min<int>(usedTracers[currentUsed].size() - i, (cbuffer_size - startFromReg) * tracersPerCommand);
    v[0] = batch_size;
    d3d::set_cs_const(startFromReg - 1, (float *)v, 1);
    d3d::set_cs_const(startFromReg, (const float *)&usedTracers[currentUsed][i],
      (elem_size(usedTracers[currentUsed]) * batch_size + 15) / 16);
    cullTracers_cs->dispatch((batch_size + TRACER_CULL_WARP_SIZE - 1) / TRACER_CULL_WARP_SIZE, 1, 1);
  }
  d3d::set_cs_constbuffer_size(0);
  d3d::set_buffer(STAGE_CS, 8, 0);
  d3d::set_buffer(STAGE_CS, 9, 0);
  d3d::set_buffer(STAGE_CS, 10, 0);
}

void BeamTracerManager::renderTrans()
{
  if (!usedTracers[currentUsed].size())
    return;
  d3d::resource_barrier({culledTracerHeads.get(), RB_RO_SRV | RB_STAGE_VERTEX});
  d3d::resource_barrier({culledTracerTails.get(), RB_RO_SRV | RB_STAGE_VERTEX});
  d3d::resource_barrier({drawIndirectBuffer.get(), RB_RO_INDIRECT_BUFFER});
  ShaderGlobal::set_real(beam_trace_current_timeVarId, cTime);
  ShaderGlobal::set_real(beam_fade_distVarId, tacLaserRenderSettings.fadeDistance);
  ShaderGlobal::set_real(beam_min_intensity_cosVarId, tacLaserRenderSettings.minIntensityCos);
  ShaderGlobal::set_real(beam_max_intensity_cosVarId, tacLaserRenderSettings.maxIntensityCos);
  ShaderGlobal::set_real(beam_min_intensity_mulVarId, tacLaserRenderSettings.minIntensityMul);
  ShaderGlobal::set_real(beam_max_intensity_mulVarId, tacLaserRenderSettings.maxIntensityMul);
  ShaderGlobal::set_real(beam_fog_intensity_mulVarId, tacLaserRenderSettings.fogIntensityMul);
  headRenderer.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);
  d3d::set_buffer(STAGE_VS, 8, culledTracerHeads.get());
  d3d::draw_indirect(PRIM_TRISTRIP, drawIndirectBuffer.get(), 0);

  /*
    tailRenderer.shader->setStates(0, true);
    d3d::set_buffer(STAGE_VS, 12, culledTracerTails.getSbuffer());
    d3d::set_buffer(STAGE_VS, 13, tracerVertsBuffer.getSbuffer());
    d3d::draw_indirect(PRIM_TRISTRIP, drawIndirectBuffer.getSbuffer(), sizeof(uint32_t) * DRAW_INDIRECT_NUM_ARGS);
    //d3d::draw(PRIM_TRISTRIP, 0, 32);
    d3d::set_buffer(STAGE_VS, 12, 0);
    d3d::set_buffer(STAGE_VS, 13, 0);
    //d3d::set_buffer(STAGE_VS, 1, 0);
    //d3d::set_buffer(STAGE_VS, 2, 0);
  */
}

BeamTracerManager::BeamTracerManager() : currentUsed(0), cTime(0), lastDt(0)
{
  freeTracers.resize(MAX_TRACERS);
  for (int i = 0; i < MAX_TRACERS; ++i)
  {
    freeTracers[i] = MAX_TRACERS - 1 - i;
    tracersVaporTime[i] = 0.f;
  }
}

BeamTracerManager::~BeamTracerManager() { closeGPU(); }
