#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiGlobals.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_mathBase.h>
#include <debug/dag_assert.h>
#include <osApiWrappers/dag_atomic.h>
#include "touchpadTouchSender.h"

namespace hid
{
using namespace HumanInput;

bool gpcm_TouchBegan(PointingRawState &state, IGenPointing *drv, IGenPointingClient *cli, float mx, float my, intptr_t touch_id,
  PointingRawState::Touch::Source src, uint32_t pad_button_state = 0);
void gpcm_TouchMoved(PointingRawState &state, IGenPointing *drv, IGenPointingClient *cli, float mx, float my, intptr_t touch_id);
void gpcm_TouchEnded(PointingRawState &state, IGenPointing *drv, IGenPointingClient *cli, float mx, float my, intptr_t touch_id,
  bool no_move = false);
} // namespace hid

static intptr_t touch_id_tbl[HumanInput::MAX_TOUCHES] = {0};
static inline int allocTouch(intptr_t touch_id)
{
  if (!touch_id)
    return -1;
  for (intptr_t &t : touch_id_tbl)
    if (t == touch_id)
      return &t - touch_id_tbl;
    else if (!t)
    {
      t = touch_id;
      return &t - touch_id_tbl;
    }
  return -1;
}
static inline int resolveTouch(intptr_t touch_id)
{
  if (!touch_id)
    return -1;
  for (intptr_t &t : touch_id_tbl)
    if (t == touch_id)
      return &t - touch_id_tbl;
  return -1;
}
static inline void releaseTouchIdx(int t_idx) { touch_id_tbl[t_idx] = 0; }

static inline void updateTouchPos(HumanInput::PointingRawState::Touch &touch, float mx, float my)
{
  touch.t1msec = get_time_msec();
  touch.deltaX = mx - touch.x;
  touch.deltaY = my - touch.y;
  touch.x = mx;
  touch.y = my;
  if (touch.touchSrc != HumanInput::PointingRawState::Touch::TS_touchPad)
  {
    float dist2 = sqr(mx - touch.x0) + sqr(my - touch.y0);
    if (sqr(touch.maxDist) < dist2)
      touch.maxDist = sqrtf(dist2);
  }
}

bool hid::gpcm_TouchBegan(HumanInput::PointingRawState &state, HumanInput::IGenPointing *drv, HumanInput::IGenPointingClient *cli,
  float mx, float my, intptr_t touch_id, HumanInput::PointingRawState::Touch::Source src, uint32_t pad_button_state)
{
  static int next_touch_idx = 0;

  int t = allocTouch(touch_id);
  if (t < 0)
    return false;

  bool first_touch = !state.touchesActive;
  state.touchesActive |= 1 << t;
  state.touch[t].touchSrc = src;
  state.touch[t].padButtonState = pad_button_state;
  state.touch[t].touchOrdId = ++next_touch_idx;
  state.touch[t].t0msec = state.touch[t].t1msec = get_time_msec();
  state.touch[t].x = state.touch[t].x0 = mx;
  state.touch[t].y = state.touch[t].y0 = my;
  state.touch[t].deltaX = state.touch[t].deltaY = state.touch[t].maxDist = 0;

  raw_state_pnt.touch[t] = state.touch[t];
  raw_state_pnt.touchesActive = state.touchesActive;
  if (cli)
    cli->gmcTouchBegan(drv, t, state.touch[t]);
  return first_touch;
}

void hid::gpcm_TouchMoved(HumanInput::PointingRawState &state, HumanInput::IGenPointing *drv, HumanInput::IGenPointingClient *cli,
  float mx, float my, intptr_t touch_id)
{
  int t = resolveTouch(touch_id);
  if (t < 0)
    return;

  updateTouchPos(state.touch[t], mx, my);

  float gdx = raw_state_pnt.touch[t].deltaX, gdy = raw_state_pnt.touch[t].deltaY;
  raw_state_pnt.touch[t] = state.touch[t];
  raw_state_pnt.touch[t].deltaX += gdx;
  raw_state_pnt.touch[t].deltaY += gdy;
  if (cli && (state.touch[t].deltaX || state.touch[t].deltaY))
    cli->gmcTouchMoved(drv, t, state.touch[t]);
}

void hid::gpcm_TouchEnded(HumanInput::PointingRawState &state, HumanInput::IGenPointing *drv, HumanInput::IGenPointingClient *cli,
  float mx, float my, intptr_t touch_id, bool no_move)
{
  int t = resolveTouch(touch_id);
  if (t < 0)
    return;

  state.touchesActive &= ~(1 << t);
  state.touch[t].t1msec = get_time_msec();
  if (!no_move)
    updateTouchPos(state.touch[t], mx, my);

  float gdx = raw_state_pnt.touch[t].deltaX, gdy = raw_state_pnt.touch[t].deltaY;
  raw_state_pnt.touch[t] = state.touch[t];
  raw_state_pnt.touch[t].deltaX += gdx;
  raw_state_pnt.touch[t].deltaY += gdy;
  raw_state_pnt.touchesActive = state.touchesActive;
  if (cli)
    cli->gmcTouchEnded(drv, t, state.touch[t]);

  releaseTouchIdx(t);
}

static const HumanInput::JoystickTouchpadFrame::Contact *find_contact(const HumanInput::JoystickTouchpadFrame &f, uint8_t id)
{
  for (int i = 0; i < f.count; i++)
    if (f.contacts[i].id == id)
      return &f.contacts[i];
  return nullptr;
}

static Point2 contact_pos(const HumanInput::JoystickTouchpadFrame::Contact &c) { return Point2(c.x, c.y); }

