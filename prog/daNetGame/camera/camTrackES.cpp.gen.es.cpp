#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t camtrack__filename_get_type();
static ecs::LTComponentList camtrack__filename_component(ECS_HASH("camtrack__filename"), camtrack__filename_get_type(), "prog/daNetGame/camera/camTrackES.cpp.inl", "", 0);
#include "camTrackES.cpp.inl"
ECS_DEF_PULL_VAR(camTrack);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc camtrack_updater_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("fov"), ecs::ComponentTypeInfo<float>()},
//start of 1 ro components at [2]
  {ECS_HASH("camtrack"), ecs::ComponentTypeInfo<CamTrack>()}
};
static void camtrack_updater_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    camtrack_updater_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(camtrack_updater_es_comps, "camtrack", CamTrack)
    , ECS_RW_COMP(camtrack_updater_es_comps, "transform", TMatrix)
    , ECS_RW_COMP(camtrack_updater_es_comps, "fov", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc camtrack_updater_es_es_desc
(
  "camtrack_updater_es",
  "prog/daNetGame/camera/camTrackES.cpp.inl",
  ecs::EntitySystemOps(camtrack_updater_es_all),
  make_span(camtrack_updater_es_comps+0, 2)/*rw*/,
  make_span(camtrack_updater_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,nullptr,"camtrack_executor_es");
static constexpr ecs::component_t camtrack__filename_get_type(){return ecs::ComponentTypeInfo<eastl::basic_string<char, eastl::allocator>>::type; }
