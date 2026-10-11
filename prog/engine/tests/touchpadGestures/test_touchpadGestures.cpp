// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <unittest/catch2_eastl_tostring.h>

#include <drv/hid/dag_hiTouchpadGestures.h>
#include <math/dag_mathBase.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <math.h>

template <>
struct Catch::StringMaker<Point2>
{
  static std::string convert(const Point2 &p) { return "(" + std::to_string(p.x) + ", " + std::to_string(p.y) + ")"; }
};

using namespace HumanInput;
using Catch::Matchers::WithinAbs;
using Recognizer = TouchpadGestureRecognizer;

namespace
{
constexpr int REPORT_MS = 4; // the input report rate of a DualSense or a DualShock 4

struct Logged
{
  Gesture g;
  Gesture::State state;
  int ms;
};

const char *const type_names[] = {"NONE", "TAP", "DRAG", "HOLD", "PINCH", "ROTATE", "FLICK", "DOUBLE_TAP"};
const char *const state_names[] = {"INACTIVE", "BEGIN", "ACTIVE", "END", "CANCELLED"};

// Point sequences of a 1920 x 1080 pad, one report every REPORT_MS, fed to the recognizer as the driver does
class Pad
{
public:
  Pad &touch(int slot, float x, float y)
  {
    points[slot].down = true;
    points[slot].contactId = nextId++ & 0x7F;
    setPos(slot, Point2(x, y));
    return *this;
  }
  Pad &lift(int slot)
  {
    points[slot].down = false;
    return *this;
  }
  Pad &press(bool down)
  {
    button = down;
    return *this;
  }
  Pad &wait(int ms, float jitter = 0)
  {
    const Point2 rest[2] = {pos(0), pos(1)};
    for (int t = 0, i = 0; t < ms; t += REPORT_MS, i++)
    {
      const Point2 shake = i ? Point2(i & 1 ? jitter : -jitter, i & 2 ? jitter : -jitter) : Point2(0, 0);
      for (int s = 0; s < 2; s++)
        setPos(s, rest[s] + shake);
      report();
    }
    for (int s = 0; s < 2; s++)
      setPos(s, rest[s]);
    return *this;
  }
  Pad &slide(int ms, int slot, float x, float y)
  {
    const Point2 from = pos(slot), to(x, y);
    const int steps = ms / REPORT_MS;
    for (int i = 1; i <= steps; i++)
    {
      setPos(slot, from + (to - from) * (float(i) / steps));
      report();
    }
    return *this;
  }
  Pad &slide2(int ms, float ax, float ay, float bx, float by)
  {
    const Point2 from[2] = {pos(0), pos(1)}, to[2] = {Point2(ax, ay), Point2(bx, by)};
    const int steps = ms / REPORT_MS;
    for (int i = 1; i <= steps; i++)
    {
      for (int s = 0; s < 2; s++)
        setPos(s, from[s] + (to[s] - from[s]) * (float(i) / steps));
      report();
    }
    return *this;
  }
  // slot 0 at center + arm, slot 1 opposite; with y down a growing angle turns clockwise
  Pad &orbit(int ms, const Point2 &center, float radius, float from_rad, float to_rad)
  {
    const int steps = ms / REPORT_MS;
    for (int i = 1; i <= steps; i++)
    {
      const float a = from_rad + (to_rad - from_rad) * (float(i) / steps);
      const Point2 arm(radius * cosf(a), radius * sinf(a));
      setPos(0, center + arm);
      setPos(1, center - arm);
      report();
    }
    return *this;
  }
  Pad &reset()
  {
    recognizer.reset(logger);
    return *this;
  }

  int now() const { return timeMs; }

