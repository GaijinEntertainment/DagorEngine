// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFx/dafx.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_mathUtils.h>
#include <daFx/dafx_def.hlsli>
#include <daFx/dafx_hlsl_funcs.hlsli>
#include <daFx/dafx_random.hlsli>
#include <daFx/dafx_loaders.hlsli>
#include <daFx/dafx_packers.hlsli>
#include <daFx/dafx_gravity_zone.hlsli>
#include <dafx_globals.hlsli>

float3 (*modfx_get_additional_wind_at_pos)(const float3 &p) = nullptr;
float3 (*modfx_get_cfd_wind_cpu)(const float3 &p, const float2 &wind_dir, float wind_speed, float directional_force,
  float impulse_force) = nullptr;
float modfx_cfd_wind_infl_threshold = 0.0f; // CFD wind is disabled if the directional_force * life_limit for the particle is below
                                            // this threshold
extern int dafx_gravity_zone_count;
extern GravityZoneDescriptor_buffer dafx_gravity_zone_buffer;

namespace dafx_modfx
{
extern void *g_cluster_wind_ctx;

#include "modfx_decl.hlsli"
#include "modfx.hlsli"

#include <daFx/dafx_shaders.hlsli>
} // namespace dafx_modfx
using namespace dafx_modfx;


void dafx_modfx_emission_cpu(dafx::CpuExecData &data) { dafx_emission_shader(data); }

void dafx_modfx_simulation_cpu(dafx::CpuExecData &data) { dafx_simulation_shader(data); }

void dafx_modfx_register_cpu_overrides(dafx::ContextId ctx)
{
  dafx::register_cpu_override_shader(ctx, "dafx_modfx_emission", &dafx_modfx_emission_cpu);
  dafx::register_cpu_override_shader(ctx, "dafx_modfx_simulation", &dafx_modfx_simulation_cpu);
}

void dafx_modfx_register_cpu_overrides_adv(dafx::ContextId ctx)
{
  dafx::register_cpu_override_shader(ctx, "dafx_modfx_emission_adv", &dafx_modfx_emission_cpu);
  dafx::register_cpu_override_shader(ctx, "dafx_modfx_simulation_adv", &dafx_modfx_simulation_cpu);
}
