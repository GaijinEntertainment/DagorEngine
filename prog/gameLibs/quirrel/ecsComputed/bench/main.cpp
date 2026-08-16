// Copyright (C) Gaijin Games KFT.  All rights reserved.

// Headless benchmark of ECS -> quirrel(frp) state mirroring, at HUD-shaped
// parameters: small matching sets, a handful of tracked changes per frame,
// subscribed and unsubscribed. See readme.txt.

#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <EASTL/string.h>
#include <stdarg.h>
#include <stdio.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/updateStage.h>
#include <daECS/io/blk.h>
#include <ecs/scripts/scripts.h>
#include <quirrel/frp/dag_frp.h>
#include <quirrel/ecsComputed/ecsComputed_api.h>
#include <sqmodules/sqmodules.h>
#include <sqrat.h>
#include <sqstdaux.h>

// The quirrel ECS binding routes events through daScript; this bench has no
// daScript, so it answers "not a das event" instead of linking the das module.
namespace bind_dascript
{
extern const uint8_t DASEVENT_NO_ROUTING = 0xFF; // const at namespace scope is internal without extern
extern const uint8_t DASEVENT_NATIVE_ROUTING = 0xFE;
uint8_t get_dasevent_routing(ecs::event_type_t) { return DASEVENT_NO_ROUTING; }
} // namespace bind_dascript

Tab<const char *> ecs_get_global_tags_context()
{
  Tab<const char *> tags(framemem_ptr());
  tags.push_back("server");
  return tags;
}

static void sq_print_fn(HSQUIRRELVM, const char *fmt, ...)
{
  va_list vl;
  va_start(vl, fmt);
  char buf[2048];
  vsnprintf(buf, sizeof(buf), fmt, vl);
  buf[sizeof(buf) - 1] = 0;
  va_end(vl);
  debug("[SQ] %s", buf);
  printf("[SQ] %s\n", buf);
}

static SQInteger bench_log(HSQUIRRELVM vm)
{
  const char *s = nullptr;
  sq_getstring(vm, 2, &s);
  debug("[bench.nut] %s", s);
  printf("[bench.nut] %s\n", s);
  return 0;
}

static int call_int_getter(const Sqrat::Object &exports, const char *fn_name)
{
  Sqrat::Function f(exports, fn_name);
  if (f.IsNull())
    return -1;
  auto res = f.Eval<int>();
  return res ? res.value() : -1;
}

static int arg_int(const char *name, int def)
{
  const char *v = ::dgs_get_argv(name);
  return v ? atoi(v) : def;
}

