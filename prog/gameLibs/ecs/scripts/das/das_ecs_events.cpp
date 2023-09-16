#include "das_ecs.h"
#include <dasModules/dasEvent.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/net/message.h> // net::MessageRouting, PacketReliability
#include <daNet/bitStream.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcs.h>
#include <dasModules/aotEcsEvents.h>
#include <dasModules/dasScriptsLoader.h>

namespace das
{
template <>
struct ManagedStructureAlignof<ecs::Event>
{
  static constexpr size_t alignment = 16;
};
}; // namespace das

namespace bind_dascript
{
// TODO: exract network related functionality to separate module
const uint8_t DASEVENT_NO_ROUTING = 0XFF;
const uint8_t DASEVENT_NATIVE_ROUTING = 0xFE;

static eastl::vector_map<ecs::event_type_t, DascriptEventDesc> dascript_net_events;
static uint64_t dasEventsGeneration = 0;
uint64_t get_dasevents_generation() { return dasEventsGeneration; }

DascriptEventDesc::NetLiable get_dasevent_net_liable(ecs::event_type_t type)
{
  const auto desc = dascript_net_events.find(type);
  if (desc != dascript_net_events.end())
    return desc->second.netLiable;
  return DascriptEventDesc::NetLiable::Strict;
}

uint8_t get_dasevent_routing(ecs::event_type_t type)
{
  const auto desc = dascript_net_events.find(type);
  if (desc != dascript_net_events.end())
    return desc->second.routing;
  return DASEVENT_NO_ROUTING;
}

ecs::event_flags_t get_dasevent_cast_flags(ecs::event_type_t type)
{
  const auto desc = dascript_net_events.find(type);
  if (desc != dascript_net_events.end())
    return desc->second.castFlags;
  return ecs::EVCAST_UNKNOWN;
}

eastl::tuple<uint8_t, PacketReliability> get_dasevent_routing_reliability(ecs::event_type_t type)
{
  const auto desc = dascript_net_events.find(type);
  if (desc != dascript_net_events.end())
    return {desc->second.routing, desc->second.reliability};
  return {DASEVENT_NO_ROUTING, RELIABLE_ORDERED};
}

static bool lockedNetEventsRegistration = false;
uint32_t lock_dasevent_net_version()
{
  lockedNetEventsRegistration = true;
  return get_dasevent_net_version();
}

uint32_t get_dasevent_net_version()
{
  static constexpr uint32_t DASEVENTS_VERSION = 2;
  uint32_t hash = fnv1a_step<32>(DASEVENTS_VERSION);
  int num = 0;
  for (const auto &event : dascript_net_events)
  {
    const uint8_t routing = event.second.routing;
    if (event.second.netLiable == DascriptEventDesc::NetLiable::Strict && routing != DASEVENT_NO_ROUTING &&
        routing != DASEVENT_NATIVE_ROUTING)
    {
      const ecs::EventsDB::event_id_t eventId = g_entity_mgr->getEventsDb().findEvent(event.first);
      const ecs::EventsDB::event_scheme_hash_t eventHash = g_entity_mgr->getEventsDb().getEventSchemeHash(eventId);
      G_ASSERTF_CONTINUE(eventHash != ecs::EventsDB::invalid_event_scheme_hash,
        "event '%s' <0x%X> with routing, but without registered scheme", g_entity_mgr->getEventsDb().getEventName(eventId),
        event.first);
      hash = fnv1a_step<32>(eventHash, hash);
      num += 1;
      debug("das_net: net proto calc with %s = <0x%X>", g_entity_mgr->getEventsDb().getEventName(eventId), hash);
    }
  }
  debug("das_net: net proto version 0x%X, events num=%d", hash, num);
  return hash;
}

void unlock_dasevent_net_version() { lockedNetEventsRegistration = false; }

static constexpr int SKIP_FIRST_EVENT_FIELDS = 3;

struct EventAnnotation final : das::ManagedStructureAnnotation<ecs::Event, false>
{
  EventAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Event", ml)
  {
    cppName = " ::ecs::Event";
    addProperty<DAS_BIND_MANAGED_PROP(getName)>("eventName", "getName");
    addProperty<DAS_BIND_MANAGED_PROP(getType)>("eventType", "getType");
  }
  virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
  virtual bool isRefType() const override { return true; }
  virtual bool isLocal() const override { return false; }
  virtual bool isPod() const override { return false; }
  virtual bool canCopy() const override { return false; }
  virtual bool canMove() const override { return false; }
};

static inline bool common_touch(const das::StructurePtr &st)
{
  G_STATIC_ASSERT(sizeof(ecs::Event) == sizeof(uint32_t) + sizeof(ecs::event_size_t) + sizeof(ecs::event_flags_t));
  struct TestEvent : public ecs::Event
  {
    int field;
    TestEvent() : Event(0, 0, 0) {}
  };
  G_STATIC_ASSERT(sizeof(TestEvent) == sizeof(uint32_t) + sizeof(uint32_t) + sizeof(ecs::event_size_t) + sizeof(ecs::event_flags_t));
  G_STATIC_ASSERT(offsetof(TestEvent, field) == sizeof(ecs::Event));
  st->fields
    .emplace(st->fields.begin(), "eventFlags", das::make_smart<das::TypeDecl>(das::Type::tUInt16), das::ExpressionPtr(),
      das::AnnotationArgumentList(), false, st->at)
    ->generated = true;
  st->fields
    .emplace(st->fields.begin(), "eventSize", das::make_smart<das::TypeDecl>(das::Type::tUInt16), das::ExpressionPtr(),
      das::AnnotationArgumentList(), false, st->at)
    ->generated = true;
  st->fields
    .emplace(st->fields.begin(), "eventType", das::make_smart<das::TypeDecl>(das::Type::tUInt), das::ExpressionPtr(),
      das::AnnotationArgumentList(), false, st->at)
    ->generated = true;
  return true;
}

static inline ecs::event_flags_t resolve_cast_type(const das::AnnotationArgumentList &args)
{
  const bool unicastFlag = args.getBoolOption("unicast", false);
  const bool broadcastFlag = args.getBoolOption("broadcast", false);
  if (unicastFlag || broadcastFlag)
    return unicastFlag && broadcastFlag ? ecs::EVCAST_BOTH : unicastFlag ? ecs::EVCAST_UNICAST : ecs::EVCAST_BROADCAST;
  return ecs::EVCAST_UNKNOWN;
}

static bool resolve_event_version(const das::AnnotationArgumentList &args, uint16_t &res, bool &found, das::string &err)
{
  res = 0;
  found = false;
  for (const das::AnnotationArgument &arg : args)
  {
    if (arg.name != "version")
      continue;
    if (found)
    {
      err.sprintf("Multiple version values");
      return false;
    }
    if (arg.type == das::Type::tInt)
    {
      G_ASSERT(uint32_t(arg.iValue) < EASTL_LIMITS_MAX_U(uint16_t));
      res = uint16_t(arg.iValue);
      found = true;
      continue;
    }
    err.append("Invalid version value, expected int");
    return false;
  }
  return true;
}

static bool resolve_reliability(const das::AnnotationArgumentList &args, PacketReliability &res, bool &found, das::string &err)
{
  res = RELIABLE_ORDERED;
  found = false;
  for (const das::AnnotationArgument &arg : args)
  {
    if (arg.name != "reliability")
      continue;
    if (found)
    {
      err.sprintf("Multiple reliability values");
      return false;
    }
    if (arg.type == das::Type::tString)
    {
      if (arg.sValue == "UNRELIABLE")
      {
        res = UNRELIABLE;
        found = true;
        continue;
      }
      if (arg.sValue == "UNRELIABLE_SEQUENCED")
      {
        res = UNRELIABLE_SEQUENCED;
        found = true;
        continue;
      }
      if (arg.sValue == "RELIABLE_ORDERED")
      {
        res = RELIABLE_ORDERED;
        found = true;
        continue;
      }
      if (arg.sValue == "RELIABLE_UNORDERED")
      {
        res = RELIABLE_UNORDERED;
        found = true;
        continue;
      }
    }
    err.sprintf("Invalid reliability value, supported values reliability=UNRELIABLE or reliability=UNRELIABLE_SEQUENCED \
 or reliability=RELIABLE_ORDERED or reliability=RELIABLE_UNORDERED");
    return false;
  }
  return true;
}

static bool resolve_event_routing(const das::AnnotationArgumentList &args, uint8_t &res, das::string &err)
{
  res = DASEVENT_NO_ROUTING;
  bool found = false;
  for (const das::AnnotationArgument &arg : args)
  {
    if (arg.name != "routing")
      continue;
    if (found)
    {
      err.sprintf("Multiple routing values");
      return false;
    }
    if (arg.type == das::Type::tString)
    {
      if (arg.sValue == "ROUTING_CLIENT_TO_SERVER")
      {
        res = net::ROUTING_CLIENT_TO_SERVER;
        found = true;
        continue;
      }
      if (arg.sValue == "ROUTING_SERVER_TO_CLIENT")
      {
        res = net::ROUTING_SERVER_TO_CLIENT;
        found = true;
        continue;
      }
      if (arg.sValue == "ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER")
      {
        res = net::ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER;
        found = true;
        continue;
      }
    }
    err.sprintf("Invalid routing value, supported values routing=ROUTING_CLIENT_TO_SERVER or routing=ROUTING_SERVER_TO_CLIENT \
 or routing=ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER");
    return false;
  }
  return true;
}

static bool is_BitStream(const das::TypeDeclPtr &type)
{
  return type->isPointer() && type->firstType->isHandle() && type->firstType->annotation->module->name == "BitStream" &&
         type->firstType->annotation->name == "BitStream";
}

static bool is_ecs_type(const das::TypeDeclPtr &type)
{
  return type->isPointer() && type->firstType->isHandle() && type->firstType->annotation->module->name == "ecs";
}

static bool is_ecs_type_fast(const das::TypeDeclPtr &type, const char *name) { return type->firstType->annotation->name == name; }

static bool get_component_type(const das::TypeDeclPtr &type, const das::string &name, ecs::component_type_t &res, das::string &errors)
{
  if (type == nullptr)
  {
    errors.append_sprintf("Argument '%s' has unknown type", name.c_str());
    return false;
  }
  switch (type->baseType)
  {
    case das::Type::tBool: res = ecs::ComponentTypeInfo<bool>::type; return true;

    case das::Type::tFloat: res = ecs::ComponentTypeInfo<float>::type; return true;
    case das::Type::tFloat2: res = ecs::ComponentTypeInfo<Point2>::type; return true;
    case das::Type::tFloat3: res = ecs::ComponentTypeInfo<Point3>::type; return true;
    case das::Type::tFloat4: res = ecs::ComponentTypeInfo<Point4>::type; return true;

    case das::Type::tInt:
    case das::Type::tEnumeration: res = ecs::ComponentTypeInfo<int32_t>::type; return true;
    case das::Type::tInt8:
    case das::Type::tEnumeration8: res = ecs::ComponentTypeInfo<int8_t>::type; return true;
    case das::Type::tInt16:
    case das::Type::tEnumeration16: res = ecs::ComponentTypeInfo<int16_t>::type; return true;
    case das::Type::tInt64: res = ecs::ComponentTypeInfo<int64_t>::type; return true;
    case das::Type::tInt2: res = ecs::ComponentTypeInfo<IPoint2>::type; return true;
    case das::Type::tInt3: res = ecs::ComponentTypeInfo<IPoint3>::type; return true;
    case das::Type::tInt4: res = ecs::ComponentTypeInfo<IPoint4>::type; return true;

    case das::Type::tUInt: res = ecs::ComponentTypeInfo<uint32_t>::type; return true;
    case das::Type::tUInt8: res = ecs::ComponentTypeInfo<uint8_t>::type; return true;
    case das::Type::tUInt16: res = ecs::ComponentTypeInfo<uint16_t>::type; return true;
    case das::Type::tUInt64: res = ecs::ComponentTypeInfo<uint64_t>::type; return true;

    case das::Type::tString: res = ecs::ComponentTypeInfo<eastl::string>::type; return true;

    case das::Type::tPointer:
      if (is_BitStream(type))
      {
        res = ECS_HASH("danet::BitStream").hash;
        return true;
      }
      if (is_ecs_type(type))
      {
#define DECL_LIST_TYPE(t, tn)              \
  if (is_ecs_type_fast(type, tn))          \
  {                                        \
    res = ecs::ComponentTypeInfo<t>::type; \
    return true;                           \
  }

        DAS_EVENT_ECS_CONT_TYPES
#undef DECL_LIST_TYPE
      }
      break;

    case das::Type::tHandle:
      if (type->annotation && type->annotation->module)
      {
        if (type->annotation->module->name == "ecs" && type->annotation->name == "EntityId")
        {
          res = ecs::ComponentTypeInfo<ecs::EntityId>::type;
          return true;
        }
        if (type->annotation->module->name == "math" && type->annotation->name == "float3x4")
        {
          res = ecs::ComponentTypeInfo<TMatrix>::type;
          return true;
        }
        if (type->annotation->module->name == "DagorMath" && type->annotation->name == "E3DCOLOR")
        {
          res = ecs::ComponentTypeInfo<E3DCOLOR>::type;
          return true;
        }
      }
      break;

    default:;
  }

  const eastl::string typeName = type->describe();
  errors.append_sprintf("Argument '%s' has unsupported type '%s'. Only base ecs types should to be supported", name.c_str(),
    typeName.c_str());
  return false;
// HINT: add getUserHash to TypeAnnotation
// virtual uint64_t getUserHash() const { string s = name + "_" + cppName; return hash_blockz32((uint8_t *)s.c_str()); }
// and override it in base type as
// override uint64_t getUserHash() const { return (uint64_t)ecs::ComponentTypeInfo<ManagedType>::type; }
#if 0
  das::string typeName;
  das::Type resType;
  get_underlying_ecs_type(*type.get(), typeName, resType);
  return ECS_HASH_SLOW(typeName.c_str()).hash;
#endif
}

static bool calc_struct_hash(const das::StructurePtr &st, uint16_t evt_version,
  eastl::vector<ecs::component_type_t, framemem_allocator> &components, ecs::EventsDB::event_scheme_hash_t &hash, das::string &errors)
{
  hash = fnv1a_step<32>(evt_version, hash);
  hash = fnv1a_step<32>(st->fields.size());

  int skip = SKIP_FIRST_EVENT_FIELDS;
  for (auto &field : st->fields)
  {
    if (--skip >= 0)
      continue;
    ecs::component_type_t type;
    if (!get_component_type(field.type, field.name, type, errors))
      return false;
    components.push_back(type);
    hash = fnv1a_step<32>(type, hash);
  }
  return true;
}

static void calc_struct_scheme(const das::StructurePtr &st, eastl::vector<ecs::component_type_t, framemem_allocator> &components,
  ecs::event_scheme_t &event_scheme)
{
  int skip = SKIP_FIRST_EVENT_FIELDS;
  int i = 0;
  for (auto &field : st->fields)
  {
    if (--skip >= 0)
      continue;
    event_scheme.push_back(field.name, components[i], (uint8_t)field.offset);
    ++i;
  }
}

static bool resolve_net_liable(const das::AnnotationArgumentList &args, DascriptEventDesc::NetLiable &res, bool &found,
  das::string &err)
{
  res = DascriptEventDesc::NetLiable::Strict;
  found = false;
  for (const das::AnnotationArgument &arg : args)
  {
    if (arg.name != "net_liable")
      continue;
    if (found)
    {
      err.sprintf("Multiple net_liable values");
      return false;
    }
    if (arg.type == das::Type::tString)
    {
      if (arg.sValue == "strict")
      {
        res = DascriptEventDesc::NetLiable::Strict;
        found = true;
        continue;
      }
      if (arg.sValue == "logerr")
      {
        res = DascriptEventDesc::NetLiable::Logerr;
        found = true;
        continue;
      }
      if (arg.sValue == "ignore")
      {
        res = DascriptEventDesc::NetLiable::Ignore;
        found = true;
        continue;
      }
    }
    err.sprintf("Invalid net_liable value, supported values net_liable=strict or net_liable=logerr or net_liable=ignore");
    return false;
  }
  return true;
}

struct EventRegistrator final : das::StructureAnnotation
{
  EventRegistrator() : das::StructureAnnotation("event") {}
  bool touch(const das::StructurePtr &st, das::ModuleGroup & /*libGroup*/, const das::AnnotationArgumentList & /*args*/,
    das::string & /*err*/) override
  {
    return common_touch(st);
  }

