// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "wndLayoutSaver.h"
#include "wndManager.h"
#include "windows/wndVirtualWindow.h"
#include "windows/wndClientWindow.h"
#include "windows/wndMovableWindow.h"

#include <windows.h>
#include <stdio.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <ioSys/dag_dataBlock.h>


const char *DEFFAULT_LAYOUT_BLK_NAME = "winlayout.blk";


//=============================================================================
LayoutSaver::LayoutSaver(WinManager *cur_owner) : mOwner(cur_owner), mNodeCounter(0), mNodeInfoArray(midmem), mMainWinPos(0, 0) {}


//=============================================================================
LayoutSaver::~LayoutSaver() { emptyNodeInfoArrays(); }


//=============================================================================
void LayoutSaver::saveLayoutToDataBlock(DataBlock &save_data_block)
{
  if (!mOwner->getRoot())
    return;

  if (!fillNodeInfoArrays())
    return;

  saveToDataBlock(save_data_block);
}


//=============================================================================
bool LayoutSaver::fillNodeInfoArrays()
{
  mNodeCounter = 0;
  emptyNodeInfoArrays();

  int fake_child_ID;
  if (!fillNodeInfoArray(mOwner->getRoot(), fake_child_ID))
    return false;
  if (!fillMovableInfoArray())
    return false;

  return true;
}


//=============================================================================
void LayoutSaver::saveLayout(const char *filename)
{
  if (!mOwner->getRoot())
    return;

  if (!fillNodeInfoArrays())
    return;

  saveToBlk(filename);
}


//=============================================================================
void LayoutSaver::fillLayout()
{
  if (mNodeInfoArray.size() < 1)
    return;

  NodeInfo *rootNode = mNodeInfoArray[0];

  int x = rootNode->mPosition.x;
  int y = rootNode->mPosition.y;
  int w = rootNode->mPosition.z - rootNode->mPosition.x + _pxS(MAIN_CLIENT_AREA_OFFSET);
  int h = rootNode->mPosition.w - rootNode->mPosition.y + _pxS(MAIN_CLIENT_AREA_OFFSET);

  RECT rect = {0, 0, w, h};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, true);

  SetWindowPos((HWND)mOwner->getMainWindow(), 0, mMainWinPos.x, mMainWinPos.y, rect.right - rect.left, rect.bottom - rect.top,
    SWP_NOZORDER);

  mOwner->reset();

  ClientWindow *root = mOwner->getRoot()->getClientWindow();
  G_ASSERT(root && "root node is not ClientWindow");
  addWindow(root, rootNode);

  loadMovableWindowsWithoutOriginal();

  SendMessage((HWND)mOwner->getMainWindow(), WM_CAPTURECHANGED, 0, 0);
}


//=============================================================================
bool LayoutSaver::loadLayout(const char *filename)
{
  if (!loadFromBlk(filename))
    return false;
  fillLayout();
  return true;
}


//=============================================================================
void LayoutSaver::loadMovableWindowsWithoutOriginal()
{
  for (int i = 0; i < mNodeInfoArray.size(); ++i)
    if (mNodeInfoArray[i]->mIsMovable)
    {
      Point4 pos = mNodeInfoArray[i]->mPosition;
      RECT windowRect = {0, 0, 10, 10};
      AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, true, WS_EX_TOOLWINDOW);
      int xOffset = windowRect.right - windowRect.left - 10;
      int yOffset = windowRect.bottom - windowRect.top - 10;

      ClientWindow *movableWnd = mOwner->makeMovableWindow(NULL, pos.x, pos.y, pos.z - pos.x - xOffset, pos.w - pos.y - yOffset, "");

      if (!movableWnd)
      {
        MessageBoxA(0, "Can't load movable window!", "Error", 0);
        break;
      }

      movableWnd->setType(mNodeInfoArray[i]->mWndType);
      WindowSizeLock fixedType = (WindowSizeLock)mNodeInfoArray[i]->mFixedType;
      if (fixedType != WSL_NONE)
      {
        movableWnd->getMovable()->lockSize(fixedType, pos.z - pos.x, pos.w - pos.y);
        movableWnd->setFixed(true);
      }
    }
}


