//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_globDef.h>
#include <dag/dag_relocatable.h>

#define SQRAT_ASSERT  G_ASSERT
#define SQRAT_ASSERTF G_ASSERTF
#define SQRAT_VERIFY  G_VERIFY

#define SQRAT_HAS_SKA_HASH_MAP
#define SQRAT_HAS_EASTL

#define SQRAT_STD eastl


namespace Sqrat
{
class Object;
class Function;
} // namespace Sqrat

DAG_DECLARE_RELOCATABLE(Sqrat::Object);
DAG_DECLARE_RELOCATABLE(Sqrat::Function);

// Forward-declare Var<> specializations for Dagor types whose definitions live
// in separate headers (sqratDagor.h, etc.).  If user code instantiates one of
// these without including the definition header, the incomplete-type error
// points directly at the missing include instead of a cryptic runtime assert.
class SimpleString;
class String;
#define SQRAT_FORWARD_VAR_SPECIALIZATIONS \
  template <>                             \
  struct Var<SimpleString>;               \
  template <>                             \
  struct Var<const SimpleString &>;       \
  template <>                             \
  struct Var<String>;                     \
  template <>                             \
  struct Var<const String &>;
