// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotDagorMathUtils.h>

namespace bind_dascript
{
class DagorMathUtils final : public das::Module
{
public:
  DagorMathUtils() : das::Module("DagorMathUtils")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("DagorMath"));

    das::addExtern<DAS_BIND_FUN(test_segment_cylinder_intersection)>(*this, lib, "test_segment_cylinder_intersection",
      das::SideEffects::none, "::test_segment_cylinder_intersection");
    das::addExtern<DAS_BIND_FUN(test_segment_box_intersection)>(*this, lib, "test_segment_box_intersection", das::SideEffects::none,
      "::test_segment_box_intersection");
    das::addExtern<DAS_BIND_FUN(does_line_intersect_box_side)>(*this, lib, "does_line_intersect_box_side",
      das::SideEffects::modifyArgument, "::does_line_intersect_box_side");
    das::addExtern<DAS_BIND_FUN(rayIntersectWaBoxIgnoreInside)>(*this, lib, "rayIntersectWaBoxIgnoreInside", das::SideEffects::none,
      "::rayIntersectWaBoxIgnoreInside");
    das::addExtern<DAS_BIND_FUN(rayIntersectBoxIgnoreInside)>(*this, lib, "rayIntersectBoxIgnoreInside", das::SideEffects::none,
      "::rayIntersectBoxIgnoreInside");
    das::addExtern<bool (*)(const Point3 &, const Point3 &, const Point3 &, float), &test_segment_sphere_intersection>(*this, lib,
      "test_segment_sphere_intersection", das::SideEffects::none, "::test_segment_sphere_intersection");
    das::addExtern<bool (*)(const Point3 &, const Point3 &, const Point3 &, float, float &), &test_segment_sphere_intersection>(*this,
      lib, "test_segment_sphere_intersection", das::SideEffects::modifyArgument, "::test_segment_sphere_intersection");
    das::addExtern<DAS_BIND_FUN(dir_to_angles)>(*this, lib, "dir_to_angles", das::SideEffects::none, "::dir_to_angles");
    das::addExtern<DAS_BIND_FUN(angles_to_dir)>(*this, lib, "angles_to_dir", das::SideEffects::none, "::angles_to_dir");
    das::addExtern<DAS_BIND_FUN(basis_aware_dir_to_angles)>(*this, lib, "basis_aware_dir_to_angles", das::SideEffects::none,
      "::basis_aware_dir_to_angles");
    das::addExtern<DAS_BIND_FUN(basis_aware_angles_to_dir)>(*this, lib, "basis_aware_angles_to_dir", das::SideEffects::none,
      "::basis_aware_angles_to_dir");
    das::addExtern<DAS_BIND_FUN(basis_aware_clamp_angles_by_dir)>(*this, lib, "basis_aware_clamp_angles_by_dir",
      das::SideEffects::none, "::basis_aware_clamp_angles_by_dir");
    das::addExtern<DAS_BIND_FUN(basis_aware_xVz)>(*this, lib, "basis_aware_xVz", das::SideEffects::none, "::basis_aware_xVz");
    das::addExtern<DAS_BIND_FUN(basis_aware_x0z)>(*this, lib, "basis_aware_x0z", das::SideEffects::none, "::basis_aware_x0z");
    das::addExtern<DAS_BIND_FUN(relative_2d_dir_to_absolute_3d_dir)>(*this, lib, "relative_2d_dir_to_absolute_3d_dir",
      das::SideEffects::none, "::relative_2d_dir_to_absolute_3d_dir");
    das::addExtern<DAS_BIND_FUN(absolute_3d_dir_to_relative_2d_dir)>(*this, lib, "absolute_3d_dir_to_relative_2d_dir",
      das::SideEffects::none, "::absolute_3d_dir_to_relative_2d_dir");
    das::addExtern<float (*)(float, float), &angle_diff>(*this, lib, "angle_diff", das::SideEffects::none, "::angle_diff");
    das::addExtern<Point2 (*)(const Point2 &, const Point2 &), &angle_diff>(*this, lib, "angle_diff", das::SideEffects::none,
      "::angle_diff");
    das::addExtern<DAS_BIND_FUN(renorm_ang)>(*this, lib, "renorm_ang", das::SideEffects::none, "::renorm_ang");
    das::addExtern<DAS_BIND_FUN(lineLineIntersect)>(*this, lib, "line_line_intersect", das::SideEffects::modifyArgument,
      "::lineLineIntersect");
    das::addExtern<DAS_BIND_FUN(is_point_in_poly)>(*this, lib, "is_point_in_poly", das::SideEffects::none, "::is_point_in_poly");
    das::addExtern<DAS_BIND_FUN(reverse_bits32)>(*this, lib, "reverse_bits32", das::SideEffects::none, "::reverse_bits32");

    das::addExtern<DAS_BIND_FUN(point3_get_positional_seed)>(*this, lib, "get_positional_seed", das::SideEffects::none,
      "bind_dascript::point3_get_positional_seed");

    das::addExtern<DAS_BIND_FUN(get_some_normal)>(*this, lib, "get_some_normal", das::SideEffects::none, "get_some_normal");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorMathUtils.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorMathUtils, bind_dascript);