  // "TYPE STATE, ...", a run of equal entries is one entry with "..."
  eastl::string log(Gesture::Type only = Gesture::NONE) const
  {
    eastl::string s, prev;
    bool repeated = false;
    for (const Logged &e : events)
    {
      if (only != Gesture::NONE && e.g.type != only)
        continue;
      const eastl::string item(eastl::string::CtorSprintf(), "%s %s", type_names[e.g.type], state_names[e.state]);
      if (item == prev)
      {
        if (!repeated)
          s += "...";
        repeated = true;
        continue;
      }
      if (!s.empty())
        s += ", ";
      s += item;
      prev = item;
      repeated = false;
    }
    return s;
  }

  const Logged &last(Gesture::Type type) const
  {
    for (int i = int(events.size()) - 1; i >= 0; i--)
      if (events[i].g.type == type)
        return events[i];
    FAIL("no " << type_names[type] << " event");
    return events[0];
  }

  Point2 sumDelta(Gesture::Type type) const
  {
    Point2 sum(0, 0);
    for (const Logged &e : events)
      if (e.g.type == type)
        sum += Point2(e.g.deltaX, e.g.deltaY);
    return sum;
  }

  eastl::vector<Logged> events;

private:
  struct Logger
  {
    Pad &pad;
    void operator()(Gesture::State state, const Gesture &g) { pad.events.push_back({g, state, pad.timeMs}); }
  };

  Point2 pos(int slot) const { return Point2(points[slot].x, points[slot].y); }
  void setPos(int slot, const Point2 &p)
  {
    points[slot].x = uint16_t(p.x + 0.5f);
    points[slot].y = uint16_t(p.y + 0.5f);
  }
  void report()
  {
    recognizer.update(points, button, timeMs, logger);
    timeMs += REPORT_MS;
  }

  Recognizer recognizer;
  Logger logger{*this};
  JoystickTouchpadState::Point points[2];
  int timeMs = 1000;
  uint8_t nextId = 0;
  bool button = false;
};

Pad &tap(Pad &pad, float x, float y) { return pad.touch(0, x, y).wait(100, 20).lift(0); }

Gesture make(Gesture::Type type, const Point2 &pos, const Point2 &delta = Point2(0, 0))
{
  Gesture g;
  g.type = type;
  g.x = pos.x;
  g.y = pos.y;
  g.deltaX = delta.x;
  g.deltaY = delta.y;
  return g;
}

eastl::vector<Logged> drain(TouchpadGestureQueue &q)
{
  eastl::vector<Logged> out;
  q.drain([&](Gesture::State state, const Gesture &g) { out.push_back({g, state, 0}); });
  return out;
}
} // namespace


TEST_CASE("a tap begins on release and ends when no second tap follows")
{
  Pad pad;
  pad.wait(20);
  tap(pad, 900, 500);
  const int releaseMs = pad.now();
  pad.wait(Recognizer::DOUBLE_TAP_GAP_MS + 40);
  REQUIRE(pad.log() == "TAP BEGIN, TAP END");
  CHECK(pad.events[0].ms == releaseMs);
  CHECK(pad.events[0].g.x == 900);
  CHECK(pad.events[0].g.y == 500);
  CHECK(pad.events[1].ms > releaseMs + Recognizer::DOUBLE_TAP_GAP_MS);
  CHECK(pad.events[1].ms <= releaseMs + Recognizer::DOUBLE_TAP_GAP_MS + REPORT_MS);
}

TEST_CASE("a second tap cancels the first and gives one double tap at the first tap")
{
  Pad pad;
  tap(pad, 900, 500).wait(120);
  tap(pad, 980, 540).wait(400);
  REQUIRE(pad.log() == "TAP BEGIN, TAP CANCELLED, DOUBLE_TAP ACTIVE");
  CHECK(pad.events[2].g.x == 900);
  CHECK(pad.events[2].g.y == 500);
}

TEST_CASE("a second tap that starts in time counts when it ends after the gap")
{
  Pad pad;
  tap(pad, 900, 500).wait(Recognizer::DOUBLE_TAP_GAP_MS - 40);
  pad.touch(0, 900, 500).wait(Recognizer::TAP_MAX_MS - 20).lift(0).wait(400);
  CHECK(pad.log() == "TAP BEGIN, TAP CANCELLED, DOUBLE_TAP ACTIVE");
}

