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


enum class DecodeJwtResult
{
  OK = 0,
  INVALID_TOKEN,
  WRONG_HEADER,
  INVALID_TOKEN_TYPE,
  INVALID_TOKEN_SIGNATURE_ALGORITHM,
  INVALID_PAYLOAD_COMPRESSION,
  WRONG_PAYLOAD,
  SIGNATURE_VERIFICATION_FAILED,

  NUM
};

enum class JwtCompressionType
{
  NONE,
  ZLIB,
  ZSTD
};

} // namespace jsonutils
