//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_compilerDefs.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <debug/dag_assert.h>
#include <osApiWrappers/dag_spinlock.h>
#include <generic/dag_initOnDemand.h>
#include <util/dag_stdint.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <daECS/core/entityId.h>
#include <daECS/core/ecsHash.h>
#include <util/dag_hashedKeyMap.h>
#include <EASTL/bonus/tuple_vector.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_declAlign.h>
#include "internal/typesAndLimits.h"
#include "internal/componentTypeBoxedConstructor.h"

#define CONCATENATE(x, y) x##y
#define MAKE_UNIQUE(x, y) CONCATENATE(x, y)
#define ECS_COMMA         ,

#if DAGOR_DBGLEVEL > 0
#define ECS_VERBOSE_LOG debug
#else
#define ECS_VERBOSE_LOG(...)
#endif

// Components of size > than this are allocated as boxed (on heap) despite being declared as relocatable
#ifdef DAGOR_PREFER_HEAP_ALLOCATION
#define ECS_BOXED_THRESHOLD_SIZE 128 // Use more heap to catch memory related errors by Adress Sanitizer
#else
#define ECS_BOXED_THRESHOLD_SIZE 4095 // Assume that objects with sizeof > than this are too expesive to copy (e.g. on re-creates)
#endif

#if DAGOR_DBGLEVEL > 0 && _DEBUG_TAB_ && !DAGOR_ADDRESS_SANITIZER
#define ECS_DEBUG_FILL_MEM_WITH_PATTERN(p, sz, ptrn)           \
  for (int *i = (int *)(p), *e = i + sz / sizeof(int); i < e;) \
  *i++ = ptrn
#else
#define ECS_DEBUG_FILL_MEM_WITH_PATTERN(p, sz, ptrn) ((void)0)
#endif

#define ECS_PULL_VAR(x)      ecs_pull_##x
#define ECS_DECL_PULL_VAR(x) extern const size_t ecs_pull_##x
#define ECS_DEF_PULL_VAR(x)  extern const size_t ecs_pull_##x = (size_t)(&ecs_pull_##x)

namespace ecs
{
template <typename T>
struct ComponentTypeInfo;
typedef std::nullptr_t auto_type; // this is to be used with REQUIRE/REQUIRE_NOT only
template <>
struct ComponentTypeInfo<auto_type>
{
  static constexpr component_type_t type = ECS_HASH("ecs::auto_type").hash;
};
inline constexpr size_t get_data_alignment(size_t sz, size_t current_align)
{
  return (sz % current_align == 0) ? current_align : get_data_alignment(sz, current_align >> 1);
}

inline constexpr size_t ecs_data_alignment(size_t sz) { return get_data_alignment(sz, max_alignment); }
template <typename T, typename = void>
constexpr bool is_defined = false;

template <typename T>
constexpr bool is_defined<T, decltype(sizeof(T), void())> = true;
template <class T>
struct IsCreatedOnTemplInstantiate
{
  static constexpr bool create_on_templ_instantiate = false;
};
template <class T>
class SharedComponent;
template <class T>
struct IsCreatedOnTemplInstantiate<SharedComponent<T>>
{
  static constexpr bool create_on_templ_instantiate = true;
};

class ResourceRequestCb;
typedef ResourceRequestCb resource_request_cb_t;
}; // namespace ecs

