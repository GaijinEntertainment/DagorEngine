#pragma once
#include <EASTL/functional.h>
#include <EASTL/type_traits.h>

namespace ecs
{

class ResourceRequestCb;
typedef ResourceRequestCb resource_request_cb_t;

class EntityManager;
class ComponentsMap;
class EntityId;

template <bool cond, int v, int d = 0>
using select_int = eastl::conditional_t<cond, eastl::integral_constant<int, v>, eastl::integral_constant<int, d>>;

template <typename T>
struct OnComponentLoaded
{
  template <typename U = T>
  static auto SFINAE3(U *u) -> decltype(u->onLoaded(), eastl::yes_type{});
  template <typename U = T>
  static eastl::no_type SFINAE3(...);

  template <typename U = T>
  static auto SFINAE2(U *u) -> decltype(u->onLoaded(eastl::declval<EntityManager &>(), eastl::declval<const EntityId &>()),
                              eastl::yes_type{});
  template <typename U = T>
  static eastl::no_type SFINAE2(...);

  // Check what size SFINAE ended up being, this tells us if the constructor matched the right signature or not
  static constexpr bool hasSimpleOnLoaded = sizeof(SFINAE3<T>(nullptr)) == sizeof(eastl::yes_type);
  static constexpr bool hasOnLoaded = !hasSimpleOnLoaded && (sizeof(SFINAE2<T>(nullptr)) == sizeof(eastl::yes_type));
  static constexpr bool noOnLoaded = !hasSimpleOnLoaded && !hasOnLoaded;
};

template <typename T>
struct CtorFlavorSelector
{
  static constexpr int value =
    eastl::disjunction<select_int<eastl::is_constructible<T, EntityManager &, EntityId, component_index_t>::value, 4>,
      select_int<eastl::is_constructible<T, EntityManager &, EntityId, const ComponentsMap &>::value, 3>,
      select_int<eastl::is_constructible<T, EntityManager &, EntityId>::value, 2>,
      select_int<eastl::is_constructible<T, const ComponentsMap &>::value, 1>>::value;
};

template <typename T>
struct TypeCopyAssignable
{
  static constexpr bool value = eastl::is_copy_assignable<T>::value;
};
template <typename T>
struct TypeCopyConstructible
{
  static constexpr bool value = eastl::is_copy_constructible<T>::value;
};
template <typename T>
struct TypeComparable
{
  static constexpr bool value = eastl::has_equality<T>::value;
};
namespace details
{
template <typename T>
struct Replicating
{
  template <typename U = T>
  static eastl::yes_type SFINAE3(decltype(&U::replicateCompare));
  template <typename U = T>
  static eastl::no_type SFINAE3(...);

  static constexpr bool use_replicate = sizeof(SFINAE3<T>(nullptr)) == sizeof(eastl::yes_type);
  static constexpr bool use_compare_and_assign = TypeComparable<T>::value && TypeCopyAssignable<T>::value && !use_replicate;
};
}; // namespace details

template <typename T>
struct TypeReplicatable
{
  static constexpr bool value = details::Replicating<T>::use_replicate || details::Replicating<T>::use_compare_and_assign;
};

template <typename T>
class HasRequestResources
{
  // Note: intentionally without exact signature match in order catch incompatible declarations of 'requestResources' in compile-time
  template <typename U>
  static eastl::yes_type SFINAE(decltype(&U::requestResources));
  template <typename U>
  static eastl::no_type SFINAE(...);

public:
  static constexpr bool value = sizeof(SFINAE<T>(nullptr)) == sizeof(eastl::yes_type);
};

template <typename T>
class HasOnTPMClear
{
  template <typename U>
  static eastl::yes_type SFINAE(decltype(&U::onTypeManagerClear));
  template <typename U>
  static eastl::no_type SFINAE(...);

public:
  static constexpr bool value = sizeof(SFINAE<T>(nullptr)) == sizeof(eastl::yes_type);
};

template <typename T>
class ConstructInplaceType
{
  template <typename U = T>
  static eastl::enable_if_t<HasRequestResources<U>::value> requestResourcesImpl(const char *nm, const resource_request_cb_t &res_cb)
  {
    U::requestResources(nm, res_cb);
  }
  template <typename U = T>
  static eastl::disable_if_t<HasRequestResources<U>::value> requestResourcesImpl(const char *, const resource_request_cb_t &)
  {} // noop

