#pragma once

#include "constants.h"
#include <generic/dag_tab.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>


class CascadeWindow;
class ClientWindow;
class WinManager;
class MovableWindow;
class DataBlock;


//=============================================================================
struct NodeInfo
{
  NodeInfo(int own_ID, const IPoint2 &left_top, const IPoint2 &right_bottom, unsigned wnd_type, WindowSizeLock fixed_type,
    CascadeWindow *window, bool is_movable = false) :
    mOwnID(own_ID),
    mPosition(left_top.x, left_top.y, right_bottom.x, right_bottom.y),
    mWndType(wnd_type),
    mFixedType(fixed_type),
    mIsMovable(is_movable),
    mWindow(window)
  {
    mIsVerticalSplitter = true;
  }

  int mOwnID;
  Point4 mPosition;
  int mFirstChildID;
  int mSecondChildID;
  bool mIsVerticalSplitter;
  int mWndType;
  WindowSizeLock mFixedType;
  bool mIsMovable;
  CascadeWindow *mWindow;
};

//=============================================================================
class LayoutSaver
{
public:
  LayoutSaver(WinManager *cur_owner);
  ~LayoutSaver();

  void saveLayout(const char *filename);
  void loadLayout(const char *filename);

  void saveLayoutToDataBlock(DataBlock &save_data_block);
  void loadLayoutFromDataBlock(const DataBlock &load_data_block);

private:
  void saveToDataBlock(DataBlock &data_block);
  bool loadFromDataBlock(const DataBlock &data_block);

  bool fillNodeInfoArrays();
  void fillLayout();

  void saveToBlk(const char *filename);
  bool loadFromBlk(const char *filename);

  void loadMovableWindowsWithoutOriginal();

  bool fillNodeInfoArray(CascadeWindow *node, int &child_ID);
  bool fillMovableInfoArray();
  void emptyNodeInfoArrays();

  NodeInfo *findNodeByID(int ID);
  void addWindow(ClientWindow *window, NodeInfo *node);

  int mNodeCounter;
  Tab<NodeInfo *> mNodeInfoArray;

  WinManager *mOwner;

  IPoint2 mMainWinPos;
};
