//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <memory.h>
#include <util/dag_stdint.h>
#include <debug/dag_assert.h>
#include <EASTL/functional.h>
#include <EASTL/optional.h>
#include "componentType.h"

namespace ecs
{

template <typename T>
struct ComponentTypeInfo;
class EntityComponentRef;
struct InstantiatedTemplate;

class ChildComponent
{
public:
  ChildComponent() = default;
  ChildComponent(std::nullptr_t) : ChildComponent() {}
  explicit ChildComponent(const EntityComponentRef &);
  const EntityComponentRef getEntityComponentRef() const; // this is const reference! you should write to it!

  bool operator==(const ChildComponent &a) const;
  bool operator!=(const ChildComponent &a) const { return !(*this == a); }
  bool operator==(const EntityComponentRef &a) const;

  ChildComponent &operator=(ChildComponent &&a);
  ChildComponent &operator=(const ChildComponent &a);
  ChildComponent &operator=(std::nullptr_t)
  {
    G_ASSERT(isNull());
    return *this;
  }
  ChildComponent(ChildComponent &&a);
  ChildComponent(const ChildComponent &a) : ChildComponent() { *this = a; }
  ~ChildComponent() { free(); }

  const char *getStr() const; // for ecs20 compatibility!
  template <typename T>
  bool is() const
  {
    return componentType == ComponentTypeInfo<T>::type;
  }

  template <typename T>
  const T *getNullable() const
  {
    return is<T>() ? (const T *)getTypedData<T>() : nullptr;
  }
  template <typename T>
  T *getNullable()
  {
    return is<T>() ? (T *)getTypedData<T>() : nullptr;
  }

  template <class T, typename U = ECS_MAYBE_VALUE_T(T)>
  U get() const
  {
    G_ASSERTF(is<T>() && !isNull(), "0x%X != 0x%X", +ComponentTypeInfo<T>::type, componentType);
    return *(const T *)getTypedData<T>();
  }
  template <typename T>
  T &getRW()
  {
    G_ASSERTF(is<T>() && !isNull(), "0x%X != 0x%X", +ComponentTypeInfo<T>::type, componentType);
    return *(T *)getTypedData<T>();
  }
  template <typename T>
  T &getRWInternal()
  {
    return getRW<T>();
  } // ecs20 compatibility//todo: remove
  const char *getOr(const char *def) const;
  ChildComponent &operator=(const char *v);
  template <class T, typename U = ECS_MAYBE_VALUE_T(T)>
  U getOr(const T &def) const
  {
    static_assert(!eastl::is_same<T, ecs::string>::value, "Generic \"getOr\" is not strings, use non-generic method for that");
    if (isNull())
      return def;
    G_ASSERT_RETURN(is<T>(), def);
    return *(const T *)getTypedData<T>();
  }
  template <typename T>
  ChildComponent &operator=(const T &v)
  {
    G_ASSERT_RETURN(is<T>() || isNull(), *this);
    if (isNull())
    {
      initTypeIndex(ComponentTypeInfo<T>::type);
      componentTypeSize = ComponentTypeInfo<T>::size;
      if (isAttrBoxedType<T>())
      {
        // value.data = (void*)(new typename eastl::remove_reference<T>::type(v));
        // do it same way, as free in BoxedCreator, as data can be MOVED to it. may be make allocator part of type to remove it?
        value.data = memalloc(sizeof(T), midmem_ptr());
        if (!eastl::is_scalar<T>::value)
          memset(value.data, 0, sizeof(T));
        new (value.data, _NEW_INPLACE) typename eastl::remove_reference<T>::type(v);
      }
      else
      {
        if (!eastl::is_scalar<T>::value)
          memset(value.buffer, 0, sizeof(T));
        new (value.buffer, _NEW_INPLACE) typename eastl::remove_reference<T>::type(v);
      }
    }
    else
      getRW<typename eastl::remove_reference<T>::type>() = v;
    return *this;
  }

  template <typename T, typename... Args>
  void emplace(Args &&...args)
  {
    G_ASSERT_RETURN(isNull(), );
    initTypeIndex(ComponentTypeInfo<T>::type);
    componentTypeSize = ComponentTypeInfo<T>::size;
    if (isAttrBoxedType<T>())
    {
      // value.data = (void*)(new typename eastl::remove_reference<T>::type(v));
      // do it same way, as free in BoxedCreator, as data can be MOVED to it. may be make allocator part of type to remove it?
      value.data = memalloc(sizeof(T), midmem_ptr());
      if (!eastl::is_scalar<T>::value)
        memset(value.data, 0, sizeof(T));
      new (value.data, _NEW_INPLACE) typename eastl::remove_reference<T>::type(eastl::forward<Args>(args)...);
    }
    else
    {
      if (!eastl::is_scalar<T>::value)
        memset(value.buffer, 0, sizeof(T));
      new (value.buffer, _NEW_INPLACE) typename eastl::remove_reference<T>::type(eastl::forward<Args>(args)...);
    }
  }

