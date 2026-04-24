// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/baseIo.h>
#include <daECS/core/entityManager.h>
#include <EASTL/optional.h>

namespace ecs
{

bool can_serialize_type(ecs::type_index_t typeId, const ecs::EntityManager &mgr)
{
  ecs::ComponentType componentTypeInfo = mgr.getComponentTypes().getTypeInfo(typeId);
  // todo: check for component serializer
  return ecs::is_pod(componentTypeInfo.flags) || (componentTypeInfo.flags & COMPONENT_TYPE_HAS_IO);
}

bool can_serialize(const ecs::EntityComponentRef &comp, const ecs::EntityManager &mgr)
{
  return can_serialize_type(comp.getTypeId(), mgr);
}

bool should_replicate(const ecs::EntityComponentRef &comp, const ecs::EntityManager &mgr)
{
  if (!can_serialize(comp, mgr) || comp.getUserType() == ComponentTypeInfo<ecs::Tag>::type)
    return false;
  DataComponent component = mgr.getDataComponents().getComponentById(comp.getComponentId());
  return !(component.flags & (DataComponent::DONT_REPLICATE | DataComponent::IS_COPY));
}

bool should_replicate(component_index_t cidx, const ecs::EntityManager &mgr)
{
  DataComponent component = mgr.getDataComponents().getComponentById(cidx);
  if (!can_serialize_type(component.componentType, mgr) || component.componentTypeName == ComponentTypeInfo<ecs::Tag>::type)
    return false;
  return !(component.flags & (DataComponent::DONT_REPLICATE | DataComponent::IS_COPY));
}

void write_string(ecs::SerializerCb &cb, const char *pStr, uint32_t max_string_len)
{
  for (const char *str = pStr; *str && str - pStr < max_string_len; str++)
    cb.write(str, sizeof(str[0]) * CHAR_BIT, 0);
  const char c = 0;
  cb.write(&c, sizeof(c) * CHAR_BIT, 0);
}

int read_string(const ecs::DeserializerCb &cb, char *buf, uint32_t buf_size)
{
  buf_size--;
  char *str = buf;
  do
  {
    if (!cb.read(str, 8, 0))
      return -1;
    if (str == buf + buf_size)
      *str = 0;
  } while (*(str++));
  return str - buf;
}

void serialize_entity_component_ref_typeless(const void *comp_data, component_index_t cidx, component_type_t type_name,
  type_index_t type_id, SerializerCb &serializer, const ecs::EntityManager &mgr)
{
  const auto &componentTypes = mgr.getComponentTypes();
  ComponentSerializer *typeIO = nullptr;
  if (cidx != ecs::INVALID_COMPONENT_INDEX)
    typeIO = mgr.getDataComponents().getComponentIO(cidx);
  ComponentType componentTypeInfo = componentTypes.getTypeInfo(type_id);

  const bool isPod = is_pod(componentTypeInfo.flags);
  if (!typeIO && has_io(componentTypeInfo.flags))
    typeIO = componentTypes.getTypeIO(type_id);
  G_ASSERT(typeIO || isPod);
  if (!typeIO && isPod) // pod data can be just readed as-is
  {
    serializer.write(comp_data, componentTypeInfo.size * 8, type_name);
  }
  else if (typeIO)
  {
    const bool isBoxed = (componentTypeInfo.flags & COMPONENT_TYPE_BOXED) != 0;
    typeIO->serialize(serializer, isBoxed ? *(void **)comp_data : comp_data, componentTypeInfo.size, type_name);
  }
}

void serialize_entity_component_ref_typeless(const void *comp_data, component_type_t type_name, SerializerCb &serializer,
  const ecs::EntityManager &mgr)
{
  serialize_entity_component_ref_typeless(comp_data, ecs::INVALID_COMPONENT_INDEX, type_name,
    mgr.getComponentTypes().findType(type_name), serializer, mgr);
}

void serialize_entity_component_ref_typeless(const EntityComponentRef &comp, SerializerCb &serializer, const ecs::EntityManager &mgr)
{
  G_ASSERT(comp.getComponentId() != INVALID_COMPONENT_INDEX);
  serialize_entity_component_ref_typeless(comp.getRawData(), comp.getComponentId(), comp.getUserType(), comp.getTypeId(), serializer,
    mgr);
}

bool deserialize_component_typeless(EntityComponentRef &comp, const DeserializerCb &deserializer, const ecs::EntityManager &mgr)
{
  auto &componentTypes = mgr.getComponentTypes();
  ecs::component_index_t cidx = comp.getComponentId();
  type_index_t typeId = comp.getTypeId();
  component_type_t userType = comp.getUserType();
  ComponentSerializer *typeIO = nullptr;
  if (cidx != ecs::INVALID_COMPONENT_INDEX)
    typeIO = mgr.getDataComponents().getComponentIO(cidx);
  ComponentType componentTypeInfo = componentTypes.getTypeInfo(typeId);
  if (!typeIO && has_io(componentTypeInfo.flags))
    typeIO = componentTypes.getTypeIO(typeId);
  mgr.trackComponentIndex(comp.getComponentId(), EntityManager::TrackComponentOp::WRITE, "deserialize");
  if (typeIO)
  {
    const bool isBoxed = (componentTypeInfo.flags & COMPONENT_TYPE_BOXED) != 0;
    return typeIO->deserialize(deserializer, isBoxed ? *(void **)comp.getRawData() : comp.getRawData(), componentTypeInfo.size,
      userType);
  }
  else if (is_pod(componentTypeInfo.flags))
    return deserializer.read(comp.getRawData(), componentTypeInfo.size * CHAR_BIT, userType);
  else
  {
    logerr("Attempt to deserialize type 0x%X %d<%s>, which has no typeIO and not pod", userType, typeId,
      componentTypes.getTypeNameById(typeId));
    return false;
  }
}

bool deserialize_component_typeless(void *raw_data, component_type_t type_name, const DeserializerCb &deserializer,
  const ecs::EntityManager &mgr)
{
  const auto &componentTypes = mgr.getComponentTypes();
  type_index_t typeId = componentTypes.findType(type_name);
  ComponentSerializer *typeIO = nullptr;
  ComponentType componentTypeInfo = componentTypes.getTypeInfo(typeId);
  if (has_io(componentTypeInfo.flags))
    typeIO = componentTypes.getTypeIO(typeId);
  if (typeIO)
  {
    const bool isBoxed = (componentTypeInfo.flags & COMPONENT_TYPE_BOXED) != 0;
    return typeIO->deserialize(deserializer, isBoxed ? *(void **)raw_data : raw_data, componentTypeInfo.size, type_name);
  }
  else if (is_pod(componentTypeInfo.flags))
    return deserializer.read(raw_data, componentTypeInfo.size * CHAR_BIT, type_name);
  else
  {
    logerr("Attempt to deserialize type 0x%X %d<%s>, which has no typeIO and not pod", type_name, typeId,
      componentTypes.getTypeNameById(typeId));
    return false;
  }
}

// in network deserializing of entity component we should not use this codepath
// instead of always deserializing to list of components, we should directly deserialize components to already created components
// ChildComponent should be only for case of initial/template serialization (or object types, obviously)
// if replication packet arrive before Entity is created, just store it is _packet_, no need to deserialize it
// current version always make a lot of copies/allocations for no reason
eastl::optional<ChildComponent> deserialize_init_component_typeless(component_type_t userType, ecs::component_index_t cidx,
  const DeserializerCb &deserializer, ecs::EntityManager &mgr)
{
  if (userType == 0)
    return ChildComponent(); // Assume that null type is not an error
  const auto &componentTypes = mgr.getComponentTypes();
  ComponentSerializer *typeIO = nullptr;
  type_index_t typeId;
  if (cidx != ecs::INVALID_COMPONENT_INDEX)
  {
    const auto &components = mgr.getDataComponents();
    DataComponent dataComp = components.getComponentById(cidx);
    typeId = dataComp.componentType;
    DAECS_EXT_ASSERT(dataComp.componentTypeName == userType);
    if (dataComp.flags & DataComponent::HAS_SERIALIZER)
    {
      typeIO = components.getComponentIO(cidx);
      DAECS_EXT_FAST_ASSERT(typeIO != nullptr);
    }
  }
  else
  {
    typeId = componentTypes.findType(userType); // hash lookup
    if (DAGOR_UNLIKELY(typeId == INVALID_COMPONENT_TYPE_INDEX))
    {
      logerr("Attempt to deserialize component of invalid/unknown type 0x%X", userType);
      return MaybeChildComponent(); // We can't read unknown type of unknown size
    }
  }
  ComponentType componentTypeInfo = componentTypes.getTypeInfo(typeId);
  if (has_io(componentTypeInfo.flags) && !typeIO)
    typeIO = mgr.getComponentTypes().getTypeIO(typeId);
  mgr.trackComponentIndex(cidx, EntityManager::TrackComponentOp::WRITE, "deserialize_init");
  const bool isPod = is_pod(componentTypeInfo.flags);
  const bool allocatedMem = ChildComponent::is_child_comp_boxed_by_size(componentTypeInfo.size);
  alignas(void *) char compData[ChildComponent::value_size]; // actually, should be sizeof(ChildComponent::Value), but it is protected
  void *tempData;
  if (allocatedMem)
    *(void **)compData = tempData = memalloc(componentTypeInfo.size, tmpmem);
  else
    tempData = compData;
  if (typeIO)
  {
    const bool isBoxed = (componentTypeInfo.flags & COMPONENT_TYPE_BOXED) != 0;
    ComponentTypeManager *ctm = NULL;
    if (need_constructor(componentTypeInfo.flags))
    {
      // yes, this const cast is ugly. It yet has to be here, there is no other way (besides explicit method in entity manager of
      // course).
      ctm = const_cast<ComponentTypes &>(componentTypes).createTypeManager(typeId);
      G_ASSERTF(ctm, "type manager for type 0x%X (%d) missing", userType, typeId);
    }
    if (ctm)
      ctm->create(tempData, mgr, INVALID_ENTITY_ID, ComponentsMap(), cidx);
    else if (!isPod)
      memset(tempData, 0, componentTypeInfo.size);
    if (typeIO->deserialize(deserializer, isBoxed ? *(void **)tempData : tempData, componentTypeInfo.size, userType))
      return ChildComponent(componentTypeInfo.size, typeId, userType, compData);
  }
  else if (isPod) // pod data can be just readed as-is
  {
    if (deserializer.read(tempData, componentTypeInfo.size * CHAR_BIT, userType))
      return ChildComponent(componentTypeInfo.size, typeId, userType, compData);
  }
  else
    logerr("Attempt to deserialize type 0x%X %d<%s>, which has no typeIO and not pod", userType, typeId,
      componentTypes.getTypeNameById(typeId));
  if (allocatedMem)
    memfree_anywhere(tempData);
  return MaybeChildComponent();
}

void serialize_child_component(const ChildComponent &comp, SerializerCb &serializer, ecs::EntityManager &mgr)
{
  component_type_t userType = comp.getUserType();
  serializer.write(&userType, sizeof(userType) * CHAR_BIT, 0);
  serialize_entity_component_ref_typeless(comp.getRawData(), ecs::INVALID_COMPONENT_INDEX, comp.getUserType(), comp.getTypeId(),
    serializer, mgr);
}

MaybeChildComponent deserialize_child_component(const DeserializerCb &deserializer, ecs::EntityManager &mgr)
{
  component_type_t userType = 0;
  if (deserializer.read(&userType, sizeof(userType) * CHAR_BIT, 0))
    return deserialize_init_component_typeless(userType, ecs::INVALID_COMPONENT_INDEX, deserializer, mgr);
  else
    return MaybeChildComponent();
}

} // namespace ecs
