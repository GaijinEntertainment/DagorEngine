// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_cpuFreq.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/random/dag_random.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_logSys.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_statDrv.h>
#include <daECS/core/internal/performQuery.h>
#include <daECS/core/internal/typesAndLimits.h>
#include <ecs/scripts/dasEs.h>
#include <startup/dag_globalSettings.h>
#include <daECS/core/sharedComponent.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_findFiles.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
#include <string.h>

#include <ioSys/dag_dataBlock.h>
#include <daECS/io/blk.h>

#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>
#include <dasModules/dasFsFileAccess.h>
#include <dasModules/aotEcs.h>
#include "unitModule.h"

extern bool NEED_DAS_AOT_COMPILE;

static constexpr int DEF_RUNS = 100;

struct SomeComponent
{
  SomeComponent() { debug("created some"); }
  static void requestResources(const char *, const ecs::resource_request_cb_t &res_cb)
  {
    res_cb("some_name", 0);
    debug("requested");
  }
};
ECS_DECLARE_RELOCATABLE_TYPE(SomeComponent);
ECS_REGISTER_RELOCATABLE_TYPE(SomeComponent, nullptr);

ECS_REGISTER_TYPE(TMatrix4, nullptr); // declared in unitModule.h


ECS_BROADCAST_EVENT_TYPE(EventCPPSimpleTestEvent, float, int)
ECS_REGISTER_EVENT(EventCPPSimpleTestEvent)

ECS_BROADCAST_EVENT_TYPE(EventCPPTestEvent, float, int, SimpleString)
ECS_BROADCAST_EVENT_TYPE(EventStart)
ECS_BROADCAST_EVENT_TYPE(EventStart2)
ECS_BROADCAST_EVENT_TYPE(EventEnd)

ECS_REGISTER_EVENT(EventCPPTestEvent)
ECS_REGISTER_EVENT(EventStart)
ECS_REGISTER_EVENT(EventStart2)
ECS_REGISTER_EVENT(EventEnd)

ECS_BROADCAST_EVENT_TYPE(EventCPPTestStringEvent, SimpleString)
ECS_REGISTER_EVENT(EventCPPTestStringEvent)

ECS_BROADCAST_EVENT_TYPE(EventEmpty)
ECS_REGISTER_EVENT(EventEmpty)

static bool had_errors = 0;


void os_debug_break()
{
  logerr("script break");
  had_errors = true;
}
void os_message_box(const char *s, const char *s2, int) { logerr("%s:%s", s, s2); }
static void my_fatal_handler(const char *title, const char *msg, const char *call_stack)
{
  printf("%s:%s\n%s", title, msg, call_stack);
  exit(1);
}

static bool count_expected_errors = false;
static int expected_errors_count = 0;
// matches only the Template::validateSets logerr; unrelated errors keep failing the run
static const char *validate_sets_err_marker = "is not a component of any known template";
// swallows only the expected broken _component fixture logerr at load
static bool swallow_component_block_err = false;

static int log_callback(int lev_tag, const char *fmt, const void * /*arg*/, int /*anum*/, const char * /*ctx_file*/, int /*ctx_line*/)
{
  if (count_expected_errors && lev_tag == LOGLEVEL_ERR && fmt && strstr(fmt, validate_sets_err_marker))
  {
    expected_errors_count++;
    return 1;
  }
  if (swallow_component_block_err && lev_tag == LOGLEVEL_ERR && fmt && strstr(fmt, "_component block at") && strstr(fmt, "is unknown"))
    return 1;
  if (!ignore_log_errors && (lev_tag == LOGLEVEL_ERR || lev_tag == LOGLEVEL_FATAL))
    had_errors = true;
  return 1;
}

#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_dbgStr.h> //set_debug_console_handle
#if _TARGET_PC_WIN
#include <windows.h> //set_debug_console_handle
#endif
extern bool dgs_execute_quiet;

static int count_entities_with_query()
{
  ecs::ComponentDesc eidComp{ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()};
  ecs::NamedQueryDesc desc{
    "free_per_thread_query_data_test",
    dag::ConstSpan<ecs::ComponentDesc>(),
    dag::ConstSpan<ecs::ComponentDesc>(&eidComp, 1),
    dag::ConstSpan<ecs::ComponentDesc>(),
    dag::ConstSpan<ecs::ComponentDesc>(),
  };
  ecs::QueryId qid = g_entity_mgr->createQuery(desc);
  int count = 0;
  ecs::perform_query(g_entity_mgr, qid, [&count](const ecs::QueryView &qv) { count += qv.end() - qv.begin(); });
  g_entity_mgr->destroyQuery(qid);
  return count;
}

