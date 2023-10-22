#include <ecs/game/generic/grid.h>
#include <ecs/rendInst/riExtra.h>
#include <ecs/phys/collRes.h>
#include <ecs/phys/physEvents.h>
#include <ecs/anim/anim.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/entityId.h>
#include <daECS/net/time.h>
#include <EASTL/intrusive_list.h>
#include <EASTL/map.h>
#include <rendInst/rendInstExtra.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_bounds3.h>
#include <math/dag_mathUtils.h>
#include <math/dag_math3d.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <debug/dag_debug3d.h>
#include <ADT/spatialHash.h>
#include <perfMon/dag_cpuFreq.h>
#include <memory/dag_fixedBlockAllocator.h>
#include "gridEvent.h"
#include <ecs/delayedAct/actInThread.h>

ECS_REGISTER_EVENT(CmdUpdateGrid);

ECS_DECLARE_BOXED_TYPE(GridHolder);
ECS_REGISTER_BOXED_TYPE(GridHolder, nullptr);
ECS_AUTO_REGISTER_COMPONENT(GridHolder, "grid_holder", nullptr, 0);

CONSOLE_BOOL_VAL("grid", draw_bsph, false);

template <typename Callable>
static void find_grid_holder_by_type_ecs_query(Callable c);
template <typename Callable>
static void get_grid_holder_from_eid_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
static void all_grid_holders_ecs_query(Callable fn);
template <typename Callable>
static void all_grid_holders_with_type_ecs_query(Callable fn);
template <typename Callable>
static void all_grid_objects_ecs_query(Callable fn);
template <typename Callable>
static void all_doors_ecs_query(Callable fn);

static ecs::HashedConstString invalid_grid_holder_hints[] = {ECS_HASH("human"), ECS_HASH("humans"), ECS_HASH("vehicle"),
  ECS_HASH("vehicles"), ECS_HASH("stationary_guns"), ECS_HASH("interactable"), ECS_HASH("loot"), ECS_HASH("attract_points"),
  ECS_HASH("zones"), ECS_HASH("doors")};

// logerr spam limit
static int grid_logerr_counter = 0;
static const int grid_logerr_before_rate_limit = 5;
static float grid_logerr_next = 0.f;
static const float grid_logerr_interval = 10.f;

DAGOR_NOINLINE static void grid_logerr(const char *fmt, ...)
{
  if (++grid_logerr_counter > grid_logerr_before_rate_limit)
  {
    float curTime = get_sync_time();
    if (curTime < grid_logerr_next)
      return;
    grid_logerr_next = curTime + grid_logerr_interval;
  }
  char buffer[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof buffer, fmt, args);
  logerr(buffer);
  va_end(args);
}

static GridHolder *find_grid_holder_by_hash_impl(uint32_t grid_name_hash)
{
  GridHolder *res = nullptr;
  find_grid_holder_by_type_ecs_query([&res, grid_name_hash](GridHolder &grid_holder, int grid_holder__typeHash) {
    if (grid_name_hash == grid_holder__typeHash)
    {
      res = &grid_holder;
      return ecs::QueryCbResult::Stop;
    }
    return ecs::QueryCbResult::Continue;
  });
  return res;
}

GridHolder *find_grid_holder(ecs::EntityId gridh_ent)
{
  GridHolder *res = nullptr;
  get_grid_holder_from_eid_ecs_query(gridh_ent, [&](GridHolder &grid_holder) { res = &grid_holder; });
  if (EASTL_UNLIKELY(!res))
    grid_logerr("Invalid grid holder eid %u passed to grid query", ecs::entity_id_t(gridh_ent));
  return res;
}

GridHolder *find_grid_holder(uint32_t grid_name_hash)
{
  GridHolder *holder = find_grid_holder_by_hash_impl(grid_name_hash);
  if (EASTL_LIKELY(holder))
    return holder;
  for (ecs::HashedConstString hint : invalid_grid_holder_hints)
  {
    if (hint.hash == grid_name_hash)
    {
      grid_logerr("Invalid grid type %s passed to grid query, grid holder not found", hint.str);
      return nullptr;
    }
  }
  grid_logerr("Invalid grid type hash %u passed to grid query, grid holder not found", grid_name_hash);
  return nullptr;
}

