// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <json/value.h>
#include <EASTL/optional.h>

namespace local_storage
{

void init();
namespace hidden
{
void store_json(const char *key, Json::Value const &val);
Json::Value load_json(const char *key);
} // namespace hidden

} // namespace local_storage
