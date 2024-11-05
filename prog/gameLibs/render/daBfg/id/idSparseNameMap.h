// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/optional.h>
#include <EASTL/string_view.h>
#include <util/dag_oaHashNameMap.h>
#include <dag/dag_vector.h>
#include <EASTL/span.h>

template <class EnumType>
struct IdSparseNameMap
{
  void add(const char *name, EnumType id)
  {
    auto idx = names.addNameId(name);
    if (idx >= ids.size())
      ids.emplace_back() = id;
  }

  EnumType get(const char *name) const
  {
    if (auto nameId = names.getNameId(name); nameId != -1)
      return ids[nameId];
    else
      return EnumType::Invalid;
  }

  eastl::span<const EnumType> getIds() const { return ids; }

  size_t size() const { return ids.size(); }

private:
  OAHashNameMap<false> names;
  dag::Vector<EnumType> ids;
};
