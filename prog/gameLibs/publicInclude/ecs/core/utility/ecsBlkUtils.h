//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <daECS/core/componentTypes.h>
#include <debug/dag_assert.h>
#include <util/dag_string.h>
#include <math/dag_e3dColor.h>
#include <daECS/core/sharedComponent.h>


#define COMP_SEPARATOR "__"
namespace ecs
{
#define LIST_TYPE(NM, T, WR_SUF, CVT_SUFF)                   \
  case ComponentTypeInfo<ecs::List<T>>::type:                \
  {                                                          \
    String blkName(0, "%s:list<%s>", paramName, #NM);        \
    DataBlock *arrBlk = currBlk->addNewBlock(blkName.str()); \
    for (auto &child : comp.get<ecs::List<T>>())             \
      arrBlk->add##WR_SUF("item", child CVT_SUFF);           \
    return;                                                  \
  }

#define LIST_TYPES                         \
  LIST_TYPE(i, int, Int, )                 \
  LIST_TYPE(u8, uint8_t, Int, )            \
  LIST_TYPE(u16, uint16_t, Int, )          \
  LIST_TYPE(t, ecs::string, Str, .c_str()) \
  LIST_TYPE(r, float, Real, )              \
  LIST_TYPE(p2, Point2, Point2, )          \
  LIST_TYPE(p3, Point3, Point3, )          \
  LIST_TYPE(p4, Point4, Point4, )          \
  LIST_TYPE(ip2, IPoint2, IPoint2, )       \
  LIST_TYPE(ip3, IPoint3, IPoint3, )       \
  LIST_TYPE(b, bool, Bool, )               \
  LIST_TYPE(m, TMatrix, Tm, )              \
  LIST_TYPE(c, E3DCOLOR, E3dcolor, )       \
  LIST_TYPE(i64, int64_t, Int64, )


inline void component_to_blk_param(const char *paramName, const ecs::EntityComponentRef &comp, DataBlock *currBlk)
{
  uint32_t ctype = comp.getUserType();
#define PROC_COMP(Type, type_name)                        \
  case ComponentTypeInfo<type_name>::type:                \
  {                                                       \
    currBlk->add##Type(paramName, comp.get<type_name>()); \
    return;                                               \
  }

#define PROC_BLOCK_COMP(block_type_name, Type, type_name)     \
  case ComponentTypeInfo<type_name>::type:                    \
  {                                                           \
    String blkName(0, "%s:" #block_type_name, paramName);     \
    DataBlock *compBlk = currBlk->addNewBlock(blkName.str()); \
    compBlk->add##Type("value", comp.get<type_name>());       \
    return;                                                   \
  }

  switch (ctype)
  {
    PROC_COMP(Real, float)
    PROC_COMP(Int, int)
    PROC_COMP(E3dcolor, E3DCOLOR)
    PROC_COMP(Bool, bool)
    PROC_COMP(Point2, Point2)
    PROC_COMP(Point3, Point3)
    PROC_COMP(Point4, Point4)
    PROC_COMP(IPoint2, IPoint2)
    PROC_COMP(IPoint3, IPoint3)
    PROC_COMP(Tm, TMatrix)
    PROC_COMP(Int64, int64_t)
    PROC_BLOCK_COMP(u8, Int, uint8_t)
    PROC_BLOCK_COMP(u16, Int, uint16_t)
    PROC_BLOCK_COMP(u32, Int, uint32_t)
    PROC_BLOCK_COMP(u64, Int64, uint64_t)
    case ComponentTypeInfo<ecs::string>::type:
    {
      currBlk->addStr(paramName, comp.get<ecs::string>().c_str());
      return;
    }
    case ComponentTypeInfo<ecs::Object>::type:
    {
      String blkName(0, "%s:object", paramName);
      DataBlock *objBlk = currBlk->addNewBlock(blkName.str());
      for (auto &child : comp.get<ecs::Object>())
        component_to_blk_param(ecs::get_key_string(child.first).data(), child.second.getEntityComponentRef(), objBlk);
      return;
    }
    case ComponentTypeInfo<ecs::Array>::type:
    {
      String blkName(0, "%s:array", paramName);
      DataBlock *vecBlk = currBlk->addNewBlock(blkName.str());
      for (auto &child : comp.get<ecs::Array>())
        component_to_blk_param(paramName, child.getEntityComponentRef(), vecBlk);
      return;
    }
    case ComponentTypeInfo<ecs::EntityId>::type:
    {
      String pName(0, "%s:eid", paramName);
      currBlk->addInt(pName.str(), 0);
      return;
    }
      LIST_TYPES
    case ComponentTypeInfo<ecs::List<EntityId>>::type:
    {
      logerr("skipped saving ecs::List<EntityId> %s", paramName);
      return;
    }
    case ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::Array>>::type:
    {
      const ecs::SharedComponent<ecs::Array> &sharedArr = comp.get<ecs::SharedComponent<ecs::Array>>();
      const ecs::Array *arr = sharedArr.get();

      String blkName(0, "%s:shared:array", paramName);
      DataBlock *vecBlk = currBlk->addNewBlock(blkName.str());
      for (auto &child : *arr)
        component_to_blk_param(paramName, child.getEntityComponentRef(), vecBlk);

      return;
    }
    case ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::Object>>::type:
    {
      const ecs::SharedComponent<ecs::Object> &sharedObj = comp.get<ecs::SharedComponent<ecs::Object>>();
      const ecs::Object *obj = sharedObj.get();

      String blkName(0, "%s:object", paramName);
      DataBlock *objBlk = currBlk->addNewBlock(blkName.str());
      for (auto &child : *obj)
        component_to_blk_param(ecs::get_key_string(child.first).data(), child.second.getEntityComponentRef(), objBlk);

      return;
    }
    case ComponentTypeInfo<ecs::Tag>::type:
    {
      String blkName(0, "%s:tag", paramName);
      currBlk->addBlock(blkName.c_str());
      return;
    }
    default:
      logerr("Unsupported component type %s <%d> for comp %s", g_entity_mgr->getComponentTypes().findTypeName(ctype), ctype,
        paramName);
  }
#undef PROC_COMP
#undef PROC_BLOCK_COMP
}
inline void component_to_blk(const char *name, const ecs::EntityComponentRef &comp, DataBlock &blk)
{
  DataBlock *currBlk = &blk;
  const char *path = name;
  char paramName[64];
  char *ptr = paramName;
  for (;; ++path)
  {
    *ptr = *path;
    if (*path && memcmp(COMP_SEPARATOR, path, strlen(COMP_SEPARATOR)) != 0)
    {
      ++ptr;
      G_ASSERTF_RETURN(ptr < paramName + sizeof(paramName), , "Parameter or block name is too long \"%s\"", name);
      continue;
    }
    *ptr = 0;
    ptr = paramName;
    G_ASSERTF_RETURN(*paramName, , "Parameter or block name is empty \"%s\"", name);
    if (memcmp(COMP_SEPARATOR, path, strlen(COMP_SEPARATOR)) == 0)
      currBlk = currBlk->addBlock(paramName);
    else
      component_to_blk_param(paramName, comp, currBlk);
    if (!*path)
      break;
  }
}
inline void component_to_blk(const char *name, const ecs::ChildComponent &comp, DataBlock &blk)
{
  component_to_blk(name, comp.getEntityComponentRef(), blk);
}
inline void component_to_blk_param(const char *paramName, const ecs::ChildComponent &comp, DataBlock *currBlk)
{
  component_to_blk_param(paramName, comp.getEntityComponentRef(), currBlk);
}

inline const char *get_c_str(const char *a) { return a; }
inline const char *get_c_str(const eastl::string &a) { return a.c_str(); }

template <class Component>
inline bool component_to_blk(Component &comp, DataBlock &blk, const char *prefix = NULL, bool sep_on_dot = true)
{
  const char *name = get_c_str(comp.first);
  size_t prefixLen = prefix ? strlen(prefix) : 0;
  if (prefix && name && strncmp(name, prefix, prefixLen) != 0)
    return false;
  const char *path = name + prefixLen; //-V769

  if (sep_on_dot)
    component_to_blk(path, comp.second, blk);
  else
    component_to_blk_param(path, comp.second, &blk);
  return true;
}

template <class Iterator, class EndIterator>
inline void components_to_blk(Iterator iter, EndIterator end, DataBlock &blk, const char *prefix = NULL, bool sep_on_dot = true)
{
  for (; iter != end; ++iter)
  {
    const auto &comp = *iter;
    component_to_blk(comp, blk, prefix, sep_on_dot);
  }
}

template <class EndIterator>
inline void components_to_blk(ecs::ComponentsIterator iter, EndIterator end, DataBlock &blk, const char *prefix, bool sep_on_dot)
{
  for (; iter != end; ++iter)
  {
    const auto &comp = *iter;
    const auto compData = g_entity_mgr->getDataComponents().getComponentById(comp.second.getComponentId());
    if ((compData.flags & ecs::DataComponent::IS_COPY) != 0)
      continue;
    component_to_blk(comp, blk, prefix, sep_on_dot);
  }
}

inline void blk_to_components(ecs::EntityId eid, const DataBlock &blk, const char *prefix = NULL)
{
  for (int i = 0; i < blk.paramCount(); ++i)
  {
    String paramName(128, "%s%s", prefix, blk.getParamName(i));
#define SET_COMP(Type, TYPE) \
  case DataBlock::TYPE_##TYPE: g_entity_mgr->set(eid, ECS_HASH_SLOW(paramName.str()), blk.get##Type(i)); break
    switch (blk.getParamType(i))
    {
      SET_COMP(Str, STRING);
      SET_COMP(Real, REAL);
      SET_COMP(Bool, BOOL);
      SET_COMP(Point2, POINT2);
      SET_COMP(Point3, POINT3);
      SET_COMP(Point4, POINT4);
      SET_COMP(IPoint2, IPOINT2);
      SET_COMP(IPoint3, IPOINT3);
      SET_COMP(Tm, MATRIX);
      SET_COMP(Int, INT);
      SET_COMP(Int64, INT64);
      SET_COMP(E3dcolor, E3DCOLOR);
      default: break;
    }
#undef SET_COMP
  }
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *subBlk = blk.getBlock(i);
    String subName(128, "%s%s" COMP_SEPARATOR, prefix, subBlk->getBlockName());
    blk_to_components(eid, *subBlk, subName.str());
  }
}

#undef LIST_TYPES
} // namespace ecs
