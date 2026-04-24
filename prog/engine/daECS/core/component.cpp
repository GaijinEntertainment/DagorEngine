// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/component.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentType.h>

namespace ecs
{

ChildComponent ChildComponent::null_attr; // ro!

void ChildComponent::initTypeIndex(component_type_t component_type)
{
  componentType = component_type;
  componentTypeIndex = g_entity_mgr->componentTypes.findType(componentType);
}


ChildComponent::ChildComponent(size_t size, type_index_t ti, component_type_t t, const void *raw_data)
{
  G_STATIC_ASSERT(offsetof(ChildComponent, value) == 0);
  componentTypeSize = size;
  componentTypeIndex = ti;
  componentType = t;
  memcpy(&value, raw_data, sizeof(Value));
}

void ChildComponent::setRaw(ecs::EntityManager &mgr, component_type_t component_type, const void *raw_data, CopyType copy_type)
{
  if (DAGOR_UNLIKELY(component_type == 0))
  {
    reset();
    return;
  }
  initTypeIndex(component_type);
  const ComponentType typeInfo = mgr.componentTypes.getTypeInfo(componentTypeIndex);
  componentTypeSize = typeInfo.size;
  if (!isAttrBoxedBySize())
  {
    G_ASSERT(componentTypeSize <= sizeof(Value));
    memcpy(&value, raw_data, componentTypeSize);
  }
  else
  {
    value.data = memalloc(componentTypeSize, tmpmem);
    memcpy(value.data, raw_data, componentTypeSize);
  }
  if ((typeInfo.flags & COMPONENT_TYPE_BOXED) && copy_type == CopyType::Shallow)
    return;
  if (need_constructor(typeInfo.flags)) // we will need copy constructor
  {
    if (ComponentTypeManager *tm = mgr.componentTypes.createTypeManager(componentTypeIndex))
      tm->copy(getRawData(), raw_data, componentTypeIndex);
  }
}

ChildComponent &ChildComponent::operator=(const ChildComponent &a)
{
  if (DAGOR_UNLIKELY(this == &a))
    return *this;
  free();
  setRaw(*g_entity_mgr, a.getUserType(), a.getRawData());
  return *this;
}

void ChildComponent::free()
{
  if (DAGOR_LIKELY(componentType != 0))
  {
    const ComponentType typeInfo = g_entity_mgr->componentTypes.getTypeInfo(componentTypeIndex);
    if (need_constructor(typeInfo.flags))
    {
      ComponentTypeManager *tm = g_entity_mgr->componentTypes.createTypeManager(componentTypeIndex);
      tm->destroy(getRawData());
    }
    resetBoxedMem();
  }
  else
    reset();
}

ChildComponent::ChildComponent(ecs::EntityManager &mgr, const EntityComponentRef &a) { setRaw(mgr, a.getUserType(), a.getRawData()); }

const EntityComponentRef ChildComponent::getEntityComponentRef() const
{
  return EntityComponentRef(const_cast<void *>(getRawData()), getUserType(), getTypeId(), INVALID_COMPONENT_INDEX);
}
bool ChildComponent::operator==(const EntityComponentRef &a) const { return getEntityComponentRef() == a; }
bool ChildComponent::operator==(const ChildComponent &a) const { return getEntityComponentRef() == a.getEntityComponentRef(); }

}; // namespace ecs
