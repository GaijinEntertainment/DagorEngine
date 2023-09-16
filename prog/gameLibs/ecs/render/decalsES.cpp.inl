#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <billboardDecals/billboardDecals.h>
#include <ecs/rendInst/riExtra.h>
#include <ecs/render/decalsES.h>

namespace decals
{
void (*update_bullet_hole)(uint32_t matrix_id, const TMatrix &matrix) = nullptr;

uint32_t (*add_new_matrix_id_bullet_hole)() = nullptr;
void (*del_matrix_id_bullet_hole)(uint32_t matrix_id) = nullptr;

void (*entity_update)(ecs::EntityId eid, const TMatrix &tm) = nullptr;
void (*entity_delete)(ecs::EntityId eid) = nullptr;
}; // namespace decals

ECS_TRACK(transform)
ECS_REQUIRE(eastl::true_type decals__useDecalMatrices)
static __forceinline void decals_es_event_handler(const ecs::Event &, ecs::EntityId eid, const TMatrix &transform)
{
  G_ASSERT(decals::entity_update);
  decals::entity_update(eid, transform);
}

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(eastl::true_type decals__useDecalMatrices)
static __forceinline void decals_delete_es_event_handler(const ecs::Event &, ecs::EntityId eid)
{
  G_ASSERT(decals::entity_delete);
  decals::entity_delete(eid);
}
