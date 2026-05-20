// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/ballistics/projectileBallisticProps.h>
#include <gamePhys/ballistics/projectileBallistics.h>
#include <gamePhys/ballistics/shellBallisticProps.h>
#include <gamePhys/ballistics/shellBallistics.h>
#include <gamePhys/ballistics/rocketMotorProps.h>
#include <gamePhys/ballistics/shellPayloadProps.h>
#include <gamePhys/ballistics/ballisticBrakeProps.h>

const ballistics::PropulsionsValue ballistics::propulsionsMultDefault = {1.0f, 0.0f /*...*/};
ballistics::RocketMotorSettings ballistics::rocket_motor_settings = {};

namespace ballistics
{
float get_terrain_elevation(const Point3 &, Point3 &) { return 0.f; }
const PropulsionsValue &get_propulsions_mult_default() { return propulsionsMultDefault; }
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
const ShellProps *ShellProps::try_get_props(int) { G_ASSERT_RETURN(false, nullptr); }

const BallisticBrakeProps *BallisticBrakeProps::get_props(int) { G_ASSERT_RETURN(false, nullptr); }
const BallisticBrakeProps *BallisticBrakeProps::try_get_props(int) { G_ASSERT_RETURN(false, nullptr); }
void calc_ballistic_brake_effect(const BallisticBrakeProps &, float, float, float &, float &) { G_ASSERT(0); }

void setup(ShellState &, const Point3 &, const Quat &, const Point3 &, const Point3 &, float, float, float, bool, bool, float)
{
  G_ASSERT(0);
}
void simulate(const ShellEnv &, const ShellBallisticsProperties &, ShellState &, float, float) { G_ASSERT(0); }
void reset(ShellState &) { G_ASSERT(0); }
ShellBallisticsProperties::ShellBallisticsProperties() {}
ShellState interpolate(const ShellState &a, const ShellState &, float) { G_ASSERT_RETURN(false, a); }
float calc_cx(float) { G_ASSERT_RETURN(false, 0.f); }
float calc_cxi(float) { G_ASSERT_RETURN(false, 0.f); }

const RocketMotorProps *RocketMotorProps::get_props(int) { G_ASSERT_RETURN(false, nullptr); }

float get_fire_delay_0(const RocketMotorProps &prop)
{
  G_ASSERT(0);
  return 0.0f;
}

void add_propulsion_time(float, const RocketMotorPropulsionsValue &, RocketMotorPropulsionsValue &) { G_ASSERT(0); }

void calc_rocket_motor_thrust_mass(const RocketMotorProps &,
  const Point2 &,
  const Point2 &,
  const RocketMotorPropulsionsValue &,
  const RocketMotorPropulsionsValue &,
  int &,
  Point3 &,
  Point3 &,
  Point3 &,
  float &)
{
  G_ASSERT(0);
}
void calc_rocket_motor_propulsion_mass(
  const RocketMotorProps &, const RocketMotorPropulsionsValue &, const RocketMotorPropulsionsValue &, float &, float &, float &)
{
  G_ASSERT(0);
}
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
