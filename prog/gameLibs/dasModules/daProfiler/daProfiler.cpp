#include <folders/folders.h>
#include <osApiWrappers/basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_files.h>
#include <dasModules/aotEcs.h>
#include <daScript/daScript.h>
#include <daScript/simulate/simulate_nodes.h>
#include <daScript/src/builtin/module_builtin_rtti.h>
#include <daScript/ast/ast_interop.h>
#include <dasModules/aotDaProfiler.h>
#include <EASTL/vector_map.h>
#include <perfMon/dag_perfTimer.h>

namespace das
{
#if DAS_DEBUGGER && DA_PROFILER_ENABLED

struct DaProfilerSimNodeDebugInstrumentFunction : SimNodeDebug_InstrumentFunction
{
  DaProfilerSimNodeDebugInstrumentFunction(const das::LineInfo &at, das::SimFunction *simF, int64_t mnh, SimNode *se, uint64_t ud) :
    SimNodeDebug_InstrumentFunction(at, simF, mnh, se, ud)
  {}
  virtual vec4f DAS_EVAL_ABI eval(das::Context &context) override
  {
    vec4f res;
    DAS_PROFILE_NODE
    {
      da_profiler::ScopedEvent scope((da_profiler::desc_id_t)userData);
      res = subexpr->eval(context);
    }
    return res;
  }
#define EVAL_NODE(TYPE, CTYPE)                                          \
  virtual CTYPE eval##TYPE(Context &context) override                   \
  {                                                                     \
    DAS_PROFILE_NODE                                                    \
    CTYPE res;                                                          \
    {                                                                   \
      da_profiler::ScopedEvent scope((da_profiler::desc_id_t)userData); \
      res = subexpr->eval##TYPE(context);                               \
    }                                                                   \
    return res;                                                         \
  }
  DAS_EVAL_NODE
#undef EVAL_NODE
};

#endif // DAS_DEBUGGER && DA_PROFILER_ENABLED

} // namespace das

namespace bind_dascript
{

#if DAS_DEBUGGER && DA_PROFILER_ENABLED

void instrument_function(das::Context &ctx, das::SimFunction *FNPTR, bool isInstrumenting, uint64_t userData)
{
  auto instFn = [&](das::SimFunction *fun, uint64_t fnMnh) {
    if (!fun->code)
      return;
    if (isInstrumenting)
    {
      if (!fun->code->rtti_node_isInstrumentFunction())
      {
        fun->code =
          ctx.code->makeNode<das::DaProfilerSimNodeDebugInstrumentFunction>(fun->code->debugInfo, fun, fnMnh, fun->code, userData);
      }
    }
    else
    {
      if (fun->code->rtti_node_isInstrumentFunction())
      {
        auto inode = (das::SimNodeDebug_InstrumentFunction *)fun->code;
        fun->code = inode->subexpr;
      }
    }
  };
  if (FNPTR == nullptr)
  {
    for (int fni = 0, fnis = ctx.getTotalFunctions(); fni != fnis; ++fni)
    {
      das::SimFunction *fun = ctx.getFunction(fni);
      instFn(fun, fun->mangledNameHash);
    }
  }
  else
  {
    instFn(FNPTR, FNPTR->mangledNameHash);
  }
}

void daProfiler_resolve_path(const char *fname, const das::TBlock<void, das::TTemporary<const char *>> &blk, das::Context *context,
  das::LineInfoArg *at)
{
  String mountPath;
  if (fname && dd_resolve_named_mount(mountPath, fname))
    fname = mountPath.c_str();

  if (!fname)
  {
    vec4f args = das::cast<char *>::from(fname);
    context->invoke(blk, &args, nullptr, at);
    return;
  }

  String res(df_get_real_name(fname));
  if (res.empty())
  {
    // fallback logic
    auto daslibName = String(0, "%s/daslib/%s", das::getDasRoot().c_str(), fname);
    if (dd_file_exists(daslibName.c_str()))
    {
      res = das::move(daslibName);
    }
    else
    {
      auto builtinName = String(0, "%s/src/builtin/%s", das::getDasRoot(), fname);
      if (dd_file_exist(builtinName.c_str()))
        res = das::move(builtinName);
    }
  }
  if (!res.empty() && !is_path_abs(res.c_str()))
  {
    String t2(0, "%s../%s", folders::get_exe_dir(), res); // magic ../, works correct only on PC
    dd_simplify_fname_c(t2);
    if (const char *abs = df_get_real_name(t2.c_str()))
      res = abs;
  }
  vec4f args = das::cast<char *>::from(res.c_str());
  context->invoke(blk, &args, nullptr, at);
}

#else

void instrument_function(das::Context &, das::SimFunction *, bool, uint64_t) {}

void daProfiler_resolve_path(const char *fname, const das::TBlock<void, das::TTemporary<const char *>> &blk, das::Context *context,
  das::LineInfoArg *at)
{
  vec4f args = das::cast<char *>::from(fname);
  context->invoke(blk, &args, nullptr, at);
}

#endif // DAS_DEBUGGER && DA_PROFILER_ENABLED

void da_profiler_instrument_all_functions_ex(das::Context &ctx, const das::TBlock<uint64_t, das::Func, const das::SimFunction *> &blk,
  das::Context *context, das::LineInfoArg *arg)
{
  for (int fni = 0, fnis = ctx.getTotalFunctions(); fni != fnis; ++fni)
  {
    das::Func fn;
    fn.PTR = ctx.getFunction(fni);
    uint64_t userData = das::das_invoke<uint64_t>::invoke(context, arg, blk, fn, fn.PTR);
    instrument_function(ctx, fn.PTR, true, userData);
  }
}

class DaProfilerModule final : public das::Module
{
public:
  DaProfilerModule() : das::Module("daProfiler")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(da_profiler::get_tls_description)>(*this, lib, "get_tls_description", das::SideEffects::modifyExternal,
      "da_profiler::get_tls_description");

