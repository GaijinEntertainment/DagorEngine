#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>

void (*tonemap_changed_callback)() = nullptr;

void set_tonemap_changed_callback(void (*callback)()) { tonemap_changed_callback = callback; }

ECS_REQUIRE(ecs::Tag tonemapper)
ECS_TRACK(white_balance, color_grading, tonemap)
ECS_REQUIRE(ecs::Object white_balance, ecs::Object color_grading, ecs::Object tonemap)
ECS_ON_EVENT(on_disappear, on_appear)
static __forceinline void tonemap_update_es_event_handler(const ecs::Event &)
{
  if (tonemap_changed_callback)
    tonemap_changed_callback();
}
