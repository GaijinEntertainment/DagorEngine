//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdio.h> // snprintf
#include <EASTL/tuple.h>
#include <EASTL/type_traits.h>
#include <daECS/core/event.h>
#include <daECS/core/componentType.h>
#include <daECS/core/entityManager.h>
#include <ecs/scripts/scripts.h>
#include <ska_hash_map/flat_hash_map2.hpp>

namespace Sqrat
{
void PushVar(HSQUIRRELVM vm, const ecs::Event &value);
void PushVar(HSQUIRRELVM vm, ecs::Event *value);
}; // namespace Sqrat
#include "sqEntity.h"

namespace ecs
{
namespace sq
{

template <typename Evt, size_t N>
struct EventPushImpl;
template <typename Evt>
struct EventPushImpl<Evt, 0>
{
  static void push(HSQUIRRELVM, const Evt &, int)
  {
    G_FAST_ASSERT(0);
  } // shall not be called as we explicitly checked for index earlier
};
template <typename Evt>
struct EventPushImpl<Evt, 1>
{
  static void push(HSQUIRRELVM vm, const Evt &evt, int i)
  {
    G_ASSERT(i == 0);
    G_UNUSED(i);
    Sqrat::PushVar(vm, evt.template get<0>());
  }
};
template <typename Evt, size_t N>
struct EventPushImpl
{
  static void push(HSQUIRRELVM vm, const Evt &evt, int i)
  {
    if (i == N - 1)
      Sqrat::PushVar(vm, evt.template get<N - 1>());
    else
      EventPushImpl<Evt, N - 1>::push(vm, evt, i);
  }
};

template <typename E>
static SQInteger evt_get(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<E *>(vm))
    return SQ_ERROR;

  if (sq_gettype(vm, 2) != OT_INTEGER)
  {
    sq_pushnull(vm);
    return sq_throwobject(vm);
  }

  Sqrat::Var<E *> evtVar(vm, 1);
  G_ASSERT_RETURN(evtVar.value, sq_throwerror(vm, "Bad 'this'"));
  int i = (int)Sqrat::Var<SQInteger>(vm, 2).value;
  constexpr size_t sz = eastl::tuple_size<typename E::TupleType>::value;
  if (sz == 0 || (unsigned)i >= sz)
  {
    sq_pushnull(vm);
    return sq_throwobject(vm);
  }
  EventPushImpl<E, sz>::push(vm, *evtVar.value, i); // To consider: can also be implemented as table of (push<T>) function pointers
  return 1;
}


template <size_t N>
static SQInteger evt_len(ecs::Event *)
{
  return N;
}

template <size_t N>
static SQInteger evt_nexti(HSQUIRRELVM vm)
{
  Sqrat::Object self = Sqrat::Var<Sqrat::Object>(vm, 1).value;
  G_ASSERT_RETURN(!self.IsNull(), sq_throwerror(vm, "Bad 'this'"));
  Sqrat::Object prevIdx = Sqrat::Var<Sqrat::Object>(vm, 2).value;
  if (prevIdx.IsNull()) // initial iteration
  {
    if (N == 0)
      sq_pushnull(vm);
    else
      sq_pushinteger(vm, 0);
  }
  else
  {
    SQInteger nextI = prevIdx.Cast<SQInteger>() + 1;
    if (nextI < N)
      sq_pushinteger(vm, nextI);
    else
      sq_pushnull(vm);
  }
  return 1;
}

template <typename T, bool val = ECS_MAYBE_VALUE(T)>
struct MaybeAddRef
{
  typedef typename eastl::conditional<val, T, const T &>::type type;
};

template <typename E, size_t I>
static typename MaybeAddRef<typename eastl::tuple_element<I, typename E::TupleType>::type>::type ecs_event_get(const E *evt)
{
  return eastl::get<I>(*evt);
}

template <typename Cls, typename Tuple, size_t... Indexes>
inline void event_ctor_bind_helper(Cls &cls, eastl::index_sequence<Indexes...>)
{
  cls.template Ctor<typename MaybeAddRef<typename eastl::tuple_element<Indexes, Tuple>::type>::type...>();
}

typedef bool (*push_instance_fn_t)(HSQUIRRELVM, void *);
void register_native_event_impl(ecs::event_type_t evtt, push_instance_fn_t push_inst_fn);
template <typename C>
inline void register_native_event()
{
  register_native_event_impl(C::staticType(), (push_instance_fn_t)&Sqrat::ClassType<C>::PushNativeInstance);
}

typedef ::std::nullptr_t term_event_t;

template <typename... Evts>
class EventsBind;
template <>
class EventsBind<>
{
public:
  static void bind(HSQUIRRELVM, Sqrat::Table &) {}
};
template <>
class EventsBind<term_event_t>
{
public:
  static void bind(HSQUIRRELVM, Sqrat::Table &) {}
};
template <typename First, typename... Evts>
class EventsBind<First, Evts...>
{
public:
  static void bind(HSQUIRRELVM vm, Sqrat::Table &tbl)
  {
    constexpr size_t argsN = eastl::tuple_size<typename First::TupleType>::value;

    Sqrat::DerivedClass<First, ecs::Event> sqEventCls(vm, First::staticName());
    register_native_event<First>();
    event_ctor_bind_helper<decltype(sqEventCls), typename First::TupleType>(sqEventCls, eastl::make_index_sequence<argsN>());
    sqEventCls //
      .SquirrelFunc("_get", evt_get<First>, 2, "x.")
      .GlobalFunc("len", evt_len<argsN>)
      .SquirrelFunc("_nexti", evt_nexti<argsN>, 2, "xi|o")
      /**/;

    {
      Sqrat::Object clsObj(sqEventCls.GetObject(), vm);
      tbl.Bind(First::staticName(), clsObj);
    }

    EventsBind<Evts...>::bind(vm, tbl);
  }

  static Sqrat::Table bindall(Sqrat::Table &tbl)
  {
    G_ASSERT(Sqrat::ClassType<ecs::Event>::hasClassData(tbl.GetVM())); // if this assertion triggered then perhaps
                                                                       // ecs_register_sq_binding() wasn't called for given vm
    ecs::sq::EventsBind<First, Evts...>::bind(tbl.GetVM(), tbl);
    return tbl;
  }

  static Sqrat::Table bindall(HSQUIRRELVM vm)
  {
    Sqrat::Table tbl(vm);
    return bindall(tbl);
  }
};

SQInteger push_event_field(HSQUIRRELVM vm, ecs::Event *event, ecs::EventsDB::event_id_t event_id, int idx);
bool pop_event_field(ecs::event_type_t event_type, Sqrat::Object slot, const ecs::string &field_name, ecs::component_type_t type,
  uint8_t offset, char *evtData);

} // namespace sq
} // namespace ecs
