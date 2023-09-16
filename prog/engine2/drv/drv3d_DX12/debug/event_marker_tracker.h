#pragma once

#include <EASTL/unordered_set.h>

namespace drv3d_dx12::debug::event_marker
{
namespace core
{
class Tracker
{
  static constexpr char separator = '/';

  using NameTableType = eastl::unordered_set<eastl::string>;
  struct EventPathEntry
  {
    const EventPathEntry *parent = nullptr;
    const eastl::string *name = nullptr;
    // We keep a persistent copy of the full path for systems that rely on this, like Nvidia Aftermath
    eastl::string fullPath;

    friend bool operator==(const EventPathEntry &l, const EventPathEntry &r) { return l.parent == r.parent && l.name == r.name; }
  };
  struct EventPathEntryHasher
  {
    size_t operator()(const EventPathEntry &e) const { return eastl::hash<eastl::string>{}(e.fullPath); }
  };
  using PathTableType = eastl::unordered_set<EventPathEntry, EventPathEntryHasher>;
  NameTableType nameTable;
  PathTableType pathTable;
  const EventPathEntry *currentPath = nullptr;
  const eastl::string *lastMarker = nullptr;

public:
  eastl::string_view beginEvent(eastl::string_view name)
  {
    auto stringRef = nameTable.emplace(name.data(), name.length()).first;

    EventPathEntry path;
    path.parent = currentPath;
    path.name = &*stringRef;
    if (currentPath)
    {
      path.fullPath.reserve(currentPath->fullPath.length() + 1 + name.length());
      path.fullPath.assign(begin(currentPath->fullPath), end(currentPath->fullPath));
      path.fullPath.push_back(separator);
      path.fullPath.append(begin(name), end(name));
    }
    else
    {
      path.fullPath.assign(begin(name), end(name));
    }

    auto pathRef = pathTable.insert(path).first;
    currentPath = &*pathRef;

    return {currentPath->name->data(), currentPath->name->length()};
  }

  void endEvent()
  {
    if (currentPath)
    {
      currentPath = currentPath->parent;
    }
  }

  eastl::string_view marker(eastl::string_view name)
  {
    lastMarker = &*nameTable.emplace(name.data(), name.length()).first;
    return {lastMarker->data(), lastMarker->length()};
  }

  eastl::string_view currentEventPath() const
  {
    if (currentPath)
    {
      return {currentPath->fullPath.data(), currentPath->fullPath.length()};
    }
    else
    {
      return {};
    }
  }

  eastl::string_view currentEvent() const
  {
    if (currentPath)
    {
      return {currentPath->name->data(), currentPath->name->length()};
    }
    else
    {
      return {};
    }
  }

  eastl::string_view currentMarker() const
  {
    if (lastMarker)
    {
      return {lastMarker->data(), lastMarker->length()};
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
public:
  eastl::string_view beginEvent(eastl::string_view name) { return name; }
  void endEvent() {}
  eastl::string_view marker(eastl::string_view name) { return name; }
  constexpr eastl::string_view currentEventPath() const { return {}; }
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