TEST_CASE("taps too far apart in time or place are two single taps")
{
  Pad pad;
  SECTION("late")
  {
    tap(pad, 900, 500).wait(Recognizer::DOUBLE_TAP_GAP_MS + 40);
    tap(pad, 900, 500).wait(400);
    CHECK(pad.log() == "TAP BEGIN, TAP END, TAP BEGIN, TAP END");
  }
  SECTION("far")
  {
    tap(pad, 300, 500).wait(100);
    const int secondPressMs = pad.now();
    tap(pad, 1500, 500).wait(400);
    REQUIRE(pad.log() == "TAP BEGIN, TAP END, TAP BEGIN, TAP END");
    CHECK(pad.events[1].ms == secondPressMs);
  }
}

TEST_CASE("a touch longer than a tap and shorter than a hold gives nothing")
{
  Pad pad;
  pad.touch(0, 900, 500).wait(Recognizer::TAP_MAX_MS + 100, 20).lift(0).wait(400);
  CHECK(pad.log() == "");
}

TEST_CASE("a hold begins when the time passes and ends on release")
{
  Pad pad;
  const int pressMs = pad.now();
  pad.touch(0, 900, 500).wait(800, 20).lift(0);
  const int releaseMs = pad.now();
  pad.wait(400);
  REQUIRE(pad.log() == "HOLD BEGIN, HOLD END");
  CHECK(pad.events[0].ms >= pressMs + Recognizer::HOLD_MS);
  CHECK(pad.events[0].ms < pressMs + Recognizer::HOLD_MS + REPORT_MS);
  CHECK(pad.events[0].g.x == 900);
  CHECK(pad.events[1].ms == releaseMs);
}

TEST_CASE("a hold ends when the finger moves away and a drag begins")
{
  Pad pad;
  pad.touch(0, 900, 500).wait(600).slide(200, 0, 1300, 500).lift(0).wait(100);
  CHECK(pad.log() == "HOLD BEGIN, HOLD END, DRAG BEGIN, DRAG ACTIVE..., DRAG END");
}

TEST_CASE("the pad button stops taps, double taps and holds")
{
  Pad pad;
  SECTION("pressed with the touch")
  {
    pad.touch(0, 900, 500).press(true).wait(80).press(false).wait(20).lift(0).wait(400);
    CHECK(pad.log() == "");
  }
  SECTION("pressed during the touch")
  {
    pad.touch(0, 900, 500).wait(40).press(true).wait(40).press(false).wait(20).lift(0).wait(400);
    CHECK(pad.log() == "");
  }
  SECTION("held long")
  {
    pad.touch(0, 900, 500).wait(20).press(true).wait(800).press(false).lift(0).wait(100);
    CHECK(pad.log() == "");
  }
  SECTION("pressed after the hold began")
  {
    pad.touch(0, 900, 500).wait(600).press(true).wait(40).press(false).wait(40).lift(0).wait(100);
    CHECK(pad.log() == "HOLD BEGIN, HOLD CANCELLED");
  }
  SECTION("pressed in the second tap")
  {
    tap(pad, 900, 500).wait(100);
    pad.touch(0, 900, 500).wait(20);
    const int pressMs = pad.now();
    pad.press(true).wait(40).press(false).wait(20).lift(0).wait(400);
    REQUIRE(pad.log() == "TAP BEGIN, TAP END");
    CHECK(pad.events[1].ms == pressMs);
  }
  SECTION("pressed at the second press")
  {
    tap(pad, 900, 500).wait(100);
    const int pressMs = pad.now();
    pad.press(true).touch(0, 900, 500).wait(40).press(false).wait(20).lift(0).wait(400);
    REQUIRE(pad.log() == "TAP BEGIN, TAP END");
    CHECK(pad.events[1].ms == pressMs);
  }
}

