//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/utility/enumDescription.h>

namespace bind_dascript
{
inline bool get_ecs_enum_values(const char *type_name, const das::TBlock<void, const das::TTemporary<das::TArray<char *>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  auto values = ecs::get_ecs_enum_values(type_name);
  if (!values.empty())
  {
    das::Array arr;
    arr.data = (char *)values.data();
    arr.size = values.size();
    arr.capacity = values.size();
    arr.lock = 1;
    arr.flags = 0;
    vec4f arg = das::cast<das::Array *>::from(&arr);
    context->invoke(block, &arg, nullptr, at);
  }
  return !values.empty();
}

} // namespace bind_dascript
