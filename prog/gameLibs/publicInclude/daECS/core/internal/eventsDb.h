#pragma once
#include <daECS/core/event.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/bonus/tuple_vector.h>
#include <daECS/core/componentType.h>

namespace ecs
{
typedef eastl::tuple_vector</*name*/ eastl::string, /*type*/ component_type_t, /*offset*/ uint8_t> event_scheme_t;

class EventsDB
{
public:
  typedef uint32_t event_id_t;
  typedef uint32_t event_scheme_hash_t;
  static constexpr event_id_t invalid_event_id = eastl::numeric_limits<event_id_t>::max();
  static constexpr event_scheme_hash_t invalid_event_scheme_hash = eastl::numeric_limits<event_scheme_hash_t>::max();
  void validate();
  const char *findEventName(event_type_t type) const;
  event_id_t findEvent(event_type_t type) const;
  bool registerEvent(event_type_t type, event_size_t sz, event_flags_t flag, const char *name, destroy_event *d, move_out_event *m);
  bool registerEventScheme(event_type_t type, event_scheme_hash_t scheme_hash, event_scheme_t &&scheme);
  event_flags_t getEventFlags(event_id_t) const;
  event_size_t getEventSize(event_id_t) const;
  const char *getEventName(event_id_t) const;
  event_type_t getEventType(event_id_t) const; // for inspection
  bool hasEventScheme(event_id_t) const;
  event_scheme_hash_t getEventSchemeHash(event_id_t) const;
  const event_scheme_t &getEventScheme(event_id_t) const;
  uint32_t getFieldsCount(event_id_t) const;
  const char *getFieldName(event_id_t, int) const;
  int findFieldIndex(event_id_t type, const char *field_name) const;
  uint8_t getFieldOffset(event_id_t, int) const;
  component_type_t getFieldType(event_id_t, int) const;
  void *getEventFieldRawValue(const ecs::Event &, event_id_t, int) const;
  template <class T, typename U = ECS_MAYBE_VALUE_T(T)>
  U getEventFieldValue(const ecs::Event &, event_id_t, int) const;

  uint32_t getEventsCount() const { return (uint32_t)eventsInfo.size(); }
  void dump() const;
  void destroy(EntityManager &mgr, Event &) const;
  void moveOut(EntityManager &mgr, void *__restrict to, Event &&from) const;

protected:
  friend class EntityManager;
  void validateInternal();
  uint32_t lastUpdatedGeneration = 0;
  void setEventFlags(event_id_t, event_flags_t);


  enum
  {
    EVENT_SIZE,
    EVENT_FLAGS,
    EVENT_TYPE,
    EVENT_NAME
  };
  ska::flat_hash_map<event_type_t, event_id_t, ska::power_of_two_std_hash<event_type_t>> eventsMap; // for faster getting names. Assume
                                                                                                    // unique hashes, no collision.
  eastl::tuple_vector<event_size_t, event_flags_t, event_type_t, eastl::string> eventsInfo;

