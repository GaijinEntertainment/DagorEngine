//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/control/container.h>
#include <EASTL/unique_ptr.h>

namespace PropPanel
{

class IMenu;
class PanelWindowContextMenuEventHandler;

class PanelWindowPropertyControl : public ContainerPropertyControl
{
public:
  PanelWindowPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[]);
  ~PanelWindowPropertyControl() override;

  unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const override { return 0; }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  // saving and loading state with datablock
  int saveState(DataBlock &datablk, bool by_name = false) override;
  int loadState(DataBlock &datablk, bool by_name = false) override;

  virtual int getScrollPos();
  virtual void setScrollPos(int pos);

  // This must be called before ImGui::Begin to be able to set the scroll position.
  virtual void beforeImguiBegin();

  void updateImgui() override;

  // There is getCaption but that returns with a new SimpleString, and there is getCaptionValue that copies to a buffer,
  // so here is a third function for the PanelWindowPropertyControl to use.
  const String &getStringCaption() const { return controlCaption; }

private:
  void onWcRightClick(WindowBase *source) override;

  String controlCaption;
  int scrollingRequestedPositionY = -1;
  int scrollingRequestedForFrames = 0;
  eastl::unique_ptr<IMenu> contextMenu;
  eastl::unique_ptr<PanelWindowContextMenuEventHandler> contextMenuEventHandler;

  static PanelWindowPropertyControl *middleMouseDragWindow;
  static ImVec2 middleMouseDragStartPos;
  static float middleMouseDragStartScrollY;
};

} // namespace PropPanel