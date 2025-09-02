// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/collisionCache.h>
#include <gamePhys/collision/collisionInstances.h>

namespace dacoll
{
bool traceray_normalized(const Point3 &, const Point3 &, real &, int *, Point3 *, int, rendinst::RendInstDesc *, int,
  const TraceMeshFaces *)
{
  G_ASSERT_RETURN(false, false);
}
bool traceray_normalized_coll_type(const Point3 &, const Point3 &, real &, int *, Point3 *, int, rendinst::RendInstDesc *, int *, int,
  const TraceMeshFaces *)
{
  G_ASSERT_RETURN(false, false);
}
bool tracedown_normalized(const Point3 &, real &, int *, Point3 *, int, rendinst::RendInstDesc *, int, const TraceMeshFaces *)
{
  G_ASSERT_RETURN(false, false);
}
bool traceray_normalized_frt(const Point3 &, const Point3 &, real &, int *, Point3 *) { G_ASSERT_RETURN(false, false); }
bool traceray_normalized_lmesh(const Point3 &, const Point3 &, real &, int *, Point3 *) { G_ASSERT_RETURN(false, false); }
bool traceray_normalized_ri(const Point3 &, const Point3 &, real &, int *, Point3 *, rendinst::TraceFlags, rendinst::RendInstDesc *,
  int, const TraceMeshFaces *, rendinst::riex_handle_t)
{
  G_ASSERT_RETURN(false, false);
}
void validate_trace_cache(const bbox3f &, const vec3f &, float, TraceMeshFaces *, float) { G_ASSERT(0); }
bool trace_game_objects(const Point3 &, const Point3 &, float &, Point3 &, int, int) { G_ASSERT_RETURN(false, false); }
bool traceht_water(const Point3 &, float &) { G_ASSERT_RETURN(false, false); }
float traceht_lmesh(const Point2 &) { G_ASSERT_RETURN(false, 0.f); }
float traceht_hmap(const Point2 &) { G_ASSERT_RETURN(false, 0.f); }
bool is_valid_heightmap_pos(const Point2 &) { G_ASSERT_RETURN(false, false); }
bool is_valid_water_height(float) { G_ASSERT_RETURN(false, false); }
float traceht_water_at_time(const Point3 &, float, float, bool &, float) { G_ASSERT_RETURN(false, false); }
bool traceray_water_at_time(const Point3 &, const Point3 &, float, float &) { G_ASSERT_RETURN(false, false); }
bool get_min_max_hmap_in_circle(const Point2 &, float, float &, float &) { G_ASSERT_RETURN(false, false); }
void get_min_max_hmap_list_in_circle(const Point2 &, float, Tab<Point2> &) { G_ASSERT(0); };
bool rayhit_normalized_lmesh(const Point3 &, const Point3 &, real) { G_ASSERT_RETURN(false, false); }
bool rayhit_normalized(const Point3 &, const Point3 &, real, int, int, const TraceMeshFaces *, rendinst::riex_handle_t)
{
  G_ASSERT_RETURN(false, false);
}
bool sphere_cast(const Point3 &, const Point3 &, float, ShapeQueryOutput &, int, const TraceMeshFaces *)
{
  G_ASSERT_RETURN(false, false);
}
bool sphere_cast_land(const Point3 &, const Point3 &, float, ShapeQueryOutput &, int) { G_ASSERT_RETURN(false, false); }
bool sphere_cast_ex(const Point3 &, const Point3 &, float, ShapeQueryOutput &, int, dag::ConstSpan<CollisionObject>,
  const TraceMeshFaces *, int, int)
{
  G_ASSERT_RETURN(false, false);
}
bool sphere_query_ri(const Point3 &, const Point3 &, float, ShapeQueryOutput &, int, Tab<rendinst::RendInstDesc> *,
  const TraceMeshFaces *, RIFilterCB *)
{
  G_ASSERT_RETURN(false, false);
}
bool trace_sphere_cast_ex(const Point3 &, const Point3 &, float, int, dacoll::ShapeQueryOutput &, int, int, const TraceMeshFaces *,
  int)
{
  G_ASSERT_RETURN(false, false);
}
CollisionObject add_dynamic_collision_from_coll_resource(const DataBlock *, const CollisionResource *, void *, int, int, int,
  const TMatrix *, const char *, const TMatrix *)
{
  G_ASSERT_RETURN(false, CollisionObject());
}
CollisionObject add_simple_dynamic_collision_from_coll_resource(const DataBlock &, const CollisionResource *, GeomNodeTree *, float,
  float, Point3 &, TMatrix &, TMatrix &, bool)
{
  G_ASSERT_RETURN(false, CollisionObject());
}
CollisionObject add_dynamic_collision_with_mask(const DataBlock &, void *, bool, bool, bool, int, const TMatrix *)
{
  G_ASSERT_RETURN(false, CollisionObject());
}
CollisionObject add_dynamic_sphere_collision(const TMatrix &, float, void *, bool) { G_ASSERT_RETURN(false, CollisionObject()); }
CollisionObject add_dynamic_capsule_collision(const TMatrix &, float, float, void *, bool)
{
  G_ASSERT_RETURN(false, CollisionObject());
}
CollisionObject add_dynamic_cylinder_collision(const TMatrix &, float, float, void *, bool)
{
  G_ASSERT_RETURN(false, CollisionObject());
}
void add_collision_hmap_custom(const Point3 &, const BBox3 &, const Point2 &, float, int) { G_ASSERT(0); }

void destroy_dynamic_collision(CollisionObject) { G_ASSERT(0); }
bool test_pair_collision(dag::ConstSpan<CollisionObject>, uint64_t, const TMatrix &, dag::ConstSpan<CollisionObject>, uint64_t,
  const TMatrix &, Tab<gamephys::CollisionContactData> &, TestPairFlags, bool)
{
  G_ASSERT_RETURN(false, false);
}
bool test_collision_world(dag::ConstSpan<CollisionObject>, const TMatrix &, float, Tab<gamephys::CollisionContactData> &,
  const TraceMeshFaces *)
{
  G_ASSERT_RETURN(false, false);
}
void update_ri_cache_in_volume_to_phys_world(const BBox3 &) {}
bool check_ri_collision_filtered(const rendinst::RendInstDesc &, const TMatrix &, const TMatrix &, int)
{
  G_ASSERT_RETURN(false, false);
}
bool test_collision_ri(const CollisionObject &, const BBox3 &, Tab<gamephys::CollisionContactData> &, const TraceMeshFaces *, int)
{
  G_ASSERT_RETURN(false, false);
}
bool test_collision_ri(const CollisionObject &, const BSphere3 &, Tab<gamephys::CollisionContactData> &, const TraceMeshFaces *, int)
{
  G_ASSERT_RETURN(false, false);
}
bool test_collision_frt(const CollisionObject &, Tab<gamephys::CollisionContactData> &, int) { G_ASSERT_RETURN(false, false); }
bool test_sphere_collision_world(const Point3 &, float, int, Tab<gamephys::CollisionContactData> &, dacoll::PhysLayer, int)
{
  G_ASSERT_RETURN(false, false);
}
bool test_box_collision_world(const TMatrix &, int, Tab<gamephys::CollisionContactData> &, dacoll::PhysLayer, int)
{
  G_ASSERT_RETURN(false, false);
}
bool test_capsule_collision_world(const TMatrix &, float, float, int, Tab<gamephys::CollisionContactData> &, dacoll::PhysLayer, int)
{
  G_ASSERT_RETURN(false, false);
}
PhysBody *get_convex_shape(struct CollisionObject) { G_ASSERT_RETURN(false, nullptr); }
void shape_query_frt(const PhysBody *, class TMatrix const &, class TMatrix const &, dacoll::ShapeQueryOutput &) { G_ASSERT(0); }
void shape_query_lmesh(const PhysBody *, class TMatrix const &, class TMatrix const &, dacoll::ShapeQueryOutput &) { G_ASSERT(0); }
void shape_query_ri(const PhysBody *, class TMatrix const &, class TMatrix const &, float, dacoll::ShapeQueryOutput &, int,
  Tab<struct rendinst::RendInstDesc> *, struct TraceMeshFaces const *, RIFilterCB *)
{
  G_ASSERT(0);
}
void fetch_sim_res(bool) { G_ASSERT(0); }
void set_vert_capsule_shape_size(const CollisionObject &, float, float) { G_ASSERT(0); }
void set_collision_sphere_rad(const CollisionObject &, float) { G_ASSERT(0); }
void set_collision_object_tm(const CollisionObject &, const TMatrix &) { G_ASSERT(0); }
void draw_collision_object(const CollisionObject &) { G_ASSERT(0); }

int set_hmap_step(int) { G_ASSERT_RETURN(false, -1); }
int get_hmap_step() { G_ASSERT_RETURN(false, -1); }
void set_obj_motion(CollisionObject, const TMatrix &, const Point3 &, const Point3 &) { G_ASSERT(0); }
PhysMat::MatID get_lmesh_mat_id_at_point(const Point2 &) { G_ASSERT_RETURN(false, PHYSMAT_INVALID); }
static CollisionObject tmpObj;
CollisionObject &get_reusable_sphere_collision() { G_ASSERT_RETURN(false, tmpObj); }
CollisionObject &get_reusable_capsule_collision() { G_ASSERT_RETURN(false, tmpObj); }
CollisionObject &get_reusable_box_collision() { G_ASSERT_RETURN(false, tmpObj); }

void move_ri_instance(const rendinst::RendInstDesc &, const Point3 &, const Point3 &) { G_ASSERT(0); }
void enable_disable_ri_instance(const rendinst::RendInstDesc &, bool) { G_ASSERT(0); }
void flush_ri_instances() { G_ASSERT(0); }
bool is_ri_instance_enabled(const CollisionInstances *, const rendinst::RendInstDesc &) { G_ASSERT_RETURN(false, true); }
int get_link_name_id(const char *) { G_ASSERT_RETURN(false, -1); }
const char *get_link_name(const int) { G_ASSERT_RETURN(false, NULL); }

} // namespace dacoll

#include <gamePhys/collision/collisionResponse.h>
namespace daphys
{
void resolve_penetration(DPoint3 &, Quat &, dag::ConstSpan<gamephys::CollisionContactData>, double, const DPoint3 &, double, bool, int,
  float, float)
{
  G_ASSERT(0);
};
void resolve_contacts(const DPoint3 &, const Quat &, DPoint3 &, DPoint3 &, dag::ConstSpan<gamephys::CollisionContactData>, double,
  const DPoint3 &, const ResolveContactParams &, int)
{
  G_ASSERT(0);
};
} // namespace daphys