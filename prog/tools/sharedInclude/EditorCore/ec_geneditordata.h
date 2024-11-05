// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_status_bar.h>
#include <sepGui/wndCommon.h>

#include <generic/dag_tab.h>

class IWndManager;


/// A class for storing general Editor parameters.
/// @ingroup EditorCore
/// @ingroup EditorBaseClasses Editor base classes
class GeneralEditorData
{
public:
  /// Constructor.
  /// @param[in] main_wnd - pointer to main Editor's window. Used as parent
  ///   window in viewport creating functions.
  GeneralEditorData();

  /// Destructor.
  ~GeneralEditorData();

  /// Load data from blk file.
  /// @param[in] blk - Data Block that contains data to load (see #DataBlock)
  void load(const DataBlock &blk);
  /// Save data to blk file.
  /// @param[in] blk - Data Block that contains data to save (see #DataBlock)
  void save(DataBlock &blk) const;

  //*****************************************************************
  /// @name Viewport functions
  //@{
  /// Use this instead of createViewport if you want to add a subclassed ViewportWindow.
  void addViewport(void *parent, IGenEventHandler *eh, IWndManager *manager, ViewportWindow *v);
  /// Creates new viewport
  ViewportWindow *createViewport(void *parent, int x, int y, int w, int h, IGenEventHandler *eh, IWndManager *manager);
  /// Deletes viewport with specified index
  void deleteViewport(int n);

  /// Gets viewport count
  inline int getViewportCount() const { return vpw.size(); }

  /// Set event handler for all viewports.
  /// @param[in] eh - pointer to event handler
  void setEH(IGenEventHandler *eh);

  /// Get active viewport.
  /// @return - pointer to active viewport,
  ///\n- @b NULL if all viewports are not active
  ViewportWindow *getActiveViewport() const;

  /// Activate current viewport
  inline void activateCurrentViewport() const
  {
    IGenViewportWnd *curVp = getCurrentViewport();
    if (curVp)
      curVp->activate();
  }

  /// Get last active viewport.
  /// @return - pointer to the last active viewport
  /// \n- @b NULL if no viewports were active yet
  inline ViewportWindow *getCurrentViewport() const { return (currentViewportId == -1) ? NULL : vpw[currentViewportId]; }

  /// Get ID of last active viewport.
  /// @return - index of last active viewport
  /// \n- @b -1 if no viewports were active yet
  inline int getCurrentViewportId() const { return currentViewportId; }

  inline void setCurrentViewportId(int id) { currentViewportId = id; }

  /// Get rendering viewport.
  /// @return - pointer to viewport that is rendering now
  /// \n- @b NULL if no rendering viewports
  ViewportWindow *getRenderViewport() const;

  /// Get viewport by index.
  /// @param[in] n - viewport index
  /// @return pointer to viewport
  inline ViewportWindow *getViewport(int n) const { return (n >= 0 && n < vpw.size()) ? vpw[n] : NULL; }

  /// Find viewport index.
  /// @param[in] w - pointer to viewport
  /// @return - viewport index
  ///\n - <b>#VIEWPORT_NUM - 1</b> if viewport not found
  int findViewportIndex(IGenViewportWnd *w) const;

  /// Returns pointer to viewport containing specified point.
  /// @param[in] x,y - point coordinates
  /// @param[out] x,y - viewport point coordinates
  /// @return pointer to viewport containing specified point
  IGenViewportWnd *screenToViewport(int &x, int &y) const;

  /// Set viewports cache mode.
  /// @param[in] mode - viewport cache mode (see
  /// IEditorCoreEngine::ViewportCacheMode)
  void setViewportCacheMode(IEditorCoreEngine::ViewportCacheMode mode);
  /// Update viewports cache.
  void invalidateCache();
  /// Set camera clipping for all viewports.
  /// @param[in] zn - z-near, a distance to nearest visible parts of scene
  ///                 (all parts more close to camera will be invisible)
  /// @param[in] zf - z-far, a distance to the farthest visible parts of scene
  ///                     (all parts more distant from camera will be invisible)
  void setZnearZfar(real zn, real zf);
  //@}

  /// Called by EditorCore at the act stage.
  /// This function calls act() function of viewports and toolbar manager,
  /// stores last active viewport.
  void act(real dt);

  /// The function calls #ViewportWindow::redrawClientRect() for all viewports.
  void redrawClientRect();

  /// Default event handler.
  IGenEventHandler *curEH;

  /// Toolbar manager.
  ToolBarManager *tbManager;

  inline int getViewportLayout() { return viewportLayout; }
  inline void setViewportLayout(int value) { viewportLayout = value; }

private:
  /// Array of pointers to viewports.
  Tab<ViewportWindow *> vpw;

  /// Viewports layout.
  /// Posible values is defined in #EditorCoreCM:\n
  /// @verbatim
  /// CM_VIEWPORT_LAYOUT_1     - one viewport
  /// CM_VIEWPORT_LAYOUT_2VERT - two vertical viewports
  /// CM_VIEWPORT_LAYOUT_2HOR  - two horizontal viewports
  /// CM_VIEWPORT_LAYOUT_4     - four viewports  @endverbatim
  int viewportLayout;

  /// Viewport cache mode.
  IEditorCoreEngine::ViewportCacheMode vcmode;

  /// Last active viewport number.
  int currentViewportId;
};
