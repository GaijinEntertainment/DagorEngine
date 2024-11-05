//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityManager.h>
#include <ecs/core/utility/ecsBlkUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasDataBlock.h>
#include <daECS/scene/scene.h>
#include <daECS/io/blk.h>

MAKE_TYPE_FACTORY(ComponentType, ecs::ComponentType)
MAKE_TYPE_FACTORY(ComponentTypes, ecs::ComponentTypes);
MAKE_TYPE_FACTORY(DataComponent, ecs::DataComponent);
MAKE_TYPE_FACTORY(DataComponents, ecs::DataComponents);
MAKE_TYPE_FACTORY(ComponentsList, ecs::ComponentsList);
MAKE_TYPE_FACTORY(TemplateDB, ecs::TemplateDB);
MAKE_TYPE_FACTORY(Scene, ecs::Scene);
MAKE_TYPE_FACTORY(EntitySystemDesc, ecs::EntitySystemDesc)
MAKE_TYPE_FACTORY(ComponentDesc, ecs::ComponentDesc)
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::ecs::ComponentTypeFlags, ComponentTypeFlags)


namespace bind_dascript
{
inline const ecs::ComponentTypes &getComponentTypes() { return g_entity_mgr->getComponentTypes(); }
inline const ecs::DataComponents &getDataComponents() { return g_entity_mgr->getDataComponents(); }
inline const ecs::TemplateDB &getTemplateDB() { return g_entity_mgr->getTemplateDB(); }
inline void components_to_blk(const ecs::EntityId eid, DataBlock &blk, const char *prefix, bool sep_on_dot)
{
  ecs::components_to_blk(g_entity_mgr->getComponentsIterator(eid), false, blk, prefix, sep_on_dot);
}
inline void components_list_to_blk(const ecs::ComponentsList &list, DataBlock &blk, const char *prefix, bool sep_on_dot)
{
  ecs::components_to_blk(list.cbegin(), list.cend(), blk, prefix, sep_on_dot);
}
inline void component_to_blk(const char *name, const ecs::EntityComponentRef &comp, DataBlock &blk)
{
  ecs::component_to_blk(name ? name : "", comp, blk);
}
inline void component_to_blk_param(const char *name, const ecs::EntityComponentRef &comp, DataBlock &blk)
{
  ecs::component_to_blk_param(name ? name : "", comp, &blk);
}

// TODO: Remove this binding and bind ComponentsList as a vector type
inline bool find_component(const ecs::ComponentsList &list,
  const das::TBlock<bool, const char *, const das::TTemporary<const ecs::ChildComponent>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  vec4f args[2];
  bool found = false;
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      for (const auto &component : list)
      {
        args[0] = das::cast<const char *>::from(component.first.c_str());
        args[1] = das::cast<const ecs::ChildComponent &>::from(component.second);
        found = code->evalBool(*context);
        if (found)
          break;
      }
    },
    at);
  return found;
}

inline bool find_component_eid(ecs::EntityId eid,
  const das::TBlock<bool, das::TTemporary<const char *>, das::TTemporary<ecs::EntityComponentRef>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  bool found = false;
  vec4f args[2];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      for (auto aiter = g_entity_mgr->getComponentsIterator(eid); !found && aiter; ++aiter)
      {
        eastl::pair<const char *, const ecs::EntityComponentRef> info = *aiter;
        args[0] = das::cast<char *>::from(info.first);
        args[1] = das::cast<ecs::EntityComponentRef &>::from(info.second);
        found = code->evalBool(*context);
      }
    },
    at);
  return found;
}
inline ecs::Scene &get_active_scene() { return ecs::g_scenes->getActiveScene(); }
bool das_get_underlying_ecs_type(das::TypeDeclPtr info, bool with_module_name,
  const das::TBlock<void, das::TTemporary<const char *>> &block, das::Context *context, das::LineInfoArg *at);

inline bool find_templateDB(const ecs::TemplateDB &db,
  const das::TBlock<bool, das::TTemporary<const char *>, das::TTemporary<ecs::Template>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  vec4f args[2];
  bool found = false;
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      for (auto &pair : db)
      {
        args[0] = das::cast<char *>::from(pair.getName());
        args[1] = das::cast<ecs::Template &>::from(pair);
        found = code->evalBool(*context);
        if (found)
          break;
      }
    },
    at);
  return found;
}

inline bool find_systemDB(const das::TBlock<bool, das::TTemporary<const char *>, das::TTemporary<ecs::EntitySystemDesc>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  auto systems = g_entity_mgr->getSystems();
  vec4f args[2];
  bool found = false;
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      for (const ecs::EntitySystemDesc *s : systems)
      {
        args[0] = das::cast<char *>::from(s->name);
        args[1] = das::cast<ecs::EntitySystemDesc &>::from(*s);
        found = code->evalBool(*context);
        if (found)
          break;
      }
    },
    at);
  return found;
}

inline void getEvSet(const ecs::EntitySystemDesc &sys,
  const das::TBlock<void, const das::TTemporary<const das::TArray<ecs::event_type_t>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  const ecs::EventSet &eventsSet = sys.getEvSet();
  das::Array arr;
  arr.data = (char *)eventsSet.cbegin();
  arr.size = eventsSet.size();
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline const char *component_desc_name(const ecs::ComponentDesc &comp)
{
#if DAGOR_DBGLEVEL > 0
  return comp.nameStr;
#else
  G_UNUSED(comp);
  return nullptr;
#endif
}

inline void query_componentsRW(const ecs::EntitySystemDesc &sys,
  const das::TBlock<void, const das::TTemporary<const das::TArray<const ecs::ComponentDesc>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  auto &comps = sys.componentsRW;
  das::Array arr;
  arr.data = (char *)comps.data();
  arr.size = comps.size();
  arr.capacity = comps.size();
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void query_componentsRO(const ecs::EntitySystemDesc &sys,
  const das::TBlock<void, const das::TTemporary<const das::TArray<const ecs::ComponentDesc>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  auto &comps = sys.componentsRO;
  das::Array arr;
  arr.data = (char *)comps.data();
  arr.size = comps.size();
  arr.capacity = comps.size();
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void query_componentsRQ(const ecs::EntitySystemDesc &sys,
  const das::TBlock<void, const das::TTemporary<const das::TArray<const ecs::ComponentDesc>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  auto &comps = sys.componentsRQ;
  das::Array arr;
  arr.data = (char *)comps.data();
  arr.size = comps.size();
  arr.capacity = comps.size();
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void query_componentsNO(const ecs::EntitySystemDesc &sys,
  const das::TBlock<void, const das::TTemporary<const das::TArray<const ecs::ComponentDesc>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  auto &comps = sys.componentsNO;
  das::Array arr;
  arr.data = (char *)comps.data();
  arr.size = comps.size();
  arr.capacity = comps.size();
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void create_entities_blk(const DataBlock &blk, const char *blk_path) { ecs::create_entities_blk(*g_entity_mgr, blk, blk_path); }
inline void load_comp_list_from_blk(const DataBlock &blk, ecs::ComponentsList &alist)
{
  ecs::load_comp_list_from_blk(*g_entity_mgr, blk, alist);
}

inline const ecs::EntityComponentRef getComponentRef(const ecs::EntityManager &mgr, ecs::EntityId eid, const char *name)
{
  return mgr.getComponentRef(eid, ECS_HASH_SLOW(name ? name : ""));
}

} // namespace bind_dascript
