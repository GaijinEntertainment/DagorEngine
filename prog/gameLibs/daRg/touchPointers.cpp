#include "touchPointers.h"

namespace darg
{

void TouchPointers::updateState(InputEvent event, int touch_idx, const HumanInput::PointingRawState::Touch &touch)
{
  switch (event)
  {
    case INP_EV_PRESS:
    {
      auto it = eastl::find_if(activePointers.begin(), activePointers.end(),
        [touch_idx](const PointerInfo &pi) { return pi.id == touch_idx; });
      if (it == activePointers.end())
        activePointers.push_back(PointerInfo{touch_idx, Point2(touch.x, touch.y)});
      else
      {
        it->pos = Point2(touch.x, touch.y);
        logwarn_ctx("Touch #%d is already started", touch_idx);
      }
      break;
    }
    case INP_EV_RELEASE:
    {
      auto it = eastl::find_if(activePointers.begin(), activePointers.end(),
        [touch_idx](const PointerInfo &pi) { return pi.id == touch_idx; });
      if (it != activePointers.end()) // press event might be sent before scene is loaded
        activePointers.erase(it);
      break;
    }
    case INP_EV_POINTER_MOVE:
    {
      auto it = eastl::find_if(activePointers.begin(), activePointers.end(),
        [touch_idx](const PointerInfo &pi) { return pi.id == touch_idx; });
      if (it != activePointers.end()) // press event might be sent before scene is loaded
        it->pos.set(touch.x, touch.y);
      break;
    }
    default: break;
  }
}


bool TouchPointers::contains(int id) const
{
  const auto found = eastl::find_if(activePointers.begin(), activePointers.end(), [id](const PointerInfo &v) { return v.id == id; });
  return found != activePointers.end();
}


eastl::optional<TouchPointers::PointerInfo> TouchPointers::get(int id) const
{
  const auto found = eastl::find_if(activePointers.begin(), activePointers.end(), [id](const PointerInfo &pi) { return pi.id == id; });
  if (found != activePointers.end())
    return *found;
  return {};
}


void TouchPointers::reset() { activePointers.clear(); }

} // namespace darg
