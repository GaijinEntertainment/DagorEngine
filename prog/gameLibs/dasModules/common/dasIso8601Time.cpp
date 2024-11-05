// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotDagorTime.h>

namespace bind_dascript
{

class DagorTime final : public das::Module
{
public:
  DagorTime() : das::Module("DagorTime")
  {
    das::ModuleLibrary lib(this);
    das::addExtern<DAS_BIND_FUN(iso8601_parse_sec)>(*this, lib, "iso8601_parse_sec", das::SideEffects::none,
      "bind_dascript::iso8601_parse_sec");
    das::addExtern<DAS_BIND_FUN(iso8601_parse_msec)>(*this, lib, "iso8601_parse_msec", das::SideEffects::none,
      "bind_dascript::iso8601_parse_msec");
    das::addExtern<DAS_BIND_FUN(iso8601_format_msec)>(*this, lib, "iso8601_format_msec", das::SideEffects::none,
      "bind_dascript::iso8601_format_msec");
    das::addExtern<DAS_BIND_FUN(iso8601_format_sec)>(*this, lib, "iso8601_format_sec", das::SideEffects::none,
      "bind_dascript::iso8601_format_sec");
    das::addExtern<DAS_BIND_FUN(get_time_msec)>(*this, lib, "get_time_msec", das::SideEffects::modifyExternal, "get_time_msec");
    das::addExtern<DAS_BIND_FUN(ref_time_ticks)>(*this, lib, "ref_time_ticks", das::SideEffects::modifyExternal, "ref_time_ticks");
    das::addExtern<DAS_BIND_FUN(get_time_usec)>(*this, lib, "get_time_usec", das::SideEffects::modifyExternal, "get_time_usec");
    das::addExtern<DAS_BIND_FUN(get_local_time)>(*this, lib, "get_local_time", das::SideEffects::modifyExternal,
      "bind_dascript::get_local_time");
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorTime.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DagorTime, bind_dascript);