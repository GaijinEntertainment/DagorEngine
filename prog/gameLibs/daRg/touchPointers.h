#pragma once

#include <humanInput/dag_hiPointing.h>
#include <daRg/dag_inputIds.h>
#include <math/dag_Point2.h>
#include <dag/dag_vector.h>


namespace darg
{

class TouchPointers
{
public:
  struct PointerInfo
  {
    int id;
    Point2 pos;
  };

  void updateState(InputEvent event, int touch_idx, const HumanInput::PointingRawState::Touch &touch);
  bool contains(int id) const;
  void reset();

public:
  dag::Vector<PointerInfo> activePointers;
};

} // namespace darg