static int count_entities_with_int()
{
  ecs::ComponentDesc intComp{ECS_HASH("just_int_variable"), ecs::ComponentTypeInfo<int>()};
  ecs::NamedQueryDesc desc{
    "count_entities_with_int_test",
    dag::ConstSpan<ecs::ComponentDesc>(),
    dag::ConstSpan<ecs::ComponentDesc>(&intComp, 1),
    dag::ConstSpan<ecs::ComponentDesc>(),
    dag::ConstSpan<ecs::ComponentDesc>(),
  };
  ecs::QueryId qid = g_entity_mgr->createQuery(desc);
  int count = 0;
  ecs::perform_query(g_entity_mgr, qid, [&count](const ecs::QueryView &qv) { count += qv.end() - qv.begin(); });
  g_entity_mgr->destroyQuery(qid);
  return count;
}

// an exiting thread frees its query TLS node; the later main thread clear() must then free only live-TLS nodes
static void test_free_per_thread_query_data()
{
  const int64_t mainThreadId = get_current_thread_id();
  const int expected = count_entities_with_query();
  struct QueryThread final : public DaThread
  {
    int count = -1;
    QueryThread() : DaThread("freeQueryDataTest") {}
    void execute() override
    {
      g_entity_mgr->setOwnerThreadId(get_current_thread_id());
      count = count_entities_with_query();
      g_entity_mgr->freePerThreadQueryData();
    }
  } thread;
  G_VERIFY(thread.start());
  thread.terminate(true /*wait*/);
  g_entity_mgr->setOwnerThreadId(mainThreadId);
  G_ASSERTF(thread.count == expected, "%d != %d", thread.count, expected);
  count_entities_with_query(); // re-create the main thread node so the final clear() frees it with live TLS
  printf("freePerThreadQueryData test passed (%d entities)\n", expected);
}

// a query block keeps its descriptor as a generational handle; once the owning script is unloaded the
// handle must fail with a das error instead of resolving to reused memory
static void test_stale_query_desc_handle()
{
#if DAGOR_DBGLEVEL > 0
  eastl::string scriptName;
  das::Context *scriptCtx = nullptr;
  uint64_t handle = 0;
  bind_dascript::find_loaded_das_script([&](const char *name, das::Context &ctx, das::smart_ptr<das::Program>) {
    if (!strstr(name, "findQuery.das"))
      return false;
    for (const auto &sidAndData : *ctx.tabAdLookup)
      if (bind_dascript::is_query_desc_handle(sidAndData.second))
      {
        handle = sidAndData.second;
        break;
      }
    scriptName = name;
    scriptCtx = &ctx;
    return true;
  });
  G_ASSERTF(handle != 0, "no query block handle found in findQuery.das");
  das::LineInfoArg at;
  G_VERIFY(scriptCtx->runWithCatch([&]() {
    const bind_dascript::EsQueryRef live = bind_dascript::resolve_query_desc(handle, scriptCtx, &at);
    G_ASSERT(live.desc != nullptr && (bool)live.query);
  }));
  G_VERIFY(bind_dascript::unload_es_script(scriptName.c_str()));
  das::Context ctx(4096); // the script's own context died with it
  const bool failed = !ctx.runWithCatch([&]() { bind_dascript::resolve_query_desc(handle, &ctx, &at); });
  // the error names the block whose descriptor is gone
  G_ASSERTF(failed && ctx.getException() && strstr(ctx.getException(), "stale") && strstr(ctx.getException(), "findQuery.das"), "%s",
    ctx.getException());

  // reload the script: its blocks take the freed slots back (the low half of a handle is the slot), so the
  // old handle now meets a live descriptor of a later generation and only the generation compare rejects it
  G_VERIFY(bind_dascript::load_das_script(scriptName.c_str()));
  uint64_t reused = 0;
  bind_dascript::find_loaded_das_script([&](const char *name, das::Context &reloadedCtx, das::smart_ptr<das::Program>) {
    if (strcmp(name, scriptName.c_str()) != 0)
      return false;
    for (const auto &sidAndData : *reloadedCtx.tabAdLookup)
      if (bind_dascript::is_query_desc_handle(sidAndData.second) && uint32_t(sidAndData.second) == uint32_t(handle))
        reused = sidAndData.second;
    return true;
  });
  G_ASSERTF(reused != 0 && reused != handle, "the reloaded script did not take the freed slot");
  G_VERIFY(ctx.runWithCatch([&]() { G_ASSERT(bind_dascript::resolve_query_desc(reused, &ctx, &at).desc != nullptr); }));
  const bool failedOnReuse = !ctx.runWithCatch([&]() { bind_dascript::resolve_query_desc(handle, &ctx, &at); });
  G_ASSERTF(failedOnReuse && ctx.getException() && strstr(ctx.getException(), "stale"), "%s", ctx.getException());
  printf("stale query desc handle test passed\n");
#endif
}

