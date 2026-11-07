// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/denoiser/denoiser_api.h>

namespace denoiser
{
Config resolution_config;

bool is_available() { return false; }

void initialize(bool, bool) {}
void teardown() {}

bool is_ray_reconstruction_enabled() { return false; }
bool is_ptgi_ray_reconstruction_enabled() { return false; }

int get_frame_number() { return 0; }

ParamsHandle<PrepareParams> create_prepare_params(dafg::Registry registry)
{
  return registry.createBlob<PrepareParams>("denoiser_prepare_params").handle();
}
ParamsHandle<ShadowParams> create_shadow_params(dafg::Registry registry)
{
  return registry.createBlob<ShadowParams>("denoiser_shadow_params").handle();
}
ParamsHandle<AOParams> create_ao_params(dafg::Registry registry)
{
  return registry.createBlob<AOParams>("denoiser_ao_params").handle();
}
ParamsHandle<GIParams> create_gi_params(dafg::Registry registry)
{
  return registry.createBlob<GIParams>("denoiser_gi_params").handle();
}
ParamsHandle<ReflectionParams> create_reflection_params(dafg::Registry registry)
{
  return registry.createBlob<ReflectionParams>("denoiser_reflection_params").handle();
}

OutputRwHandle create_shadow_trace_output(dafg::Registry, const char *) { return eastl::nullopt; }
OutputRwHandle create_ao_trace_output(dafg::Registry, const char *) { return eastl::nullopt; }
OutputRwHandle create_gi_trace_output(dafg::Registry, const char *) { return eastl::nullopt; }
OutputRwHandle create_reflection_trace_output(dafg::Registry, const char *) { return eastl::nullopt; }
OutputRwHandle create_reflection_output(dafg::Registry, const char *, ReflectionMethod) { return eastl::nullopt; }
dafg::AutoResolutionRequest<2> reflection_resolution(dafg::Registry registry) { return registry.getResolution<2>("main_view"); }

void read_normal_roughness(dafg::Registry, const char *, Inputs) {}
void read_selected_depth(dafg::Registry, const char *) {}
Inputs reflection_inputs(bool, bool) { return Inputs::Full; }
void order_after_prepare(dafg::Registry) {}

void modify_view_z_for_shadow(dafg::Registry, const char *) {}
void modify_view_z_for_reflection(dafg::Registry, const char *, Inputs) {}

OutputHandle read_shadow_output(dafg::Registry) { return eastl::nullopt; }
OutputHandle read_ao_output(dafg::Registry) { return eastl::nullopt; }
OutputHandle read_gi_output(dafg::Registry) { return eastl::nullopt; }
OutputHandle read_reflection_output(dafg::Registry) { return eastl::nullopt; }
OutputHandle read_reflection_trace_output(dafg::Registry) { return eastl::nullopt; }
OutputHandle read_gi_validation(dafg::Registry) { return eastl::nullopt; }
OutputHandle read_reflection_validation(dafg::Registry) { return eastl::nullopt; }

#if DAGOR_DBGLEVEL > 0
Texture *fit_debug_image(DebugImage &, const IPoint2 &, uint32_t, const char *) { return nullptr; }
void copy_for_debug(BaseTexture *, DebugImage &, const char *) {}
#endif

dafg::NodeHandle make_prepare_node(const PrepareNodeParams &) { return {}; }
dafg::NodeHandle make_shadow_node(const ShadowNodeParams &) { return {}; }
dafg::NodeHandle make_ao_node(const AONodeParams &) { return {}; }
dafg::NodeHandle make_gi_node(const GINodeParams &) { return {}; }
dafg::NodeHandle make_reflection_node(const ReflectionNodeParams &) { return {}; }
dafg::NodeHandle make_gi_view_z_node() { return {}; }
dafg::NodeHandle make_reflection_normal_roughness_node() { return {}; }

} // namespace denoiser
