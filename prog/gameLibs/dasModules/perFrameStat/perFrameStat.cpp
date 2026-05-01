// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <perFrameStat/perFrameStat.h>

namespace bind_dascript
{
class PerFrameStat final : public das::Module
{
public:
  PerFrameStat() : das::Module("PerFrameStat")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(::benchmark_perframe_stat::gpu_time::init)>(*this, lib, "benchmark_perframe_stat_gpu_time_init",
      das::SideEffects::modifyExternal, "benchmark_perframe_stat::gpu_time::init");
    das::addExtern<DAS_BIND_FUN(::benchmark_perframe_stat::gpu_time::initialized)>(*this, lib,
      "benchmark_perframe_stat_gpu_time_initialized", das::SideEffects::accessExternal,
      "benchmark_perframe_stat::gpu_time::initialized");
    das::addExtern<DAS_BIND_FUN(::benchmark_perframe_stat::gpu_time::close)>(*this, lib, "benchmark_perframe_stat_gpu_time_close",
      das::SideEffects::modifyExternal, "benchmark_perframe_stat::gpu_time::close");

    das::addExtern<DAS_BIND_FUN(::benchmark_perframe_stat::init)>(*this, lib, "benchmark_perframe_stat_init",
      das::SideEffects::modifyExternal, "benchmark_perframe_stat::init");
    das::addExtern<DAS_BIND_FUN(::benchmark_perframe_stat::reset)>(*this, lib, "benchmark_perframe_stat_reset",
      das::SideEffects::modifyExternal, "benchmark_perframe_stat::reset");
    das::addExtern<DAS_BIND_FUN(::benchmark_perframe_stat::add_last_frame)>(*this, lib, "benchmark_perframe_stat_add_last_frame",
      das::SideEffects::modifyExternal, "benchmark_perframe_stat::add_last_frame");
    das::addExtern<DAS_BIND_FUN(::benchmark_perframe_stat::dump_to_file)>(*this, lib, "benchmark_perframe_stat_dump_to_file",
      das::SideEffects::modifyExternal, "benchmark_perframe_stat::dump_to_file");

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <perFrameStat/perFrameStat.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(PerFrameStat, bind_dascript);