  using EventOffsets = eastl::tuple_vector<ecs::component_type_t, uint16_t>;
  static ska::flat_hash_map<ecs::event_type_t, EventOffsets, ska::power_of_two_std_hash<ecs::EventsDB::event_id_t>> events_offsets;

  static void destroy(ecs::Event &e)
  {
    const auto it = events_offsets.find(e.getType());
    if (it != events_offsets.end())
    {
      const eastl_size_t size = it->second.size();
      if (size > 0)
      {
        char *s = (char *)&e;
        const ecs::component_type_t *types = it->second.get<ecs::component_type_t>();
        const uint16_t *offsets = it->second.get<uint16_t>();
        for (eastl_size_t i = 0; i < size; ++i)
        {
          if (types[i] == ecs::ComponentTypeInfo<ecs::string>::type)
            memfree_anywhere(*(char **)(s + offsets[i]));
          else if (types[i] == ECS_HASH("danet::BitStream").hash)
            delete *(danet::BitStream **)(s + offsets[i]);

#define DECL_LIST_TYPE(t, tn) else if (types[i] == ecs::ComponentTypeInfo<t>::type) delete *(t **)(s + offsets[i]);

          DAS_EVENT_ECS_CONT_TYPES
#undef DECL_LIST_TYPE

          else logerr(
            "Internal error. Das event field with type '%s' was added without destruction support. Please fix it right here.",
            g_entity_mgr->getComponentTypes().findTypeName(types[i]));
        }
      }
    }
    else
      logerr("Non pod event is unregistered 0x%X. we can't be here, internal error", e.getType());
  }