TEST_CASE("a drag begins past the move limit and its deltas add up to the move")
{
  const Point2 press(400, 500), move(600, -200);
  Pad pad;
  SECTION("stopped before the release")
  {
    pad.touch(0, press.x, press.y).wait(40).slide(400, 0, press.x + move.x, press.y + move.y).wait(100).lift(0).wait(400);
  }
  SECTION("slow at the release")
  {
    pad.touch(0, press.x, press.y).wait(REPORT_MS).slide(600, 0, press.x + move.x, press.y + move.y).lift(0).wait(400);
  }
  REQUIRE(pad.log() == "DRAG BEGIN, DRAG ACTIVE..., DRAG END");
  const Gesture &begin = pad.events[0].g;
  const Point2 beginMove = Point2(begin.x, begin.y) - press;
  CHECK(Point2(begin.x0, begin.y0) == press);
  CHECK(beginMove.length() > Recognizer::MOVE_LIMIT_PX);
  CHECK(beginMove.length() < Recognizer::MOVE_LIMIT_PX + 10);
  CHECK(Point2(begin.deltaX, begin.deltaY) == beginMove);
  CHECK(pad.sumDelta(Gesture::DRAG) == move);
  const Gesture &end = pad.last(Gesture::DRAG).g;
  CHECK(Point2(end.x, end.y) == press + move);
}

TEST_CASE("a fast release gives a flick in the direction of the move")
{
  struct
  {
    float clockwiseFromUpDeg;
    Gesture::Direction dir;
  } const swipes[] = {{0, Gesture::UP}, {50, Gesture::UP}, {60, Gesture::RIGHT}, {90, Gesture::RIGHT}, {120, Gesture::RIGHT},
    {130, Gesture::DOWN}, {180, Gesture::DOWN}, {230, Gesture::DOWN}, {240, Gesture::LEFT}, {270, Gesture::LEFT}, {300, Gesture::LEFT},
    {310, Gesture::UP}};
  for (const auto &s : swipes)
  {
    const float rad = s.clockwiseFromUpDeg * DEG_TO_RAD;
    const Point2 move(400 * sinf(rad), -400 * cosf(rad));
    Pad pad;
    pad.touch(0, 960, 540).wait(20).slide(60, 0, 960 + move.x, 540 + move.y).lift(0).wait(400);
    INFO("swipe at " << s.clockwiseFromUpDeg << " degrees");
    REQUIRE(pad.log() == "DRAG BEGIN, DRAG ACTIVE..., DRAG END, FLICK ACTIVE");
    const Gesture &flick = pad.events.back().g;
    CHECK(flick.dir == s.dir);
    CHECK_THAT(flick.deltaX, WithinAbs(move.x, 1));
    CHECK_THAT(flick.deltaY, WithinAbs(move.y, 1));
    CHECK(flick.x0 == 960);
  }
}

TEST_CASE("a swipe gives the same flick answer whenever it starts")
{
  for (const float px : {200.f, 400.f})
  {
    const auto flicks = [px](int rest_ms) {
      Pad pad;
      pad.touch(0, 400, 540).wait(rest_ms).slide(40, 0, 400 + px, 540).lift(0).wait(20);
      return pad.events.back().g.type == Gesture::FLICK;
    };
    const bool first = flicks(20);
    for (int rest = 20; rest < 20 + 2 * Recognizer::FLICK_SPEED_SMOOTHING_MS; rest += REPORT_MS)
    {
      INFO(px << " px after a rest of " << rest << " ms");
      CHECK(flicks(rest) == first);
    }
  }
}

