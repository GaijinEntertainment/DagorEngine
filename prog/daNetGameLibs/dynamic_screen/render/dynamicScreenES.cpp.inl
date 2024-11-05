// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <shaders/dag_dynSceneRes.h>
#include <daECS/core/entityId.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/resPtr.h>
#include <ecs/anim/anim.h>
#include <ecs/render/animCharUtils.h>

ShaderVariableInfo dynamic_screen_texVarId("dynamic_screen_tex");

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void dynamic_screen_on_appear_es(const ecs::Event &,
  SharedTex &dynamic_screen__texture,
  IPoint2 dynamic_screen__texture_size,
  int dynamic_screen__texture_mips,
  AnimV20::AnimcharRendComponent &animchar_render)
{
  static int counter = 0;
  int texcf = TEXFMT_R11G11B10F | TEXCF_RTARGET;
  if (dynamic_screen__texture_mips != 1)
    texcf |= TEXCF_GENERATEMIPS;
  dynamic_screen__texture = SharedTex(dag::create_tex(nullptr, dynamic_screen__texture_size.x, dynamic_screen__texture_size.y, texcf,
    dynamic_screen__texture_mips, String(32, "dynamic_screen_tex_%d", counter++)));
  recreate_material_with_new_params(animchar_render, "dynamic_screen", [&dynamic_screen__texture](ShaderMaterial *mat) {
    mat->set_texture_param(dynamic_screen_texVarId.get_var_id(), dynamic_screen__texture.getTexId());
  });
}
