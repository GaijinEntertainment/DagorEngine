// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "semiTransRenderES.cpp.inl"
ECS_DEF_PULL_VAR(semiTransRender);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc animchar_render_semi_trans_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
//start of 5 ro components at [2]
  {ECS_HASH("semi_transparent__placingColor"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("semi_transparent__lod"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("semi_transparent__placingColorAlpha"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("semi_transparent__endStage"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("semi_transparent__visible"), ecs::ComponentTypeInfo<bool>()}
};
static void animchar_render_semi_trans_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<RenderLateTransEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(animchar_render_semi_trans_es_event_handler_comps, "semi_transparent__visible", bool)) )
      continue;
    animchar_render_semi_trans_es_event_handler(static_cast<const RenderLateTransEvent&>(evt)
          , components.manager()
      , ECS_RW_COMP(animchar_render_semi_trans_es_event_handler_comps, "animchar_render", AnimV20::AnimcharRendComponent)
      , ECS_RO_COMP(animchar_render_semi_trans_es_event_handler_comps, "semi_transparent__placingColor", Point3)
      , ECS_RW_COMP(animchar_render_semi_trans_es_event_handler_comps, "animchar_visbits", animchar_visbits_t)
      , ECS_RO_COMP(animchar_render_semi_trans_es_event_handler_comps, "semi_transparent__lod", int)
      , ECS_RO_COMP_OR(animchar_render_semi_trans_es_event_handler_comps, "semi_transparent__placingColorAlpha", float(1.0f))
      , ECS_RO_COMP_OR(animchar_render_semi_trans_es_event_handler_comps, "semi_transparent__endStage", int(ShaderMesh::STG_imm_decal))
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_render_semi_trans_es_event_handler_es_desc
(
  "animchar_render_semi_trans_es",
  "prog/daNetGameLibs/semi_trans_render/render/semiTransRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_render_semi_trans_es_event_handler_all_events),
  make_span(animchar_render_semi_trans_es_event_handler_comps+0, 2)/*rw*/,
  make_span(animchar_render_semi_trans_es_event_handler_comps+2, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderLateTransEvent>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc set_shader_semi_trans_rendinst_es_event_handler_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("semi_transparent__resIdx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("semi_transparent__placingColor"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("semi_transparent__placingColorAlpha"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("semi_transparent__visible"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [6]
  {ECS_HASH("ri_preview__name"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 1 no components at [7]
  {ECS_HASH("use_texture"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void set_shader_semi_trans_rendinst_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<RenderLateTransEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(set_shader_semi_trans_rendinst_es_event_handler_comps, "semi_transparent__visible", bool)) )
      continue;
    set_shader_semi_trans_rendinst_es_event_handler(static_cast<const RenderLateTransEvent&>(evt)
          , components.manager()
      , ECS_RO_COMP(set_shader_semi_trans_rendinst_es_event_handler_comps, "semi_transparent__resIdx", int)
      , ECS_RO_COMP(set_shader_semi_trans_rendinst_es_event_handler_comps, "eid", ecs::EntityId)
      , ECS_RO_COMP(set_shader_semi_trans_rendinst_es_event_handler_comps, "semi_transparent__placingColor", Point3)
      , ECS_RO_COMP(set_shader_semi_trans_rendinst_es_event_handler_comps, "transform", TMatrix)
      , ECS_RO_COMP_OR(set_shader_semi_trans_rendinst_es_event_handler_comps, "semi_transparent__placingColorAlpha", float(1.0f))
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc set_shader_semi_trans_rendinst_es_event_handler_es_desc
(
  "set_shader_semi_trans_rendinst_es",
  "prog/daNetGameLibs/semi_trans_render/render/semiTransRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_shader_semi_trans_rendinst_es_event_handler_all_events),
  empty_span(),
  make_span(set_shader_semi_trans_rendinst_es_event_handler_comps+0, 6)/*ro*/,
  make_span(set_shader_semi_trans_rendinst_es_event_handler_comps+6, 1)/*rq*/,
  make_span(set_shader_semi_trans_rendinst_es_event_handler_comps+7, 1)/*no*/,
  ecs::EventSetBuilder<RenderLateTransEvent>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc set_shader_semi_trans_rendinst_with_tex_es_event_handler_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("semi_transparent__resIdx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("semi_transparent__placingColor"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("semi_transparent__placingColorAlpha"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("semi_transparent__visible"), ecs::ComponentTypeInfo<bool>()},
//start of 2 rq components at [6]
  {ECS_HASH("use_texture"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("ri_preview__name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void set_shader_semi_trans_rendinst_with_tex_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<RenderLateTransEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(set_shader_semi_trans_rendinst_with_tex_es_event_handler_comps, "semi_transparent__visible", bool)) )
      continue;
    set_shader_semi_trans_rendinst_with_tex_es_event_handler(static_cast<const RenderLateTransEvent&>(evt)
          , components.manager()
      , ECS_RO_COMP(set_shader_semi_trans_rendinst_with_tex_es_event_handler_comps, "semi_transparent__resIdx", int)
      , ECS_RO_COMP(set_shader_semi_trans_rendinst_with_tex_es_event_handler_comps, "eid", ecs::EntityId)
      , ECS_RO_COMP(set_shader_semi_trans_rendinst_with_tex_es_event_handler_comps, "semi_transparent__placingColor", Point3)
      , ECS_RO_COMP(set_shader_semi_trans_rendinst_with_tex_es_event_handler_comps, "transform", TMatrix)
      , ECS_RO_COMP_OR(set_shader_semi_trans_rendinst_with_tex_es_event_handler_comps, "semi_transparent__placingColorAlpha", float(1.0f))
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc set_shader_semi_trans_rendinst_with_tex_es_event_handler_es_desc
(
  "set_shader_semi_trans_rendinst_with_tex_es",
  "prog/daNetGameLibs/semi_trans_render/render/semiTransRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_shader_semi_trans_rendinst_with_tex_es_event_handler_all_events),
  empty_span(),
  make_span(set_shader_semi_trans_rendinst_with_tex_es_event_handler_comps+0, 6)/*ro*/,
  make_span(set_shader_semi_trans_rendinst_with_tex_es_event_handler_comps+6, 2)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<RenderLateTransEvent>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc semi_trans_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("semi_trans_render__manager"), ecs::ComponentTypeInfo<SemiTransRenderManager>()}
};
static ecs::CompileTimeQueryDesc semi_trans_manager_ecs_query_desc
(
  "semi_trans_manager_ecs_query",
  make_span(semi_trans_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void semi_trans_manager_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, semi_trans_manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(semi_trans_manager_ecs_query_comps, "semi_trans_render__manager", SemiTransRenderManager)
            );

        }while (++comp != compE);
    }
  );
}
