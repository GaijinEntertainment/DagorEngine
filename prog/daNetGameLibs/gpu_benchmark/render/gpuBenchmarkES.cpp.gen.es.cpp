#include "gpuBenchmarkES.cpp.inl"
ECS_DEF_PULL_VAR(gpuBenchmark);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc graphics_autodetect_wrapper_on_appear_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("graphics_auto_detect"), ecs::ComponentTypeInfo<GraphicsAutodetectWrapper>()}
};
static void graphics_autodetect_wrapper_on_appear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    graphics_autodetect_wrapper_on_appear_es_event_handler(evt
        , ECS_RW_COMP(graphics_autodetect_wrapper_on_appear_es_event_handler_comps, "graphics_auto_detect", GraphicsAutodetectWrapper)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc graphics_autodetect_wrapper_on_appear_es_event_handler_es_desc
(
  "graphics_autodetect_wrapper_on_appear_es",
  "prog/daNetGameLibs/gpu_benchmark/render/gpuBenchmarkES.cpp.inl",
  ecs::EntitySystemOps(nullptr, graphics_autodetect_wrapper_on_appear_es_event_handler_all_events),
  make_span(graphics_autodetect_wrapper_on_appear_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc graphics_autodetect_before_render_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("graphics_auto_detect"), ecs::ComponentTypeInfo<GraphicsAutodetectWrapper>()},
  {ECS_HASH("graphics_auto_detect_tex"), ecs::ComponentTypeInfo<UniqueTex>()},
  {ECS_HASH("graphics_auto_detect_depth"), ecs::ComponentTypeInfo<UniqueTex>()},
//start of 1 ro components at [3]
  {ECS_HASH("selfdestruct"), ecs::ComponentTypeInfo<bool>()}
};
static void graphics_autodetect_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    graphics_autodetect_before_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(graphics_autodetect_before_render_es_comps, "graphics_auto_detect", GraphicsAutodetectWrapper)
    , ECS_RW_COMP(graphics_autodetect_before_render_es_comps, "graphics_auto_detect_tex", UniqueTex)
    , ECS_RW_COMP(graphics_autodetect_before_render_es_comps, "graphics_auto_detect_depth", UniqueTex)
    , ECS_RO_COMP(graphics_autodetect_before_render_es_comps, "selfdestruct", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc graphics_autodetect_before_render_es_es_desc
(
  "graphics_autodetect_before_render_es",
  "prog/daNetGameLibs/gpu_benchmark/render/gpuBenchmarkES.cpp.inl",
  ecs::EntitySystemOps(nullptr, graphics_autodetect_before_render_es_all_events),
  make_span(graphics_autodetect_before_render_es_comps+0, 3)/*rw*/,
  make_span(graphics_autodetect_before_render_es_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_graphics_autodetect_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("graphics_auto_detect"), ecs::ComponentTypeInfo<GraphicsAutodetectWrapper>()}
};
static ecs::CompileTimeQueryDesc get_graphics_autodetect_ecs_query_desc
(
  "get_graphics_autodetect_ecs_query",
  make_span(get_graphics_autodetect_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_graphics_autodetect_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_graphics_autodetect_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_graphics_autodetect_ecs_query_comps, "graphics_auto_detect", GraphicsAutodetectWrapper)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc delete_graphics_autodetect_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("graphics_auto_detect"), ecs::ComponentTypeInfo<GraphicsAutodetectWrapper>()}
};
static ecs::CompileTimeQueryDesc delete_graphics_autodetect_ecs_query_desc
(
  "delete_graphics_autodetect_ecs_query",
  empty_span(),
  make_span(delete_graphics_autodetect_ecs_query_comps+0, 1)/*ro*/,
  make_span(delete_graphics_autodetect_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void delete_graphics_autodetect_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, delete_graphics_autodetect_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(delete_graphics_autodetect_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