// the AOT and dry-run tools compile each file in its own module group and drop the group after the
// file, while a shared module promoted by that compile stays for the next file: its query descriptors
// must survive the group
static void test_shared_query_desc_outlives_group()
{
#if DAGOR_DBGLEVEL > 0
  bind_dascript::RAIIDasEnvBound bound;
  das::TextPrinter tout;
  das::LineInfoArg at;
  uint64_t handle = 0;
  {
    das::ModuleGroup group;
    das::CodeOfPolicies policies;
    policies.gen2_make_syntax = true;
    das::ProgramPtr program = das::compileDaScript("fixtures/shared_query_user.das",
      das::make_smart<bind_dascript::DagFileAccess>(bind_dascript::HotReload::DISABLED), tout, group, policies);
    G_ASSERTF(program && !program->failed(), "fixtures/shared_query_user.das does not compile");
    das::Module *shared = das::Module::require("shared_query_common");
    G_ASSERT(shared && shared->promoted);
    for (const auto &sidAndData : shared->annotationData)
      if (bind_dascript::is_query_desc_handle(sidAndData.second))
      {
        handle = sidAndData.second;
        break;
      }
    G_ASSERTF(handle != 0, "no query block handle found in shared_query_common");
  }
  das::Context ctx(4096);
  G_VERIFY(ctx.runWithCatch([&]() { G_ASSERT(bind_dascript::resolve_query_desc(handle, &ctx, &at).desc != nullptr); }));
  printf("shared query desc outlives group test passed\n");
#endif
}

// the crash this started from: a script that fails to compile after it promoted a shared module
// must not take that module's query descriptors with it
static void test_shared_query_desc_survives_failed_load()
{
#if DAGOR_DBGLEVEL > 0
  ignore_log_errors = true; // the broken fixture logerrs its compile errors by design
  const bool loaded = bind_dascript::load_das_script("fixtures/failed_load_broken.das");
  ignore_log_errors = false;
  G_ASSERTF(!loaded, "fixtures/failed_load_broken.das must not compile");
  das::Module *shared = das::Module::require("failed_load_common");
  G_ASSERTF(shared && shared->promoted, "failed_load_common must stay promoted after the failed load");
  uint64_t handle = 0;
  for (const auto &sidAndData : shared->annotationData)
    if (bind_dascript::is_query_desc_handle(sidAndData.second))
    {
      handle = sidAndData.second;
      break;
    }
  G_ASSERTF(handle != 0, "no query block handle found in failed_load_common");
  das::Context ctx(4096);
  das::LineInfoArg at;
  G_VERIFY(ctx.runWithCatch([&]() { G_ASSERT(bind_dascript::resolve_query_desc(handle, &ctx, &at).desc != nullptr); }));

  // resolving is not enough: the next good script must be able to run that block, which means the
  // lazy loop had to create its ecs query too
  G_VERIFY(bind_dascript::load_das_script("fixtures/failed_load_user.das"));
  const int expected = count_entities_with_int();
  G_ASSERTF(expected > 0, "the scene has no entity with just_int_variable to count");
  auto runSharedQuery = [](const char *when) {
    int got = -1;
    bind_dascript::find_loaded_das_script([&](const char *name, das::Context &userCtx, das::smart_ptr<das::Program>) {
      if (!strstr(name, "failed_load_user.das"))
        return false;
      das::SimFunction *fn = userCtx.findFunction("use_count_ints");
      G_ASSERTF_RETURN(fn, true, "use_count_ints not found %s", when);
      // a query may only run in a locked context, and an ES context borrows the shared stack
      auto call = [&]() { got = das::cast<int>::to(userCtx.eval(fn, nullptr)); };
      userCtx.tryRestartAndLock();
      bool ok;
      if (!userCtx.ownStack)
      {
        das::SharedFramememStackGuard guard(userCtx);
        ok = userCtx.runWithCatch(call);
      }
      else
        ok = userCtx.runWithCatch(call);
      userCtx.unlock();
      G_ASSERTF(ok, "use_count_ints threw %s: %s", when, userCtx.getException() ? userCtx.getException() : "?");
      return true;
    });
    return got;
  };
  G_ASSERTF(runSharedQuery("on the first run") == expected, "the promoted module's query did not count %d", expected);

  // the crash report had one broken module fail 101 dependent scripts: a later failed load must not
  // disturb the module that already works
  ignore_log_errors = true;
  G_VERIFY(!bind_dascript::load_das_script("fixtures/failed_load_broken.das"));
  ignore_log_errors = false;
  G_ASSERTF(runSharedQuery("after a second failed load") == expected, "a second failed load changed the count from %d", expected);

  // the descriptor belongs to the loader, not to the script that happened to compile the module:
  // unloading that script must leave the handle alive
  G_VERIFY(bind_dascript::unload_es_script("fixtures/failed_load_user.das"));
  G_VERIFY(ctx.runWithCatch([&]() { G_ASSERT(bind_dascript::resolve_query_desc(handle, &ctx, &at).desc != nullptr); }));
  printf("shared query desc survives failed load test passed (%d entities)\n", expected);
#endif
}

