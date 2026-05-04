// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "cacheThrashingDetectorES.cpp.inl"
ECS_DEF_PULL_VAR(cacheThrashingDetector);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc cache_thrashing_detector_subscribe_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("cache_thrashing_detector__callback_token"), ecs::ComponentTypeInfo<CallbackToken>()},
//start of 2 ro components at [1]
  {ECS_HASH("cache_thrashing_detector__logerr_at"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("cache_thrashing_detector__multiplier"), ecs::ComponentTypeInfo<int>()}
};
static void cache_thrashing_detector_subscribe_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    cache_thrashing_detector_subscribe_es(evt
        , ECS_RO_COMP(cache_thrashing_detector_subscribe_es_comps, "cache_thrashing_detector__logerr_at", int)
    , ECS_RO_COMP(cache_thrashing_detector_subscribe_es_comps, "cache_thrashing_detector__multiplier", int)
    , ECS_RW_COMP(cache_thrashing_detector_subscribe_es_comps, "cache_thrashing_detector__callback_token", CallbackToken)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cache_thrashing_detector_subscribe_es_es_desc
(
  "cache_thrashing_detector_subscribe_es",
  "prog/daNetGameLibs/cache_thrashing_detector/render/cacheThrashingDetectorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, cache_thrashing_detector_subscribe_es_all_events),
  make_span(cache_thrashing_detector_subscribe_es_comps+0, 1)/*rw*/,
  make_span(cache_thrashing_detector_subscribe_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"dev,render");