//=============================================================================
void LayoutSaver::addWindow(ClientWindow *window, NodeInfo *node)
{
  if (node->mFirstChildID == 0 && node->mSecondChildID == 0)
  {
    window->setType(node->mWndType);
    window->setFixed((bool)node->mFixedType);
    return;
  }

  NodeInfo *firstChild = findNodeByID(node->mFirstChildID);
  NodeInfo *secondChild = findNodeByID(node->mSecondChildID);

  IPoint2 splitterPos(firstChild->mPosition.z, firstChild->mPosition.w);

  if (node->mIsVerticalSplitter)
  {
    splitterPos.x += _pxS(SPLITTER_SPACE);
    splitterPos.y -= _pxS(DISTANCE_TO_BORDER) + _pxS(SPLITTER_THICKNESS);
  }
  else
  {
    splitterPos.x -= _pxS(DISTANCE_TO_BORDER) + _pxS(SPLITTER_THICKNESS);
    splitterPos.y += _pxS(SPLITTER_SPACE);
  }

  client_to_screen(mOwner->getMainWindow(), splitterPos);

  VirtualWindow *virtualWindow = mOwner->addSplitter(window, splitterPos, node->mIsVerticalSplitter);

  G_ASSERT(virtualWindow && "parent not virtual!");

  if (ClientWindow *leftWnd = virtualWindow->getFirst()->getClientWindow())
  {
    NodeInfo *subNode = findNodeByID(node->mFirstChildID);
    G_ASSERT(subNode && "LayoutSaver::addWindow(): first child node == NULL");

    addWindow(leftWnd, subNode);
  }

  if (ClientWindow *rightWnd = virtualWindow->getSecond()->getClientWindow())
  {
    NodeInfo *subNode = findNodeByID(node->mSecondChildID);
    G_ASSERT(subNode && "LayoutSaver::addWindow(): second child node == NULL");

    addWindow(rightWnd, subNode);
  }
}


//=============================================================================
void LayoutSaver::emptyNodeInfoArrays()
{
  for (int i = 0; i < mNodeInfoArray.size(); i++)
  {
    if (mNodeInfoArray[i])
      delete mNodeInfoArray[i];
  }

  clear_and_shrink(mNodeInfoArray);
}


//=============================================================================
bool LayoutSaver::fillMovableInfoArray()
{
  dag::ConstSpan<MovableWindow *> movableWnds = mOwner->getMovableWindows();

  for (int i = 0; i < movableWnds.size(); ++i)
  {
    G_ASSERT(movableWnds[i] && "LayoutSaver::fillMovableInfoArray(): movable window == NULL");

    ClientWindow *node = movableWnds[i]->getClientWindow();

    G_ASSERT(node && "LayoutSaver::fillMovableInfoArray(): movable client window == NULL");

    RECT rect;
    GetWindowRect((HWND)movableWnds[i]->getHandle(), &rect);
    IPoint2 wndLeftTop(rect.left, rect.top), wndRightBottom(rect.right, rect.bottom);
    if (wndLeftTop.x > wndRightBottom.x || wndLeftTop.y > wndRightBottom.y)
    {
      logerr("LayoutSaver: invalid coordinates! wndLeftTop: %d, %d, wndRightBottom: %d, %d, caption: %s", wndLeftTop.x, wndLeftTop.y,
        wndRightBottom.x, wndRightBottom.y, node->getClientWindow() ? node->getClientWindow()->getCaption() : "<null>");
      return false;
    }

    int type = node->getType();

    MovableWindow *movable = node->getMovable();
    WindowSizeLock fixedType = movable ? movable->getSizeLock() : WSL_NONE;

    NodeInfo *curNode = new NodeInfo(mNodeCounter, wndLeftTop, wndRightBottom, type, fixedType, node, true);

    curNode->mFirstChildID = curNode->mSecondChildID = 0;
    mNodeInfoArray.push_back(curNode);
    ++mNodeCounter;
  }

  return true;
}


//=============================================================================
bool LayoutSaver::fillNodeInfoArray(CascadeWindow *node, int &child_ID)
{
  child_ID = mNodeCounter;
  IPoint2 wndLeftTop, wndRightBottom;
  node->getPos(wndLeftTop, wndRightBottom);
  if (wndLeftTop.x > wndRightBottom.x || wndLeftTop.y > wndRightBottom.y)
  {
    logerr("LayoutSaver: invalid coordinates! wndLeftTop: %d, %d, wndRightBottom: %d, %d, caption: %s", wndLeftTop.x, wndLeftTop.y,
      wndRightBottom.x, wndRightBottom.y, node->getClientWindow() ? node->getClientWindow()->getCaption() : "<null>");
    return false;
  }

  int type = 0;
  WindowSizeLock fixedType = WSL_NONE;
  ClientWindow *clientNode = node->getClientWindow();
  if (clientNode)
  {
    type = clientNode->getType();
    fixedType = clientNode->getSizeLock();
  }

  NodeInfo *curNode = new NodeInfo(mNodeCounter, wndLeftTop, wndRightBottom, type, fixedType, node, false);
  mNodeInfoArray.push_back(curNode);

  ++mNodeCounter;

  VirtualWindow *virtualWindow = node->getVirtualWindow();
  if (!virtualWindow)
  {
    curNode->mFirstChildID = 0;
    curNode->mSecondChildID = 0;
  }
  else
  {
    int leftID, rightID;
    CascadeWindow *left = virtualWindow->getFirst();
    CascadeWindow *right = virtualWindow->getSecond();
    if (!fillNodeInfoArray(left, leftID))
      return false;
    if (!fillNodeInfoArray(right, rightID))
      return false;
    curNode->mFirstChildID = leftID;
    curNode->mSecondChildID = rightID;
    curNode->mIsVerticalSplitter = virtualWindow->isVertical();
  }

  return true;
}


