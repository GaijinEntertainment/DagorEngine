// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotDagorMath.h>
#include <dasModules/dasModulesCommon.h>


namespace bind_dascript
{
static char dagorMath_das[] =
#include "dagorMath.das.inl"
  ;

// #include <3d/dag_render.h>
struct Color3Annotation final : das::ManagedStructureAnnotation<Color3, false>
{
  Color3Annotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Color3", ml)
  {
    cppName = " ::Color3";
    addField<DAS_BIND_MANAGED_FIELD(r)>("r");
    addField<DAS_BIND_MANAGED_FIELD(g)>("g");
    addField<DAS_BIND_MANAGED_FIELD(b)>("b");
  }
};

struct Color4Annotation final : das::ManagedStructureAnnotation<Color4, false>
{
  Color4Annotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Color4", ml)
  {
    cppName = " ::Color4";
    addField<DAS_BIND_MANAGED_FIELD(r)>("r");
    addField<DAS_BIND_MANAGED_FIELD(g)>("g");
    addField<DAS_BIND_MANAGED_FIELD(b)>("b");
    addField<DAS_BIND_MANAGED_FIELD(a)>("a");
  }
};

struct E3DCOLORAnnotation final : das::ManagedValueAnnotation<E3DCOLOR>
{
  E3DCOLORAnnotation(das::ModuleLibrary &ml) : ManagedValueAnnotation(ml, "E3DCOLOR") { cppName = " ::E3DCOLOR"; }
  virtual void walk(das::DataWalker &walker, void *data) override { walker.UInt(((E3DCOLOR *)data)->u); }
};

struct Mat44fAnnotation final : das::ManagedStructureAnnotation<mat44f, false>
{
  Mat44fAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("mat44f", ml)
  {
    cppName = " ::mat44f";
    addFieldEx("col0", "col0", offsetof(mat44f, col0), das::makeType<das::float4>(ml));
    addFieldEx("col1", "col1", offsetof(mat44f, col1), das::makeType<das::float4>(ml));
    addFieldEx("col2", "col2", offsetof(mat44f, col2), das::makeType<das::float4>(ml));
    addFieldEx("col3", "col3", offsetof(mat44f, col3), das::makeType<das::float4>(ml));
  }
};

struct TMatrixAnnotation final : das::ManagedStructureAnnotation<TMatrix, false>
{
  TMatrixAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("TMatrix", ml) { cppName = " ::TMatrix"; }
};

struct FrustumAnnotation final : das::ManagedStructureAnnotation<Frustum, false>
{
  FrustumAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Frustum", ml, "Frustum") {}
};

struct BBox3Annotation final : das::ManagedStructureAnnotation<BBox3, false>
{
  BBox3Annotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("BBox3", ml)
  {
    cppName = " ::BBox3";
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(center)>("center");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(width)>("width");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(isempty)>("isempty");
    addPropertyExtConst<Point3 &(BBox3::*)(), &BBox3::boxMin, const Point3 &(BBox3::*)() const, &BBox3::boxMin>("boxMin", "boxMin");
    addPropertyExtConst<Point3 &(BBox3::*)(), &BBox3::boxMax, const Point3 &(BBox3::*)() const, &BBox3::boxMax>("boxMax", "boxMax");
  }
  bool isLocal() const override { return true; } // force isLocal, because ctor is non trivial
};

struct BBox3fAnnotation final : das::ManagedStructureAnnotation<bbox3f, false>
{
  BBox3fAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("bbox3f", ml)
  {
    cppName = " ::bbox3f";
    addFieldEx("bmin", "bmin", offsetof(bbox3f, bmin), das::makeType<das::float4>(ml));
    addFieldEx("bmax", "bmax", offsetof(bbox3f, bmax), das::makeType<das::float4>(ml));
  }
  bool isLocal() const override { return true; } // force isLocal, because ctor is non trivial
  bool canCopy() const override { return true; } // force canCopy, because exists manually added trivial copy operator
};

struct BBox2Annotation final : das::ManagedStructureAnnotation<BBox2, false>
{
  BBox2Annotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("BBox2", ml)
  {
    cppName = " ::BBox2";
    addProperty<DAS_BIND_MANAGED_PROP(center)>("center");
    addProperty<DAS_BIND_MANAGED_PROP(width)>("width");
    addProperty<DAS_BIND_MANAGED_PROP(isempty)>("isempty");
  }
  bool isLocal() const override { return true; } // force isLocal, because ctor is non trivial
};

struct BSphere3Annotation final : das::ManagedStructureAnnotation<BSphere3, false>
{
  BSphere3Annotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("BSphere3", ml)
  {
    cppName = " ::BSphere3";
    addField<DAS_BIND_MANAGED_FIELD(c)>("c");
    addField<DAS_BIND_MANAGED_FIELD(r)>("r");
    addField<DAS_BIND_MANAGED_FIELD(r2)>("r2");
    addProperty<DAS_BIND_MANAGED_PROP(isempty)>("isempty");
  }
  virtual bool isLocal() const override { return true; } // force isLocal, because ctor is non trivial
};

struct DPoint3Annotation final : das::ManagedStructureAnnotation<DPoint3, false>
{
  DPoint3Annotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DPoint3", ml)
  {
    cppName = " ::DPoint3";
    addField<DAS_BIND_MANAGED_FIELD(x)>("x");
    addField<DAS_BIND_MANAGED_FIELD(y)>("y");
    addField<DAS_BIND_MANAGED_FIELD(z)>("z");
  }
};

struct CapsuleAnnotation final : das::ManagedStructureAnnotation<Capsule, false>
{
  CapsuleAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Capsule", ml)
  {
    cppName = " ::Capsule";
    addField<DAS_BIND_MANAGED_FIELD(a)>("a");
    addField<DAS_BIND_MANAGED_FIELD(r)>("r");
    addField<DAS_BIND_MANAGED_FIELD(b)>("b");
  }
};

struct QuatAnnotation final : das::ManagedValueAnnotation<Quat>
{
  QuatAnnotation(das::ModuleLibrary &ml) : ManagedValueAnnotation(ml, "quat", " ::Quat") {}

