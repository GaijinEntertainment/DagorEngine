// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>

#include "../../drv/hid_commonCode/joyMotionHistory.h"
#include <EASTL/vector.h>

using namespace HumanInput;

namespace
{
constexpr uint32_t REPORT_US = 4000;

JoystickMotionSample make_sample(uint32_t i)
{
  JoystickMotionSample s;
  for (int a = 0; a < 3; a++)
  {
    s.angVelDegS[a] = float(i * 3 + a);
    s.accelG[a] = float(i * 3 + a) + 0.5f;
  }
  s.timestampUs = i * REPORT_US;
  return s;
}

void push_range(JoystickMotionHistory &h, uint32_t from, uint32_t to)
{
  for (uint32_t i = from; i < to; i++)
    h.push(make_sample(i));
}

void check_range(const eastl::vector<JoystickMotionSample> &got, int n, uint32_t from)
{
  for (int k = 0; k < n; k++)
  {
    CAPTURE(k);
    const JoystickMotionSample want = make_sample(from + k);
    CHECK(got[k].timestampUs == want.timestampUs);
    for (int a = 0; a < 3; a++)
    {
      CHECK(got[k].angVelDegS[a] == want.angVelDegS[a]);
      CHECK(got[k].accelG[a] == want.accelG[a]);
    }
  }
}
} // namespace

TEST_CASE("off keeps nothing", "[joyMotionHistory]")
{
  JoystickMotionHistory h;
  push_range(h, 0, 10);
  uint32_t dropped = 1;
  CHECK(h.fetch({}, dropped) == 0);
  CHECK(dropped == 0);
}

TEST_CASE("fetch gives the samples since the last fetch, oldest first", "[joyMotionHistory]")
{
  JoystickMotionHistory h;
  h.enable(true);
  eastl::vector<JoystickMotionSample> out(JoystickMotionHistory::CAPACITY);
  uint32_t dropped = 1;

  push_range(h, 0, 5);
  REQUIRE(h.fetch(make_span(out), dropped) == 5);
  CHECK(dropped == 0);
  check_range(out, 5, 0);

  dropped = 1;
  CHECK(h.fetch(make_span(out), dropped) == 0);
  CHECK(dropped == 0);

  push_range(h, 5, 8);
  REQUIRE(h.fetch(make_span(out), dropped) == 3);
  check_range(out, 3, 5);
}

TEST_CASE("a full ring drops the oldest samples and counts them", "[joyMotionHistory]")
{
  JoystickMotionHistory h;
  h.enable(true);
  eastl::vector<JoystickMotionSample> out(JoystickMotionHistory::CAPACITY);
  uint32_t dropped = 0;

  push_range(h, 0, JoystickMotionHistory::CAPACITY + 9);
  REQUIRE(h.fetch(make_span(out), dropped) == JoystickMotionHistory::CAPACITY);
  CHECK(dropped == 9);
  check_range(out, JoystickMotionHistory::CAPACITY, 9);

  CHECK(h.fetch(make_span(out), dropped) == 0);
  CHECK(dropped == 0);
}

TEST_CASE("a short output gets the newest samples", "[joyMotionHistory]")
{
  JoystickMotionHistory h;
  h.enable(true);
  eastl::vector<JoystickMotionSample> out(4);
  uint32_t dropped = 0;

  push_range(h, 0, 10);
  REQUIRE(h.fetch(make_span(out), dropped) == 4);
  CHECK(dropped == 6);
  check_range(out, 4, 6);

  push_range(h, 10, 12);
  REQUIRE(h.fetch({}, dropped) == 0);
  CHECK(dropped == 2);

  push_range(h, 0, JoystickMotionHistory::CAPACITY + 3);
  REQUIRE(h.fetch(make_span(out), dropped) == 4);
  CHECK(dropped == JoystickMotionHistory::CAPACITY - 1);
  check_range(out, 4, JoystickMotionHistory::CAPACITY - 1);
}

TEST_CASE("a repeated enable keeps the samples, clear and disable drop them", "[joyMotionHistory]")
{
  JoystickMotionHistory h;
  eastl::vector<JoystickMotionSample> out(JoystickMotionHistory::CAPACITY);
  uint32_t dropped = 0;

  h.enable(true);
  push_range(h, 0, 3);
  h.enable(true);
  REQUIRE(h.fetch(make_span(out), dropped) == 3);
  check_range(out, 3, 0);

  push_range(h, 3, 3 + JoystickMotionHistory::CAPACITY + 1);
  h.clear();
  dropped = 1;
  CHECK(h.fetch(make_span(out), dropped) == 0);
  CHECK(dropped == 0);

  push_range(h, 3, 3 + JoystickMotionHistory::CAPACITY + 1);
  h.enable(false);
  dropped = 1;
  CHECK(h.fetch(make_span(out), dropped) == 0);
  CHECK(dropped == 0);

  h.enable(true);
  CHECK(h.fetch(make_span(out), dropped) == 0);
  push_range(h, 100, 102);
  REQUIRE(h.fetch(make_span(out), dropped) == 2);
  check_range(out, 2, 100);
}
