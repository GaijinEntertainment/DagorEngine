//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <debug/dag_assert.h>
#include <EASTL/type_traits.h>
#include <EASTL/tuple.h>
#include <EABase/nullptr.h>
#include "ecsHash.h"
#include "entityId.h"
#include "utility/ecsForEach.h"
#include "internal/typesAndLimits.h"
#include "internal/asserts.h"
#include "internal/inplaceKeySet.h"

namespace ecs
{
typedef uint32_t event_type_t;
typedef InplaceKeySet<event_type_t, 7> EventSet; // sorted vector with fixed initial storage

template <typename... Args>
struct EventSetBuilder;
template <>
struct EventSetBuilder<>
{
  static void build(EventSet &) {}
  static EventSet build() { return EventSet(); }
};
template <>
struct EventSetBuilder<std::nullptr_t>
{
  static void build(EventSet &) {}
  static EventSet build() { return EventSet(); }
};
template <typename T>
struct EventSetInsertHelper
{
  static void insert(EventSet &ev) { ev.insert(T::staticType()); }
};
template <typename T, T v>
struct EventSetInsertHelper<eastl::integral_constant<T, v>>
{
  static void insert(EventSet &ev) { ev.insert(v); }
};
template <typename First, typename... Rest>
struct EventSetBuilder<First, Rest...>
{
  static void build(EventSet &ev)
  {
    EventSetInsertHelper<First>::insert(ev);
    EventSetBuilder<Rest...>::build(ev);
  }
  static EventSet build()
  {
    EventSet ret;
    build(ret);
    return ret;
  }
};

typedef uint16_t event_flags_t;
typedef uint16_t event_size_t;

enum EventFlags : event_flags_t
{
  //  EventCastType
  EVCAST_UNKNOWN = 0x00,
  EVCAST_UNICAST = 0x01,
  EVCAST_BROADCAST = 0x02,
  EVCAST_BOTH = 0x03,

  EVFLG_CASTMASK = EVCAST_BOTH,
  EVFLG_SERIALIZE = 0x04,  // need (de)serialization for networking
  EVFLG_DESTROY = 0x08,    // need destruction
  EVFLG_SCHEMELESS = 0x10, // actual class is ecs::SchemelessEvent. This is temporarily, as we plan to get rid of all schemeless
                           // events, once we make inspection code
  EVFLG_CORE = 0x20,       // this is not really required, that's for validation purpose
  EVFLG_PROFILE = 0x40,    // ES for this event will be profiled by default
};
// we currently don't allow vec4f in Event
//  If ever needed, just change EVENT_ALIGNMENT to 16, and/or
//  change align_event_on_emplace to true
static constexpr int EVENT_ALIGNMENT = alignof(uint32_t);
static constexpr bool align_event_on_emplace = false;


struct alignas(align_event_on_emplace ? 0 : EVENT_ALIGNMENT) Event
{
protected:
  event_type_t type;
  event_size_t length;
  event_flags_t flags;
  Event(event_type_t tp, event_size_t l, event_flags_t fl) : type(tp), length(l), flags(fl)
  {
    DAECS_EXT_FAST_ASSERT(l >= sizeof(Event));
  }

public:
  static constexpr size_t max_event_size = 1024; // can be increased if needed
  event_type_t getType() const { return type; }
  event_flags_t getFlags() const { return flags; }
  event_size_t getLength() const { return length; }
  const char *getName() const; // this is hashmap lookup, slow, should be only used for inspection!
  template <typename T>
  bool is() const
  {
    return getType() == T::staticType();
  }