int DagorWinMain(bool /*debugmode*/)
{
  ::measure_cpu_freq();

  const char *mode = ::dgs_get_argv("mode") ? ::dgs_get_argv("mode") : "pull";
  const char *shape = ::dgs_get_argv("shape") ? ::dgs_get_argv("shape") : "map";
  const int numEntities = arg_int("entities", 8);
  const int changesPerFrame = arg_int("changes", 4);
  const int subscribed = arg_int("subscribed", 1);
  const int recreateEvery = arg_int("recreate", 0); // exercises row removal in the incremental path
  const int totalFrames = arg_int("frames", 2400);
  // observables the consumer derives from the row, see readme.txt
  const int derivedCount = arg_int("derived", 0);
  const int warmupFrames = 200, windowFrames = 400;
  const bool mapShape = strcmp(shape, "map") == 0;
  // off/churn/pass/native/script, see readme.txt. Everything but "off" writes
  // the same extra component per change, so the four are comparable to each
  // other and only "off" is not
  const char *filterMode = ::dgs_get_argv("filter") ? ::dgs_get_argv("filter") : "off";
  const bool filterChurn = strcmp(filterMode, "off") != 0;
  const bool filterSelects = strcmp(filterMode, "native") == 0 || strcmp(filterMode, "script") == 0;
  // rows flagged without their values changing, see readme.txt
  const bool sameWrites = arg_int("samewrites", 0) != 0;
  if (filterChurn && !filterSelects && strcmp(filterMode, "churn") != 0 && strcmp(filterMode, "pass") != 0)
  {
    printf("FATAL: -filter: must be one of off, churn, pass, native, script\n");
    return 1;
  }
  if (filterChurn && strcmp(mode, "pull") != 0)
  {
    printf("FATAL: -filter: needs -mode:pull, the script mirroring modes have no query filter\n");
    return 1;
  }
  if (sameWrites && (strcmp(shape, "map") != 0 || strcmp(filterMode, "pass") != 0))
  {
    // pass only: daECS drops the same-value writes, so the alive flips are all that
    // is left, and a filter that reads alive turns each one into a membership change
    // instead of the flagged-but-unchanged row this mode exists to produce
    printf("FATAL: -samewrites: needs -shape:map and -filter:pass\n");
    return 1;
  }
  if (derivedCount < 0 || (derivedCount > 0 && mapShape))
  {
    printf("FATAL: -derived: must be >= 0 and needs -shape:single, a per-row fan-out is a different question\n");
    return 1;
  }
  if (derivedCount > 0 && strcmp(mode, "none") == 0)
  {
    printf("FATAL: -derived: needs a mirroring mode, -mode:none has no row to derive from\n");
    return 1;
  }

  char buf[512];
  // the jamfile builds the exe into this directory, so game/ sits next to it
  eastl::string path(dd_get_fname_location(buf, dgs_argv[0]));
  dd_add_base_path(path.c_str());

  g_entity_mgr.demandInit();
  const char *esTags[] = {"server"};
  g_entity_mgr->setEsTags(dag::Span<const char *>(esTags, countof(esTags)));

  ecs::TemplateRefs trefs(*g_entity_mgr);
  ecs::load_templates_blk_file(*g_entity_mgr, "game/entities.blk", trefs, NULL);
  g_entity_mgr->addTemplates(trefs, 1);

  HSQUIRRELVM vm = sq_open(1024);
  sq_setprintfunc(vm, sq_print_fn, sq_print_fn);
  sqstd_seterrorhandlers(vm);
  DefSqModulesFileAccess fileAccess;
  fileAccess.useAbsolutePath = true;
  SqModules *moduleMgr = new SqModules(vm, &fileAccess);
  sqfrp::bind_frp_classes(moduleMgr);
  sqfrp::ObservablesGraph *frpGraph = new sqfrp::ObservablesGraph(vm, "bench");
  ecs_register_sq_binding(moduleMgr, /*create_systems*/ true, /*create_factories*/ false);
  ecscomputed::bind_module(moduleMgr);

  Sqrat::Table benchTbl(vm);
  benchTbl.SetValue("mode", mode);
  benchTbl.SetValue("shape", shape);
  benchTbl.SetValue("subscribed", subscribed != 0);
  benchTbl.SetValue("subscribedCount", subscribed); // with -derived:N, how many of them are consumed
  benchTbl.SetValue("filterMode", filterMode);
  benchTbl.SetValue("derivedCount", derivedCount);
  benchTbl.SetValue("sameWrites", sameWrites);
  benchTbl.SquirrelFunc("log", bench_log, 2, ".s");
  moduleMgr->addNativeModule("bench", benchTbl);

  Sqrat::Object benchExports;
  {
    start_es_loading();
    eastl::string nutPath = path + "game/bench.nut";
    Sqrat::string errMsg;
    if (!moduleMgr->requireModule(nutPath.c_str(), true, SqModules::__main__, benchExports, errMsg))
    {
      printf("FATAL: failed to load bench.nut: %s\n", errMsg.c_str());
      return 1;
    }
    end_es_loading();
  }
  Sqrat::Function onFrame(benchExports, "onFrame");

  Tab<ecs::EntityId> bots(tmpmem);
  ecs::EntityId heroEid;
  if (mapShape)
  {
    bots.reserve(numEntities);
    for (int i = 0; i < numEntities; ++i)
    {
      ecs::ComponentsInitializer init;
      init[ECS_HASH("bench__index")] = i;
      init[ECS_HASH("bench__value")] = i;
      bots.push_back(g_entity_mgr->createEntitySync("bench_bot", eastl::move(init)));
    }
  }
  else
    heroEid = g_entity_mgr->createEntitySync("bench_hero");

  printf("[BENCH] mode=%s shape=%s entities=%d changes=%d subscribed=%d frames=%d filter=%s derived=%d samewrites=%d\n", mode, shape,
    mapShape ? numEntities : 1, changesPerFrame, subscribed, totalFrames, filterMode, derivedCount, sameWrites ? 1 : 0);

  // ticks, not usec: a HUD-shaped frame costs well under a microsecond, so
  // per-frame rounding to whole usec would report zero
  int64_t churnT = 0, tickT = 0, frpT = 0;
  int winStart = warmupFrames;
  const float dt = 1.0f / 60.0f;

  for (int f = 0; f < totalFrames; ++f)
  {
    int64_t t0 = ref_time_ticks();
    if (mapShape && recreateEvery > 0 && (f % recreateEvery) == 0)
    {
      const int idx = (f / recreateEvery) % numEntities;
      g_entity_mgr->destroyEntity(bots[idx]);
      ecs::ComponentsInitializer init;
      init[ECS_HASH("bench__index")] = idx;
      init[ECS_HASH("bench__value")] = f + idx;
      bots[idx] = g_entity_mgr->createEntitySync("bench_bot", eastl::move(init));
    }
    if (mapShape)
    {
      const int total = max(numEntities, 1);
      const int per = min(changesPerFrame, total);
      for (int i = 0; i < per; ++i)
      {
        const int idx = (f * per + i) % total;
        if (sameWrites)
        {
          // same cost on the write side, but daECS drops these when it compares
          // the shadow copies: only the alive flips below reach the mirror, so
          // every flagged row re-reads as unchanged
          g_entity_mgr->set(bots[idx], ECS_HASH("bench__value"), g_entity_mgr->getOr(bots[idx], ECS_HASH("bench__value"), 0));
          g_entity_mgr->set(bots[idx], ECS_HASH("bench__hp"), g_entity_mgr->getOr(bots[idx], ECS_HASH("bench__hp"), 0.f));
        }
        else
        {
          g_entity_mgr->set(bots[idx], ECS_HASH("bench__value"), f + idx);
          g_entity_mgr->set(bots[idx], ECS_HASH("bench__hp"), float(100 - (f + idx) % 100));
        }
        if (filterChurn) // the row enters or leaves membership without a mirrored value changing
          g_entity_mgr->set(bots[idx], ECS_HASH("bench__alive"), ((f + idx) % 4) != 0);
      }
    }
    else
    {
      // one entity, so changesPerFrame is how many of its tracked comps churn
      g_entity_mgr->set(heroEid, ECS_HASH("hero__fuel"), float(f % 100));
      if (changesPerFrame > 1)
        g_entity_mgr->set(heroEid, ECS_HASH("hero__altitude"), float(f % 250));
      if (changesPerFrame > 2)
        g_entity_mgr->set(heroEid, ECS_HASH("hero__alert"), ((f / 30) & 1) != 0);
      if (changesPerFrame > 3)
        g_entity_mgr->set(heroEid, ECS_HASH("hero__flightMode"), ((f / 50) & 1) != 0);
      if ((f & 15) == 0) // occasional object churn; kept off the per-frame path
      {
        ecs::Object obj;
        obj.addMember("ammo", f);
        g_entity_mgr->set(heroEid, ECS_HASH("hero__loadout"), eastl::move(obj));
      }
    }
    int64_t t1 = ref_time_ticks();
    churnT += t1 - t0;
    g_entity_mgr->tick();
    g_entity_mgr->update(ecs::UpdateStageInfoAct(dt, dt * f));

    int64_t t2 = ref_time_ticks();
    tickT += t2 - t1;
    if (!onFrame.IsNull())
      onFrame.Execute();
    frpGraph->updateDeferred();
    frpT += ref_time_ticks() - t2;

    if (f == warmupFrames - 1)
    {
      churnT = tickT = frpT = 0;
      winStart = f + 1;
    }
    else if (f >= warmupFrames && (f - warmupFrames + 1) % windowFrames == 0)
    {
      const double n = double(f - winStart + 1) * 1000.0; // ticks -> nsec -> usec/frame
      const double churn = ref_time_delta_to_nsec(churnT) / n;
      const double ecs = ref_time_delta_to_nsec(tickT) / n;
      const double frp = ref_time_delta_to_nsec(frpT) / n;
      unsigned events = 0, recalcs = 0, copiedRows = 0;
      ecscomputed::get_stats(events, recalcs, copiedRows);
      printf("[BENCH] frames=%d-%d avg_us_per_frame: churn=%.3f ecs=%.3f frp=%.3f total=%.3f | events=%u recalc=%u copied=%u\n",
        winStart, f, churn, ecs, frp, churn + ecs + frp, events, recalcs, copiedRows);
      ecscomputed::reset_stats();
      churnT = tickT = frpT = 0;
      winStart = f + 1;
    }
  }

  // correctness, read once at the very end so an unsubscribed pull mirror stays
  // dormant for the whole measured run
  bool ok = true;

  // we are past end_es_loading, so creating a mirror now must be refused
  const int refused = call_int_getter(benchExports, "readCreateOutsideLoadingRefused");
  ok = ok && refused == 1;
  printf("[BENCH] mkEcsComputed outside es loading refused=%d (want 1) -> %s\n", refused, refused == 1 ? "OK" : "MISMATCH");

  if (strcmp(mode, "none") != 0)
  {
    const int count = call_int_getter(benchExports, "readCount");
    const int checksum = call_int_getter(benchExports, "readChecksum");
    const int triggers = call_int_getter(benchExports, "readTriggers");
    int expectedCount = 0, expectedChecksum = 0;
    if (mapShape)
    {
      for (ecs::EntityId eid : bots)
      {
        if (filterSelects && !g_entity_mgr->getOr(eid, ECS_HASH("bench__alive"), false))
          continue;
        ++expectedCount;
        expectedChecksum += g_entity_mgr->getOr(eid, ECS_HASH("bench__value"), 0);
      }
      if (filterSelects && (expectedCount == 0 || expectedCount == numEntities))
        printf("[BENCH] WARNING: the filter kept %d of %d rows, so it proves little\n", expectedCount, numEntities);
    }
    else if (filterSelects && !g_entity_mgr->getOr(heroEid, ECS_HASH("hero__alert"), false))
    {
      expectedCount = 0;     // filtered out, so the mirror reads as defVal
      expectedChecksum = -1; // what readChecksum answers for a null value
    }
    else
    {
      expectedCount = 1;
      expectedChecksum = (int)g_entity_mgr->getOr(heroEid, ECS_HASH("hero__fuel"), 0.f);
    }
    const bool valuesOk = count == expectedCount && checksum == expectedChecksum;
    ok = ok && valuesOk;
    printf("[BENCH] count=%d (want %d) checksum=%d (want %d) triggers=%d -> %s\n", count, expectedCount, checksum, expectedChecksum,
      triggers, valuesOk ? "OK" : "MISMATCH");
    // The values above only catch a compare that wrongly said "equal" and left the
    // mirror stale. One that wrongly says "changed" rebuilds every flagged row and
    // still mirrors the right values, so the wakeup count is the only witness.
    if (sameWrites)
    {
      ok = ok && triggers == 1;
      printf("[BENCH] triggers=%d (want 1, the initial value) -> %s\n", triggers, triggers == 1 ? "OK" : "REBUILT UNCHANGED ROWS");
    }
    if (!mapShape && strcmp(mode, "pull") == 0)
    {
      const int ammo = call_int_getter(benchExports, "readLoadoutAmmo");
      const ecs::Object *lo = g_entity_mgr->getNullable<ecs::Object>(heroEid, ECS_HASH("hero__loadout"));
      const int expectedAmmo = lo ? lo->getMemberOr(ECS_HASH("ammo"), -1) : -1;
      ok = ok && ammo == expectedAmmo;
      printf("[BENCH] loadout ammo=%d (want %d) -> %s\n", ammo, expectedAmmo, ammo == expectedAmmo ? "OK" : "MISMATCH");
    }

    // Field-by-field cross-check against an independent read of the same
    // components: the count and checksum above can alias a wrong field, a wrong
    // type or a row that should have left the mirror
    Sqrat::Array eidArr(vm, mapShape ? bots.size() : 1);
    if (mapShape)
      for (int i = 0; i < bots.size(); ++i)
        eidArr.SetValue(SQInteger(i), (SQInteger)(ecs::entity_id_t)bots[i]);
    else
      eidArr.SetValue(SQInteger(0), (SQInteger)(ecs::entity_id_t)heroEid);
    Sqrat::Function verifyFields(benchExports, "verifyFields");
    auto verdict = verifyFields.Eval<Sqrat::Object>(eidArr);
    if (!verdict.has_value())
    {
      ok = false;
      printf("[BENCH] verifyFields failed to run -> MISMATCH\n");
    }
    else if (verdict.value().GetType() == OT_NULL)
      printf("[BENCH] every mirrored field matches ECS -> OK\n");
    else
    {
      ok = false;
      printf("[BENCH] field mismatch: %s -> MISMATCH\n", verdict.value().GetVar<const char *>().value);
    }
  }

  if (strcmp(mode, "pull") == 0)
  {
    // dropping the script handles must let the sweep unregister their systems
    Sqrat::Function(benchExports, "dropMirrors").Execute();
    const int swept = ecscomputed::sweep();
    const int expectedSwept = mapShape ? 1 : 2; // the state mirror, plus loadout on the single shape
    ok = ok && swept == expectedSwept;
    printf("[BENCH] swept %d dead mirror systems (want %d) -> %s\n", swept, expectedSwept, swept == expectedSwept ? "OK" : "MISMATCH");
    // further churn must not reach the removed systems
    if (mapShape && !bots.empty())
      g_entity_mgr->set(bots[0], ECS_HASH("bench__value"), -1);
    else if (!mapShape)
      g_entity_mgr->set(heroEid, ECS_HASH("hero__fuel"), -1.f);
    g_entity_mgr->tick();
    frpGraph->updateDeferred();
  }

  onFrame.Release();
  benchExports.Release();
  benchTbl.Release();
  shutdown_ecs_sq_script(vm);
  ecscomputed::shutdown_vm(vm);
  frpGraph->shutdown(true);
  delete moduleMgr;
  sq_collectgarbage(vm);
  sq_close(vm);
  delete frpGraph;
  g_entity_mgr.demandDestroy();

  printf(ok ? "[BENCH] done\n" : "[BENCH] FAILED\n");
  return ok ? 0 : 1;
}

#define __UNLIMITED_BASE_PATH 1
#define __DEBUG_FILEPATH      "debug"
#include <startup/dag_mainCon.inc.cpp>
