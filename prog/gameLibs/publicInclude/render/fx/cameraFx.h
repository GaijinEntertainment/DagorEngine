//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "cameraFxAreaId.h"
#include <math/dag_Point3.h>
#include <math/dag_bounds2.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathUtils.h>
#include <EASTL/type_traits.h>
#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>

namespace fx::camera_fx
{

template <typename EffectAsset>
struct Area
{
  Point3 pos;
  float rad;
  // If height is <= 0, this Area is spherical, otherwise it's cylindrical.
  // The cylinder has its position at the bottom:
  //  ^                 _____________
  //  | y              |             |
  //  |                |             | height
  //  +-------> x/z    |____(pos)____|
  //                     rad
  float height;

  EffectAsset asset;
};

template <typename T>
inline bool is_asset_valid(T asset);

template <typename EffectAsset>
struct EffectCommon
{
  EffectAsset asset = {};
  float spawnRate = 1.f;
  float alpha = 1.f;
};

template <typename EffectAsset>
using GetEffectTypeByNameFn = EffectAsset (*)(const char *);

template <typename Effect>
using StartEffectFn = void (*)(Effect &, const TMatrix &, const TMatrix &);

template <typename Effect>
using StopEffectFn = void (*)(Effect &);

template <typename Effect>
using ModifyEffectFn = void (*)(Effect &);

template <typename EffectAsset, typename Effect, GetEffectTypeByNameFn<EffectAsset> get_effect_type_by_name,
  StartEffectFn<Effect> start_effect, StopEffectFn<Effect> stop_effect, ModifyEffectFn<Effect> modify_effect>
class Context
{
public:
  static constexpr int GRID_SIZE = 16;
  static constexpr float MINIMAL_RADIUS = 1.f;
  static constexpr float MINIMAL_HEIGHT = 1.f;

  static_assert(eastl::is_base_of_v<EffectCommon<EffectAsset>, Effect>);

  bool visible = true;
  bool isDirty = true;
  GenerationReferencedData<CameraFxId, Area<EffectAsset>> areaList;
  eastl::array<eastl::vector<CameraFxId>, GRID_SIZE * GRID_SIZE> grid;
  BBox2 gridBounds;
  eastl::vector<Effect> activeEffects;

  void clear()
  {
    for (Effect &e : activeEffects)
      stop_effect(e);

    areaList.clear();
    activeEffects.clear();
    isDirty = true;
  }

  // If height <= 0, change this area to a spherical area. Otherwise, change it to a cylindrical area.
  void changeArea(CameraFxId id, const char *res, const Point3 &pos, float radius, float height)
  {
    Area<EffectAsset> *a = areaList.get(id);
    G_ASSERT_RETURN(a, );

    bool failed = false;
    if (radius > 0 && radius < MINIMAL_RADIUS)
    {
      logwarn("camera_fx: area radius for: %s is less that minimal value %.2f/%.2f", res, radius, MINIMAL_RADIUS);
      failed = true;
    }

    if (height > 0 && height < MINIMAL_HEIGHT)
    {
      logwarn("camera_fx: area height for: %s is less that minimal value %.2f/%.2f", res, height, MINIMAL_HEIGHT);
      failed = true;
    }

    if (res)
    {
      a->asset = get_effect_type_by_name(res);
      if (!is_asset_valid(a->asset))
      {
        logwarn("camera_fx: invalid resource for area: %s", res);
        failed = true;
      }
    }

    a->pos = pos;
    a->rad = failed ? -1.0f : radius;
    a->height = failed ? -1.0f : height;

    isDirty = true;
  }

  // If height <= 0, this is a spherical area. Otherwise, it's a cylindrical area.
  CameraFxId addArea(const char *res, const Point3 &pos, float radius, float height)
  {
    CameraFxId id = areaList.emplaceOne();
    changeArea(id, res, pos, radius, height);
    return id;
  }

  void removeArea(CameraFxId id)
  {
    areaList.destroyReference(id);
    isDirty = true;
  }

  void recalculateGrid()
  {
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; ++i)
    {
      grid[i].clear();
    }

    gridBounds.setempty();
    for (int i = 0; i < areaList.totalSize(); ++i)
    {
      Area<EffectAsset> *a = areaList.getByIdx(i);
      if (!a || a->rad <= 0.0f)
        continue;

      gridBounds += BBox2(Point2(a->pos.x, a->pos.z), a->rad * 2.0f);
    }

