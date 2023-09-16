#include <render/ssao_common.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_tex3d.h>

namespace ssao_detail
{

uint32_t creation_flags_to_format(uint32_t flags)
{
  constexpr int CONTACT_SHADOWS_STEPS = 20;
  constexpr int CONTACT_SHADOWS_SIMPLIFIED_STEPS = 8;

  bool use_wsao = flags & SSAO_USE_WSAO;
  bool use_contact_shadows = flags & SSAO_USE_CONTACT_SHADOWS;
  bool contact_shadows_simplified = flags & SSAO_USE_CONTACT_SHADOWS_SIMPLIFIED;
  ShaderGlobal::set_int(get_shader_variable_id("use_wsao", true), use_wsao);
  ShaderGlobal::set_int(get_shader_variable_id("use_contact_shadows", true), use_contact_shadows);
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