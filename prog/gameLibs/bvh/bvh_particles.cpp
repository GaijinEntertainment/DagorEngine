// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <debug/dag_log.h>
#include <generic/dag_align.h>
#include <shaders/dag_shaderVar.h>

#include "bvh_context.h"
#include "bvh_particles.h"

#include "../commonFx/commonFxGame/modfx/modfx_bvh.hlsli"

namespace bvh::particles
{

namespace
{
struct BvhParticleSharedBuffer
{
  UniqueBuf data;
  int fxCapacity = 0;
  int tracerCapacity = 0;
  bool ready = false;
  bool overflowWarned = false;

  void ensureCapacity(int fx_max, int tracer_max);
  void teardown();

private:
  void grow(int new_fx_cap, int new_tracer_cap);
};

void BvhParticleSharedBuffer::grow(int new_fx_cap, int new_tracer_cap)
{
  static int nameCounter = 0;
  UniqueBuf newBuf = dag::buffers::create_ua_sr_structured(sizeof(ModfxBVHParticleData), new_fx_cap + new_tracer_cap,
    String(0, "bvh_particle_data_%d", nameCounter++), d3d::buffers::Init::No, RESTAG_BVH);
  HANDLE_LOST_DEVICE_STATE(newBuf, );

  data.close();
  data = eastl::move(newBuf);
  fxCapacity = new_fx_cap;
  tracerCapacity = new_tracer_cap;

  static int bvh_particle_dataVarId = get_shader_variable_id("bvh_particle_data");
  static int smoke_tracer_instance_offsetVarId = get_shader_variable_id("smoke_tracer_instance_offset", true);
  ShaderGlobal::set_buffer(bvh_particle_dataVarId, data);
  ShaderGlobal::set_int(smoke_tracer_instance_offsetVarId, fxCapacity);
}

void BvhParticleSharedBuffer::ensureCapacity(int fx_max, int tracer_max)
{
  fx_max = dag::align_up(fx_max, FX_CAPACITY_ALIGN);
  tracer_max = dag::align_up(tracer_max, TRACER_CAPACITY_ALIGN);
  if (fxCapacity < fx_max || tracerCapacity < tracer_max)
    grow(max(fxCapacity, fx_max), max(tracerCapacity, tracer_max));
  const bool isCapacityGood = fxCapacity >= fx_max && tracerCapacity >= tracer_max;
  if (!isCapacityGood && !overflowWarned)
  {
    logerr("bvh::particles: failed to grow shared buffer (fx %d/%d, tracers %d/%d) -- writers will skip this frame", fxCapacity,
      fx_max, tracerCapacity, tracer_max);
    overflowWarned = true;
  }
  if (isCapacityGood)
    overflowWarned = false;
  ready = isCapacityGood;
}

void BvhParticleSharedBuffer::teardown()
{
  data.close();
  fxCapacity = 0;
  tracerCapacity = 0;
  ready = false;
  overflowWarned = false;
  ShaderGlobal::set_buffer(get_shader_variable_id("bvh_particle_data", true), BAD_D3DRESID);
  ShaderGlobal::set_int(get_shader_variable_id("smoke_tracer_instance_offset", true), 0);
}

static BvhParticleSharedBuffer shared;
} // namespace

void init() {}

void ensure_capacity(int fx_max, int smoke_tracer_max) { shared.ensureCapacity(fx_max, smoke_tracer_max); }

bool is_ready() { return shared.ready; }
Sbuffer *get_data_buf() { return shared.data.getBuf(); }
int get_fx_capacity() { return shared.fxCapacity; }
int get_smoke_tracer_capacity() { return shared.tracerCapacity; }
int64_t get_size_bytes() { return shared.data ? shared.data->getSize() : 0; }

void teardown() { shared.teardown(); }

} // namespace bvh::particles