    Point2 corner = gridBounds.getMin();
    Point2 size = gridBounds.size();

    for (int i = 0; i < areaList.totalSize(); ++i)
    {
      Area<EffectAsset> *a = areaList.getByIdx(i);
      if (!a || a->rad <= 0.0f)
        continue;

      int x0 = static_cast<int>((a->pos.x - a->rad - corner.x) / size.x * GRID_SIZE);
      int y0 = static_cast<int>((a->pos.z - a->rad - corner.y) / size.y * GRID_SIZE);
      int x1 = static_cast<int>((a->pos.x + a->rad - corner.x) / size.x * GRID_SIZE);
      int y1 = static_cast<int>((a->pos.z + a->rad - corner.y) / size.y * GRID_SIZE);

      x0 = clamp(x0, 0, GRID_SIZE - 1);
      y0 = clamp(y0, 0, GRID_SIZE - 1);
      x1 = clamp(x1, 0, GRID_SIZE - 1);
      y1 = clamp(y1, 0, GRID_SIZE - 1);

      for (int y = y0; y <= y1; ++y)
      {
        for (int x = x0; x <= x1; ++x)
        {
          grid[y * GRID_SIZE + x].push_back(areaList.getRefByIdx(i));
        }
      }
    }

    isDirty = false;
  }

  void setAreaModifiers(CameraFxId id, float spawn_rate, float opacity)
  {
    Area<EffectAsset> *a = areaList.get(id);
    G_ASSERT_RETURN(a, );

    auto it = eastl::find_if(activeEffects.begin(), activeEffects.end(),
      [target = a->asset](const Effect &effect) { return effect.asset == target; });

    if (it != activeEffects.end())
    {
      it->spawnRate = spawn_rate;
      it->alpha = opacity;
      modify_effect(*it);
    }
  }

  void beforeRender(const Point3 &view_pos)
  {
    if (isDirty)
      recalculateGrid();

    TMatrix fxTm = TMatrix::IDENT;
    TMatrix emTm = TMatrix::IDENT;
    emTm.setcol(3, view_pos);

    for (Effect &effect : activeEffects)
    {
      effect.spawnRate = 0.0f;
    }

    if (gridBounds & Point2(view_pos.x, view_pos.z))
    {
      int gridX = static_cast<int>((view_pos.x - gridBounds.getMin().x) / gridBounds.size().x * GRID_SIZE);
      int gridY = static_cast<int>((view_pos.z - gridBounds.getMin().y) / gridBounds.size().y * GRID_SIZE);
      gridX = clamp(gridX, 0, GRID_SIZE - 1);
      gridY = clamp(gridY, 0, GRID_SIZE - 1);


      for (CameraFxId areaId : grid[gridY * GRID_SIZE + gridX])
      {
        Area<EffectAsset> *area = areaList.get(areaId);

        float rad2 = area->rad * area->rad;
        bool inside;
        float atten;
        if (area->height <= 0)
        {
          // spherical area
          float d2 = lengthSq(view_pos - area->pos);
          inside = d2 < rad2;
          atten = d2 / rad2;
        }
        else
        {
          // cylindrical area
          float dy = view_pos.y - area->pos.y;
          float d2 = lengthSq(Point2(view_pos.x, view_pos.z) - Point2(area->pos.x, area->pos.z));
          inside = dy > 0 && dy < area->height && d2 < rad2;
          atten = (d2 / rad2) * (dy / area->height);
        }

        if (inside)
        {
          auto it = eastl::find_if(activeEffects.begin(), activeEffects.end(),
            [target = area->asset](const Effect &effect) { return effect.asset == target; });

          if (it == activeEffects.end())
          {
            it = activeEffects.insert(activeEffects.end(), Effect{area->asset});
          }

          it->spawnRate += saturate(1.0f - atten * atten);
        }
      }
    }

    int i = 0;
    while (i < activeEffects.size())
    {
      Effect &effect = activeEffects[i];
      if (effect.spawnRate == 0.0f)
      {
        stop_effect(effect);
        eastl::swap(effect, activeEffects.back());
        activeEffects.pop_back();
        continue;
      }

      start_effect(effect, emTm, fxTm);

      ++i;
    }
  }

}; // class Context

} // namespace fx::camera_fx