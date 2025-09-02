// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/rendinstDestr.h>

namespace rendinstdestr
{
bool apply_damage_to_riextra(rendinst::riex_handle_t, float, const Point3 &, const Point3 &, float) { G_ASSERT_RETURN(false, false); }
void remove_ri_without_collision_in_radius(const Point3 &, float) { G_ASSERT(0); }
void damage_ri_in_sphere(const Point3 &, float, const Point2 &, float, float, bool, on_riextra_destroyed_callback &&,
  riextra_should_damage &&)
{
  G_ASSERT(0);
}
void doRIExtraDamageInBox(const BBox3 &, float, bool, const Point3 &, calc_expl_damage_cb, const BSphere3 *, const TMatrix *,
  rendinst::DestrOptionFlags)
{
  G_ASSERT(0);
}
void destroyRiExtra(rendinst::riex_handle_t, const TMatrix &, bool, const Point3 &, const Point3 &, rendinst::DestrOptionFlags)
{
  G_ASSERT(0);
}
rendinst::RendInstDesc destroyRendinst(rendinst::RendInstDesc, bool, const Point3 &, const Point3 &, float,
  const rendinst::CollisionInfo *, bool, ApexDmgInfo *, int, float, on_destr_callback, rendinst::DestrOptionFlags)
{
  G_ASSERT_RETURN(false, rendinst::RendInstDesc());
}
rendinst::ri_damage_effect_cb get_ri_damage_effect_cb() { G_ASSERT_RETURN(false, nullptr); }
const DestrSettings &get_destr_settings()
{
  G_ASSERT(0);
  static DestrSettings s;
  return s;
}
CachedCollisionObjectInfo *get_or_add_cached_collision_object(rendinst::RendInstDesc const &, float)
{
  G_ASSERT_RETURN(false, nullptr);
}
} // namespace rendinstdestr

#include <gamePhys/phys/rendinstDestr.h>

dag::ConstSpan<gamephys::DestructableObject *> destructables::getDestructableObjects()
{
  G_ASSERT_RETURN(false, dag::ConstSpan<gamephys::DestructableObject *>());
}

#include <gamePhys/phys/physVars.h>
int PhysVars::registerVar(const char *, float) { G_ASSERT_RETURN(false, -1); }
int PhysVars::registerPullVar(const char *, float) { G_ASSERT_RETURN(false, -1); }
float PhysVars::getVar(int) const { G_ASSERT_RETURN(false, -1); }

#include <gamePhys/phys/animatedPhys.h>
void AnimatedPhys::init(const AnimV20::AnimcharBaseComponent &, const PhysVars &) { G_ASSERT(0); }
void AnimatedPhys::appendVar(const char *, const AnimV20::AnimcharBaseComponent &, const PhysVars &) { G_ASSERT(0); }
void AnimatedPhys::update(AnimV20::AnimcharBaseComponent &, PhysVars &) { G_ASSERT(0); }

#include <gamePhys/phys/commonPhysBase.h>
bool CommonPhysPartialState::deserialize(const danet::BitStream &, IPhysBase &) { G_ASSERT_RETURN(false, false); }

#include <gamePhys/phys/utils.h>
namespace gamephys
{
namespace atmosphere
{
float density(float)
{
  G_ASSERT(0);
  return 0.0f;
}
Point3 get_wind()
{
  G_ASSERT(0);
  return Point3();
}
float temperature(float)
{
  G_ASSERT(0);
  return 0.0f;
}
float sonicSpeed(float)
{
  G_ASSERT(0);
  return 0.0f;
}
} // namespace atmosphere
void Orient::setYP0(const Point3 &) { G_ASSERT(0); }
void Orient::setQuat(const Quat &) { G_ASSERT(0); }
void Orient::wrap() { G_ASSERT(0); }
void extrapolate_circular(const Point3 &, const Point3 &, const Point3 &, float, Point3 &, Point3 &) { G_ASSERT(0); }
} // namespace gamephys

#include <gamePhys/common/mass.h>
namespace gamephys
{
void Mass::setFuel(float, int, bool) { G_ASSERT(0); }
bool Mass::hasFuel(float, int) const { G_ASSERT_RETURN(false, false); }
float Mass::getFuelMassCurrent() const { G_ASSERT_RETURN(false, 0.f); };
float Mass::getFuelMassCurrent(int) const { G_ASSERT_RETURN(false, 0.f); };
float Mass::getFuelMassMax() const { G_ASSERT_RETURN(false, 0.f); };
float Mass::getFuelMassMax(int) const { G_ASSERT_RETURN(false, 0.f); };
} // namespace gamephys
