// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/net.h>

#include "net/netStat.h"
#include "dasModules/netstat.h"

DAS_BASE_BIND_ENUM_98(netstat::AggregationType, AggregationType, AG_SUM, AG_AVG, AG_MIN, AG_MAX, AG_CNT, AG_VAR, AG_LAST)

MAKE_TYPE_FACTORY(NetstatSample, netstat::Sample);

struct NetstatSampleAnnotation final : das::ManagedStructureAnnotation<netstat::Sample, false>
{
  NetstatSampleAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("NetstatSample", ml, "netstat::Sample")
  {
#define _NS(_1, var, ...)                      \
  addField<DAS_BIND_MANAGED_FIELD(var)>(#var); \
  DEF_NS_COUNTERS
#undef _NS
  }
  bool canBePlacedInContainer() const override { return true; } // To pass array of NetstatSample
};

namespace bind_dascript
{

class NetstatModule final : public das::Module
{
public:
  NetstatModule() : das::Module("netstat")
  {
    das::ModuleLibrary lib(this);

    addEnumeration(das::make_smart<EnumerationAggregationType>());
    addAnnotation(das::make_smart<NetstatSampleAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(netstat_get_aggregations)>(*this, lib, "netstat_get_aggregations", das::SideEffects::worstDefault,
      "bind_dascript::netstat_get_aggregations");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/netstat.h\"\n";
    tw << "#include \"net/netStat.h\"\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(NetstatModule, bind_dascript);
