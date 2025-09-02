// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "varMap.h"
#include <generic/dag_tab.h>
#include <util/dag_bindump_ext.h>

namespace shc
{
class TargetContext;
}

struct VarMapAdapter
{
  static const char *getName(int id);
  static int addName(const char *name);
};

template <typename Adapter>
class NameId
{
  int mNameId = -1;

  void serialize() const
  {
    bindump::string name = Adapter::getName(mNameId);
    bindump::stream::write(name);
  }
  void deserialize()
  {
    bindump::string name;
    bindump::stream::read(name);
    mNameId = Adapter::addName(name.c_str());
  }
  void hashialize() const {}

public:
  BINDUMP_DECLARE_ASSIGNMENT_OPERATOR(NameId, mNameId = other.mNameId);
  operator int() const { return mNameId; }
  NameId &operator=(int id)
  {
    mNameId = id;
    return *this;
  }
};

template <typename Type>
class SerializableTab : public Tab<Type>
{
  void serialize() const
  {
    uint32_t count = this->size();
    bindump::stream::writeAux(count);
    if constexpr (eastl::is_pointer_v<Type>)
    {
      for (uint32_t i = 0; i < count; i++)
      {
        bool exist = (*this)[i] != nullptr;
        bindump::stream::writeAux(exist);
        if (exist)
          bindump::stream::write((*this)[i], 1);
      }
    }
    else
      bindump::stream::write(this->begin(), count);
  }
  void deserialize()
  {
    uint32_t count = 0;
    bindump::stream::readAux(count);
    this->resize(count);
    if constexpr (eastl::is_pointer_v<Type>)
    {
      for (uint32_t i = 0; i < count; i++)
      {
        (*this)[i] = nullptr;
        bool exist = false;
        bindump::stream::readAux(exist);
        if (!exist)
          continue;

        (*this)[i] = new eastl::remove_pointer_t<Type>();
        bindump::stream::read((*this)[i], 1);
      }
    }
    else
      bindump::stream::read(this->begin(), count);
  }
  void hashialize() const
  {
    if constexpr (eastl::is_pointer_v<Type>)
      bindump::stream::hash<eastl::remove_pointer_t<Type>>();
    else
      bindump::stream::hash<Type>();
  }

public:
  BINDUMP_ENABLE_STREAMING(SerializableTab, Tab<Type>, Tab);
  SerializableTab(const SerializableTab &other) = default;
  SerializableTab(SerializableTab &&other) = default;
  SerializableTab &operator=(SerializableTab &&other) = default;
};
