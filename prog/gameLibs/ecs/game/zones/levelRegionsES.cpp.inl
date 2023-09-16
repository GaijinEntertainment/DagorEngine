#include <ecs/game/zones/levelRegions.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/updateStage.h>

#include <util/dag_convar.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>

#include <quirrel/sqModules/sqModules.h>
#include <quirrel/bindQuirrelEx/autoBind.h>

#include <math/dag_math2d.h>
#include <levelSplines/splineRegions.h>

ECS_REGISTER_RELOCATABLE_TYPE(LevelRegions, nullptr);
ECS_AUTO_REGISTER_COMPONENT(LevelRegions, "level_regions", nullptr, 0);

static CONSOLE_BOOL_VAL("app", debug_regions_border, false);
static CONSOLE_BOOL_VAL("app", debug_regions_convexes, false);

ECS_NO_ORDER
ECS_TAG(render, dev)
static void level_regions_debug_es(const ecs::UpdateStageInfoRenderDebug &, const LevelRegions &level_regions)
{
  if (!debug_regions_border.get() && !debug_regions_convexes.get())
    return;
  begin_draw_cached_debug_lines(false, false);
  int i = 0;
  for (const splineregions::SplineRegion &reg : level_regions)
  {
    // draw borders + 0 && 1 points to visual detection of ccw
    add_debug_text_mark(reg.border[0], String(4, "i%d", 0));
    add_debug_text_mark(reg.border[1], String(4, "i%d", 1));

    if (debug_regions_border.get())
    {
      draw_cached_debug_line(&reg.border[0], reg.border.size(), E3DCOLOR_MAKE(0, 0, 255, 255));
      draw_cached_debug_line(reg.border.back(), reg.border[0], E3DCOLOR_MAKE(0, 0, 255, 255));
      draw_cached_debug_line(&reg.originalBorder[0], reg.originalBorder.size(), E3DCOLOR_MAKE(0, 255, 0, 255));
      draw_cached_debug_line(reg.originalBorder.back(), reg.originalBorder[0], E3DCOLOR_MAKE(0, 255, 0, 255));
      Point3 regionCenter = Point3(0.f, 0.f, 0.f);
      for (const Point3 &pt : reg.border)
        regionCenter += pt;
      regionCenter *= safeinv(float(reg.border.size()));
      add_debug_text_mark(regionCenter, String(64, "%s (%d), %d", i++, reg.name.str(), is_poly_ccw(reg.originalBorder)));
    }
    float h = reg.border[0].y;

    // draw original lines normals
    int ptsCnt = reg.originalBorder.size();
    for (int k = 0; k < ptsCnt; ++k)
    {
      Point2 p0 = Point2::xz(reg.originalBorder[k == 0 ? ptsCnt - 1 : k - 1]);
      Point2 p1 = Point2::xz(reg.originalBorder[k]);
      Point2 p2 = Point2::xz(reg.originalBorder[(k + 1) % ptsCnt]);
      Line2 ln0 = Line2(p0, p1);
      Line2 ln1 = Line2(p1, p2);
      draw_cached_debug_line(reg.originalBorder[k], reg.originalBorder[k] + Point3::xVy((ln0.norm + ln1.norm) / 2, 0.f),
        E3DCOLOR_MAKE(0, 255, 0, 255));
    }

    // draw convexes
    if (debug_regions_convexes.get())
    {
      for (auto &convex : reg.convexes)
      {
        float d;
        Point2 norm, st0, st1, st2, end0, end1, end2, int01, int12, int20;
        d = convex.lines[0].d;
        norm = convex.lines[0].norm;
        st0 = norm * (-d);
        end0 = st0 + Point2(norm.y, -norm.x);
        d = convex.lines[1].d;
        norm = convex.lines[1].norm;
        st1 = norm * (-d);
        end1 = st1 + Point2(norm.y, -norm.x);
        d = convex.lines[2].d;
        norm = convex.lines[2].norm;
        st2 = norm * (-d);
        end2 = st2 + Point2(norm.y, -norm.x);
        get_lines_intersection(st0, end0, st1, end1, int01);
        get_lines_intersection(st1, end1, st2, end2, int12);
        get_lines_intersection(st2, end2, st0, end0, int20);
        draw_cached_debug_line(Point3::xVy(int01, h), Point3::xVy(int12, h), E3DCOLOR_MAKE(255, 0, 0, 255));
        draw_cached_debug_line(Point3::xVy(int12, h), Point3::xVy(int20, h), E3DCOLOR_MAKE(255, 0, 0, 255));
        draw_cached_debug_line(Point3::xVy(int20, h), Point3::xVy(int01, h), E3DCOLOR_MAKE(255, 0, 0, 255));
      }
    }
  }
  end_draw_cached_debug_lines();
}

