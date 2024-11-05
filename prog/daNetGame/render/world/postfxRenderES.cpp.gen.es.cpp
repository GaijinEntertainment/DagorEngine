#include "postfxRenderES.cpp.inl"
ECS_DEF_PULL_VAR(postfxRender);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc init_dof_es_comps[] ={};
static void init_dof_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<BeforeLoadLevel>());
  init_dof_es(static_cast<const BeforeLoadLevel&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc init_dof_es_es_desc
(
  "init_dof_es",
  "prog/daNetGame/render/world/postfxRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_dof_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc set_dof_resolution_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dof"), ecs::ComponentTypeInfo<DepthOfFieldPS>()}
};
static void set_dof_resolution_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<SetResolutionEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    set_dof_resolution_es(static_cast<const SetResolutionEvent&>(evt)
        , ECS_RW_COMP(set_dof_resolution_es_comps, "dof", DepthOfFieldPS)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc set_dof_resolution_es_es_desc
(
  "set_dof_resolution_es",
  "prog/daNetGame/render/world/postfxRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_dof_resolution_es_all_events),
  make_span(set_dof_resolution_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SetResolutionEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc init_dof_params_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("dof"), ecs::ComponentTypeInfo<DepthOfFieldPS>()},
  {ECS_HASH("dof__depth_for_transparency_node_handle"), ecs::ComponentTypeInfo<dabfg::NodeHandle>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("dof__downsample_depth_for_transparency_node_handle"), ecs::ComponentTypeInfo<dabfg::NodeHandle>(), ecs::CDF_OPTIONAL},
//start of 15 ro components at [3]
  {ECS_HASH("dof__on"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dof__is_filmic"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dof__focusDistance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__fStop"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__sensorHeight_mm"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__focalLength_mm"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__nearDofStart"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__nearDofEnd"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__nearDofAmountPercent"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__farDofStart"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__farDofEnd"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__farDofAmountPercent"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__bokehShape_kernelSize"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__bokehShape_bladesCount"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__minCheckDistance"), ecs::ComponentTypeInfo<float>()}
};
static void init_dof_params_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_dof_params_es(evt
        , ECS_RW_COMP(init_dof_params_es_comps, "dof", DepthOfFieldPS)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__on", bool)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__is_filmic", bool)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__focusDistance", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__fStop", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__sensorHeight_mm", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__focalLength_mm", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__nearDofStart", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__nearDofEnd", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__nearDofAmountPercent", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__farDofStart", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__farDofEnd", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__farDofAmountPercent", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__bokehShape_kernelSize", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__bokehShape_bladesCount", float)
    , ECS_RO_COMP(init_dof_params_es_comps, "dof__minCheckDistance", float)
    , ECS_RW_COMP_PTR(init_dof_params_es_comps, "dof__depth_for_transparency_node_handle", dabfg::NodeHandle)
    , ECS_RW_COMP_PTR(init_dof_params_es_comps, "dof__downsample_depth_for_transparency_node_handle", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_dof_params_es_es_desc
(
  "init_dof_params_es",
  "prog/daNetGame/render/world/postfxRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_dof_params_es_all_events),
  make_span(init_dof_params_es_comps+0, 3)/*rw*/,
  make_span(init_dof_params_es_comps+3, 15)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc render_dof_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dof"), ecs::ComponentTypeInfo<DepthOfFieldPS>()},
//start of 1 ro components at [1]
  {ECS_HASH("downsampled_dof_frame_fallback"), ecs::ComponentTypeInfo<UniqueTex>()}
};
static void render_dof_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<RenderPostFx>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    render_dof_es(static_cast<const RenderPostFx&>(evt)
        , ECS_RW_COMP(render_dof_es_comps, "dof", DepthOfFieldPS)
    , ECS_RO_COMP(render_dof_es_comps, "downsampled_dof_frame_fallback", UniqueTex)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc render_dof_es_es_desc
(
  "render_dof_es",
  "prog/daNetGame/render/world/postfxRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, render_dof_es_all_events),
  make_span(render_dof_es_comps+0, 1)/*rw*/,
  make_span(render_dof_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderPostFx>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc set_fade_mul_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation_override_settings"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static ecs::CompileTimeQueryDesc set_fade_mul_ecs_query_desc
(
  "set_fade_mul_ecs_query",
  make_span(set_fade_mul_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_fade_mul_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, set_fade_mul_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(set_fade_mul_ecs_query_comps, "adaptation_override_settings", ecs::Object)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc init_dof_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("dof"), ecs::ComponentTypeInfo<DepthOfFieldPS>()},
  {ECS_HASH("downsampled_dof_frame_fallback"), ecs::ComponentTypeInfo<UniqueTex>()}
};
static ecs::CompileTimeQueryDesc init_dof_ecs_query_desc
(
  "init_dof_ecs_query",
  make_span(init_dof_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void init_dof_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, init_dof_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(init_dof_ecs_query_comps, "dof", DepthOfFieldPS)
            , ECS_RW_COMP(init_dof_ecs_query_comps, "downsampled_dof_frame_fallback", UniqueTex)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_dof_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dof"), ecs::ComponentTypeInfo<DepthOfFieldPS>()}
};
static ecs::CompileTimeQueryDesc get_dof_ecs_query_desc
(
  "get_dof_ecs_query",
  make_span(get_dof_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_dof_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_dof_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_dof_ecs_query_comps, "dof", DepthOfFieldPS)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc set_far_dof_ecs_query_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("dof__on"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dof__farDofStart"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__farDofEnd"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("dof__farDofAmountPercent"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc set_far_dof_ecs_query_desc
(
  "set_far_dof_ecs_query",
  make_span(set_far_dof_ecs_query_comps+0, 4)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_far_dof_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, set_far_dof_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(set_far_dof_ecs_query_comps, "dof__on", bool)
            , ECS_RW_COMP(set_far_dof_ecs_query_comps, "dof__farDofStart", float)
            , ECS_RW_COMP(set_far_dof_ecs_query_comps, "dof__farDofEnd", float)
            , ECS_RW_COMP(set_far_dof_ecs_query_comps, "dof__farDofAmountPercent", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc postfx_bind_additional_textures_from_registry_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("postfx__additional_bind_textures"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static ecs::CompileTimeQueryDesc postfx_bind_additional_textures_from_registry_ecs_query_desc
(
  "postfx_bind_additional_textures_from_registry_ecs_query",
  empty_span(),
  make_span(postfx_bind_additional_textures_from_registry_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void postfx_bind_additional_textures_from_registry_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, postfx_bind_additional_textures_from_registry_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(postfx_bind_additional_textures_from_registry_ecs_query_comps, "postfx__additional_bind_textures", ecs::StringList)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc postfx_bind_additional_textures_from_namespace_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("postfx__additional_bind_textures"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static ecs::CompileTimeQueryDesc postfx_bind_additional_textures_from_namespace_ecs_query_desc
(
  "postfx_bind_additional_textures_from_namespace_ecs_query",
  empty_span(),
  make_span(postfx_bind_additional_textures_from_namespace_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void postfx_bind_additional_textures_from_namespace_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, postfx_bind_additional_textures_from_namespace_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(postfx_bind_additional_textures_from_namespace_ecs_query_comps, "postfx__additional_bind_textures", ecs::StringList)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc postfx_read_additional_textures_from_registry_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("postfx__additional_read_textures"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static ecs::CompileTimeQueryDesc postfx_read_additional_textures_from_registry_ecs_query_desc
(
  "postfx_read_additional_textures_from_registry_ecs_query",
  empty_span(),
  make_span(postfx_read_additional_textures_from_registry_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void postfx_read_additional_textures_from_registry_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, postfx_read_additional_textures_from_registry_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(postfx_read_additional_textures_from_registry_ecs_query_comps, "postfx__additional_read_textures", ecs::StringList)
            );

        }while (++comp != compE);
    }
  );
}