  // we keep it as separate map, as there is ~1% of events that requires destruction/move
  // since first map is only used for inspection, there is both better chance of cache hit and less memory needed
  ska::flat_hash_map<event_type_t, destroy_event *, ska::power_of_two_std_hash<event_type_t>> eventsDestroyMap;
  // although move is same map, we very rarely call move at all (only when event is sent to loading entity)
  // so to increase cache locality for eventsDestroyMap
  ska::flat_hash_map<event_type_t, move_out_event *, ska::power_of_two_std_hash<event_type_t>> eventsMoveMap;
  eastl::tuple_vector<event_scheme_hash_t, event_scheme_t> eventsScheme;
};

inline void EventsDB::validate()
{
  if (EventInfoLinkedList::getGeneration() == lastUpdatedGeneration)
    return;
  validateInternal();
  lastUpdatedGeneration = EventInfoLinkedList::getGeneration();
}

inline EventsDB::event_id_t EventsDB::findEvent(event_type_t type) const
{
  auto it = eventsMap.find(type);
  return it == eventsMap.end() ? invalid_event_id : it->second;
}

inline event_size_t EventsDB::getEventSize(event_id_t id) const
{
  G_ASSERT_RETURN(eventsInfo.size() > id, 0);
  return eventsInfo.get<EVENT_SIZE>()[id];
}

inline const char *EventsDB::getEventName(event_id_t id) const
{
  G_ASSERT_RETURN(eventsInfo.size() > id, "");
  return eventsInfo.get<EVENT_NAME>()[id].c_str();
}

inline event_type_t EventsDB::getEventType(event_id_t id) const
{
  G_ASSERT_RETURN(eventsInfo.size() > id, 0);
  return eventsInfo.get<EVENT_TYPE>()[id];
}

inline bool EventsDB::hasEventScheme(event_id_t event_id) const
{
  G_ASSERT_RETURN(eventsScheme.size() > event_id, false);
  return eventsScheme.get<event_scheme_hash_t>()[event_id] != invalid_event_scheme_hash;
}

inline ecs::EventsDB::event_scheme_hash_t EventsDB::getEventSchemeHash(event_id_t event_id) const
{
  G_ASSERT_RETURN(eventsScheme.size() > event_id, invalid_event_scheme_hash);
  return eventsScheme.get<event_scheme_hash_t>()[event_id];
}

inline const event_scheme_t &EventsDB::getEventScheme(event_id_t event_id) const
{
  return eventsScheme.get<event_scheme_t>()[event_id];
}

inline uint32_t EventsDB::getFieldsCount(event_id_t event_id) const
{
  G_ASSERT_RETURN(eventsScheme.size() > event_id, 0);
  return (uint32_t)eventsScheme.get<event_scheme_t>()[event_id].size();
}

inline const char *EventsDB::getFieldName(event_id_t event_id, int idx) const
{
  G_ASSERT_RETURN(eventsScheme.size() > event_id, "");
  const event_scheme_t *schemes = eventsScheme.get<event_scheme_t>();
  const event_scheme_t &scheme = schemes[event_id];
  if ((uint32_t)idx < scheme.size())
    return scheme.get<eastl::string>()[idx].c_str();
  return nullptr;
}

inline int EventsDB::findFieldIndex(event_id_t event_id, const char *field_name) const
{
  G_ASSERT_RETURN(eventsScheme.size() > event_id, -1);
  const event_scheme_t *schemes = eventsScheme.get<event_scheme_t>();
  const event_scheme_t &scheme = schemes[event_id];
  const eastl::string *names = scheme.get<eastl::string>();
  for (int i = 0, n = (int)scheme.size(); i < n; ++i)
    if (names[i] == field_name)
      return i;
  return -1;
}

inline uint8_t EventsDB::getFieldOffset(event_id_t event_id, int idx) const
{
  G_ASSERT_RETURN(eventsScheme.size() > event_id, -1);
  const event_scheme_t *schemes = eventsScheme.get<event_scheme_t>();
  const event_scheme_t &scheme = schemes[event_id];
  return (uint32_t)idx < scheme.size() ? scheme.get<uint8_t>()[idx] : -1;
}

inline component_type_t EventsDB::getFieldType(event_id_t event_id, int idx) const
{
  G_ASSERT_RETURN(eventsScheme.size() > event_id, 0);
  const event_scheme_t *schemes = eventsScheme.get<event_scheme_t>();
  const event_scheme_t &scheme = schemes[event_id];
  return (uint32_t)idx < scheme.size() ? scheme.get<component_type_t>()[idx] : 0;
}

inline void *EventsDB::getEventFieldRawValue(const ecs::Event &evt, event_id_t event_id, int idx) const
{
  G_ASSERT_RETURN(eventsScheme.size() > event_id, nullptr);
  const event_scheme_t *schemes = eventsScheme.get<event_scheme_t>();
  const event_scheme_t &scheme = schemes[event_id];
  const uint8_t offset = scheme.get<uint8_t>()[idx];
  return ((char *)&evt) + offset;
}

template <class T, typename U>
U EventsDB::getEventFieldValue(const ecs::Event &evt, event_id_t event_id, int idx) const
{
  const event_scheme_t *schemes = eventsScheme.get<event_scheme_t>();
  const event_scheme_t &scheme = schemes[event_id];
  const uint8_t offset = scheme.get<uint8_t>()[idx];
  const component_type_t type = scheme.get<component_type_t>()[idx];
  G_UNUSED(type);
  G_FAST_ASSERT(type == ecs::ComponentTypeInfo<T>::type);
  return *(const T *)(((char *)&evt) + offset);
}

template <>
inline const char *EventsDB::getEventFieldValue<const char *, const char *>(const ecs::Event &evt, event_id_t event_id, int idx) const
{
  const event_scheme_t *schemes = eventsScheme.get<event_scheme_t>();
  const event_scheme_t &scheme = schemes[event_id];
  const uint8_t offset = scheme.get<uint8_t>()[idx];
  const component_type_t type = scheme.get<component_type_t>()[idx];
  G_UNUSED(type);
  G_FAST_ASSERT(type == ecs::ComponentTypeInfo<eastl::string>::type);
  return *(const char **)(((char *)&evt) + offset);
}

inline event_flags_t EventsDB::getEventFlags(event_id_t id) const
{
  G_ASSERT_RETURN(eventsInfo.size() > id, (event_flags_t)0);
  return eventsInfo.get<EVENT_FLAGS>()[id];
}

inline void EventsDB::setEventFlags(event_id_t id, event_flags_t f)
{
  G_ASSERT_RETURN(eventsInfo.size() > id, );
  eventsInfo.get<EVENT_FLAGS>()[id] = f;
}

inline const char *EventsDB::findEventName(event_type_t type) const
{
  event_id_t id = findEvent(type);
  return id == invalid_event_id ? nullptr : getEventName(id);
}

} // namespace ecs