void create_level_splines(dag::ConstSpan<levelsplines::Spline> splines)
{
  for (const levelsplines::Spline &spline : splines)
  {
    ecs::Point3List pointList;
    for (const levelsplines::SplinePoint &point : spline.pathPoints)
      pointList.push_back(point.pt);

    ecs::ComponentsInitializer attrs;
    ECS_INIT(attrs, level_spline__name, ecs::string(spline.name.str()));
    ECS_INIT(attrs, level_spline__points, eastl::move(pointList));
    g_entity_mgr->createEntityAsync("level_spline", eastl::move(attrs));
  }
}

void create_level_regions(dag::ConstSpan<levelsplines::Spline> splines, const DataBlock &regions_blk)
{
  if (splines.empty())
    return;
  G_ASSERT(!g_entity_mgr->getSingletonEntity(ECS_HASH("level_regions")));
  LevelRegions regions;
  splineregions::load_regions_from_splines(regions, splines, regions_blk);
  if (!regions.empty())
  {
    ecs::ComponentsInitializer attrs;
    attrs[ECS_HASH("level_regions")] = eastl::move(regions); // Note: intentionally not ECS_INIT since LevelRegions (Tab<>) has
                                                             // different spelling in different clang versions
    g_entity_mgr->createEntityAsync("level_regions", eastl::move(attrs));
  }
}

template <typename Callable>
static void level_splines_ecs_query(Callable c);

void clean_up_level_entities()
{
  debug("clean_up_level_entities - regions, roads, splines.");
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("level_regions")));
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("level_roads")));
  level_splines_ecs_query([](ECS_REQUIRE(ecs::auto_type level_spline__name) ecs::EntityId eid) { g_entity_mgr->destroyEntity(eid); });
}

const splineregions::SplineRegion *get_region_by_pos(const Point2 &pos)
{
  ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH("level_regions"));
  const LevelRegions *level_regions = g_entity_mgr->getNullable<LevelRegions>(eid, ECS_HASH("level_regions"));
  if (level_regions)
    for (const splineregions::SplineRegion &reg : *level_regions)
      if (reg.checkPoint(pos))
        return &reg;
  return nullptr;
}

const char *get_region_name_by_pos(const Point2 &pos)
{
  const splineregions::SplineRegion *reg = get_region_by_pos(pos);
  return reg ? reg->name.str() : nullptr;
}

static SQInteger get_regions(HSQUIRRELVM vm)
{
  ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH("level_regions"));
  const LevelRegions *level_regions = g_entity_mgr->getNullable<LevelRegions>(eid, ECS_HASH("level_regions"));
  if (!level_regions)
  {
    sq_newarray(vm, 0);
    return 1;
  }

  sq_newarray(vm, level_regions->size());
  for (int iRegion = 0; iRegion < level_regions->size(); ++iRegion)
  {
    const splineregions::SplineRegion &reg = (*level_regions)[iRegion];
    Sqrat::Table table(vm);

    Sqrat::Array points(vm, reg.border.size());
    for (int i = 0; i < reg.border.size(); i++)
      points.SetValue(SQInteger(i), reg.border[i]);

    table.SetValue("name", reg.name.str());
    table.SetValue("points", points);
    table.SetValue("isVisible", reg.isVisible);

    sq_pushinteger(vm, iRegion);
    sq_pushobject(vm, table.GetObject());
    G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  }
  return 1;
}


SQ_DEF_AUTO_BINDING_MODULE(bind_level_regions, "game.regions")
{
  Sqrat::Table regionsApi(vm);
  regionsApi.Func("get_region_name_by_pos", get_region_name_by_pos).SquirrelFunc("get_regions", get_regions, 1);
  return regionsApi;
}