// Windows 11 win32kbase "Enhance pointer precision"
static float windows_pointer_gain(float counts_per_report, float y_scale, int &prev_segment)
{
  static constexpr float X[5] = {0, 0x6e15 * 3.5f / 65536, 0x14000 * 3.5f / 65536, 0x3dc29 * 3.5f / 65536, 0x280000 * 3.5f / 65536};
  static constexpr float Y[5] = {0, 0x111fd / 65536.f, 0x42400 / 65536.f, 0x12fc00 / 65536.f, 0x1bbc000 / 65536.f};
  auto segmentGain = [&](int s) {
    const float slope = (Y[s + 1] - Y[s]) * y_scale / (X[s + 1] - X[s]);
    return slope + (Y[s] * y_scale - slope * X[s]) / counts_per_report;
  };
  int s = 0;
  while (s < 3 && counts_per_report > X[s + 1])
    s++;
  float gain = segmentGain(s);
  if (prev_segment < s)
    gain = (gain + segmentGain(prev_segment)) * 0.5f;
  prev_segment = s;
  return gain;
}

// Windows speed 1..20, curve off
static float windows_linear_gain(int speed) { return speed <= 2 ? speed / 32.f : speed <= 10 ? (speed - 2) / 8.f : (speed - 6) / 4.f; }

void hid::TouchpadTouchSender::send(HumanInput::PointingRawState &state, HumanInput::IGenPointing *drv,
  HumanInput::IGenPointingClient *cli, dag::ConstSpan<HumanInput::JoystickTouchpadFrame> frames, const Point2 &anchor_pos,
  float screen_height)
{
  using Contact = HumanInput::JoystickTouchpadFrame::Contact;
  G_ASSERT(&state != &HumanInput::raw_state_pnt); // gpcm_Touch* add the deltas of the state to raw_state_pnt
  static const HumanInput::JoystickTouchpadFrame NO_PAD;
  if (frames.empty())
    frames = make_span_const(&NO_PAD, 1);
  // one move per batch, else daRg works per report
  const HumanInput::JoystickTouchpadFrame *moved = nullptr;
  auto sendMoves = [&] {
    for (int i = 0; moved && i < moved->count; i++)
    {
      const Contact &c = moved->contacts[i];
      const float scale = moved->padHeight ? screen_height / moved->padHeight : 1.f;
      const intptr_t id = idBase + c.id;
      const Point2 pos = anchor + (contact_pos(c) - origin) * scale;
      hid::gpcm_TouchMoved(state, drv, cli, pos.x, pos.y, id);
    }
    moved = nullptr;
  };

  for (const HumanInput::JoystickTouchpadFrame &frame : frames)
  {
    const float scale = frame.padHeight ? screen_height / frame.padHeight : 1.f;
    bool anyDown = false;
    for (const intptr_t &id : touch_id_tbl)
      if (id >= idBase && id < idBase + ID_RANGE)
      {
        state.touch[&id - touch_id_tbl].padButtonState = frame.buttonState;
        if (find_contact(frame, uint8_t(id - idBase)))
          anyDown = true;
        else
        {
          sendMoves();
          hid::gpcm_TouchEnded(state, drv, cli, 0, 0, id, /*no_move*/ true);
        }
      }

    if (frame.count && !anyDown)
    {
      origin = contact_pos(frame.contacts[0]);
      anchor = anchor_pos;
      segment = 0;
    }
    // Windows gives two-finger scroll no gain
    const Contact *was = anyDown && frame.count == 1 && prev.count == 1 ? find_contact(prev, frame.contacts[0].id) : nullptr;
    const Point2 d = was ? contact_pos(frame.contacts[0]) - contact_pos(*was) : Point2(0, 0);
    const HumanInput::TouchpadBallistics &b = HumanInput::stg_pnt.touchPadBallistics;
    if (d.x || d.y)
    {
      const int speed = clamp(interlocked_relaxed_load(b.speed), 1, 20);
      float gain = scale * windows_linear_gain(speed);
      if (interlocked_relaxed_load(b.enabled))
      {
        constexpr float REPORT_SEC = 1.f / 125; // precision touchpad report rate
        const float countsPerPixel = max(b.countsPerMm, 1.f) / HumanInput::JoystickTouchpadFrame::PAD_PIXELS_PER_MM;
        const float dt = float(int32_t(frame.timestampUs - prev.timestampUs)) * 1e-6f; // a restarted clock goes back
        const float ax = fabsf(d.x), ay = fabsf(d.y);
        const float countsPerReport = (max(ax, ay) + min(ax, ay) * 0.5f) * countsPerPixel * (dt > 0 ? REPORT_SEC / dt : 1.f);
        const float dpi = 96.f * screen_height / 1080; // no Windows clamp: aim stays resolution-free
        gain = countsPerPixel * windows_pointer_gain(countsPerReport, dpi / 120 * speed / 10, segment);
      }
      anchor += d * (gain - scale);
    }

    for (int i = 0; i < frame.count; i++)
      if (int t = resolveTouch(idBase + frame.contacts[i].id); t >= 0)
        state.touch[t].maxDist = max(state.touch[t].maxDist, length(contact_pos(frame.contacts[i]) - contactStart[t]) * scale);
    moved = &frame;
    for (int i = 0; i < frame.count; i++)
    {
      const Contact &c = frame.contacts[i];
      const intptr_t id = idBase + c.id;
      if (resolveTouch(id) >= 0)
        continue;
      sendMoves();
      const Point2 pos = anchor + (contact_pos(c) - origin) * scale;
      hid::gpcm_TouchBegan(state, drv, cli, pos.x, pos.y, id, HumanInput::PointingRawState::Touch::TS_touchPad, frame.buttonState);
      if (int t = resolveTouch(id); t >= 0)
        contactStart[t] = contact_pos(c);
    }
    prev = frame;
  }
  sendMoves();
}