GridHolder *find_grid_holder(ecs::HashedConstString grid_name_hash)
{
  GridHolder *holder = find_grid_holder_by_hash_impl(grid_name_hash.hash);
  if (EASTL_LIKELY(holder))
    return holder;
  grid_logerr("Invalid grid type %s passed to grid query, grid holder not found", grid_name_hash.str);
  return nullptr;
}

void for_each_grid_holder(const eastl::function<void(const GridHolder &)> &cb)
{
  all_grid_holders_ecs_query([&](const GridHolder &grid_holder) { cb(grid_holder); });
}

ECS_ON_EVENT(on_disappear)
void grid_holder_destroyed_es_event_handler(const ecs::Event &, const GridHolder &grid_holder)
{
  all_grid_objects_ecs_query([&](GridObjComponent &grid_obj) {
    if (grid_obj.ownerGrid == &grid_holder)
    {
      grid_obj.removeFromGrid();
      grid_obj.ownerGrid = nullptr;
    }
  });
}

GridObjComponent::GridObjComponent(const ecs::EntityManager &mgr, ecs::EntityId eid_)
{
  wbsph = v_zero();
  eid = eid_;
  inserted = false;
  hidden = mgr.getOr(eid_, ECS_HASH("grid_obj__hidden"), 0) != 0;
  const char *gridType = mgr.getOr(eid_, ECS_HASH("grid_obj__gridType"), "default");
  uint32_t gridTypeHash = str_hash_fnv1(gridType);
  allocatorKey = uint16_t(gridTypeHash);
  ownerGrid = find_grid_holder_by_hash_impl(gridTypeHash);
}

GridObjComponent::~GridObjComponent()
{
  if (inserted)
  {
    removeFromGrid();
    logerr("GridObjComponent was inserted in grid holder in destructor, it should be already removed in on_disappear es");
  }
}

void GridObjComponent::updatePos(vec4f new_pos, float new_bounding_rad)
{
  if (ownerGrid)
  {
    vec4f oldWbsph = wbsph;
    wbsph = v_perm_xyzd(new_pos, v_splats(new_bounding_rad));
    if (EASTL_UNLIKELY(!inserted))
    {
      inserted = true;
      ownerGrid->insert(*this, wbsph, new_bounding_rad);
    }
    else
      ownerGrid->update(*this, oldWbsph, wbsph, new_bounding_rad);
  }
}

void GridObjComponent::removeFromGrid()
{
  if (ownerGrid && inserted)
  {
    inserted = false;
    ownerGrid->erase(*this, wbsph);
  }
}

class GridObjCTM final : public ecs::ComponentTypeManager
{
public:
  typedef GridObjComponent component_type;

  GridObjCTM() {}

  ~GridObjCTM() { clear(); }

  void clear() { allocators.clear(); }

  void create(void *d, ecs::EntityManager &mgr, ecs::EntityId eid, const ecs::ComponentsMap &, ecs::component_index_t) override
  {
    component_type gridObj(mgr, eid);
    FixedBlockAllocator &allocator = allocators[gridObj.allocatorKey];
    if (!allocator.isInited())
      allocator.init(sizeof(component_type), (4096 - 16) / sizeof(component_type));
    void *p = allocator.allocateOneBlock();
    *(ecs::PtrComponentType<component_type>::ptr_type)d = new (p, _NEW_INPLACE) component_type(eastl::move(gridObj));
  }

  void destroy(void *d) override
  {
    auto pptr = (ecs::PtrComponentType<component_type>::ptr_type)d;
    G_ASSERT(pptr);
    auto al = allocators.find((*pptr)->allocatorKey);
    G_ASSERT(al != allocators.end());
    FixedBlockAllocator &allocator = al->second;
    eastl::destroy_at(*pptr);
    G_VERIFY(allocator.freeOneBlock(*pptr));
    *pptr = nullptr;
  }

  eastl::vector_map<uint32_t, FixedBlockAllocator> allocators;
};

