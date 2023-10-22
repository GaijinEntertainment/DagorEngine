#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_bounds3.h>
#include <riGen/riGenExtra.h>

struct RiGridObject
{
  rendinst::riex_handle_t handle;

  RiGridObject() {} //-V730
  RiGridObject(rendinst::riex_handle_t h) : handle(h) {}
  bool operator==(RiGridObject rhs) const { return handle == rhs.handle; }
  bool operator!=(RiGridObject rhs) const { return handle != rhs.handle; }
  bool operator<(RiGridObject rhs) const { return handle < rhs.handle; }
  vec4f getWBSph() const // wpos|r
  {
    uint32_t riType = rendinst::handle_to_ri_type(handle);
    uint32_t riInstance = rendinst::handle_to_ri_inst(handle);
    return rendinst::riExtra.data()[riType].riXYZR.data()[riInstance];
  }
  bbox3f getWBBox() const
  {
    uint32_t riType = rendinst::handle_to_ri_type(handle);
    uint32_t riInstance = rendinst::handle_to_ri_inst(handle);
    const rendinst::RiExtraPool &pool = rendinst::riExtra.data()[riType];
    mat44f tm;
    v_mat43_transpose_to_mat44(tm, pool.riTm.data()[riInstance]);
    bbox3f bbox;
    v_bbox3_init(bbox, tm, pool.collBb);
    return bbox;
  }
  uint64_t getHandle() const { return handle; }
  static RiGridObject null() { return rendinst::RIEX_HANDLE_NULL; }
};

template <typename ObjectType>
union LinearGridLeaf;
template <typename ObjectType>
struct LinearGridSubCell;
template <typename ObjectType>
struct LinearGridMainCell;
template <typename CellType>
class LinearGrid;
typedef LinearGrid<RiGridObject> RiGrid;
typedef eastl::fixed_function<256, bool(RiGridObject)> RiGridObjPred;

DAG_DECLARE_RELOCATABLE(RiGridObject);
DAG_DECLARE_RELOCATABLE(LinearGridLeaf<RiGridObject>);
DAG_DECLARE_RELOCATABLE(LinearGridSubCell<RiGridObject>);
DAG_DECLARE_RELOCATABLE(LinearGridMainCell<RiGridObject>);

VECTORCALL RiGridObject rigrid_find_in_box_by_pos(const RiGrid &grid_holder, bbox3f bbox, const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_box_by_bounding(const RiGrid &grid_holder, bbox3f bbox, const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_sphere_by_pos(const RiGrid &grid_holder, const Point3 &bsphere_c, float radius,
  const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_sphere_by_bounding(const RiGrid &grid_holder, const Point3 &bsphere_c, float radius,
  const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_capsule_by_pos(const RiGrid &grid_holder, const Point3 &from, const Point3 &dir, float len,
  float radius, const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_capsule_by_bounding(const RiGrid &grid_holder, const Point3 &from, const Point3 &dir, float len,
  float radius, const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_transformed_box_by_pos(const RiGrid &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_transformed_box_by_bounding(const RiGrid &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_ray_intersections(const RiGrid &grid_holder, const Point3 &from, const Point3 &dir, float len,
  const RiGridObjPred &pred);
void rigrid_debug_pos(const RiGrid &grid_holder, const Point3 &pos);
