// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>

#include "net/netStat.h"

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::netstat::AggregationType, AggregationType);

namespace bind_dascript
{

inline void netstat_get_aggregations(
  const das::TBlock<void, const das::TTemporary<das::TArray<netstat::Sample>>> &block, das::Context *context, das::LineInfoArg *at)
{
  dag::ConstSpan<netstat::Sample> aggregated_samples = netstat::get_aggregations();

  das::Array arr;
  das::array_mark_locked(arr, (char *)aggregated_samples.data(), aggregated_samples.size());
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

} // namespace bind_dascript
