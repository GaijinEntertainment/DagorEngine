//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <util/dag_base64.h>

namespace bind_dascript
{

inline const char *encode_base64(const char *text, das::Context *context, das::LineInfoArg *at)
{
  Base64 b64Coder;
  if (text)
    b64Coder.encode((const uint8_t *)text, strlen(text));
  else
    b64Coder.encode((const uint8_t *)"", strlen(""));
  return context->allocateString(b64Coder.c_str(), uint32_t(strlen(b64Coder.c_str())), at);
}

inline const char *decode_base64(const char *text, das::Context *context, das::LineInfoArg *at)
{
  Base64 b64Coder(text ? text : "");
  String result;
  b64Coder.decode(result);
  return context->allocateString(result.c_str(), uint32_t(result.size()), at);
}

} // namespace bind_dascript
