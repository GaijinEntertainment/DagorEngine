//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "jsonUtils/jsonUtils.h"

#include <debug/dag_assert.h>
#include <EASTL/type_traits.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>


namespace jsonutils
{

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

DecodeJwtResult decode_jwt(const char *token, const char *publicKey, rapidjson::Document &headerJson,
  rapidjson::Document &payloadJson);
bool decode_jwt_block(const char *ptr, int size, rapidjson::Document &json, JwtCompressionType comprType = JwtCompressionType::NONE);
}; // namespace jsonutils
