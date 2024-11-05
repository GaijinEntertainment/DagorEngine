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
#include <dafx_globals.hlsli>
#include "shaders/dafx_flowps2_decl.hlsli"
#include "shaders/dafx_flowps2.hlsli"

#define DAFX_EMISSION_SHADER_ENABLED   1
#define DAFX_SIMULATION_SHADER_ENABLED 1

#include <daFx/dafx_shaders.hlsli>

void dafx_flowps2_emission_cpu(dafx::CpuExecData &data) { dafx_emission_shader(data); }

void dafx_flowps2_simulation_cpu(dafx::CpuExecData &data) { dafx_simulation_shader(data); }

void dafx_flowps2_register_cpu_overrides(dafx::ContextId ctx)
{
  dafx::register_cpu_override_shader(ctx, "dafx_flowps2_emission", &dafx_flowps2_emission_cpu);
  dafx::register_cpu_override_shader(ctx, "dafx_flowps2_simulation", &dafx_flowps2_simulation_cpu);
}
