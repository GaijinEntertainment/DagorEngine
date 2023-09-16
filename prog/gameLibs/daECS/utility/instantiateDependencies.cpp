#include <daECS/core/componentType.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/sharedComponent.h>

#include <EASTL/string.h>
#include <memory/dag_framemem.h>

namespace ecs
{

struct TemplatesListToInstantiate
{
  static void requestResources(const char *component_name, const ecs::resource_request_cb_t &res_cb)
  {
    using fstr = eastl::basic_string<char, framemem_allocator>;
    const auto templatesComponentName = fstr{fstr::CtorSprintf{}, "%s_list", component_name};
    const auto templatesComponentNameShared = templatesComponentName + "_shared";
    const ecs::StringList *templatesList = nullptr;
    // Allow the "list of templates to instantiate" to be shared or non-shared
    if (
      const auto list = res_cb.getNullable<ecs::SharedComponent<ecs::StringList>>(ECS_HASH_SLOW(templatesComponentNameShared.c_str())))
      templatesList = list->get();
    else
      templatesList = res_cb.getNullable<ecs::StringList>(ECS_HASH_SLOW(templatesComponentName.c_str()));
    if (DAGOR_UNLIKELY(templatesList == nullptr || templatesList->empty()))
    {
      logerr("Expected non-empty list of templates to instantiate in component %s[_shared]", templatesComponentName.c_str());
      return;
    }
    for (const auto &t : *templatesList)
      g_entity_mgr->templateByName(t.c_str());
  }
};

} // namespace ecs

ECS_DECLARE_RELOCATABLE_TYPE(ecs::TemplatesListToInstantiate);
ECS_REGISTER_RELOCATABLE_TYPE(ecs::TemplatesListToInstantiate, nullptr);
ECS_DEF_PULL_VAR(instantiateDependencies);
