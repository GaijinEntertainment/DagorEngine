#include <ecs/game/zones/zoneQuery.h>
#include <ecs/core/entityManager.h>
#include <math/dag_mathUtils.h>
#include <math/dag_math2d.h>
#include <math/dag_math3d.h>

template <typename Callable>
inline void active_sphere_capzones_ecs_query(Callable c);
template <typename Callable>
inline void active_box_capzones_ecs_query(Callable c);
template <typename Callable>
inline void active_relative_poly_capzones_ecs_query(Callable c);

namespace game
{
extern const BBox3 IDENTITY_BBOX3(Point3(-0.5f, -0.5f, -0.5f), Point3(0.5f, 0.5f, 0.5f));
}

void game::get_active_capzones_on_pos(const Point3 &pos, const char *tag_to_have_not_null, Tab<ecs::EntityId> &zones_in)
{
  const ecs::EntityManager *emgr = g_entity_mgr.operator->();
  ecs::HashedConstString tagToHave = ECS_HASH_SLOW(tag_to_have_not_null);
  active_sphere_capzones_ecs_query([&](ECS_REQUIRE(eastl::true_type active = true) ECS_REQUIRE(ecs::Tag capzone) ecs::EntityId eid,
                                     const TMatrix &transform, float sphere_zone__radius, bool sphere_zone__inverted = false) {
    if (emgr->has(eid, tagToHave))
      if (sphere_zone__inverted ^ (lengthSq(inverse(transform) * pos) < sqr(sphere_zone__radius)))
        zones_in.push_back(eid);
  });
  active_box_capzones_ecs_query(
    [&](ECS_REQUIRE(eastl::true_type active = true) ECS_REQUIRE(ecs::Tag capzone) ECS_REQUIRE(ecs::Tag box_zone) ecs::EntityId eid,
      const TMatrix &transform, bool box_zone__inverted = false) {
      if (emgr->has(eid, tagToHave))
        if (box_zone__inverted ^ (IDENTITY_BBOX3 & (inverse(transform) * pos)))
          zones_in.push_back(eid);
    });
  active_relative_poly_capzones_ecs_query(
    [&](ECS_REQUIRE(eastl::true_type active = true) ECS_REQUIRE(ecs::Tag capzone) ecs::EntityId eid,
      const ecs::Point2List &capzone__areaPoints, const float capzone__minHeight, const float capzone__maxHeight,
      bool poly_zone__inverted = false) {
      if (emgr->has(eid, tagToHave))
      {
        const bool isInPolyZone = (pos.y > capzone__minHeight && pos.y < capzone__maxHeight) &&
                                  is_point_in_poly(Point2::xz(pos), capzone__areaPoints.data(), capzone__areaPoints.size());
        if (poly_zone__inverted ^ isInPolyZone)
          zones_in.push_back(eid);
      }
    });
}

template <typename Callable>
inline bool sphere_zone_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
inline bool poly_battle_area_zone_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
inline bool poly_capzone_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
inline bool box_zone_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
inline bool relative_box_zone_ecs_query(ecs::EntityId eid, Callable c);


bool game::is_point_in_capzone(const Point3 &pos, ecs::EntityId zone_eid, float zone_scale)
{
  bool res = false;
  if (box_zone_ecs_query(zone_eid, [&](ECS_REQUIRE(ecs::Tag box_zone) const TMatrix &transform, bool box_zone__inverted = false) {
        float halfScale = zone_scale * 0.5f;
        const BBox3 box = BBox3(Point3(-halfScale, -halfScale, -halfScale), Point3(halfScale, halfScale, halfScale));
        res = box_zone__inverted ^ (box & (inverse(transform) * pos));
      }))
    return res;

  if (sphere_zone_ecs_query(zone_eid,
        [&](const TMatrix &transform, const float sphere_zone__radius, bool sphere_zone__inverted = false) {
          res = sphere_zone__inverted ^ (lengthSq(inverse(transform) * pos) < sqr(sphere_zone__radius * zone_scale));
        }))
    return res;

  if (poly_capzone_ecs_query(zone_eid, [&](const ecs::Point2List &capzone__areaPoints, const float capzone__minHeight,
                                         const float capzone__maxHeight, bool poly_zone__inverted = false) {
        const bool isInPolyZone = (pos.y > capzone__minHeight && pos.y < capzone__maxHeight) &&
                                  is_point_in_poly(Point2::xz(pos), capzone__areaPoints.data(), capzone__areaPoints.size());
        res = poly_zone__inverted ^ isInPolyZone;
      }))
    return res;

  return res;
}

bool game::is_point_in_poly_battle_area(const Point3 &pos, ecs::EntityId zone_eid)
{
  bool res = false;
  poly_battle_area_zone_ecs_query(zone_eid, [&](const ecs::Point2List &battleAreaPoints, bool poly_zone__inverted = false) {
    res = poly_zone__inverted ^ is_point_in_poly(Point2::xz(pos), battleAreaPoints.data(), battleAreaPoints.size());
  });
  return res;
}

// TODO: remove it after rewriting 2 using functions to das
bool game::is_inside_truncated_sphere_zone(const Point3 &pos, const TMatrix &zone_tm, float zone_radius, float *zone_truncate_below,
  float scale)
{
  if (lengthSq(pos - zone_tm.getcol(3)) > sqr(zone_radius * scale))
    return false;
  if (zone_truncate_below != nullptr)
  {
    float distBelow = dot(zone_tm.getcol(1), pos - zone_tm.getcol(3));
    if (distBelow < *zone_truncate_below * scale)
      return false;
  }
  return true;
}

template <typename Callable>
inline void get_transform_ecs_query(ecs::EntityId eid, Callable c);

bool game::is_entity_in_capzone(const ecs::EntityId eid, const ecs::EntityId zone_eid)
{
  bool res = false;
  get_transform_ecs_query(eid, [&](const TMatrix &transform) { res = game::is_point_in_capzone(transform.getcol(3), zone_eid); });
  return res;
}
