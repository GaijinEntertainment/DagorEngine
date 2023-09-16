#pragma once

#include <util/dag_oaHashNameMap.h>
#include <EASTL/string_view.h>


template <class EnumType>
struct IdNameMap : OAHashNameMap<false>
{
  using Container = OAHashNameMap<false>;
  using Container::Container;

  EnumType addNameId(eastl::string_view view) { return static_cast<EnumType>(Container::addNameId(view.data(), view.size())); }

  EnumType getNameId(eastl::string_view view) const
  {
    auto result = Container::getNameId(view.data(), view.size());
    return result < 0 ? EnumType::Invalid : static_cast<EnumType>(result);
  }

  eastl::string_view getName(EnumType id) const { return getNameCstr(id); }

  const char *getNameCstr(EnumType id) const
  {
    auto result = Container::getName(static_cast<int>(eastl::to_underlying(id)));
    return result == nullptr ? "INVALID_NAME_ID" : result;
  }
};
