#include "outlineRendererES.cpp.inl"
ECS_DEF_PULL_VAR(outlineRenderer);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_outline_rendrerer_locally_es_comps[] ={};
static void create_outline_rendrerer_locally_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<BeforeLoadLevel>());
  create_outline_rendrerer_locally_es(static_cast<const BeforeLoadLevel&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc create_outline_rendrerer_locally_es_es_desc
(
  "create_outline_rendrerer_locally_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_outline_rendrerer_locally_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel>::build(),
  0
);
static constexpr ecs::ComponentDesc outline_renderer_create_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("outline_renderer"), ecs::ComponentTypeInfo<OutlineRenderer>()},
//start of 2 ro components at [1]
  {ECS_HASH("outline_blur_width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("outline_brightness"), ecs::ComponentTypeInfo<float>()}
};
static void outline_renderer_create_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    outline_renderer_create_es_event_handler(evt
        , ECS_RW_COMP(outline_renderer_create_es_event_handler_comps, "outline_renderer", OutlineRenderer)
    , ECS_RO_COMP(outline_renderer_create_es_event_handler_comps, "outline_blur_width", float)
    , ECS_RO_COMP(outline_renderer_create_es_event_handler_comps, "outline_brightness", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc outline_renderer_create_es_event_handler_es_desc
(
  "outline_renderer_create_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, outline_renderer_create_es_event_handler_all_events),
  make_span(outline_renderer_create_es_event_handler_comps+0, 1)/*rw*/,
  make_span(outline_renderer_create_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc outline_render_resolution_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("outline_renderer"), ecs::ComponentTypeInfo<OutlineRenderer>()}
};
static void outline_render_resolution_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<SetResolutionEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    outline_render_resolution_es_event_handler(static_cast<const SetResolutionEvent&>(evt)
        , ECS_RW_COMP(outline_render_resolution_es_event_handler_comps, "outline_renderer", OutlineRenderer)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc outline_render_resolution_es_event_handler_es_desc
(
  "outline_render_resolution_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, outline_render_resolution_es_event_handler_all_events),
  make_span(outline_render_resolution_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SetResolutionEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc outline_prepare_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("outline_renderer"), ecs::ComponentTypeInfo<OutlineRenderer>()}
};
static void outline_prepare_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    outline_prepare_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(outline_prepare_es_comps, "outline_renderer", OutlineRenderer)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc outline_prepare_es_es_desc
(
  "outline_prepare_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, outline_prepare_es_all_events),
  make_span(outline_prepare_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,nullptr,nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc create_outline_node_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("outline_prepare_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("outline_apply_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void create_outline_node_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_outline_node_es(evt
        , ECS_RW_COMP(create_outline_node_es_comps, "outline_prepare_node", dabfg::NodeHandle)
    , ECS_RW_COMP(create_outline_node_es_comps, "outline_apply_node", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_outline_node_es_es_desc
(
  "create_outline_node_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_outline_node_es_all_events),
  make_span(create_outline_node_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc disable_outline_on_ri_destroyed_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static void disable_outline_on_ri_destroyed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventRiExtraDestroyed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    disable_outline_on_ri_destroyed_es_event_handler(static_cast<const EventRiExtraDestroyed&>(evt)
        , ECS_RW_COMP(disable_outline_on_ri_destroyed_es_event_handler_comps, "outline__enabled", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc disable_outline_on_ri_destroyed_es_event_handler_es_desc
(
  "disable_outline_on_ri_destroyed_es",
  "prog/daNetGameLibs/outline/render/outlineRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, disable_outline_on_ri_destroyed_es_event_handler_all_events),
  make_span(disable_outline_on_ri_destroyed_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventRiExtraDestroyed>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc outline_render_always_visible_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 9 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_always_visible_ecs_query_desc
(
  "outline_render_always_visible_ecs_query",
  make_span(outline_render_always_visible_ecs_query_comps+0, 2)/*rw*/,
  make_span(outline_render_always_visible_ecs_query_comps+2, 9)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_always_visible_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_always_visible_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP_OR(outline_render_always_visible_ecs_query_comps, "animchar_render__enabled", bool( true)) && ECS_RO_COMP(outline_render_always_visible_ecs_query_comps, "outline__always_visible", bool) && ECS_RO_COMP(outline_render_always_visible_ecs_query_comps, "outline__enabled", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_always_visible_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(outline_render_always_visible_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RW_COMP(outline_render_always_visible_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP(outline_render_always_visible_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RO_COMP_PTR(outline_render_always_visible_ecs_query_comps, "attaches_list", ecs::EidList)
            , ECS_RO_COMP_PTR(outline_render_always_visible_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP_OR(outline_render_always_visible_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_always_visible_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_always_visible_ri_ecs_query_comps[] =
{
//start of 8 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_always_visible_ri_ecs_query_desc
(
  "outline_render_always_visible_ri_ecs_query",
  empty_span(),
  make_span(outline_render_always_visible_ri_ecs_query_comps+0, 8)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_always_visible_ri_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_always_visible_ri_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "outline__always_visible", bool) && ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "outline__enabled", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "ri_extra", RiExtraComponent)
            , ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_always_visible_ri_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_always_visible_ri_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_always_visible_ri_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_always_visible_ri_handle_ecs_query_comps[] =
{
//start of 8 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra__handle"), ecs::ComponentTypeInfo<rendinst::riex_handle_t>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
//start of 1 no components at [8]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()}
};
static ecs::CompileTimeQueryDesc outline_render_always_visible_ri_handle_ecs_query_desc
(
  "outline_render_always_visible_ri_handle_ecs_query",
  empty_span(),
  make_span(outline_render_always_visible_ri_handle_ecs_query_comps+0, 8)/*ro*/,
  empty_span(),
  make_span(outline_render_always_visible_ri_handle_ecs_query_comps+8, 1)/*no*/);
template<typename Callable>
inline void outline_render_always_visible_ri_handle_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_always_visible_ri_handle_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "outline__always_visible", bool) && ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "outline__enabled", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "ri_extra__handle", rendinst::riex_handle_t)
            , ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_always_visible_ri_handle_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_always_visible_ri_handle_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_always_visible_ri_handle_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_pass_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 12 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__isOccluded"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_pass_ecs_query_desc
(
  "outline_render_z_pass_ecs_query",
  make_span(outline_render_z_pass_ecs_query_comps+0, 1)/*rw*/,
  make_span(outline_render_z_pass_ecs_query_comps+1, 12)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_z_pass_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_pass_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_pass_ecs_query_comps, "outline__always_visible", bool( false)) && !ECS_RO_COMP_OR(outline_render_z_pass_ecs_query_comps, "outline__z_fail", bool( false)) && ECS_RO_COMP_OR(outline_render_z_pass_ecs_query_comps, "animchar_render__enabled", bool( true)) && ECS_RO_COMP(outline_render_z_pass_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_pass_ecs_query_comps, "outline__isOccluded", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_pass_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(outline_render_z_pass_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(outline_render_z_pass_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RW_COMP(outline_render_z_pass_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_PTR(outline_render_z_pass_ecs_query_comps, "attaches_list", ecs::EidList)
            , ECS_RO_COMP_PTR(outline_render_z_pass_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP_OR(outline_render_z_pass_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_pass_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_pass_ri_ecs_query_comps[] =
{
//start of 10 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__isOccluded"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_pass_ri_ecs_query_desc
(
  "outline_render_z_pass_ri_ecs_query",
  empty_span(),
  make_span(outline_render_z_pass_ri_ecs_query_comps+0, 10)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_z_pass_ri_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_pass_ri_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_pass_ri_ecs_query_comps, "outline__always_visible", bool( false)) && !ECS_RO_COMP_OR(outline_render_z_pass_ri_ecs_query_comps, "outline__z_fail", bool( false)) && ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "outline__isOccluded", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "ri_extra", RiExtraComponent)
            , ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_z_pass_ri_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_z_pass_ri_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_pass_ri_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_pass_ri_handle_ecs_query_comps[] =
{
//start of 10 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra__handle"), ecs::ComponentTypeInfo<rendinst::riex_handle_t>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__isOccluded"), ecs::ComponentTypeInfo<bool>()},
//start of 1 no components at [10]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_pass_ri_handle_ecs_query_desc
(
  "outline_render_z_pass_ri_handle_ecs_query",
  empty_span(),
  make_span(outline_render_z_pass_ri_handle_ecs_query_comps+0, 10)/*ro*/,
  empty_span(),
  make_span(outline_render_z_pass_ri_handle_ecs_query_comps+10, 1)/*no*/);
template<typename Callable>
inline void outline_render_z_pass_ri_handle_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_pass_ri_handle_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__always_visible", bool( false)) && !ECS_RO_COMP_OR(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__z_fail", bool( false)) && ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__isOccluded", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "ri_extra__handle", rendinst::riex_handle_t)
            , ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_z_pass_ri_handle_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_pass_ri_handle_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_fail_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("outline__frames_visible"), ecs::ComponentTypeInfo<int>()},
//start of 12 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__frames_history"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_fail_ecs_query_desc
(
  "outline_render_z_fail_ecs_query",
  make_span(outline_render_z_fail_ecs_query_comps+0, 2)/*rw*/,
  make_span(outline_render_z_fail_ecs_query_comps+2, 12)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_z_fail_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_fail_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_fail_ecs_query_comps, "outline__always_visible", bool( false)) && ECS_RO_COMP_OR(outline_render_z_fail_ecs_query_comps, "animchar_render__enabled", bool( true)) && ECS_RO_COMP(outline_render_z_fail_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_fail_ecs_query_comps, "outline__z_fail", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_fail_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(outline_render_z_fail_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(outline_render_z_fail_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RW_COMP(outline_render_z_fail_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_PTR(outline_render_z_fail_ecs_query_comps, "attaches_list", ecs::EidList)
            , ECS_RO_COMP_PTR(outline_render_z_fail_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RW_COMP(outline_render_z_fail_ecs_query_comps, "outline__frames_visible", int)
            , ECS_RO_COMP_OR(outline_render_z_fail_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_fail_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            , ECS_RO_COMP_OR(outline_render_z_fail_ecs_query_comps, "outline__frames_history", int(20))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_fail_ri_ecs_query_comps[] =
{
//start of 9 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_fail_ri_ecs_query_desc
(
  "outline_render_z_fail_ri_ecs_query",
  empty_span(),
  make_span(outline_render_z_fail_ri_ecs_query_comps+0, 9)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void outline_render_z_fail_ri_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_fail_ri_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_fail_ri_ecs_query_comps, "outline__always_visible", bool( false)) && ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "outline__z_fail", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "ri_extra", RiExtraComponent)
            , ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_z_fail_ri_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_z_fail_ri_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_fail_ri_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc outline_render_z_fail_ri_handle_ecs_query_comps[] =
{
//start of 9 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra__handle"), ecs::ComponentTypeInfo<rendinst::riex_handle_t>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("outline__color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__extcolor"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__always_visible"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("outline__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outline__z_fail"), ecs::ComponentTypeInfo<bool>()},
//start of 1 no components at [9]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()}
};
static ecs::CompileTimeQueryDesc outline_render_z_fail_ri_handle_ecs_query_desc
(
  "outline_render_z_fail_ri_handle_ecs_query",
  empty_span(),
  make_span(outline_render_z_fail_ri_handle_ecs_query_comps+0, 9)/*ro*/,
  empty_span(),
  make_span(outline_render_z_fail_ri_handle_ecs_query_comps+9, 1)/*no*/);
template<typename Callable>
inline void outline_render_z_fail_ri_handle_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, outline_render_z_fail_ri_handle_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP_OR(outline_render_z_fail_ri_handle_ecs_query_comps, "outline__always_visible", bool( false)) && ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "outline__enabled", bool) && ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "outline__z_fail", bool)) )
            continue;
          function(
              ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "ri_extra__handle", rendinst::riex_handle_t)
            , ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "ri_extra__bboxMin", Point3)
            , ECS_RO_COMP(outline_render_z_fail_ri_handle_ecs_query_comps, "ri_extra__bboxMax", Point3)
            , ECS_RO_COMP_OR(outline_render_z_fail_ri_handle_ecs_query_comps, "outline__color", E3DCOLOR(0xFFFFFFFF))
            , ECS_RO_COMP_OR(outline_render_z_fail_ri_handle_ecs_query_comps, "outline__extcolor", E3DCOLOR(0))
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc attach_ecs_query_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc attach_ecs_query_desc
(
  "attach_ecs_query",
  empty_span(),
  make_span(attach_ecs_query_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void attach_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, attach_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          if ( !(ECS_RO_COMP_OR(attach_ecs_query_comps, "animchar_render__enabled", bool( true))) )
            return;
          function(
              ECS_RO_COMP(attach_ecs_query_comps, "slot_attach__attachedTo", ecs::EntityId)
            , ECS_RO_COMP(attach_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(attach_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RO_COMP(attach_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_PTR(attach_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP_PTR(attach_ecs_query_comps, "attaches_list", ecs::EidList)
            );

        }
      }
  );
}
static constexpr ecs::ComponentDesc render_outline_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("outline_renderer"), ecs::ComponentTypeInfo<OutlineRenderer>()}
};
static ecs::CompileTimeQueryDesc render_outline_ecs_query_desc
(
  "render_outline_ecs_query",
  make_span(render_outline_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void render_outline_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, render_outline_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(render_outline_ecs_query_comps, "outline_renderer", OutlineRenderer)
            );

        }while (++comp != compE);
    }
  );
}
