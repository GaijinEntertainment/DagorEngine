// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "deSerializationContext.h"


namespace
{
thread_local ShadersDeSerializationContext g_ctx = {};
}

ShadersDeSerializationContext get_shaders_de_serialization_ctx()
{
  G_ASSERTF(g_ctx, "get_shaders_de_serialization_ctx() should only be called from operator= of (de)serializable structs during "
                   "serialization-deserialization. Make sure that the context is provided via ShaderDeSerializationScope");
  return g_ctx;
}

ShadersDeSerializationScope::ShadersDeSerializationScope(shc::TargetContext &ctx)
{
  G_ASSERTF(!g_ctx, "ShaderDeSerializationScope can not be nested");
  g_ctx = &ctx;
}

ShadersDeSerializationScope::~ShadersDeSerializationScope()
{
  G_ASSERT(g_ctx);
  g_ctx = nullptr;
}
