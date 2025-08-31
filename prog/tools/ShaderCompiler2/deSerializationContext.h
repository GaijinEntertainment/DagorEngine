// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// This thread local variable is a hacky way to provide context to logic run in operator= or serialization/deserialization when using
// the bindump library. Since we can't inject a regular context ref into deserialized objects, this is the only way to do this (and
// comes in handy for NameId class). This context is a thread-local pointer which has to be intialized for the scope of
// (de)serialization, and is asserted to be non-nested and present when called through the getter.

#include "shTargetContext.h"
#include <debug/dag_assert.h>

using ShadersDeSerializationContext = shc::TargetContext *;

ShadersDeSerializationContext get_shaders_de_serialization_ctx();

struct ShadersDeSerializationScope
{
  explicit ShadersDeSerializationScope(shc::TargetContext &ctx);
  ~ShadersDeSerializationScope();
};