  static void move_out(void *__restrict allocateAt, ecs::Event &&from)
  {
    memcpy(allocateAt, &from, from.getLength());
    memset(&from, 0, sizeof(ecs::Event)); // enough to clear header, as type information is now lost
  }

  static bool isValidStruct(const das::StructurePtr &st, das::string &err)
  {
    if (st->isRawPod())
      return true;
    for (auto &field : st->fields)
    {
      if (field.type->isRawPod() || field.type->isString())
        continue;

      if (is_BitStream(field.type))
        continue;

      if (is_ecs_type(field.type))
      {
#define DECL_LIST_TYPE(t, tn)           \
  if (is_ecs_type_fast(field.type, tn)) \
    continue;

        DAS_EVENT_ECS_CONT_TYPES
#undef DECL_LIST_TYPE
      }

      err.append_sprintf("currently it is not possible to send events with member type '%s'", field.type->describe().c_str());
      return false;
    }
    return true;
  }

  bool look(const das::StructurePtr &st, das::ModuleGroup & /*libGroup*/, const das::AnnotationArgumentList &args, das::string &err)
  {
    if (!isValidStruct(st, err))
      return false;

    const size_t sz = st->getSizeOf();
    if (sz >= ecs::Event::max_event_size)
    {
      err.sprintf("size of event is too big %d, maximum possible is %d", sz, ecs::Event::max_event_size);
      return false;
    }
    if (sz < sizeof(ecs::Event))
    {
      err.sprintf("size of event is too small %d. we can't be here, internal error", sz);
      return false;
    }
    uint8_t routing;
    if (!resolve_event_routing(args, routing, err))
      return false;

    uint16_t version;
    bool versionFound = false;
    if (!resolve_event_version(args, version, versionFound, err))
      return false;

    eastl::vector<ecs::component_type_t, framemem_allocator> components;
    components.reserve(st->fields.size() - SKIP_FIRST_EVENT_FIELDS);
    ecs::EventsDB::event_scheme_hash_t structHash;
    if (!calc_struct_hash(st, version, components, structHash, err))
      return false;

    if (!versionFound)
    {
      for (const ecs::component_type_t compType : components)
        if (compType == ECS_HASH("danet::BitStream").hash)
        {
          err.append_sprintf("Events with BitStream fields requires explicitly event version. "
                             " Add version to annotation [event(... version=0 ...)]");
          return false;
        }
    }

    const ecs::event_flags_t castFlag = resolve_cast_type(args);
    const ecs::string eventName = st->describe();
    const ecs::event_type_t eventType = ECS_HASH_SLOW(eventName.c_str()).hash;
    if (castFlag == ecs::EVCAST_UNKNOWN || castFlag == ecs::EVCAST_BOTH)
    {
      err.append_sprintf("event <%s|0x%X> has %s. Specialize unicast or broadcast in parentheses: [event(unicast)]", eventName.c_str(),
        eventType, castFlag == ecs::EVCAST_UNKNOWN ? "UNKNOWN cast flag" : "unicast and broadcast flags");
      return false;
    }

    DascriptEventDesc::NetLiable netLiable;
    bool netLiableFound = false;
    if (!resolve_net_liable(args, netLiable, netLiableFound, err))
      return false;

    PacketReliability reliability;
    bool reliabilityFound = false;
    if (!resolve_reliability(args, reliability, reliabilityFound, err))
      return false;

    if (routing == DASEVENT_NO_ROUTING)
    {
      if (netLiableFound)
      {
        err.append_sprintf("net_liable can be used only with net events. Add routing param or remove net_liable");
        return false;
      }
      if (reliabilityFound)
      {
        err.append_sprintf("reliability can be used only with net events. Add routing param or remove reliability");
        return false;
      }
    }

    das::lock_guard<das::recursive_mutex> guard(DasScripts<LoadedScript, EsContext>::mutex);

    bool regNow = true;
    if (g_entity_mgr)
    {
      const ecs::EventsDB::event_id_t evtId = g_entity_mgr->getEventsDb().findEvent(eventType);
      const bool isRawPod = st->isRawPod();
      const ecs::event_flags_t evtFlags = castFlag | (isRawPod ? 0 : ecs::EVFLG_DESTROY);

      const ecs::EventsDB::event_scheme_hash_t prevHash = evtId == ecs::EventsDB::invalid_event_id
                                                            ? ecs::EventsDB::invalid_event_scheme_hash
                                                            : g_entity_mgr->getEventsDb().getEventSchemeHash(evtId);

      if (lockedNetEventsRegistration && netLiable == DascriptEventDesc::NetLiable::Strict && routing != DASEVENT_NO_ROUTING &&
          prevHash != structHash)
      {
        logerr("das_net: registration of a new dasevent '%s' <0x%X> when connection is already inited"
               " and the protocol version has beed synced.",
          g_entity_mgr->getEventsDb().findEventName(eventType), eventType);
      }

      if (evtId != ecs::EventsDB::invalid_event_id)
      {
        const ecs::event_flags_t dbEventFlag = g_entity_mgr->getEventsDb().getEventFlags(evtId);
        if (eventName != g_entity_mgr->getEventsDb().getEventName(evtId))
          logerr("hash collision for event 0x%X (%s != %s)", eventType, eventName.c_str(),
            g_entity_mgr->getEventsDb().getEventName(evtId));
        else if (g_entity_mgr->getEventsDb().getEventSize(evtId) != sz)
        {
          logerr("event <%s|0x%X> has changed it's size %d!=%d(new)", eventName.c_str(), eventType,
            g_entity_mgr->getEventsDb().getEventSize(evtId), sz);
        }
        else if (dbEventFlag != evtFlags)
        {
          logerr("event <%s|0x%X> has changed it's flags %d!=%d(new). Fix annotation to [event(%s%s%s)]", eventName.c_str(), eventType,
            dbEventFlag, evtFlags, dbEventFlag & ecs::EVCAST_UNICAST ? "unicast" : "",
            (dbEventFlag & ecs::EVCAST_BOTH) == ecs::EVCAST_BOTH ? "," : "", dbEventFlag & ecs::EVCAST_BROADCAST ? "broadcast" : "");
        }
        else
          regNow = false;
      }
      if (regNow)
      {
        if (!isRawPod)
        {
          EventOffsets offsets;
          for (auto &field : st->fields)
          {
            if (field.type->isString())
            {
              offsets.push_back(ecs::ComponentTypeInfo<ecs::string>::type, (uint16_t)field.offset);
              continue;
            }
            if (is_BitStream(field.type))
            {
              offsets.push_back(ECS_HASH("danet::BitStream").hash, (uint16_t)field.offset);
              continue;
            }
            if (is_ecs_type(field.type))
            {
#define DECL_LIST_TYPE(t, tn)                                                   \
  if (is_ecs_type_fast(field.type, tn))                                         \
  {                                                                             \
    offsets.push_back(ecs::ComponentTypeInfo<t>::type, (uint16_t)field.offset); \
    continue;                                                                   \
  }

              DAS_EVENT_ECS_CONT_TYPES
#undef DECL_LIST_TYPE
            }
          }
          events_offsets.emplace(eventType, eastl::move(offsets));
        }
        g_entity_mgr->getEventsDbMutable().registerEvent(eventType, sz, evtFlags, eventName.c_str(),
          isRawPod ? nullptr : &EventRegistrator::destroy, isRawPod ? nullptr : &EventRegistrator::move_out);
      }

      if (regNow || prevHash != structHash)
      {
        if (prevHash != ecs::EventsDB::invalid_event_scheme_hash && prevHash != structHash)
          logerr("event <%s|0x%X> has changed it's hash 0x%X!=0x%X(new)", eventName.c_str(), eventType, prevHash, structHash);
        ecs::event_scheme_t eventScheme;
        calc_struct_scheme(st, components, eventScheme);
        bool registeredScheme =
          g_entity_mgr->getEventsDbMutable().registerEventScheme(eventType, structHash, eastl::move(eventScheme));
        if (!registeredScheme)
          logerr("das_net: Unable to register event scheme for '%s' <0X%X>", eventName.c_str(), eventType);
      }
    }

    const auto desc = dascript_net_events.find(eventType);
    if (regNow || desc == dascript_net_events.end() || desc->second.routing != routing || desc->second.netLiable != netLiable ||
        desc->second.reliability != reliability || desc->second.version != version)
    {
      if (desc != dascript_net_events.end())
      {
        if (desc->second.routing != routing)
          logerr("event <%s|0x%X> routing changed to %d (was %d)", eventName, eventType, routing, desc->second.routing);
        if (desc->second.netLiable != netLiable)
          logwarn("event <%s|0x%X> netLiable changed to %d (was %d)", eventName, eventType, (uint8_t)netLiable,
            (uint8_t)desc->second.netLiable);
        if (desc->second.reliability != reliability)
          logwarn("event <%s|0x%X> reliability changed to %d (was %d)", eventName, eventType, reliability, desc->second.reliability);
        if (desc->second.version != version)
          logwarn("event <%s|0x%X> version changed to %d (was %d)", eventName, eventType, version, desc->second.version);
      }

      if (!(das::is_in_aot() || das::is_in_completion()))
        debug("das_net: register net event '%s' <0x%X> version=%@ size=%@ fields=%d routing=%d netLiable=%d hash=0x%X", eventName,
          eventType, version, sz, st->fields.size() - SKIP_FIRST_EVENT_FIELDS, routing, (uint8_t)netLiable, structHash);
      dascript_net_events[eventType] = DascriptEventDesc(version, castFlag, routing, netLiable, RELIABLE_ORDERED);
      ++dasEventsGeneration;
    }
    return true;
  }
};

ska::flat_hash_map<ecs::event_type_t, EventRegistrator::EventOffsets, ska::power_of_two_std_hash<ecs::EventsDB::event_id_t>>
  EventRegistrator::events_offsets = {};

struct CppEventRegistrator final : das::StructureAnnotation
{
  CppEventRegistrator() : das::StructureAnnotation("cpp_event") {}
  bool touch(const das::StructurePtr &st, das::ModuleGroup & /*libGroup*/, const das::AnnotationArgumentList & /*args*/,
    das::string & /*err*/) override
  {
    return common_touch(st);
  }
  bool look(const das::StructurePtr &st, das::ModuleGroup & /*libGroup*/, const das::AnnotationArgumentList &args,
    das::string &err) override
  {
    const ecs::event_flags_t castFlag = resolve_cast_type(args);
    auto cppNameIt = args.find("name", das::Type::tString);
    const eastl::string eventName = cppNameIt ? cppNameIt->sValue.c_str() : st->describe().c_str();
    const ecs::event_type_t eventType = ECS_HASH_SLOW(eventName.c_str()).hash;
    if (castFlag == ecs::EVCAST_UNKNOWN || castFlag == ecs::EVCAST_BOTH)
    {
      err.append_sprintf("event <%s|0x%X> has %s. Specialize unicast or broadcast in parentheses: [event(unicast)]", eventName.c_str(),
        eventType, castFlag == ecs::EVCAST_UNKNOWN ? "UNKNOWN cast flag" : "unicast and broadcast flags");
      return false;
    }
    const uint16_t version = 0;
    const bool withScheme = args.getBoolOption("with_scheme");
    eastl::vector<ecs::component_type_t, framemem_allocator> components;
    ecs::EventsDB::event_scheme_hash_t structHash = ecs::EventsDB::invalid_event_scheme_hash;
    if (withScheme)
    {
      components.reserve(st->fields.size() - SKIP_FIRST_EVENT_FIELDS);
      if (!calc_struct_hash(st, version, components, structHash, err))
        return false;
    }

    das::lock_guard<das::recursive_mutex> guard(bind_dascript::DasScripts<LoadedScript, EsContext>::mutex);

    if (g_entity_mgr)
    {
      ecs::EventsDB::event_id_t evtId = g_entity_mgr->getEventsDb().findEvent(eventType);
      if (evtId == ecs::EventsDB::invalid_event_id)
        logwarn("Event <%s> required in script, isn't registered in C++", eventName.c_str());
      else
      {
        if (strcmp(g_entity_mgr->getEventsDb().getEventName(evtId), eventName.c_str()) != 0)
        {
          err.append_sprintf("Event <%s> required in script has collision with <%s>, hash = 0x%X", eventName.c_str(),
            g_entity_mgr->getEventsDb().getEventName(evtId), eventType);
          return false;
        }
        if (g_entity_mgr->getEventsDb().getEventSize(evtId) != st->getSizeOf())
        {
          err.append_sprintf("Event <%s|0x%X> required in script has different size(=%d) than in C++ (=%d)!", eventName.c_str(),
            eventType, st->getSizeOf(), g_entity_mgr->getEventsDb().getEventSize(evtId));
          return false;
        }
        const ecs::event_flags_t dbEventFlag = g_entity_mgr->getEventsDb().getEventFlags(evtId);
        if (castFlag != (dbEventFlag & ecs::EVFLG_CASTMASK))
        {
          err.append_sprintf("Event <%s|0x%X> required in script has different cast flags(=%d) than in C++ (=%d)!. Fix annotation to "
                             "[cpp_event(%s%s%s)]",
            eventName.c_str(), eventType, castFlag, (dbEventFlag & ecs::EVFLG_CASTMASK),
            dbEventFlag & ecs::EVCAST_UNICAST ? "unicast" : "", (dbEventFlag & ecs::EVCAST_BOTH) == ecs::EVCAST_BOTH ? "," : "",
            dbEventFlag & ecs::EVCAST_BROADCAST ? "broadcast" : "");
          return false;
        }
        // debug("cpp event <%s> is registered for handling in daScript", eventName.c_str());
      }

      if (withScheme)
      {
        const ecs::EventsDB::event_scheme_hash_t prevHash = evtId == ecs::EventsDB::invalid_event_id
                                                              ? ecs::EventsDB::invalid_event_scheme_hash
                                                              : g_entity_mgr->getEventsDb().getEventSchemeHash(evtId);

        if (prevHash != structHash)
        {
          if (prevHash != ecs::EventsDB::invalid_event_scheme_hash)
            logerr("event <%s|0x%X> has changed it's hash 0x%X!=0x%X(new)", eventName.c_str(), eventType, prevHash, structHash);
          ecs::event_scheme_t eventScheme;
          calc_struct_scheme(st, components, eventScheme);
          bool registeredScheme =
            g_entity_mgr->getEventsDbMutable().registerEventScheme(eventType, structHash, eastl::move(eventScheme));
          if (!registeredScheme)
            logerr("das_net: Unable to register cpp event scheme for '%s' <0X%X>", eventName.c_str(), eventType);
        }
      }
    }

    const uint8_t routing = DASEVENT_NATIVE_ROUTING;
    const DascriptEventDesc::NetLiable netLiable = DascriptEventDesc::NetLiable::Ignore;

    const auto desc = dascript_net_events.find(eventType);
    if (desc != dascript_net_events.end())
    {
      if (desc->second.routing != routing)
        logerr("event <%s|0x%X> routing changed to %d (was %d)", eventName, eventType, routing, desc->second.routing);
      if (desc->second.netLiable != netLiable)
        logwarn("event <%s|0x%X> netLiable changed to %d (was %d)", eventName, eventType, (uint8_t)netLiable,
          (uint8_t)desc->second.netLiable);
    }

    dascript_net_events[eventType] = DascriptEventDesc(version, castFlag, routing, netLiable, RELIABLE_ORDERED);
    return true;
  }
};

class EventDataWalker : public das::DataWalker
{
  bool canVisitHandle(char *, das::TypeInfo *ti) override
  {
    const das::TypeAnnotation *ann = ti->getAnnotation();
    if (ann->module && ann->module->name == "ecs" && ann->name == "Object") // stop visiting Objects to prevent double duplications for
                                                                            // strings
      return false;

    return true;
  }
  void String(char *&str) override { str = str_dup(str ? str : "", strmem); }
  void beforePtr(char *pa, das::TypeInfo *ti) override
  {
    const das::TypeAnnotation *ann = ti->firstType ? ti->firstType->getAnnotation() : nullptr;
    if (!ann || !ann->module)
      return;
    if (ann->module->name == "ecs")
    {
#define DECL_LIST_TYPE(t, tn)          \
  if (ann->name == tn)                 \
  {                                    \
    t *&obj = *(t **)pa;               \
    obj = obj ? new t(*obj) : new t(); \
    return;                            \
  }

      DAS_EVENT_ECS_CONT_TYPES
#undef DECL_LIST_TYPE
      return;
    }
    if (ann->module->name == "BitStream" && ann->name == "BitStream")
    {
      danet::BitStream *&obj = *(danet::BitStream **)pa;
      obj = obj ? new danet::BitStream(*obj) : new danet::BitStream();
      return;
    }
  }
};

vec4f _builtin_event_dup(das::Context &, das::SimNode_CallBase *call, vec4f *args)
{
  EventDataWalker walker;
  walker.walk(args[0], call->types[0]);
  return v_zero();
}

struct EventsDBAnnotation final : das::ManagedStructureAnnotation<ecs::EventsDB, false>
{
  EventsDBAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("EventsDB", ml) { cppName = " ::ecs::EventsDB"; }
};

void ECS::addEvents(das::ModuleLibrary &lib)
{
  das::addInterop<_builtin_event_dup, void, vec4f>(*this, lib, "_builtin_event_dup", das::SideEffects::modifyArgumentAndExternal,
    "bind_dascript::_builtin_event_dup");
  addAnnotation(das::make_smart<EventRegistrator>());
  addAnnotation(das::make_smart<CppEventRegistrator>());
  addAnnotation(das::make_smart<EventAnnotation>(lib));
  addAnnotation(das::make_smart<EventsDBAnnotation>(lib));

  das::addExtern<DAS_BIND_FUN(_builtin_send_event)>(*this, lib, "_builtin_send_event", das::SideEffects::modifyExternal,
    "bind_dascript::_builtin_send_event");
  das::addExtern<DAS_BIND_FUN(_builtin_broadcast_event)>(*this, lib, "_builtin_broadcast_event", das::SideEffects::modifyExternal,
    "bind_dascript::_builtin_broadcast_event");
  das::addInterop<_builtin_send_blobevent, void, ecs::EntityId, vec4f, char *>(*this, lib, "_builtin_send_blobevent",
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::_builtin_send_blobevent");
  das::addInterop<_builtin_broadcast_blobevent, void, vec4f, char *>(*this, lib, "_builtin_broadcast_blobevent",
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::_builtin_broadcast_blobevent");
  das::addInterop<_builtin_send_blobevent_immediate, void, ecs::EntityId, vec4f, char *>(*this, lib,
    "_builtin_send_blobevent_immediate", das::SideEffects::modifyArgumentAndExternal,
    "bind_dascript::_builtin_send_blobevent_immediate");
  das::addInterop<_builtin_broadcast_blobevent_immediate, void, vec4f, char *>(*this, lib, "_builtin_broadcast_blobevent_immediate",
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::_builtin_broadcast_blobevent_immediate");
  das::addExtern<DAS_BIND_FUN(send_schemeless)>(*this, lib, "send_schemeless_event", das::SideEffects::modifyExternal,
    "bind_dascript::send_schemeless");
  das::addExtern<DAS_BIND_FUN(broadcast_schemeless)>(*this, lib, "broadcast_schemeless_event", das::SideEffects::modifyExternal,
    "bind_dascript::broadcast_schemeless");

  das::addExtern<DAS_BIND_FUN(get_events_db), das::SimNode_ExtFuncCallRef>(*this, lib, "get_events_db",
    das::SideEffects::accessExternal, "bind_dascript::get_events_db");
  das::addExtern<DAS_BIND_FUN(get_events_db_rw), das::SimNode_ExtFuncCallRef>(*this, lib, "get_events_db_rw",
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::get_events_db_rw");

#define DECL_LIST_TYPE(t, tn) \
  das::addExtern<DAS_BIND_FUN(ecs_addr<t>)>(*this, lib, "ecs_addr", das::SideEffects::none, "bind_dascript::ecs_addr");

  DAS_EVENT_ECS_CONT_TYPES
#undef DECL_LIST_TYPE


  using method_findEvent = DAS_CALL_MEMBER(ecs::EventsDB::findEvent);
  das::addExtern<DAS_CALL_METHOD(method_findEvent)>(*this, lib, "events_db_findEvent", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::findEvent));

  using method_findEventName = DAS_CALL_MEMBER(ecs::EventsDB::findEventName);
  das::addExtern<DAS_CALL_METHOD(method_findEventName)>(*this, lib, "events_db_findEventName", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::findEventName));

  using method_getEventsCount = DAS_CALL_MEMBER(ecs::EventsDB::getEventsCount);
  das::addExtern<DAS_CALL_METHOD(method_getEventsCount)>(*this, lib, "events_db_getEventsCount", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::getEventsCount));

  using method_getEventName = DAS_CALL_MEMBER(ecs::EventsDB::getEventName);
  das::addExtern<DAS_CALL_METHOD(method_getEventName)>(*this, lib, "events_db_getEventName", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::getEventName));

  using method_getEventFlags = DAS_CALL_MEMBER(ecs::EventsDB::getEventFlags);
  das::addExtern<DAS_CALL_METHOD(method_getEventFlags)>(*this, lib, "events_db_getEventFlags", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::getEventFlags));

