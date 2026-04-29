// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "das_ecs.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcsContainer.h>

#define DECL_LIST_TYPE(lt, t) IMPLEMENT_EXTERNAL_TYPE_FACTORY(Shared##lt, ecs::SharedComponent<ecs::lt>);
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

namespace bind_dascript
{

#define DECL_LIST_ANNOTATION(T)                                                                           \
  struct T##SharedAnnotation final : das::ManagedStructureAnnotation<ecs::SharedComponent<ecs::T>, false> \
  {                                                                                                       \
    T##SharedAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Shared" #T, ml)             \
    {                                                                                                     \
      cppName = "ecs::SharedComponent< ::ecs::" #T ">";                                                   \
      addPropertyForManagedType<DAS_BIND_MANAGED_PROP(get)>("get");                                       \
    }                                                                                                     \
  };

#define DECL_LIST_TYPE(lt, t) DECL_LIST_ANNOTATION(lt)
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

#undef DECL_LIST_ANNOTATION

void ECS::addSharedList(das::ModuleLibrary &lib)
{
#define DECL_LIST_TYPE(lt, t) addAnnotation(das::make_smart<lt##SharedAnnotation>(lib));
  ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
}

} // namespace bind_dascript