G_STATIC_ASSERT(!ecs::ComponentTypeInfo<GridObjComponent>::can_be_tracked);
ECS_REGISTER_TYPE_BASE(GridObjComponent, ecs::ComponentTypeInfo<GridObjComponent>::type_name, nullptr,
  &ecs::CTMFactory<GridObjCTM>::create, &ecs::CTMFactory<GridObjCTM>::destroy, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(GridObjComponent, "grid_obj", nullptr, 0, "?animchar", "?ri_extra", "?collres", "transform");

ECS_TAG(server)
ECS_ON_EVENT(on_appear)
static void grid_obj_verify_server_es(const ecs::Event &, ecs::EntityId eid, GridObjComponent &grid_obj,
  ecs::string grid_obj__gridType = "default")
{
  if (!grid_obj.ownerGrid)
    logerr("can't find grid of <%s> type in %i:<%s>", grid_obj__gridType.c_str(), (ecs::entity_id_t)eid,
      g_entity_mgr->getEntityTemplateName(eid));
}

ECS_ON_EVENT(on_appear)
ECS_REQUIRE(GridObjComponent &grid_obj)
void grid_obj_init_tm_scale_es_event_handler(const ecs::Event &, const TMatrix &transform, float &grid_obj__fixedTmScale)
{
  mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, transform.array);
  grid_obj__fixedTmScale = v_extract_x(v_mat44_max_scale43_x(tm));
}

ECS_TRACK(transform)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(EventOnEntityTeleported)
ECS_ON_EVENT(CmdUpdateGrid)
ECS_REQUIRE_NOT(ecs::Tag grid_obj__updateAlways) // TODO: remove
ECS_AFTER(riextra_create_es)
ECS_AFTER(animchar_act_on_phys_teleport_es)
void grid_obj_update_es_event_handler(const ecs::Event &, GridObjComponent &grid_obj, const TMatrix &transform,
  const CollisionResource *collres = nullptr, const RiExtraComponent *ri_extra = nullptr, float grid_obj__fixedTmScale = -1.f,
  const ecs::Tag *grid_obj__scaledBoxBounding = nullptr)
{
  if (EASTL_UNLIKELY(grid_obj.hidden))
    return;

  mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, transform.array);
  vec4f newPos = tm.col3;
  float scaledBoundingRad = v_extract_w(grid_obj.wbsph);
  if (!collres && ri_extra)
    collres = rendinst::getRIGenExtraCollRes(rendinst::handle_to_ri_type(ri_extra->handle));
  if (collres)
  {
    newPos = v_mat44_mul_vec3p(tm, collres->vBoundingSphere);
    if (EASTL_UNLIKELY(grid_obj__fixedTmScale < 0.f || scaledBoundingRad < FLT_EPSILON))
    {
      float boundingRad = sqrtf(v_extract_w(collres->vBoundingSphere));
      float boundingScale = grid_obj__fixedTmScale;
      if (EASTL_UNLIKELY(boundingScale < 0.f))
        boundingScale = v_extract_x(v_mat44_max_scale43_x(tm));
      scaledBoundingRad = max(boundingRad * boundingScale, FLT_EPSILON);

#if DAGOR_DBGLEVEL > 0
      if (EASTL_UNLIKELY(isnan(boundingRad) || boundingRad > MAX_VALID_BOUNDING_RADIUS))
      {
        grid_logerr("Grid entity %i (%s) update error: bounding radius %f is too big or NaN", ecs::entity_id_t(grid_obj.eid),
          g_entity_mgr->getEntityTemplateName(grid_obj.eid), boundingRad);
        boundingRad = FLT_EPSILON;
      }
#endif
    }
  }
  else if (grid_obj__scaledBoxBounding)
  {
    float boundingRad = 0.5f;
    float boundingScale = grid_obj__fixedTmScale;
    if (EASTL_UNLIKELY(boundingScale < 0.f))
      boundingScale = v_extract_x(v_mat44_max_scale43_x(tm));
    scaledBoundingRad = max(boundingRad * boundingScale, FLT_EPSILON);
  }

  grid_obj.updatePos(newPos, scaledBoundingRad);
}

void grid_obj_update_with_animchar(GridObjComponent &grid_obj, const TMatrix &transform, const GeomNodeTree *geom_node_tree,
  const CollisionResource &collres, float grid_obj__fixedTmScale)
{
  if (!grid_obj.hidden)
  {
    mat44f tm;
    v_mat44_make_from_43cu_unsafe(tm, transform.array);
    float scaledBoundingRad = v_extract_w(grid_obj.wbsph);
    if (EASTL_UNLIKELY(grid_obj__fixedTmScale < 0.f || scaledBoundingRad < FLT_EPSILON))
    {
      float boundingScale = grid_obj__fixedTmScale;
      if (EASTL_UNLIKELY(boundingScale < 0.f))
        boundingScale = v_extract_x(v_mat44_max_scale43_x(tm));
      float boundingRad = sqrtf(v_extract_w(collres.vBoundingSphere));
      if (EASTL_UNLIKELY(isnan(boundingRad) || boundingRad > MAX_VALID_BOUNDING_RADIUS))
      {
        grid_logerr("Grid entity %i (%s) update error: bounding radius %f is too big or NaN", ecs::entity_id_t(grid_obj.eid),
          g_entity_mgr->getEntityTemplateName(grid_obj.eid), boundingRad);
        boundingRad = FLT_EPSILON;
      }
      scaledBoundingRad = max(boundingRad * boundingScale, FLT_EPSILON);
    }
    vec3f newPos = collres.getWorldBoundingSphere(tm, geom_node_tree);
    grid_obj.updatePos(newPos, scaledBoundingRad);
  }
}

