//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/aotEcsEvents.h>
#include <ecs/scripts/sqBindEvent.h>

namespace bind_dascript
{

SQInteger dascript_evt_get(HSQUIRRELVM vm);

template <typename First>
class DasEventsBind
{
public:
  static SQInteger CustomNew(HSQUIRRELVM vm)
  {
    const int nArgs = sq_gettop(vm);

    First *evt = new First(typename First::TupleType());

    if (nArgs >= 2)
    {
      char *evtData = (char *)evt;
      constexpr ecs::event_type_t eventType = First::staticType();
      ecs::EventsDB::event_id_t eventId = g_entity_mgr->getEventsDb().findEvent(eventType);
      if (EASTL_UNLIKELY(eventId == ecs::EventsDB::invalid_event_id))
        return sq_throwerror(vm, "unknown event");
      if (!g_entity_mgr->getEventsDb().hasEventScheme(eventId))
        return sq_throwerror(vm, "event without event scheme");

      const ecs::event_scheme_t &scheme = g_entity_mgr->getEventsDb().getEventScheme(eventId);

      HSQOBJECT mapObj;
      G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &mapObj)));

      Sqrat::Object map(mapObj, vm);

      const eastl::string *names = scheme.get<eastl::string>();
      const uint8_t *offsets = scheme.get<uint8_t>();
      const ecs::component_type_t *types = scheme.get<ecs::component_type_t>();
      for (int i = 0, n = (int)scheme.size(); i < n; ++i)
      {
        Sqrat::Object slot = map.RawGetSlot(names[i].c_str());
        if (!slot.IsNull())
          ecs::sq::pop_event_field(eventType, slot, names[i], types[i], offsets[i], evtData);
      }

#if DAGOR_DBGLEVEL > 0
      Sqrat::Object::iterator it;
      while (map.Next(it))
      {
        auto fieldName = it.getName();
        bool found = false;
        for (int i = 0, n = (int)scheme.size(); !found && i < n; ++i)
          if (names[i] == fieldName)
            found = true;
        if (!found)
          logerr("Unexpected field %s of event %s", fieldName, g_entity_mgr->getEventsDb().getEventName(eventId));
      }
#endif
    }
    Sqrat::ClassType<First>::SetManagedInstance(vm, 1, evt);
    return 0;
  }

  static void CustomCtor(HSQUIRRELVM v)
  {
    sq_pushobject(v, Sqrat::ClassType<First>::getClassData(v)->classObj);
    sq_pushstring(v, "constructor", -1);
    sq_newclosure(v, CustomNew, 0);
    sq_setparamscheck(v, -1, ".o|t");
    SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(v, -3, false)));
    sq_pop(v, 1);
  }

  static void bind(HSQUIRRELVM vm, Sqrat::Table &tbl)
  {
    G_ASSERT(Sqrat::ClassType<ecs::Event>::hasClassData(vm));

    Sqrat::DerivedClass<First, ecs::Event> sqEventCls(vm, First::staticName());
    ecs::sq::register_native_event<First>();
    sqEventCls.SquirrelFunc("_get", dascript_evt_get, 2, "xs");

    CustomCtor(vm);

    {
      Sqrat::Object clsObj(sqEventCls.GetObject(), vm);
      tbl.Bind(First::staticName(), clsObj);
    }
  }
};

} // namespace bind_dascript
