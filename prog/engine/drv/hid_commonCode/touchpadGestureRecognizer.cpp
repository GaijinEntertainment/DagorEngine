// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiTouchpadGestures.h>
#include <math/dag_mathBase.h>
#include <math.h>

using namespace HumanInput;
using TouchPoint = JoystickTouchpadState::Point;
using Recognizer = TouchpadGestureRecognizer;

static_assert(Recognizer::HOLD_MS > Recognizer::TAP_MAX_MS, "a touch that began a hold must not end as a tap");

static const float flick_vertical_cos = cosf(Recognizer::FLICK_VERTICAL_HALF_ANGLE_DEG * DEG_TO_RAD);

static Point2 pos_of(const TouchPoint &p) { return Point2(p.x, p.y); }

static const TouchPoint *find_contact(const TouchPoint *const *down, int count, uint8_t id)
{
  for (int i = 0; i < count; i++)
    if (down[i]->contactId == id)
      return down[i];
  return nullptr;
}

static Gesture make_gesture(Gesture::Type type, const Point2 &at, const Point2 &from)
{
  Gesture g;
  g.type = type;
  g.x = at.x;
  g.y = at.y;
  g.x0 = from.x;
  g.y0 = from.y;
  return g;
}

static Gesture::Direction flick_direction(const Point2 &move)
{
  const float up = -move.y * safeinv(move.length());
  if (up > flick_vertical_cos)
    return Gesture::UP;
  if (up < -flick_vertical_cos)
    return Gesture::DOWN;
  return move.x > 0 ? Gesture::RIGHT : Gesture::LEFT;
}

void Recognizer::update(const TouchPoint (&points)[2], bool pad_button_down, int time_ms, GestureCb cb)
{
  const TouchPoint *down[2] = {};
  int count = 0;
  for (const TouchPoint &p : points)
    if (p.down)
      down[count++] = &p;

  if (tapPending && !secondTap && time_ms - tapReleaseMs > DOUBLE_TAP_GAP_MS)
    endPendingTap(Gesture::END, cb);

  switch (mode)
  {
    case Mode::IDLE: startTouch(down, count, pad_button_down, time_ms, cb); break;
    case Mode::ONE:
    {
      const TouchPoint *p = find_contact(down, count, touchId);
      if (p && count == 1)
        moveOne(pos_of(*p), pad_button_down, time_ms, cb);
      else
      {
        finishOne(!p, time_ms, cb);
        startTouch(down, count, pad_button_down, time_ms, cb);
      }
      break;
    }
    case Mode::TWO:
    {
      const TouchPoint *a = find_contact(down, count, pairId[0]), *b = find_contact(down, count, pairId[1]);
      if (a && b)
        movePair(pos_of(*a), pos_of(*b), cb);
      else
      {
        sendPair(Gesture::END, Point2(0, 0), cb);
        mode = count ? Mode::ONE_OF_TWO : Mode::IDLE;
        if (count == 2)
          beginPair(down, cb);
      }
      break;
    }
    case Mode::ONE_OF_TWO:
      if (count == 2)
        beginPair(down, cb);
      else if (count == 0)
        mode = Mode::IDLE;
      break;
    case Mode::UNTIL_LIFT:
      if (count == 0)
        mode = Mode::IDLE;
      break;
  }
}

void Recognizer::reset(GestureCb cb)
{
  if (mode == Mode::ONE)
  {
    endHold(Gesture::CANCELLED, cb);
    if (moved)
      sendDrag(Gesture::CANCELLED, lastPos, cb);
  }
  else if (mode == Mode::TWO)
    sendPair(Gesture::CANCELLED, Point2(0, 0), cb);
  if (tapPending)
    endPendingTap(Gesture::CANCELLED, cb);
  moved = secondTap = false;
  if (mode != Mode::IDLE)
    mode = Mode::UNTIL_LIFT;
}

void Recognizer::startTouch(const TouchPoint *const *down, int count, bool pad_button_down, int time_ms, GestureCb cb)
{
  if (count == 2)
    beginPair(down, cb);
  if (count != 1)
    return;
  mode = Mode::ONE;
  touchId = down[0]->contactId;
  pressPos = lastPos = flickTrail = pos_of(*down[0]);
  pressMs = lastMs = time_ms;
  moved = holding = false;
  buttonInTouch = pad_button_down;
  secondTap = tapPending && !buttonInTouch && lengthSq(pressPos - tapPos) <= sqr(DOUBLE_TAP_MAX_DIST_PX);
  if (tapPending && !secondTap)
    endPendingTap(Gesture::END, cb);
}

void Recognizer::moveOne(const Point2 &pos, bool pad_button_down, int time_ms, GestureCb cb)
{
  if (pad_button_down && !buttonInTouch)
  {
    buttonInTouch = true;
    endHold(Gesture::CANCELLED, cb);
    dropSecondTap(cb);
  }
  if (!moved && lengthSq(pos - pressPos) > sqr(MOVE_LIMIT_PX))
  {
    moved = true;
    endHold(Gesture::END, cb);
    dropSecondTap(cb);
    sendDrag(Gesture::BEGIN, pos, cb);
  }
  else if (moved && pos != lastPos)
    sendDrag(Gesture::ACTIVE, pos, cb);

  if (time_ms - pressMs > TAP_MAX_MS)
    dropSecondTap(cb);
  if (!holding && !moved && !buttonInTouch && time_ms - pressMs >= HOLD_MS)
  {
    holding = true;
    cb(Gesture::BEGIN, make_gesture(Gesture::HOLD, pressPos, pressPos));
  }
  flickTrail += (pos - flickTrail) * (1 - expf(float(lastMs - time_ms) / FLICK_SPEED_SMOOTHING_MS));
  lastMs = time_ms;
}

