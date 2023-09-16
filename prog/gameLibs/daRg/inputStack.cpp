#include "inputStack.h"

namespace darg
{

void InputStack::push(const InputEntry &e)
{
  stack.insert(e);
  if (!e.inputPassive)
  {
    summaryBhvFlags |= e.elem->behaviorsSummaryFlags;

    if (!isDirPadNavigable)
      isDirPadNavigable = e.elem->isDirPadNavigable();
  }
}


void InputStack::clear()
{
  stack.clear();
  summaryBhvFlags = 0;
  isDirPadNavigable = false;
}


Element *InputStack::hitTest(const Point2 &pos, int bhv_flags, int elem_flags) const
{
  for (const InputEntry &ie : stack)
  {
    if (ie.elem->hitTest(pos) && (ie.elem->hasBehaviors(bhv_flags) || ie.elem->hasFlags(elem_flags)))
      return ie.elem;
  }

  return nullptr;
}


} // namespace darg