  template <class U = T>
  static eastl::enable_if_t<CtorFlavorSelector<U>::value == 4, U *> constructCandidate(void *p, EntityManager &mgr, EntityId eid,
    const ComponentsMap &, ecs::component_index_t index)
  {
    return new (p, _NEW_INPLACE) U(mgr, eid, index);
  }
  template <class U = T>
  static eastl::enable_if_t<CtorFlavorSelector<U>::value == 3, U *> constructCandidate(void *p, EntityManager &mgr, EntityId eid,
    const ComponentsMap &map, ecs::component_index_t)
  {
    return new (p, _NEW_INPLACE) U(mgr, eid, map);
  }
  template <class U = T>
  static eastl::enable_if_t<CtorFlavorSelector<U>::value == 2, U *> constructCandidate(void *p, EntityManager &mgr, EntityId eid,
    const ComponentsMap &, ecs::component_index_t)
  {
    return new (p, _NEW_INPLACE) U(mgr, eid);
  }
  template <class U = T>
  static eastl::enable_if_t<CtorFlavorSelector<U>::value == 1, U *> constructCandidate(void *p, EntityManager &, EntityId,
    const ComponentsMap &map, ecs::component_index_t)
  {
    return new (p, _NEW_INPLACE) U(map);
  }
  template <class U = T>
  static eastl::enable_if_t<CtorFlavorSelector<U>::value == 0, U *> constructCandidate(void *p, EntityManager &, EntityId,
    const ComponentsMap &, ecs::component_index_t)
  {
    return new (p, _NEW_INPLACE) U;
  } // Note: intentionally not zero init

public:
  template <class U = T>
  static typename eastl::enable_if<OnComponentLoaded<U>::hasSimpleOnLoaded, bool>::type onLoadedInternal(T *a, EntityManager &,
    EntityId)
  {
    return a->onLoaded();
  }
  template <class U = T>
  static typename eastl::enable_if<OnComponentLoaded<U>::hasOnLoaded, bool>::type onLoadedInternal(T *a, EntityManager &mgr,
    EntityId eid)
  {
    return a->onLoaded(mgr, eid);
  }
  template <class U = T>
  static typename eastl::enable_if<OnComponentLoaded<U>::noOnLoaded, bool>::type onLoadedInternal(T *, EntityManager &, EntityId)
  {
    return true;
  }

  static void requestResources(const char *compname, const resource_request_cb_t &res_cb)
  {
    requestResourcesImpl<T>(compname, res_cb);
  }

  static void create(void *p, EntityManager &mgr, EntityId eid, const ComponentsMap &map, ecs::component_index_t index)
  {
    constructCandidate<T>(p, mgr, eid, map, index);
    onLoadedInternal((T *)p, mgr, eid);
  }

  template <class U = T>
  static typename eastl::enable_if<HasOnTPMClear<U>::value>::type onTypeManagerClear()
  {
    U::onTypeManagerClear();
  }
  template <class U = T>
  static typename eastl::disable_if<HasOnTPMClear<U>::value>::type onTypeManagerClear()
  {}

  template <class U = T>
  static typename eastl::enable_if<TypeCopyConstructible<U>::value, bool>::type copy(void *p, const U *from)
  {
    new (p, _NEW_INPLACE) U(*from);
    return true;
  }

  template <class U = T>
  static typename eastl::enable_if<!TypeCopyConstructible<U>::value, bool>::type copy(void *, const U *)
  {
    return false;
  }

  template <class U = T>
  static typename eastl::enable_if<TypeComparable<U>::value, bool>::type is_equal(const U *a, const U *b)
  {
    return (*a) == (*b);
  }
  template <class U = T>
  static typename eastl::enable_if<!TypeComparable<U>::value, bool>::type is_equal(const U *, const U *)
  {
    return true;
  }

  template <class U = T>
  static typename eastl::enable_if<TypeCopyAssignable<U>::value, bool>::type assign(U *to, const U *from)
  {
    (*to) = (*from);
    return true;
  }
  template <class U = T>
  static typename eastl::enable_if<!TypeCopyAssignable<U>::value && TypeCopyConstructible<U>::value, bool>::type assign(U *to,
    const U *from)
  {
    to->~U();
    return copy(to, from);
  }
  template <class U = T>
  static typename eastl::enable_if<!TypeCopyAssignable<U>::value && !TypeCopyConstructible<U>::value, bool>::type assign(U *,
    const U *)
  {
    return false;
  }

  template <class U = T>
  static typename eastl::enable_if<TypeReplicatable<U>::value && details::Replicating<U>::use_replicate, bool>::type replicateCompare(
    U *to, const U *from)
  {
    return to->replicateCompare(*from);
  }
  template <class U = T>
  static typename eastl::enable_if<TypeReplicatable<U>::value && !details::Replicating<U>::use_replicate, bool>::type replicateCompare(
    U *to, const U *from)
  {
    if ((*to) == *(from))
      return false;
    *to = *from;
    return true;
  }
  template <class U = T>
  static typename eastl::enable_if<!TypeReplicatable<U>::value, bool>::type replicateCompare(U *, const U *)
  {
    return false;
  }
};

}; // namespace ecs
