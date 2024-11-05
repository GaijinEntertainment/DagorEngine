// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>

#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_oaHashNameMap.h>
#include <dag/dag_vector.h>


namespace drv3d_dx12::debug::event_marker
{
namespace core
{
class Tracker
{
  static constexpr char separator = '/';

  OAHashNameMap<false> nameTable;
  using FullPathIdx = uint32_t;
  using NameIdx = uint32_t;
  const uint32_t INVALID_IDX = ~0u;
  struct EventPathEntry
  {
    FullPathIdx parent;
    NameIdx name;
    friend bool operator==(const EventPathEntry &l, const EventPathEntry &r) { return l.parent == r.parent && l.name == r.name; }
  };
  dag::Vector<EventPathEntry> pathTable;
  dag::Vector<eastl::string> fullpathTable;
  eastl::string invalidPath{};

  struct EventPathEntryHasher
  {
    size_t operator()(const EventPathEntry &e) const { return eastl::hash<uintptr_t>{}((e.parent << 16) | e.name); }
  };

  ska::flat_hash_map<EventPathEntry, FullPathIdx, EventPathEntryHasher> pathLookup;
  FullPathIdx currentPath = INVALID_IDX;
  NameIdx lastMarker = INVALID_IDX;

public:
  eastl::string_view beginEvent(eastl::string_view name)
  {
    NameIdx nameId = nameTable.addNameId(name.data(), name.length());

    const EventPathEntry previousEntry = {currentPath, nameId};

    if (pathLookup.find(previousEntry) == pathLookup.end())
    {
      currentPath = pathTable.size();
      pathTable.push_back(previousEntry);
      pathLookup[previousEntry] = currentPath;
      if (previousEntry.parent != INVALID_IDX)
      {
        fullpathTable.resize(pathTable.size());
        fullpathTable.back().reserve(fullpathTable[previousEntry.parent].length() + 1 + name.length());
        fullpathTable.back().assign(fullpathTable[previousEntry.parent].data(), fullpathTable[previousEntry.parent].length());
        fullpathTable.back().push_back(separator);
      }
      else
      {
        fullpathTable.resize(pathTable.size());
        fullpathTable.back().assign(name.data(), name.length());
      }
    }
    else
    {
      currentPath = pathLookup[previousEntry];
    }

    return {nameTable.getName(nameId), name.length()};
  }

  void endEvent()
  {
    if (currentPath != INVALID_IDX)
    {
      currentPath = pathTable[currentPath].parent;
    }
  }

  eastl::string_view marker(eastl::string_view name)
  {
    lastMarker = nameTable.addNameId(name.data(), name.length());
    return {nameTable.getName(lastMarker), name.length()};
  }

  const eastl::string &currentEventPath() const
  {
    if (currentPath != INVALID_IDX)
    {
      return fullpathTable[currentPath];
    }
    else
    {
      return invalidPath;
    }
  }

  eastl::string_view currentEvent() const
  {
    if (currentPath != INVALID_IDX)
    {
      const uint32_t nameId = pathTable[currentPath].name;
      const char *eventName = nameTable.getName(nameId);
      if (!eventName)
        return {};
      return {eventName, strlen(eventName)};
    }
    else
    {
      return {};
    }
  }

  eastl::string_view currentMarker() const
  {
    if (lastMarker != INVALID_IDX)
    {
      const char *markerName = nameTable.getName(lastMarker);
      if (!markerName)
        return {};
      return {markerName, strlen(markerName)};
    }
    else
    {
      return {};
    }
  }

  constexpr bool isPersistent() const { return true; }
};
} // namespace core

namespace null
{
class Tracker
{
private:
  const eastl::string invalidPath{};

public:
  eastl::string_view beginEvent(eastl::string_view name) { return name; }
  void endEvent() {}
  eastl::string_view marker(eastl::string_view name) { return name; }
  const eastl::string &currentEventPath() const { return invalidPath; }
  constexpr eastl::string_view currentEvent() const { return {}; }
  constexpr eastl::string_view currentMarker() const { return {}; }
  constexpr bool isPersistent() const { return false; }
};
} // namespace null

#if DX12_TRACK_ACTIVE_DRAW_EVENTS
using namespace core;
#else
using namespace null;
#endif
} // namespace drv3d_dx12::debug::event_marker