void Recognizer::finishOne(bool lifted, int time_ms, GestureCb cb)
{
  mode = Mode::IDLE;
  endHold(Gesture::END, cb);
  if (moved)
  {
    sendDrag(Gesture::END, lastPos, cb);
    // the trail lags a steady finger by its speed times the smoothing time
    if (lifted && length(lastPos - flickTrail) >= FLICK_MIN_SPEED_PX_PER_MS * FLICK_SPEED_SMOOTHING_MS)
    {
      Gesture g = make_gesture(Gesture::FLICK, lastPos, pressPos);
      const Point2 move = lastPos - pressPos;
      g.deltaX = move.x;
      g.deltaY = move.y;
      g.dir = flick_direction(move);
      cb(Gesture::ACTIVE, g);
    }
  }
  else if (lifted && !buttonInTouch && time_ms - pressMs <= TAP_MAX_MS)
  {
    if (secondTap)
    {
      secondTap = false;
      endPendingTap(Gesture::CANCELLED, cb);
      cb(Gesture::ACTIVE, make_gesture(Gesture::DOUBLE_TAP, tapPos, tapPos));
    }
    else
    {
      tapPending = true;
      tapPos = pressPos;
      tapReleaseMs = time_ms;
      cb(Gesture::BEGIN, make_gesture(Gesture::TAP, tapPos, tapPos));
    }
  }
  dropSecondTap(cb);
}

void Recognizer::sendDrag(Gesture::State state, const Point2 &pos, GestureCb cb)
{
  Gesture g = make_gesture(Gesture::DRAG, pos, pressPos);
  g.deltaX = pos.x - lastPos.x;
  g.deltaY = pos.y - lastPos.y;
  lastPos = pos;
  cb(state, g);
}

void Recognizer::beginPair(const TouchPoint *const *down, GestureCb cb)
{
  if (tapPending)
    endPendingTap(Gesture::END, cb);
  mode = Mode::TWO;
  for (int i = 0; i < 2; i++)
  {
    pairId[i] = down[i]->contactId;
    pairPos[i] = pos_of(*down[i]);
  }
  const Point2 span = pairPos[1] - pairPos[0];
  pairStartDist = max(span.length(), 1.f);
  pairAngle = atan2f(span.y, span.x);
  pairRotation = 0;
  pairScale = 1;
  pairStartCenter = (pairPos[0] + pairPos[1]) * 0.5f;
  sendPair(Gesture::BEGIN, Point2(0, 0), cb);
}

void Recognizer::movePair(const Point2 &a, const Point2 &b, GestureCb cb)
{
  if (a == pairPos[0] && b == pairPos[1])
    return;
  const Point2 delta = (a + b - pairPos[0] - pairPos[1]) * 0.5f;
  pairPos[0] = a;
  pairPos[1] = b;
  const Point2 span = b - a;
  const float angle = atan2f(span.y, span.x);
  const float turn = angle - pairAngle;
  pairRotation += atan2f(sinf(turn), cosf(turn));
  pairAngle = angle;
  pairScale = span.length() / pairStartDist;
  sendPair(Gesture::ACTIVE, delta, cb);
}

void Recognizer::sendPair(Gesture::State state, const Point2 &delta, GestureCb cb)
{
  Gesture g = make_gesture(Gesture::PINCH, (pairPos[0] + pairPos[1]) * 0.5f, pairStartCenter);
  g.deltaX = delta.x;
  g.deltaY = delta.y;
  g.scale = pairScale;
  g.angle = pairRotation;
  cb(state, g);
  g.type = Gesture::ROTATE;
  cb(state, g);
}

void Recognizer::endHold(Gesture::State state, GestureCb cb)
{
  if (holding)
    cb(state, make_gesture(Gesture::HOLD, pressPos, pressPos));
  holding = false;
}

void Recognizer::endPendingTap(Gesture::State state, GestureCb cb)
{
  tapPending = false;
  cb(state, make_gesture(Gesture::TAP, tapPos, tapPos));
}

void Recognizer::dropSecondTap(GestureCb cb)
{
  if (!secondTap)
    return;
  secondTap = false;
  endPendingTap(Gesture::END, cb);
}


static bool is_continuous(Gesture::Type type) { return type == Gesture::DRAG || type == Gesture::PINCH || type == Gesture::ROTATE; }

void TouchpadGestureQueue::operator()(Gesture::State state, const Gesture &g)
{
  if (state == Gesture::ACTIVE && is_continuous(g.type))
    for (int i = int(events.size()) - 1; i >= 0 && events[i].state == Gesture::ACTIVE && is_continuous(events[i].g.type); i--)
      if (events[i].g.type == g.type)
      {
        Gesture &q = events[i].g;
        q.x = g.x;
        q.y = g.y;
        q.deltaX += g.deltaX;
        q.deltaY += g.deltaY;
        q.scale = g.scale;
        q.angle = g.angle;
        return;
      }
  if (events.full())
    events.erase(events.begin());
  events.push_back({g, state});
}

void TouchpadGestureQueue::drain(GestureCb cb)
{
  for (const Event &e : events)
    cb(e.state, e.g);
  events.clear();
}
