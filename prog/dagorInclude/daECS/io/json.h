//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/baseComponentTypes/objectType.h>

namespace Json
{
class Value;
}

namespace ecs
{
Object json_to_ecs_object(const Json::Value &json);
}