  template <typename T>
  const T *cast() const
  {
    return is<T>() ? static_cast<const T *>(this) : nullptr;
  }
  template <int pow_of_two_capacity_shift>
  friend struct EventCircularBuffer; // for asserts
};
typedef void destroy_event(ecs::EntityManager &, ecs::Event &);
typedef void move_out_event(ecs::EntityManager &, void *__restrict to, ecs::Event &&from); // will be moved out (move constructor
                                                                                           // called)


#define ECS_INSIDE_EVENT_DECL(Klass, Flags)                                                       \
  static constexpr ::ecs::event_flags_t staticFlags()                                             \
  {                                                                                               \
    return (Flags) | (eastl::is_trivially_destructible<Klass>::value ? 0 : ::ecs::EVFLG_DESTROY); \
  }                                                                                               \
  static constexpr ::ecs::event_type_t staticType() { return ECS_HASH(#Klass).hash; }             \
  static constexpr const char *staticName() { return #Klass; }                                    \
  static void destroy(ecs::EntityManager &, Event &e)                                             \
  {                                                                                               \
    G_STATIC_ASSERT(sizeof(Klass) <= ::ecs::Event::max_event_size);                               \
    if (!(Klass::staticFlags() & ::ecs::EVFLG_DESTROY)) /*to reduce generated code size*/         \
      return;                                                                                     \
    DAECS_EXT_ASSERT(e.getType() == staticType());                                                \
    static_cast<Klass &>(e).~Klass();                                                             \
  }                                                                                               \
  static void move_out(ecs::EntityManager &, void *__restrict allocateAt, Event &&from)           \
  {                                                                                               \
    if (!(Klass::staticFlags() & ::ecs::EVFLG_DESTROY)) /*to reduce generated code size*/         \
      return;                                                                                     \
    DAECS_EXT_ASSERT(from.getType() == staticType());                                             \
    new (allocateAt, _NEW_INPLACE) Klass(eastl::move(static_cast<Klass &&>(from)));               \
  }

#define ECS_UNICAST_EVENT_DECL(Klass)   ECS_INSIDE_EVENT_DECL(Klass, ::ecs::EVCAST_UNICAST)
#define ECS_BROADCAST_EVENT_DECL(Klass) ECS_INSIDE_EVENT_DECL(Klass, ::ecs::EVCAST_BROADCAST)
#define ECS_EVENT_CONSTRUCTOR(Klass)    ecs::Event(ECS_HASH(#Klass).hash, sizeof(Klass), staticFlags())

#define ECS_BASE_DECL_EVENT_TYPE(Klass, Flags, ...)                                                          \
  class Klass : public ecs::Event, public eastl::tuple<__VA_ARGS__>                                          \
  {                                                                                                          \
  public:                                                                                                    \
    ECS_INSIDE_EVENT_DECL(Klass, Flags)                                                                      \
    typedef eastl::tuple<__VA_ARGS__> TupleType;                                                             \
    template <typename... Args,                                                                              \
      typename eastl::enable_if<eastl::tuple_size<TupleType>::value == sizeof...(Args), bool>::type = false> \
    Klass(Args &&...args) : ECS_EVENT_CONSTRUCTOR(Klass), TupleType(eastl::forward<Args>(args)...)           \
    {                                                                                                        \
      G_STATIC_ASSERT(sizeof(Klass) <= ecs::Event::max_event_size);                                          \
    }                                                                                                        \
    Klass(TupleType &&tup) : ECS_EVENT_CONSTRUCTOR(Klass), TupleType(eastl::move(tup)) {}                    \
    template <size_t I>                                                                                      \
    const typename eastl::tuple_element<I, TupleType>::type &get() const                                     \
    {                                                                                                        \
      return eastl::get<I>(static_cast<const TupleType &>(*this));                                           \
    }                                                                                                        \
    template <size_t I>                                                                                      \
    typename eastl::tuple_element<I, TupleType>::type &get()                                                 \
    {                                                                                                        \
      return eastl::get<I>(static_cast<TupleType &>(*this));                                                 \
    }                                                                                                        \
  };

#define ECS_UNICAST_EVENT_TYPE(Klass, ...)   ECS_BASE_DECL_EVENT_TYPE(Klass, ::ecs::EVCAST_UNICAST, __VA_ARGS__)
#define ECS_BROADCAST_EVENT_TYPE(Klass, ...) ECS_BASE_DECL_EVENT_TYPE(Klass, ::ecs::EVCAST_BROADCAST, __VA_ARGS__)
#define ECS_UNICAST_PROFILE_EVENT_TYPE(Klass, ...) \
  ECS_BASE_DECL_EVENT_TYPE(Klass, ::ecs::EVCAST_UNICAST | ::ecs::EVFLG_PROFILE, __VA_ARGS__)
#define ECS_BROADCAST_PROFILE_EVENT_TYPE(Klass, ...) \
  ECS_BASE_DECL_EVENT_TYPE(Klass, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE, __VA_ARGS__)

struct EventInfoLinkedList
{
  EventInfoLinkedList(const char *n, event_type_t typeT, event_size_t size, event_flags_t flags, destroy_event *d_,
    move_out_event *move_) :
    name(n), eventType(typeT), eventSize(size), eventFlags(flags), destroy(d_), move_out(move_)
  {
    next = (EventInfoLinkedList *)tail;
    tail = this;
    generation++;
  }
  EventInfoLinkedList(const char *n, event_type_t typeT, event_size_t size, event_flags_t flags) :
    EventInfoLinkedList(n, typeT, size, flags, nullptr, nullptr)
  {}
  const char *getName() const { return name; }
  template <typename Lambda> // return bools if should be removed
  static inline void remove_if(Lambda fn);
  template <typename Lambda> // return bools if found
  static inline EventInfoLinkedList *find_if(EventInfoLinkedList *, Lambda fn);
  event_type_t getEventType() const { return eventType; }
  const char *getEventName() const { return name; }
  event_size_t getEventSize() const { return eventSize; }
  event_flags_t getEventFlags() const { return eventFlags; }
  destroy_event *getDestroyFunc() const { return destroy; }
  move_out_event *getMoveOutFunc() const { return move_out; }
  static uint32_t getGeneration() { return generation; }
  static EventInfoLinkedList *get_tail() { return tail; }
  static EventInfoLinkedList *get_registered_tail() { return registered_tail; }

protected:
  const char *name = NULL;    // name of this entity system, must be unique
  event_type_t eventType = 0; // typically hashed name
  event_size_t eventSize = 0;
  event_flags_t eventFlags = 0;
  destroy_event *destroy = nullptr;
  move_out_event *move_out = nullptr;
  EventInfoLinkedList *next = nullptr;
  static EventInfoLinkedList *tail, *registered_tail;
  static inline uint32_t generation = 0;
  friend class EventsDB;
};

template <typename T>
class EventInfoLinkedListInst
{
public:
  EventInfoLinkedListInst() :
    inst(T::staticName(), T::staticType(), sizeof(T), T::staticFlags(), (T::staticFlags() & EVFLG_DESTROY) ? &T::destroy : nullptr,
      (T::staticFlags() & EVFLG_DESTROY) ? &T::move_out : nullptr)
  {
    G_STATIC_ASSERT(sizeof(T) < eastl::numeric_limits<event_size_t>::max());
  }

protected:
  EventInfoLinkedList inst;
};

#define ECS_REGISTER_EVENT(event_type, ...)        static ecs::EventInfoLinkedListInst<event_type> ecs_event_type_##event_type;
#define ECS_REGISTER_EVENT_NS(ns, event_type, ...) static ecs::EventInfoLinkedListInst<ns ::event_type> ecs_event_type_##event_type;


template <typename Lambda> // return bools if should be removed
inline void EventInfoLinkedList::remove_if(Lambda fn)
{
  for (EventInfoLinkedList *i = tail, *prev = nullptr; i;)
  {
    EventInfoLinkedList *nextI = i->next;
    if (fn(i))
    {
      if (prev)
        prev->next = nextI;
      else
        tail = nextI;
      ++EventInfoLinkedList::generation;
    }
    else
      prev = i;
    i = nextI;
  }
}

template <typename Lambda> // return bools if found
inline EventInfoLinkedList *EventInfoLinkedList::find_if(EventInfoLinkedList *start, Lambda fn)
{
  for (EventInfoLinkedList *i = start; i;)
  {
    EventInfoLinkedList *nextI = i->next;
    if (fn(i))
      return i;
    i = nextI;
  }
  return nullptr;
}

}; // namespace ecs

#if _ECS_CODEGEN
#define ECS_ON_EVENT_ONE(a) __attribute__((annotate("@Event:" #a)))
#define ECS_ON_EVENT(...)   ECS_FOR_EACH(ECS_ON_EVENT_ONE, __VA_ARGS__)
#else
#define ECS_ON_EVENT_ONE(a)
#define ECS_ON_EVENT(...)
#endif
