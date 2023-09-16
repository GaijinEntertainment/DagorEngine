#include <daFx/dafx.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix4.h>
#include <daFx/dafx_def.hlsli>
#include <daFx/dafx_hlsl_funcs.hlsli>
#include <daFx/dafx_random.hlsli>
#include <daFx/dafx_loaders.hlsli>
#include <daFx/dafx_packers.hlsli>
#include <dafx_globals.hlsli>

float3 (*modfx_get_additional_wind_at_pos)(const float3 &p) = nullptr;

namespace dafx_modfx
{
extern void *g_cluster_wind_ctx;

#include "modfx_decl.hlsl"
#include "modfx.hlsl"

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
