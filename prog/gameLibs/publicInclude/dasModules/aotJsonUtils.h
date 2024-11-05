//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <daScript/ast/ast_handle.h>
#include <jsonUtils/jsonUtils.h>
#include <jsonUtils/decodeJwt.h>

DAS_BIND_ENUM_CAST(jsonutils::DecodeJwtResult);
DAS_BIND_ENUM_CAST(jsonutils::JwtCompressionType);


namespace bind_dascript
{
// returns error message or empty/null string when everything is fine
inline char *load_json_from_file(const char *path, const das::TBlock<void, const rapidjson::Document> &block, // not temporary because
                                                                                                              // Document is not
                                                                                                              // copyable, movable
  das::Context *context, das::LineInfoArg *at)
{
  eastl::string errorMsg;
  jsonutils::OptionalJsonDocument maybeJson = jsonutils::load_json_from_file(path, &errorMsg);

  if (maybeJson)
  {
    vec4f arg = das::cast<rapidjson::Document &>::from(*maybeJson);
    context->invoke(block, &arg, nullptr, at);
    return nullptr; // no errors, empty/null line
  }
  return context->allocateString(errorMsg, at);
}

inline bool save_json_to_file(const rapidjson::Value &json, const char *path) { return jsonutils::save_json_to_file(json, path); }

inline jsonutils::DecodeJwtResult decode_jwt(const char *token, const char *publicKey, rapidjson::Document &payloadJson)
{
  rapidjson::Document headerJson;
  return jsonutils::decode_jwt(token, publicKey, headerJson, payloadJson);
}

} // namespace bind_dascript