//=============================================================================
NodeInfo *LayoutSaver::findNodeByID(int ID)
{
  for (int i = 0; i < mNodeInfoArray.size(); i++)
    if (mNodeInfoArray[i]->mOwnID == ID)
      return mNodeInfoArray[i];

  return NULL;
}


//=============================================================================
void LayoutSaver::saveToDataBlock(DataBlock &data_block)
{
  const int BUFFER_SIZE = 8;
  char nameID[BUFFER_SIZE];
  IPoint2 childrenID;

  RECT rect;
  GetWindowRect((HWND)mOwner->getMainWindow(), &rect);

  DataBlock *mainPos = data_block.addNewBlock("main_window");
  mainPos->addIPoint2("pos", IPoint2(rect.left, rect.top));

  for (int i = 0; i < mNodeInfoArray.size(); i++)
  {
    NodeInfo *curNode = mNodeInfoArray[i];
    sprintf(nameID, "win_%d", curNode->mOwnID);
    childrenID.x = curNode->mFirstChildID;
    childrenID.y = curNode->mSecondChildID;

    DataBlock *nodeBlock = data_block.addNewBlock(nameID);
    nodeBlock->addIPoint2("children", childrenID);
    nodeBlock->addPoint4("position", curNode->mPosition);
    nodeBlock->addInt("vertical", (int)curNode->mIsVerticalSplitter);
    nodeBlock->addInt("movable", (int)curNode->mIsMovable);
    nodeBlock->addInt("type", curNode->mWndType);
    nodeBlock->addInt("fixed", curNode->mFixedType);
  }
}


//=============================================================================
void LayoutSaver::saveToBlk(const char *filename)
{
  DataBlock saveDataBlock;

  saveToDataBlock(saveDataBlock);
  saveDataBlock.setInt("dpi", win32_system_dpi);

  const char *blk_file = filename ? filename : DEFFAULT_LAYOUT_BLK_NAME;
  saveDataBlock.saveToTextFile(blk_file);
}


//=============================================================================
bool LayoutSaver::loadFromDataBlock(const DataBlock &data_block)
{
  RECT rect;
  GetWindowRect((HWND)mOwner->getMainWindow(), &rect);
  mMainWinPos = IPoint2(rect.left, rect.top);

  int firstWinBlock = 0;
  const DataBlock *mainPos = data_block.getBlock(0);
  const char *name = mainPos->getBlockName();
  if (strstr(name, "main_window"))
  {
    mMainWinPos = mainPos->getIPoint2("pos", mMainWinPos);
    firstWinBlock = 1;
  }

  for (int i = firstWinBlock; i < data_block.blockCount(); ++i)
  {
    const DataBlock *subBlock = data_block.getBlock(i);

    const char *name = subBlock->getBlockName();

    int ownID = 0;
    int res = sscanf(name, "win_%d", &ownID);
    if (!res)
    {
      MessageBoxA(0, "Error while reading .blk!", "Error", 0);
      return false;
    }

    int isVertical = subBlock->getInt("vertical", 1);
    int type = subBlock->getInt("type", 0);
    WindowSizeLock fixedType = (WindowSizeLock)subBlock->getInt("fixed", 0);
    bool isMovable = (bool)subBlock->getInt("movable", 0);

    Point4 def4(6, 6, 6, 6);
    Point4 pos = subBlock->getPoint4("position", def4);
    IPoint2 leftTop, rightBottom;
    leftTop.x = (int)pos.x;
    leftTop.y = (int)pos.y;
    rightBottom.x = (int)pos.z;
    rightBottom.y = (int)pos.w;

    IPoint2 defaultValue(5, 5);
    IPoint2 rootChildren = subBlock->getIPoint2("children", defaultValue);

    NodeInfo *curNode = new NodeInfo(ownID, leftTop, rightBottom, type, fixedType, NULL, isMovable);
    curNode->mFirstChildID = rootChildren.x;
    curNode->mSecondChildID = rootChildren.y;
    curNode->mIsVerticalSplitter = isVertical;

    mNodeInfoArray.push_back(curNode);
  }

  return true;
}


//=============================================================================
bool LayoutSaver::loadFromBlk(const char *filename)
{
  const char *blk_file = filename ? filename : DEFFAULT_LAYOUT_BLK_NAME;
  if (!::dd_file_exist(blk_file))
  {
    MessageBoxA(0, "File is not exist!", "Error", 0);
    return false;
  }

  DataBlock savedDataBlock(blk_file);
  if (savedDataBlock.getInt("dpi", 0) != win32_system_dpi)
    return false;
  emptyNodeInfoArrays();

  return loadFromDataBlock(savedDataBlock);
}


bool LayoutSaver::loadLayoutFromDataBlock(const DataBlock &load_data_block)
{
  emptyNodeInfoArrays();
  if (!loadFromDataBlock(load_data_block))
    return false;
  fillLayout();
  return true;
}