    das::addExtern<DAS_BIND_FUN(da_profiler::get_active_mode)>(*this, lib, "get_active_mode", das::SideEffects::accessExternal,
      "da_profiler::get_active_mode");

    das::addExtern<DAS_BIND_FUN(da_profiler::create_leaf_event)>(*this, lib, "create_leaf_event", das::SideEffects::modifyExternal,
      "da_profiler::create_leaf_event");

    das::addExtern<DAS_BIND_FUN(da_profiler::add_mode)>(*this, lib, "add_mode", das::SideEffects::modifyExternal,
      "da_profiler::add_mode");

    das::addExtern<DAS_BIND_FUN(da_profiler::remove_mode)>(*this, lib, "remove_mode", das::SideEffects::modifyExternal,
      "da_profiler::remove_mode");

    das::addExtern<DAS_BIND_FUN(da_profiler::get_current_mode)>(*this, lib, "get_current_mode", das::SideEffects::modifyExternal,
      "da_profiler::get_current_mode");

    das::addExtern<DAS_BIND_FUN(da_profiler::set_mode)>(*this, lib, "set_mode", das::SideEffects::modifyExternal,
      "da_profiler::set_mode");

    das::addExtern<DAS_BIND_FUN(da_profiler::set_continuous_limits)>(*this, lib, "set_continuous_limits",
      das::SideEffects::modifyExternal, "da_profiler::set_continuous_limits");

    das::addExtern<DAS_BIND_FUN(da_profiler::pause_sampling)>(*this, lib, "pause_sampling", das::SideEffects::modifyExternal,
      "da_profiler::pause_sampling");

    das::addExtern<DAS_BIND_FUN(da_profiler::resume_sampling)>(*this, lib, "resume_sampling", das::SideEffects::modifyExternal,
      "da_profiler::resume_sampling");

    das::addExtern<DAS_BIND_FUN(da_profiler::sync_stop_sampling)>(*this, lib, "sync_stop_sampling", das::SideEffects::modifyExternal,
      "da_profiler::sync_stop_sampling");

    das::addExtern<DAS_BIND_FUN(da_profiler::request_dump)>(*this, lib, "request_dump", das::SideEffects::modifyExternal,
      "da_profiler::request_dump");

    das::addExtern<DAS_BIND_FUN(profile_ref_ticks)>(*this, lib, "profile_ref_ticks", das::SideEffects::accessExternal,
      "::profile_ref_ticks");

    das::addExtern<DAS_BIND_FUN(daProfiler_resolve_path)>(*this, lib, "daProfiler_resolve_path", das::SideEffects::accessExternal,
      "::daProfiler_resolve_path");

    das::addExtern<DAS_BIND_FUN(bind_dascript::profile_block)>(*this, lib, "profile_block", das::SideEffects::modifyExternal,
      "bind_dascript::profile_block")
      ->args({"marker_name", "block_to_profile", "context", "at"});

    das::addExtern<DAS_BIND_FUN(bind_dascript::profile_gpu_block)>(*this, lib, "profile_gpu_block", das::SideEffects::modifyExternal,
      "bind_dascript::profile_gpu_block")
      ->args({"marker_name", "block_to_profile", "context", "at"});

    das::addExtern<void (*)(da_profiler::desc_id_t, const char *), &da_profiler::add_short_string_tag>(*this, lib,
      "add_short_string_tag", das::SideEffects::modifyExternal, "da_profiler::add_short_string_tag");

    das::addExtern<DAS_BIND_FUN(da_profiler_instrument_all_functions_ex)>(*this, lib, "da_profiler_instrument_all_functions",
      das::SideEffects::modifyExternal | das::SideEffects::invoke, "bind_dascript::da_profiler_instrument_all_functions_ex")
      ->args({"ctx", "block", "context", "line"});

    das::addConstant<uint32_t>(*this, "da_profiler_EVENTS", da_profiler::EVENTS);
    das::addConstant<uint32_t>(*this, "da_profiler_GPU", da_profiler::GPU);
    das::addConstant<uint32_t>(*this, "da_profiler_TAGS", da_profiler::TAGS);
    das::addConstant<uint32_t>(*this, "da_profiler_SAMPLING", da_profiler::SAMPLING);
    das::addConstant<uint32_t>(*this, "da_profiler_SAVE_SPIKES", da_profiler::SAVE_SPIKES);
    das::addConstant<uint32_t>(*this, "da_profiler_PLATFORM_EVENTS", da_profiler::PLATFORM_EVENTS);
    das::addConstant<uint32_t>(*this, "da_profiler_CONTINUOUS", da_profiler::CONTINUOUS);

    static constexpr uint32_t profile_all =
      da_profiler::EVENTS | da_profiler::TAGS | da_profiler::GPU | da_profiler::SAMPLING | da_profiler::SAVE_SPIKES;
    das::addConstant<uint32_t>(*this, "da_profiler_ALL", profile_all);

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDaProfiler.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DaProfilerModule, bind_dascript);