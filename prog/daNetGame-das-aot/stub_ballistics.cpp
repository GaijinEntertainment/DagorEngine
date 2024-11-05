// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/ballistics/projectileBallisticProps.h>
#include <gamePhys/ballistics/projectileBallistics.h>
#include <gamePhys/ballistics/shellBallisticProps.h>
#include <gamePhys/ballistics/shellBallistics.h>
#include <gamePhys/ballistics/rocketMotorProps.h>
#include <gamePhys/ballistics/shellPayloadProps.h>
namespace ballistics
{
const ProjectileProps *ProjectileProps::get_props(int) { G_ASSERT_RETURN(false, nullptr); }
const ProjectileProps *ProjectileProps::try_get_props(int) { G_ASSERT_RETURN(false, nullptr); }
void ProjectileBallistics::setup(const Point3 &in_pos, const Point3 &in_vel, float g_mult) { G_ASSERT(0); }
void ProjectileBallistics::setupWithKV(
  const ProjBallisticsProperties &props, const Point3 &in_pos, const Point3 &in_vel, float g_mult, float density)
{
  G_ASSERT(0);
}
void ProjectileBallistics::move(int, float) { G_ASSERT(0); }

const ShellProps *ShellProps::get_props(int) { G_ASSERT_RETURN(false, nullptr); }

void setup(ShellState &, const Point3 &, const Quat &, const Point3 &, const Point3 &, float, float, float, bool, bool, float)
{
  G_ASSERT(0);
}
void simulate(const ShellEnv &, const ShellBallisticsProperties &, ShellState &, float, float) { G_ASSERT(0); }

const RocketMotorProps *RocketMotorProps::get_props(int) { G_ASSERT_RETURN(false, nullptr); }

void calc_rocket_motor_thrust_mass(
  const RocketMotorProps &, const Point2 &, const Point2 &, float, int &, Point3 &, Point3 &, Point3 &, float &)
{
  G_ASSERT(0);
}
void calc_rocket_motor_propulsion_mass(const RocketMotorProps &, float, float &, float &, float &) { G_ASSERT(0); }
void apply_rocket_start_speed(const RocketMotorProps &, const Quat &, float, int, Point3 &) { G_ASSERT(0); }
void apply_rocket_start_orientation(const RocketMotorProps &, float, int, Quat &) { G_ASSERT(0); }
void apply_rocket_end_speed(const RocketMotorProps &, Point3 &) { G_ASSERT(0); }

void ProjectileBallistics::setupWithKVEx(
  const ProjectileProps &props, const Point3 &in_pos, const Point3 &in_vel, float g_mult, float density)
{
  G_ASSERT(0);
}
} // namespace ballistics

const ShellPayloadProps *ShellPayloadProps::get_props(int) { G_ASSERT_RETURN(false, nullptr); }
