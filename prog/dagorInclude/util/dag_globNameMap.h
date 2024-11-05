//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_oaHashNameMap.h>

static constexpr int NULL_NAME_ID = -1;

// "Smart" NameId template
template <class T>
class SmartNameId
{
public:
  SmartNameId() = default;
  SmartNameId(const SmartNameId &name_id) = default;
  SmartNameId &operator=(const SmartNameId &name_id) = default;

  SmartNameId(const char *name) { id = name ? T::nameMap.addNameId(name) : NULL_NAME_ID; }
  explicit SmartNameId(int _id) { id = _id; }
  SmartNameId &operator=(const char *name)
  {
    id = name ? T::nameMap.addNameId(name) : NULL_NAME_ID;
    return *this;
  }

  operator const char *() const { return T::nameMap.getName(id); }
  const char *getName() const { return T::nameMap.getName(id); }

  inline bool isNull() const { return id == NULL_NAME_ID; }
  inline int getId() const { return id; }

  static void shutdown() { T::nameMap.reset(false); }

protected:
  int id = NULL_NAME_ID;
};

#define DECLARE_SMART_NAMEID(NAME)       \
  struct NameMapHelper##NAME             \
  {                                      \
    static OAHashNameMap<false> nameMap; \
  };                                     \
  typedef SmartNameId<NameMapHelper##NAME> NAME

#define INSTANTIATE_SMART_NAMEID(NAME) OAHashNameMap<false> NameMapHelper##NAME::nameMap