ECS_AFTER(animchar_es)
ECS_REQUIRE(const ecs::Tag grid_obj__updateAlways)
void grid_obj_update_with_animchar_es(const ParallelUpdateFrameDelayed &, GridObjComponent &grid_obj, const TMatrix &transform,
  const AnimV20::AnimcharBaseComponent *animchar, const CollisionResource &collres, float grid_obj__fixedTmScale = -1.f)
{
  grid_obj_update_with_animchar(grid_obj, transform, animchar ? &animchar->getNodeTree() : nullptr, collres, grid_obj__fixedTmScale);
}

// TODO: remove that duplicating es
ECS_ON_EVENT(EventOnEntityTeleported)
ECS_REQUIRE(const ecs::Tag grid_obj__updateAlways)
ECS_AFTER(animchar_act_on_phys_teleport_es)
void grid_obj_update_with_animchar_evt_es(const ecs::Event &, GridObjComponent &grid_obj, const TMatrix &transform,
  const AnimV20::AnimcharBaseComponent &animchar, const CollisionResource &collres, float grid_obj__fixedTmScale = -1.f)
{
  grid_obj_update_with_animchar(grid_obj, transform, &animchar.getNodeTree(), collres, grid_obj__fixedTmScale);
}

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(const TMatrix &transform)
void grid_obj_disappear_es(const ecs::Event &, GridObjComponent &grid_obj) { grid_obj.removeFromGrid(); }

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(const TMatrix &transform)
ECS_REQUIRE(const CollisionResource &collres)
ECS_REQUIRE(const AnimV20::AnimcharBaseComponent &animchar)
void grid_obj_disappear_animchar_es(const ecs::Event &, GridObjComponent &grid_obj) { grid_obj.removeFromGrid(); }

ECS_ON_EVENT(on_appear)
ECS_TRACK(grid_obj__hidden)
ECS_REQUIRE(const TMatrix &transform)
static void grid_obj_hide_es_event_handler(const ecs::Event &, GridObjComponent &grid_obj, int grid_obj__hidden)
{
  bool isHidden = grid_obj__hidden != 0;
  if (grid_obj.hidden == isHidden)
    return;
  grid_obj.hidden = isHidden;
  if (isHidden)
    grid_obj.removeFromGrid();
}

ECS_ON_EVENT(on_appear)
static void grid_holder_created_es(const ecs::Event &, GridHolder &grid_holder, const ecs::string &grid_holder__type,
  int &grid_holder__typeHash, int grid_holder__cellSize = SPATIAL_HASH_DEFAULT_CELL_SIZE)
{
  grid_holder__typeHash = mem_hash_fnv1(grid_holder__type.c_str(), grid_holder__type.length());
  grid_holder.setCellSizeWithoutObjectsReposition(grid_holder__cellSize);
}

template <typename Callable>
static void grid_object_assigned_grid_ecs_query(Callable c);

ECS_TAG(netClient)
ECS_ON_EVENT(on_appear)
static void grid_holder_created_client_es(const ecs::Event &, GridHolder &grid_holder, const ecs::string &grid_holder__type)
{
  grid_object_assigned_grid_ecs_query([&](ecs::EntityId eid, GridObjComponent &grid_obj, ecs::string grid_obj__gridType = "default") {
    if (!grid_obj.ownerGrid && grid_obj__gridType == grid_holder__type)
    {
      grid_obj.ownerGrid = &grid_holder;
      g_entity_mgr->sendEvent(eid, CmdUpdateGrid());
    }
  });
}