  using method_getEventSize = DAS_CALL_MEMBER(ecs::EventsDB::getEventSize);
  das::addExtern<DAS_CALL_METHOD(method_getEventSize)>(*this, lib, "events_db_getEventSize", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::getEventSize));

  using method_getEventType = DAS_CALL_MEMBER(ecs::EventsDB::getEventType);
  das::addExtern<DAS_CALL_METHOD(method_getEventType)>(*this, lib, "events_db_getEventType", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::getEventType));

  using method_hasEventScheme = DAS_CALL_MEMBER(ecs::EventsDB::hasEventScheme);
  das::addExtern<DAS_CALL_METHOD(method_hasEventScheme)>(*this, lib, "events_db_hasEventScheme", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::hasEventScheme));

  using method_getEventSchemeHash = DAS_CALL_MEMBER(ecs::EventsDB::getEventSchemeHash);
  das::addExtern<DAS_CALL_METHOD(method_getEventSchemeHash)>(*this, lib, "events_db_getEventSchemeHash", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::getEventSchemeHash));

  using method_getFieldsCount = DAS_CALL_MEMBER(ecs::EventsDB::getFieldsCount);
  das::addExtern<DAS_CALL_METHOD(method_getFieldsCount)>(*this, lib, "events_db_getFieldsCount", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::getFieldsCount));

  using method_getFieldName = DAS_CALL_MEMBER(ecs::EventsDB::getFieldName);
  das::addExtern<DAS_CALL_METHOD(method_getFieldName)>(*this, lib, "events_db_getFieldName", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::getFieldName));

  using method_getFieldOffset = DAS_CALL_MEMBER(ecs::EventsDB::getFieldOffset);
  das::addExtern<DAS_CALL_METHOD(method_getFieldOffset)>(*this, lib, "events_db_getFieldOffset", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::getFieldOffset));

  using method_getFieldType = DAS_CALL_MEMBER(ecs::EventsDB::getFieldType);
  das::addExtern<DAS_CALL_METHOD(method_getFieldType)>(*this, lib, "events_db_getFieldType", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::getFieldType));

  using method_findFieldIndex = DAS_CALL_MEMBER(ecs::EventsDB::findFieldIndex);
  das::addExtern<DAS_CALL_METHOD(method_findFieldIndex)>(*this, lib, "events_db_findFieldIndex", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::findFieldIndex));

  using method_getEventFieldRawValue = DAS_CALL_MEMBER(ecs::EventsDB::getEventFieldRawValue);
  das::addExtern<DAS_CALL_METHOD(method_getEventFieldRawValue)>(*this, lib, "events_db_getEventFieldRawValue", das::SideEffects::none,
    DAS_CALL_MEMBER_CPP(ecs::EventsDB::getEventFieldRawValue));

  das::addConstant(*this, "EVCAST_UNKNOWN", (ecs::event_flags_t)ecs::EVCAST_UNKNOWN);
  das::addConstant(*this, "EVCAST_UNICAST", (ecs::event_flags_t)ecs::EVCAST_UNICAST);
  das::addConstant(*this, "EVCAST_BROADCAST", (ecs::event_flags_t)ecs::EVCAST_BROADCAST);
  das::addConstant(*this, "EVCAST_BOTH", (ecs::event_flags_t)ecs::EVCAST_BOTH);
  das::addConstant(*this, "EVFLG_CASTMASK", (ecs::event_flags_t)ecs::EVFLG_CASTMASK);
  das::addConstant(*this, "EVFLG_SERIALIZE", (ecs::event_flags_t)ecs::EVFLG_SERIALIZE);
  das::addConstant(*this, "EVFLG_DESTROY", (ecs::event_flags_t)ecs::EVFLG_DESTROY);
  das::addConstant(*this, "EVFLG_SCHEMELESS", (ecs::event_flags_t)ecs::EVFLG_SCHEMELESS);
  das::addConstant(*this, "EVFLG_CORE", (ecs::event_flags_t)ecs::EVFLG_CORE);
}

} // namespace bind_dascript