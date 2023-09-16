#pragma once
#include <daScript/daScript.h>
#include <daScript/simulate/runtime_matrices.h>
#include <daECS/core/entityId.h>
#include <daECS/core/event.h>
#include <daECS/core/entitySystem.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <dasModules/dasScriptsLoader.h>
#include <util/dag_simpleString.h>
#include "das_es.h"


MAKE_TYPE_FACTORY(Event, ecs::Event)

struct DasEcsStatistics
{
  uint32_t systemsCount = 0u;
  uint32_t aotSystemsCount = 0u;
  uint32_t loadTimeMs = 0u;
  int64_t memoryUsage = 0;
};

namespace bind_dascript
{
struct LoadedScript final : public DasLoadedScript<EsContext>
{
  using EsContextUniquePtr = das::shared_ptr<EsContext>;
  eastl::set<ecs::EntitySystemDesc *> systems;
  eastl::vector<QueryData> queries;
#if DAGOR_DBGLEVEL > 0
  SimpleString fn;
#endif

  LoadedScript() : bind_dascript::DasLoadedScript<EsContext>() {}

  LoadedScript(das::ProgramPtr &&program_, EsContextUniquePtr &&ctx_, DagFileAccessPtr access_, AotMode aot_mode_override,
    AotModeIsRequired aot_mode_is_required, EnableDebugger enable_debugger) :
    bind_dascript::DasLoadedScript<EsContext>(eastl::move(program_), eastl::move(ctx_), access_, aot_mode_override,
      aot_mode_is_required, enable_debugger)
  {}

  LoadedScript(LoadedScript &&l) :
    bind_dascript::DasLoadedScript<EsContext>(eastl::move(l)),
    systems(eastl::move(l.systems)),
#if DAGOR_DBGLEVEL > 0
    fn(eastl::move(l.fn)),
#endif
    queries(eastl::move(l.queries))
  {}

  LoadedScript &operator=(LoadedScript &&l)
  {
    this->bind_dascript::DasLoadedScript<EsContext>::operator=(eastl::move(l));
    systems = eastl::move(l.systems);
    queries = eastl::move(l.queries);
#if DAGOR_DBGLEVEL > 0
    fn = eastl::move(l.fn);
#endif
    return *this;
  }

  virtual void unload() override;
  virtual ~LoadedScript() override { unload(); }
};


class ECS final : public das::Module
{
public:
  ECS();
  virtual void addPrerequisits(das::ModuleLibrary &ml) const override;
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override;
  void addES(das::ModuleLibrary &ml);
  void addEntityRead(das::ModuleLibrary &ml);
  void addEntityWrite(das::ModuleLibrary &ml);
  void addEntityWriteOptional(das::ModuleLibrary &ml);
  void addEntityRW(das::ModuleLibrary &ml);
  void addContainerAnnotations(das::ModuleLibrary &ml);
  void addChildComponent(das::ModuleLibrary &ml);
  void addArray(das::ModuleLibrary &ml);
  void addObjectRead(das::ModuleLibrary &ml);
  void addObjectWrite(das::ModuleLibrary &ml);
  void addObjectRW(das::ModuleLibrary &ml);
  void addList(das::ModuleLibrary &ml);
  void addListFn(das::ModuleLibrary &ml);
  void addInitializers(das::ModuleLibrary &lib);
  void addCompMap(das::ModuleLibrary &lib);
  void addEvents(das::ModuleLibrary &lib);
  void addTemplates(das::ModuleLibrary &);
};
} // namespace bind_dascript
