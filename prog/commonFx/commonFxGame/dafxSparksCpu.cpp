// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFx/dafx.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix4.h>

#include <daFx/dafx_def.hlsli>
#include <daFx/dafx_hlsl_funcs.hlsli>
#include <daFx/dafx_loaders.hlsli>
#include <daFx/dafx_packers.hlsli>
#include <daFx/dafx_random.hlsli>
#include <daFx/dafx_gravity_zone.hlsli>

#include <dafx_globals.hlsli>

extern int dafx_gravity_zone_count;
extern GravityZoneDescriptor_buffer dafx_gravity_zone_buffer;

namespace dafx_sparks
{
#include <dafxSparkModules/dafx_code_common_decl.hlsl>
#include <dafxSparks_decl.hlsl>
#include <dafxSparkModules/dafx_code_common.hlsl>
#include <dafx_sparks.hlsl>

#define DAFX_EMISSION_SHADER_ENABLED   1
#define DAFX_SIMULATION_SHADER_ENABLED 1

#include <daFx/dafx_shaders.hlsli>
} // namespace dafx_sparks
using namespace dafx_sparks;


void dafx_sparks_emission_cpu(dafx::CpuExecData &data) { dafx_emission_shader(data); }

void dafx_sparks_simulation_cpu(dafx::CpuExecData &data) { dafx_simulation_shader(data); }

void dafx_sparks_register_cpu_overrides(dafx::ContextId ctx)
{
  dafx::register_cpu_override_shader(ctx, "dafx_sparks_ps_emission", &dafx_sparks_emission_cpu);
  dafx::register_cpu_override_shader(ctx, "dafx_sparks_ps_simulation", &dafx_sparks_simulation_cpu);
}