  template <typename T, typename = eastl::enable_if_t<eastl::is_rvalue_reference<T &&>::value, void>>
  ChildComponent &operator=(T &&v)
  {
    G_ASSERT_RETURN(is<T>() || isNull(), *this);
    if (isNull())
    {
      initTypeIndex(ComponentTypeInfo<T>::type);
      componentTypeSize = ComponentTypeInfo<T>::size;
      if (isAttrBoxedType<T>())
      {
        // value.data = (void*)(new typename eastl::remove_reference<T>::type(eastl::move(v)));
        // do it same way, as free in BoxedCreator, as data can be MOVED to it. may be make allocator part of type to remove it?
        value.data = memalloc(sizeof(T), midmem_ptr());
        if (!eastl::is_scalar<T>::value)
          memset(value.data, 0, sizeof(T));
        new (value.data, _NEW_INPLACE) typename eastl::remove_reference<T>::type(eastl::move(v));
      }
      else
      {
        if (!eastl::is_scalar<T>::value)
          memset(value.buffer, 0, sizeof(T));
        new (value.buffer, _NEW_INPLACE) typename eastl::remove_reference<T>::type(eastl::move(v));
      }
    }
    else
    {
      getRW<typename eastl::remove_reference<T>::type>() = eastl::move(v);
    }
    return *this;
  }

  bool isNull() const { return componentType == 0; }
  component_type_t getUserType() const { return componentType; }
  type_index_t getTypeId() const { return componentTypeIndex; }


  template <typename T>
  ChildComponent(const T &t)
  {
    (*this) = t;
  }
  template <typename T, typename = eastl::enable_if_t<eastl::is_rvalue_reference<T &&>::value, void>>
  ChildComponent(T &&t)
  {
    (*this) = eastl::move(t);
  }

  static const ChildComponent &get_null() { return null_attr; }

  enum class CopyType
  {
    Deep,
    Shallow
  };
  ChildComponent(component_type_t t, const void *raw_data, CopyType cpt = CopyType::Deep) { setRaw(t, raw_data, cpt); }
  uint16_t getSize() const { return componentTypeSize; }
  static bool is_child_comp_boxed_by_size(size_t size) { return size > value_size; }
  ChildComponent(size_t size, type_index_t ti, component_type_t t, const void *raw_data); // raw_data is either pointer to allocated or
                                                                                          // stack memory, depedning on
                                                                                          // is_child_comp_boxed_by_size
protected:
  void setRaw(component_type_t t, const void *raw_data, CopyType cpt = CopyType::Deep); // temporary, for deserialization. To be
                                                                                        // replaced with typemanager
  void free();
  friend EntityManager;
  friend InstantiatedTemplate;
  template <typename T>
  static bool isAttrBoxedType()
  {
    return sizeof(T) > sizeof(value) || ComponentTypeInfo<T>::is_boxed || ComponentTypeInfo<T>::is_non_trivial_move;
  }
  template <typename T>
  const void *getTypedData() const
  {
    return isAttrBoxedType<T>() ? value.data : value.buffer;
  }
  template <typename T>
  void *getTypedData()
  {
    return isAttrBoxedType<T>() ? value.data : value.buffer;
  }
  friend void serialize_child_component(const ChildComponent &comp, class SerializerCb &serializer);

  bool isAttrBoxedBySize() const { return is_child_comp_boxed_by_size(componentTypeSize); }
  const void *getRawData() const { return isAttrBoxedBySize() ? value.data : value.buffer; }
  void *getRawData() { return isAttrBoxedBySize() ? value.data : value.buffer; }

  void reset()
  {
    value.data = 0;
    componentType = 0;
    componentTypeSize = 0;
    componentTypeIndex = INVALID_COMPONENT_TYPE_INDEX;
  }
  void resetBoxedMem()
  {
    if (isAttrBoxedBySize())
      memfree_anywhere(value.data);
    reset();
  }
  void initTypeIndex(component_type_t);
#pragma pack(push, 1)
  union Value
  {
    void *data;
    uint8_t buffer[8];
  };
  Value value; //-V730_NOINIT
  component_type_t componentType = 0;
  type_index_t componentTypeIndex = INVALID_COMPONENT_TYPE_INDEX;
  uint16_t componentTypeSize = 0;
// uint16_t componentTypeFlags = 0;//actually we need just one bit - needCTM
#pragma pack(pop)
  static ChildComponent null_attr; // ReadOnly
public:
  static constexpr size_t value_size = sizeof(Value);
};

inline ChildComponent::ChildComponent(ChildComponent &&a)
{
  memcpy(this, &a, sizeof(*this));
  a.reset();
}

inline ChildComponent &ChildComponent::operator=(ChildComponent &&a)
{
  if (EASTL_UNLIKELY(this == &a))
    return *this;
  free();
#if !_ECS_CODEGEN
  G_STATIC_ASSERT(offsetof(ChildComponent, value) == 0);
#endif
  G_STATIC_ASSERT(sizeof(Value) >= sizeof(uintptr_t)); // boxed components should return rawData to something boxed, i.e. buffer
  memcpy(this, &a, sizeof(ChildComponent));
  a.reset();
  return *this;
}

using MaybeChildComponent = eastl::optional<ChildComponent>;

}; // namespace ecs
