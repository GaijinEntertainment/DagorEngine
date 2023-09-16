#include <daScript/daScript.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>

#include <dasModules/aotStatsd.h>
#include <statsd/statsd.h>

namespace bind_dascript
{
class StatsdModule final : public das::Module
{
public:
  StatsdModule() : das::Module("statsd")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_statsd_counter<1>::statsd_counter)>(*this, lib, "statsd_counter",
      das::SideEffects::modifyExternal, "bind_dascript::get_statsd_counter<1>::statsd_counter");
    das::addExtern<DAS_BIND_FUN(bind_dascript::get_statsd_counter<2>::statsd_counter)>(*this, lib, "statsd_counter",
      das::SideEffects::modifyExternal, "bind_dascript::get_statsd_counter<2>::statsd_counter");
    das::addExtern<DAS_BIND_FUN(bind_dascript::get_statsd_counter<3>::statsd_counter)>(*this, lib, "statsd_counter",
      das::SideEffects::modifyExternal, "bind_dascript::get_statsd_counter<3>::statsd_counter");
    das::addExtern<DAS_BIND_FUN(bind_dascript::statsd_profile_float)>(*this, lib, "statsd_profile", das::SideEffects::modifyExternal,
      "bind_dascript::statsd_profile_float");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotStatsd.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(StatsdModule, bind_dascript);
