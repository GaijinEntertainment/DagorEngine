#include <humanInput/dag_hiPointing.h>
#include <humanInput/dag_hiGlobals.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_mathBase.h>

namespace hid
{
using namespace HumanInput;

bool gpcm_TouchBegan(PointingRawState &state, IGenPointing *drv, IGenPointingClient *cli, float mx, float my, intptr_t touch_id,
  int src);
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
  float dist2 = sqr(mx - touch.x0) + sqr(my - touch.y0);
  if (sqr(touch.maxDist) < dist2)
    touch.maxDist = sqrtf(dist2);
}

bool hid::gpcm_TouchBegan(HumanInput::PointingRawState &state, HumanInput::IGenPointing *drv, HumanInput::IGenPointingClient *cli,
  float mx, float my, intptr_t touch_id, int src)
{
  static int next_touch_idx = 0;

  int t = allocTouch(touch_id);
  if (t < 0)
    return false;

  bool first_touch = !state.touchesActive;
  state.touchesActive |= 1 << t;
  state.touch[t].touchSrc = src;
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
