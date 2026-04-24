//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>


namespace jsonutils
{
using OptionalJsonDocument = eastl::optional<rapidjson::Document>;

OptionalJsonDocument load_json_from_file(const char *path, eastl::string /*out*/ *err_str = nullptr);

OptionalJsonDocument load_json_from_file(const char *path, const char *who, bool log_on_success = false);

bool save_json_to_file(const rapidjson::Value &json, const char *path, eastl::string /*out*/ *err_str = nullptr);

} // namespace jsonutils
