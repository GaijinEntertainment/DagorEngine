#pragma once

#include <sepGui/wndPublic.h>
#include "constants.h"
#include "wndLayoutSaver.h"
#include "windows/wndMainWindow.h"
#include <generic/dag_tab.h>
#include <debug/dag_debug.h>
#include <math/integer/dag_IPoint2.h>
#include "wndAccel.h"

using hdpi::_px;
using hdpi::_pxActual;
using hdpi::_pxS;
using hdpi::_pxScaled;

class CascadeWindow;
class VirtualWindow;
class SplitterWindow;
class ClientWindow;
class MovableWindow;
class DragPlacesShower;
class IWndManagerEventHandler;

class IBBox2;

//=============================================================================
class WinManager : public IWndManager
{
public:
  WinManager(IWndManagerEventHandler *event_handler);
  ~WinManager();

  IWndEmbeddedWindow *onWmCreateWindow(void *handle, int type);
  void onWmDestroyWindow(void *handle);

  void onMainWindowCreated();
  void onVirtualWindowDeleted(VirtualWindow *window);
  void onClose();

  VirtualWindow *addSplitter(CascadeWindow *splitted_wnd, IPoint2 mouse_pos, bool is_vertical, bool original_to_left = true);

  ClientWindow *setMovable(ClientWindow *movable_wnd);
  void setUnMovable(ClientWindow *movable_wnd, IPoint2 *mouse_pos);
  void deleteMovableWindows();
  dag::ConstSpan<MovableWindow *> getMovableWindows() const;

  void setRoot(CascadeWindow *root_window);
  CascadeWindow *getRoot() const;
  IWndManagerEventHandler *getEventHandler() const;

  void UpdatePlacesShower(const IPoint2 &mouse_pos, void *moving_handle);
  void StopPlacesShower();
  CascadeWindow *getWindowAlign(const IPoint2 &mouse_pos, WindowAlign &align);

  // creators
  SplitterWindow *createSplitterWindow(VirtualWindow *parent, int x, int y, int w, int h);
  ClientWindow *createClientWindow(VirtualWindow *parent, int x, int y, int w, int h);

  // layout lowlevel
  VirtualWindow *insertCustomToLayout(CascadeWindow *where, ClientWindow *&new_window, float size, WindowAlign align);

  ClientWindow *makeMovableWindow(ClientWindow *window, int x, int y, int w, int h, const char *caption);
  VirtualWindow *dropMovableToLayout(CascadeWindow *where, ClientWindow *movable, float size, WindowAlign align);

  void deleteWindow(CascadeWindow *window);

  VirtualWindow *insertToLayout(CascadeWindow *where, ClientWindow *&new_window, float size, WindowAlign align);

  // IWndManager
  /////////////////////////////////////////////////////////////////////////////
  int run(int width, int height, const char *caption, const char *icon = "", WindowSizeInit size = WSI_NORMAL);
  void close();

  virtual bool loadLayout(const char *filename = NULL);
  virtual void saveLayout(const char *filename = NULL);

  virtual bool loadLayoutFromDataBlock(const DataBlock &data_block);
  virtual void saveLayoutToDataBlock(DataBlock &data_block);

  virtual void setMainWindowCaption(const char *caption);

  virtual void registerWindowHandler(IWndManagerWindowHandler *handler);
  virtual void unregisterWindowHandler(IWndManagerWindowHandler *handler);

  virtual void reset();
  virtual void show(WindowSizeInit size = WSI_NORMAL);

  virtual void clientToScreen(int &x, int &y);

  virtual void *getMainWindow() const;
  virtual void *getFirstWindow() const;

  virtual void *splitWindowF(void *old_handle, void *new_handle, float new_wsize = 0.5f, WindowAlign new_walign = WA_RIGHT);
  virtual void *splitNeighbourWindowF(void *old_handle, void *new_handle, float new_wsize = 0.5, WindowAlign new_walign = WA_RIGHT);

  virtual bool resizeWindowF(void *handle, float new_wsize);
  virtual bool fixWindow(void *handle, bool fix = true);
  virtual bool removeWindow(void *handle);

  virtual bool getMovableFlag(void *handle);

  virtual void setWindowType(void *handle, int type);
  virtual int getWindowType(void *handle);

  virtual void getWindowClientSize(void *handle, unsigned &width, unsigned &height);
  virtual bool getWindowPosSize(void *handle, int &x, int &y, unsigned &width, unsigned &height);
  virtual int getSplitterControlSize() const override;

  virtual void setHeader(void *handle, HeaderPos header_pos);
  virtual void setCaption(void *handle, const char *caption);

  virtual void setMinSize(void *handle, hdpi::Px width, hdpi::Px height);
  virtual void setMenuArea(void *handle, hdpi::Px width, hdpi::Px height);

  virtual IMenu *getMainMenu();
  virtual IMenu *getMenu(void *handle);

  virtual bool init3d(const char *drv_name, const DataBlock *blkTexStreaming);

  // accelerators
  virtual void addAccelerator(unsigned cmd_id, unsigned v_key, bool ctrl, bool alt, bool shift);
  virtual void addAcceleratorUp(unsigned cmd_id, unsigned v_key, bool ctrl, bool alt, bool shift);
  virtual void clearAccelerators();
  /////////////////////////////////////////////////////////////////////////////

private:
  void registerWinAPIWindowClasses();

  //
  void deleteMovableWindow(MovableWindow *window);
  bool deleteFromLayout(CascadeWindow *window);

  bool calculateWinRects(CascadeWindow *where, float size_complex, WindowAlign align, IBBox2 &w1, IBBox2 &w2, IBBox2 &ws,
    float &ratio);

  bool replaceCascadeWindow(VirtualWindow *parent, CascadeWindow *old_window, CascadeWindow *new_window);

  CascadeWindow *getNeighborWindow(VirtualWindow *parent, CascadeWindow *window);

  void saveOriginalWindowPlace(ClientWindow &original, MovableWindow &movable);

  WindowAlign getWindowAlign(CascadeWindow *window);

  // find by handle
  ClientWindow *findWindowByHandle(void *handle);
  ClientWindow *findLayoutWindowByHandle(void *handle);
  ClientWindow *findMovableWindowByHandle(void *handle);

  ClientWindow *treeWindowSearch(CascadeWindow *node, void *handle);

  // data
  Tab<MovableWindow *> mMovableWindows;

  MainWindow mMainWindow;
  LayoutSaver mLayoutSaver;
  DragPlacesShower *mDragPlacesShower;

  CascadeWindow *mRootWindow;

  IWndManagerEventHandler *mEventHandler;
  Tab<IWndManagerWindowHandler *> mWindowHandlers;

  unsigned mSizingEdge;

  WinAccel mAccels;
};
