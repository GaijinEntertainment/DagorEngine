//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propsRegistry/propsRegistry.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>

namespace propsreg
{
template <typename T>
class CommonPropsRegistry : public PropsRegistry
{
  eastl::vector<eastl::unique_ptr<T>> propsRegistry;

public:
#if DAGOR_DBGLEVEL > 0
  const char *debugPropsClassName;
  CommonPropsRegistry(const char *pname) : debugPropsClassName(pname) {}
#else
  CommonPropsRegistry(const char *) {}
#endif

  virtual void registerProps(int prop_id, const DataBlock *blk)
  {
    if (prop_id >= propsRegistry.size())
      propsRegistry.resize(prop_id + 1);
    if (!propsRegistry[prop_id] && T::can_load(blk))
    {
      T *newProps = new T();
      newProps->load(blk);
      propsRegistry[prop_id].reset(newProps);
    }
  }

  virtual void clearProps() { propsRegistry.clear(); }

  const T *tryGetProps(int prop_id) const { return (unsigned)prop_id < propsRegistry.size() ? propsRegistry[prop_id].get() : nullptr; }

  const T *getProps(int prop_id) const
  {
#if DAGOR_DBGLEVEL > 0
    if (DAGOR_UNLIKELY((unsigned)prop_id >= propsRegistry.size()))
    {
      logerr("%s|%s: unsigned(%d) >= %d", __FUNCTION__, debugPropsClassName, prop_id, (int)propsRegistry.size());
      return nullptr;
    }
#endif
    return tryGetProps(prop_id);
  }
};
} // namespace propsreg

#define PROPS_REGISTRY_DEF_CAN_LOAD(PropType) \
  bool PropType::can_load(const DataBlock *) { return true; }

#define PROPS_REGISTER_REGISTRY(PropType, prop_name, prop_class_name) \
  static propsreg::CommonPropsRegistry<PropType> prop_name##_registry(prop_class_name);

#define PROPS_REGISTRY_IMPL_EX_IMPL(PropType, prop_name, prop_class_name, templ_prefix)                            \
  PROPS_REGISTER_REGISTRY(PropType, prop_name, prop_class_name);                                                   \
  templ_prefix const PropType *PropType::get_props(int prop_id) { return prop_name##_registry.getProps(prop_id); } \
  templ_prefix void PropType::register_props_class() { propsreg::register_props_class(prop_name##_registry, prop_class_name); }

#define PROPS_REGISTRY_IMPL_EX(PropType, prop_name, prop_class_name) \
  PROPS_REGISTRY_IMPL_EX_IMPL(PropType, prop_name, prop_class_name, );

#define PROPS_IMPL_COMMON(PropType, prop_name, prop_class_name)                                    \
  static const PropType *get_props(int prop_id) { return prop_name##_registry.getProps(prop_id); } \
  static void register_props_class() { propsreg::register_props_class(prop_name##_registry, prop_class_name); }


#define PROPS_REGISTRY_IMPL(PropType, prop_name, prop_class_name) \
  PROPS_REGISTRY_IMPL_EX(PropType, prop_name, prop_class_name);   \
  PROPS_REGISTRY_DEF_CAN_LOAD(PropType);

#define PROPS_REGISTRY_IMPL_SP(PropType, prop_name, prop_class_name) \
  PROPS_REGISTRY_IMPL_EX(PropType, prop_name, prop_class_name);      \
  const PropType *PropType::try_get_props(int prop_id) { return prop_name##_registry.tryGetProps(prop_id); }

#define PROPS_REGISTRY_IMPL_EX_STUB(PropType)               \
  const PropType *PropType::get_props(int) { return NULL; } \
  void PropType::register_props_class() {}

#define PROPS_REGISTRY_DEF_CAN_LOAD_STUB(PropType) \
  bool PropType::can_load(const DataBlock *) { return false; }

#define PROPS_REGISTRY_DEF_LOAD_STUB(PropType) \
  void PropType::load(const DataBlock *) {}

#define PROPS_REGISTRY_IMPL_STUB(PropType)    \
  PROPS_REGISTRY_IMPL_EX_STUB(PropType);      \
  PROPS_REGISTRY_DEF_CAN_LOAD_STUB(PropType); \
  PROPS_REGISTRY_DEF_LOAD_STUB(PropType);
