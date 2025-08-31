// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "destructablesRenderES.cpp.inl"
ECS_DEF_PULL_VAR(destructablesRender);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc destructables_before_render_es_comps[] ={};
static void destructables_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  destructables_before_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        );
}
static ecs::EntitySystemDesc destructables_before_render_es_es_desc
(
  "destructables_before_render_es",
  "prog/daNetGame/render/world/destructablesRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destructables_before_render_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
//static constexpr ecs::ComponentDesc destructables_render_es_comps[] ={};
static void destructables_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoRender>());
  destructables_render_es(static_cast<const UpdateStageInfoRender&>(evt)
        );
}
static ecs::EntitySystemDesc destructables_render_es_es_desc
(
  "destructables_render_es",
  "prog/daNetGame/render/world/destructablesRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destructables_render_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRender>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc destructables_render_decals_es_comps[] ={};
static void destructables_render_decals_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<RenderDecalsOnDynamic>());
  destructables_render_decals_es(static_cast<const RenderDecalsOnDynamic&>(evt)
        );
}
static ecs::EntitySystemDesc destructables_render_decals_es_es_desc
(
  "destructables_render_decals_es",
  "prog/daNetGame/render/world/destructablesRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destructables_render_decals_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderDecalsOnDynamic>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc destructables_render_trans_es_comps[] ={};
static void destructables_render_trans_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoRenderTrans>());
  destructables_render_trans_es(static_cast<const UpdateStageInfoRenderTrans&>(evt)
        );
}
static ecs::EntitySystemDesc destructables_render_trans_es_es_desc
(
  "destructables_render_trans_es",
  "prog/daNetGame/render/world/destructablesRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destructables_render_trans_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRenderTrans>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc bvh_destructables_iterate_es_comps[] ={};
static void bvh_destructables_iterate_es_all_events(ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<BVHAdditionalAnimcharIterate>());
  bvh_destructables_iterate_es(static_cast<BVHAdditionalAnimcharIterate&>(evt)
        );
}
static ecs::EntitySystemDesc bvh_destructables_iterate_es_es_desc
(
  "bvh_destructables_iterate_es",
  "prog/daNetGame/render/world/destructablesRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bvh_destructables_iterate_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BVHAdditionalAnimcharIterate>::build(),
  0
,"render");