#define ECS_DECLARE_BASE_TYPE(class_type, class_name, boxed, non_trivial_move, no_creator_class)                          \
  namespace ecs                                                                                                           \
  {                                                                                                                       \
  template <>                                                                                                             \
  struct ComponentTypeInfo<class_type>                                                                                    \
  {                                                                                                                       \
    static constexpr const char *type_name = class_name;                                                                  \
    static constexpr component_type_t type = ECS_HASH(class_name).hash;                                                   \
    static constexpr bool is_pod_class = no_creator_class;                                                                \
    static constexpr bool is_boxed = boxed;                                                                               \
    static constexpr bool is_non_trivial_move = non_trivial_move;                                                         \
    static constexpr bool is_create_on_templ_inst = IsCreatedOnTemplInstantiate<class_type>::create_on_templ_instantiate; \
    static constexpr size_t size = sizeof(eastl::conditional<is_boxed, class_type *, class_type>::type);                  \
    static constexpr size_t ref_alignment =                                                                               \
      ecs_data_alignment(sizeof(eastl::conditional<is_defined<class_type>, class_type, char>::type));                     \
    static constexpr size_t ptr_alignment = boxed ? sizeof(void *) : ref_alignment;                                       \
    static constexpr bool can_be_tracked =                                                                                \
      is_defined<class_type>                                                                                              \
        ? (is_pod_class || TypeReplicatable<eastl::conditional<is_defined<class_type>, class_type, char>::type>::value)   \
        : true;                                                                                                           \
  };                                                                                                                      \
  template <>                                                                                                             \
  struct ComponentTypeInfo<const class_type> : ComponentTypeInfo<class_type>                                              \
  {};                                                                                                                     \
  template <>                                                                                                             \
  struct ComponentTypeInfo<class_type &> : ComponentTypeInfo<class_type>                                                  \
  {};                                                                                                                     \
  template <>                                                                                                             \
  struct ComponentTypeInfo<const class_type &> : ComponentTypeInfo<class_type>                                            \
  {};                                                                                                                     \
  }

#define ECS_DECLARE_TYPE_ALIAS(class_type, class_alias)                                          \
  ECS_DECLARE_BASE_TYPE(class_type, #class_alias, sizeof(class_type) > ECS_BOXED_THRESHOLD_SIZE, \
    !eastl::is_trivially_move_assignable<class_type>::value, eastl::is_trivially_move_assignable<class_type>::value);

#define ECS_DECLARE_TYPE(class_type)                                                            \
  ECS_DECLARE_BASE_TYPE(class_type, #class_type, sizeof(class_type) > ECS_BOXED_THRESHOLD_SIZE, \
    !eastl::is_trivially_move_assignable<class_type>::value, eastl::is_trivially_move_assignable<class_type>::value);

#define ECS_DECLARE_EXISTING_TYPE_ALIAS_EXISTING(class_alias, current_class)                                     \
  ECS_DECLARE_BASE_TYPE(class_alias, (ecs::ComponentTypeInfo<current_class>::type_name),                         \
    ecs::ComponentTypeInfo<current_class>::is_boxed, ecs::ComponentTypeInfo<current_class>::is_non_trivial_move, \
    ecs::ComponentTypeInfo<current_class>::is_pod_class);                                                        \
  G_STATIC_ASSERT(ecs::ComponentTypeInfo<current_class>::size == ecs::ComponentTypeInfo<class_alias>::size);

// if you sure you can move your data with memcpy
#define ECS_DECLARE_RELOCATABLE_TYPE_ALIAS(class_type, class_alias_str) \
  ECS_DECLARE_BASE_TYPE(class_type, class_alias_str, sizeof(class_type) > ECS_BOXED_THRESHOLD_SIZE, false, false);
