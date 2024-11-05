// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/render/resPtr.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_lockTexture.h>
#include <gamePhys/collision/collisionLib.h>
#include <physMap/physMap.h>
#include <physMap/physMatHtTex.h>
#include <physMap/physMapLoad.h>
#include <util/dag_treeBitmap.h>
#include <util/dag_convar.h>
#include <camera/sceneCam.h>

static CONSOLE_BOOL_VAL("render", debug_physmap_decals, false);
static dag::Vector<Point3> physmap_decal_vertices;
static IPoint2 physmap_selected_grid = IPoint2(-1, -1);

ECS_TAG(render, dev)
ECS_ON_EVENT(on_appear)
ECS_TRACK(drawPhysMap)
static void debug_physmap_appear_es_event_handler(const ecs::Event &, UniqueTexHolder &phys_map_tex, const bool &drawPhysMap)
{
  phys_map_tex.close();

  if (!drawPhysMap)
    return;

  const PhysMap *physMap = dacoll::get_lmesh_phys_map();
  G_ASSERT_RETURN(physMap, );

  const int size = physMap->size;
  phys_map_tex = dag::create_tex(nullptr, size, size, TEXFMT_R8UI, 1, "phys_map_tex");

  ShaderGlobal::set_color4(::get_shader_variable_id("phys_map_coords", false), physMap->worldOffset.x, physMap->worldOffset.y,
    physMap->invScale, 0);

  if (auto data = lock_texture<uint8_t>(phys_map_tex.getTex2D(), 0, TEXLOCK_WRITE))
  {
    for (int j = 0; j < size; ++j)
      for (int i = 0; i < size; ++i)
        data.at(i, j) = physMap->materials[physMap->parent->get(i, j, size)];
  }
  else
  {
    logerr("render: Cannot lock phys_map_tex");
  }
}

ECS_TAG(render, dev)
ECS_NO_ORDER
static inline void debug_physmap_decals_es(const ecs::UpdateStageInfoRenderDebug &)
{
  if (!debug_physmap_decals.get())
    return;

  const PhysMap *physMap = dacoll::get_lmesh_phys_map();
  G_ASSERT_RETURN(physMap, );
  int gridSz = sqrt(physMap->gridDecals.size());
  if (gridSz > 0)
  {
    const TMatrix &camItm = get_cam_itm();
    float t = 1000.f;
    if (dacoll::traceray_normalized_lmesh(camItm.getcol(3), camItm.getcol(2), t, nullptr, nullptr))
    {
      const Point2 posProj = Point2::xz(camItm.getcol(3) + camItm.getcol(2) * t);
      IPoint2 resPos = IPoint2::xy((posProj - physMap->worldOffset) * physMap->invGridScale);
      if (resPos.x >= 0 && resPos.y >= 0 && resPos.x < gridSz && resPos.y < gridSz && resPos != physmap_selected_grid)
      {
        physmap_decal_vertices.clear();
        physmap_selected_grid = resPos;
        size_t cellId = resPos.y * gridSz + resPos.x;
        for (const PhysMap::DecalMesh &mesh : physMap->gridDecals[cellId])
        {
          for (const PhysMap::DecalMesh::MaterialIndices &indices : mesh.matIndices)
          {
            for (int ii = 0; ii < indices.indices.size(); ii += 3)
            {
              for (int idx = 0; idx < 3; ++idx)
              {
                Point2 xz = mesh.vertices[indices.indices[ii + idx]];
                float y = dacoll::traceht_lmesh(xz);
                physmap_decal_vertices.emplace_back(xz.x, y, xz.y);
              }
            }
          }
        }
      }
    }
  }
  if (physmap_decal_vertices.empty())
  {
    physmap_decal_vertices.clear();
    for (const PhysMap::DecalMesh &mesh : physMap->decals)
    {
      for (const PhysMap::DecalMesh::MaterialIndices &indices : mesh.matIndices)
      {
        for (int ii = 0; ii < indices.indices.size(); ii += 3)
        {
          for (int idx = 0; idx < 3; ++idx)
          {
            Point2 xz = mesh.vertices[indices.indices[ii + idx]];
            float y = dacoll::traceht_lmesh(xz);
            physmap_decal_vertices.emplace_back(xz.x, y, xz.y);
          }
        }
      }
    }
  }

  begin_draw_cached_debug_lines(false, false);
  for (int ii = 0; ii < physmap_decal_vertices.size(); ii += 3)
  {
    for (int idx = 0; idx < 3; ++idx)
    {
      draw_cached_debug_line(physmap_decal_vertices[ii + idx], physmap_decal_vertices[ii + (idx + 1) % 3], E3DCOLOR(32, 255, 255));
    }
  }
  end_draw_cached_debug_lines();
}
