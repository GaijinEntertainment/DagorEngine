#pragma once

#include <EASTL/vector.h>

#include <gui/dag_stdGuiRender.h>
#include <daRg/dag_renderState.h>


namespace darg
{

class Element;


enum RenderCommand
{
  RCMD_ELEM_RENDER,
  RCMD_ELEM_POSTRENDER,
  RCMD_SET_VIEWPORT,
  RCMD_RESTORE_VIEWPORT
};


struct RenderEntry
{
  RenderCommand cmd;
  Element *elem;
  int zOrder;
  int hierOrder;
};


struct RenderEntryCompare
{
  bool operator()(const RenderEntry &lhs, const RenderEntry &rhs) const
  {
    if (lhs.zOrder != rhs.zOrder)
      return lhs.zOrder < rhs.zOrder;
    return lhs.hierOrder < rhs.hierOrder;
  }
};


struct TransformStackItem
{
  GuiVertexTransform gvtm;
  const Element *elem;
};


struct OpacityStackItem
{
  float prevValue;
  const Element *elem;
};


class RenderList
{
public:
  void push(const RenderEntry &e);
  void clear();
  void render(StdGuiRender::GuiContext &ctx);
  void afterRebuild();
  void recalcBoxes(const BBox2 &container_viewport);

private:
  void debugBlurPanelsOverlap(StdGuiRender::GuiContext &ctx);

public:
  typedef dag::Vector<RenderEntry> RListData;
  RListData list;

  dag::Vector<TransformStackItem> transformStack;
  dag::Vector<OpacityStackItem> opacityStack;

  RenderState renderState;

  dag::Vector<Element *> worldBlurElems;
};


} // namespace darg
