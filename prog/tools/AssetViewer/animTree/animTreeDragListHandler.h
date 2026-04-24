// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/dragAndDropHandler.h>
#include <EASTL/unique_ptr.h>

constexpr char ASSET_DRAG_AND_DROP_TYPE[] = "ListBoxReorder";

namespace PropPanel
{
class ContainerPropertyControl;
}

class AnimTreePlugin;
class DataBlock;
struct AnimStatesData;
enum class AnimStatesType;

struct DragDropPayloadData
{
  int idx;
  const char *name;
};

class IListReorderHandler
{
public:
  virtual ~IListReorderHandler() = default;
  virtual void handleReorder(int from, int to) = 0;
};

class BaseAnimStatesReorderHandler : public IListReorderHandler
{
public:
  BaseAnimStatesReorderHandler(AnimTreePlugin &plugin, dag::ConstSpan<AnimStatesData> states,
    PropPanel::ContainerPropertyControl *panel) :
    plugin(plugin), states(states), panel(panel)
  {}
  void handleReorder(int from, int to) override;

protected:
  virtual AnimStatesType getTargetAnimStateType() const = 0;
  virtual void handleSpecificReorder(DataBlock &props, int from, int to) = 0;

  AnimTreePlugin &plugin;
  dag::ConstSpan<AnimStatesData> states;
  PropPanel::ContainerPropertyControl *panel;
};

class AnimTreeListDragHandler : public PropPanel::IListDragHandler
{
public:
  void onBeginDrag(int idx, const char *value) override;
  ImGuiDragDropFlags getDragDropFlags() override;
};

class AnimTreeListDropHandler : public PropPanel::IListDropHandler
{
public:
  virtual PropPanel::DragAndDropResult onDropTarget(int idx);
  PropPanel::ContainerPropertyControl *panel;
  int pid;
  eastl::unique_ptr<IListReorderHandler> reorderHandler;
};