// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <drv/hid/dag_hiJoyMotionFilter.h>
#include <math/dag_mathBase.h>
#include <math.h>

using namespace HumanInput;
using Catch::Matchers::WithinAbs;
using Filter = JoystickMotionFilter;

namespace
{
constexpr float DT = 0.004f; // the report rate of a DualSense or a DualShock 4
const Point3 FLAT_UP(0, 1, 0);

int samples(float sec) { return int(sec / DT + 0.5f); }

// a pad on a table: sensor noise only
Point3 table_noise(int i) { return Point3(sinf(i * 1.7f), cosf(i * 2.3f), sinf(i * 3.1f)) * 0.15f; }

// a hand that holds the pad still: the tremor of 9 Hz
Point3 hand_tremor(int i) { return Point3(3.f, -2.f, 2.5f) * sinf(i * DT * TWOPI * 9.f); }

void rest(Filter &f, const Point3 &bias, float sec, const Point3 &up = FLAT_UP)
{
  for (int i = 0; i < samples(sec); i++)
    f.update(bias + table_noise(i), up + table_noise(i) * 0.02f, DT);
}

void check_near(const Point3 &a, const Point3 &b, float tolerance)
{
  CHECK_THAT(a.x, WithinAbs(b.x, tolerance));
  CHECK_THAT(a.y, WithinAbs(b.y, tolerance));
  CHECK_THAT(a.z, WithinAbs(b.z, tolerance));
}
} // namespace

TEST_CASE("a resting pad gives its gyro bias", "[joyMotionFilter]")
{
  Filter f;
  rest(f, Point3(-1.f, 0.5f, 0), Filter::AUTO_REST_SEC + 1.f);
  for (int i = 0; i < samples(0.5f); i++)
    f.update(hand_tremor(i), FLAT_UP, DT);
  const Point3 bias(0.4f, -0.7f, 0.25f);
  rest(f, bias, Filter::AUTO_REST_SEC + 1.f);
  check_near(f.biasDegS, bias, 0.02f);
  CHECK(f.update(bias, FLAT_UP, DT).length() < 0.02f);
}

TEST_CASE("a held pad keeps its gyro bias", "[joyMotionFilter]")
{
  Filter f;
  rest(f, Point3(0.5f, 0, 0), Filter::AUTO_REST_SEC - 0.5f); // a short pause is no rest yet
  CHECK(f.biasDegS.lengthSq() == 0);
  for (int i = 0; i < samples(10.f); i++)
    f.update(Point3(0.5f, 0, 0) + hand_tremor(i), FLAT_UP, DT);
  for (int i = 0; i < samples(10.f); i++) // a steady hand still sways the accelerometer
    f.update(Point3(0.5f, 0, 0), FLAT_UP + hand_tremor(i) * 0.02f, DT);
  CHECK(f.biasDegS.lengthSq() == 0);
}

TEST_CASE("a large gyro bias waits for calibrate", "[joyMotionFilter]")
{
  Filter f;
  const Point3 bias(5.f, 0, -1.f);
  rest(f, bias, 5.f);
  CHECK(f.biasDegS.lengthSq() == 0);

  f.calibrate();
  rest(f, bias, Filter::CALIBRATE_REST_SEC - 1.f);
  for (int i = 0; i < samples(0.5f); i++)
    f.update(bias + hand_tremor(i), FLAT_UP, DT);
  rest(f, bias, Filter::CALIBRATE_REST_SEC - 1.f);
  CHECK(f.calibrating);
  rest(f, bias, 1.5f);
  CHECK_FALSE(f.calibrating);
  check_near(f.biasDegS, bias, 0.02f);
}

TEST_CASE("a turn around gravity of a pitched pad", "[joyMotionFilter]")
{
  Filter f;
  const float pitch = DegToRad(40.f);
  const Point3 up(0, cosf(pitch), sinf(pitch));
  rest(f, Point3(0, 0, 0), 1.f, up);

  float aroundGravityDeg = 0, ownAxisDeg = 0;
  for (int i = 0; i < samples(1.f); i++)
  {
    const Point3 w = f.update(up * 90.f, up, DT);
    aroundGravityDeg += f.yaw(w, true) * DT;
    ownAxisDeg += f.yaw(w, false) * DT;
  }
  CHECK_THAT(aroundGravityDeg, WithinAbs(90.f, 0.5f));
  CHECK_THAT(ownAxisDeg, WithinAbs(90.f * cosf(pitch), 0.5f));

  const Point3 roll(0, 0, 90.f); // off the gravity axis, so the player space projection stays below the clamp
  CHECK_THAT(f.yaw(roll, true), WithinAbs(90.f * sinf(pitch) * 1.41f, 0.5f));
  CHECK(f.yaw(roll, false) == 0);
}

TEST_CASE("player space leaves the pitch axis out of a rolled pad's yaw", "[joyMotionFilter]")
{
  Filter f;
  const Point3 up(sinf(DegToRad(45.f)), cosf(DegToRad(45.f)), 0);
  rest(f, Point3(0, 0, 0), 1.f, up);
  CHECK_THAT(f.yaw(up * 90.f, true), WithinAbs(90.f * 0.5f * 1.41f, 0.05f));
}

TEST_CASE("an unknown gravity turns around the pad's own axis", "[joyMotionFilter]")
{
  Filter f;
  f.update(Point3(0, 0, 0), Point3(0, 0.3f, 0), DT);
  CHECK(f.yaw(Point3(10.f, 20.f, 30.f), true) == 20.f);
}

TEST_CASE("gravity follows the gyro and ignores hand shake", "[joyMotionFilter]")
{
  Filter f;
  const Point3 bias(40.f, 0, 0); // turning gravity by the raw rate would overshoot 10 degrees
  f.calibrate();
  rest(f, bias, Filter::CALIBRATE_REST_SEC + 3.f);
  for (int i = 0; i < samples(0.25f); i++) // the accelerometer reads too little in a swing, the gyro alone turns gravity
    f.update(bias + Point3(360.f, 0, 0), Point3(0, 0.3f, 0), DT);
  check_near(f.up, Point3(0, 0, -1), 0.05f);

  for (int i = 0; i < samples(1.f); i++)
    f.update(bias, Point3(0.5f * sinf(i * DT * TWOPI * 10.f), 0, -1), DT);
  check_near(f.up, Point3(0, 0, -1), 0.05f);
}
