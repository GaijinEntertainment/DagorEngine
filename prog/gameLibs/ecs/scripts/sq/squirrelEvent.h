// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/schemelessEvent.h>

namespace ecs
{

namespace sq
{

struct SQEvent : public ecs::SchemelessEvent
{
private:
  static inline ecs::event_type_t getHCS(const Sqrat::Object &name)
  {
    G_ASSERTF(name.GetType() == OT_STRING || name.GetType() == OT_INTEGER,
      "SQEvent's first ctor parameter should be event name (string) or event hash");
    if (name.GetType() == OT_INTEGER)
      return sq_objtointeger(&const_cast<Sqrat::Object &>(name).GetObject());
    const char *cname = sq_objtostring(&const_cast<Sqrat::Object &>(name).GetObject());
#if DAGOR_DBGLEVEL > 0
    logerr("Quirrel sending non-registered event <%s|0x%X>", cname, ECS_HASH_SLOW(cname).hash);
#endif
    G_ASSERTF(cname && *cname, "SQEvent's name can't be empty");
    return ECS_HASH_SLOW(cname).hash;
  }

public:
  SQEvent(ecs::event_type_t tp, const Sqrat::Object &data_);
  SQEvent(const Sqrat::Object &ename) : ecs::SchemelessEvent(getHCS(ename)) // no need for destructor, no payload
  {}
  SQEvent(const ecs::HashedConstString &hstr, const Sqrat::Object &data_) : SQEvent(hstr.hash, data_) {}
  SQEvent(const Sqrat::Object &ename, const Sqrat::Object &data_) : SQEvent(getHCS(ename), data_) {}
};

}; // namespace sq

}; // namespace ecs