  virtual void walk(das::DataWalker &walker, void *data) override
  {
    if (!walker.reading)
      walker.Float4(*(das::float4 *)data);
  }
};

struct Plane3Annotation final : das::ManagedStructureAnnotation<Plane3, false>
{
  Plane3Annotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Plane3", ml)
  {
    cppName = " ::Plane3";

    addField<DAS_BIND_MANAGED_FIELD(n)>("n");
    addField<DAS_BIND_MANAGED_FIELD(d)>("d");

    addProperty<DAS_BIND_MANAGED_PROP(getNormal)>("normal", "getNormal");
  }
  virtual bool isLocal() const override { return true; } // force isLocal, because ctor is non trivial
};

struct InterpolateTabFloatAnnotation final : das::ManagedStructureAnnotation<InterpolateTabFloat, false>
{
  InterpolateTabFloatAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("InterpolateTabFloat", ml)
  {
    cppName = " ::InterpolateTabFloat";
  }
};

class DagorMath final : public das::Module
{
public:
  DagorMath() : das::Module("DagorMath")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addAnnotation(das::make_smart<Color3Annotation>(lib));
    addAnnotation(das::make_smart<Color4Annotation>(lib));
    addAnnotation(das::make_smart<E3DCOLORAnnotation>(lib));
    addAnnotation(das::make_smart<Mat44fAnnotation>(lib));
    addAnnotation(das::make_smart<TMatrixAnnotation>(lib));
    addAnnotation(das::make_smart<FrustumAnnotation>(lib));
    addAnnotation(das::make_smart<BSphere3Annotation>(lib));
    addAnnotation(das::make_smart<DPoint3Annotation>(lib));
    addAnnotation(das::make_smart<BBox3Annotation>(lib));
    addAnnotation(das::make_smart<BBox3fAnnotation>(lib));
    addAnnotation(das::make_smart<BBox2Annotation>(lib));
    addAnnotation(das::make_smart<CapsuleAnnotation>(lib));
    addAnnotation(das::make_smart<QuatAnnotation>(lib));
    addAnnotation(das::make_smart<Plane3Annotation>(lib));
    addAnnotation(das::make_smart<InterpolateTabFloatAnnotation>(lib));
    das::addExtern<DAS_BIND_FUN(make_e3dcolor_from_color4)>(*this, lib, "E3DCOLOR", das::SideEffects::none,
      "bind_dascript::make_e3dcolor_from_color4");
    das::addExtern<DAS_BIND_FUN(make_color)>(*this, lib, "E3DCOLOR", das::SideEffects::none, "bind_dascript::make_color");
    das::addExtern<DAS_BIND_FUN(make_uint4)>(*this, lib, "uint4", das::SideEffects::none, "bind_dascript::make_uint4");
    das::addExtern<DAS_BIND_FUN(cast_to_uint)>(*this, lib, "uint", das::SideEffects::none, "bind_dascript::cast_to_uint");
    das::addExtern<DAS_BIND_FUN(cast_from_uint)>(*this, lib, "E3DCOLOR", das::SideEffects::none, "bind_dascript::cast_from_uint");
    das::addExtern<DAS_BIND_FUN(e3dcolor_lerp)>(*this, lib, "e3dcolor_lerp", das::SideEffects::none, "e3dcolor_lerp");
    das::addExtern<DAS_BIND_FUN(make_color3), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "Color3", das::SideEffects::none,
      "bind_dascript::make_color3");
    das::addExtern<DAS_BIND_FUN(from_color3)>(*this, lib, "float3", das::SideEffects::none, "bind_dascript::from_color3");
    das::addExtern<DAS_BIND_FUN(from_dpoint3)>(*this, lib, "float3", das::SideEffects::none, "bind_dascript::from_dpoint3");
    das::addExtern<DAS_BIND_FUN(dpoint3_from_point3), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "DPoint3",
      das::SideEffects::none, "bind_dascript::dpoint3_from_point3");
    das::addExtern<DAS_BIND_FUN(make_dpoint3), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "DPoint3", das::SideEffects::none,
      "bind_dascript::make_dpoint3");
    das::addExtern<DAS_BIND_FUN(make_color4), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "Color4", das::SideEffects::none,
      "bind_dascript::make_color4");
    das::addExtern<DAS_BIND_FUN(make_color4_2), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "Color4", das::SideEffects::none,
      "bind_dascript::make_color4_2");
    das::addExtern<DAS_BIND_FUN(make_color4_3), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "Color4", das::SideEffects::none,
      "bind_dascript::make_color4_3");
    das::addExtern<DAS_BIND_FUN(from_color4)>(*this, lib, "float4", das::SideEffects::none, "bind_dascript::from_color4");
    das::addExtern<DAS_BIND_FUN(bbox3_add_point)>(*this, lib, "bbox3_add", das::SideEffects::modifyArgument,
      "bind_dascript::bbox3_add_point");
    das::addExtern<DAS_BIND_FUN(bbox3_add_bbox3)>(*this, lib, "bbox3_add", das::SideEffects::modifyArgument,
      "bind_dascript::bbox3_add_bbox3");
    das::addExtern<DAS_BIND_FUN(bbox3_inflate)>(*this, lib, "bbox3_inflate", das::SideEffects::modifyArgument,
      "bind_dascript::bbox3_inflate");
    das::addExtern<DAS_BIND_FUN(bbox3_inflateXZ)>(*this, lib, "bbox3_inflateXZ", das::SideEffects::modifyArgument,
      "bind_dascript::bbox3_inflateXZ");
    das::addExtern<DAS_BIND_FUN(bbox3_scale)>(*this, lib, "bbox3_scale", das::SideEffects::modifyArgument,
      "bind_dascript::bbox3_scale");
    das::addExtern<DAS_BIND_FUN(bbox3_eq)>(*this, lib, "==", das::SideEffects::none, "bind_dascript::bbox3_eq");
    das::addExtern<DAS_BIND_FUN(bbox3_neq)>(*this, lib, "!=", das::SideEffects::none, "bind_dascript::bbox3_neq");
    das::addExtern<DAS_BIND_FUN(bbox3_intersect_point)>(*this, lib, "&", das::SideEffects::none,
      "bind_dascript::bbox3_intersect_point");
    das::addExtern<DAS_BIND_FUN(bbox3_intersect_bbox3)>(*this, lib, "&", das::SideEffects::none,
      "bind_dascript::bbox3_intersect_bbox3");
    das::addExtern<DAS_BIND_FUN(bbox3_intersect_bsphere)>(*this, lib, "&", das::SideEffects::none,
      "bind_dascript::bbox3_intersect_bsphere");
    das::addExtern<DAS_BIND_FUN(bsphere_intersect_bsphere)>(*this, lib, "&", das::SideEffects::none,
      "bind_dascript::bsphere_intersect_bsphere");
    das::addExtern<DAS_BIND_FUN(bbox3_transform), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "*", das::SideEffects::none,
      "bind_dascript::bbox3_transform");
    das::addExtern<DAS_BIND_FUN(bsphere_transform), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "*", das::SideEffects::none,
      "bind_dascript::bsphere_transform");
    das::addExtern<DAS_BIND_FUN(make_bsphere_from_bbox), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "BSphere3",
      das::SideEffects::none, "bind_dascript::make_bsphere_from_bbox");
    das::addExtern<DAS_BIND_FUN(make_bsphere), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "BSphere3", das::SideEffects::none,
      "bind_dascript::make_bsphere");
    das::addExtern<DAS_BIND_FUN(make_empty_bbox3), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "BBox3", das::SideEffects::none,
      "bind_dascript::make_empty_bbox3");
    das::addExtern<DAS_BIND_FUN(make_bbox3), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "BBox3", das::SideEffects::none,
      "bind_dascript::make_bbox3");
    das::addExtern<DAS_BIND_FUN(make_empty_bbox2), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "BBox2", das::SideEffects::none,
      "bind_dascript::make_empty_bbox2");
    das::addExtern<DAS_BIND_FUN(make_bbox2), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "BBox2", das::SideEffects::none,
      "bind_dascript::make_bbox2");
    das::addExtern<DAS_BIND_FUN(make_bbox_from_bsphere), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "BBox3",
      das::SideEffects::none, "bind_dascript::make_bbox_from_bsphere");
    das::addExtern<DAS_BIND_FUN(v_stu_bbox3)>(*this, lib, "bbox3f_to_scalar_bbox3", das::SideEffects::modifyArgument, "v_stu_bbox3");
    das::addExtern<DAS_BIND_FUN(make_bbox3_center), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "BBox3", das::SideEffects::none,
      "bind_dascript::make_bbox3_center");
    das::addExtern<DAS_BIND_FUN(check_bbox3_intersection)>(*this, lib, "check_bbox3_intersection", das::SideEffects::none,
      "bind_dascript::check_bbox3_intersection");
    das::addExtern<DAS_BIND_FUN(check_bbox3_intersection_scaled)>(*this, lib, "check_bbox3_intersection", das::SideEffects::none,
      "bind_dascript::check_bbox3_intersection_scaled");
    using bbox3_isempty = DAS_CALL_MEMBER(BBox3::isempty);
    das::addExtern<DAS_CALL_METHOD(bbox3_isempty)>(*this, lib, "bbox3_isempty", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(BBox3::isempty));

    das::addExtern<float (*)(const Point3 &, const Point3 &, const Point3 &, float), &rayIntersectSphereDist>(*this, lib,
      "rayIntersectSphereDist", das::SideEffects::none, "::rayIntersectSphereDist");

    das::addUsing<bbox3f>(*this, lib, "bbox3f");

    das::addExtern<DAS_BIND_FUN(cvt)>(*this, lib, "cvt", das::SideEffects::none, "cvt");
    das::addExtern<DAS_BIND_FUN(cvt_double)>(*this, lib, "cvt", das::SideEffects::none, "cvt_double");

    das::addExtern<DAS_BIND_FUN(length_dpoint3)>(*this, lib, "length", das::SideEffects::none, "bind_dascript::length_dpoint3");
    das::addExtern<DAS_BIND_FUN(length_sq_dpoint3)>(*this, lib, "length_sq", das::SideEffects::none,
      "bind_dascript::length_sq_dpoint3");
    das::addExtern<DAS_BIND_FUN(distance_dpoint3)>(*this, lib, "distance", das::SideEffects::none, "bind_dascript::distance_dpoint3");
    das::addExtern<DAS_BIND_FUN(distance_sq_dpoint3)>(*this, lib, "distance_sq", das::SideEffects::none,
      "bind_dascript::distance_sq_dpoint3");
    das::addExtern<DAS_BIND_FUN(distance_plane_point3)>(*this, lib, "distance_plane_point3", das::SideEffects::none,
      "bind_dascript::distance_plane_point3");
    das::addExtern<DAS_BIND_FUN(project_onto_plane)>(*this, lib, "project_onto_plane", das::SideEffects::none, "project_onto_plane");

    das::addExtern<DAS_BIND_FUN(quat_mul)>(*this, lib, "*", das::SideEffects::none, "bind_dascript::quat_mul");
    das::addExtern<DAS_BIND_FUN(quat_mul_vec)>(*this, lib, "*", das::SideEffects::none, "bind_dascript::quat_mul_vec");
    das::addExtern<DAS_BIND_FUN(quat_mul_scalar)>(*this, lib, "*", das::SideEffects::none, "bind_dascript::quat_mul_scalar");
    das::addExtern<DAS_BIND_FUN(quat_mul_scalar_first)>(*this, lib, "*", das::SideEffects::none,
      "bind_dascript::quat_mul_scalar_first");
    das::addExtern<DAS_BIND_FUN(quat_add)>(*this, lib, "+", das::SideEffects::none, "bind_dascript::quat_add");
    das::addExtern<DAS_BIND_FUN(quat_sub)>(*this, lib, "-", das::SideEffects::none, "bind_dascript::quat_sub");
    das::addExtern<DAS_BIND_FUN(quat_rotation_arc)>(*this, lib, "quat_rotation_arc", das::SideEffects::none, "::quat_rotation_arc");
    das::addExtern<DAS_BIND_FUN(quat_inverse)>(*this, lib, "inverse", das::SideEffects::none, "bind_dascript::quat_inverse");
    das::addExtern<DAS_BIND_FUN(quat_normalize)>(*this, lib, "normalize", das::SideEffects::none, "bind_dascript::quat_normalize");
    das::addExtern<DAS_BIND_FUN(quat_get_forward)>(*this, lib, "quat_get_forward", das::SideEffects::none,
      "bind_dascript::quat_get_forward");
    das::addExtern<DAS_BIND_FUN(quat_get_up)>(*this, lib, "quat_get_up", das::SideEffects::none, "bind_dascript::quat_get_up");
    das::addExtern<DAS_BIND_FUN(quat_get_left)>(*this, lib, "quat_get_left", das::SideEffects::none, "bind_dascript::quat_get_left");
    das::addExtern<DAS_BIND_FUN(approach<Quat>)>(*this, lib, "approach", das::SideEffects::none, "approach<Quat>");
    das::addExtern<DAS_BIND_FUN(approach_vel<float>)>(*this, lib, "approach_vel", das::SideEffects::modifyArgument,
      "approach_vel<float>");
    das::addExtern<DAS_BIND_FUN(approach_vel<Point2>)>(*this, lib, "approach_vel", das::SideEffects::modifyArgument,
      "approach_vel<Point2>");
    das::addExtern<DAS_BIND_FUN(approach_vel<Point3>)>(*this, lib, "approach_vel", das::SideEffects::modifyArgument,
      "approach_vel<Point3>");
    das::addExtern<DAS_BIND_FUN(dpoint3_approach_vel), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "approach_vel",
      das::SideEffects::modifyArgument, "bind_dascript::dpoint3_approach_vel");
    das::addExtern<DAS_BIND_FUN(dpoint3_minus), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "-", das::SideEffects::none,
      "bind_dascript::dpoint3_minus");
    das::addExtern<DAS_BIND_FUN(dpoint3_add), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "+", das::SideEffects::none,
      "bind_dascript::dpoint3_add");
    das::addExtern<DAS_BIND_FUN(dpoint3_sub), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "-", das::SideEffects::none,
      "bind_dascript::dpoint3_sub");
    das::addExtern<DAS_BIND_FUN(dpoint3_mul_scalar), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "*", das::SideEffects::none,
      "bind_dascript::dpoint3_mul_scalar");
    das::addExtern<DAS_BIND_FUN(dpoint3_mul_scalar_first), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "*",
      das::SideEffects::none, "bind_dascript::dpoint3_mul_scalar_first");
    das::addExtern<DAS_BIND_FUN(dpoint3_mul_scalar_float), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "*",
      das::SideEffects::none, "bind_dascript::dpoint3_mul_scalar_float");
    das::addExtern<DAS_BIND_FUN(dpoint3_mul_scalar_first_float), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "*",
      das::SideEffects::none, "bind_dascript::dpoint3_mul_scalar_first_float");
    das::addExtern<DAS_BIND_FUN(dpoint3_lerp), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "lerp", das::SideEffects::none,
      "bind_dascript::dpoint3_lerp");
    das::addExtern<DAS_BIND_FUN(dpoint3_add_dpoint3)>(*this, lib, "+=", das::SideEffects::modifyArgument,
      "bind_dascript::dpoint3_add_dpoint3");
    das::addExtern<DAS_BIND_FUN(quat_slerp)>(*this, lib, "slerp", das::SideEffects::none, "bind_dascript::quat_slerp");
    das::addExtern<DAS_BIND_FUN(quat_make_axis_ang)>(*this, lib, "quat", das::SideEffects::none, "bind_dascript::quat_make_axis_ang");
    das::addExtern<DAS_BIND_FUN(quat_make_float4)>(*this, lib, "quat", das::SideEffects::none, "bind_dascript::quat_make_float4");
    das::addExtern<DAS_BIND_FUN(quat_make)>(*this, lib, "quat", das::SideEffects::none, "bind_dascript::quat_make");
    das::addExtern<DAS_BIND_FUN(quat_make_matrix)>(*this, lib, "quat", das::SideEffects::none, "bind_dascript::quat_make_matrix");
    das::addExtern<DAS_BIND_FUN(euler_heading_attitude_to_quat)>(*this, lib, "euler_heading_attitude_to_quat",
      das::SideEffects::modifyArgument, "::euler_heading_attitude_to_quat");
    das::addExtern<DAS_BIND_FUN(quat_to_float4)>(*this, lib, "float4", das::SideEffects::none, "bind_dascript::quat_to_float4");
    das::addExtern<DAS_BIND_FUN(quat_make_tm)>(*this, lib, "make_tm", das::SideEffects::modifyArgument, "bind_dascript::quat_make_tm");
    das::addExtern<DAS_BIND_FUN(quat_make_tm_pos)>(*this, lib, "make_tm", das::SideEffects::modifyArgument,
      "bind_dascript::quat_make_tm_pos");
    das::addExtern<DAS_BIND_FUN(make_tm)>(*this, lib, "make_tm", das::SideEffects::modifyArgument, "bind_dascript::make_tm");
    das::addExtern<DAS_BIND_FUN(orthonormalize)>(*this, lib, "orthonormalize", das::SideEffects::modifyArgument,
      "bind_dascript::orthonormalize");
    das::addExtern<DAS_BIND_FUN(TMatrix_det)>(*this, lib, "det", das::SideEffects::none, "bind_dascript::TMatrix_det");
    das::addExtern<DAS_BIND_FUN(euler_to_quat)>(*this, lib, "euler_to_quat", das::SideEffects::modifyArgument, "::euler_to_quat");
    das::addExtern<DAS_BIND_FUN(quat_to_euler)>(*this, lib, "quat_to_euler", das::SideEffects::modifyArgument, "::quat_to_euler");
    das::addExtern<DAS_BIND_FUN(euler_heading_to_quat)>(*this, lib, "euler_heading_to_quat", das::SideEffects::modifyArgument,
      "::euler_heading_to_quat");
    das::addExtern<DAS_BIND_FUN(matrix_to_euler)>(*this, lib, "matrix_to_euler", das::SideEffects::modifyArgument,
      "::matrix_to_euler");
    das::addExtern<DAS_BIND_FUN(dir_to_quat)>(*this, lib, "dir_to_quat", das::SideEffects::none, "::dir_to_quat");
    das::addExtern<DAS_BIND_FUN(dir_and_up_to_quat)>(*this, lib, "dir_and_up_to_quat", das::SideEffects::none, "::dir_and_up_to_quat");
    das::addExtern<DAS_BIND_FUN(dir_to_sph_ang)>(*this, lib, "dir_to_sph_ang", das::SideEffects::none, "::dir_to_sph_ang");
    das::addExtern<DAS_BIND_FUN(sph_ang_to_dir)>(*this, lib, "sph_ang_to_dir", das::SideEffects::none, "::sph_ang_to_dir");
    das::addExtern<DAS_BIND_FUN(norm_s_ang)>(*this, lib, "norm_s_ang", das::SideEffects::none, "::norm_s_ang");
    das::addExtern<DAS_BIND_FUN(norm_s_ang_deg)>(*this, lib, "norm_s_ang_deg", das::SideEffects::none, "::norm_s_ang_deg");
    das::addExtern<DAS_BIND_FUN(norm_ang_deg_positive)>(*this, lib, "norm_ang_deg", das::SideEffects::none, "::norm_ang_deg_positive");
    das::addExtern<DAS_BIND_FUN(norm_ang)>(*this, lib, "norm_ang", das::SideEffects::none, "::norm_ang");
    das::addExtern<DAS_BIND_FUN(mat_rotxTM)>(*this, lib, "rotxTM", das::SideEffects::modifyArgument, "bind_dascript::mat_rotxTM");
    das::addExtern<DAS_BIND_FUN(mat_rotyTM)>(*this, lib, "rotyTM", das::SideEffects::modifyArgument, "bind_dascript::mat_rotyTM");
    das::addExtern<DAS_BIND_FUN(mat_rotzTM)>(*this, lib, "rotzTM", das::SideEffects::modifyArgument, "bind_dascript::mat_rotzTM");

    das::addExtern<DAS_BIND_FUN(perlin_noise::noise1)>(*this, lib, "perlin_noise1", das::SideEffects::none, "perlin_noise::noise1");
    das::addExtern<DAS_BIND_FUN(bind_dascript::noise2)>(*this, lib, "perlin_noise2", das::SideEffects::none, "bind_dascript::noise2");

    das::addExtern<DAS_BIND_FUN(bind_dascript::bbox3f_init)>(*this, lib, "bbox3f_init", das::SideEffects::modifyArgument,
      "bind_dascript::bbox3f_init");
    das::addExtern<DAS_BIND_FUN(bind_dascript::bbox3f_init2)>(*this, lib, "bbox3f_init", das::SideEffects::modifyArgument,
      "bind_dascript::bbox3f_init2");
    das::addExtern<DAS_BIND_FUN(bind_dascript::bbox3f_add_pt)>(*this, lib, "bbox3f_add_pt", das::SideEffects::modifyArgument,
      "bind_dascript::bbox3f_add_pt");
    das::addExtern<DAS_BIND_FUN(bind_dascript::bbox3f_test_pt_inside)>(*this, lib, "bbox3f_test_pt_inside", das::SideEffects::none,
      "bind_dascript::bbox3f_test_pt_inside");
    das::addExtern<DAS_BIND_FUN(bind_dascript::bbox3f_add_box)>(*this, lib, "bbox3f_add_box", das::SideEffects::modifyArgument,
      "bind_dascript::bbox3f_add_box");

    das::addExtern<float (*)(float, float), &safediv>(*this, lib, "safediv", das::SideEffects::none, "safediv");
    das::addExtern<double (*)(double, double), &safediv>(*this, lib, "safediv", das::SideEffects::none, "safediv");
    das::addExtern<float (*)(float), &safeinv>(*this, lib, "safeinv", das::SideEffects::none, "safeinv");
    das::addExtern<double (*)(double), &safeinv>(*this, lib, "safeinv", das::SideEffects::none, "safeinv");
    das::addExtern<float (*)(float), &safe_sqrt>(*this, lib, "safe_sqrt", das::SideEffects::none, "safe_sqrt");
    das::addExtern<double (*)(double), &safe_sqrt>(*this, lib, "safe_sqrt", das::SideEffects::none, "safe_sqrt");
    das::addExtern<float (*)(float), &safe_acos>(*this, lib, "safe_acos", das::SideEffects::none, "safe_acos");
    das::addExtern<double (*)(double), &safe_acos>(*this, lib, "safe_acos", das::SideEffects::none, "safe_acos");
    das::addExtern<float (*)(float), &safe_asin>(*this, lib, "safe_asin", das::SideEffects::none, "safe_asin");
    das::addExtern<double (*)(double), &safe_asin>(*this, lib, "safe_asin", das::SideEffects::none, "safe_asin");
    // don't add safe_atan2. Use atan2 instead. It's safe

    // todo: add TMatrix, Capsule functions
    das::addCtorAndUsing<Capsule>(*this, lib, "Capsule", " ::Capsule");
    using capsule_set = das::das_call_member<void (Capsule::*)(const Point3 &v0, const Point3 &v1, float radius), &Capsule::set>;
    das::addExtern<DAS_CALL_METHOD(capsule_set)>(*this, lib, "capsule_set", das::SideEffects::modifyArgument,
      "das_call_member<void (Capsule::*)(const Point3&, const Point3&, float), &Capsule::set>::invoke");
    using capsule_transform = das::das_call_member<void (Capsule::*)(const TMatrix &tm), &Capsule::transform>;
    das::addExtern<DAS_CALL_METHOD(capsule_transform)>(*this, lib, "capsule_transform", das::SideEffects::modifyArgument,
      "das_call_member<void (Capsule::*)(const TMatrix&), &Capsule::transform>::invoke");
    using capsule_traceray =
      das::das_call_member<bool (Capsule::*)(const Point3 &from, const Point3 &dir, float &t, Point3 &norm) const, &Capsule::traceRay>;
    das::addExtern<DAS_CALL_METHOD(capsule_traceray)>(*this, lib, "capsule_traceray", das::SideEffects::modifyArgument,
      "das_call_member<bool (Capsule::*)(const Point3&, const Point3&, float&, Point3&) const,"
      "&Capsule::traceRay>::invoke");

    das::addExtern<DAS_BIND_FUN(interpolate_tab_float_interpolate)>(*this, lib, "interpolate_tab_float_interpolate",
      das::SideEffects::none, "bind_dascript::interpolate_tab_float_interpolate");

    das::addCtorAndUsing<Plane3>(*this, lib, "Plane3", " ::Plane3");
    das::addCtorAndUsing<Plane3, real, real, real, real>(*this, lib, "Plane3", " ::Plane3");
    das::addCtorAndUsing<Plane3, const Point3 &, real>(*this, lib, "Plane3", " ::Plane3");
    das::addCtorAndUsing<Plane3, const Point3 &, const Point3 &>(*this, lib, "Plane3", " ::Plane3");
    das::addCtorAndUsing<Plane3, const Point3 &, const Point3 &, const Point3 &>(*this, lib, "Plane3", " ::Plane3");

    using plane3_normalize = DAS_CALL_MEMBER(Plane3::normalize);
    das::addExtern<DAS_CALL_METHOD(plane3_normalize)>(*this, lib, "normalize", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(Plane3::normalize));
    using plane3_calcLineIntersectionPoint = DAS_CALL_MEMBER(Plane3::calcLineIntersectionPoint);
    das::addExtern<DAS_CALL_METHOD(plane3_calcLineIntersectionPoint)>(*this, lib, "calcLineIntersectionPoint", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(Plane3::calcLineIntersectionPoint));

    das::addExtern<DAS_BIND_FUN(bind_dascript::mat44f_make_matrix), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "mat44f",
      das::SideEffects::none, "bind_dascript::mat44f_make_matrix");
    das::addExtern<DAS_BIND_FUN(bind_dascript::mat44f_make_tm), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "float3x4",
      das::SideEffects::none, "bind_dascript::mat44f_make_tm");

    das::addExtern<DAS_BIND_FUN(bind_dascript::v_distance_sq_to_bbox)>(*this, lib, "v_distance_sq_to_bbox", das::SideEffects::none,
      "bind_dascript::v_distance_sq_to_bbox");
    das::addExtern<DAS_BIND_FUN(bind_dascript::v_test_segment_box_intersection)>(*this, lib, "v_test_segment_box_intersection",
      das::SideEffects::none, "bind_dascript::v_test_segment_box_intersection");

    das::addExtern<DAS_BIND_FUN(bind_dascript::triangulate_poly)>(*this, lib, "triangulate_poly", das::SideEffects::accessExternal,
      "bind_dascript::triangulate_poly");

    compileBuiltinModule("dagorMath.das", (unsigned char *)dagorMath_das, sizeof(dagorMath_das));
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorMath.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorMath, bind_dascript);
