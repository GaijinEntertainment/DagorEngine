// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/ssao_common.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_tex3d.h>

namespace ssao_detail
{
static const char *ssao_use_wsao_cstr = "ssao_use_wsao";
static const char *use_contact_shadows_cstr = "use_contact_shadows";

static uint32_t get_flags_from_shader_assumes(uint32_t &flags_assumed)
{
  int ssao_use_wsaoVarId = get_shader_variable_id(ssao_use_wsao_cstr, true);
  int use_contact_shadowsVarId = get_shader_variable_id(use_contact_shadows_cstr, true);

  flags_assumed = 0;
  uint32_t flags_values = 0;
  for (auto it : {eastl::pair{ssao_use_wsaoVarId, SSAO_USE_WSAO}, eastl::pair{use_contact_shadowsVarId, SSAO_USE_CONTACT_SHADOWS}})
  {
    if (ShaderGlobal::is_var_assumed(it.first))
    {
      flags_assumed |= it.second;
      if (ShaderGlobal::get_interval_assumed_value(it.first))
        flags_values |= it.second;
    }
  }
  return flags_values;
}

static uint32_t combine_flags(uint32_t init_flags, uint32_t shader_flags, uint32_t shader_flags_assumed_bits)
{
  uint32_t commonFlagsDeclared = shader_flags_assumed_bits & init_flags;
  if ((commonFlagsDeclared & shader_flags) != (commonFlagsDeclared & init_flags))
  {
    logerr("SSAO: Bad shader assumes! They cannot support features requested. Fallback to what shader assumes allow.");
    init_flags = shader_flags | (~shader_flags_assumed_bits & init_flags);
  }
  else if ((commonFlagsDeclared & shader_flags) != shader_flags)
  {
    bool allowImplicitFlags = (init_flags & SSAO_ALLOW_IMPLICIT_FLAGS_FROM_SHADER_ASSUMES);
    if (!allowImplicitFlags)
      logerr("SSAO: Shader assumes introduce more features than requested. Adding them...");
    init_flags = shader_flags | init_flags;
  }
  return init_flags;
}

uint32_t consider_shader_assumes(uint32_t flags)
{
  uint32_t shader_flags_assumed = 0;
  uint32_t shader_flags = get_flags_from_shader_assumes(shader_flags_assumed);
  uint32_t combined_flags = combine_flags(flags, shader_flags, shader_flags_assumed);
  return combined_flags;
}

uint32_t creation_flags_to_format(uint32_t flags)
{
  constexpr int CONTACT_SHADOWS_STEPS = 20;
  constexpr int CONTACT_SHADOWS_SIMPLIFIED_STEPS = 8;

  bool use_wsao = flags & SSAO_USE_WSAO;
  bool use_contact_shadows = flags & SSAO_USE_CONTACT_SHADOWS;
  bool contact_shadows_simplified = flags & SSAO_USE_CONTACT_SHADOWS_SIMPLIFIED;
  ShaderGlobal::set_int(get_shader_variable_id(ssao_use_wsao_cstr, true), use_wsao);
  ShaderGlobal::set_int(get_shader_variable_id(use_contact_shadows_cstr, true), use_contact_shadows);
  ShaderGlobal::set_int(get_shader_variable_id("contact_shadows_num_steps", true),
    contact_shadows_simplified ? CONTACT_SHADOWS_SIMPLIFIED_STEPS : CONTACT_SHADOWS_STEPS);

  if (use_wsao && use_contact_shadows)
    return TEXFMT_R8G8B8A8;
  else if (use_wsao || use_contact_shadows)
    return TEXFMT_R8G8;
  else
    return TEXFMT_R8;
}

} // namespace ssao_detail