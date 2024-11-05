//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <memory.h>
#include <util/dag_stdint.h>
#include <debug/dag_assert.h>
#include <EASTL/functional.h>
#include "componentType.h"

namespace ecs
{
// EntityComponentRef is not 'owning' it's data. It is reference to data (or null)
// it is not const, i.e. data can be changed

class EntityComponentRef
{
public:
  bool operator==(const EntityComponentRef &a) const;
  template <typename T>
  bool is() const
  {
    return componentType == ComponentTypeInfo<T>::type;
  }
  template <typename T>
  const T *getNullable() const
  {
    return is<T>() ? &PtrComponentType<T>::cref(getRawData()) : nullptr;
  }

  // Note: intentionally absent getNullableRW since it's mutation won't registered as tracked change

  template <typename T>
  const T &get() const
  {
    G_ASSERTF(is<T>(), "0x%X == componentType, 0x%X", componentType, +ComponentTypeInfo<T>::type);
    return PtrComponentType<T>::cref(getRawData());
  }
  template <typename T>
  T &get()
  {
    G_ASSERT(is<T>());
    return PtrComponentType<T>::ref(getRawData());
  }
  template <typename T>
  T &getRW()
  {
    G_ASSERT(is<T>());
    return PtrComponentType<T>::ref(getRawData());
  }
  template <typename T>
  T &getRWInternal()
  {
    return getRW<T>();
  } // ecs20 compatibility//todo: remove
  template <typename T>
  const T &getOr(const T &v) const
  {
    if (isNull())
      return v;
    G_ASSERT_RETURN(is<T>(), v);
    return PtrComponentType<T>::cref(getRawData());
  }
  template <typename T>
  EntityComponentRef &operator=(const T &v)
  {
    G_ASSERT_RETURN(is<T>(), *this);
    PtrComponentType<typename eastl::remove_const<typename eastl::remove_reference<T>::type>::type>::ref(getRawData()) = v;
    return *this;
  }

  template <typename T, typename = eastl::enable_if_t<eastl::is_rvalue_reference<T &&>::value, void>>
  EntityComponentRef &operator=(T &&v)
  {
    G_ASSERT_RETURN(is<T>(), *this);
    PtrComponentType<typename eastl::remove_const<typename eastl::remove_reference<T>::type>::type>::ref(getRawData()) =
      eastl::move(v);
    return *this;
  }
  bool isNull() const { return componentType == 0; }
  component_type_t getUserType() const { return componentType; }
  type_index_t getTypeId() const { return componentTypeIndex; }
  component_index_t getComponentId() const { return componentId; }
  const void *getRawData() const { return data; }
  void *getRawData() { return data; }

protected:
  EntityComponentRef() = default;
  EntityComponentRef(void *d, component_type_t tp, type_index_t tp_id, component_index_t id) :
    data(d), componentType(tp), componentId(id), componentTypeIndex(tp_id)
  {}
  friend class SqBindingHelper;
  friend class EntityManager;
  friend class ChildComponent;
  friend struct InstantiatedTemplate;
  template <typename>
  friend class List;
  void reset()
  {
    data = 0;
    componentType = 0;
    componentId = INVALID_COMPONENT_INDEX;
    componentTypeIndex = INVALID_COMPONENT_TYPE_INDEX;
  }
  void *data = nullptr;
  component_type_t componentType = 0;
  type_index_t componentTypeIndex = INVALID_COMPONENT_TYPE_INDEX;
  component_index_t componentId = INVALID_COMPONENT_INDEX;
  // size?
};

// To compile template (operator =) when T == EntityComponentRef
// EntityComponentRef & operator =(EntityComponentRef&& v)
template <>
struct ComponentTypeInfo<EntityComponentRef>
{
  static constexpr bool is_boxed = false;
  static constexpr size_t ref_alignment =
    ecs_data_alignment(sizeof(eastl::type_select<is_defined<EntityComponentRef>, EntityComponentRef, char>::type));
  ;
};

template <>
struct ComponentTypeInfo<const EntityComponentRef>
{
  static constexpr component_type_t type = ECS_HASH("ecs::EntityComponentRef").hash;
};

}; // namespace ecs
