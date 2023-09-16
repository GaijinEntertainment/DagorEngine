//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