TEST_CASE("two fingers give a pinch and a rotation per update")
{
  Pad pad;
  pad.touch(0, 760, 540).touch(1, 1160, 540).wait(20).slide2(200, 660, 540, 1460, 540).wait(20).lift(1).wait(20).lift(0).wait(400);
  REQUIRE(pad.log(Gesture::PINCH) == "PINCH BEGIN, PINCH ACTIVE..., PINCH END");
  REQUIRE(pad.events.size() % 2 == 0);
  for (int i = 0; i < int(pad.events.size()); i += 2)
  {
    CHECK(pad.events[i].g.type == Gesture::PINCH);
    CHECK(pad.events[i + 1].g.type == Gesture::ROTATE);
    CHECK(pad.events[i + 1].state == pad.events[i].state);
  }
  const Gesture &end = pad.last(Gesture::PINCH).g;
  CHECK_THAT(end.scale, WithinAbs(2, 0.01));
  CHECK_THAT(end.angle, WithinAbs(0, 0.01));
  CHECK(end.x0 == 960);
  CHECK(end.x == 1060);
  CHECK(pad.sumDelta(Gesture::PINCH) == Point2(100, 0));
}

TEST_CASE("two fingers end a pending tap before the pair begins")
{
  Pad pad;
  tap(pad, 900, 500).wait(100);
  pad.touch(0, 760, 540).touch(1, 1160, 540).wait(20).lift(0).lift(1).wait(400);
  CHECK(pad.log() == "TAP BEGIN, TAP END, PINCH BEGIN, ROTATE BEGIN, PINCH END, ROTATE END");
}

TEST_CASE("the rotation angle is clockwise and goes past half a turn")
{
  const Point2 center(960, 540);
  const float turns[] = {HALFPI, -HALFPI, PI * 1.5f};
  for (const float turn : turns)
  {
    Pad pad;
    pad.touch(0, center.x + 300, center.y).touch(1, center.x - 300, center.y).wait(20);
    pad.orbit(400, center, 300, 0, turn).wait(20);
    INFO("turn to " << turn);
    const Gesture &rot = pad.last(Gesture::ROTATE).g;
    CHECK_THAT(rot.angle, WithinAbs(turn, 0.01));
    CHECK_THAT(rot.scale, WithinAbs(1, 0.01));
  }
}

TEST_CASE("a second finger ends a drag and the last finger of a pair gives nothing")
{
  Pad pad;
  pad.touch(0, 400, 500).slide(200, 0, 800, 500).touch(1, 1400, 600).wait(100).lift(1).wait(20);
  pad.slide(200, 0, 400, 500).wait(20).lift(0).wait(20);
  CHECK(pad.log() == "DRAG BEGIN, DRAG ACTIVE..., DRAG END, PINCH BEGIN, ROTATE BEGIN, PINCH END, ROTATE END");

  pad.events.clear();
  tap(pad, 900, 500).wait(400);
  CHECK(pad.log() == "TAP BEGIN, TAP END");
}

TEST_CASE("a finger that returns to the last finger of a pair begins a new pair")
{
  Pad pad;
  pad.touch(0, 760, 540).touch(1, 1160, 540).wait(20).lift(1).wait(20).touch(1, 1200, 540).wait(20);
  pad.lift(0).lift(1).wait(20);
  CHECK(pad.log() == "PINCH BEGIN, ROTATE BEGIN, PINCH END, ROTATE END, PINCH BEGIN, ROTATE BEGIN, PINCH END, ROTATE END");
}

TEST_CASE("a reset cancels what runs and ignores the fingers until they lift")
{
  Pad pad;
  SECTION("drag")
  {
    pad.touch(0, 400, 500).slide(200, 0, 800, 500).reset().slide(200, 0, 1200, 500).lift(0).wait(400);
    CHECK(pad.log() == "DRAG BEGIN, DRAG ACTIVE..., DRAG CANCELLED");
  }
  SECTION("hold")
  {
    pad.touch(0, 900, 500).wait(600).reset().wait(100).lift(0).wait(400);
    CHECK(pad.log() == "HOLD BEGIN, HOLD CANCELLED");
  }
  SECTION("pinch")
  {
    pad.touch(0, 760, 540).touch(1, 1160, 540).wait(20).reset().slide2(100, 660, 540, 1260, 540).lift(0).lift(1).wait(20);
    CHECK(pad.log() == "PINCH BEGIN, ROTATE BEGIN, PINCH CANCELLED, ROTATE CANCELLED");
  }
  SECTION("pending tap")
  {
    tap(pad, 900, 500).wait(100).reset().wait(400);
    CHECK(pad.log() == "TAP BEGIN, TAP CANCELLED");
  }
  pad.events.clear();
  tap(pad, 900, 500).wait(400);
  CHECK(pad.log() == "TAP BEGIN, TAP END");
}

