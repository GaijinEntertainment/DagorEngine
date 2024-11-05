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

static ecs::EntitySystemDesc kinematics_es_desc("kinematics_es", ecs::EntitySystemOps(kinematics_es_all, nullptr),
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
  debug("(no cached)query in %gus, best =%gus", double(totalTime) / Q_CACHE_CNT / profiler_ticks_to_us,
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
  debug("(cached)query in %gus, best =%gus", double(totalTime) / ECS_RUNS / profiler_ticks_to_us,
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
      Point3 * /*__restrict*/ pos = (Point3 *)(pos8.data()); // with __restrict it is theoretical limit. But noone really writes
                                                             // __restrict, so we omit it intentionally
      const Point3 * /*__restrict*/ vel = (const Point3 *)(vel8.data()); // with __restrict it is theoretical limit. But noone really
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
    debug("(no cache) speed limit avg %gus, best =%gus", double(totalTime) / Q_CACHE_CNT / profiler_ticks_to_us,
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
    debug("(cached) speed limit avg %gus, best =%gus", double(totalTime) / Q_CNT / profiler_ticks_to_us,
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
    debug("(no cache)speed limit with entity %gus", double(result_time) / Q_CACHE_CNT / profiler_ticks_to_us);

    reft = profile_ref_ticks();
    for (int i = 0; i < Q_CNT; ++i)
      testQ();
    debug("(cached) speed limit with entity avg %gus", double(profile_ref_ticks() - reft) / Q_CNT / profiler_ticks_to_us);
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
    debug("(no cache)speed limit with ptr entity %gus", double(result_time) / Q_CACHE_CNT / profiler_ticks_to_us);

    reft = profile_ref_ticks();
    for (int i = 0; i < Q_CNT; ++i)
      testQ();
    debug("(cached) speed limit with ptr entity avg %gus", double(profile_ref_ticks() - reft) / Q_CNT / profiler_ticks_to_us);

    reft = profile_ref_ticks();
    result_time = 0;
    for (int i = 0; i < Q_CACHE_CNT; ++i)
    {
      prune_cache();
      reft = profile_ref_ticks();
      testVQ();
      result_time += profile_ref_ticks() - reft;
    }
    debug("(no cache)speed limit with virtual call ptr entity %gus", double(result_time) / Q_CACHE_CNT / profiler_ticks_to_us);

    reft = profile_ref_ticks();
    for (int i = 0; i < Q_CNT; ++i)
      testVQ();
    debug("(cached) speed limit with virtual call ptr entity avg %gus",
      double(profile_ref_ticks() - reft) / Q_CNT / profiler_ticks_to_us);
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