int myMain2(int startArgC)
{
  if (df_get_real_name("entities.blk"))
  {
    ecs::TemplateRefs trefs(*g_entity_mgr);
    SimpleString fname("entities.blk");
    ecs::load_templates_blk(*g_entity_mgr, make_span_const(&fname, 1), trefs);
    g_entity_mgr->addTemplates(trefs);
  }

  if (dd_dir_exist(dgs_argv[startArgC]))
  {
    Tab<SimpleString> file_list;
    if (find_files_in_folder(file_list, dgs_argv[startArgC], "*.das", false, true, true))
    {
      for (auto &s : file_list)
      {
        if (!bind_dascript::load_das_script(s.c_str()))
        {
          printf("Can't compile <%s>\n", s.c_str());
          return 1;
        }
        printf("Successfully compiled <%s>\n", s.c_str());
      }
    }
    else
    {
      printf("Successfully compiled <%s>\n", dgs_argv[startArgC]);
      return 1;
    }
  }
  else
  {
    // check compilation
    if (bind_dascript::load_das_script(dgs_argv[startArgC]))
    {
      printf("Successfully compiled <%s>\n", dgs_argv[startArgC]);
      return 0;
    }
    else
    {
      printf("Error compiling <%s>\n", dgs_argv[startArgC]);
      return 1;
    }
  }

  if (df_get_real_name("scene.blk"))
    ecs::create_entities_blk(*g_entity_mgr, DataBlock("scene.blk"), NULL);
  printf("scene loaded entities\n");
  for (int i = 0; i < 100; ++i)
    g_entity_mgr->tick();
  g_entity_mgr->broadcastEventImmediate(EventStart());
  printf("EventStart sent\n");
  debug("EventStart");
  g_entity_mgr->broadcastEventImmediate(EventCPPTestEvent(2.f, 10, "test_string"));


  g_entity_mgr->tick();
  for (int i = 0; i < DEF_RUNS; ++i)
    g_entity_mgr->update(ecs::UpdateStageInfoAct(0.1, 0.1));

  printf("%d acts called\n", DEF_RUNS);
  for (int i = 0; i < 100; ++i)
    g_entity_mgr->tick();

  g_entity_mgr->broadcastEventImmediate(EventStart2());
  printf("EventStart2 sent\n");
  debug("EventStart2");

  printf("%d acts called\n", DEF_RUNS);
  for (int i = 0; i < 100; ++i)
    g_entity_mgr->tick();

  g_entity_mgr->broadcastEventImmediate(EventEnd());
  printf("EventEnd sent\n");

  G_ASSERT(get_test_value("EventStartTriggered") == 1);
  G_ASSERT(get_test_value("EventEndTriggered") == 1);
  {
    // the replicated half of tests/trackedInherited.das; das observes only change events
    const ecs::Template *t = g_entity_mgr->getTemplateDB().getTemplateByName("trackedInheritedChild");
    G_ASSERT(t && t->isReplicated(ECS_HASH("inherited_track_val").hash, g_entity_mgr->getTemplateDB().data()));
    G_UNUSED(t);
  }
  {
    // validateSets: a never declared _tracked name logerrs at instantiate; a tag
    // filtered component and a descendant declared one do not (see validate_sets.blk)
    // a component created by code exists only in DataComponents (like net synced ones)
    G_VERIFY(g_entity_mgr->createComponent(ECS_HASH("code_registered_comp"),
               g_entity_mgr->getComponentTypes().findType(ecs::ComponentTypeInfo<int>::type), dag::Span<ecs::component_t>(), nullptr,
               0) != ecs::INVALID_COMPONENT_INDEX);
    ecs::TemplateRefs vtrefs(*g_entity_mgr);
    swallow_component_block_err = true; // the failed_registered _component logerrs its unknown type by design
    G_VERIFY(ecs::load_templates_blk_file(*g_entity_mgr, "validate_sets.blk", vtrefs, &g_entity_mgr->getTemplateDB().info()));
    swallow_component_block_err = false;
    g_entity_mgr->addTemplates(vtrefs);
    // the dev suite must compile the validation it counts; rel builds skip the counts
#if DAGOR_DBGLEVEL > 0
    G_STATIC_ASSERT(DAECS_EXTENSIVE_CHECKS);
#endif
    // one instantiation per case; the counter resets per case, failures name the template
    auto expectErrors = [&](const char *tname, int expected) {
      G_UNUSED(expected);
      expected_errors_count = 0;
      count_expected_errors = true;
      G_VERIFY(g_entity_mgr->createEntitySync(tname) != ecs::INVALID_ENTITY_ID);
      count_expected_errors = false;
#if DAECS_EXTENSIVE_CHECKS && DAGOR_DBGLEVEL > 0
      G_ASSERTF(expected_errors_count == expected, "%s: %d != %d", tname, expected_errors_count, expected);
#endif
    };
    // exempt through the components name map only: entities.blk loads without info, so
    // noinfo_declared_comp is not in componentTags; pin that premise first
#if DAGOR_DBGLEVEL > 0
    G_ASSERT(g_entity_mgr->getTemplateDB().info().componentTags.count(ECS_HASH("noinfo_declared_comp").hash) == 0);
#endif
    expectErrors("tracksNoInfoDeclared", 0);
    expectErrors("tracksCodeRegistered", 0);
    // the instantiation consumes the parent's sets, so the parent's dangling name
    // reports here, once
    expectErrors("childOfDangling", 1);
    // the second child must not re-report the parent: validateSets memoizes per template
    expectErrors("childOfDangling2", 0);
    // the ancestor walk reports every set; a parent's bad replicated name too
    expectErrors("childOfDanglingRepl", 1);
    // the walk is transitive: a grandparent's bad name reports through the grandchild
    expectErrors("grandChildOfDangling", 1);
    // exempt through componentTags: _component blocks and template components are
    // recorded before the tag gate, a failed registration included
    expectErrors("tracksTagFilteredComponent", 0);
    expectErrors("tracksFailedRegistered", 0);
    expectErrors("tracksPlainComponent", 0);
    expectErrors("trackedTagFiltered", 0);
    expectErrors("baseTracksChildComp", 0);
    expectErrors("sameTemplTagSkipped", 0);
    {
      // tag skipped names (param and block form) must leave the sets at parse; a
      // loaded component's name must stay
      const ecs::Template *st = g_entity_mgr->getTemplateDB().getTemplateByName("sameTemplTagSkipped");
      G_ASSERT(st && st->trackedSet().count(ECS_HASH("tag_skipped_own").hash) == 0);
      G_ASSERT(st && st->trackedSet().count(ECS_HASH("tag_skipped_block").hash) == 0);
      G_ASSERT(st && st->trackedSet().count(ECS_HASH("loaded_block").hash) == 1);
      G_ASSERT(st && st->trackedSet().count(ECS_HASH("skip_own_plain").hash) == 1);
      G_ASSERT(st && st->ignoredSet().count(ECS_HASH("tag_skipped_own").hash) == 0);
      G_ASSERT(st && st->ignoredSet().count(ECS_HASH("skip_own_plain").hash) == 1);
      G_ASSERT(st && st->replicatedSet().count(ECS_HASH("tag_skipped_own").hash) == 0);
      G_ASSERT(st && st->replicatedSet().count(ECS_HASH("skip_own_plain").hash) == 1);
      // per template isolation: the parent's tag skips do not drop the child's list name
      const ecs::Template *ct = g_entity_mgr->getTemplateDB().getTemplateByName("trackedTagFiltered");
      G_ASSERT(ct && ct->trackedSet().count(ECS_HASH("tag_filtered_comp").hash) == 1);
      G_UNUSED(ct);
      G_UNUSED(st);
    }
    expectErrors("typoTracked", 1);
    expectErrors("typoReplicated", 1);
    expectErrors("typoIgnored", 1);
    // every bad name of a set is reported, not only the first
    expectErrors("typoTrackedTwo", 2);
    // late load order: the first instantiation reports the not yet declared name once;
    // after the declaring load a second create adds no report
    expectErrors("tracksLateDeclared", 1);
    {
      ecs::TemplateRefs ltrefs(*g_entity_mgr);
      G_VERIFY(ecs::load_templates_blk_file(*g_entity_mgr, "validate_sets_late.blk", ltrefs, &g_entity_mgr->getTemplateDB().info()));
      g_entity_mgr->addTemplates(ltrefs);
    }
    expectErrors("tracksLateDeclared", 0);
    // the net sync shape: the first instantiation runs before the component exists in
    // DataComponents and reports once; after creation a second create adds no report
    expectErrors("tracksLateCodeRegistered", 1);
    G_VERIFY(g_entity_mgr->createComponent(ECS_HASH("late_code_registered_comp"),
               g_entity_mgr->getComponentTypes().findType(ecs::ComponentTypeInfo<int>::type), dag::Span<ecs::component_t>(), nullptr,
               0) != ecs::INVALID_COMPONENT_INDEX);
    expectErrors("tracksLateCodeRegistered", 0);
    {
      // membership branch: a code created template is in no load registry, so only
      // hasComponent exempts its own tracked component
      ecs::ComponentsMap cmap;
      cmap[ECS_HASH("code_tracked_comp")] = ecs::ChildComponent(0);
      ecs::Template::component_set tracked;
      tracked.insert(ECS_HASH("code_tracked_comp").hash);
      g_entity_mgr->addTemplate(ecs::Template("codeTracked", eastl::move(cmap), eastl::move(tracked), ecs::Template::component_set(),
        ecs::Template::component_set(), false));
      // parent walk half: the child tracks the parent's code declared component; the
      // child instantiates first, so only the hierarchy walk can exempt the name
      ecs::ComponentsMap childCmap;
      ecs::Template::component_set childTracked;
      childTracked.insert(ECS_HASH("code_tracked_comp").hash);
      const char *codeParents[] = {"codeTracked"};
      dag::ConstSpan<const char *> pspan(codeParents, 1);
      G_VERIFY(
        g_entity_mgr->getTemplateDB().addTemplate(ecs::Template("codeTrackedChild", eastl::move(childCmap), eastl::move(childTracked),
                                                    ecs::Template::component_set(), ecs::Template::component_set(), false),
          &pspan) == ecs::TemplateDB::AR_OK);
      expectErrors("codeTrackedChild", 0);
      expectErrors("codeTracked", 0);
      // the inverse direction: a blk parent's name that only the code created child's
      // component map declares is legal through the instantiating template's view
      ecs::ComponentsMap invCmap;
      invCmap[ECS_HASH("code_child_declared_comp")] = ecs::ChildComponent(0);
      const char *invParents[] = {"parentTracksCodeChildComp"};
      dag::ConstSpan<const char *> invSpan(invParents, 1);
      G_VERIFY(g_entity_mgr->getTemplateDB().addTemplate(
                 ecs::Template("codeChildOfTrackingParent", eastl::move(invCmap), ecs::Template::component_set(),
                   ecs::Template::component_set(), ecs::Template::component_set(), false),
                 &invSpan) == ecs::TemplateDB::AR_OK);
      expectErrors("codeChildOfTrackingParent", 0);
    }
  }
  test_free_per_thread_query_data();
  test_stale_query_desc_handle();
  test_shared_query_desc_outlives_group();
  test_shared_query_desc_survives_failed_load();
  int64_t reft = ref_time_ticks();
  g_entity_mgr->clear();
  debug("clear in %dus", get_time_usec(reft));
  printf("closed%s\n", had_errors ? " with unhandled errors (see debug file, run with -debug)" : "");
  return had_errors ? 1 : 0;
}