TEST_CASE("the queue merges the moves of a gesture and keeps the rest in order")
{
  TouchpadGestureQueue q;
  SECTION("drag")
  {
    q(Gesture::BEGIN, make(Gesture::DRAG, Point2(101, 51), Point2(101, 51)));
    q(Gesture::ACTIVE, make(Gesture::DRAG, Point2(103, 50), Point2(2, -1)));
    q(Gesture::ACTIVE, make(Gesture::DRAG, Point2(106, 47), Point2(3, -3)));
    q(Gesture::END, make(Gesture::DRAG, Point2(106, 47)));
    q(Gesture::BEGIN, make(Gesture::DRAG, Point2(111, 47), Point2(5, 0)));
    q(Gesture::ACTIVE, make(Gesture::DRAG, Point2(112, 48), Point2(1, 1)));
    const eastl::vector<Logged> out = drain(q);
    REQUIRE(out.size() == 5);
    CHECK(out[1].state == Gesture::ACTIVE);
    CHECK(Point2(out[1].g.x, out[1].g.y) == Point2(106, 47));
    CHECK(Point2(out[1].g.deltaX, out[1].g.deltaY) == Point2(5, -4));
    CHECK(out[2].state == Gesture::END);
    CHECK(Point2(out[4].g.x, out[4].g.y) == Point2(112, 48));
  }
  SECTION("pinch and rotate")
  {
    Gesture pinch = make(Gesture::PINCH, Point2(0, 0)), rotate = make(Gesture::ROTATE, Point2(0, 0));
    q(Gesture::BEGIN, pinch);
    q(Gesture::BEGIN, rotate);
    for (int i = 1; i <= 3; i++)
    {
      pinch.scale = 1 + i * 0.25f;
      rotate.angle = i * 0.25f;
      pinch.deltaX = rotate.deltaX = 2;
      pinch.deltaY = rotate.deltaY = -1;
      q(Gesture::ACTIVE, pinch);
      q(Gesture::ACTIVE, rotate);
    }
    const eastl::vector<Logged> out = drain(q);
    REQUIRE(out.size() == 4);
    CHECK(out[2].g.type == Gesture::PINCH);
    CHECK(out[2].g.scale == 1.75f);
    CHECK(Point2(out[2].g.deltaX, out[2].g.deltaY) == Point2(6, -3));
    CHECK(out[3].g.type == Gesture::ROTATE);
    CHECK(out[3].g.angle == 0.75f);
  }
  SECTION("one event gestures")
  {
    q(Gesture::ACTIVE, make(Gesture::DRAG, Point2(1, 1), Point2(1, 1)));
    q(Gesture::ACTIVE, make(Gesture::FLICK, Point2(1, 1)));
    q(Gesture::ACTIVE, make(Gesture::DRAG, Point2(2, 2), Point2(1, 1)));
    q(Gesture::ACTIVE, make(Gesture::DOUBLE_TAP, Point2(2, 2)));
    q(Gesture::ACTIVE, make(Gesture::DOUBLE_TAP, Point2(2, 2)));
    CHECK(drain(q).size() == 5);
  }
  SECTION("full")
  {
    for (int i = 0; i < TouchpadGestureQueue::CAPACITY + 8; i++)
      q(Gesture::BEGIN, make(Gesture::TAP, Point2(i, 0)));
    const eastl::vector<Logged> out = drain(q);
    REQUIRE(out.size() == TouchpadGestureQueue::CAPACITY);
    CHECK(out[0].g.x == 8);
  }
  CHECK(drain(q).empty());
}
