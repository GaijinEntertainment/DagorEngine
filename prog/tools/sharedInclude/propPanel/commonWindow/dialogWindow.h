//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/c_control_event_handler.h>
#include <libTools/util/hdpiUtil.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>
#include <util/dag_string.h>

namespace PropPanel
{

class ContainerPropertyControl;

enum
{
  DIALOG_ID_NONE = 0,
  DIALOG_ID_OK,
  DIALOG_ID_CANCEL,
  DIALOG_ID_CLOSE,
  DIALOG_ID_YES = 6,
  DIALOG_ID_NO = 7,
};

class DialogWindow : public ControlEventHandler
{
public:
  DialogWindow(void *phandle, hdpi::Px w, hdpi::Px h, const char caption[], bool hide_panel = false);
  DialogWindow(void *phandle, int x, int y, hdpi::Px w, hdpi::Px h, const char caption[], bool hide_panel = false);
  ~DialogWindow();

  virtual ContainerPropertyControl *getPanel();

  // Modeless
  virtual void show();

  // Modal
  virtual int showDialog();

  virtual void hide(int result = DIALOG_ID_NONE);
  virtual void showButtonPanel(bool show = true) { buttonsVisible = show; }
  bool isVisible() const { return visible; }
  bool isModal() const { return modal; }

  const String &getCaption() const { return dialogCaption; }
  void setCaption(const char *caption) { dialogCaption = caption; }

  virtual void clickDialogButton(int id);
  virtual SimpleString getDialogButtonText(int id) const;
  virtual void setDialogButtonText(int id, const char *text);
  virtual bool removeDialogButton(int id);

  virtual IPoint2 getWindowPosition() const;
  virtual void setWindowPosition(const IPoint2 &position, const Point2 &pivot = Point2::ZERO);

  virtual IPoint2 getWindowSize() const;
  virtual void setWindowSize(const IPoint2 &size);
  void setWindowSize(hdpi::Px w, hdpi::Px h) { setWindowSize(IPoint2(hdpi::_px(w), hdpi::_px(h))); }

  virtual void centerWindow();

  // Set the center of the dialog to the mouse position.
  // This is rather specific function but moveWindow was not used anywhere else.
  // Also this prevents issues coming caused by the following in ImGui:
  // "When multi-viewports are enabled, all Dear ImGui coordinates become absolute coordinates"
  virtual void centerWindowToMousePos();

  virtual void positionLeftToWindow(const char *window_name, bool use_same_height = false);
  virtual void dockTo(unsigned dock_node_id);

  virtual void autoSize(bool auto_center = true);

  virtual int getScrollPos() const;
  virtual void setScrollPos(int pos);

  // events
  virtual bool onOk() { return true; }
  virtual bool onCancel() { return true; }
  virtual bool onClose() { return onCancel(); }

  virtual int closeReturn() { return DIALOG_ID_CANCEL; }

  void setCloseHandler(ControlEventHandler *close_event_handler) { closeEventHandler = close_event_handler; }

  // When showing the dialog the focus will be set to the speficified control.
  // id: must be DIALOG_ID_NONE, DIALOG_ID_OK or DIALOG_ID_CANCEL.
  //     When DIALOG_ID_NONE is specified then no focus will be set.
  void setInitialFocus(int id) { initialFocusId = id; }

  // Prevent the procession of the Tab key, so changing the focused control and the selected combo box item will not work with the
  // Tab key.
  // This is rather specific function that is used by the "Heightmap brush" dialog in daEditorX's Landscape edit mode. It is shown
  // while the Tab key is active.
  void setPreventNavigationWithTheTabKey() { preventNavigationWithTheTabKey = true; }

  // Modal dialogs are automatically sized by default. Manual sizing can be enabled with this function.
  void setManualModalSizingEnabled() { manualModalSizingEnabled = true; }
  bool isManualModalSizingEnabled() const { return manualModalSizingEnabled; }

  void setModalBackgroundDimmingEnabled(bool enabled) { modalBackgroundDimmingEnabled = enabled; }
  bool isModalBackgroundDimmingEnabled() const { return modalBackgroundDimmingEnabled; }

  virtual void beforeUpdateImguiDialog(bool &use_auto_size_for_the_current_frame);
  virtual void updateImguiDialog();

protected:
  void create(unsigned w, unsigned h, bool hide_panel);

  // The height taken by the button panel.
  float getButtonPanelHeight() const { return buttonPanelHeight; }

  ControlEventHandler *buttonEventHandler = nullptr;
  ControlEventHandler *closeEventHandler = nullptr;
  ContainerPropertyControl *propertiesPanel = nullptr;
  ContainerPropertyControl *buttonsPanel = nullptr;
  String dialogCaption;
  int dialogResult;
  int initialWidth = 0;
  int initialHeight = 0;
  int initialFocusId = DIALOG_ID_OK;
  bool modal = false;
  bool visible = false;
  bool buttonsVisible = true;
  bool manualModalSizingEnabled = false;
  bool preventNavigationWithTheTabKey = false;
  bool modalBackgroundDimmingEnabled = false; // Off by default, artists wanted to see the viewport without dimming.

  int autoSizingRequestedForFrames = 0;
  bool moveRequested = false;
  Point2 moveRequestPosition = Point2::ZERO;
  Point2 moveRequestPivot = Point2::ZERO;

  bool sizingRequested = false;
  Point2 sizingRequestSize = Point2::ZERO;

  bool dockingRequested = false;
  unsigned dockingRequestNodeId = 0;

  int scrollingRequestedPositionY = -1;
  int scrollingRequestedForFrames = 0;

  float buttonPanelHeight = 0.0f;
};

} // namespace PropPanel