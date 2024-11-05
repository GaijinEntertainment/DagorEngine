// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dataBlockUtils/blk2sqrat/blk2sqrat.h>
#include <ioSys/dag_dataBlock.h>
#include <sqModules/sqModules.h>
#include <sqrat.h>
#include <squirrel.h>
#include <userSystemInfo/userSystemInfo.h>
#include <userSystemInfo/systemInfo.h>

static int get_thermal_status() { return int(systeminfo::get_thermal_state()); }

namespace bindquirrel
{
void bind_sysinfo(SqModules *module_mgr)
{
  Sqrat::Table nsTbl(module_mgr->getVM());
  ///@module sysinfo
  nsTbl //
    .Func("get_user_system_info",
      [module_mgr]() {
        DataBlock blk;
        get_user_system_info(&blk);
        return blk_to_sqrat(module_mgr->getVM(), blk);
      })
    ///@return DataBlock : datablock of user system info
    .Func("inline_raytracing_available", systeminfo::get_inline_raytracing_available)
    ///@return bool : returns true if inline raytracing is available on device, otherwise false
    .Func("get_battery", systeminfo::get_battery)
    ///@return float : return battery level from 0.0 to 1.0. When battery level unknown return -1.0
    .Func("is_charging", systeminfo::get_is_charging)
    ///@return bool : return "is battery charging"
    .Func("get_thermal_state", get_thermal_status)
    ///@return int : return capacity of battery or -1 if it is unavailable
    .Func("get_battery_capacity_mah", systeminfo::get_battery_capacity_mah)
    ///@return int : -1 -- Unknown, 0 -- Not connected, 1 -- Cellular, 2 -- Wi-Fi
    .Func("get_network_connection_type", systeminfo::get_network_connection_type)
    ///@return int : return thermal state. from TS_NORMAL to TS_SHUTDOWN
    .SetValue("TS_NORMAL", systeminfo::ThermalStatus::Normal)
    .SetValue("TS_LIGHT", systeminfo::ThermalStatus::Light)
    .SetValue("TS_MODERATE", systeminfo::ThermalStatus::Moderate)
    .SetValue("TS_SEVERE", systeminfo::ThermalStatus::Severe)
    .SetValue("TS_CRITICAL", systeminfo::ThermalStatus::Critical)
    .SetValue("TS_EMERGENCY", systeminfo::ThermalStatus::Emergency)
    .SetValue("TS_SHUTDOWN", systeminfo::ThermalStatus::Shutdown)
    /**/;

  module_mgr->addNativeModule("sysinfo", nsTbl);
}
} // namespace bindquirrel
