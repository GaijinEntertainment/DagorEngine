//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotCollisionResource.h>
#include <daScript/ast/ast_typedecl.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/contactData.h>
#include <gamePhys/collision/collisionLinks.h>
#include <gamePhys/collision/collisionCache.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <math/dag_vecMathCompatibility.h>
#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>
#include <dasModules/aotRendInst.h>

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::dacoll::PhysLayer, PhysLayer);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::dacoll::CollType, CollType);
MAKE_TYPE_FACTORY(TraceMeshFaces, TraceMeshFaces);
MAKE_TYPE_FACTORY(CollisionObject, CollisionObject);
MAKE_TYPE_FACTORY(ShapeQueryOutput, dacoll::ShapeQueryOutput);
MAKE_TYPE_FACTORY(CollisionContactDataMin, gamephys::CollisionContactDataMin);
MAKE_TYPE_FACTORY(CollisionContactData, gamephys::CollisionContactData);
MAKE_TYPE_FACTORY(CollisionLinkData, dacoll::CollisionLinkData);

namespace bind_dascript
{

inline bool dacoll_traceray_normalized(Point3 p, Point3 dir, float &t, das::float3 &out_norm, int flags)
{
  return dacoll::traceray_normalized(p, dir, t, nullptr, reinterpret_cast<Point3 *>(&out_norm), flags);
}

inline bool dacoll_traceray_normalized_coll_type(const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm,
  int flags, rendinst::RendInstDesc &out_desc, int &out_coll_type, int ray_mat_id)
{
  return dacoll::traceray_normalized_coll_type(p, dir, t, &out_pmid, &out_norm, flags, &out_desc, &out_coll_type, ray_mat_id);
}

inline bool dacoll_traceray_normalized_trace_handle(const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm,
  int flags, rendinst::RendInstDesc &out_desc, int ray_mat_id, const TraceMeshFaces *handle)
{
  return dacoll::traceray_normalized(p, dir, t, &out_pmid, &out_norm, flags, &out_desc, ray_mat_id, handle);
}

inline bool dacoll_traceray_normalized_trace_handle2(const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm,
  int flags, rendinst::RendInstDesc &out_desc, int ray_mat_id, const TraceMeshFaces &handle)
{
  return dacoll_traceray_normalized_trace_handle(p, dir, t, out_pmid, out_norm, flags, out_desc, ray_mat_id, &handle);
}

inline bool dacoll_traceray_normalized_ex(const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm, int flags,
  rendinst::RendInstDesc &out_desc, int ray_mat_id)
{
  return dacoll::traceray_normalized(p, dir, t, &out_pmid, &out_norm, flags, &out_desc, ray_mat_id);
}

inline bool dacoll_tracedown_normalized(Point3 p, float &out_t, int flags)
{
  return dacoll::tracedown_normalized(p, out_t, nullptr, nullptr, flags);
}

inline bool dacoll_tracedown_normalized_with_mat_id(Point3 p, float &out_t, int flags, int ray_mat_id)
{
  return dacoll::tracedown_normalized(p, out_t, nullptr, nullptr, flags, nullptr, ray_mat_id);
}

inline bool dacoll_tracedown_normalized_with_mat_id_ex(Point3 p, float &out_t, Point3 &out_norm, int flags, int ray_mat_id)
{
  return dacoll::tracedown_normalized(p, out_t, nullptr, &out_norm, flags, nullptr, ray_mat_id);
}

inline bool dacoll_tracedown_normalized_with_norm(Point3 p, float &out_t, Point3 &out_norm, int flags)
{
  return dacoll::tracedown_normalized(p, out_t, nullptr, &out_norm, flags);
}

inline bool dacoll_tracedown_normalized_with_pmid(Point3 p, float &out_t, int &out_pmid, int flags)
{
  return dacoll::tracedown_normalized(p, out_t, &out_pmid, nullptr, flags);
}

inline bool dacoll_tracedown_normalized_with_norm_and_pmid(Point3 p, float &out_t, int &out_pmid, Point3 &out_norm, int flags)
{
  return dacoll::tracedown_normalized(p, out_t, &out_pmid, &out_norm, flags);
}

inline bool dacoll_tracedown_normalized_trace_handle_with_pmid(Point3 p, float &out_t, int &out_pmid, int flags,
  const TraceMeshFaces *handle)
{
  return dacoll::tracedown_normalized(p, out_t, &out_pmid, /*out_norm*/ nullptr, flags, /*out_desc*/ nullptr, /*ray_mat_id*/ -1,
    handle);
}

inline float dacoll_traceht_water_at_time(Point3 p, float t, float time, bool &underwater)
{
  return dacoll::traceht_water_at_time(p, t, time, underwater);
}

inline bool dacoll_rayhit_normalized(const Point3 &p, const Point3 &dir, real t, int flags, int ray_mat_id)
{
  return dacoll::rayhit_normalized(p, dir, t, flags, ray_mat_id);
}

inline bool dacoll_rayhit_normalized_trace_handle(const Point3 &p, const Point3 &dir, real t, int flags, int ray_mat_id,
  TraceMeshFaces *trace_handle)
{
  return dacoll::rayhit_normalized(p, dir, t, flags, ray_mat_id, trace_handle);
}

inline bool dacoll_traceray_normalized_frt(const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm)
{
  return dacoll::traceray_normalized_frt(p, dir, t, &out_pmid, &out_norm);
}

inline bool dacoll_traceray_normalized_lmesh(const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm)
{
  return dacoll::traceray_normalized_lmesh(p, dir, t, &out_pmid, &out_norm);
}

inline bool dacoll_traceray_normalized_ri(const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm, int flags,
  rendinst::RendInstDesc &out_desc, int ray_mat_id, rendinst::riex_handle_t skip_riex_handle)
{
  rendinst::TraceFlags additionalTraceFlags = {};
  if ((flags & dacoll::ETF_RI_TREES) != 0)
    additionalTraceFlags |= rendinst::TraceFlag::Trees;
  if ((flags & dacoll::ETF_RI_PHYS) != 0)
    additionalTraceFlags |= rendinst::TraceFlag::Phys;
  return dacoll::traceray_normalized_ri(p, dir, t, &out_pmid, &out_norm, additionalTraceFlags, &out_desc, ray_mat_id, nullptr,
    skip_riex_handle);
}

inline void dacoll_validate_trace_cache(const bbox3f &query_box, const das::float3 &expands, float physmap_expands,
  TraceMeshFaces &handle)
{
  return dacoll::validate_trace_cache(query_box, expands, physmap_expands, &handle);
}

inline void get_min_max_hmap_list_in_circle(das::float2 center, float rad,
  const das::TBlock<void, das::TTemporary<const das::TArray<das::float2>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<Point2> list(framemem_ptr());
  dacoll::get_min_max_hmap_list_in_circle(Point2::xy(center), rad, list);

  das::Array arr;
  arr.data = (char *)list.data();
  arr.size = uint32_t(list.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline bool _builtin_sphere_cast(const Point3 &from, const Point3 &to, float rad, dacoll::ShapeQueryOutput &out, int cast_mat_id)
{
  return dacoll::sphere_cast(from, to, rad, out, cast_mat_id, nullptr);
}

inline bool dacoll_sphere_cast_with_collision_object_ex(const Point3 &from, const Point3 &to, float rad, dacoll::ShapeQueryOutput &out,
  int cast_mat_id, const CollisionObject &ignore_obj, TraceMeshFaces *handle, int mask)
{
  return dacoll::sphere_cast_ex(from, to, rad, out, cast_mat_id, make_span_const(&ignore_obj, 1), handle, mask);
}

inline bool dacoll_sphere_cast_land(const Point3 &from, const Point3 &to, float rad, dacoll::ShapeQueryOutput &out)
{
  return dacoll::sphere_cast_land(from, to, rad, out);
}

inline bool dacoll_sphere_cast_ex(const Point3 &from, const Point3 &to, float rad, dacoll::ShapeQueryOutput &out, int cast_mat_id,
  TraceMeshFaces *handle, int mask)
{
  return dacoll::sphere_cast_ex(from, to, rad, out, cast_mat_id, dag::ConstSpan<CollisionObject>(), handle, mask);
}

inline bool dacoll_sphere_query_ri(const Point3 &from, const Point3 &to, float rad, dacoll::ShapeQueryOutput &out, int cast_mat_id,
  TraceMeshFaces *handle, const das::TBlock<void, const das::TTemporary<const das::TArray<rendinst::RendInstDesc>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  Tab<rendinst::RendInstDesc> out_desc(framemem_ptr());
  bool result = dacoll::sphere_query_ri(from, to, rad, out, cast_mat_id, &out_desc, handle);
  das::Array arr;
  arr.data = (char *)out_desc.data();
  arr.size = uint32_t(out_desc.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return result;
}


inline bool dacoll_sphere_query_ri_filtered(const Point3 &from, const Point3 &to, float rad, dacoll::ShapeQueryOutput &out,
  int cast_mat_id, TraceMeshFaces *handle, const das::TBlock<bool, const rendinst::RendInstDesc &, float> &block,
  das::Context *context, das::LineInfoArg *at)
{
  auto filter = [&](const rendinst::RendInstDesc &desc, float t) {
    vec4f args[] = {das::cast<rendinst::RendInstDesc *>::from(&desc), das::cast<float>::from(t)};
    vec4f res = context->invoke(block, args, nullptr, at);
    return das::cast<bool>::to(res);
  };
  dacoll::RIFilterCB filterCB(filter);
  return dacoll::sphere_query_ri(from, to, rad, out, cast_mat_id, nullptr, handle, &filterCB);
}

inline void dacoll_add_dynamic_collision_from_coll_resource(CollisionObject &co, const CollisionResource &coll_res,
  const char *blk_name)
{
  const DataBlock blk(blk_name, framemem_ptr());
  const DataBlock *collResBlk = blk.getBlockByNameEx("collisionResource", nullptr);
  const DataBlock *collPropsBlk = collResBlk ? collResBlk->getBlockByNameEx("props", nullptr) : nullptr;
  using namespace dacoll;
  destroy_dynamic_collision(co);
  co = add_dynamic_collision_from_coll_resource(collPropsBlk, &coll_res);
}

inline void dacoll_destroy_dynamic_collision(CollisionObject &coll_obj)
{
  dacoll::destroy_dynamic_collision(coll_obj);
  coll_obj.clear_ptrs();
}

inline bool dacoll_test_collision_world(const CollisionObject &coll_obj, const das::float3x4 &tm, float bounding_rad)
{
  Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
  return dacoll::test_collision_world(make_span_const(&coll_obj, 1), reinterpret_cast<const TMatrix &>(tm), bounding_rad, contacts,
    nullptr /*cache*/);
}

inline bool dacoll_test_collision_world_ex(const CollisionObject &coll_obj, const das::float3x4 &tm, float bounding_rad,
  const das::TBlock<void, const das::TTemporary<const das::TArray<gamephys::CollisionContactData>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
  bool result = dacoll::test_collision_world(make_span_const(&coll_obj, 1), reinterpret_cast<const TMatrix &>(tm), bounding_rad,
    contacts, nullptr /*cache*/);

  // TODO: make invoke generic for reuse
  das::Array arr;
  arr.data = (char *)contacts.data();
  arr.size = uint32_t(contacts.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);

  return result;
}


inline bool dacoll_test_collision_ri_ex(const CollisionObject &coll_obj, const BBox3 &box, int mat_id,
  const das::TBlock<void, const das::TTemporary<const das::TArray<gamephys::CollisionContactData>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
  bool result = dacoll::test_collision_ri(coll_obj, box, contacts, nullptr, mat_id);

  das::Array arr;
  arr.data = (char *)contacts.data();
  arr.size = uint32_t(contacts.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);

  return result;
}

inline bool dacoll_test_collision_ri(const CollisionObject &coll_obj, const BBox3 &box,
  const das::TBlock<void, const das::TTemporary<const das::TArray<gamephys::CollisionContactData>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  return dacoll_test_collision_ri_ex(coll_obj, box, -1, block, context, at);
}

inline bool dacoll_test_collision_frt(const CollisionObject &coll_obj, int mat_id,
  const das::TBlock<void, const das::TTemporary<const das::TArray<gamephys::CollisionContactData>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
  bool result = dacoll::test_collision_frt(coll_obj, contacts, mat_id);

  das::Array arr;
  arr.data = (char *)contacts.data();
  arr.size = uint32_t(contacts.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);

  return result;
}

inline bool dacoll_test_sphere_collision_world(const DPoint3 &pos, float bounding_radius, int mat_id, dacoll::PhysLayer group,
  int mask, const das::TBlock<void, const das::TTemporary<const das::TArray<gamephys::CollisionContactData>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
  bool result = dacoll::test_sphere_collision_world(pos, bounding_radius, mat_id, contacts, group, mask);

  das::Array arr;
  arr.data = (char *)contacts.data();
  arr.size = uint32_t(contacts.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);

  return result;
}

inline bool dacoll_test_box_collision_world(const TMatrix &tm, int mat_id, dacoll::PhysLayer group, int mask,
  const das::TBlock<void, const das::TTemporary<const das::TArray<gamephys::CollisionContactData>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
  bool result = dacoll::test_box_collision_world(tm, mat_id, contacts, group, mask);

  das::Array arr;
  arr.data = (char *)contacts.data();
  arr.size = uint32_t(contacts.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);

  return result;
}

inline void use_box_collision(const das::TBlock<void, CollisionObject> &block, das::Context *context, das::LineInfoArg *at)
{
  CollisionObject &coll = dacoll::get_reusable_box_collision();
  vec4f arg = das::cast<CollisionObject &>::from(coll);
  context->invoke(block, &arg, nullptr, at);
}

inline void use_sphere_collision(const das::TBlock<void, CollisionObject> &block, das::Context *context, das::LineInfoArg *at)
{
  CollisionObject &coll = dacoll::get_reusable_sphere_collision();
  vec4f arg = das::cast<CollisionObject &>::from(coll);
  context->invoke(block, &arg, nullptr, at);
}

inline void use_capsule_collision(const das::TBlock<void, CollisionObject> &block, das::Context *context, das::LineInfoArg *at)
{
  CollisionObject &coll = dacoll::get_reusable_capsule_collision();
  vec4f arg = das::cast<CollisionObject &>::from(coll);
  context->invoke(block, &arg, nullptr, at);
}

inline void dacoll_fetch_sim_res(bool wait) { dacoll::fetch_sim_res(wait); }

inline void dacoll_add_dynamic_sphere_collision(CollisionObject &obj, float rad, bool add_to_world)
{
  obj = dacoll::add_dynamic_sphere_collision(TMatrix::IDENT, rad, /*up*/ nullptr, add_to_world);
}

inline void dacoll_add_dynamic_cylinder_collision(CollisionObject &obj, float rad, float ht, bool add_to_world)
{
  obj = dacoll::add_dynamic_cylinder_collision(TMatrix::IDENT, rad, ht, /*up*/ nullptr, add_to_world);
}

inline void dacoll_add_dynamic_capsule_collision(CollisionObject &obj, float radius, float height, bool add_to_world)
{
  obj = dacoll::add_dynamic_capsule_collision(TMatrix::IDENT, radius, height, /*up*/ nullptr, add_to_world);
}

inline void dacoll_draw_collision_object(const CollisionObject &co) { dacoll::draw_collision_object(co); }

inline bool dacoll_shape_query_frt(const CollisionObject &co, const das::float3x4 &from, const das::float3x4 &to,
  dacoll::ShapeQueryOutput &out)
{
  auto convexShape = dacoll::get_convex_shape(co);
  if (!convexShape)
    return false;
  dacoll::shape_query_frt(convexShape, reinterpret_cast<const TMatrix &>(from), reinterpret_cast<const TMatrix &>(to), out);
  return out.t < 1.f;
}

inline bool dacoll_shape_query_lmesh(const CollisionObject &co, const das::float3x4 &from, const das::float3x4 &to,
  dacoll::ShapeQueryOutput &out)
{
  auto convexShape = dacoll::get_convex_shape(co);
  if (!convexShape)
    return false;
  dacoll::shape_query_lmesh(convexShape, reinterpret_cast<const TMatrix &>(from), reinterpret_cast<const TMatrix &>(to), out);
  return out.t < 1.f;
}

inline bool dacoll_shape_query_ri(const CollisionObject &co, const das::float3x4 &from, const das::float3x4 &to, float rad,
  dacoll::ShapeQueryOutput &out)
{
  auto convexShape = dacoll::get_convex_shape(co);
  if (!convexShape)
    return false;
  dacoll::shape_query_ri(convexShape, reinterpret_cast<const TMatrix &>(from), reinterpret_cast<const TMatrix &>(to), rad, out);
  return out.t < 1.f;
}

inline bool dacoll_shape_query_world(const CollisionObject &co, const das::float3x4 &from, const das::float3x4 &to, float rad,
  dacoll::ShapeQueryOutput &out)
{
  auto convexShape = dacoll::get_convex_shape(co);
  if (!convexShape)
    return false;

  dacoll::ShapeQueryOutput frtOut, lmeshOut, riOut;
  dacoll::shape_query_frt(convexShape, reinterpret_cast<const TMatrix &>(from), reinterpret_cast<const TMatrix &>(to), frtOut);
  dacoll::shape_query_lmesh(convexShape, reinterpret_cast<const TMatrix &>(from), reinterpret_cast<const TMatrix &>(to), lmeshOut);
  out = lmeshOut.t < frtOut.t ? lmeshOut : frtOut;

  dacoll::shape_query_ri(convexShape, reinterpret_cast<const TMatrix &>(from), reinterpret_cast<const TMatrix &>(to), rad, riOut);
  out = out.t < frtOut.t ? out : frtOut;

  return out.t < 1.f;
}

} // namespace bind_dascript
