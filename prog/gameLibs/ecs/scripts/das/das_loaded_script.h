// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasFsFileAccess.h>
#include <dasModules/aotEcs.h>
#include "das_es.h"

namespace bind_dascript
{
extern bool das_is_in_init_phase;
struct LoadedScript
{
  using DagFileAccessPtr = das::smart_ptr<DagFileAccess>;
  das::ProgramPtr program;
  das::shared_ptr<EsContext> ctx;
  eastl::unique_ptr<das::ModuleGroup> moduleGroup;
  DasSyntax syntax = DasSyntax::V1_5;
  union
  {
    struct
    {
      bool hasAot : 1;
      bool requireAot : 1;
      bool hasDebugger : 1;
      bool postProcessed : 1;
    };
    uint8_t flags = 0;
  };

  das::smart_ptr<DagFileAccess> access;
  using EsContextUniquePtr = das::shared_ptr<EsContext>;
  dag::VectorSet<ecs::EntitySystemDesc *> systems;
  eastl::vector<QueryData> queries;
#if DAGOR_DBGLEVEL > 0
  SimpleString fn;
#endif

  LoadedScript() = default;
  LoadedScript &operator=(const LoadedScript &l) = delete;
  LoadedScript(const LoadedScript &l) = delete;


  LoadedScript(das::ProgramPtr &&program_, EsContextUniquePtr &&ctx_, DagFileAccessPtr access_, AotMode aot_mode_override,
    AotModeIsRequired aot_mode_is_required, DasSyntax syntax_, EnableDebugger enable_debugger) :
    program(eastl::move(program_)),
    ctx(eastl::move(ctx_)),
    access(access_),
    hasAot(aot_mode_override == AotMode::AOT),
    requireAot(aot_mode_is_required == AotModeIsRequired::YES),
    syntax(syntax_),
    hasDebugger(enable_debugger == EnableDebugger::YES),
    postProcessed(false)
  {}

  LoadedScript(LoadedScript &&l) :
    ctx(eastl::move(l.ctx)),
    program(eastl::move(l.program)),
    moduleGroup(eastl::move(l.moduleGroup)),
    syntax(l.syntax),
    flags(eastl::move(l.flags)),
    access(eastl::move(l.access)),
    systems(eastl::move(l.systems)),
#if DAGOR_DBGLEVEL > 0
    fn(eastl::move(l.fn)),
#endif
    queries(eastl::move(l.queries))
  {}

  LoadedScript &operator=(LoadedScript &&l)
  {
    ctx = eastl::move(l.ctx);
    program = eastl::move(l.program);
    moduleGroup = eastl::move(l.moduleGroup);
    syntax = l.syntax;
    flags = l.flags;
    access = eastl::move(l.access);
    systems = eastl::move(l.systems);
    queries = eastl::move(l.queries);
#if DAGOR_DBGLEVEL > 0
    fn = eastl::move(l.fn);
#endif
    return *this;
  }

  void unload()
  {
    if (!systems.empty())
    {
      auto systemsCopy = eastl::move(systems);
      for (auto system : systemsCopy)
      {
        remove_system_from_list(system);
        if (system->getUserData())
          system->freeIfDynamic();
      }
      if (!das_is_in_init_phase && ctx->mgr)
        ctx->mgr->resetEsOrder();
    }
    queries.clear();

    if (program)
      debug("DaScript: unload program");
    G_ASSERT(!ctx || ctx->insideContext == 0);
    ctx.reset();
    program = nullptr;
  }
  ~LoadedScript() { unload(); }
};
} // namespace bind_dascript