static void save_grid_map(FILE *fp, const GridHolder &grid_holder, const ecs::string &grid_holder__type)
{
  GridHolder::GridCellsStats stats;
  grid_holder.getCellsStats(stats);
  double avgObjectSize = 0.0;
  BBox3 maxBox(Point3(-FLT_MAX, -FLT_MAX, -FLT_MAX), Point3(FLT_MAX, FLT_MAX, FLT_MAX));
  grid_find_entity(grid_holder, maxBox, GridEntCheck::POS, [&](ecs::EntityId, vec3f pos) {
    avgObjectSize += v_extract_w(pos);
    return false;
  });
  avgObjectSize = safediv(avgObjectSize, (double)stats.totalObjects);

  fprintf(fp,
    "Type: %s; Cell size: %u; Empty cells: %.1f%%; Max load: %i; Avg load: %i; "
    "Total objects: %i; Max obj size: %.2f; Avg obj size: %.2f\n",
    grid_holder__type.c_str(), grid_holder.getCellSize(),
    double(stats.totalCells - stats.notEmptyCells) / double(stats.totalCells) * 100.0, // empty cnt
    stats.maxObjectsInCell,
    stats.notEmptyCells ? stats.totalObjects / stats.notEmptyCells : 0, // avg load
    stats.totalObjects, grid_holder.getMaxObjectBoundingRadius(), avgObjectSize);

  for (int z = 0; z < grid_holder.getGridHeight(); z++)
  {
    for (int x = 0; x < grid_holder.getGridWidth(); x++)
    {
      auto &cell = grid_holder.getCellByIndex(x, z);
      fprintf(fp, "[%3i]", int(cell.size()));
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "\n");
}

static bool grid_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("grid", "save_map", 1, 1)
  {
    const char *fileName = "grid_map.txt";
    FILE *fp = fopen(fileName, "wt");
    if (!fp)
    {
      logerr("cant open '%s' file for writing", fileName);
      return false;
    }

    all_grid_holders_with_type_ecs_query(
      [&](GridHolder &grid_holder, const ecs::string &grid_holder__type) { save_grid_map(fp, grid_holder, grid_holder__type); });

    fclose(fp);
  }
  CONSOLE_CHECK_NAME("grid", "test_cell_size_map", 2, 2)
  {
    const char *gridType = argv[1];
    GridHolder *gridHolder = find_grid_holder(ECS_HASH_SLOW(gridType));
    if (!gridHolder)
    {
      logerr("cant find grid type %s holder", gridType);
      return false;
    }

    char fileName[260];
    snprintf(fileName, sizeof fileName, "grid_map_%s.txt", gridType);
    FILE *fp = fopen(fileName, "wt");
    if (!fp)
    {
      logerr("cant open '%s' file for writing", fileName);
      return false;
    }

    unsigned curCellSize = gridHolder->getCellSize();
    for (unsigned cellSize = 64; cellSize >= 4; cellSize /= 2)
    {
      gridHolder->setCellSizeSlow(cellSize);
      save_grid_map(fp, *gridHolder, gridType);
    }
    gridHolder->setCellSizeSlow(curCellSize);
    fclose(fp);
  }
  CONSOLE_CHECK_NAME("grid", "perf_test", 1, 2)
  {
    const char *fileName = "grid_perf.txt";
    FILE *fp = fopen(fileName, "at");
    if (!fp)
    {
      logerr("cant open '%s' file for writing", fileName);
      return false;
    }

    float searchDist = 16.f;
    if (argc == 2)
      searchDist = console::to_real(argv[1]);
    GridHolder &gridHolder = *find_grid_holder(ECS_HASH("doors"));
    const Point3 niceNormandyVillage(-146.909f, 16.5194f, -133.962f);
    vec4f vQueryCenter = v_ldu(&niceNormandyVillage.x);
    const BSphere3 bsph(niceNormandyVillage, searchDist);
    BBox3 box3d;
    v_stu_bbox3(box3d, {v_sub(vQueryCenter, v_splats(searchDist)), v_add(vQueryCenter, v_splats(searchDist))});
    const int saveCellSize = gridHolder.getCellSize();
    const int launchCount = 100000;
    const int minCellSize = 4;
    const int maxCellSize = 64;
    volatile bool volatile_var;

    int totalDoorsCount = 0;
    all_doors_ecs_query([&](const TMatrix &transform, float net__scopeDistanceSq ECS_REQUIRE(bool isDoor)) {
      G_UNUSED(transform);
      G_UNUSED(net__scopeDistanceSq);
      totalDoorsCount++;
    });

    for (unsigned cellSize = maxCellSize; cellSize >= minCellSize; cellSize /= 2)
    {
      gridHolder.setCellSizeSlow(cellSize);
      int objectsCount = 0;
      unsigned cellsIter = 0;
      grid_find_in_box_by_pos(gridHolder, box3d, [&](const GridObject *) {
        objectsCount++;
        return false;
      });
      fprintf(fp, "Run 3 tests, each %i times. searchDist=%.1f. Grid cell size=%u. Query %u cells, objects=%i (%.1f%%).\n",
        launchCount, searchDist, cellSize, cellsIter, objectsCount, double(objectsCount) / totalDoorsCount * 100.0);
      debug("Run 3 tests, each %i times. searchDist=%.1f. Grid cell size=%u. Query %u cells, objects=%i (%.1f%%).", launchCount,
        searchDist, cellSize, cellsIter, objectsCount, double(objectsCount) / totalDoorsCount * 100.0);

      // Safe grid query
      {
        int64_t ref = ref_time_ticks();
        for (int i = 0; i < launchCount; i++)
        {
          grid_find_entity(gridHolder, bsph, GridEntCheck::POS, [&](ecs::EntityId eid, vec3f pos) {
            volatile_var = v_extract_x(v_length3_sq_x(v_sub(pos, vQueryCenter))) < ECS_GET(float, eid, net__scopeDistanceSq);
            return false;
          });
        }
        int result = get_time_usec(ref);
        fprintf(fp, "Safe grid query + distance check: %.2f msec\n", result / 1000.0);
        debug("Safe grid query + distance check: %.2f msec", result / 1000.0);
      }
      // Unsafe grid query
      /*{
        int64_t ref = ref_time_ticks();
        for (int i = 0; i < launchCount; i++)
        {
          find_entity_in_grid_unsafe_impl(gridHolder, bsph, [&](ecs::EntityId eid, vec3f pos)
          {
            volatile_var = v_extract_x(v_length3_sq_x(v_sub(pos, vQueryCenter))) < ECS_GET(float, eid, net__scopeDistanceSq);
            return false;
          });
        }
        int result = get_time_usec(ref);
        fprintf(fp, "Unsafe grid query + distance check: %.2f msec\n", result / 1000.0);
      }*/
      // ECS query
      {
        int64_t ref = ref_time_ticks();
        for (int i = 0; i < launchCount / 100; i++)
        {
          all_doors_ecs_query([&](const TMatrix &transform, float net__scopeDistanceSq ECS_REQUIRE(bool isDoor)) {
            vec3f vPos = v_ldu(&transform.getcol(3).x);
            volatile_var = v_extract_x(v_length3_sq_x(v_sub(vPos, vQueryCenter))) < net__scopeDistanceSq;
          });
        }
        int result = get_time_usec(ref);
        fprintf(fp, "ECS query + distance check: %.2f msec\n", result / 10.0);
        debug("ECS query + distance check: %.2f msec", result / 10.0);
      }
      fprintf(fp, cellSize == minCellSize ? "\n****************************\n" : "____________________________\n\n");
      debug(cellSize == minCellSize ? "\n****************************" : "____________________________\n");
    }
    fprintf(fp, "\n");
    fclose(fp);
    gridHolder.setCellSizeSlow(saveCellSize);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(grid_console_handler);

ECS_NO_ORDER
ECS_TAG(render, dev)
ECS_REQUIRE(eastl::true_type camera__active)
static void grid_debug_draw_es(const ecs::UpdateStageInfoRenderDebug &, const TMatrix &transform)
{
  if (!draw_bsph.get())
    return;
  begin_draw_cached_debug_lines();
  for_each_grid_holder([&](const GridHolder &grid_holder) {
    BSphere3 bsph(transform.getcol(3), 64.f);
    grid_find_entity(grid_holder, bsph, GridEntCheck::POS, [](ecs::EntityId, vec3f wbsph) {
      float radius = v_extract_w(wbsph);
      if (radius > 0.1f)
      {
        Point3_vec4 center;
        v_st(&center.x, wbsph);
        draw_cached_debug_sphere(center, radius, E3DCOLOR_MAKE(255, 255, 32, 255));
      }
      return false;
    });
  });
  end_draw_cached_debug_lines();
}
