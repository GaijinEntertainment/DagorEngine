// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_inputIds.h>
#include <math/dag_Point2.h>
#include <dag/dag_vector.h>
#include <EASTL/optional.h>


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

  void updateState(InputEvent event, int touch_idx, Point2 pos);
  bool contains(int id) const;
  eastl::optional<PointerInfo> get(int id) const;

  void reset();

public:
  dag::Vector<PointerInfo> activePointers;
};

} // namespace darg
