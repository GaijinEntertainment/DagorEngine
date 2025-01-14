// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_computeShaders.h>
#include <render/smokeTracers.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResSystem.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <workCycle/dag_workCycle.h>
#include <frustumCulling/frustumPlanes.h>

#define GLOBAL_VARS_LIST VAR(smoke_trace_current_time)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

void SmokeTracerManager::updateAlive()
{
  const int nextUsed = 1 - currentUsed;
  int count = 0;

  auto &nextUsedTracers = usedTracers[nextUsed];
  nextUsedTracers.resize(usedTracers[currentUsed].size());
  for (int id : usedTracers[currentUsed])
  {
    if (aliveTracers.get(id))
    {
      nextUsedTracers[count++] = id;
#if DAGOR_DBGLEVEL > 0
      if (tracers[id].framesWithoutUpdate >= 0)
      {
        tracers[id].framesWithoutUpdate++;
        // Note: allow 1 extra frame miss due to potential delayed nature of projectile entity destruction
        if (tracers[id].framesWithoutUpdate > ::dagor_frame_no() - lastGpuUpdateFrame + 1)
        {
          logerr("Alive tracer with id = %d wasn't updated every frame!", id);
          tracers[id].framesWithoutUpdate = -1;
        }
      }
#endif
    }
    else
    {
      tracers[id].vaporTime -= lastDt;
      if (tracers[id].vaporTime <= 0.0f)
        freeTracers.push_back(id);
      else
        nextUsedTracers[count++] = id;
    }
  }
  nextUsedTracers.resize(count); // Resize down to actual count
  currentUsed = nextUsed;
  G_ASSERT(usedTracers[currentUsed].size() + freeTracers.size() == MAX_TRACERS);
}

void SmokeTracerManager::closeGPU()
{
  tailTex.close();
  tracerBuffer.close();
  tracerVertsBuffer.close();
  tracerBufferDynamic.close();
  culledTracerTails.close();
  culledTracerHeads.close();
  drawIndirectBuffer.close();
}

void SmokeTracerManager::initGPU(const DataBlock &settings)
{
  closeGPU();

#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR

  const char *texName = settings.getStr("tail_texture", "tracer_tail_density");
  tailTex = dag::get_tex_gameres(texName, "smoke_tracer_tail_tex");

  updateCommands_cs.reset(new_compute_shader("update_tracer_commands_cs"));
  createCommands_cs.reset(new_compute_shader("create_tracer_commands_cs"));
  cullTracers_cs.reset(new_compute_shader("cull_tracers_cs"));
  clearIndirectBuffers.reset(new_compute_shader("clear_indirect_buffers"));
  G_ASSERT(cullTracers_cs);
  G_ASSERT(createCommands_cs);
  G_ASSERT(updateCommands_cs);
  G_ASSERT(clearIndirectBuffers);

  tracerBuffer = dag::buffers::create_ua_sr_structured(sizeof(GPUSmokeTracer), MAX_TRACERS, "smokeTracers");
  tracerVertsBuffer =
    dag::buffers::create_ua_sr_structured(sizeof(GPUSmokeTracerVertices), TRACER_SEGMENTS_COUNT * MAX_TRACERS, "smokeTracerVerts");
  tracerBufferDynamic = dag::buffers::create_ua_sr_structured(sizeof(GPUSmokeTracerDynamic), MAX_TRACERS, "smokeTracersDynamic");
  culledTracerTails = dag::buffers::create_ua_sr_structured(sizeof(GPUSmokeTracerTailRender), MAX_TRACERS, //*2
    "culledTracerTails");
  culledTracerHeads = dag::buffers::create_ua_sr_structured(sizeof(GPUSmokeTracerHeadRender), MAX_TRACERS, "culledTracerHeads");
  drawIndirectBuffer = dag::buffers::create_ua_indirect(dag::buffers::Indirect::Draw, 2, "culledTracersIndirect");

  headRenderer.init("smoke_tracers_head", NULL, 0, "smoke_tracers_head");
  tailRenderer.init("smoke_tracers_tail", NULL, 0, "smoke_tracers_tail");

  initFillIndirectBuffer();
}

