// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <render/renderEvent.h>
#include <render/resourceSlot/registerAccess.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <ecs/render/resPtr.h>
#include <shaders/dag_postFxRenderer.h>


template <typename Callable>
static void screen_frost_renderer_ecs_query(ecs::EntityId, Callable);

static void set_texture_resolution_var(const SharedTexHolder &texture, const char *texture_resolution_var)
{
  if (texture)
  {
    prefetch_and_check_managed_texture_loaded(texture.getTexId());
    TextureInfo info;
    texture.getTex2D()->getinfo(info);
    ShaderGlobal::set_color4(get_shader_variable_id(texture_resolution_var, true), info.w, info.h, 1.f / info.w, 1.f / info.h);
  }
}

ECS_TAG(render)
static void screen_frost_renderer_init_es_event_handler(const BeforeLoadLevel &, ecs::EntityManager &manager)
{
  screen_frost_renderer_ecs_query(manager.getOrCreateSingletonEntity(ECS_HASH("screen_frost_renderer")),
    [](const SharedTexHolder &frost_tex, const SharedTexHolder &corruption_tex) {
      set_texture_resolution_var(frost_tex, "screen_frost_tile_resolution");
      set_texture_resolution_var(corruption_tex, "screen_corruption_tile_resolution");
    });
}
