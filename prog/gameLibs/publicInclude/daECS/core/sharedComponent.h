//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/shared_ptr.h>

#include <daECS/core/componentTypes.h>
#include <daECS/core/componentsMap.h>

#define ECS_DECLARE_SHARED_TYPE(klass) ECS_DECLARE_RELOCATABLE_TYPE(ecs::SharedComponent<klass>)

#define ECS_REGISTER_SHARED_TYPE(klass, io) \
  ECS_REGISTER_MANAGED_TYPE(ecs::SharedComponent<klass>, io, ecs::SharedComponentCreator<klass>)

#if _ECS_CODEGEN
#define ECS_SHARED(type) ecs::SharedComponent<type>
#else
#define ECS_SHARED(type) const type &
#endif

// usage:
//  ECS_DECLARE_SHARED_TYPE(ecs::Object);
//  ECS_REGISTER_SHARED_TYPE(ecs::Object, nullptr);
//  then in ES you can use const ecs::SharedComponent<ecs::Object> & or
//  EntityManager::get<ecs::SharedComponent<ecs::Object>>
//  this component is shared between all data of one template, and can be used as replacement to Props
//  as soon as we will support it in IO.
//  usage ECS_SHARED(ecs::Object) props (will be visible as const ecs::Object& props)
//  todo: support in IO (reading from BLK)


namespace ecs
{

template <class T>
struct SharedComponentCreator;

template <class T>
class SharedComponent : public eastl::shared_ptr<T>
{
public:
  operator const typename eastl::shared_ptr<T>::reference_type() const
  {
    auto vptr = base_type::get();
    DAECS_EXT_FAST_ASSERT(vptr != nullptr);
    return *vptr;
  }
  SharedComponent(const T &v) : base_type(eastl::make_shared<T>(v)) {}
  SharedComponent(T &&v) : base_type(eastl::make_shared<T>(v)) {}
  template <typename U, typename = eastl::enable_if_t<eastl::is_rvalue_reference<U &&>::value, void>>
  SharedComponent(U &&v) : base_type(eastl::make_shared<U>(eastl::move(v)))
  {}
  SharedComponent(SharedComponent &&v) : base_type(eastl::move(v)) {}
  SharedComponent &operator=(SharedComponent &&v)
  {
    (base_type &)(*this) = eastl::move(v);
    return *this;
  }

protected:
  friend struct SharedComponentCreator<T>;
  typedef eastl::shared_ptr<T> base_type;
  SharedComponent(const SharedComponent &sharedPtr) : base_type(sharedPtr) {}
  SharedComponent(T *initial) : base_type(initial) {}
  SharedComponent() {}
};

EntityManager &get_entity_mgr();

template <class T>
struct SharedComponentCreator final : public ComponentTypeManager
{
  typedef T component_type;
  typedef SharedComponent<T> shared_type;

  bool copy(void *to, const void *from, ecs::component_index_t index, ecs::EntityId to_eid) const override
  {
    G_ASSERT_RETURN(from && to, false);
    if (*(const shared_type *)from || !to_eid)
      new (to, _NEW_INPLACE) shared_type(*(const shared_type *)from);
    else // first copy
    {
      T *data = (T *)memalloc(sizeof(T), midmem);
      ((shared_type *)from)->reset(data);
      new (to, _NEW_INPLACE) shared_type(*(const shared_type *)from);
      // Note: Make sure that actual comp construction called after ptr creation (as ctor might want to access it)
      construct(data, get_entity_mgr(), to_eid, index);
    }
    return true;
  }
  bool is_equal(const void *to, const void *from) const override
  {
    G_ASSERT_RETURN(from && to, true);
    if (!((shared_type *)to)->get() && !((const shared_type *)from)->get()) // equal
      return true;
    if (!((shared_type *)to)->get() || !((const shared_type *)from)->get()) // not equal
      return false;
    return ConstructInplaceType<T>::is_equal(((shared_type *)to)->get(), ((const shared_type *)from)->get());
  }
  bool assign(void *to, const void *from) const override
  {
    G_ASSERT_RETURN(from && to, false);
    G_ASSERT_RETURN(((shared_type *)to)->get() && ((const shared_type *)from)->get(), false);
    return ConstructInplaceType<T>::assign(((shared_type *)to)->get(), ((const shared_type *)from)->get());
  }
  void create(void *p, EntityManager &mgr, EntityId eid, const ComponentsMap &, ecs::component_index_t index) override
  {
    new (p, _NEW_INPLACE) shared_type;
    if (!eid)
      return;

    T *data = (T *)memalloc(sizeof(T), midmem);
    construct(data, mgr, eid, index);
    ((shared_type *)p)->reset(data);
  }

  void destroy(void *p) override
  {
    G_ASSERT_RETURN(p, );
    ((shared_type *)p)->~shared_type();
    memset(p, 0, sizeof(shared_type));
  } // memset is not needed, we keep it for debug purposes

  bool replicateCompare(void *to, const void *from) const override
  {
    G_ASSERT_RETURN(from && to, false);
    G_ASSERT_RETURN(((shared_type *)to)->get() && ((const shared_type *)from)->get(), false);
    return ConstructInplaceType<T>::replicateCompare(((shared_type *)to)->get(), ((const shared_type *)from)->get());
  }
  void clear() override { ConstructInplaceType<T>::onTypeManagerClear(); }

private:
  void construct(T *data, EntityManager &mgr, EntityId eid, ecs::component_index_t index) const
  {
    ConstructInplaceType<T>::create(data, mgr, eid, ComponentsMap(), index);
  }
};
} // namespace ecs

// Digraphs-frienly aliases
ECS_DECLARE_RELOCATABLE_TYPE_ALIAS(ecs::SharedComponent<ecs::Object>, "ecs::SharedComponent< ::ecs::Object>");
ECS_DECLARE_RELOCATABLE_TYPE_ALIAS(ecs::SharedComponent<ecs::Array>, "ecs::SharedComponent< ::ecs::Array>");
ECS_DECLARE_RELOCATABLE_TYPE_ALIAS(ecs::SharedComponent<ecs::string>, "ecs::SharedComponent< ::ecs::string>");

#define DECL_LIST_TYPE(lt, t) \
  ECS_DECLARE_RELOCATABLE_TYPE_ALIAS(ecs::SharedComponent<ecs::lt>, "ecs::SharedComponent< ::ecs::" #lt ">");
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
