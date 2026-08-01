// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_perfTimer.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/random/dag_random.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point3.h>
#include <debug/dag_logSys.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/io/blk.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_statDrv.h>
#include <daECS/core/internal/performQuery.h>
#include <daECS/core/sharedComponent.h>
#include <daECS/core/internal/circularBuffer.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
enum
{
  TESTS = 50000,
  ECS_RUNS = 10,
  CMP_RUNS = 40,
  CREATE_RUNS = 10,
  EID_QUERY_RUNS = 100,
  Q_CACHE_CNT = 5,
  Q_CNT = 40
};
enum class Drv3dCommand;
namespace d3d
{
int driver_command(Drv3dCommand, void *, void *, void *) { return 0; }
void beginEvent(const char *) {}
void endEvent() {}
// DriverCode get_driver_code() { return DriverCode::make(d3d::null); }
void *get_device() { return 0; }
} // namespace d3d

ECS_AUTO_REGISTER_COMPONENT(TMatrix, "transform", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(Point3, "pos", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(Point3, "pos$copy", nullptr, 0, "pos");
ECS_AUTO_REGISTER_COMPONENT(Point3, "vel", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(vec4f, "pos_vec", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(vec4f, "vel_vec", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(int, "int_variable", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(ecs::Tag, "tag_sample", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(float, "float_component", nullptr, 0, "int_variable");
ECS_AUTO_REGISTER_COMPONENT_DEPS(int, "int_component2", nullptr, 0, "int_variable");
ECS_AUTO_REGISTER_COMPONENT(ecs::Object, "object", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(ecs::string, "str_test", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<ecs::string>, "shared_str", nullptr, 0);

namespace ecs
{
static int common_job_mgr_id = -1;
int get_common_loading_job_mgr()
{
  cpujobs::init();
  if (common_job_mgr_id < 0)
    common_job_mgr_id = cpujobs::create_virtual_job_manager();
  return common_job_mgr_id;
}
struct LoadGameResJob : public cpujobs::IJob
{
  ecs::gameres_list_t resnm;
  eastl::vector<EntityId> entities;
  const char *getJobName(bool &) const override { return "LoadGameResJob"; }
  virtual void doJob()
  {
    debug("doJob");
    sleep_msec(1);
    debug("jobDone");
  }
  virtual void releaseJob()
  {
    if (g_entity_mgr && !entities.empty()) // in case if this callback is called after destruction of EntityManager
      g_entity_mgr->onEntitiesLoaded(entities, true);
    delete this;
  }
};
eastl::string format_not_loaded_gameres_message(ecs::EntityId, const gameres_list_t &) { return ""; }
extern bool load_gameres_list(const ecs::gameres_list_t &) { return true; }
extern bool filter_out_loaded_gameres(ecs::gameres_list_t &, unsigned) { return false; }
extern void place_gameres_request(eastl::vector<ecs::EntityId> &&eids, ecs::gameres_list_t &&nms)
{
  G_UNUSED(nms);
  for (auto &n : nms)
    debug("place_gameres_request <%s>", n.first.c_str());
  // g_entity_mgr->onEntitiesLoaded(eids);
  LoadGameResJob *job = new LoadGameResJob;
  job->entities = eastl::move(eids);
  job->resnm = eastl::move(nms);
  int jobMgr = get_common_loading_job_mgr();
  // if (jobMgr >= 0)
  G_VERIFY(cpujobs::add_job(jobMgr, job));
}
}; // namespace ecs

static int refcnt = 0;
struct SampleComponent
{
  int a = 2;
  int b = 1;
  SampleComponent &operator=(const SampleComponent &v)
  {
    a = v.a;
    b = v.b;
    debug("copy =");
    return *this;
  }
  SampleComponent &operator=(SampleComponent &&v)
  {
    a = v.a;
    b = v.b;
    debug("move =");
    return *this;
  }

  SampleComponent(const SampleComponent &v) : a(v.a), b(v.b) { debug("copy constr %d", ++refcnt); }
  SampleComponent(SampleComponent &&v) : a(v.a), b(v.b) { debug("move constr %d", refcnt); }
  SampleComponent() { debug("def constr %d", ++refcnt); }
  static void requestResources(const char *compname, const ecs::resource_request_cb_t &rcb)
  {
    debug("request resource for %s", compname);
    G_UNUSED(compname);
    rcb("fake name", 0);
  }
  // bool onLoaded() const{return true;}//simple
  bool onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid) const
  {
    debug("onLoaded (%d)!", mgr.getOr(eid, ECS_HASH("int_variable"), -1));
    return true;
  }
  ~SampleComponent() { debug("destr %d", --refcnt); }
  /*SampleComponent(const ecs::EntityManager &mgr, ecs::EntityId eid, const ecs::ComponentsMap &map)
  {
    a = mgr.getOr<int>(eid, ECS_HASH("int_variable"), -1);
    debug("constructor3 %d (%d)", mgr.getOr<int>(eid, ECS_HASH("int_variable"), -1),
           map[ECS_HASH("int_variable")].getOr<int>(-1));
  }
  SampleComponent(const ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    a = mgr.getOr<int>(eid, ECS_HASH("int_variable"), -1);
    debug("constructor2 %d", mgr.getOr<int>(eid, ECS_HASH("int_variable"), -1));
  }
  SampleComponent(const ecs::ComponentsMap &map)
  {
    a = map[ECS_HASH("int_variable")].getOr<int>(-1);
    debug("constructor1 %d", a);
  }*/
  /*SampleComponent()
  {
    a=0;
    debug("constructor0 %d", a);
  }
  ~SampleComponent() {debug("destructor");}*/
};

ECS_DECLARE_RELOCATABLE_TYPE(SampleComponent);
ECS_REGISTER_RELOCATABLE_TYPE(SampleComponent, nullptr);
ECS_AUTO_REGISTER_COMPONENT(SampleComponent, "sample_component", nullptr, 0);

struct SampleComponent2
{
  int a;
};
ECS_DECLARE_RELOCATABLE_TYPE(SampleComponent2);
ECS_REGISTER_RELOCATABLE_TYPE(SampleComponent2, nullptr);
ECS_AUTO_REGISTER_COMPONENT(SampleComponent2, "sample_component2", nullptr, 0);

volatile int cache0 = 0;

void prune_cache()
{
  static eastl::vector<int> memory;
  if (!memory.size())
    memory.resize(4 << 20, 1);
  for (auto i : memory)
    cache0 += i;
}

void testAllocator()
{
  StackAllocator<8> allocator;
  auto testEmpty = [&](int cnt, int blocks_cnt, uint32_t size) {
    size_t mem = allocator.calcMemAllocated();
    G_UNUSED(mem);
    for (int i = 0; i < cnt; ++i)
    {
      eastl::vector<uint8_t *> blocks(blocks_cnt);
      for (auto &b : blocks)
        b = allocator.allocate(size);
      G_ASSERT(!allocator.chunks.empty());
      for (auto bi = blocks.rbegin(), e = blocks.rend(); bi != e; ++bi)
        allocator.deallocate(*bi, size);
    }
    G_ASSERT(allocator.calcMemAllocated() == mem);
  };
  G_ASSERT(allocator.calcMemAllocated() == 0);
  testEmpty(10, 100, 9);
  testEmpty(3, 100, 16);
  testEmpty(3, 100, 24);
  debug("allocator tested");
}

void testObject()
{
  printf("object");
  ecs::Object object;
  object.insert(ECS_HASH("1")) = 11;
  printf("current %d\n", object[ECS_HASH("1")].get<int>());
  object.insert(ECS_HASH("1")) = 2;
  printf("current %d\n", object[ECS_HASH("1")].get<int>());
  object.insert(ECS_HASH("2")) = 1.0f;
  object.insert(ECS_HASH("obj")) = ecs::Object();
  object.insert(ECS_HASH("obj")).getRW<ecs::Object>().insert(ECS_HASH("1")) = 13;
  ecs::Object &child = object.insert(ECS_HASH("obj")).getRW<ecs::Object>();
  printf("current child.1 %d\n", child[ECS_HASH("1")].get<int>());
  child.insert(ECS_HASH("1")) = 14;
  printf("current child.1 %d\n", child[ECS_HASH("1")].get<int>());
  ecs::Object object2 = object;
  printf("compare object == object2 = %d\n", object2 == object);
  object.insert(ECS_HASH("obj")).getRW<ecs::Object>().insert(ECS_HASH("obj")) = ecs::Object();
  printf("compare object == object2 = %d\n", object2 == object);
}

void testArray()
{
  printf("array");
  ecs::Array object;
  object.push_back(11);
  printf("current %d\n", object[0].get<int>());
  object[0] = 2;
  printf("current %d\n", object[0].get<int>());
  object.push_back(1.0f);
  object.push_back(ecs::Object());
  object[2].getRW<ecs::Object>().insert(ECS_HASH("1")) = 13;
  ecs::Object &child = object[2].getRW<ecs::Object>();
  printf("current child.1 %d\n", child[ECS_HASH("1")].get<int>());
  child.insert(ECS_HASH("1")) = 14;
  printf("current child.1 %d\n", child[ECS_HASH("1")].get<int>());
}

static constexpr ecs::ComponentDesc kinematics_comps[] = {
  {ECS_HASH("pos"), ecs::ComponentTypeInfo<Point3>()},         // rw
  {ECS_HASH("vel"), ecs::ComponentTypeInfo<Point3>()},         // ro
  {ECS_HASH("int_variable"), ecs::ComponentTypeInfo<int>()},   // rq
  {ECS_HASH("tag_sample"), ecs::ComponentTypeInfo<ecs::Tag>()} // no
};
static constexpr ecs::ComponentDesc kinematics_events_comps[] = {
  {ECS_HASH("pos"), ecs::ComponentTypeInfo<Point3>()}, // ro
};

static __forceinline void kinematics_es(const ecs::UpdateStageInfoAct &info, Point3 &p, const Point3 &v) { p += v * info.dt; }

static void kinematics_es_all(const ecs::UpdateStageInfo &info, const ecs::QueryView &components)
{
  const ecs::UpdateStageInfoAct &act = (const ecs::UpdateStageInfoAct &)info;
  auto *__restrict pos = ECS_QUERY_COMP_RW_PTR(Point3, kinematics_comps, "pos");
  auto *__restrict posE = pos + components.end();
  pos += components.begin();
  auto *__restrict vel = ECS_QUERY_COMP_RO_PTR(Point3, kinematics_comps, "vel") + components.begin();
  do
  {
    kinematics_es(act, ecs::getRef(pos), ecs::getRef(vel)); // somehow getRef is slower, codegen just * or ** depending on type
    // kinematics_es(act, *pos, *vel);
    pos++;
    vel++;
  } while (pos < posE);
}

Point3 ppp(0, 0, 0);

static void kinematics_es_event_all(const ecs::Event &, const ecs::QueryView &components)
{
  auto *__restrict pos = ECS_QUERY_COMP_RO_PTR(Point3, kinematics_events_comps, "pos") + components.begin();
  for (uint32_t i = 0, ei = components.getEntitiesCount(); i < ei; ++i)
  {
    ppp = ppp + ecs::getRef(pos);
    pos++;
  }
}

static ecs::EntitySystemDesc kinematics_es_desc("kinematics_es", ecs::EntitySystemOps(kinematics_es_all, (ecs::EventFuncType) nullptr),
  make_span(kinematics_comps + 0, 1), make_span(kinematics_comps + 1, 1), make_span(kinematics_comps + 2, 1),
  make_span(kinematics_comps + 3, 1),
  ecs::EventSetBuilder<>::build(), // ecs::EventComponentChanged
  (1 << ecs::UpdateStageInfoAct::STAGE));

static ecs::EntitySystemDesc kinematics_events_es_desc("kinematics_events_es", ecs::EntitySystemOps(nullptr, kinematics_es_event_all),
  dag::ConstSpan<ecs::ComponentDesc>(),
  // make_span(kinematics_events_comps_rw),
  make_span(kinematics_events_comps), dag::ConstSpan<ecs::ComponentDesc>(), dag::ConstSpan<ecs::ComponentDesc>(),
  ecs::EventSetBuilder<ecs::EventComponentChanged>::build(), //
  0);

static constexpr ecs::ComponentDesc object_events_comps[] = {
  {ECS_HASH("object"), ecs::ComponentTypeInfo<ecs::Object>()} // ro
};

static void object_es_event_all(const ecs::Event &, const ecs::QueryView &components)
{
  auto *__restrict pos = ECS_QUERY_COMP_RO_PTR(ecs::Object, object_events_comps, "object");
  for (uint32_t i = 0, ei = components.getEntitiesCount(); i < ei; ++i)
  {
    pos++;
  }
  debug("changed %d", components.getEntitiesCount());
  // debug("changed = %s", evt.cast<ecs::EventComponentChanged>()->get<0>());
}

static ecs::EntitySystemDesc object_events_es_desc("object_events_es", ecs::EntitySystemOps(nullptr, object_es_event_all),
  dag::ConstSpan<ecs::ComponentDesc>(), make_span(object_events_comps), dag::ConstSpan<ecs::ComponentDesc>(),
  dag::ConstSpan<ecs::ComponentDesc>(),
  ecs::EventSetBuilder<ecs::EventComponentChanged>::build(), //
  0);

struct TestEntity
{
  TMatrix transform = {TMatrix::IDENT};
  int iv = 10, ic2 = 10;
  Point3 p = {1, 0, 0};
  TMatrix d[9] = {TMatrix::IDENT};
  Point3 v = {1, 0, 0};
  int ivCopy = 10;
  virtual void update(float dt) { p += dt * v; }
  TestEntity() = default;
  TestEntity(int i) : iv(i), ivCopy(i) {}
};

static bool check_string_relocatable()
{
  ecs::string sh = "a";
  ecs::string ln = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  char buf[sizeof(ecs::string)];
  memcpy(buf, &ln, sizeof(ecs::string));
  G_ASSERT_RETURN(ln == *(const ecs::string *)buf, false);
  memcpy(buf, &sh, sizeof(ecs::string));
  G_ASSERT_RETURN(sh == *(const ecs::string *)buf, false);
  return true;
}


static ecs::LTComponentList int_variable_component(ECS_HASH("int_variable"), ECS_HASH("int").hash, __FILE__, "DagorWinMain", __LINE__);
static ecs::LTComponentList int_component2_component(ECS_HASH("int_component2"), ECS_HASH("int").hash, __FILE__, "DagorWinMain",
  __LINE__);

DAGOR_NOINLINE
static void compare_gets()
{
  static constexpr ecs::ComponentDesc comps[] = {// ro
    {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
    // rq
    {ECS_HASH("int_variable"), ecs::ComponentTypeInfo<int>()}, {ECS_HASH("int_component2"), ecs::ComponentTypeInfo<int>()},
    // no
    {ECS_HASH("tag_sample"), ecs::ComponentTypeInfo<ecs::Tag>()}};
  ecs::NamedQueryDesc desc{
    "q_compare_gets",
    empty_span(),
    make_span(comps + 0, 1),
    make_span(comps + 1, 2),
    make_span(comps + 3, 1),
  };
  static ecs::QueryId persistentQuery = g_entity_mgr->createQuery(desc);

  static constexpr ecs::ComponentDesc calc_comps[] = {
    {ECS_HASH("int_variable"), ecs::ComponentTypeInfo<int>()}, {ECS_HASH("int_component2"), ecs::ComponentTypeInfo<int>()},
    //{ECS_HASH("tag_sample"), ecs::ComponentTypeInfo<ecs::Tag>()}
  };
  ecs::NamedQueryDesc descCalc{
    "q1C",
    empty_span(),
    make_span(calc_comps),
    empty_span(),
    empty_span(),
  };
  static ecs::QueryId calc_query_id = g_entity_mgr->createQuery(descCalc);

  int int_component_calc = 0;
  int64_t reft;
  eastl::vector<ecs::EntityId> eids;
  ecs::perform_query(g_entity_mgr, persistentQuery, [&](const ecs::QueryView &qv) {
    for (auto it = qv.begin(), endIt = qv.end(); it != endIt; ++it)
      eids.push_back(qv.getComponentRO<ecs::EntityId>(ECS_QUERY_COMP_RO_INDEX(comps, "eid"), it));
  });
  uint32_t time = ~0u;
  for (int i = 0; i < EID_QUERY_RUNS; ++i)
  {
    reft = profile_ref_ticks();
    int_component_calc = 0;
    for (auto eid : eids)
    {
      perform_query(g_entity_mgr, eid, // qv.getComponentRO<ecs::EntityId>(ECS_QUERY_COMP_RO_INDEX(comps, "eid"), it)
        calc_query_id, [&int_component_calc](const ecs::QueryView &qv) {
          int_component_calc += qv.getComponentRO<int>(0, 0) + qv.getComponentRO<int>(1, 0);
        });
    }
    uint32_t ctime = profile_time_usec(reft);
    if (time > ctime)
      time = ctime;
  }
  debug("single eid get query in %dus, ret= %d", time, int_component_calc);

  time = ~0u;
  for (int i = 0; i < EID_QUERY_RUNS; ++i)
  {
    reft = profile_ref_ticks();
    int_component_calc = 0;
    for (auto eid : eids)
      int_component_calc += *ECS_GET_NULLABLE(int, eid, int_variable) + *ECS_GET_NULLABLE(int, eid, int_component2);
    uint32_t ctime = profile_time_usec(reft);
    if (time > ctime)
      time = ctime;
  }
  debug("fast get query in %dus, ret= %d", time, int_component_calc);

  time = ~0u;
  for (int i = 0; i < EID_QUERY_RUNS; ++i)
  {
    reft = profile_ref_ticks();
    int_component_calc = 0;
    for (auto eid : eids)
      int_component_calc += *g_entity_mgr->getNullable<int>(eid, ECS_HASH("int_variable")) +
                            *g_entity_mgr->getNullable<int>(eid, ECS_HASH("int_component2"));
    uint32_t ctime = profile_time_usec(reft);
    if (time > ctime)
      time = ctime;
  }
  debug("get query in %dus, ret= %d", time, int_component_calc);

  time = ~0u;
  for (int i = 0; i < EID_QUERY_RUNS; ++i)
  {
    reft = profile_ref_ticks();
    int_component_calc = 0;
    ecs::perform_query(g_entity_mgr, calc_query_id, [&int_component_calc](const ecs::QueryView &qv) {
      for (auto it = qv.begin(), endIt = qv.end(); it != endIt; ++it)
        int_component_calc += qv.getComponentRO<int>(0, it) + qv.getComponentRO<int>(1, it);
    });
    uint32_t ctime = profile_time_usec(reft);
    if (time > ctime)
      time = ctime;
  }
  debug("just query in %dus, ret= %d", time, int_component_calc);
}

#include <future>

DAGOR_NOINLINE
void constrained_mt_mode_example()
{
  static constexpr ecs::ComponentDesc comps[] = {{ECS_HASH("int_variable"), ecs::ComponentTypeInfo<int>()}};
  ecs::NamedQueryDesc descRead{
    "thread1",
    empty_span(),
    make_span(comps),
    empty_span(),
    empty_span(),
  };
  ecs::NamedQueryDesc descWrite{
    "thread2",
    make_span(comps),
    empty_span(),
    empty_span(),
    empty_span(),
  };
  ecs::QueryId write_intQuery = g_entity_mgr->createQuery(descWrite);
  ecs::QueryId read_intQuery = g_entity_mgr->createQuery(descRead);
  G_UNUSED(write_intQuery);

  {
    ecs::ComponentsMap map;
    map[ECS_HASH("int_variable")] = 10;
    ecs::Template::component_set tracked;
    tracked.insert(ECS_HASH("int_variable").hash);
    ecs::Template templD("tsanTemplate1", eastl::move(map), eastl::move(tracked), ecs::Template::component_set(),
      ecs::Template::component_set(), false);
    g_entity_mgr->addTemplate(eastl::move(templD));
  }
  {
    ecs::ComponentsMap map;
    map[ECS_HASH("int_variable")] = 100;
    ecs::Template templD("tsanTemplate2", eastl::move(map), ecs::Template::component_set(), ecs::Template::component_set(),
      ecs::Template::component_set(), false);
    g_entity_mgr->addTemplate(eastl::move(templD));
  }

  {
    ecs::ComponentsMap map;
    map[ECS_HASH("int_variable")] = 100;
    map[ECS_HASH("int_variable2")] = 100;
    ecs::Template templD("tsanTemplate3", ecs::ComponentsMap(map), ecs::Template::component_set(), ecs::Template::component_set(),
      ecs::Template::component_set(), false);
    g_entity_mgr->addTemplate(eastl::move(templD));

    map[ECS_HASH("int_variable3")] = 100;
    ecs::Template templD2("tsanTemplate4", eastl::move(map), ecs::Template::component_set(), ecs::Template::component_set(),
      ecs::Template::component_set(), false);
    g_entity_mgr->addTemplate(eastl::move(templD2));
  }

  ecs::EntityId eid1 = g_entity_mgr->createEntitySync("tsanTemplate1");
  g_entity_mgr->setConstrainedMTMode(true);
  auto create = [&]() {
    int64_t reft = profile_ref_ticks();
    do
    {
      static constexpr int CNT = 100;
      ecs::EntityId eid3[CNT];

      for (int i = 0; i < CNT; ++i)
      {
        eid3[i] = g_entity_mgr->createEntityAsync("tsanTemplate2");
        g_entity_mgr->reCreateEntityFromAsync(eid3[i], "tsanTemplate3");
        g_entity_mgr->reCreateEntityFromAsync(eid3[i], "tsanTemplate4");
      }

      for (int i = 0; i < CNT; ++i)
        g_entity_mgr->destroyEntity(eid3[i]);
    } while (profile_time_usec(reft) < 100000); // perform for one second
  };
  auto read = [&]() {
    sleep_msec(10);
    int64_t reft = profile_ref_ticks();
    int readValues = 0;
    do
    {
      ecs::perform_query(g_entity_mgr, read_intQuery, [&](const ecs::QueryView &qv) {
        auto &w = qv.getComponentRO<int>(0, 0);
        readValues += w;
      });
      readValues += g_entity_mgr->get<int>(eid1, ECS_HASH("int_variable"));
      // printf("thread %d readValues = %d\n", int(get_current_thread_id()), readValues);
      sleep_msec(1);
    } while (profile_time_usec(reft) < 100000); // perform for one second
    printf("thread %d readValues = %d\n", int(get_current_thread_id()), readValues);
  };

  auto future1 = std::async(std::launch::async, read), future2 = std::async(std::launch::async, read),
       future3 = std::async(std::launch::async, create);
  // G_UNUSED(write_intQuery);
  while (future1.wait_for(std::chrono::milliseconds(1020)) != std::future_status::ready ||
         future2.wait_for(std::chrono::milliseconds(1020)) != std::future_status::ready ||
         future3.wait_for(std::chrono::milliseconds(1020)) != std::future_status::ready)
  {
    sleep_msec(1);
    create();
  }
  printf("ended\n");
  g_entity_mgr->setConstrainedMTMode(false);
  g_entity_mgr->destroyEntity(eid1);
  g_entity_mgr->destroyQuery(read_intQuery);
  g_entity_mgr->destroyQuery(write_intQuery);
  g_entity_mgr->tick();
}

#include <util/dag_string.h>
ecs::template_t create_template(ecs::ComponentsMap &&map, ecs::Template::component_set &&tracked = ecs::Template::component_set(),
  eastl::string *name = nullptr)
{
  static int tn = 0;
  char buf[64];
  snprintf(buf, sizeof(buf), "_t%d", tn++);
  g_entity_mgr->addTemplate(
    ecs::Template(buf, eastl::move(map), eastl::move(tracked), ecs::Template::component_set(), ecs::Template::component_set(), false));
  if (name)
    *name = buf;
  return g_entity_mgr->instantiateTemplate(g_entity_mgr->buildTemplateIdByName(buf));
}
ecs::template_t create_template(ecs::ComponentsMap &&map, const char *track)
{
  ecs::Template::component_set tracked;
  G_ASSERT(strchr(track, '^') == 0);
  tracked.insert(ECS_HASH_SLOW(track).hash);
  return create_template(eastl::move(map), eastl::move(tracked));
}

// Dependency validation must not derive a self id for templates that are not
// resident in the checked db (e.g. the updateTemplate argument).
static void test_standalone_template_validation()
{
  const ecs::type_index_t intType = g_entity_mgr->getComponentTypes().findType(ecs::ComponentTypeInfo<int>::type);
  G_VERIFY(g_entity_mgr->createComponent(ECS_HASH("dep_master"), intType, dag::Span<ecs::component_t>(), nullptr, 0) !=
           ecs::INVALID_COMPONENT_INDEX);
  ecs::component_t deps[] = {ECS_HASH("dep_master").hash};
  G_VERIFY(g_entity_mgr->createComponent(ECS_HASH("dep_slave"), intType, dag::Span<ecs::component_t>(deps, 1), nullptr, 0) !=
           ecs::INVALID_COMPONENT_INDEX);
  auto makeTempl = [](const char *name, int slave_val, bool with_master) {
    ecs::ComponentsMap map;
    map[ECS_HASH("dep_slave")] = slave_val;
    if (with_master)
      map[ECS_HASH("dep_master")] = 0;
    return ecs::Template(name, eastl::move(map), ecs::Template::component_set(), ecs::Template::component_set(),
      ecs::Template::component_set(), false);
  };

  // updateTemplate: the argument is validated as a standalone template with a
  // dependency-carrying component
  g_entity_mgr->addTemplate(makeTempl("depUpdTemplate", 1, true));
  ecs::EntityId updEid = g_entity_mgr->createEntitySync("depUpdTemplate");
  G_VERIFY(g_entity_mgr->doesEntityExist(updEid));
  G_VERIFY(g_entity_mgr->updateTemplate(makeTempl("depUpdTemplate", 2, true), nullptr, true) == ecs::TemplateDB::AR_DUP);
  G_VERIFY(g_entity_mgr->getOr(updEid, ECS_HASH("dep_slave"), -1) == 2);

  // updateTemplates: dropping the dependency while entities are alive is
  // refused; the tref is reported against trefs, where its id is valid
  const uint32_t tag = ECS_HASH("standalone_dep_test").hash;
  ecs::TemplateRefs initial(*g_entity_mgr);
  initial.emplace(makeTempl("depRefTemplate", 1, true), ecs::Template::ParentsList());
  G_VERIFY(g_entity_mgr->updateTemplates(initial, true, tag, [](const char *, ecs::EntityManager::UpdateTemplateResult) {}));
  ecs::EntityId refEid = g_entity_mgr->createEntitySync("depRefTemplate");
  G_VERIFY(g_entity_mgr->doesEntityExist(refEid));

  ecs::TemplateRefs broken(*g_entity_mgr);
  broken.emplace(makeTempl("depRefTemplate", 3, false), ecs::Template::ParentsList());
  bool sawRefusal = false;
  const bool ok = g_entity_mgr->updateTemplates(broken, true, tag, [&](const char *, ecs::EntityManager::UpdateTemplateResult r) {
    sawRefusal |= r == ecs::EntityManager::UpdateTemplateResult::RemoveHasEntities;
  });
  G_VERIFY(!ok && sawRefusal);
  G_VERIFY(g_entity_mgr->getOr(refEid, ECS_HASH("dep_slave"), -1) == 1); // refused update left values intact

  // same refusal through a parented tref: its parent ids index the trefs
  // space, so reporting must not walk them in the main db
  const uint32_t ptag = ECS_HASH("standalone_dep_ptest").hash;
  auto makeBase = [](bool with_master) {
    ecs::ComponentsMap map;
    if (with_master)
      map[ECS_HASH("dep_master")] = 0;
    return ecs::Template("depRefBase", eastl::move(map), ecs::Template::component_set(), ecs::Template::component_set(),
      ecs::Template::component_set(), false);
  };
  auto makeChild = [](int slave_val) {
    ecs::ComponentsMap map;
    map[ECS_HASH("dep_slave")] = slave_val;
    return ecs::Template("depRefChild", eastl::move(map), ecs::Template::component_set(), ecs::Template::component_set(),
      ecs::Template::component_set(), false);
  };
  ecs::TemplateRefs pinitial(*g_entity_mgr);
  pinitial.emplace(makeBase(true), ecs::Template::ParentsList());
  ecs::Template::ParentsList pinitialParents;
  pinitialParents.push_back(pinitial.ensureTemplate("depRefBase"));
  pinitial.emplace(makeChild(1), eastl::move(pinitialParents));
  G_VERIFY(g_entity_mgr->updateTemplates(pinitial, true, ptag, [](const char *, ecs::EntityManager::UpdateTemplateResult) {}));
  ecs::EntityId childEid = g_entity_mgr->createEntitySync("depRefChild");
  G_VERIFY(g_entity_mgr->doesEntityExist(childEid));

  ecs::TemplateRefs pbroken(*g_entity_mgr);
  pbroken.emplace(makeBase(false), ecs::Template::ParentsList());
  ecs::Template::ParentsList pbrokenParents;
  pbrokenParents.push_back(pbroken.ensureTemplate("depRefBase"));
  pbroken.emplace(makeChild(3), eastl::move(pbrokenParents));
  sawRefusal = false;
  const bool pok = g_entity_mgr->updateTemplates(pbroken, true, ptag, [&](const char *, ecs::EntityManager::UpdateTemplateResult r) {
    sawRefusal |= r == ecs::EntityManager::UpdateTemplateResult::RemoveHasEntities;
  });
  G_VERIFY(!pok && sawRefusal);
  G_VERIFY(g_entity_mgr->getOr(childEid, ECS_HASH("dep_slave"), -1) == 1);

  g_entity_mgr->destroyEntity(updEid);
  g_entity_mgr->destroyEntity(refEid);
  g_entity_mgr->destroyEntity(childEid);
  g_entity_mgr->tick();
  debug("standalone template validation ok");
}

void testRecreate()
{
  ecs::template_t templ, templ2;
  eastl::string templ2Name;
  {
    ecs::ComponentsMap map;
    for (int i = 0; i < 128; ++i)
    {
      String str(0, "bool_var_%d", i);
      map[ECS_HASH_SLOW(str.c_str())] = i ? true : false;
    }
    for (int i = 0; i < 128; ++i)
    {
      String str(0, "int_var_%d", i);
      map[ECS_HASH_SLOW(str.c_str())] = i;
    }
    for (int i = 0; i < 512; ++i)
    {
      String str(0, "int2_var_%d", i);
      map[ECS_HASH_SLOW(str.c_str())] = IPoint2(i, i);
    }
    for (int i = 0; i < 512; ++i)
    {
      String str(0, "point3_var_%d", i);
      map[ECS_HASH_SLOW(str.c_str())] = Point3(i, i, i);
    }
    {
      ecs::ComponentsMap map2 = map;
      map2["sample_component2"];
      templ = create_template(eastl::move(map2));
    }
    map[ECS_HASH("tag_sample")] = ecs::Tag();
    templ2 = create_template(eastl::move(map), ecs::Template::component_set(), &templ2Name);
  }
  enum
  {
    TESTS = 100,
    RUNS = 200
  };
  eastl::vector<ecs::EntityId> eid(TESTS);
  uint64_t bestCreate = ~uint64_t(0), bestRecreate = ~uint64_t(0);
  int64_t reft;
  for (int r = 0; r < RUNS; ++r)
  {
    reft = profile_ref_ticks();
    for (auto &i : eid)
      i = g_entity_mgr->createEntitySync(templ);
    bestCreate = min(bestCreate, uint64_t(profile_ref_ticks() - reft));

    g_entity_mgr->tick();
    g_entity_mgr->tick();
    g_entity_mgr->tick();
    g_entity_mgr->tick();

    for (auto &i : eid)
      g_entity_mgr->reCreateEntityFromAsync(i, templ2Name.c_str());
    reft = profile_ref_ticks();
    g_entity_mgr->tick();
    bestRecreate = min(bestRecreate, uint64_t(profile_ref_ticks() - reft));

    for (auto &i : eid)
      g_entity_mgr->destroyEntity(i);
    g_entity_mgr->tick();
  }
  debug("Create=%dus bestRecreate  = %dus", profile_usec_from_ticks_delta(bestCreate), profile_usec_from_ticks_delta(bestRecreate));
};

void testSampleComponent()
{
  ecs::ComponentsMap map;
  // map[ECS_HASH("sample_component")];//ecs::ChildComponent();
  map[ECS_HASH("sample_component")] = SampleComponent();
  map[ECS_HASH("int_variable")] = 13;
  eastl::string templ2Name;
  ecs::template_t templ2 = create_template(eastl::move(map), ecs::Template::component_set(), &templ2Name);
  ecs::ComponentsInitializer init;
  init[ECS_HASH("int_variable")] = 27;
  ecs::ComponentsMap map2;
  map2[ECS_HASH("int_variable")] = 17;
  auto eid2 = g_entity_mgr->createEntitySync(templ2, eastl::move(init), eastl::move(map2));
  debug("getOr %d", g_entity_mgr->getOr(eid2, ECS_HASH("int_variable"), -1));
  while (g_entity_mgr->getNumComponents(eid2) < 0)
  {
    debug("wait");
    sleep_msec(2);
    cpujobs::release_done_jobs();
    g_entity_mgr->tick();
  };
  // if (0)
  {
    ecs::ComponentsInitializer init;
    init[ECS_HASH("int_variable")] = 29;
    g_entity_mgr->reCreateEntityFromAsync(eid2, templ2Name.c_str(), ecs::ComponentsInitializer(init));
    // g_entity_mgr->reCreateEntityFromSync(eid2, templ2, init);
    // g_entity_mgr->reCreateEntityFromSync(eid2, templ2, init);
  }
  g_entity_mgr->tick();

  static constexpr ecs::ComponentDesc comps[] = {{ECS_HASH("sample_component"), ecs::ComponentTypeInfo<SampleComponent>()},
    {ECS_HASH("int_variable"), ecs::ComponentTypeInfo<int>()}};
  ecs::NamedQueryDesc desc{
    "q2",
    make_span(comps),
    dag::ConstSpan<ecs::ComponentDesc>(),
    dag::ConstSpan<ecs::ComponentDesc>(),
    dag::ConstSpan<ecs::ComponentDesc>(),
  };
  auto qid = g_entity_mgr->createQuery(desc);
  ecs::perform_query(g_entity_mgr, qid, [](const ecs::QueryView &components) {
    for (auto it = components.begin(), endIt = components.end(); it != endIt; ++it)
    {
      printf("a = %d\n", components.getComponentRW<SampleComponent>(ECS_QUERY_COMP_RW_INDEX(comps, "sample_component"), it).a);
      printf("int var = %d\n", components.getComponentRW<int>(ECS_QUERY_COMP_RW_INDEX(comps, "int_variable"), it));
    }
  });
  g_entity_mgr->destroyQuery(qid);
  printf("a = %d\n", g_entity_mgr->get<SampleComponent>(eid2, ECS_HASH("sample_component")).a);

  printf("a = %d\n", g_entity_mgr->getEntityComponentRef(eid2, 0).get<SampleComponent>().a);

  // DEBUG_CP();
  // eid2 = g_entity_mgr->reCreateEntityFromSync(eid2, templ);
  // DEBUG_CP();
  g_entity_mgr->destroyEntity(eid2);
  g_entity_mgr->tick();
  // exit(0);
}

void testSharedComponent()
{
  // test SharedComponent
  ecs::ComponentsMap map;
  // map[ECS_HASH("sample_component")] = ecs::ChildComponent();

  map[ECS_HASH("shared_str")] = ecs::SharedComponent<ecs::string>(ecs::string("test_string"));
  auto templ = create_template(eastl::move(map));
  auto eid = g_entity_mgr->createEntitySync(templ);
  auto eid2 = g_entity_mgr->createEntitySync(templ);
  // const ecs::string &shared_str = g_entity_mgr->get<SharedComponent<ecs::string>>(eid, ECS_HASH("shared_str"));
  debug("get shared_str <%s>", g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(eid, ECS_HASH("shared_str"))->c_str());

  // const cast to change shared component for test
  *const_cast<ecs::string *>(g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(eid, ECS_HASH("shared_str")).get()) =
    "changed_string_to_long_enough_string";
  debug("get shared_str <%s>", g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(eid2, ECS_HASH("shared_str"))->c_str());
  G_ASSERT(g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(eid2, ECS_HASH("shared_str"))->c_str() ==
           g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(eid, ECS_HASH("shared_str"))->c_str());
}

static void profileQuery(ecs::QueryId q)
{
  uint64_t bestQ = ~uint64_t(0), avgQ = 0;
  const int qRuns = 1000;
  for (int i = qRuns; i >= 0; --i)
  {
    uint64_t reft = profile_ref_ticks();
    ecs::perform_query(g_entity_mgr, q, [](const ecs::QueryView &) {});
    const uint64_t c = profile_ref_ticks() - reft;
    avgQ += c;
    bestQ = eastl::min(bestQ, c);
  }
  debug("zero cost query cost best %dticks %dus, avg = %d (%gus)", bestQ, profile_usec_from_ticks_delta(bestQ), double(avgQ / qRuns),
    double(profile_usec_from_ticks_delta(avgQ)) / qRuns);
}

void testCreateObjectOfArray()
{
  // test object of array
  ecs::ComponentsMap map;
  // map[ECS_HASH("sample_component")] = ecs::ChildComponent();

  map[ECS_HASH("object")] = ecs::Object();
  map[ECS_HASH("object")].getRW<ecs::Object>().insert(ECS_HASH("int_data")) = 111;
  map[ECS_HASH("object")].getRW<ecs::Object>().insert(ECS_HASH("array_data")) = ecs::Array();
  map[ECS_HASH("object")].getRW<ecs::Object>().insert(ECS_HASH("array_data")).getRW<ecs::Array>().push_back(2.0f);
  map[ECS_HASH("int_variable")] = 13;
  map[ECS_HASH("str_test")] = ecs::string("def_temp");
  ecs::template_t templ = create_template(eastl::move(map), "object");
  ecs::ComponentsInitializer init;
  init[ECS_HASH("int_variable")] = 27;
  ecs::ComponentsMap map2;
  map2[ECS_HASH("int_variable")] = 17;
  init[ECS_HASH("str_test")] = ecs::string("test");
  auto eid2 = g_entity_mgr->createEntitySync(templ, eastl::move(init), eastl::move(map2));
  DEBUG_CP();
  debug("string %s", g_entity_mgr->getOr(eid2, ECS_HASH("no"), "def"));
  debug("string %s", g_entity_mgr->get<ecs::string>(eid2, ECS_HASH("str_test")).c_str());

  printf("object.int_data = %d\n", g_entity_mgr->get<ecs::Object>(eid2, ECS_HASH("object"))[ECS_HASH("int_data")].get<int>());
  printf("object.array_data[0] = %f\n",
    g_entity_mgr->get<ecs::Object>(eid2, ECS_HASH("object"))[ECS_HASH("array_data")].get<ecs::Array>()[0].get<float>());
  g_entity_mgr->getRW<ecs::Object>(eid2, ECS_HASH("object")).insert(ECS_HASH("array_data")).getRW<ecs::Array>()[0] = 3.0f;
  printf("object.array_data[0] = %f\n",
    g_entity_mgr->get<ecs::Object>(eid2, ECS_HASH("object"))[ECS_HASH("array_data")].get<ecs::Array>()[0].get<float>());
  debug("check changed");
  g_entity_mgr->tick();
  debug("check changed-");
  g_entity_mgr->destroyEntity(eid2);
  g_entity_mgr->tick();
  // exit(0);
}

// Models title-scale archetype metadata: many archetypes sharing a big component pool,
// so cidx spans are wide. Numbers land in the debug log via dumpMemoryUsage.
static void benchmark_metadata_scale()
{
  constexpr int POOL = 2000, TEMPLATES = 600, COMPS_PER_TEMPLATE = 16;
  int64_t reft = profile_ref_ticks();
  for (int t = 0; t < TEMPLATES; ++t)
  {
    ecs::ComponentsMap map;
    uint32_t seed = t * 2654435761u + 12345u;
    {
      String str(0, "pool_comp_%04d", t % POOL); // low-index anchor widens the span
      map[ECS_HASH_SLOW(str.c_str())] = t;
    }
    for (int c = 1; c < COMPS_PER_TEMPLATE; ++c)
    {
      seed = seed * 1664525u + 1013904223u;
      String str(0, "pool_comp_%04d", seed % POOL);
      map[ECS_HASH_SLOW(str.c_str())] = int(seed);
    }
    create_template(eastl::move(map));
  }
  debug("metadata scale: %d templates over %d component pool in %d us", TEMPLATES, POOL, profile_time_usec(reft));
  g_entity_mgr->dumpMemoryUsage();
}

// Isolation profile of the cidx -> localId lookup structures over quasi-real
// shapes: registry of ~3000 cidx, archetypes of 16 and 1000 components, plus a
// cache-cold sweep across 600 mixed archetypes (the updateAllQueries shape).
// Structures: dense span array (original), open-addressing hash (previous),
// bitmap+rank (engine's ecs::Archetypes::ArchetypeInfo).
struct CidxDense
{
  uint16_t first = 1, last = 0;
  eastl::vector<uint16_t> slots;
  void build(const eastl::vector<uint16_t> &comps)
  {
    first = comps.front(), last = comps.back();
    slots.assign(last - first + 1, uint16_t(0xFFFF));
    for (uint32_t i = 0; i < comps.size(); ++i)
      slots[comps[i] - first] = uint16_t(i + 1);
  }
  uint16_t get(uint16_t cidx) const
  {
    uint32_t at = uint32_t(int(cidx) - int(first));
    return at < slots.size() ? slots[at] : uint16_t(0xFFFF);
  }
  size_t bytes() const { return slots.size() * 2; }
};
struct CidxHash // open addressing, load <= 0.5, range gate
{
  uint16_t first = 1, last = 0;
  uint8_t shift = 31;
  eastl::vector<uint32_t> table;
  void build(const eastl::vector<uint16_t> &comps)
  {
    first = comps.front(), last = comps.back();
    const uint32_t cap = eastl::max((uint32_t)get_bigger_pow2(int(comps.size() * 2)), 2u);
    shift = uint8_t(32 - get_log2i(cap));
    table.assign(cap, 0xFFFFFFFFu);
    for (uint32_t i = 0; i < comps.size(); ++i)
    {
      uint32_t slot = (uint32_t(comps[i]) * 0x9E3779B9u) >> shift;
      while (table[slot] != 0xFFFFFFFFu)
        slot = (slot + 1) & (cap - 1);
      table[slot] = comps[i] | ((i + 1) << 16);
    }
  }
  uint16_t get(uint16_t cidx) const
  {
    if (cidx < first || cidx > last)
      return uint16_t(0xFFFF);
    const uint32_t mask = 0xFFFFFFFFu >> shift;
    uint32_t slot = (uint32_t(cidx) * 0x9E3779B9u) >> shift;
    for (;;)
    {
      const uint32_t kv = table[slot];
      if ((kv & 0xFFFFu) == cidx)
        return uint16_t(kv >> 16);
      if (kv == 0xFFFFFFFFu)
        return uint16_t(0xFFFF);
      slot = (slot + 1) & mask;
    }
  }
  size_t bytes() const { return table.size() * 4; }
};
struct CidxBitmap // bitmap + per-word rank, as the engine's ArchetypeInfo
{
  struct Block
  {
    uint64_t bits;
    uint32_t rank, reserved;
  };
  uint16_t first = 1, last = 0;
  eastl::vector<Block> blocks;
  void build(const eastl::vector<uint16_t> &comps)
  {
    first = comps.front(), last = comps.back();
    blocks.assign((uint32_t(last) - first + 1 + 63) >> 6, Block{0, 0, 0});
    for (uint32_t i = 0; i < comps.size(); ++i)
      blocks[uint32_t(comps[i] - first) >> 6].bits |= 1ull << ((comps[i] - first) & 63);
    uint32_t run = 0;
    for (auto &b : blocks)
    {
      b.rank = run;
      run += dag::popcount(b.bits);
    }
  }
  uint16_t get(uint16_t cidx) const
  {
    if (cidx < first || cidx > last)
      return uint16_t(0xFFFF);
    const uint32_t at = uint32_t(cidx) - first;
    const Block &b = blocks[at >> 6];
    const uint64_t bit = 1ull << (at & 63);
    if (!(b.bits & bit))
      return uint16_t(0xFFFF);
    return uint16_t(b.rank + dag::popcount(b.bits & (bit - 1)) + 1);
  }
  size_t bytes() const { return blocks.size() * sizeof(Block); }
};

static eastl::vector<uint16_t> make_comp_set(uint32_t count, uint32_t registry, uint32_t &seed)
{
  eastl::vector_set<uint16_t> set;
  while (set.size() < count)
  {
    seed = seed * 1664525u + 1013904223u;
    set.insert(uint16_t(1 + (seed % registry)));
  }
  return eastl::vector<uint16_t>(set.begin(), set.end());
}

static void benchmark_cidx_map()
{
  constexpr uint32_t REGISTRY = 3000, LOOKUPS = 1 << 20;
  uint32_t seed = 12345, sink = 0;
  auto measure = [&](const char *name, auto &&get, const eastl::vector<uint16_t> &keys) {
    uint64_t best = ~uint64_t(0);
    for (int run = 0; run < 5; ++run)
    {
      int64_t reft = profile_ref_ticks();
      for (uint32_t i = 0; i < LOOKUPS; ++i)
        sink += get(keys[i]);
      best = min(best, uint64_t(profile_ref_ticks() - reft));
    }
    debug("cidx map %s: %g ns/lookup", name, 1000. * profile_usec_from_ticks_delta(best) / LOOKUPS);
  };

  // single-archetype, cache-hot: 16-comp and 1000-comp shapes
  for (uint32_t comps_cnt : {16u, 1000u})
  {
    eastl::vector<uint16_t> comps = make_comp_set(comps_cnt, REGISTRY, seed);
    CidxDense dense;
    CidxHash hash;
    CidxBitmap bmp;
    dense.build(comps), hash.build(comps), bmp.build(comps);
    eastl::vector<uint16_t> hitKeys(LOOKUPS), regKeys(LOOKUPS);
    for (uint32_t i = 0; i < LOOKUPS; ++i)
    {
      seed = seed * 1664525u + 1013904223u;
      hitKeys[i] = comps[seed % comps.size()];
      regKeys[i] = uint16_t(1 + (seed % REGISTRY)); // query-resolution shape: uniform over the registry
    }
    for (uint32_t i = 0; i < LOOKUPS; ++i) // correctness: all three agree
      G_VERIFY(dense.get(regKeys[i]) == hash.get(regKeys[i]) && dense.get(regKeys[i]) == bmp.get(regKeys[i]));
    String pfx(0, "%4d comps", comps_cnt);
    measure(String(0, "%s dense  hit", pfx.c_str()), [&](uint16_t c) { return dense.get(c); }, hitKeys);
    measure(String(0, "%s hash   hit", pfx.c_str()), [&](uint16_t c) { return hash.get(c); }, hitKeys);
    measure(String(0, "%s bitmap hit", pfx.c_str()), [&](uint16_t c) { return bmp.get(c); }, hitKeys);
    measure(String(0, "%s dense  reg-miss", pfx.c_str()), [&](uint16_t c) { return dense.get(c); }, regKeys);
    measure(String(0, "%s hash   reg-miss", pfx.c_str()), [&](uint16_t c) { return hash.get(c); }, regKeys);
    measure(String(0, "%s bitmap reg-miss", pfx.c_str()), [&](uint16_t c) { return bmp.get(c); }, regKeys);
    debug("cidx map %d comps bytes: dense = %d hash = %d bitmap = %d", comps_cnt, int(dense.bytes()), int(hash.bytes()),
      int(bmp.bytes()));
  }

  // cache-cold sweep: 600 archetypes (1/16 big), random (archetype, cidx) pairs -
  // the updateAllQueries shape, where the structure footprint decides cache behavior
  constexpr uint32_t ARCHES = 600;
  eastl::vector<CidxDense> denses(ARCHES);
  eastl::vector<CidxHash> hashes(ARCHES);
  eastl::vector<CidxBitmap> bmps(ARCHES);
  size_t denseB = 0, hashB = 0, bmpB = 0;
  for (uint32_t a = 0; a < ARCHES; ++a)
  {
    eastl::vector<uint16_t> comps = make_comp_set((a % 16) ? 16 : 1000, REGISTRY, seed);
    denses[a].build(comps), hashes[a].build(comps), bmps[a].build(comps);
    denseB += denses[a].bytes(), hashB += hashes[a].bytes(), bmpB += bmps[a].bytes();
  }
  eastl::vector<uint32_t> coldKeys(LOOKUPS); // packed {arch | cidx << 16}
  for (uint32_t i = 0; i < LOOKUPS; ++i)
  {
    seed = seed * 1664525u + 1013904223u;
    coldKeys[i] = (seed % ARCHES) | ((1 + ((seed >> 10) % REGISTRY)) << 16);
  }
  auto measureCold = [&](const char *name, auto &&arr) {
    uint64_t best = ~uint64_t(0);
    for (int run = 0; run < 5; ++run)
    {
      int64_t reft = profile_ref_ticks();
      for (uint32_t i = 0; i < LOOKUPS; ++i)
        sink += arr[coldKeys[i] & 0xFFFFu].get(uint16_t(coldKeys[i] >> 16));
      best = min(best, uint64_t(profile_ref_ticks() - reft));
    }
    debug("cidx map cold-sweep %s: %g ns/lookup", name, 1000. * profile_usec_from_ticks_delta(best) / LOOKUPS);
  };
  measureCold("dense ", denses);
  measureCold("hash  ", hashes);
  measureCold("bitmap", bmps);
  debug("cidx map cold-sweep sink %d bytes: dense = %d hash = %d bitmap = %d", sink, int(denseB), int(hashB), int(bmpB));
}

#include "../core/specialized_memcpy.h"
static void test_specialized_nequal()
{
  alignas(16) uint8_t a[33], b[33];
  for (size_t sz = 1; sz <= 32; ++sz)
  {
    for (int i = 0; i < 33; ++i)
      a[i] = b[i] = uint8_t(i * 37 + 11);
    G_VERIFY(specialized_mem_nequal(a, b, sz) == (memcmp(a, b, sz) != 0));
    b[sz - 1] ^= 0x80; // differ in the last byte of the compared range
    G_VERIFY(specialized_mem_nequal(a, b, sz) == (memcmp(a, b, sz) != 0));
    b[sz - 1] ^= 0x80;
    b[0] ^= 1; // differ in the first byte
    G_VERIFY(specialized_mem_nequal(a, b, sz) == (memcmp(a, b, sz) != 0));
    // unaligned sources, as chunk component offsets are packed, not aligned
    G_VERIFY(specialized_mem_nequal(a + 1, b + 1, sz) == (memcmp(a + 1, b + 1, sz) != 0));
  }
  debug("specialized_mem_nequal ok");
}

// Same-archetype template value update: template defaults propagate to live
// entities, per-entity overrides survive, template ids are repointed.
static void test_update_template_values()
{
  {
    ecs::ComponentsMap map;
    map[ECS_HASH("int_variable")] = 42;
    map[ECS_HASH("pos")] = Point3(1, 2, 3);
    g_entity_mgr->addTemplate(ecs::Template("updValTemplate", eastl::move(map), ecs::Template::component_set(),
      ecs::Template::component_set(), ecs::Template::component_set(), false));
  }
  eastl::vector<ecs::EntityId> eids;
  for (int i = 0; i < 100; ++i)
    eids.push_back(g_entity_mgr->createEntitySync("updValTemplate"));
  ecs::ComponentsInitializer init;
  init[ECS_HASH("int_variable")] = 555;
  ecs::EntityId overridden = g_entity_mgr->createEntitySync("updValTemplate", eastl::move(init));
  {
    ecs::ComponentsMap map;
    map[ECS_HASH("int_variable")] = 43;
    map[ECS_HASH("pos")] = Point3(1, 2, 3);
    ecs::Template upd("updValTemplate", eastl::move(map), ecs::Template::component_set(), ecs::Template::component_set(),
      ecs::Template::component_set(), false);
    G_VERIFY(g_entity_mgr->updateTemplate(eastl::move(upd), nullptr, true) == ecs::TemplateDB::AR_DUP);
  }
  for (auto e : eids)
    G_VERIFY(g_entity_mgr->getOr(e, ECS_HASH("int_variable"), -1) == 43);
  G_VERIFY(g_entity_mgr->getOr(overridden, ECS_HASH("int_variable"), -1) == 555);
  for (auto e : eids)
    g_entity_mgr->destroyEntity(e);
  g_entity_mgr->destroyEntity(overridden);
  g_entity_mgr->tick();
  debug("update template values ok");
}

// Template updates must reach descendants even when a child's db id precedes
// its parent's: a reload can point an existing template at a later-appended
// parent, so the descendant walk cannot assume topological id order.
static void test_update_template_descendants()
{
  constexpr uint32_t dagTag = 1;
  auto noErrors = [](const char *tn, ecs::EntityManager::UpdateTemplateResult r) {
    G_UNUSED(tn);
    G_VERIFY(r == ecs::EntityManager::UpdateTemplateResult::Added || r == ecs::EntityManager::UpdateTemplateResult::Updated ||
             r == ecs::EntityManager::UpdateTemplateResult::Same);
  };
  {
    ecs::TemplateRefs trefs(*g_entity_mgr);
    ecs::ComponentsMap cm;
    cm[ECS_HASH("pos")] = Point3(1, 2, 3);
    trefs.emplace(ecs::Template("updDagChild", eastl::move(cm), ecs::Template::component_set(), ecs::Template::component_set(),
                    ecs::Template::component_set(), false),
      ecs::Template::ParentsList());
    ecs::ComponentsMap pm;
    pm[ECS_HASH("int_variable")] = 1;
    trefs.emplace(ecs::Template("updDagParent", eastl::move(pm), ecs::Template::component_set(), ecs::Template::component_set(),
                    ecs::Template::component_set(), false),
      ecs::Template::ParentsList());
    G_VERIFY(g_entity_mgr->updateTemplates(trefs, true, dagTag, noErrors));
  }
  ecs::EntityId eid = g_entity_mgr->createEntitySync("updDagChild");
  G_VERIFY(g_entity_mgr->doesEntityExist(eid));
  G_VERIFY(g_entity_mgr->getOr(eid, ECS_HASH("int_variable"), -1) == -1);
  {
    // reload points the existing child at the later-appended parent: the db now
    // holds an edge whose child id precedes its parent id
    ecs::TemplateRefs trefs(*g_entity_mgr);
    ecs::ComponentsMap cm;
    cm[ECS_HASH("pos")] = Point3(1, 2, 3);
    ecs::Template::ParentsList plist;
    plist.push_back(trefs.ensureTemplate("updDagParent"));
    trefs.emplace(ecs::Template("updDagChild", eastl::move(cm), ecs::Template::component_set(), ecs::Template::component_set(),
                    ecs::Template::component_set(), false),
      eastl::move(plist));
    ecs::ComponentsMap pm;
    pm[ECS_HASH("int_variable")] = 1;
    trefs.emplace(ecs::Template("updDagParent", eastl::move(pm), ecs::Template::component_set(), ecs::Template::component_set(),
                    ecs::Template::component_set(), false),
      ecs::Template::ParentsList());
    G_VERIFY(g_entity_mgr->updateTemplates(trefs, true, dagTag, noErrors));
  }
  G_VERIFY(g_entity_mgr->getOr(eid, ECS_HASH("int_variable"), -1) == 1); // inherited through the new edge
  {
    ecs::ComponentsMap pm;
    pm[ECS_HASH("int_variable")] = 2;
    ecs::Template upd("updDagParent", eastl::move(pm), ecs::Template::component_set(), ecs::Template::component_set(),
      ecs::Template::component_set(), false);
    G_VERIFY(g_entity_mgr->updateTemplate(eastl::move(upd), nullptr, true) == ecs::TemplateDB::AR_DUP);
  }
  // the parent update reaches the child's live entity only through the
  // descendant walk over the non-topological edge
  G_VERIFY(g_entity_mgr->getOr(eid, ECS_HASH("int_variable"), -1) == 2);
  g_entity_mgr->destroyEntity(eid);
  g_entity_mgr->tick();
  debug("update template descendants ok");
}

// Diamond ladder: X_k inherits {A_k, B_k}, both inherit X_{k-1}, giving 2^DEPTH
// root-to-leaf paths. Instantiation must stay linear in unique ancestors, the
// resolved entity must carry the union of all ancestor components, and conflict
// resolution must match Template::getComponent (first found, stored parent order).
static void test_diamond_dag()
{
  constexpr int DEPTH = 20;
  ecs::TemplateRefs trefs(*g_entity_mgr);
  int uniq = 0;
  auto add = [&](const char *name, int shared, std::initializer_list<const char *> parents) {
    ecs::ComponentsMap map;
    String u(0, "ladder_comp_%03d", uniq);
    map[ECS_HASH_SLOW(u.c_str())] = uniq;
    ++uniq;
    if (shared >= 0)
      map[ECS_HASH("ladder_shared")] = shared;
    ecs::Template::ParentsList plist;
    for (const char *p : parents)
      plist.push_back(trefs.ensureTemplate(p));
    trefs.emplace(ecs::Template(name, eastl::move(map), ecs::Template::component_set(), ecs::Template::component_set(),
                    ecs::Template::component_set(), false),
      eastl::move(plist));
  };
  add("ladder_x_000", -1, {});
  for (int k = 1; k <= DEPTH; ++k)
  {
    String xp(0, "ladder_x_%03d", k - 1), xn(0, "ladder_x_%03d", k), an(0, "ladder_a_%03d", k), bn(0, "ladder_b_%03d", k);
    add(an.c_str(), k == 1 ? 101 : -1, {xp.c_str()});
    add(bn.c_str(), k == 1 ? 201 : -1, {xp.c_str()});
    add(xn.c_str(), -1, {an.c_str(), bn.c_str()});
  }
  g_entity_mgr->addTemplates(trefs);
  int64_t reft = profile_ref_ticks();
  auto eid = g_entity_mgr->createEntitySync("ladder_x_020");
  debug("diamond ladder depth %d: first create (instantiation) in %d us", DEPTH, profile_time_usec(reft));
  G_VERIFY(g_entity_mgr->doesEntityExist(eid));
  for (int i = 0; i < uniq; ++i)
  {
    String u(0, "ladder_comp_%03d", i);
    G_VERIFY(g_entity_mgr->getOr(eid, ECS_HASH_SLOW(u.c_str()), -1) == i);
  }
  const ecs::Template *xt = g_entity_mgr->getTemplateDB().getTemplateByName("ladder_x_020");
  G_VERIFY(xt != nullptr);
  int visitCount = 0;
  g_entity_mgr->getTemplateDB().data().iterate_template_parents(*xt, [&](const ecs::Template &) { visitCount++; });
  G_VERIFY(visitCount == uniq); // memoized walk: self + each unique ancestor exactly once, not once per path
  const ecs::ChildComponent &ref = xt->getComponent(ECS_HASH("ladder_shared"), g_entity_mgr->getTemplateDB().data());
  G_VERIFY(!ref.isNull());
  G_VERIFY(ref.get<int>() == 101); // pinned winner: a_001 precedes b_001 in stored parent order
  G_VERIFY(g_entity_mgr->getOr(eid, ECS_HASH("ladder_shared"), -1) == ref.get<int>());
  {
    // a null-valued hit prunes that ancestor's own parents but not its siblings:
    // mid_a's null must hide r1's 5 while the path through mid_b to r2's 9 stays
    ecs::TemplateRefs ntrefs(*g_entity_mgr);
    auto addNull = [&](const char *name, ecs::ComponentsMap &&map, std::initializer_list<const char *> parents) {
      ecs::Template::ParentsList plist;
      for (const char *p : parents)
        plist.push_back(ntrefs.ensureTemplate(p));
      ntrefs.emplace(ecs::Template(name, eastl::move(map), ecs::Template::component_set(), ecs::Template::component_set(),
                       ecs::Template::component_set(), false),
        eastl::move(plist));
    };
    ecs::ComponentsMap m1;
    m1[ECS_HASH("ladder_nshared")] = 5;
    addNull("ladder_null_r1", eastl::move(m1), {});
    ecs::ComponentsMap m2;
    m2[ECS_HASH("ladder_nshared")] = ecs::ChildComponent(); // authored null, as the blk reader does for typeless params
    addNull("ladder_null_mid_a", eastl::move(m2), {"ladder_null_r1"});
    ecs::ComponentsMap m3;
    m3[ECS_HASH("ladder_nshared")] = 9;
    addNull("ladder_null_r2", eastl::move(m3), {});
    addNull("ladder_null_mid_b", ecs::ComponentsMap(), {"ladder_null_r2"});
    addNull("ladder_null_top", ecs::ComponentsMap(), {"ladder_null_mid_a", "ladder_null_mid_b"});
    g_entity_mgr->addTemplates(ntrefs);
    const ecs::Template *nt = g_entity_mgr->getTemplateDB().getTemplateByName("ladder_null_top");
    G_VERIFY(nt != nullptr);
    const ecs::ChildComponent &nref = nt->getComponent(ECS_HASH("ladder_nshared"), g_entity_mgr->getTemplateDB().data());
    G_VERIFY(!nref.isNull());
    G_VERIFY(nref.get<int>() == 9);
  }
  g_entity_mgr->destroyEntity(eid);
  g_entity_mgr->tick();
  debug("diamond ladder ok");
}

// Archetype/template GC must reclaim descriptor memory: churn unique component
// sets (with a tracked member to exercise allTrackedPodsCidx), drain them,
// defrag, and require flat total memory after warmup.
static void test_churn_gc()
{
  constexpr int CYCLES = 100, PER_CYCLE = 30, POOL = 200;
  int churnA = 0, churnB = 0;
  size_t warmMem = 0, endMem = 0;
  for (int c = 0; c < CYCLES; ++c)
  {
    eastl::vector<ecs::EntityId> eids;
    for (int j = 0; j < PER_CYCLE; ++j)
    {
      if (++churnB >= POOL) // enumerate unique pool pairs across all cycles
      {
        ++churnA;
        churnB = churnA + 1;
      }
      ecs::ComponentsMap map;
      String sa(0, "churn_comp_%03d", churnA);
      String sb(0, "churn_comp_%03d", churnB);
      map[ECS_HASH_SLOW(sa.c_str())] = Point3(1, 2, 3);
      map[ECS_HASH_SLOW(sb.c_str())] = c * PER_CYCLE + j;
      ecs::Template::component_set tracked;
      tracked.insert(ECS_HASH_SLOW(sb.c_str()).hash);
      ecs::template_t t = create_template(eastl::move(map), eastl::move(tracked));
      G_VERIFY(t != ecs::INVALID_TEMPLATE_INDEX);
      eids.push_back(g_entity_mgr->createEntitySync(t));
    }
    for (auto e : eids)
      g_entity_mgr->destroyEntity(e);
    g_entity_mgr->tick();
    g_entity_mgr->defragArchetypes();
    if (c == 9)
      warmMem = g_entity_mgr->dumpMemoryUsage();
    else if (c == CYCLES - 1)
      endMem = g_entity_mgr->dumpMemoryUsage();
  }
  debug("churn gc: warm total = %d bytes, end total = %d bytes", int(warmMem), int(endMem));
  G_VERIFY(endMem <= warmMem + warmMem / 50); // flat steady state: <= 2% drift over 90 churn cycles
}

// Exhausts the uint16 id spaces on purpose: creation must be refused loudly
// (logerr + invalid template), never wrap, and the manager must stay usable.
static void test_width_guards()
{
  // test_churn_gc de-instantiated theTemplate1; re-instantiate it before exhausting
  // ids so the post-refusal check below exercises an already-instantiated template
  auto keeper = g_entity_mgr->createEntitySync("theTemplate1");
  G_VERIFY(g_entity_mgr->doesEntityExist(keeper));
  {
    ecs::ComponentsMap map;
    for (int i = 0; i < 1400; ++i) // 1400 * sizeof(TMatrix)=48 + eid > 64K entity
    {
      String str(0, "big_mat_%04d", i);
      map[ECS_HASH_SLOW(str.c_str())] = TMatrix::IDENT;
    }
    G_VERIFY(create_template(eastl::move(map)) == ecs::INVALID_TEMPLATE_INDEX);
  }
  {
    // raw byte sum fits uint16 but alignment padding does not; only the aligned
    // pre-flight in addArchetype can refuse this layout
    const ecs::type_index_t boolType = g_entity_mgr->getComponentTypes().findType(ecs::ComponentTypeInfo<bool>::type);
    const ecs::type_index_t matType = g_entity_mgr->getComponentTypes().findType(ecs::ComponentTypeInfo<TMatrix>::type);
    ecs::ComponentsMap map;
    for (int i = 0; i < 1030; ++i) // a bool+TMatrix pair is 49 raw / 64 aligned bytes: 1030 pairs pass raw, overflow aligned
    {
      String sb(0, "pad_bool_%04d", i);
      String sm(0, "pad_mat_%04d", i);
      G_VERIFY(g_entity_mgr->createComponent(ECS_HASH_SLOW(sb.c_str()), boolType, dag::Span<ecs::component_t>(), nullptr, 0) !=
               ecs::INVALID_COMPONENT_INDEX);
      G_VERIFY(g_entity_mgr->createComponent(ECS_HASH_SLOW(sm.c_str()), matType, dag::Span<ecs::component_t>(), nullptr, 0) !=
               ecs::INVALID_COMPONENT_INDEX);
      map[ECS_HASH_SLOW(sb.c_str())] = false;
      map[ECS_HASH_SLOW(sm.c_str())] = TMatrix::IDENT;
    }
    G_VERIFY(create_template(eastl::move(map)) == ecs::INVALID_TEMPLATE_INDEX);
  }
  int refusedAt = -1;
  for (int a = 0, t = 0; a < 2000 && refusedAt < 0; ++a)
    for (int b = a + 1; b < 2000; ++b, ++t) // unique pool pairs: unique archetypes, no new components
    {
      ecs::ComponentsMap map;
      String sa(0, "pool_comp_%04d", a);
      String sb(0, "pool_comp_%04d", b);
      map[ECS_HASH_SLOW(sa.c_str())] = t;
      map[ECS_HASH_SLOW(sb.c_str())] = -t;
      if (create_template(eastl::move(map)) == ecs::INVALID_TEMPLATE_INDEX)
      {
        refusedAt = t;
        break;
      }
    }
  debug("width guards: creation refused at extra template %d", refusedAt);
  G_VERIFY(refusedAt >= 0);
  // previously instantiated templates must still work after refusals
  auto eid = g_entity_mgr->createEntitySync("theTemplate1");
  G_VERIFY(g_entity_mgr->doesEntityExist(eid));
  G_VERIFY(g_entity_mgr->getOr(eid, ECS_HASH("int_variable"), -1) == 10);
  g_entity_mgr->destroyEntity(eid);
  g_entity_mgr->destroyEntity(keeper);
  g_entity_mgr->tick();
}

int myMain()
{
  // static const char *filter[] = {"int_variable"};
  // g_entity_mgr->setFilterTags(make_span(filter));
  testAllocator();
  g_entity_mgr.demandInit();
  // testRecreate();//return 0;
  constrained_mt_mode_example();
  {
#define DEBUG_TYPE_INFO(type)                                                                                            \
  debug("eastl::is_copy_assignable<" #type ">::value = %d", eastl::is_copy_assignable<type>::value);                     \
  debug("eastl::is_copy_constructible<" #type ">::value = %d", eastl::is_copy_constructible<type>::value);               \
  debug("eastl::is_move_assignable<" #type ">::value = %d", eastl::is_move_assignable<type>::value);                     \
  debug("eastl::is_trivially_move_assignable<" #type ">::value = %d", eastl::is_trivially_move_assignable<type>::value); \
  debug("eastl::is_move_constructible<" #type ">::value = %d", eastl::is_move_constructible<type>::value);

    DEBUG_TYPE_INFO(ecs::Object);
    DEBUG_TYPE_INFO(ecs::Array);
    DEBUG_TYPE_INFO(ecs::ChildComponent);
    DEBUG_TYPE_INFO(ecs::ChildComponent);
    DEBUG_TYPE_INFO(ecs::ComponentsInitializer);
    DEBUG_TYPE_INFO(ecs::ComponentsMap);
    DEBUG_TYPE_INFO(SampleComponent);
#undef DEBUG_TYPE_INFO
  }
  // testObject();
  // testArray();
  auto fillTemplateComponentMap = [&]() {
    ecs::ComponentsMap map;
    map[ECS_HASH("int_variable")] = 10;
    map[ECS_HASH("int_component2")] = 10;
    map[ECS_HASH("pos")] = Point3(1, 0, 0);
    map[ECS_HASH("vel")] = Point3(1, 0, 0);
    map[ECS_HASH("data0")] = TMatrix::IDENT;
    map[ECS_HASH("data1")] = TMatrix::IDENT;
    map[ECS_HASH("data2")] = TMatrix::IDENT;
    map[ECS_HASH("data3")] = TMatrix::IDENT;
    map[ECS_HASH("data4")] = TMatrix::IDENT;
    map[ECS_HASH("data5")] = TMatrix::IDENT;
    map[ECS_HASH("data6")] = TMatrix::IDENT;
    map[ECS_HASH("data7")] = TMatrix::IDENT;
    map[ECS_HASH("data8")] = TMatrix::IDENT;
    map[ECS_HASH("data9")] = TMatrix::IDENT;
    // map[ECS_HASH("pos$copy")] = Point3(0,0,0);
    // map[ECS_HASH("float_component")] = ecs::Attribute(10.5f);
    // map[ECS_HASH("int_component2")] = ecs::Attribute(20);
    return map;
  };
  ecs::template_t t1;
  {
    ecs::ComponentsMap templMap = fillTemplateComponentMap();
    ecs::Template::component_set tracked;
    tracked.insert(ECS_HASH("int_component2").hash);
    tracked.insert(ECS_HASH("int_variable").hash);
    ecs::Template templD("theTemplate1", eastl::move(templMap), eastl::move(tracked), ecs::Template::component_set(),
      ecs::Template::component_set(), false);
    g_entity_mgr->addTemplate(eastl::move(templD));
    auto eid = g_entity_mgr->createEntitySync("theTemplate1");
    t1 = g_entity_mgr->getEntityTemplateId(eid);
    g_entity_mgr->destroyEntity(eid);

    g_entity_mgr->tick();
  }
  ecs::template_t templ = t1;
  G_ASSERT(templ != ecs::INVALID_TEMPLATE_INDEX);
  eastl::vector<ecs::EntityId> eid(TESTS);
  int64_t reft = profile_ref_ticks();
  uint64_t bestCreate = ~uint64_t(0);
  for (int j = 0; j < CREATE_RUNS; ++j)
  {
    int64_t reft = profile_ref_ticks();
    for (int i = 0; i < TESTS; ++i)
    {
      // eid.data()[i] = g_entity_mgr->createEntitySync(templ);
      ecs::ComponentsInitializer init;
      ECS_INIT(init, int_variable, i);
      // init[ECS_HASH("int_variable")] = i;
      // if (i%2 == 0)
      //   init[ECS_HASH("vel")] = Point3(0,0,0);
      eid.data()[i] = g_entity_mgr->createEntitySync("theTemplate1", eastl::move(init)); //"theTemplate1",templ
      // eid.data()[i] = g_entity_mgr->createEntitySync(templ);
      // eid = g_entity_mgr->createEntitySync(templ, ecs::ComponentsMap());
    }
    bestCreate = min(bestCreate, uint64_t(profile_ref_ticks() - reft));
    if (j != CREATE_RUNS - 1)
    {
      for (int i = TESTS - 1; i >= 0; --i)
        g_entity_mgr->destroyEntity(eid.data()[i]);
      g_entity_mgr->tick();
    }
  }
  debug("total create time = %d us", profile_usec_from_ticks_delta(bestCreate));
  // for (int i = 0; i < 500; ++i)//to move to one chunk, simulate relaxation during many frames
  //   g_entity_mgr->tick();
  {
    int64_t reft = profile_ref_ticks();
    eastl::vector<TestEntity> tests(TESTS);
    for (int i = 0; i < TESTS; ++i)
      tests[i].iv = tests[i].ivCopy = i;
    debug("best possible (single alloc, init iv) create time = %d us", profile_time_usec(reft));
    {
      int64_t reft = profile_ref_ticks();
      eastl::vector<TestEntity> tests;
      for (int i = 0; i < TESTS; ++i)
        tests.emplace_back(TestEntity(i));
      debug("best possible grow create time = %d us", profile_time_usec(reft));
    }
    reft = profile_ref_ticks();
    for (int i = 0; i < TESTS; ++i)
      tests.erase_unsorted(tests.begin());
    debug("best (erase_unsorted) possible reverse destroy time = %d us", profile_time_usec(reft));
  }
  // exit(0);


  uint64_t totalTime = 0, bestTime = ~0ULL;

  // if (0)
  {
    ecs::ComponentsMap map;
    DEBUG_CP();
    // map[ECS_HASH("sample_component")];//ecs::ChildComponent();
    map[ECS_HASH("sample_component")] = SampleComponent();
    map[ECS_HASH("int_variable")] = 13;
    eastl::string templ2Name;
    ecs::template_t templ2 = create_template(eastl::move(map), ecs::Template::component_set(), &templ2Name);
    ecs::ComponentsInitializer init;
    init[ECS_HASH("int_variable")] = 27;
    ecs::ComponentsMap map2;
    map2[ECS_HASH("int_variable")] = 17;
    auto eid2 = g_entity_mgr->createEntitySync(templ2, eastl::move(init), eastl::move(map2));
    debug("getOr %d", g_entity_mgr->getOr(eid2, ECS_HASH("int_variable"), -1));
    while (g_entity_mgr->getNumComponents(eid2) < 0)
    {
      debug("wait");
      sleep_msec(2);
      cpujobs::release_done_jobs();
      g_entity_mgr->tick();
    };
    // if (0)
    {
      ecs::ComponentsInitializer init;
      init[ECS_HASH("int_variable")] = 29;
      g_entity_mgr->reCreateEntityFromAsync(eid2, templ2Name.c_str(), ecs::ComponentsInitializer(init));
      // g_entity_mgr->reCreateEntityFromSync(eid2, templ2, init);
      // g_entity_mgr->reCreateEntityFromSync(eid2, templ2, init);
    }
    DEBUG_CP();
    g_entity_mgr->tick();
    DEBUG_CP();

    static constexpr ecs::ComponentDesc comps[] = {{ECS_HASH("sample_component"), ecs::ComponentTypeInfo<SampleComponent>()},
      {ECS_HASH("int_variable"), ecs::ComponentTypeInfo<int>()}};
    ecs::NamedQueryDesc desc{
      "q2",
      make_span(comps),
      dag::ConstSpan<ecs::ComponentDesc>(),
      dag::ConstSpan<ecs::ComponentDesc>(),
      dag::ConstSpan<ecs::ComponentDesc>(),
    };
    auto qid = g_entity_mgr->createQuery(desc);
    ecs::perform_query(g_entity_mgr, qid, [](const ecs::QueryView &components) {
      for (auto it = components.begin(), endIt = components.end(); it != endIt; ++it)
      {
        printf("a = %d\n", components.getComponentRW<SampleComponent>(ECS_QUERY_COMP_RW_INDEX(comps, "sample_component"), it).a);
        printf("int var = %d\n", components.getComponentRW<int>(ECS_QUERY_COMP_RW_INDEX(comps, "int_variable"), it));
      }
    });
    g_entity_mgr->destroyQuery(qid);
    printf("a = %d\n", g_entity_mgr->get<SampleComponent>(eid2, ECS_HASH("sample_component")).a);

    printf("a = %d\n", g_entity_mgr->getEntityComponentRef(eid2, 0).get<SampleComponent>().a);

    // DEBUG_CP();
    // eid2 = g_entity_mgr->reCreateEntityFromSync(eid2, templ);
    // DEBUG_CP();
    g_entity_mgr->destroyEntity(eid2);
    g_entity_mgr->tick();
    // exit(0);
  }

  testSampleComponent();
  testSharedComponent();
  testCreateObjectOfArray();

  for (int i = 0; i < 500; ++i) // to move to one chunk, simulate relaxation during many frames
    g_entity_mgr->tick();

  /*{
    SimpleString fname("entities.blk");
    ecs::TemplateRefs trefs(*g_entity_mgr);
    ecs::load_templates_blk(*g_entity_mgr, dag::ConstSpan<SimpleString>(&fname, 1), trefs,
  &g_entity_mgr->getMutableTemplateDB().info()); if (!trefs.empty()) g_entity_mgr->getMutableTemplateDB().addTemplates(trefs); auto eid
  = g_entity_mgr->createEntityAsync("test_blk"); g_entity_mgr->destroyEntity(eid);
  }*/
  {
    ecs::ComponentsMap map;
    map[ECS_HASH("int_variable")] = 12;
    map[ECS_HASH("pos")] = Point3(0, 0, 0);
    map[ECS_HASH("vel")] = Point3(0, 0, 0);
    map[ECS_HASH("tag_sample")] = ecs::Tag();
    map[ECS_HASH("transform")] = TMatrix::IDENT;
    // map[ECS_HASH("int_component2")] = 10;
    templ = create_template(eastl::move(map));
    ecs::ComponentsInitializer map2;
    map2[ECS_HASH("transform")] = TMatrix::IDENT;
    // DEBUG_CP();
    g_entity_mgr->createEntitySync(templ, eastl::move(map2));
  }
  {
    ecs::ComponentsMap map;
    map[ECS_HASH("pos")] = Point3(0, 0, 0);
    map[ECS_HASH("vel")] = Point3(1, 0, 0);
    // map[ECS_HASH("int_variable")] = 13;
    templ = create_template(eastl::move(map), "pos");
    auto eid2 = g_entity_mgr->createEntitySync(templ);
    g_entity_mgr->set(eid2, ECS_HASH("pos"), Point3(10, 0, 0));
    DEBUG_CP();
    g_entity_mgr->tick();
    DEBUG_CP();
  }

  static constexpr ecs::ComponentDesc comps[] = {
    {ECS_HASH("pos"), ecs::ComponentTypeInfo<Point3>()},         // rw
    {ECS_HASH("vel"), ecs::ComponentTypeInfo<Point3>()},         // ro
    {ECS_HASH("int_variable"), ecs::ComponentTypeInfo<int>()},   // rq
    {ECS_HASH("data0"), ecs::ComponentTypeInfo<TMatrix>()},      // rq
    {ECS_HASH("tag_sample"), ecs::ComponentTypeInfo<ecs::Tag>()} // no
  };
  ecs::NamedQueryDesc desc{
    "q1",
    make_span(comps + 0, 1),
    make_span(comps + 1, 1),
    make_span(comps + 2, 2),
    make_span(comps + 4, 1),
  };

  float dt = 0.1;
  prune_cache();
  reft = profile_ref_ticks();
  // ecs::perform_query(g_entity_mgr, desc, [](const ecs::QueryView& ){});
  // debug("zero query cost %dus, ", profile_time_usec(reft));
  ecs::QueryId persistentQuery = g_entity_mgr->createQuery(desc);
  debug("create query cost %dus", profile_time_usec(reft));
  profileQuery(persistentQuery);

  compare_gets();

  //_exit(0);

  prune_cache();
  reft = profile_ref_ticks();
  g_entity_mgr->tick();
  debug("tick in %dus ", profile_time_usec(reft));
  prune_cache();
  reft = profile_ref_ticks();
  ecs::perform_query(g_entity_mgr, persistentQuery, [dt](const ecs::QueryView &qv) {
    for (auto it = qv.begin(), endIt = qv.end(); it != endIt; ++it)
    {
      // if ((grnd()&31) == 0)
      qv.getComponentRW<Point3>(ECS_QUERY_COMP_RW_INDEX(comps, "pos"), it) +=
        dt * qv.getComponentRO<Point3>(ECS_QUERY_COMP_RO_INDEX(comps, "vel"), it);
      // debug("%p", it.getComponentRORaw(0, sizeof(int)));
      // debug("%d", (*(int*)it.getComponentRORaw(0, sizeof(int))));
      // break;
      // int_component_calc += *it.getComponentRORaw<int>(1);
    }
  });
  debug("persistent query in %dus", profile_time_usec(reft));

  prune_cache();
  reft = profile_ref_ticks();
  g_entity_mgr->tick();
  debug("tick in %dus ", profile_time_usec(reft));
  prune_cache();

  reft = profile_ref_ticks();
  g_entity_mgr->destroyQuery(persistentQuery);
  debug("destroy query cost %dus", profile_time_usec(reft));
  ecs::QueryId persistentScalarQuery = g_entity_mgr->createQuery(desc);
  prune_cache();
  reft = profile_ref_ticks();
  // code-gen like
  auto queryFun = [&, dt](const ecs::QueryView &components) {
    // G_ASSERT(qv.isRW<Point3>(0));G_ASSERT(qv.isRO<Point3>(0));
    auto *__restrict pos = ECS_QUERY_COMP_RW_PTR(Point3, kinematics_comps, "pos");
    auto *__restrict posE = pos + components.end();
    pos += components.begin();
    auto *__restrict vel = ECS_QUERY_COMP_RO_PTR(Point3, comps, "vel");
    // const int * int_component = qv.getComponentRORaw<int>(1, cit);
    do
    {
      // if ((grnd()&31) == 0)
      // pos[i] += dt*vel[i];
      ecs::getRef(pos) += dt * ecs::getRef(vel);
      //*pos += dt * *vel;
      // if (int_component)
      //   int_component_calc += *int_component;
      pos++;
      vel++;
      // if (int_component)
      //   int_component++;
    } while (pos < posE);
  };
  totalTime = 0, bestTime = ~uint64_t(0);
  for (int i = 0; i < Q_CACHE_CNT; ++i)
  {
    prune_cache();
    reft = profile_ref_ticks();
    ecs::perform_query(g_entity_mgr, persistentScalarQuery, queryFun);
    const uint64_t ctime = profile_ref_ticks() - reft;
    bestTime = min(ctime, bestTime);
    totalTime += ctime;
  }
  debug("(no cached)query in %gus, best =%gus", double(totalTime) / int(Q_CACHE_CNT) / profiler_ticks_to_us,
    double(bestTime) / profiler_ticks_to_us);

  totalTime = 0, bestTime = ~0ULL;
  for (int i = 0; i < ECS_RUNS; ++i)
  {
    reft = profile_ref_ticks();
    ecs::perform_query(g_entity_mgr, persistentScalarQuery, queryFun);
    const uint64_t ctime = profile_ref_ticks() - reft;
    bestTime = min(ctime, bestTime);
    totalTime += ctime;
  }
  debug("(cached)query in %gus, best =%gus", (double(totalTime) / int(ECS_RUNS)) / profiler_ticks_to_us,
    double(bestTime) / profiler_ticks_to_us);
  g_entity_mgr->destroyQuery(persistentScalarQuery);

  {
    static constexpr ecs::ComponentDesc comps[] = {{ECS_HASH("pos_vec"), ecs::ComponentTypeInfo<vec4f>()},
      {ECS_HASH("vel_vec"), ecs::ComponentTypeInfo<vec4f>()},
      {ECS_HASH("int_variable"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL}};
    ecs::NamedQueryDesc desc{
      "vecq",
      make_span(comps, 1),
      make_span(comps + 1, 2),
      dag::ConstSpan<ecs::ComponentDesc>(),
      dag::ConstSpan<ecs::ComponentDesc>(),
    };
    ecs::QueryId persistentVecQuery = g_entity_mgr->createQuery(desc);
    prune_cache();
    reft = profile_ref_ticks();
    // code-gen like
    ecs::perform_query(g_entity_mgr, persistentVecQuery, [dt](const ecs::QueryView &components) {
      auto *__restrict pos = ECS_QUERY_COMP_RW_PTR(vec4f, comps, "pos_vec"); // qv.getComponentRaw_rw<vec4f>(0, cit);
      auto *__restrict vel = ECS_QUERY_COMP_RO_PTR(vec4f, comps, "vel_vec"); // qv.getComponentRaw_rw<vec4f>(0, cit);
      // const auto* __restrict vel = qv.getComponentRaw_ro<vec4f>(0, cit);
      // const int * int_component = qv.getComponentRORaw<int>(1, cit);
      for (uint32_t i = 0, ei = components.getEntitiesCount(); i < ei; ++i)
      {
        *pos = v_madd(*vel, v_splats(dt), *pos);
        // if (int_component)
        //   int_component_calc += *int_component;
        pos++;
        vel++;
        // if (int_component)
        //   int_component++;
      }
    });
    debug("vec query in %dus", profile_time_usec(reft));
    g_entity_mgr->destroyQuery(persistentVecQuery);
  }

  prune_cache();
  reft = profile_ref_ticks();
  g_entity_mgr->tick();
  debug("tick in %dus ", profile_time_usec(reft));
  prune_cache();
  {
    debug("before ");
    uint64_t best = ~0u, total = 0;
    for (int i = 0; i < 1000; ++i)
    {
      reft = profile_ref_ticks();
      g_entity_mgr->update(ecs::UpdateStageInfoAct(0.1, 0.1));
      const uint64_t ct = profile_ref_ticks() - reft;
      best = min(best, ct);
      total += ct;
    }
    debug("best update in %fus ", double(best) / profiler_ticks_to_us);
  }
  prune_cache();
  reft = profile_ref_ticks();
  g_entity_mgr->tick();
  debug("tick in %dus ", profile_time_usec(reft));
  prune_cache();
  reft = profile_ref_ticks();
  int ret = 0;
  // for (int j = 0; j < 100000; ++j)
  for (int j = 0; j < 100; ++j)
    for (int i = 0; i < TESTS; ++i)
      ret += g_entity_mgr->get<int>(eid.data()[i], ECS_HASH("int_variable"));
  debug("int_component = %d in %g us", ret, profile_time_usec(reft) / 100.);

  prune_cache();
  reft = profile_ref_ticks();
  g_entity_mgr->tick();
  debug("tick in %dus ", profile_time_usec(reft));

  prune_cache();
  reft = profile_ref_ticks();
  // for (int i = TESTS-1; i >= 0; --i)
  for (int i = 0; i < TESTS; ++i)
    g_entity_mgr->destroyEntity(eid.data()[i]);
  g_entity_mgr->tick();
  debug("destroy = %d us", profile_time_usec(reft));

  prune_cache();
  reft = profile_ref_ticks();
  g_entity_mgr->tick();
  debug("tick in %dus ", profile_time_usec(reft));

  eastl::vector<Point3> posOld, pos8, vel8;
  posOld.resize(TESTS, Point3(0, 0, 0));
  pos8.resize(TESTS, Point3(1, 0, 0));
  vel8.resize(TESTS, Point3(0, 1, 0));
  {
    prune_cache();
    auto testQ = [&pos8, &vel8, dt]() {
      Point3 * /*__restrict*/ pos = (Point3 *)(pos8.data()); // with __restrict it is theoretical limit. But no one really writes
                                                             // __restrict, so we omit it intentionally
      const Point3 * /*__restrict*/ vel = (const Point3 *)(vel8.data()); // with __restrict it is theoretical limit. But no one really
                                                                         // writes __restrict, so we omit it intentionally
      for (int i = 0; i < TESTS; ++i)
        pos[i] += dt * vel[i];
    };
    totalTime = 0, bestTime = ~uint64_t(0);
    for (int i = 0; i < Q_CACHE_CNT; ++i)
    {
      prune_cache();
      reft = profile_ref_ticks();
      testQ();
      const uint64_t ctime = profile_ref_ticks() - reft;
      bestTime = min(ctime, bestTime);
      totalTime += ctime;
    }
    debug("(no cache) speed limit avg %gus, best =%gus", double(totalTime) / int(Q_CACHE_CNT) / profiler_ticks_to_us,
      double(bestTime) / profiler_ticks_to_us);

    totalTime = 0, bestTime = ~0ULL;
    for (int i = 0; i < Q_CNT; ++i)
    {
      reft = profile_ref_ticks();
      testQ();
      const uint64_t ctime = profile_ref_ticks() - reft;
      bestTime = min(ctime, bestTime);
      totalTime += ctime;
    }
    debug("(cached) speed limit avg %gus, best =%gus", double(totalTime) / int(Q_CNT) / profiler_ticks_to_us,
      double(bestTime) / profiler_ticks_to_us);
  }
  {
    eastl::vector<TestEntity> tests(TESTS);
    prune_cache();
    auto testQ = [&tests, dt]() {
      for (auto &t : tests)
        t.p += dt * t.v;
    };
    reft = profile_ref_ticks();
    uint64_t result_time = 0;
    for (int i = 0; i < Q_CACHE_CNT; ++i)
    {
      prune_cache();
      reft = profile_ref_ticks();
      testQ();
      result_time += profile_ref_ticks() - reft;
    }
    debug("(no cache)speed limit with entity %gus", double(result_time) / int(Q_CACHE_CNT) / profiler_ticks_to_us);

    reft = profile_ref_ticks();
    for (int i = 0; i < Q_CNT; ++i)
      testQ();
    debug("(cached) speed limit with entity avg %gus", double(profile_ref_ticks() - reft) / int(Q_CNT) / profiler_ticks_to_us);
  }
  {
    eastl::vector<eastl::unique_ptr<TestEntity>> tests;
    reft = profile_ref_ticks();
    tests.resize(TESTS);
    for (auto &t : tests)
      t.reset(new TestEntity(&t - tests.begin()));
    debug("best possible (single alloc + ptrs) create time = %d us", profile_time_usec(reft));
    tests.clear();
    tests.shrink_to_fit();
    reft = profile_ref_ticks();
    for (int i = 0; i < TESTS; ++i)
      tests.emplace_back(new TestEntity(i));
    debug("best possible (grow ptrs) create time = %d us", profile_time_usec(reft));
    // we shouldn't assume all of them were allocated simultaneously, so allocator returns ordered. that's optimistic scenario
    // to simulate real environment, shuffle data
    struct Rand
    {
      uint32_t operator()(uint32_t n) { return (uint32_t)(grnd() % n); }
    };
    Rand r;
    eastl::random_shuffle(tests.begin(), tests.end(), r);
    prune_cache();
    auto testQ = [&tests, dt]() {
      for (auto &t : tests)
        t->p += dt * t->v;
    };
    auto testVQ = [&tests, dt]() {
      for (auto &t : tests)
        t->update(dt);
    };
    reft = profile_ref_ticks();
    int64_t result_time = 0;
    for (int i = 0; i < Q_CACHE_CNT; ++i)
    {
      prune_cache();
      reft = profile_ref_ticks();
      testQ();
      result_time += profile_ref_ticks() - reft;
    }
    debug("(no cache)speed limit with ptr entity %gus", double(result_time) / int(Q_CACHE_CNT) / profiler_ticks_to_us);

    reft = profile_ref_ticks();
    for (int i = 0; i < Q_CNT; ++i)
      testQ();
    debug("(cached) speed limit with ptr entity avg %gus", double(profile_ref_ticks() - reft) / int(Q_CNT) / profiler_ticks_to_us);

    reft = profile_ref_ticks();
    result_time = 0;
    for (int i = 0; i < Q_CACHE_CNT; ++i)
    {
      prune_cache();
      reft = profile_ref_ticks();
      testVQ();
      result_time += profile_ref_ticks() - reft;
    }
    debug("(no cache)speed limit with virtual call ptr entity %gus", double(result_time) / int(Q_CACHE_CNT) / profiler_ticks_to_us);

    reft = profile_ref_ticks();
    for (int i = 0; i < Q_CNT; ++i)
      testVQ();
    debug("(cached) speed limit with virtual call ptr entity avg %gus",
      double(profile_ref_ticks() - reft) / int(Q_CNT) / profiler_ticks_to_us);
    {
      reft = profile_ref_ticks();
      tests.clear();
      tests.shrink_to_fit();
      debug("single destroy ptr entity avg %gus", double(profile_ref_ticks() - reft) / profiler_ticks_to_us);
    }
  }
  /*{
    prune_cache();
    reft = profile_ref_ticks();
    Point3 * __restrict pos = (Point3 *__restrict )(pos8.data());
    Point3 * __restrict vel = (Point3 *__restrict )(vel8.data());
    int changed = 0;
    for (int i = 0; i < TESTS; ++i)
    {
      auto opos = pos[i];
      pos[i] += dt*vel[i];
      if (pos[i] != opos)
        changed++;
    }
    debug("track change inplace %dus, ret= %d", profile_time_usec(reft), changed);
  }

  {
    prune_cache();
    reft = profile_ref_ticks();
    Point3 * __restrict pos = (Point3 *__restrict )(pos8.data());
    Point3 * __restrict opos = (Point3 *__restrict )(posOld.data());
    int changed = 0;
    for (int i = 0; i < TESTS; ++i)
      if (pos[i] != opos[i])
        changed++;
    debug("track change stream %dus, ret= %d", profile_time_usec(reft), changed);
  }*/
  // debug("float_component = %f", g_entity_mgr->get<float>(eid, ECS_HASH("float_component")));
  test_specialized_nequal();
  test_update_template_values();
  test_update_template_descendants();
  test_diamond_dag();
  benchmark_cidx_map();
  test_standalone_template_validation();
  benchmark_metadata_scale();
  test_churn_gc();
  test_width_guards();
  g_entity_mgr.demandDestroy();
  TIME_PROFILER_SHUTDOWN();
  return 0;
}
extern bool dgs_execute_quiet;

int DagorWinMain(int)
{
  dgs_execute_quiet = true;
#if TIME_PROFILER_ENABLED
  da_profiler::set_mode(0);
#endif
  measure_cpu_freq();
  void init_profile_timer();
  init_profile_timer();
  start_classic_debug_system("debug", false);
  dd_get_fname(""); //== pull in directoryService.obj
  printf("ECS3.0\n");
  debug_flush(true);
  check_string_relocatable();
  dd_add_base_path("");

  DagorHwException::setHandler("main");
  int retcode = 0;

  DAGOR_TRY { retcode = myMain(); }
  DAGOR_CATCH(DagorException e)
  {
#ifdef DAGOR_EXCEPTIONS_ENABLED
    DagorHwException::reportException(e, true);
#endif
    printf("exception");
    return 1;
  }

  DagorHwException::cleanup();
  return retcode;
}

#if _TARGET_PC_WIN
#include <windows.h>
#include <startup/dag_leakDetector.inc.cpp>
#endif
int os_message_box(const char *, const char *, int) { return 0; }
#include <startup/dag_mainCon.inc.cpp>
