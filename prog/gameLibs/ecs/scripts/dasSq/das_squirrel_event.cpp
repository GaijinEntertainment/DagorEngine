#include <dasModules/dasEvent.h>
#include <dasModules/dasModulesCommon.h>
#include <ecs/scripts/sqBindEvent.h>
#include <quirrel/bindQuirrelEx/autoBind.h>
#include <quirrel/sqModules/sqModules.h>


// sq ----
namespace bind_dascript
{
static SQInteger das_event_ctor(HSQUIRRELVM vm)
{
  const int nArgs = sq_gettop(vm);
  // class object at index 1
  Sqrat::Var<Sqrat::Object> classObj(vm, 1);
  const ecs::event_type_t eventType = classObj.value.RawGetSlotValue<int>("eventType", -1);


  ecs::EventsDB::event_id_t eventId = g_entity_mgr->getEventsDb().findEvent(eventType);
  if (EASTL_UNLIKELY(eventId == ecs::EventsDB::invalid_event_id))
    return sq_throwerror(vm, "event without event scheme");

  const ecs::event_size_t eventSize = g_entity_mgr->getEventsDb().getEventSize(eventId);
  const ecs::event_flags_t flags = g_entity_mgr->getEventsDb().getEventFlags(eventId);

  char *evtData = new char[eventSize];
  memset(evtData, 0, eventSize);
  DasEvent *evt = (DasEvent *)evtData;
  evt->set(eventType, eventSize, flags);

  if (nArgs >= 2)
  {
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
  Sqrat::ClassType<DasEvent>::SetManagedInstance(vm, 1, evt);
  return 0;
}


SQInteger dascript_evt_get(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ecs::Event *>(vm, 1))
    return SQ_ERROR;

  Sqrat::Var<ecs::Event *> evtVar(vm, 1);
  G_ASSERT_RETURN(evtVar.value, sq_throwerror(vm, "Bad 'this'"));
  const char *key;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &key)));

  const ecs::EventsDB::event_id_t eventId = g_entity_mgr->getEventsDb().findEvent(evtVar.value->getType());
  if (EASTL_UNLIKELY(eventId == ecs::EventsDB::invalid_event_id))
  {
    sq_pushnull(vm);
    return sq_throwobject(vm);
  }
  const int fieldIndex = g_entity_mgr->getEventsDb().findFieldIndex(eventId, key);
  if (EASTL_UNLIKELY(fieldIndex < 0))
  {
    sq_pushnull(vm);
    return sq_throwobject(vm);
  }
  return ecs::sq::push_event_field(vm, evtVar.value, eventId, fieldIndex);
}


void bind_das_events(HSQUIRRELVM vm, uint32_t vm_mask, SqModules *modules_mgr)
{
  if (!Sqrat::ClassType<ecs::Event>::hasClassData(vm))
    return;
  if (vm_mask != sq::VM_GAME && vm_mask != sq::VM_UI)
    return;

  Sqrat::DerivedClass<DasEvent, ecs::Event> baseDasEvent(vm, "DasEvent");
  baseDasEvent.SquirrelCtor(das_event_ctor, -1, ".o|t").SquirrelFunc("_get", dascript_evt_get, 2, "xs");

  Sqrat::ClassData<DasEvent> *sqDasEventClassData = Sqrat::ClassType<DasEvent>::getClassData(vm);
  G_ASSERT(sqDasEventClassData);

  Sqrat::Table eventClasses(vm);

  const ecs::EventsDB &eventsDb = g_entity_mgr->getEventsDb();
  for (int id = 0, n = eventsDb.getEventsCount(); id < n; ++id)
  {
    if (!eventsDb.hasEventScheme(id))
      continue;
    const ecs::event_type_t eventType = eventsDb.getEventType(id);
    const char *eventName = g_entity_mgr->getEventsDb().getEventName(id);

    sq_pushobject(vm, baseDasEvent.GetObject());
    sq_newclass(vm, true);
    sq_pushstring(vm, "eventType", -1);
    sq_pushinteger(vm, eventType);
    sq_rawset(vm, -3);

    G_VERIFY(SQ_SUCCEEDED(sq_settypetag(vm, -1, sqDasEventClassData->staticData.get())));

    Sqrat::Var<Sqrat::Object> eventClass(vm, -1);
    eventClasses.SetValue(eventName, eventClass.value);
    sq_poptop(vm); // pop class

    ecs::sq::register_native_event_impl(eventType,
      (ecs::sq::push_instance_fn_t)&Sqrat::ClassType<bind_dascript::DasEvent>::PushNativeInstance);
  }

  if (Sqrat::Object *existingModule = modules_mgr->findNativeModule("dasevents")) // if exist -> merge with overwrite
  {
    debug("das_net: override dasevents module in vm %d", vm_mask);
    Sqrat::Table existingTbl(*existingModule);
    Sqrat::Object::iterator it;
    while (eventClasses.Next(it))
      existingTbl.SetValue(Sqrat::Object(it.getKey(), vm), Sqrat::Object(it.getValue(), vm));
  }
  else // not exist -> add new
  {
    debug("das_net: register dasevents module in vm %d", vm_mask);
    modules_mgr->addNativeModule("dasevents", eventClasses);
  }
}
} // namespace bind_dascript

namespace Sqrat
{
template <>
struct InstanceToString<bind_dascript::DasEvent>
{
  static SQInteger Format(HSQUIRRELVM vm)
  {
    if (!Sqrat::check_signature<bind_dascript::DasEvent *>(vm))
      return SQ_ERROR;

    Sqrat::Var<bind_dascript::DasEvent *> varSelf(vm, 1);
    bind_dascript::DasEvent *evt = varSelf.value;
    const ecs::EventsDB &eventsDb = g_entity_mgr->getEventsDb();
    ecs::event_type_t eventType = evt->getType();
    ecs::EventsDB::event_id_t eventId = eventsDb.findEvent(eventType);

    if (EASTL_UNLIKELY(eventId == ecs::EventsDB::invalid_event_id))
    {
      String s(0, "Unknown event '%s' <0x%X>", evt->getName(), evt->getType());
      sq_pushstring(vm, s.c_str(), s.size());
      return 1;
    }
    if (EASTL_UNLIKELY(!eventsDb.hasEventScheme(eventId)))
    {
      String s(0, "('%s' <0x%X>)", evt->getName(), evt->getType());
      sq_pushstring(vm, s.c_str(), s.size());
      return 1;
    }
    const ecs::event_scheme_t &scheme = eventsDb.getEventScheme(eventId);
    String res(eventsDb.getEventName(eventId));
    const auto *names = scheme.get<eastl::string>();
    for (int i = 0, n = (int)scheme.size(); i < n; ++i)
    {
      res.append(" ");
      res.append(names[i].c_str());
      res.append(": ");
      ecs::sq::push_event_field(vm, evt, eventId, i);
      if (SQ_SUCCEEDED(sq_tostring(vm, -1)))
      {
        const char *valStr = nullptr;
        G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &valStr)));
        res.append(valStr);
      }
      else
        res.append("<n/a>");
      sq_pop(vm, 1);
    }
    sq_pushstring(vm, res.c_str(), res.size());
    return 1;
  }
};

} // namespace Sqrat
