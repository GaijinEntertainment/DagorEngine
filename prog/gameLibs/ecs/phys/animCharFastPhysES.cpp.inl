#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <ecs/anim/anim.h>
#include <animChar/dag_animCharacter2.h>
#include <debug/dag_assert.h>
#include <phys/dag_fastPhys.h>
#include <gameRes/dag_stdGameRes.h>
#include <gamePhys/props/atmosphere.h>
#include <ecs/render/updateStageRender.h>
#include <debug/dag_debug3d.h>
#include <util/dag_console.h>
#include <EASTL/vector_set.h>
#include <ecs/core/utility/ecsRecreate.h>


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

eastl::vector_set<eastl::string> debugAnimCharsSet;
#define TEMPLATE_NAME "animchar_fast_phys_debug_render"
const char *template_name = TEMPLATE_NAME;
void createTemplate()
{
  ecs::ComponentsMap map;
  map[ECS_HASH(TEMPLATE_NAME)] = ecs::Tag();
  eastl::string name = template_name;
  g_entity_mgr->addTemplate(ecs::Template(template_name, eastl::move(map), ecs::Template::component_set(),
    ecs::Template::component_set(), ecs::Template::component_set(), false));
  g_entity_mgr->instantiateTemplate(g_entity_mgr->buildTemplateIdByName(template_name));
}

void removeSubTemplateAsync(ecs::EntityId eid)
{
  if (const char *fromTemplate = g_entity_mgr->getEntityTemplateName(eid))
  {
    if (!g_entity_mgr->getTemplateDB().getTemplateByName(template_name))
      createTemplate();
    const auto newTemplate = remove_sub_template_name(fromTemplate, template_name ? template_name : "");
    g_entity_mgr->reCreateEntityFromAsync(eid, newTemplate.c_str());
  }
}

void addSubTemplateAsync(ecs::EntityId eid)
{
  if (const char *fromTemplate = g_entity_mgr->getEntityTemplateName(eid))
  {
    if (!g_entity_mgr->getTemplateDB().getTemplateByName(template_name))
      createTemplate();
    const auto newTemplate = add_sub_template_name(fromTemplate, template_name ? template_name : "");
    g_entity_mgr->reCreateEntityFromAsync(eid, newTemplate.c_str());
  }
}

template <typename Callable>
static void get_animchar_by_name_ecs_query(Callable c);


void toggleDebugAnimChar(eastl::string &str)
{
  auto it = debugAnimCharsSet.find(str);
  if (it != debugAnimCharsSet.end())
  {
    get_animchar_by_name_ecs_query([&](ecs::EntityId eid, ecs::string &animchar__res) {
      if (animchar__res == str)
        removeSubTemplateAsync(eid);
    });
    debugAnimCharsSet.erase(it);
  }
  else
  {
    get_animchar_by_name_ecs_query([&](ecs::EntityId eid, ecs::string &animchar__res) {
      if (animchar__res == str)
        addSubTemplateAsync(eid);
    });
    debugAnimCharsSet.insert(str);
  }
}

void resetDebugAnimChars()
{
  for (auto it = debugAnimCharsSet.begin(); it != debugAnimCharsSet.end(); ++it)
  {
    eastl::string str = *it;
    get_animchar_by_name_ecs_query([&](ecs::EntityId eid, ecs::string &animchar__res) {
      if (animchar__res == str)
        removeSubTemplateAsync(eid);
    });
  }
  debugAnimCharsSet.clear();
}

ECS_NO_ORDER
ECS_REQUIRE(FastPhysTag animchar_fast_phys, ecs::Tag animchar_fast_phys_debug_render)
ECS_TAG(dev, render)
static void debug_draw_fast_phys_es(const UpdateStageInfoRenderDebug &, AnimcharBaseComponent &animchar)
{
  FastPhysSystem *fastPhys = animchar.getFastPhysSystem();
  if (!fastPhys)
    return;
  begin_draw_cached_debug_lines();

  for (auto action : fastPhys->updateActions)
    action->debugRender();

  end_draw_cached_debug_lines();
}

static bool fastphys_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("fastphys", "debug", 1, 2)
  {
    if (argc > 1)
    {
      eastl::string resName(argv[1]);
      toggleDebugAnimChar(resName);
    }
    else
      resetDebugAnimChars();
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(fastphys_console_handler);