#define ECS_DECLARE_RELOCATABLE_TYPE(class_type) \
  ECS_DECLARE_BASE_TYPE(class_type, #class_type, sizeof(class_type) > ECS_BOXED_THRESHOLD_SIZE, false, false);

#define ECS_DECLARE_BOXED_TYPE(class_type) ECS_DECLARE_BASE_TYPE(class_type, #class_type, true, false, false);

#define ECS_DECLARE_BOXED_TYPE_ALIAS(class_type, class_alias) ECS_DECLARE_BASE_TYPE(class_type, #class_alias, true, false, false);

// todo: add pull variable
#define ECS_REGISTER_TYPE_BASE(klass, klass_name, io, creator, destroyer, flags_)                                                   \
  static ecs::CompileComponentTypeRegister MAKE_UNIQUE(ecs_type_rec_, __COUNTER__)(klass_name, ecs::ComponentTypeInfo<klass>::type, \
    ecs::ComponentTypeInfo<klass>::size, io, creator, destroyer,                                                                    \
    ecs::ComponentTypeFlags(                                                                                                        \
      ((ecs::ComponentTypeInfo<klass>::is_non_trivial_move ? ecs::COMPONENT_TYPE_NON_TRIVIAL_MOVE : 0) |                            \
        (ecs::ComponentTypeInfo<klass>::is_pod_class ? ecs::COMPONENT_TYPE_IS_POD : 0) |                                            \
        (ecs::ComponentTypeInfo<klass>::is_create_on_templ_inst ? ecs::COMPONENT_TYPE_CREATE_ON_TEMPL_INSTANTIATE : 0) |            \
        (ecs::ComponentTypeInfo<klass>::is_boxed ? ecs::COMPONENT_TYPE_BOXED : 0) | (flags_))));

#define ECS_REGISTER_MANAGED_TYPE(klass, io, klass_manager)                                                              \
  ECS_REGISTER_TYPE_BASE(klass, ecs::ComponentTypeInfo<klass>::type_name, io, (&ecs::CTMFactory<klass_manager>::create), \
    (&ecs::CTMFactory<klass_manager>::destroy),                                                                          \
    ((ecs::HasRequestResources<klass_manager::component_type>::value) ? ecs::COMPONENT_TYPE_NEED_RESOURCES : 0))

#define ECS_REGISTER_BOXED_TYPE(kl, io) ECS_REGISTER_MANAGED_TYPE(kl, io, typename ecs::CreatorSelector<kl>::type)
#define ECS_REGISTER_RELOCATABLE_TYPE   ECS_REGISTER_BOXED_TYPE

// for POD types
#define ECS_REGISTER_TYPE(klass, io)                          \
  G_STATIC_ASSERT(sizeof(klass) <= ECS_BOXED_THRESHOLD_SIZE); \
  ECS_REGISTER_TYPE_BASE(klass, #klass, io, nullptr, nullptr, 0)

namespace ecs
{

typedef eastl::string string;

// special type = Tag. It has zero size, which is otherwise not possible
struct Tag
{};
#define ECS_TAG_NAME "ecs::Tag"

#define ECS_ASSUME_ALIGNED_PTR(Type, ptr) (ASSUME_ALIGNED((ptr), ecs::ComponentTypeInfo<Type>::ptr_alignment))
#define ECS_ASSUME_ALIGNED_REF(Type, ptr) (ASSUME_ALIGNED((ptr), ecs::ComponentTypeInfo<Type>::ref_alignment))

#define ECS_MAYBE_VALUE(T)   (eastl::is_scalar<T>::value || eastl::is_same<T, ecs::EntityId>::value)
#define ECS_MAYBE_VALUE_T(T) typename eastl::conditional<ECS_MAYBE_VALUE(T), T, const T &>::type

template <typename T>
struct PtrComponentType
{
  static constexpr bool is_boxed = ComponentTypeInfo<T>::is_boxed;
  typedef T &__restrict ref_type;
  typedef const T &__restrict cref_type;
  typedef typename eastl::conditional<is_boxed, T *__restrict *__restrict, T *__restrict>::type ptr_type;
  typedef typename eastl::conditional<is_boxed, const T *__restrict const *__restrict, const T *__restrict>::type cptr_type;

  template <class U = ref_type>
  static typename eastl::disable_if<is_boxed, U>::type ref(ptr_type p)
  {
    return *(T *__restrict)ECS_ASSUME_ALIGNED_REF(T, p);
  }

  template <class U = ref_type>
  static typename eastl::enable_if<is_boxed, U>::type ref(ptr_type p)
  {
    return *(T *__restrict)ECS_ASSUME_ALIGNED_REF(T, *p);
  }

  template <class U = cref_type>
  static typename eastl::disable_if<is_boxed, U>::type ref(cptr_type p)
  {
    return *(const T *__restrict)ECS_ASSUME_ALIGNED_REF(T, p);
  }

  template <class U = cref_type>
  static typename eastl::enable_if<is_boxed, U>::type ref(cptr_type p)
  {
    return *(const T *__restrict)ECS_ASSUME_ALIGNED_REF(T, *p);
  }

  static cref_type cref(cptr_type p) { return ref(p); }

  static cref_type ref(const void *__restrict p) { return ref(cptr_type(p)); }
  static ref_type ref(void *__restrict p) { return ref(ptr_type(p)); }
  static cref_type cref(const void *__restrict p) { return ref(p); }
};

template <typename T>
__forceinline T &__restrict getRef(T *__restrict p)
{
  return *p;
}
template <typename T>
__forceinline T &__restrict getRef(T *__restrict *__restrict p)
{
  return **p;
}
template <typename T>
__forceinline const T &__restrict getRef(const T *__restrict p)
{
  return *p;
}
template <typename T>
__forceinline const T &__restrict getRef(const T *__restrict const *__restrict p)
{
  return **p;
}

enum ComponentTypeFlags : uint16_t
{
  COMPONENT_TYPE_TRIVIAL = 0, // basically it is pod
  COMPONENT_TYPE_NON_TRIVIAL_CREATE = 1,
  COMPONENT_TYPE_NON_TRIVIAL_MOVE = 2,
  COMPONENT_TYPE_BOXED = 4, // always COMPONENT_TYPE_NON_TRIVIAL_MOVE==0 COMPONENT_TYPE_NON_TRIVIAL_CREATE==1, just sanity
  COMPONENT_TYPE_CREATE_ON_TEMPL_INSTANTIATE = 8,
  COMPONENT_TYPE_NEED_RESOURCES = 16,
  COMPONENT_TYPE_HAS_IO = 32, // that is for optimization, to skip getting io, when io isn't available.
  COMPONENT_TYPE_TRIVIAL_MASK = COMPONENT_TYPE_HAS_IO - 1,
  COMPONENT_TYPE_IS_POD = 64, // that is just for verification
  COMPONENT_TYPE_REPLICATION = 128,

  COMPONENT_TYPE_LAST_PLUS_1,
};

// that is not precisely is_pod. We assume it can be non-trivially creatable, BUT we can memcmp/memcpy on it
inline bool is_pod(ComponentTypeFlags flags) { return (flags & COMPONENT_TYPE_TRIVIAL_MASK) == COMPONENT_TYPE_TRIVIAL; }
inline bool has_io(ComponentTypeFlags flags) { return flags & COMPONENT_TYPE_HAS_IO; }
inline bool need_constructor(ComponentTypeFlags flags)
{
  return (flags & (COMPONENT_TYPE_BOXED | COMPONENT_TYPE_NON_TRIVIAL_CREATE)) != 0;
}
// BITMASK_DECLARE_FLAGS_OPERATORS(ComponentTypeFlags);

class ComponentsMap;
class EntityManager;
class ComponentTypeManager
{
public:
  // Called before any component creation as as chance to request game resources.
  virtual void requestResources(const char *, const ecs::resource_request_cb_t &) {}

  virtual void create(void *, EntityManager &, EntityId, const ComponentsMap &, ecs::component_index_t) {} // allocate (inplace new).
                                                                                                           // We don't have to pass
                                                                                                           // EntityId. But
                                                                                                           // some 1.0/2.0 CMs need
                                                                                                           // that. This is called only
                                                                                                           // for NON_TRIVIAL_CREATE
  virtual void destroy(void *) {} // doesn't make sense to pass EntityId, as it is already killed

  /// copy assignment. This can be non-optimal implementated (destroy(to); return copy(to, from);).But has to has copy
  virtual bool copy(void * /*to*/, const void * /*from*/, ecs::component_index_t, ecs::EntityId = INVALID_ENTITY_ID) const
  {
    return false;
  } // copy constructor. This is called only for NON_TRIVIAL_CREATE, if it is initialized in template

  // comparison. If not implemented, we assume it is always equal.
  virtual bool is_equal(const void * /*to*/, const void * /*from*/) const { return true; }
  /// copy assignment. This can be non-optimal implementated (destroy(to); return copy(to, from);).But has to has copy
  virtual bool assign(void * /*to*/, const void * /*from*/) const
  {
    return false;
  } // assign. returns false if NOT assigned (destination is not changed)

  // this is optimized version. Should be conservatively same as if (!is_equal) {return assign();} return false;(i.e. it can presume
  // changes; but if is_equal returns false IT HAS to return result of assign. return true if changed (not equal). "to" after it should
  // be equal to from (i.e. operator = )
  virtual bool replicateCompare(void *to, const void *from) const
  {
    if (!is_equal(to, from))
      return assign(to, from);
    return false;
  }
  virtual void move(void * /*to*/, void * /*from*/) const {} // move constructor. This is called only for NON_TRIVIAL_MOVE. Currently
                                                             // we don't support non memcpy moveable types

  virtual void clear() {} // this is called on each EntityManager::clear(), after all components were removed. That is used for
                          // unrecommended and rare case when type manager has state
  virtual ~ComponentTypeManager() {}
};

template <class T>
struct BoxedCreator : public ComponentTypeManager
{
  typedef T component_type;

  bool copy(void *to, const void *from, ecs::component_index_t, ecs::EntityId) const override
  {
    G_ASSERT_RETURN(from && to && *(const T **)from, false);
    if (!eastl::is_copy_constructible<T>::value)
    {
      *(void **)to = nullptr;
      return false;
    }
    *(void **)to = allocateOne();
    return ConstructInplaceType<T>::copy(*(T **)to, *(const T **)from);
  }
  bool is_equal(const void *to, const void *from) const override
  {
    G_ASSERT_RETURN(from && to && *(const T **)to && *(const T **)from, true);
    return ConstructInplaceType<T>::is_equal(*(const T **)to, *(const T **)from);
  }
  bool assign(void *to, const void *from) const override
  {
    G_ASSERT_RETURN(from && to && *(const T **)from, false);
    return ConstructInplaceType<T>::assign(*(T **)to, *(const T **)from);
  }
  void create(void *pp, EntityManager &mgr, EntityId eid, const ComponentsMap &map, ecs::component_index_t index) override
  {
    void *p = allocateOne();
    *(void **)pp = p;
    ConstructInplaceType<T>::create(p, mgr, eid, map, index);
  }
  void requestResources(const char *nm, const ecs::resource_request_cb_t &res_cb) override
  {
    ConstructInplaceType<T>::requestResources(nm, res_cb);
  }
  void destroy(void *p) override
  {
    T *t = *(T **)p;
    G_FAST_ASSERT(t);
    eastl::destroy_at(t);
    deallocateOne(t);
  }

  bool replicateCompare(void *to, const void *from) const override
  {
    G_ASSERT_RETURN(from && to && *(const T **)from, false);
    return ConstructInplaceType<T>::replicateCompare(*(T **)to, *(const T **)from);
  }

  void clear() override { ConstructInplaceType<T>::onTypeManagerClear(); }

  BoxedCreator() : allocator(sizeof(T), 0) {}

protected:
  mutable FixedBlockAllocator allocator;
  void *allocateOne() const { return allocator.allocateOneBlock(); }
  void deallocateOne(void *p) const
  {
    if (!allocator.freeOneBlock(p)) // block wasn't allocated in allocator. probably in InitComponent and then moved.
      memfree_anywhere(p);          // BEWARE! this deallocate must match allocateOne AND the one in ChildComponent
  }
};

template <class T>
struct InplaceCreator : public ComponentTypeManager
{
  typedef T component_type;

  bool copy(void *to, const void *from, ecs::component_index_t, ecs::EntityId) const override
  {
    G_ASSERT_RETURN(from && to, false);
    return ConstructInplaceType<T>::copy(to, (const T *)from);
  }
  bool is_equal(const void *to, const void *from) const override
  {
    G_ASSERT_RETURN(from && to, true);
    return ConstructInplaceType<T>::is_equal((const T *)to, (const T *)from);
  }
  bool assign(void *to, const void *from) const override
  {
    G_ASSERT_RETURN(from && to, false);
    return ConstructInplaceType<T>::assign((T *)to, (const T *)from);
  }
  void create(void *p, EntityManager &mgr, EntityId eid, const ComponentsMap &map, ecs::component_index_t index) override
  {
    ECS_DEBUG_FILL_MEM_WITH_PATTERN(p, sizeof(T), 0x7ffdcdcd);
    ConstructInplaceType<T>::create(p, mgr, eid, map, index);
  }
  void requestResources(const char *nm, const ecs::resource_request_cb_t &res_cb) override
  {
    ConstructInplaceType<T>::requestResources(nm, res_cb);
  }
  void destroy(void *p) override
  {
    G_FAST_ASSERT(p);
    eastl::destroy_at((T *)p);
    ECS_DEBUG_FILL_MEM_WITH_PATTERN(p, sizeof(T), 0x7ffddddd);
  }

  bool replicateCompare(void *to, const void *from) const override
  {
    G_ASSERT_RETURN(from && to, false);
    return ConstructInplaceType<T>::replicateCompare((T *)to, (const T *)from);
  }

  void clear() override { ConstructInplaceType<T>::onTypeManagerClear(); }
};

template <class T>
struct BoxedFinalCreator final : public BoxedCreator<T>
{};
template <class T>
struct InplaceFinalCreator final : public InplaceCreator<T>
{};

template <typename T, typename U = T, bool bBoxed = ComponentTypeInfo<T>::is_boxed>
struct CreatorSelector
{
  typedef InplaceFinalCreator<U> type;
};
template <typename T, typename U>
struct CreatorSelector<T, U, true>
{
  typedef BoxedFinalCreator<U> type;
};

struct ComponentType
{
  uint16_t size;
  ComponentTypeFlags flags;
};

typedef ComponentTypeManager *pComponentTypeManager;

typedef pComponentTypeManager (*create_ctm_t)(void *);
typedef void (*destroy_ctm_t)(pComponentTypeManager);

class ComponentSerializer;
// used for auto registration
struct CompileComponentTypeRegister
{
  CompileComponentTypeRegister(const char *name_, uint32_t name_hash_, uint32_t size_, ComponentSerializer *io_, create_ctm_t ctm_,
    destroy_ctm_t dtm_, ComponentTypeFlags flags_) :
    name(name_), name_hash(name_hash_), size(size_), ctm(ctm_), dtm(dtm_), flags(flags_), io(io_)
  {
    next = tail;
    tail = this;
  }

protected:
  const char *name = NULL;                   // name of this entity system, must be unique
  CompileComponentTypeRegister *next = NULL; // slist
  create_ctm_t ctm = 0;
  destroy_ctm_t dtm = 0;
  ComponentSerializer *io = 0;
  uint32_t name_hash;
  uint32_t size;
  ComponentTypeFlags flags;
  static CompileComponentTypeRegister *tail;
  friend struct ComponentTypes;
};


template <typename T>
struct CTMFactory
{
  static ComponentTypeManager *create(void *) { return new T(); }
  static void destroy(ComponentTypeManager *ptr) { delete ptr; }
};

template <typename T, bool ptr>
struct CTMFactory<InitOnDemand<T, ptr>>
{
  static InitOnDemand<T, ptr> obj;
  static ComponentTypeManager *create(void *)
  {
    obj.demandInit();
    return obj;
  }
  static void destroy(ComponentTypeManager *cf)
  {
    G_ASSERT(cf == obj.get());
    (void)cf;
    obj.demandDestroy();
  }
};
template <typename T, bool ptr>
InitOnDemand<T, ptr> CTMFactory<InitOnDemand<T, ptr>>::obj;

struct ComponentTypes
{
  ComponentType getTypeInfo(type_index_t) const;
  ComponentType findTypeInfo(component_type_t) const;
  ComponentTypeManager *getTypeManager(type_index_t) const;
  ComponentTypeManager *createTypeManager(type_index_t);
  ComponentSerializer *getTypeIO(type_index_t) const;
  const char *getTypeNameById(type_index_t) const;
  const char *findTypeName(component_type_t) const;

  component_type_t getTypeById(type_index_t) const;

  type_index_t findType(component_type_t) const;
  type_index_t registerType(const char *name, component_type_t type, uint16_t data_size, ComponentSerializer *io,
    ComponentTypeFlags flags, create_ctm_t ctm, destroy_ctm_t dtm, void *user = nullptr); // manual registration. will not overwrite
                                                                                          // existing
  uint32_t getTypeCount() const { return (uint32_t)types.size(); }
  ~ComponentTypes() { clear(); }

protected:
  ComponentTypes() = default; // only allow to be part of EntityManager
  EA_NON_COPYABLE(ComponentTypes)
  friend class EntityManager;
  void initialize();
  void clear();
  ComponentTypeManager *createTypeManagerImpl(type_index_t);
  G_STATIC_ASSERT(sizeof(ComponentType) == 4);
  eastl::tuple_vector<ComponentSerializer *, component_type_t, ComponentType,
    ComponentTypeManager *, // ctm
    void *,                 // user_data
    eastl::string,          // name
    create_ctm_t, destroy_ctm_t>
    types;
  HashedKeyMap<component_type_t, type_index_t> typesIndex; // hash to index.
  OSSpinlock ctmCreationMutex;
};

inline ComponentTypeManager *ComponentTypes::getTypeManager(type_index_t t) const
{
  return (t < types.size()) ? types.get<ComponentTypeManager *>()[t] : nullptr;
}

inline ComponentTypeManager *ComponentTypes::createTypeManager(type_index_t t)
{
  if (t >= types.size())
    return nullptr;
  // We should use acquire load to get CTM for it to be completely thread-safe, otherwise we can't guarantee we read fully
  // constructed state of CTM, if it was constructed in other thread. However race here is already a very rare case, and
  // acquire load has noticeable performance penalty, so we decided to use relaxed load instead.
  // To consider: pre-create all CTMs for types, declared in loaded templates. This will eliminate the issue
  // entirely, but can introduce other problems.
  if (ComponentTypeManager *ctm = interlocked_relaxed_load_ptr(types.get<ComponentTypeManager *>()[t]))
    return ctm;
  if (!types.get<create_ctm_t>()[t])
    return nullptr;
  return createTypeManagerImpl(t);
}

inline ComponentSerializer *ComponentTypes::getTypeIO(type_index_t t) const
{
  return (t < types.size()) ? types.get<ComponentSerializer *>()[t] : nullptr;
}

inline ComponentType ComponentTypes::getTypeInfo(type_index_t t) const
{
  return (t < types.size()) ? types.get<ComponentType>()[t] : ComponentType{0, COMPONENT_TYPE_TRIVIAL};
}
inline ComponentType ComponentTypes::findTypeInfo(component_type_t tp) const { return getTypeInfo(findType(tp)); }

inline type_index_t ComponentTypes::findType(component_type_t tp) const { return typesIndex.findOr(tp, INVALID_COMPONENT_TYPE_INDEX); }

inline component_type_t ComponentTypes::getTypeById(type_index_t t) const
{
  return (t < types.size()) ? types.get<component_type_t>()[t] : 0;
}

inline const char *ComponentTypes::getTypeNameById(type_index_t t) const
{
  return (t < types.size()) ? types.get<eastl::string>()[t].c_str() : nullptr;
}

inline const char *ComponentTypes::findTypeName(component_type_t tp) const { return getTypeNameById(findType(tp)); }

template <>
struct ComponentTypeInfo<Tag> // tag type traits, so it has 0 size
{
  static constexpr const char *type_name = ECS_TAG_NAME;
  static constexpr component_type_t type = ECS_HASH(ECS_TAG_NAME).hash;
  static constexpr bool is_pod_class = true;
  static constexpr bool is_boxed = false;
  static constexpr bool is_non_trivial_move = false;
  static constexpr bool is_create_on_templ_inst = false;
  static constexpr size_t size = 0;
  static constexpr size_t ref_alignment = 1;
  static constexpr size_t ptr_alignment = ref_alignment;
};

}; // namespace ecs

#undef ECS_DEBUG_FILL_MEM_WITH_PATTERN

ECS_DECLARE_TYPE(ecs::EntityId);
ECS_DECLARE_RELOCATABLE_TYPE(ecs::string);
