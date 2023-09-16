#pragma once

#include <EASTL/vector_set.h>
#include <daRg/dag_element.h>


namespace darg
{

class Element;

struct InputEntry
{
  Element *elem = nullptr;
  int zOrder = 0;
  int hierOrder = 0;
  bool inputPassive = false;
};


struct InputEntryCompare
{
  bool operator()(const InputEntry &lhs, const InputEntry &rhs) const
  {
    if (lhs.zOrder != rhs.zOrder)
      return lhs.zOrder > rhs.zOrder;
    return lhs.hierOrder > rhs.hierOrder;
  }
};


class InputStack
{
public:
  void push(const InputEntry &e);
  void clear();
  Element *hitTest(const Point2 &pos, int bhv_flags, int elem_flags) const;

public:
  typedef eastl::vector_set<InputEntry, InputEntryCompare> Stack;
  Stack stack;
  int summaryBhvFlags = 0;
  bool isDirPadNavigable = false;
};


} // namespace darg