void SmokeTracerManager::initFillIndirectBuffer()
{
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

void SmokeTracerManager::afterDeviceReset() { initFillIndirectBuffer(); }

void SmokeTracerManager::leaveTracer(unsigned id)
{
  G_ASSERT_RETURN(id < tracers.size(), );
  G_ASSERT(aliveTracers.get(id));
#if _TARGET_PC_WIN && defined(_DEBUG_TAB_)
  G_ASSERT(eastl::find(usedTracers[currentUsed].begin(), usedTracers[currentUsed].end(), id) != usedTracers[currentUsed].end());
#endif
  aliveTracers.reset(id);
}

int SmokeTracerManager::updateTracerPos(unsigned id, const Point3 &pos)
{
  G_ASSERT_RETURN(id < tracers.size(), -1);
  G_ASSERT(aliveTracers.get(id));
#if _TARGET_PC_WIN && defined(_DEBUG_TAB_)
  G_ASSERT(eastl::find(usedTracers[currentUsed].begin(), usedTracers[currentUsed].end(), id) != usedTracers[currentUsed].end());
#endif
  tracers[id].lastPos = pos;
#if DAGOR_DBGLEVEL > 0
  tracers[id].framesWithoutUpdate = 0;
#endif
  updateCommands.push_back(TracerUpdateCommand(id, pos));
  return id;
}

int SmokeTracerManager::createTracer(const Point3 &start_pos, const Point3 &ndir, float radius, const Color4 &smoke_color,
  const Color4 &head_color, float time_to_live, const Color3 &start_head_color, float start_time)
{
  if (!freeTracers.size())
  {
    debug("no free smoke tracers");
    return -1;
  }
  if (head_color.a < 0)
    logerr("negative tracer head color alpha, head burnTime = %f", head_color.a);
  if (time_to_live < 0)
    logerr("negative tracer smoke ttl, smoke vaporTime = %f", time_to_live);
  int id = freeTracers.back();
  G_FAST_ASSERT(id >= 0);
  freeTracers.pop_back();
  tracers[id].startPos = tracers[id].lastPos = start_pos;
  tracers[id].vaporTime = time_to_live;
#if DAGOR_DBGLEVEL > 0
  tracers[id].framesWithoutUpdate = 0;
#endif
  aliveTracers.set(id);
  createCommands.push_back(
    TracerCreateCommand(id, start_pos, ndir * radius, time_to_live, reinterpret_cast<const float4 &>(smoke_color),
      reinterpret_cast<const float4 &>(head_color), reinterpret_cast<const float3 &>(start_head_color), safeinv(start_time)));

  usedTracers[currentUsed].push_back(id);
  return id;
}

void SmokeTracerManager::performGPUCommands()
{
#if DAGOR_DBGLEVEL > 0
  lastGpuUpdateFrame = ::dagor_frame_no();
#endif
  if (!createCommands_cs)
  {
    createCommands.clear();
    updateCommands.clear();
    return;
  }
  if (!createCommands.size() && !updateCommands.size())
    return;
  // ShaderGlobal::set_real(smoke_trace_current_timeVarId, cTime);
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
    G_STATIC_ASSERT(sizeof(createCommands[0]) == TRACER_SMOKE_CREATE_COMMAND_SIZE * sizeof(float4));
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
    G_STATIC_ASSERT(sizeof(updateCommands[0]) == TRACER_SMOKE_UPDATE_COMMAND_SIZE * sizeof(float4));
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

void SmokeTracerManager::beforeRender(const Frustum &frustum, float exposure_time, float pixel_scale)
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

void SmokeTracerManager::renderTrans()
{
  if (!usedTracers[currentUsed].size())
    return;
  d3d::resource_barrier({culledTracerHeads.get(), RB_RO_SRV | RB_STAGE_VERTEX});
  d3d::resource_barrier({culledTracerTails.get(), RB_RO_SRV | RB_STAGE_VERTEX});
  d3d::resource_barrier({drawIndirectBuffer.get(), RB_RO_INDIRECT_BUFFER});
  ShaderGlobal::set_real(smoke_trace_current_timeVarId, cTime);
  headRenderer.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);
  static int smoke_tracers_head_culledTracers_const_no = ShaderGlobal::get_slot_by_name("smoke_tracers_head_culledTracers_const_no");
  d3d::set_buffer(STAGE_VS, smoke_tracers_head_culledTracers_const_no, culledTracerHeads.get());
  d3d::draw_indirect(PRIM_TRISTRIP, drawIndirectBuffer.get(), 0);
  d3d::set_buffer(STAGE_VS, smoke_tracers_head_culledTracers_const_no, 0);

  tailRenderer.shader->setStates(0, true);
  static int smoke_tracers_tail_tracers_const_no = ShaderGlobal::get_slot_by_name("smoke_tracers_tail_tracers_const_no");
  static int smoke_tracers_tail_tracerVerts_const_no = ShaderGlobal::get_slot_by_name("smoke_tracers_tail_tracerVerts_const_no");
  d3d::set_buffer(STAGE_VS, smoke_tracers_tail_tracers_const_no, culledTracerTails.get());
  d3d::set_buffer(STAGE_VS, smoke_tracers_tail_tracerVerts_const_no, tracerVertsBuffer.get());
  d3d::draw_indirect(PRIM_TRISTRIP, drawIndirectBuffer.get(), sizeof(uint32_t) * DRAW_INDIRECT_NUM_ARGS);
  // d3d::draw(PRIM_TRISTRIP, 0, 32);
  d3d::set_buffer(STAGE_VS, smoke_tracers_tail_tracers_const_no, 0);
  d3d::set_buffer(STAGE_VS, smoke_tracers_tail_tracerVerts_const_no, 0);
  // d3d::set_buffer(STAGE_VS, 1, 0);
  // d3d::set_buffer(STAGE_VS, 2, 0);
}

SmokeTracerManager::SmokeTracerManager() : currentUsed(0), cTime(0), lastDt(0)
{
  freeTracers.resize(MAX_TRACERS);
  for (int i = 0; i < MAX_TRACERS; ++i)
    freeTracers[i] = MAX_TRACERS - 1 - i;
}

SmokeTracerManager::~SmokeTracerManager() { closeGPU(); }
