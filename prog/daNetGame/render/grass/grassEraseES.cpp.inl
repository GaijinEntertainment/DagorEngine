// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_bounds2.h>
#include <render/grass_eraser_consts.hlsli>

#define INSIDE_RENDERER 1 // fixme: move to jam

#include <render/world/private_worldRenderer.h> //need only for invalidateGI
#include "grassRenderer.h"


void GrassRenderer::setGrassErasers(int count, const Point4 *erasers)
{
  if (grassErasers.empty())
    return;
  uint32_t grassEraserCounter = min(uint32_t(count), MAX_GRASS_ERASERS);
  eastl::copy_n(erasers, grassEraserCounter, grassErasers.begin());
  grassErasersIndexToAdd = grassEraserCounter;
  grassErasersActualSize = grassErasersIndexToAdd;
  grassErasersIndexToAdd %= MAX_GRASS_ERASERS;
  grassErasersModified = true;
  invalidateGrass(true);
}

void GrassRenderer::addGrassEraser(const Point3 &world_pos, float radius)
{
  if (grassErasers.empty())
    return;
  grassErasers[grassErasersIndexToAdd] = Point4::xyzV(world_pos, radius);
  grassErasersIndexToAdd++;
  grassErasersActualSize = max(grassErasersIndexToAdd, grassErasersActualSize);
  grassErasersIndexToAdd %= MAX_GRASS_ERASERS;
  grassErasersModified = true;
  const auto bbox = BBox2(Point2(world_pos.x, world_pos.z), radius * 2.0);
  invalidateGrassBoxes({&bbox, 1});
}

void GrassRenderer::clearGrassErasers()
{
  grassErasers.clear();
  invalidateGrass(true);
}

template <typename Callable>
static void get_grass_render_ecs_query(Callable c);

void erase_grass(const Point3 &world_pos, float radius)
{
  if (WorldRenderer *renderer = (WorldRenderer *)get_world_renderer())
  {
    get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.addGrassEraser(world_pos, radius); });
    // due to the size of the grass, gi is affected in a larger radius
    float giRad = radius + 1;
    BBox3 modelBox(Point3(0, 0, 0), giRad);
    TMatrix tm;
    tm.identity();
    tm.setcol(3, world_pos);
    BBox3 approxBbox(world_pos, giRad);
    renderer->invalidateGI(modelBox, tm, approxBbox);
  }
}

ECS_ON_EVENT(on_appear)
static void grass_eraser_es(const ecs::Event &, const ecs::Point4List &grass_erasers__spots)
{
  get_grass_render_ecs_query(
    [&](GrassRenderer &grass_render) { grass_render.setGrassErasers(grass_erasers__spots.size(), grass_erasers__spots.data()); });
}

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Point4List grass_erasers__spots)
static void grass_eraser_destroy_es(const ecs::Event &)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.clearGrassErasers(); });
}

#include <util/dag_console.h>
#include <camera/sceneCam.h>
#include <gamePhys/collision/collisionLib.h>

static bool grass_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("grass", "erase", 1, 2)
  {
    float radius = 5;
    if (argc > 1)
      radius = atof(argv[1]);
    ecs::EntityId camera = get_cur_cam_entity();

    TMatrix camT = g_entity_mgr->getOr(camera, ECS_HASH("transform"), TMatrix::IDENT);
    Point3 camPos = camT.getcol(3);
    Point3 camDir = camT.getcol(2);

    float dist = 10000;
    if (dacoll::traceray_normalized(camPos, camDir, dist, nullptr, nullptr))
    {
      Point3 p = camPos + camDir * dist;
      erase_grass(p, radius);
      console::print_d("Creating grass eraser at (%f, %f, %f) with radius=%f", p.x, p.y, p.z, dist);
    }
    else
      console::print("Look at the ground to place a grass eraser");
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(grass_console_handler);