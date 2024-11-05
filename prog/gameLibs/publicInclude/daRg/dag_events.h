//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace Sqrat
{
class Object;
}

namespace Json
{
class Value;
}

namespace darg
{
class IEventList
{
public:
  virtual bool sendEvent(const char *id, const Sqrat::Object &data) = 0;
  virtual void postEvent(const char *id, const Json::Value &data) = 0;
};
} // namespace darg
