// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "das_ecs.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcsContainer.h>

#define DECL_LIST_TYPE(lt, t)                   \
  IMPLEMENT_EXTERNAL_TYPE_FACTORY(lt, ecs::lt); \
  IMPLEMENT_EXTERNAL_TYPE_FACTORY(Shared##lt, ecs::SharedComponent<ecs::lt>);
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

namespace bind_dascript
{

#define DECL_LIST_ANNOTATION(T)                                                                                                  \
  struct T##Annotation : das::ManagedVectorAnnotation<ecs::T>                                                                    \
  {                                                                                                                              \
    using OT = typename ecs::T::value_type;                                                                                      \
    das::TypeDeclPtr parentType;                                                                                                 \
    T##Annotation(das::ModuleLibrary &ml) : das::ManagedVectorAnnotation<ecs::T>(#T, ml)                                         \
    {                                                                                                                            \
      cppName = " ::ecs::" #T;                                                                                                   \
      parentType = das::makeType<das::vector<OT>>(ml);                                                                           \
    }                                                                                                                            \
    virtual das::string getSmartAnnotationCloneFunction() const override { return "smart_ptr_clone"; }                           \
    virtual bool canBeSubstituted(TypeAnnotation *passType) const override { return parentType->annotation == passType; }        \
    virtual bool canClone() const override { return true; }                                                                      \
    das::SimNode *simulateClone(das::Context &context, const das::LineInfo &at, das::SimNode *l, das::SimNode *r) const override \
    {                                                                                                                            \
      return context.code->makeNode<das::SimNode_CloneRefValueT<ecs::T>>(at, l, r);                                              \
    }                                                                                                                            \
  };                                                                                                                             \
  struct T##SharedAnnotation final : das::ManagedStructureAnnotation<ecs::SharedComponent<ecs::T>, false>                        \
  {                                                                                                                              \
    T##SharedAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Shared" #T, ml)                                    \
    {                                                                                                                            \
      cppName = "ecs::SharedComponent< ::ecs::" #T ">";                                                                          \
      addPropertyForManagedType<DAS_BIND_MANAGED_PROP(get)>("get");                                                              \
    }                                                                                                                            \
  };

#define DECL_LIST_TYPE(lt, t) DECL_LIST_ANNOTATION(lt)
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

#undef DECL_LIST_ANNOTATION

void ECS::addList(das::ModuleLibrary &lib)
{
#define DECL_LIST_TYPE(lt, t)                          \
  addAnnotation(das::make_smart<lt##Annotation>(lib)); \
  addAnnotation(das::make_smart<lt##SharedAnnotation>(lib));

  ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
  static constexpr bool is_same = sizeof(das::vector<int>) == sizeof(ecs::List<int>::base_type); // parentType =
                                                                                                 // das::makeType<das::vector<OT>>(ml);
                                                                                                 // relies on that!
  G_STATIC_ASSERT(is_same);
}

} // namespace bind_dascript