void require_project_specific_debugger_modules() {} // stub for dng debugger

int myMain(int startArgC)
{
  dgs_execute_quiet = true;
  g_entity_mgr.demandInit();
  // bind_dascript::set_das_root("../../../1stPartyLibs/daScript/"); // use current dir as root path
  bind_dascript::init_systems(NEED_DAS_AOT_COMPILE ? bind_dascript::AotMode::AOT : bind_dascript::AotMode::NO_AOT,
    bind_dascript::HotReload::ENABLED, bind_dascript::LoadDebugCode::YES,
    DAGOR_DBGLEVEL > 0 ? bind_dascript::LogAotErrors::YES : bind_dascript::LogAotErrors::NO, bind_dascript::DasSyntax::V1_5);
  NEED_MODULE(DagorFiles)
  NEED_MODULE(DasEcsUnitTest)
  das::Module::Initialize();

  int ret = myMain2(startArgC);

  bind_dascript::shutdown_systems();
  g_entity_mgr->getTemplateDB().clear();
  g_entity_mgr.demandDestroy();
  return ret;
}

extern void default_crt_init_kernel_lib();
extern void default_crt_init_core_lib();

extern "C" int main(int argc, char **argv)
{
  G_STATIC_ASSERT(ecs::ecs_data_alignment(alignof(Point3)) == 4);
  G_STATIC_ASSERT(ecs::ecs_data_alignment(alignof(void *)) == sizeof(size_t));
  default_crt_init_kernel_lib();
  default_crt_init_core_lib();
  symhlp_init_default();
  ::dgs_argc = argc;
  ::dgs_argv = argv;
  dgs_report_fatal_error = my_fatal_handler;
  debug_set_log_callback(&log_callback);
  int startArgC = 1;
  bool debugFile = false;
  bool verbose = false;
  while (dgs_argc - startArgC > 1)
  {
    if (strcmp(dgs_argv[startArgC], "-debug") == 0)
    {
      debugFile = true;
      startArgC++;
    }
    else if (strcmp(dgs_argv[startArgC], "-verbose") == 0)
    {
      verbose = true;
      startArgC++;
    }
    else
    {
      startArgC = dgs_argc;
      break;
    }
  }
  if (dgs_argc - startArgC < 1)
  {
    printf("Usage: [-debug] [-verbose] file_name|dir_name; file_name.das or dir_name/*.das will be checked\n");
    return 1;
  }
#if _TARGET_PC_WIN
  if (verbose)
    set_debug_console_handle((intptr_t)::GetStdHandle(STD_OUTPUT_HANDLE));
#endif
  start_classic_debug_system(debugFile ? "debug" : nullptr, false);
  dd_get_fname(""); //== pull in directoryService.obj
  char buf[512];
  eastl::string loc;
  if (strstr(dgs_argv[0], "/") || strstr(dgs_argv[0], "\\"))
    loc = eastl::string(dd_get_fname_location(buf, dgs_argv[0]));
  else
    loc = "../../";
  dd_set_named_mount_path("daslibEcs", (loc + "../../prog/gameLibs/das/ecs").c_str());
  dd_set_named_mount_path("daslib", (loc + "../../prog/1stPartyLibs/daScript/daslib").c_str());
  bind_dascript::set_das_root((loc + "../../prog/1stPartyLibs/daScript").c_str()); // use exe dir as root path
  printf("daScript+daECS unit test\n");
  debug_flush(true);
  dd_add_base_path("");
  DagorHwException::setHandler("main");
  int retcode = 0;

  DAGOR_TRY { retcode = myMain(startArgC); }
  DAGOR_CATCH(DagorException e)
  {
#ifdef DAGOR_EXCEPTIONS_ENABLED
    DagorHwException::reportException(e, true);
#endif
    printf("exception");
    return 1;
  }

  DagorHwException::cleanup();

  TIME_PROFILER_SHUTDOWN();
  return retcode;
}

#include <startup/dag_leakDetector.inc.cpp>

// extern bool dgs_execute_quiet;
// #include <windows.h>
// #include <startup/dag_leakDetector.inc.cpp>
// #include <startup/dag_mainCon.inc.cpp>
