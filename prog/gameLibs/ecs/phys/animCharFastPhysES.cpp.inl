#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <ecs/anim/anim.h>
#include <animChar/dag_animCharacter2.h>
#include <debug/dag_assert.h>
#include <phys/dag_fastPhys.h>
#include <gameRes/dag_stdGameRes.h>
#include <gamePhys/props/atmosphere.h>

using namespace AnimV20;
struct FastPhysTag
{}; // just a tag. We can't use ecs::Tag because we have constructor. But data is stored within Animchar. Unfortunately

// we can of course use ECS_DECLARE_TYPE, but than it would be type of size 1.
//  Empty structure (size 0) isn't common type component (I think it is the only one!), but because we pass ownership to AnimChar, it
//  is already there, so... and we have to have CTM (for component)

namespace ecs
{
template <>
struct ComponentTypeInfo<FastPhysTag>
{
  static constexpr const char *type_name = "FastPhysTag";
  static constexpr component_type_t type = ECS_HASH(type_name).hash;
  static constexpr bool is_pod_class = false;
  static constexpr bool is_boxed = false;
  static constexpr bool is_non_trivial_move = false;
  static constexpr bool is_legacy = false;
  static constexpr bool is_create_on_templ_inst = false;
  static constexpr size_t size = 0;
};
} // namespace ecs

class FastPhysCTM final : public ecs::ComponentTypeManager
{
public:
  typedef FastPhysTag component_type;

  void requestResources(const char *, const ecs::resource_request_cb_t &res_cb) override
  {
    auto &res = res_cb.get<ecs::string>(ECS_HASH("animchar_fast_phys__res"));
    res_cb(!res.empty() ? res.c_str() : "<missing_animchar_fast_phys>", FastPhysDataGameResClassId);
  }

  void create(void *, ecs::EntityManager &mgr, ecs::EntityId eid, const ecs::ComponentsMap &, ecs::component_index_t) override
  {
    const char *res = mgr.get<ecs::string>(eid, ECS_HASH("animchar_fast_phys__res")).c_str();
    FastPhysSystem *fpsys = create_fast_phys_from_gameres(res);
    if (!fpsys)
    {
      logerr("Cannot create fastphys by res '%s' for entity %d<%s>", res, (ecs::entity_id_t)eid,
        g_entity_mgr->getEntityTemplateName(eid));
      return;
    }
    AnimcharBaseComponent *animChar = g_entity_mgr->getNullableRW<AnimcharBaseComponent>(eid, ECS_HASH("animchar"));
    animChar->setFastPhysSystem(fpsys); // Note: fpsys get owned by animchar
  }
};

ECS_REGISTER_TYPE_BASE(FastPhysTag, ecs::ComponentTypeInfo<FastPhysTag>::type_name, nullptr, &ecs::CTMFactory<FastPhysCTM>::create,
  &ecs::CTMFactory<FastPhysCTM>::destroy, ecs::COMPONENT_TYPE_NEED_RESOURCES);
ECS_AUTO_REGISTER_COMPONENT(ecs::string, "animchar_fast_phys__res", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(FastPhysTag, "animchar_fast_phys", nullptr, 0, "animchar", "animchar_fast_phys__res");

ECS_ON_EVENT(on_appear) // todo: add wind change event, if happen
ECS_REQUIRE(FastPhysTag animchar_fast_phys)
static inline void animchar_fast_phys_es_event_handler(const ecs::Event &, AnimcharBaseComponent &animchar)
{
  if (!animchar.getFastPhysSystem())
    return;
  Point3 wind = gamephys::atmosphere::get_wind();
  float windLen = length(wind); // can be optimized
  animchar.getFastPhysSystem()->windVel = wind;
  animchar.getFastPhysSystem()->windPower = windLen;
  animchar.getFastPhysSystem()->windTurb = windLen;
}


ECS_REQUIRE(FastPhysTag animchar_fast_phys)
ECS_ON_EVENT(on_disappear)
static void animchar_fast_phys_destroy_es_event_handler(const ecs::Event &, AnimcharBaseComponent &animchar)
{
  if (FastPhysSystem *fastPhys = animchar.getFastPhysSystem())
  {
    animchar.setFastPhysSystem(nullptr);
    delete fastPhys;
  }
}
