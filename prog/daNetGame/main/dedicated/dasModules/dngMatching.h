// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <daScript/ast/ast_typedecl.h>
#include "net/dedicated/matching.h"
#include <daScriptModules/rapidjson/rapidjson.h>

namespace bind_dascript
{
inline void das_get_mode_info(const das::TBlock<void, const rapidjson::Document> &block, das::Context *context, das::LineInfoArg *at)
{
  const Json::Value &modeInfo = dedicated_matching::get_mode_info();
  if (!modeInfo.isObject())
    return;
  rapidjson::Document doc;
  eastl::string modeInfoStr = modeInfo.toStyledString();
  doc.Parse(modeInfoStr.c_str(), modeInfoStr.size());
  vec4f arg = das::cast<const rapidjson::Document *>::from(&doc);
  context->invoke(block, &arg, nullptr, at);
}


inline float das_get_mode_info_max_player_mult() { return dedicated_matching::get_mode_info().get("maxPlayersMult", 1.0).asFloat(); }

inline void das_get_mode_info_extra_params(
  const das::TBlock<void, const rapidjson::Document> &block, das::Context *context, das::LineInfoArg *at)
{
  Json::Value extraParams = dedicated_matching::get_mode_info().get("extraParams", Json::Value());
  if (!extraParams.isObject())
    return;
  rapidjson::Document doc;
  eastl::string extraParamsStr = extraParams.toStyledString();
  doc.Parse(extraParamsStr.c_str(), extraParamsStr.size());
  vec4f arg = das::cast<const rapidjson::Document *>::from(&doc);
  context->invoke(block, &arg, nullptr, at);
}

inline void das_get_session_players(
  const das::TBlock<void, const rapidjson::Document> &block, das::Context *context, das::LineInfoArg *at)
{
  rapidjson::Document jval(rapidjson::Type::kArrayType);
  rapidjson::Value::Array jarr = jval.GetArray();
  for (const auto &[_, info] : dedicated_matching::get_session_players())
  {
    rapidjson::Document doc(&jval.GetAllocator());
    eastl::string infoStr = info.toStyledString();
    doc.Parse(infoStr.c_str(), infoStr.size());
    jarr.PushBack(eastl::move(doc), jval.GetAllocator());
  }
  vec4f arg = das::cast<const rapidjson::Value *>::from(&jval);
  context->invoke(block, &arg, nullptr, at);
}

} // namespace bind_dascript
