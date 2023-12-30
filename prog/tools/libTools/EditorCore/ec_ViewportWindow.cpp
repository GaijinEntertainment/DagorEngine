// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "ec_cachedRender.h"
#include "ec_stat3d.h"
#include "ec_ViewportWindowStatSettingsDialog.h"

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>

#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_camera_dlg.h>
#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/captureCursor.h>
#include <EditorCore/ec_cm.h>

#include <gui/dag_stdGuiRenderEx.h>

#include <libTools/renderViewports/renderViewport.h>

#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <3d/dag_drv3d_pc.h>

#include <stdlib.h>

#include <generic/dag_initOnDemand.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameScene.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>
#include <gui/dag_baseCursor.h>
#include <scene/dag_visibility.h>

#include <propPanel2/comWnd/dialog_window.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

using hdpi::_pxActual;
using hdpi::_pxScaled;

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif

static real def_viewport_zn = 0.1f, def_viewport_zf = 1000.f;
static real def_viewport_fov = 1.57f;

bool ViewportWindow::showDagorUiCursor = true;

static void render_viewport_frame_stub(ViewportWindow *) {}
void (*ViewportWindow::render_viewport_frame)(ViewportWindow *vpw) = &render_viewport_frame_stub;


using namespace ec_camera_elem;

enum
{
  MENU_AREA_SIZE_W = 128,
  MENU_AREA_SIZE_H = 24,
};


static bool pointInMenuArea(ViewportWindow *vpw, int x, int y)
{
  hdpi::Px w, h;
  vpw->getMenuAreaSize(w, h);

  return x >= 0 && y >= 0 && x < hdpi::_px(w) && y <= hdpi::_px(h);
}


Tab<ViewportParams> ViewportWindow::viewportsParams(midmem);

class ViewportWindow::ViewportClippingDlg : public CDialogWindow
{
public:
  ViewportClippingDlg(float z_near, float z_far);

  float get_z_near();
  float get_z_far();

private:
  enum
  {
    Z_NEAR_ID,
    Z_FAR_ID,
  };
};


ViewportWindow::ViewportClippingDlg::ViewportClippingDlg(float z_near, float z_far) :
  CDialogWindow(0, _pxScaled(240), _pxScaled(135), "Viewport clipping")
{
  PropertyContainerControlBase *_panel = getPanel();
  G_ASSERT(_panel && "No panel in ViewportWindow::ViewportClippingDlg");

  _panel->createEditFloat(Z_NEAR_ID, "Near z-plane:", z_near, 3);
  _panel->createEditFloat(Z_FAR_ID, "Far z-plane:", z_far, 0);
}


real ViewportWindow::ViewportClippingDlg::get_z_near()
{
  PropertyContainerControlBase *_panel = getPanel();
  G_ASSERT(_panel && "No panel in ViewportWindow::ViewportClippingDlg");

  return _panel->getFloat(Z_NEAR_ID);
}

real ViewportWindow::ViewportClippingDlg::get_z_far()
{
  PropertyContainerControlBase *_panel = getPanel();
  G_ASSERT(_panel && "No panel in ViewportWindow::ViewportClippingDlg");

  return _panel->getFloat(Z_FAR_ID);
}


namespace ViewportType
{
enum
{
  PERSPECTIVE = 0,
  FRONT,
  BACK,
  TOP,
  BOTTOM,
  LEFT,
  RIGHT,
};
};


///////////////////////////////////////////////////////////////////////////////


unsigned ViewportWindow::restoreFlags = 0;


ViewportWindow::ViewportWindow(TEcHandle parent, int left, int top, int w, int h) : EcWindow(parent, left, top, w, h)
{
  showStats = false;
  calcStat3d = true;
  opaqueStat3d = false;
  curEH = NULL;
  prevMousePositionX = 0;
  prevMousePositionY = 0;
  isMoveRotateAllowed = false;
  isXLocked = false;
  isYLocked = false;
  skipNextAlt = false;
  orthogonalProjection = false;
  projectionFov = def_viewport_fov;
  projectionFarPlane = def_viewport_zf;
  projectionNearPlane = def_viewport_zn;
  orthogonalZoom = 1.f;
  currentProjection = CM_VIEW_PERSPECTIVE;
  viewText = "Perspective";
  nextStat3dLineY = hdpi::Px::ZERO;
  memset(&rectSelect, 0, sizeof(rectSelect));
  updatePluginCamera = false;
  customCameras = NULL;
  popupMenu = NULL;
  hidePopupMenu = false;
  mIsCursorVisible = true;
  showCameraStats = true;
  showCameraPos = true;
  showCameraDist = false;
  showCameraFov = false;
  showCameraSpeed = false;
  showCameraTurboSpeed = false;
  statSettingsDialog = nullptr;
  wireframeOverlay = false;

  viewport = new (midmem) RenderViewport;
  viewport->setPerspHFov(1.3, 0.01, 1000.0);

  if (!ec_cached_viewports)
    ec_cached_viewports = new (inimem) CachedRenderViewports;

  vpId = ec_cached_viewports->addViewport(viewport, this, false);
}


ViewportWindow::~ViewportWindow()
{
  del_it(statSettingsDialog);
  if (ec_cached_viewports)
    ec_cached_viewports->removeViewport(vpId);
  del_it(viewport);
  vpId = -1;
}


void ViewportWindow::init(IMenu *menu, IGenEventHandler *eh)
{
  setEventHandler(eh);
  popupMenu = menu;
  OnChangePosition();
}


void ViewportWindow::setEventHandler(IGenEventHandler *eh) { curEH = eh; }


void ViewportWindow::getMenuAreaSize(hdpi::Px &w, hdpi::Px &h)
{
  w = _pxScaled(MENU_AREA_SIZE_W);
  h = _pxScaled(MENU_AREA_SIZE_H);
}


void ViewportWindow::fillPopupMenu(IMenu &menu)
{
  menu.clearMenu(ROOT_MENU_ITEM);
  menu.setEventHandler(this);

  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_PERSPECTIVE, "Perspective\tShift+P");
  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_FRONT, "Front\tShift+F");
  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_BACK, "Back\tShift+K");
  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_TOP, "Top\tShift+T");
  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_BOTTOM, "Bottom\tShift+B");
  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_LEFT, "Left\tShift+L");
  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_RIGHT, "Right\tShift+R");
  menu.setCheckById(currentProjection);


  if (customCameras)
  {
    // CtlMenuManager mCameras ( -1, MF_POPUP | MF_VERT );
    // customCameras->addCamerasToMenu(mCameras, currentProjection);
    // menu.insertSubMenu("Cameras", 0, mCameras );
  }

  // if (currentProjection != 0xFFFFFFFF)
  //   menu.checkItem(currentProjection, true, false);

  menu.addSeparator(ROOT_MENU_ITEM, 0);
  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_WIREFRAME, "Wireframe, Edged faces\tF3");
  menu.setCheckById(CM_VIEW_WIREFRAME, viewport && (viewport->wireframe || wireframeOverlay));

  {
    menu.addSubMenu(ROOT_MENU_ITEM, CM_GROUP_GRID, "Grid");

    menu.addItem(CM_GROUP_GRID, CM_VIEW_GRID_SHOW, "Show grid\tG");
    menu.addItem(CM_GROUP_GRID, CM_OPTIONS_GRID, "Grid settings...");

    menu.setCheckById(CM_VIEW_GRID_SHOW, EDITORCORE->getGrid().isVisible(vpId));
  }

  menu.addSeparator(ROOT_MENU_ITEM, 0);
  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_SHOW_STATS, "Show stats");
  menu.setCheckById(CM_VIEW_SHOW_STATS, showStats);
  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_STAT3D_OPAQUE, "Opaque stats");
  menu.setCheckById(CM_VIEW_STAT3D_OPAQUE, opaqueStat3d);

  if (viewport)
  {
    String projectionName(viewportCommandToName(currentProjection));
    /*
    unsigned id;

    switch (currentProjection)
    {
      case CM_VIEW_PERSPECTIVE:
        id = ViewportType::PERSPECTIVE;
        break;

      case CM_VIEW_FRONT:
        id = ViewportType::FRONT;
        break;

      case CM_VIEW_BACK:
        id = ViewportType::BACK;
        break;

      case CM_VIEW_TOP:
        id = ViewportType::TOP;
        break;

      case CM_VIEW_BOTTOM:
        id = ViewportType::BOTTOM;
        break;

      case CM_VIEW_LEFT:
        id = ViewportType::LEFT;
        break;

      case CM_VIEW_RIGHT:
        id = ViewportType::RIGHT;
        break;
    }
    */

    if ((stricmp(projectionName, "Custom")) && (stricmp(projectionName, "Camera")))
    {
      String saveStr(128, "Save active %s view", (char *)projectionName);
      String restoreStr(128, "Restore active %s view", (char *)projectionName);
      menu.addSeparator(ROOT_MENU_ITEM, 0);

      menu.addItem(ROOT_MENU_ITEM, CM_SAVE_ACTIVE_VIEW, saveStr.str());
      menu.addItem(ROOT_MENU_ITEM, CM_RESTORE_ACTIVE_VIEW, restoreStr.str());
      menu.setEnabledById(CM_RESTORE_ACTIVE_VIEW, restoreFlags & (1 << vpId));
    }

    menu.addSeparator(ROOT_MENU_ITEM, 0);

    menu.addSubMenu(ROOT_MENU_ITEM, CM_GROUP_CLIPPING, "Viewport clipping");
    menu.addItem(CM_GROUP_CLIPPING, CM_SET_VIEWPORT_CLIPPING, "Set");
    menu.addItem(CM_GROUP_CLIPPING, CM_SET_DEFAULT_VIEWPORT_CLIPPING, "Reset to default");
  }
}


void ViewportWindow::processCameraEvents(CCameraElem *camera_elem, unsigned msg, TEcWParam w_param, TEcLParam l_param)
{
  G_ASSERT(camera_elem && "ViewportWindow::processCameraEvents camera_elem is NULL!!!");

  camera_elem->setMultiply(wingw::is_key_pressed(VK_SHIFT));

  if (isActive())
  {
    switch (msg)
    {
      case WM_KEYDOWN: camera_elem->handleKeyPress((int)(uintptr_t)w_param); break;
      case WM_KEYUP: camera_elem->handleKeyRelease((int)(uintptr_t)w_param); break;
      case WM_MOUSEMOVE:
      {
        real deltaX, deltaY;
        int p1 = GET_X_LPARAM(l_param);
        int p2 = GET_Y_LPARAM(l_param);
        processMouseMoveInfluence(deltaX, deltaY, msg, p1, p2, (int)(uintptr_t)w_param);
        camera_elem->handleMouseMove(-deltaX, -deltaY);
        int p11 = p1, p22 = p2;
        ::cursor_wrap(p11, p22, (void *)getHandle());
        prevMousePositionX = p11;
        prevMousePositionY = p22;
        updatePluginCamera = true;
        break;
      }
    }
  }

  if (msg == WM_MOUSEWHEEL)
    camera_elem->handleMouseWheel((short)HIWORD(w_param) / 120);
}


void ViewportWindow::processHotKeys(unsigned key_code)
{
  bool shiftPressed = wingw::is_key_pressed(VK_SHIFT);

  switch (key_code)
  {
    case 'P':
      if (shiftPressed)
        onMenuItemClick(CM_VIEW_PERSPECTIVE);
      break;

    case 'F':
      if (shiftPressed)
        onMenuItemClick(CM_VIEW_FRONT);
      break;

    case 'K':
      if (shiftPressed)
        onMenuItemClick(CM_VIEW_BACK);
      break;

    case 'T':
      if (shiftPressed)
        onMenuItemClick(CM_VIEW_TOP);
      break;

    case 'B':
      if (shiftPressed)
        onMenuItemClick(CM_VIEW_BOTTOM);
      break;

    case 'L':
      if (shiftPressed)
        onMenuItemClick(CM_VIEW_LEFT);
      break;

    case 'R':
      if (shiftPressed)
        onMenuItemClick(CM_VIEW_RIGHT);
      break;

    case 'G': onMenuItemClick(CM_VIEW_GRID_SHOW); break;

    case VK_F3: onMenuItemClick(CM_VIEW_WIREFRAME); break;
  }
}


int ViewportWindow::windowProc(TEcHandle h_wnd, unsigned msg, TEcWParam w_param, TEcLParam l_param)
{
  int _modif = wingw::get_modif();
  bool ctrlPressed = (bool)(_modif & wingw::M_CTRL);
  bool shiftPressed = (bool)(_modif & wingw::M_SHIFT);

  switch (msg)
  {
    case WM_KILLFOCUS:
      if (!mIsCursorVisible)
      {
        ShowCursor(true);
        mIsCursorVisible = true;
      }
      prevMousePositionX = prevMousePositionY = INT32_MIN;
      break;
    case WM_SETFOCUS:
      switch (CCameraElem::getCamera())
      {
        case CCameraElem::FREE_CAMERA:
        case CCameraElem::FPS_CAMERA:
        case CCameraElem::TPS_CAMERA:
        case CCameraElem::CAR_CAMERA:
          capture_cursor((void *)getHandle());
          if (mIsCursorVisible)
          {
            ShowCursor(false);
            mIsCursorVisible = false;
          }
          break;
      }
      break;

    case WM_KEYDOWN:
      processHotKeys((int)(uintptr_t)w_param);
      if (curEH)
        curEH->handleKeyPress(this, (int)(uintptr_t)w_param, _modif);
      break;

    case WM_KEYUP:
      if (curEH)
        curEH->handleKeyRelease(this, (int)(uintptr_t)w_param, _modif);
      break;

    case WM_RBUTTONUP:
      // call popup menu
      if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
      {
        int x = GET_X_LPARAM(l_param);
        int y = GET_Y_LPARAM(l_param);

        bool in_menu_area = pointInMenuArea(this, x, y);
        if (popupMenu && in_menu_area)
          fillPopupMenu(*popupMenu);

        // if (!hidePopupMenu || in_menu_area)
        if (in_menu_area)
          SendMessage(GetParent((HWND)h_wnd), msg, (WPARAM)w_param, (LPARAM)l_param);
      }
      break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN: this->activate(); break;

    case WM_PAINT: handleWindowPaint(); break;

    case WM_ERASEBKGND:
      if (dagor_get_current_game_scene())
        return 1;
      break;

    case WM_CAPTURECHANGED:
      /*
      POINT pt;
      GetCursorPos(&pt);
      if (wingw::is_key_pressed(wingw::V_LBUTTON))
        SendMessage((HWND) h_wnd, WM_LBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
      if (wingw::is_key_pressed(wingw::V_RBUTTON))
        SendMessage((HWND) h_wnd, WM_RBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
      */
      break;

    case WM_DROPFILES:
    {
      HDROP hdrop = (HDROP)w_param;
      const int fileCount = DragQueryFileA(hdrop, -1, nullptr, 0);
      if (fileCount > 0)
      {
        dag::Vector<String> files;
        files.set_capacity(fileCount);

        for (int i = 0; i < fileCount; ++i)
        {
          const int lengthWithoutNullTerminator = DragQueryFileA(hdrop, 0, nullptr, 0);
          if (lengthWithoutNullTerminator > 0)
          {
            String path;
            path.resize(lengthWithoutNullTerminator + 1);
            if (DragQueryFileA(hdrop, 0, path.begin(), lengthWithoutNullTerminator + 1))
              files.emplace_back(path);
          }
        }

        if (onDropFiles(files))
          return 0;
      }
    }
    break;
  }

  // pass messages to cameras

  switch (CCameraElem::getCamera())
  {
    case CCameraElem::FPS_CAMERA: processCameraEvents(fpsCameraElem, msg, w_param, l_param); break;

    case CCameraElem::TPS_CAMERA: processCameraEvents(tpsCameraElem, msg, w_param, l_param); break;

    case CCameraElem::CAR_CAMERA: processCameraEvents(carCameraElem, msg, w_param, l_param); break;

    case CCameraElem::FREE_CAMERA: processCameraEvents(freeCameraElem, msg, w_param, l_param); break;

    case CCameraElem::MAX_CAMERA:
    {
      if (maxCameraElem)
        maxCameraElem->setMultiply(ctrlPressed);

      switch (msg)
      {
        case WM_MBUTTONDOWN:
          prevMousePositionX = GET_X_LPARAM(l_param);
          prevMousePositionY = GET_Y_LPARAM(l_param);
          isMoveRotateAllowed = true;
          isXLocked = false;
          isYLocked = false;

          // CtlWindow::activate(false);
          break;

        case WM_MBUTTONUP:
          isMoveRotateAllowed = false;

          if (wingw::is_key_pressed(VK_MENU))
            skipNextAlt = true;
          else
            skipNextAlt = false;
          break;

        case WM_KEYDOWN:
          if (maxCameraElem)
            maxCameraElem->handleKeyPress((int)(uintptr_t)w_param);
          break;
      }

      processCameraControl(h_wnd, msg, w_param, l_param);
    }
  }

  // route message to external event handler

  if (curEH && !isMoveRotateAllowed && CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
  {
    POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};

    switch (msg)
    {
      case WM_MOUSEMOVE: curEH->handleMouseMove(this, pt.x, pt.y, true, (int)(uintptr_t)w_param, _modif); break;
      case WM_LBUTTONDOWN:
        captureMouse();
        curEH->handleMouseLBPress(this, pt.x, pt.y, true, (int)(uintptr_t)w_param, _modif);
        break;
      case WM_RBUTTONDOWN:
        if (!pointInMenuArea(this, pt.x, pt.y))
        {
          captureMouse();
          hidePopupMenu = curEH->handleMouseRBPress(this, pt.x, pt.y, true, (int)(uintptr_t)w_param, _modif);
        }
        break;
      case WM_LBUTTONUP:
        releaseMouse();
        curEH->handleMouseLBRelease(this, pt.x, pt.y, true, (int)(uintptr_t)w_param, _modif);
        break;
      case WM_RBUTTONUP:
        releaseMouse();
        curEH->handleMouseRBRelease(this, pt.x, pt.y, true, (int)(uintptr_t)w_param, _modif);
        break;
      case WM_LBUTTONDBLCLK: curEH->handleMouseDoubleClick(this, pt.x, pt.y, _modif); break;
      case WM_KEYUP: curEH->handleKeyRelease(this, (int)(uintptr_t)l_param, (int)(uintptr_t)w_param); break;
      case WM_MOUSEWHEEL: curEH->handleMouseWheel(this, (short)HIWORD(w_param) / 120, pt.x, pt.y, _modif); break;
    }
  }

  if (rectSelect.active)
  {
    if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
      processRectSelection(h_wnd, msg, w_param, l_param);
    else
      rectSelect.active = false;
  }

  return EcWindow::windowProc(h_wnd, msg, w_param, l_param);
}


int ViewportWindow::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case CM_VIEW_PERSPECTIVE:
      setProjection(false, projectionFov, projectionNearPlane, projectionFarPlane);
      currentProjection = id;
      setCameraViewText();
      redrawClientRect();
      return 0;

    case CM_VIEW_FRONT:
      setProjection(true, projectionFov, projectionNearPlane, projectionFarPlane);
      setCameraDirection(Point3(0.f, 0.f, 1.f), Point3(0.f, 1.f, 0.f));
      currentProjection = id;
      setCameraViewText();
      redrawClientRect();
      return 0;

    case CM_VIEW_BACK:
      setProjection(true, projectionFov, projectionNearPlane, projectionFarPlane);
      setCameraDirection(Point3(0.f, 0.f, -1.f), Point3(0.f, 1.f, 0.f));
      currentProjection = id;
      setCameraViewText();
      redrawClientRect();
      return 0;

    case CM_VIEW_TOP:
      setProjection(true, projectionFov, projectionNearPlane, projectionFarPlane);
      setCameraDirection(Point3(0.f, -1.f, 0.f), Point3(0.f, 0.f, 1.f));
      currentProjection = id;
      setCameraViewText();
      redrawClientRect();
      return 0;

    case CM_VIEW_BOTTOM:
      setProjection(true, projectionFov, projectionNearPlane, projectionFarPlane);
      setCameraDirection(Point3(0.f, 1.f, 0.f), Point3(0.f, 0.f, -1.f));
      currentProjection = id;
      setCameraViewText();
      redrawClientRect();
      return 0;

    case CM_VIEW_LEFT:
      setProjection(true, projectionFov, projectionNearPlane, projectionFarPlane);
      setCameraDirection(Point3(1.f, 0.f, 0.f), Point3(0.f, 1.f, 0.f));
      currentProjection = id;
      setCameraViewText();
      redrawClientRect();
      return 0;

    case CM_VIEW_RIGHT:
      setProjection(true, projectionFov, projectionNearPlane, projectionFarPlane);
      setCameraDirection(Point3(-1.f, 0.f, 0.f), Point3(0.f, 1.f, 0.f));
      currentProjection = id;
      setCameraViewText();
      redrawClientRect();
      return 0;

    case CM_VIEW_WIREFRAME:
      if (viewport)
      {
        if (!viewport->wireframe && !wireframeOverlay)
        {
          // base shading => wireframe
          viewport->wireframe = true;
        }
        else if (viewport->wireframe)
        {
          // wireframe => edged faces
          viewport->wireframe = false;
          wireframeOverlay = true;
        }
        else if (wireframeOverlay)
        {
          // edged faces => base shading
          viewport->wireframe = false;
          wireframeOverlay = false;
        }
      }

      redrawClientRect();
      return 0;

    case CM_SET_VIEWPORT_CLIPPING:
    {
      ViewportClippingDlg dialog(projectionNearPlane, projectionFarPlane);
      if (dialog.showDialog() != DIALOG_ID_OK)
        return 0;

      projectionNearPlane = dialog.get_z_near();
      projectionFarPlane = dialog.get_z_far();
      OnChangePosition();
      redrawClientRect();
      return 0;
    }

    case CM_SET_DEFAULT_VIEWPORT_CLIPPING:
      projectionNearPlane = def_viewport_zn;
      projectionFarPlane = def_viewport_zf;
      OnChangePosition();
      redrawClientRect();
      return 0;

    case CM_OPTIONS_GRID:
    {
      GridEditDialog dlg(0, EDITORCORE->getGrid(), "Viewport grid settings", vpId);
      dlg.showDialog();
    }
      return 0;

    case CM_VIEW_GRID_SHOW:
    {
      bool is_visible = EDITORCORE->getGrid().isVisible(vpId);
      EDITORCORE->getGrid().setVisible(!is_visible, vpId);
    }
      return 0;

    case CM_VIEW_SHOW_STATS: showStats = !showStats; return 0;

    case CM_VIEW_STAT3D_OPAQUE: opaqueStat3d = !opaqueStat3d; return 0;

    case CM_SAVE_ACTIVE_VIEW:
    {
      if (vpId > 32)
        break;

      if (viewportsParams.size() <= vpId)
        viewportsParams.resize(vpId + 1);

      ViewportWindow::restoreFlags |= (1 << vpId);

      getCameraTransform(viewportsParams[vpId].view);
      viewportsParams[vpId].fov = getFov();

      break;
    }
    case CM_RESTORE_ACTIVE_VIEW:
    {
      if (!(ViewportWindow::restoreFlags & (1 << vpId)) || (vpId >= viewportsParams.size()))
        break;

      setCameraTransform(viewportsParams[vpId].view);
      setFov(viewportsParams[vpId].fov);

      break;
    }
  }

  return 1;
}


int ViewportWindow::handleCommand(int p1, int p2, int p3)
{
  bool ctrlPressed = wingw::is_key_pressed(VK_CONTROL);
  bool shiftPressed = wingw::is_key_pressed(VK_SHIFT);

  switch (p1)
  {
    case CM_CAMERAS_FREE:
    case CM_CAMERAS_FPS:
    case CM_CAMERAS_TPS:
    case CM_CAMERAS_CAR:
      if (p1 == CM_CAMERAS_FPS)
      {
        ctrlPressed = true;
        shiftPressed = false;
      }
      if (p1 == CM_CAMERAS_TPS)
      {
        ctrlPressed = true;
        shiftPressed = true;
      }
      if (p1 == CM_CAMERAS_CAR)
      {
        ctrlPressed = false;
        shiftPressed = true;
      }

      POINT restore_pos = {restoreCursorAtX, restoreCursorAtY};

      if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
      {
        GetCursorPos(&restore_pos);
        restoreCursorAtX = restore_pos.x;
        restoreCursorAtY = restore_pos.y;
      }

      if (p1 != CM_CAMERAS_CAR && CCameraElem::getCamera() == CCameraElem::CAR_CAMERA)
      {
        carCameraElem->end();
      }

      CCameraElem::switchCamera(ctrlPressed, shiftPressed); // change camera type
      activate();

      switch (CCameraElem::getCamera())
      {
        case CCameraElem::FREE_CAMERA:
        case CCameraElem::FPS_CAMERA:
        case CCameraElem::TPS_CAMERA:
        case CCameraElem::CAR_CAMERA:
        {
          if (CCameraElem::getCamera() == CCameraElem::FPS_CAMERA)
          {
            fpsCameraElem->setAboveSurface(true);
            fpsCameraElem->handleKeyPress(VK_SPACE);
          }
          if (CCameraElem::getCamera() == CCameraElem::TPS_CAMERA)
          {
            tpsCameraElem->setAboveSurface(true);
            tpsCameraElem->handleKeyPress(VK_SPACE);
          }
          if (CCameraElem::getCamera() == CCameraElem::CAR_CAMERA)
          {
            carCameraElem->setAboveSurface(true);
            if (!carCameraElem->begin())
            {
              this->handleCommand(CM_CAMERAS_FREE, p2, p3);
              return 0;
            }
            carCameraElem->handleKeyPress(VK_SPACE);
          }

          capture_cursor((void *)getHandle());
          if (mIsCursorVisible)
          {
            ShowCursor(false);
            mIsCursorVisible = false;
          }
        }
        break;

        case CCameraElem::MAX_CAMERA:
          release_cursor();
          if (!mIsCursorVisible)
          {
            ShowCursor(true);
            mIsCursorVisible = true;
          }
          SetCursorPos(restoreCursorAtX, restoreCursorAtY);
          break;

        default: DAG_FATAL("Unknown camera type. Contact developers.");
      }

      break;
  }

  return 0;
}


void ViewportWindow::OnChangePosition()
{
  EcRect clientRect;
  getClientRect(clientRect);

  if (orthogonalProjection)
  {
    Matrix44 projection;
    // -FarPlane and +Farplane are used instead of NearPlane and FarPlane to avoid geometry
    // clipping by NearPlane. That clipping is hardly controllable by user because the mouse wheel does not
    // change camera position.

    projection = matrix_ortho_lh_reverse((clientRect.r - clientRect.l) / orthogonalZoom,
      (clientRect.b - clientRect.t) / orthogonalZoom, -projectionFarPlane, projectionFarPlane);

    viewport->setProjectionMatrix(projection);
  }
  else
  {
    real verticalFov =
      2.f * atanf(tanf(0.5f * projectionFov) * ((real)(clientRect.b - clientRect.t) / (real)(clientRect.r - clientRect.l)));
    real wk = 1.f / tanf(0.5f * projectionFov);
    real hk = 1.f / tanf(0.5f * verticalFov);
    viewport->setPersp(wk, hk, projectionNearPlane, projectionFarPlane);
  }

  viewport->setlt(Point2(clientRect.l, clientRect.t));
  viewport->setrb(Point2(clientRect.r, clientRect.b));
}


void ViewportWindow::setZnearZfar(real zn, real zf, bool change_defaults)
{
  if (change_defaults)
  {
    def_viewport_zn = zn;
    def_viewport_zf = zf;
  }
  projectionFarPlane = zf;
  projectionNearPlane = zn;
  OnChangePosition();
}


void ViewportWindow::getZnearZfar(real &zn, real &zf) const
{
  zn = projectionNearPlane;
  zf = projectionFarPlane;
}


void ViewportWindow::OnDestroy() { ec_cached_viewports->disableViewport(vpId); }


void ViewportWindow::activate()
{
  EcWindow::activate(true);
  OnChangeState();
}


void ViewportWindow::OnChangeState()
{
  if (isActive())
  {
    CCameraElem::setViewportWindow(this);
  }
}


int ViewportWindow::processCameraControl(TEcHandle h_wnd, unsigned msg, TEcWParam w_param, TEcLParam l_param)
{
  if ((CCameraElem::getCamera() != CCameraElem::MAX_CAMERA) || (!maxCameraElem))
    return 1;

  bool ctrlPressed = wingw::is_key_pressed(VK_CONTROL);
  bool shiftPressed = wingw::is_key_pressed(VK_SHIFT);
  bool altPressed = wingw::is_key_pressed(VK_MENU);

  maxCameraElem->setMultiply(ctrlPressed);
  real multiplier = ctrlPressed ? maxCameraElem->getMultiplier() : 1;
  switch (msg)
  {
    case WM_MOUSEWHEEL:
    {
      short ml = (short)HIWORD(w_param);

      if (orthogonalProjection)
      {
        float _zoom = (1.f + 0.1f * multiplier);
        setOrthogonalZoom((ml < 0) && (_zoom != 0) ? orthogonalZoom / _zoom : orthogonalZoom * _zoom);
      }
      else if (ml != 0)
        maxCameraElem->moveForward(1.0 * ml / abs(ml) * ::dagor_game_act_time, true, this);
    }

      updatePluginCamera = true;
      break;

    case WM_MBUTTONDOWN: capture_cursor((void *)getHandle()); break;

    case WM_MBUTTONUP: release_cursor(); break;

    case WM_MOUSEMOVE:
    {
      int p1 = GET_X_LPARAM(l_param);
      int p2 = GET_Y_LPARAM(l_param);

      if (uintptr_t(w_param) & MK_MBUTTON && isMoveRotateAllowed)
      {
        real deltaX, deltaY;

        processMouseMoveInfluence(deltaX, deltaY, msg, p1, p2, (int)(uintptr_t)w_param);
        deltaX *= ::dagor_game_act_time;
        deltaY *= ::dagor_game_act_time;

        if (altPressed)
        {
          if (ctrlPressed)
          {
            if (!orthogonalProjection)
              maxCameraElem->moveForward(-deltaY, true, this);
            else
            {
              if (deltaY > 0)
                setOrthogonalZoom(orthogonalZoom / (deltaY / 10.0 + 0.1f * multiplier));
              else
                setOrthogonalZoom(orthogonalZoom * (deltaY / 10.0 + 0.1f * multiplier));
            }

            updatePluginCamera = true;
          }
          else
          {
            maxCameraElem->rotate(-deltaX, -deltaY, true, true);
            if (orthogonalProjection && currentProjection != CM_VIEW_CUSTOM_CAMERA)
            {
              currentProjection = CM_VIEW_CUSTOM_CAMERA;
              setCameraViewText();
            }
          }
        }
        else
        {
          if (orthogonalProjection)
          {
            real moveStepX, moveStepY;
            moveStepX = 2.f / orthogonalZoom / ::dagor_game_act_time;
            moveStepY = 2.f / orthogonalZoom / ::dagor_game_act_time;
            maxCameraElem->strife(moveStepX * deltaX, moveStepY * deltaY, false, false);
          }

          else
          {
            BBox3 box;
            bool selection = IEditorCoreEngine::get()->getSelectionBox(box);

            if (selection)
            {
              Point3 pos = box.center();
              real sx = sqrtf(getLinearSizeSq(pos, 1, 0));
              real sy = sqrtf(getLinearSizeSq(pos, 1, 1));

              deltaX /= ::dagor_game_act_time;
              deltaY /= ::dagor_game_act_time;

              deltaX /= sx;
              deltaY /= sy;

              maxCameraElem->strife(deltaX, deltaY, false, false);
            }
            else
              maxCameraElem->strife(deltaX, deltaY, true, true);
          }
        }

        updatePluginCamera = true;
        int p11 = p1, p22 = p2;
        ::cursor_wrap(p11, p22, (void *)getHandle());
        p1 = p11;
        p2 = p22;
      }
      prevMousePositionX = p1;
      prevMousePositionY = p2;

      break;
    }
  }


  return 0;
}


void ViewportWindow::processMouseMoveInfluence(real &deltaX, real &deltaY, int id, int32_t p1, int32_t p2, int32_t p3)
{
  if (prevMousePositionX == INT32_MIN && prevMousePositionY == INT32_MIN)
  {
    deltaX = deltaY = 0;
    return;
  }

  deltaX = (p1 - prevMousePositionX);
  deltaY = (p2 - prevMousePositionY);

  if (abs(deltaX) > getW() * 3 / 4)
    deltaX = 0;
  if (abs(deltaY) > getH() * 3 / 4)
    deltaY = 0;

  if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA && wingw::is_key_pressed(VK_SHIFT))
  {
    if (!isXLocked && !isYLocked)
    {
      if (fabs(deltaX) > fabs(deltaY))
        isYLocked = true;
      else
        isXLocked = true;
    }

    if (isXLocked)
      deltaX = 0;
    else if (isYLocked)
      deltaY = 0;
  }
}


int ViewportWindow::processRectSelection(TEcHandle h_wnd, unsigned msg, TEcWParam w_param, TEcLParam l_param)
{
  switch (msg)
  {
    case WM_MOUSEMOVE:
      if (!wingw::is_key_pressed(VK_LBUTTON))
        break;

      rectSelect.sel.r = GET_X_LPARAM(l_param);
      rectSelect.sel.b = GET_Y_LPARAM(l_param);
      break;
  }

  return 0;
}


void ViewportWindow::setProjection(bool orthogonal, real fov, real near_plane, real far_plane)
{
  orthogonalProjection = orthogonal;
  projectionFov = fov;
  projectionFarPlane = far_plane;
  projectionNearPlane = near_plane;

  OnChangePosition();
}


void ViewportWindow::setCameraDirection(const Point3 &forward, const Point3 &up)
{
  Point3 right = normalize(up % forward);
  TMatrix viewMatrix;
  viewMatrix.m[0][0] = right.x;
  viewMatrix.m[1][0] = right.y;
  viewMatrix.m[2][0] = right.z;

  viewMatrix.m[0][1] = up.x;
  viewMatrix.m[1][1] = up.y;
  viewMatrix.m[2][1] = up.z;

  viewMatrix.m[0][2] = forward.x;
  viewMatrix.m[1][2] = forward.y;
  viewMatrix.m[2][2] = forward.z;
  viewMatrix.setcol(3, viewport->getViewMatrix().getcol(3));
  viewport->setViewMatrix(viewMatrix);

  currentProjection = CM_VIEW_CUSTOM_CAMERA;
  setCameraViewText();
  redrawClientRect();
}


void ViewportWindow::setCameraPos(const Point3 &pos)
{
  TMatrix temp = viewport->getViewMatrix();
  temp.orthonormalize();
  temp = inverse(temp);
  temp.setcol(3, pos);
  viewport->setViewMatrix(inverse(temp));
  //  viewport->viewMatrix.orthonormalize();

  redrawClientRect();
  updatePluginCamera = true;
}


void ViewportWindow::setCameraTransform(const TMatrix &tm)
{
  TMatrix newTm = tm;
  newTm.orthonormalize();

  viewport->setViewMatrix(inverse(newTm));
  //  viewport->viewMatrix.orthonormalize();

  redrawClientRect();
  updatePluginCamera = true;
}


void ViewportWindow::getCameraTransform(TMatrix &m) const { m = inverse(viewport->getViewMatrix()); }


//==============================================================================
real ViewportWindow::getOrthogonalZoom() const { return orthogonalZoom; }


//==============================================================================
bool ViewportWindow::isOrthogonal() const { return orthogonalProjection; }


//==============================================================================
bool ViewportWindow::isFlyMode() const { return CCameraElem::getCamera() != CCameraElem::MAX_CAMERA; }


//==============================================================================
void ViewportWindow::setOrthogonalZoom(real zoom)
{
  orthogonalZoom = zoom;
  OnChangePosition();
  OnCameraChanged();
  redrawClientRect();

  updatePluginCamera = true;
}


//==============================================================================
void ViewportWindow::setCameraViewProjection(const TMatrix &view, real fov)
{
  if (currentProjection != CM_VIEW_CUSTOM_CAMERA &&
      (currentProjection < CM_PLUGIN_FIRST_CAMERA || currentProjection >= CM_PLUGIN_LAST_CAMERA))
    return;

  viewport->setViewMatrix(view);

  EcRect clientRect;
  getClientRect(clientRect);

  real horizontalFov, verticalFov;
  if ((clientRect.b - clientRect.t) * 4 / 3 > clientRect.r - clientRect.l)
  {
    horizontalFov = fov;
    verticalFov = 2.f * atanf(tanf(0.5f * fov) * ((real)(clientRect.b - clientRect.t) / (real)(clientRect.r - clientRect.l)));
  }
  else
  {
    verticalFov = 2.f * atanf(tanf(0.5f * fov) * 3.f / 4.f);
    horizontalFov =
      2.f * atanf(tanf(0.5f * verticalFov) * ((real)(clientRect.r - clientRect.l) / (real)(clientRect.b - clientRect.t)));
  }
  projectionFov = horizontalFov;
  real wk = 1.f / tanf(0.5f * projectionFov);
  real hk = 1.f / tanf(0.5f * verticalFov);
  viewport->setPersp(wk, hk, projectionNearPlane, projectionFarPlane);
}


void ViewportWindow::setCameraMode(bool camera_mode)
{
  if (camera_mode == (currentProjection == CM_VIEW_CUSTOM_CAMERA))
    return;

  if (camera_mode)
  {
    currentProjection = CM_VIEW_CUSTOM_CAMERA;
    viewText = "Camera";
  }
  else
  {
    setProjection(false, projectionFov, projectionNearPlane, projectionFarPlane);
    currentProjection = CM_VIEW_PERSPECTIVE;
    setCameraViewText();
  }
  redrawClientRect();
}


void ViewportWindow::switchCamera(unsigned from, unsigned to)
{
  if (currentProjection == from)
  {
    currentProjection = to;
    setCameraViewText();
    redrawClientRect();
  }
}


void ViewportWindow::setFov(real fov)
{
  projectionFov = fov;
  const bool orthogonal = (currentProjection != CM_VIEW_PERSPECTIVE) && (currentProjection != CM_VIEW_CUSTOM_CAMERA);
  setProjection(orthogonal, projectionFov, projectionNearPlane, projectionFarPlane);
  redrawClientRect();
}


real ViewportWindow::getFov() { return projectionFov; }


void ViewportWindow::act()
{
  if (customCameras)
  {
    TMatrix cameraMatrix;
    getCameraTransform(cameraMatrix);

    if (currentProjection >= CM_PLUGIN_FIRST_CAMERA && currentProjection < CM_PLUGIN_LAST_CAMERA)
    {
      bool cameraFound = false;
      if (updatePluginCamera)
      {
        if (customCameras->setCamera(currentProjection, cameraMatrix))
          cameraFound = true;
      }
      else
      {
        String cameraName;
        real fov;
        if (customCameras->getCamera(currentProjection, cameraMatrix, fov, cameraName))
        {
          setCameraViewProjection(inverse(cameraMatrix), fov);
          if (strcmp(viewText, cameraName) != 0)
          {
            viewText = cameraName;
            redrawClientRect();
          }
          cameraFound = true;
        }
      }

      if (!cameraFound)
      {
        setProjection(false, projectionFov, projectionNearPlane, projectionFarPlane);
        currentProjection = CM_VIEW_PERSPECTIVE;
        setCameraViewText();
        redrawClientRect();
      }
    }
  }

  updatePluginCamera = false;
}


const char *ViewportWindow::viewportCommandToName(int id) const
{
  if (id == CM_VIEW_FRONT)
    return "Front";
  else if (id == CM_VIEW_BACK)
    return "Back";
  else if (id == CM_VIEW_TOP)
    return "Top";
  else if (id == CM_VIEW_BOTTOM)
    return "Bottom";
  else if (id == CM_VIEW_LEFT)
    return "Left";
  else if (id == CM_VIEW_RIGHT)
    return "Right";
  else if (id == CM_VIEW_PERSPECTIVE)
    return "Perspective";
  else if (id == CM_VIEW_CUSTOM_CAMERA)
    return "Custom";
  else if (id >= CM_PLUGIN_FIRST_CAMERA && id < CM_PLUGIN_LAST_CAMERA)
    return "Camera";
  return NULL;
}


int ViewportWindow::viewportNameToCommand(const char *name)
{
  if (!strcmp(name, "Front"))
    return CM_VIEW_FRONT;
  else if (!strcmp(name, "Back"))
    return CM_VIEW_BACK;
  else if (!strcmp(name, "Top"))
    return CM_VIEW_TOP;
  else if (!strcmp(name, "Bottom"))
    return CM_VIEW_BOTTOM;
  else if (!strcmp(name, "Left"))
    return CM_VIEW_LEFT;
  else if (!strcmp(name, "Right"))
    return CM_VIEW_RIGHT;
  else if (!strcmp(name, "Perspective"))
    return CM_VIEW_PERSPECTIVE;
  else if (!strcmp(name, "Custom"))
    return CM_VIEW_CUSTOM_CAMERA;
  else if (!strcmp(name, "Camera"))
    return CM_PLUGIN_FIRST_CAMERA;
  return -1;
}


void ViewportWindow::setCameraViewText()
{
  const char *viewportName = viewportCommandToName(currentProjection);
  viewText = viewportName ? viewportName : "Error";
}


void ViewportWindow::clientToZeroLevelPlane(int x, int y, Point3 &world)
{
  Point3 campos, camdir;
  clientToWorld(Point2(x, y), campos, camdir);
  real t = -campos.y / camdir.y;
  if (t > 0 && t < 1000)
    world = campos + camdir * t;
  if (t >= 1000)
  {
    world = campos + camdir * 1000;
  }
  if (t <= 0)
  {
    t = campos.y / camdir.y;
    if (t < 1000)
      world = -campos + camdir * t;
    if (t >= 1000)
    {
      world = -campos + camdir * 1000;
    }
  }
  world.y = 0;
}


void ViewportWindow::clientRectToWorld(Point3 *pt, Point3 *dirs, float fake_persp_side)
{
  EcRect clientRect;
  getClientRect(clientRect);

  Point2 d = Point2(clientRect.r - clientRect.l, clientRect.b - clientRect.t);
  Point3 world_dir;
  clientToWorld(Point2(0, 0), pt[0], world_dir);
  clientToWorld(Point2(0, d.y), pt[1], world_dir);
  clientToWorld(d, pt[2], world_dir);
  clientToWorld(Point2(d.x, 0), pt[3], world_dir);
  switch (currentProjection)
  {
    case CM_VIEW_TOP:
    case CM_VIEW_BOTTOM:
      pt[0].y = pt[1].y = pt[2].y = pt[3].y = 0;
      dirs[0] = Point3(0, 0, 1);
      dirs[1] = Point3(1, 0, 0);
      break;

    case CM_VIEW_LEFT:
    case CM_VIEW_RIGHT:
      pt[0].x = pt[1].x = pt[2].x = pt[3].x = 0;
      dirs[0] = Point3(0, 0, 1);
      dirs[1] = Point3(0, 1, 0);
      break;

    case CM_VIEW_FRONT:
    case CM_VIEW_BACK:
      pt[0].z = pt[1].z = pt[2].z = pt[3].z = 0;
      dirs[0] = Point3(0, 1, 0);
      dirs[1] = Point3(1, 0, 0);
      break;

    default:
    {
      Point3 p = inverse(viewport->getViewMatrix()).getcol(3);
      p.y = 0;

      pt[0] = p + fake_persp_side * Point3(-1, 0, -1);
      pt[1] = p + fake_persp_side * Point3(-1, 0, 1);
      pt[2] = p + fake_persp_side * Point3(1, 0, 1);
      pt[3] = p + fake_persp_side * Point3(1, 0, -1);

      dirs[0] = Point3(0, 0, 1);
      dirs[1] = Point3(1, 0, 0);
    }
    break;
  }
}


void ViewportWindow::clientToWorld(const Point2 &screen, Point3 &world, Point3 &world_dir)
{
  EcRect clientRect;
  getClientRect(clientRect);

  Point2 normalizedScreen;
  normalizedScreen.x = 2.f * screen.x / (clientRect.r - clientRect.l) - 1.f;
  normalizedScreen.y = -(2.f * screen.y / (clientRect.b - clientRect.t) - 1.f);

  if (orthogonalProjection)
  {
    Point3 camera3;
    camera3.x = normalizedScreen.x / viewport->getProjectionMatrix().m[0][0];
    camera3.y = normalizedScreen.y / viewport->getProjectionMatrix().m[1][1];
    camera3.z = 0.5f * projectionFarPlane;

    world = inverse(viewport->getViewMatrix()) * camera3;

    camera3.z = -0.5f * projectionFarPlane;
    world_dir = inverse(viewport->getViewMatrix()) * camera3;
    world_dir -= world;
  }
  else
  {
    TMatrix camera = inverse(viewport->getViewMatrix());
    world = camera.getcol(3);
    world_dir = camera.getcol(0) * normalizedScreen.x / viewport->getProjectionMatrix()[0][0] +
                camera.getcol(1) * normalizedScreen.y / viewport->getProjectionMatrix()[1][1] + camera.getcol(2);
  }

  world_dir.normalize();
}


bool ViewportWindow::worldToClient(const Point3 &world, Point2 &screen, real *screen_z)
{
  Point3 camera3;
  camera3 = viewport->getViewMatrix() * world;
  Point4 screen4 = Point4(camera3.x, camera3.y, camera3.z, 1.f) * viewport->getProjectionMatrix();

  if (screen4.w != 0.f)
  {
    screen4.w = 1.f / screen4.w;
    screen4.x *= screen4.w;
    screen4.y *= screen4.w;
    screen4.z *= screen4.w;
  }

  if (screen4.w < 0.f)
  {
    screen4.x = -screen4.x;
    screen4.y = -screen4.y;
    screen4.z = -screen4.z;
  }

  EcRect clientRect;
  getClientRect(clientRect);

  bool res = screen4.x >= -1.f && screen4.x <= 1.f && screen4.y >= -1.f && screen4.y <= 1.f && screen4.z >= 0.f;


  screen.x = 0.5f * (clientRect.r - clientRect.l) * (screen4.x + 1.f);
  screen.y = 0.5f * (clientRect.b - clientRect.t) * (-screen4.y + 1.f);
  if (screen_z)
    *screen_z = screen4.z;

  return res;
}


void ViewportWindow::clientToScreen(int &x, int &y) { EcWindow::translateToScreen(x, y); }


void ViewportWindow::screenToClient(int &x, int &y) { EcWindow::translateToClient(x, y); }


void ViewportWindow::redrawClientRect()
{
  if (isVisible())
  {
    invalidateCache();
  }
}


void ViewportWindow::invalidateCache() { ec_cached_viewports->invalidateViewportCache(vpId); }


void ViewportWindow::enableCache(bool en)
{
  if (ec_cached_viewports->isViewportCacheUsed(vpId) && en)
    return;

  ec_cached_viewports->enableViewportCache(vpId, en);
  ec_cached_viewports->invalidateViewportCache(vpId);
}


void ViewportWindow::setViewProj() { viewport->setViewProjTms(); }


void ViewportWindow::OnCameraChanged()
{
  if (curEH)
  {
    ::grs_cur_view.tm = viewport->getViewMatrix();
    ::grs_cur_view.itm = inverse(::grs_cur_view.tm);
    ::grs_cur_view.pos = ::grs_cur_view.itm.getcol(3);
    curEH->handleViewChange(this);
  }
}


void ViewportWindow::drawText(hdpi::Px x, hdpi::Px y, const String &text)
{
  using hdpi::_px;
  using hdpi::_pxS;

  if (opaqueStat3d)
  {
    StdGuiRender::set_color(COLOR_BLACK);
    BBox2 box = StdGuiRender::get_str_bbox(text.c_str(), text.size());
    StdGuiRender::render_box(_px(x) + box[0].x, _px(y) + box[0].y - _pxS(2), _px(x) + box[1].x, _px(y) + box[1].y + _pxS(4));
  }
  else
  {
    StdGuiRender::set_color(COLOR_BLACK);
    StdGuiRender::draw_strf_to(_px(x) + 1, _px(y), text.c_str());
    StdGuiRender::draw_strf_to(_px(x) - 1, _px(y), text.c_str());
    StdGuiRender::draw_strf_to(_px(x), _px(y) + 1, text.c_str());
    StdGuiRender::draw_strf_to(_px(x), _px(y) - 1, text.c_str());
  }

  StdGuiRender::set_color(COLOR_WHITE);
  StdGuiRender::draw_strf_to(_px(x), _px(y), text.c_str());
}


void ViewportWindow::paintRect()
{
  using hdpi::_px;
  using hdpi::_pxS;

  EcRect r;
  getClientRect(r);
  if (isActive())
  {
    StdGuiRender::set_color(COLOR_YELLOW);
    StdGuiRender::render_frame(r.l, r.t, r.r, r.b, 3);
  }

  hdpi::Px rx, ry;
  getMenuAreaSize(rx, ry);
  StdGuiRender::set_color(isActive() ? COLOR_YELLOW : COLOR_BLACK);
  StdGuiRender::render_box(0, 0, _px(rx), _px(ry));

  StdGuiRender::set_font(0);
  Point2 ts = StdGuiRender::get_str_bbox(viewText).size();

  StdGuiRender::set_color(isActive() ? COLOR_BLACK : COLOR_WHITE);
  StdGuiRender::draw_strf_to(int((_px(rx) - ts.x) / 2.f), _pxS(17), viewText);
}


void ViewportWindow::paint(int w, int h)
{
  using hdpi::_px;

  StdGuiRender::reset_per_frame_dynamic_buffer_pos();
  StdGuiRender::start_render(w, h);

  if (curEH)
  {
    ::grs_cur_view.tm = viewport->getViewMatrix();
    ::grs_cur_view.itm = inverse(::grs_cur_view.tm);
    ::grs_cur_view.pos = ::grs_cur_view.itm.getcol(3);
    curEH->handleViewportPaint(this);
  }

  /*
  dc.selectFont ( RESFONT_SSerif );
  dc.selectColor ( isActive() ? PC_Black : PC_White  );
  dc.selectFillColor ( isActive() ? PC_Yellow : PC_Black );
  dc.selectFontAlign ( TF_ALIGNHCENTER|TF_ALIGNVCENTER );
  dc.drawTextS ( 0, 0, 127, 23, viewText );

  if ((currentProjection >= CM_PLUGIN_FIRST_CAMERA && currentProjection < CM_PLUGIN_LAST_CAMERA))
  {
    CtlRect r;
    getClientRect ( r );
    CtlRect clientRect;
    clientRect.l = 0;
    clientRect.r = r.r - r.l;
    clientRect.t = 0;
    clientRect.b = r.b - r.t;

    dc.selectColor ( PC_LtRed );
    CtlRect cameraFrameRect;

    int width = (clientRect.b - clientRect.t) * 4 / 3;
    cameraFrameRect.l = (clientRect.l + clientRect.r - width) / 2;
    cameraFrameRect.r = (clientRect.l + clientRect.r + width) / 2;
    if (cameraFrameRect.l < clientRect.l)
      cameraFrameRect.l = clientRect.l;
    if (cameraFrameRect.r > clientRect.r - 1)
      cameraFrameRect.r = clientRect.r - 1;

    int height = (clientRect.r - clientRect.l) * 3 / 4;
    cameraFrameRect.t = (clientRect.t + clientRect.b - height) / 2;
    cameraFrameRect.b = (clientRect.t + clientRect.b + height) / 2;
    if (cameraFrameRect.t < clientRect.t)
      cameraFrameRect.t = clientRect.t;
    if (cameraFrameRect.b > clientRect.b - 1)
      cameraFrameRect.b = clientRect.b - 1;

    dc.rectangle(cameraFrameRect.l, cameraFrameRect.t, cameraFrameRect.r, cameraFrameRect.b);
    dc.rectangle(cameraFrameRect.l + 1, cameraFrameRect.t + 1, cameraFrameRect.r - 1, cameraFrameRect.b - 1);
  }
  */

  if (rectSelect.active)
    paintSelectionRect();

  paintRect();

  if (showStats && showCameraStats)
  {
    TMatrix tm;
    int _w, _h;
    getCameraTransform(tm);
    getViewportSize(_w, _h);

    hdpi::Px cTextPosX = _pxScaled(5), cTextPosY = _pxActual(_h) - _pxScaled(15); // lower left corner
    const Point3 pos = tm.getcol(3);

    CCameraElem *cameraElem = nullptr;
    switch (CCameraElem::getCamera())
    {
      case CCameraElem::FPS_CAMERA: cameraElem = fpsCameraElem; break;
      case CCameraElem::TPS_CAMERA: cameraElem = tpsCameraElem; break;
      case CCameraElem::CAR_CAMERA: cameraElem = carCameraElem; break;
      case CCameraElem::FREE_CAMERA: cameraElem = freeCameraElem; break;
    }

    if (showCameraTurboSpeed && cameraElem)
    {
      const float turboSpeed = cameraElem->getConfig()->moveStep * cameraElem->getConfig()->controlMultiplier;
      const String cameraTurboSpeed(32, "Camera turbo speed: %.2f", turboSpeed);
      drawText(cTextPosX, cTextPosY, cameraTurboSpeed);

      cTextPosY -= _pxScaled(20);
    }

    if (showCameraSpeed && cameraElem)
    {
      const float speed = cameraElem->getConfig()->moveStep;
      const String cameraSpeed(32, "Camera speed: %.2f", speed);
      drawText(cTextPosX, cTextPosY, cameraSpeed);

      cTextPosY -= _pxScaled(20);
    }

    if (showCameraFov)
    {
      const float fov = getFov() * RAD_TO_DEG;
      const String cameraFov(32, "Camera FOV: %.1f", fov);
      drawText(cTextPosX, cTextPosY, cameraFov);

      cTextPosY -= _pxScaled(20);
    }

    if (showCameraDist)
    {
      const float distance = pos.length();
      const String cameraDistance(32, "Camera dist: %.2f", distance);
      drawText(cTextPosX, cTextPosY, cameraDistance);

      cTextPosY -= _pxScaled(20);
    }

    if (showCameraPos)
    {
      const String cameraPos(32, "Camera pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
      drawText(cTextPosX, cTextPosY, cameraPos);
    }
  }

  StdGuiRender::end_render();
}


void ViewportWindow::handleWindowPaint()
{
  HWND hwnd = (HWND)getHandle();

  PAINTSTRUCT ps;
  BeginPaint(hwnd, &ps);
  EndPaint(hwnd, &ps);
  if (!d3d::is_inited() || ec_cached_viewports->getCurRenderVp() != -1)
    return;

  ec_cached_viewports->invalidateViewportCache(vpId);
  d3d::set_render_target();
  G_VERIFY(ec_cached_viewports->startRender(vpId));
  render_viewport_frame(this);
  ec_cached_viewports->endRender();
  d3d::pcwin32::set_present_wnd(hwnd);
  d3d::update_screen();
  d3d::pcwin32::set_present_wnd(NULL);
}


void ViewportWindow::startRectangularSelection(int mx, int my, int type)
{
  if (rectSelect.active)
    endRectangularSelection(NULL, NULL);

  rectSelect.active = true;
  rectSelect.type = type;
  rectSelect.sel.l = mx;
  rectSelect.sel.t = my;
  rectSelect.sel.r = mx;
  rectSelect.sel.b = my;
}


bool ViewportWindow::endRectangularSelection(EcRect *result, int *type)
{
  if (!rectSelect.active)
    return false;

  rectSelect.active = false;
  if (result)
  {
#define SWAP(a, b) _t = (a), (a) = (b), (b) = _t
    real _t;
    if (rectSelect.sel.l > rectSelect.sel.r)
      SWAP(rectSelect.sel.l, rectSelect.sel.r);
    if (rectSelect.sel.t > rectSelect.sel.b)
      SWAP(rectSelect.sel.t, rectSelect.sel.b);
    *result = rectSelect.sel;
#undef SWAP
  }
  if (type)
    *type = rectSelect.type;

  return true;
}


void ViewportWindow::paintSelectionRect()
{
  if (rectSelect.sel.l == rectSelect.sel.r && rectSelect.sel.t == rectSelect.sel.b)
    return;

  // dc.selectLineStyle ( 0xF0F0F0F0 );
  StdGuiRender::set_color(COLOR_LTRED);
  StdGuiRender::render_frame(rectSelect.sel.l, rectSelect.sel.t, rectSelect.sel.r, rectSelect.sel.b, 1);

  StdGuiRender::set_color(COLOR_LTGREEN);
  StdGuiRender::render_frame(rectSelect.sel.l + 1, rectSelect.sel.t + 1, rectSelect.sel.r - 1, rectSelect.sel.b - 1, 1);
  // dc.selectLineStyle ( 0xFFFFFFFF );
  // dc.selectWriteMode ( WMODE_SET );
}


real ViewportWindow::getLinearSizeSq(const Point3 &pos, real world_rad, int xy)
{
  Point3 x = pos + inverse(viewport->getViewMatrix()).getcol(xy) * world_rad;

  Point2 coord;
  Point2 xs;

  worldToClient(pos, coord);
  worldToClient(x, xs);

  return lengthSq(coord - xs);
}


void ViewportWindow::load(const DataBlock &blk)
{
  orthogonalProjection = blk.getBool("orthogonalProjection", orthogonalProjection);
  projectionFov = blk.getReal("projectionFov", projectionFov);
  projectionFarPlane = blk.getReal("projectionFarPlane", projectionFarPlane);
  projectionNearPlane = blk.getReal("projectionNearPlane", projectionNearPlane);
  orthogonalZoom = blk.getReal("orthogonalZoom", orthogonalZoom);

  const char *projectionName = blk.getStr("currentProjection", NULL);
  if (!projectionName)
    currentProjection = blk.getInt("currentProjection", currentProjection);
  else
  {
    const int projectionId = viewportNameToCommand(projectionName);
    if (projectionId != -1)
    {
      if (projectionId != CM_PLUGIN_FIRST_CAMERA)
        currentProjection = projectionId;
      else
      {
        int customCameraId = blk.getInt("customCameraId", -1);
        if (customCameraId != -1)
          currentProjection = customCameraId + CM_PLUGIN_FIRST_CAMERA;
      }
    }
  }
  setCameraViewText();
  TMatrix viewTm;
  viewTm = viewport->getViewMatrix();
  viewTm.setcol(0, blk.getPoint3("viewMatrix0", viewTm.getcol(0)));
  viewTm.setcol(1, blk.getPoint3("viewMatrix1", viewTm.getcol(1)));
  viewTm.setcol(2, blk.getPoint3("viewMatrix2", viewTm.getcol(2)));
  viewTm.setcol(3, blk.getPoint3("viewMatrix3", viewTm.getcol(3)));
  viewport->setViewMatrix(viewTm);
  viewport->wireframe = blk.getBool("wireframe", viewport->wireframe);

  ::grs_cur_view.tm = viewTm;
  ::grs_cur_view.itm = inverse(::grs_cur_view.tm);
  ::grs_cur_view.pos = ::grs_cur_view.itm.getcol(3);

  showStats = blk.getBool("show_stats", false);
  calcStat3d = blk.getBool("stat3d", true);
  opaqueStat3d = blk.getBool("stat3d_opaque", false);
  showCameraStats = blk.getBool("show_camera_stats", true);
  showCameraPos = blk.getBool("show_camera_pos", true);
  showCameraDist = blk.getBool("show_camera_dist", false);
  showCameraFov = blk.getBool("show_camera_fov", false);
  showCameraSpeed = blk.getBool("show_camera_speed", false);
  showCameraTurboSpeed = blk.getBool("show_camera_turbo_speed", false);
  ec_stat3d_load_enabled_graphs(vpId, blk.getBlockByName("displayed_stat3d_list"));
}


void ViewportWindow::save(DataBlock &blk) const
{
  blk.setBool("orthogonalProjection", orthogonalProjection);
  blk.setReal("projectionFov", projectionFov);
  blk.setReal("projectionFarPlane", projectionFarPlane);
  blk.setReal("projectionNearPlane", projectionNearPlane);
  blk.setReal("orthogonalZoom", orthogonalZoom);

  blk.setStr("currentProjection", viewportCommandToName(currentProjection));
  if ((currentProjection >= CM_PLUGIN_FIRST_CAMERA) && (currentProjection < CM_PLUGIN_LAST_CAMERA))
    blk.setInt("customCameraId", currentProjection - CM_PLUGIN_FIRST_CAMERA);

  blk.setPoint3("viewMatrix0", viewport->getViewMatrix().getcol(0));
  blk.setPoint3("viewMatrix1", viewport->getViewMatrix().getcol(1));
  blk.setPoint3("viewMatrix2", viewport->getViewMatrix().getcol(2));
  blk.setPoint3("viewMatrix3", viewport->getViewMatrix().getcol(3));
  blk.setBool("wireframe", viewport->wireframe);

  blk.setBool("show_stats", showStats);
  blk.setBool("stat3d", calcStat3d);
  blk.setBool("stat3d_opaque", opaqueStat3d);
  blk.setBool("show_camera_stats", showCameraStats);
  blk.setBool("show_camera_pos", showCameraPos);
  blk.setBool("show_camera_dist", showCameraDist);
  blk.setBool("show_camera_fov", showCameraFov);
  blk.setBool("show_camera_speed", showCameraSpeed);
  blk.setBool("show_camera_turbo_speed", showCameraTurboSpeed);
  ec_stat3d_save_enabled_graphs(*blk.addBlock("displayed_stat3d_list"));
}


//===============================================================================
void ViewportWindow::getViewportSize(int &x, int &y)
{
  EcRect rect;
  getClientRect(rect);

  x = rect.r - rect.l;
  y = rect.b - rect.t;
}


//===============================================================================
void ViewportWindow::onWmEmbeddedResize(int width, int height)
{
  EcWindow::resizeWindow(width, height);
  OnChangePosition();
}


//===============================================================================
void ViewportWindow::zoomAndCenter(BBox3 &box)
{
  if (box.isempty())
    return;
  const Point3 width = box.width();

  Point3 vert[8] = {
    box[0],
    box[0] + Point3(width.x, 0, 0),
    box[0] + Point3(width.x, 0, width.z),
    box[0] + Point3(0, 0, width.z),

    box[1],
    box[1] - Point3(width.x, 0, 0),
    box[1] - Point3(width.x, 0, width.z),
    box[1] - Point3(0, 0, width.z),
  };

  int w, h;
  getViewportSize(w, h);

  TMatrix tm;
  getCameraTransform(tm);
  tm = inverse(tm);

  const Point3 dir = Point3(tm.m[0][2], tm.m[1][2], tm.m[2][2]);

  Point3 prev_campos = viewport->getViewMatrix().getcol(3);
  Point3 pos = box.center();
  setCameraPos(pos);

  if (!orthogonalProjection)
  {
    real step = -50;

    for (int i = 0; i < 1000 && fabs(step) > 0.001; ++i)
    {
      bool canSwitch = step < 0;

      for (int i = 0; i < 8; ++i)
      {
        Point2 coord;
        bool result = worldToClient(vert[i], coord);

        if (step < 0)
        {
          if (!result || coord.x < 0 || coord.x > w || coord.y < 0 || coord.y > h)
            canSwitch = false;
        }
        else
        {
          if (!result || coord.x < 0 || coord.x > w || coord.y < 0 || coord.y > h)
            canSwitch = true;
        }
      }

      pos += step * dir;
      setCameraPos(pos);

      if (canSwitch)
        step = -(step / 5);
    }
  }
  else
  {
    real scale = 10;
    bool result = false;
    Point2 vert2[8];
    int i;

    while (scale > 0.00000001)
    {
      setOrthogonalZoom(scale);

      bool canBreak = true;

      for (i = 0; i < 8; ++i)
        canBreak &= worldToClient(vert[i], vert2[i]);

      if (canBreak)
        break;

      scale /= 10;
    }

    Point2 lt = Point2(MAX_REAL, MAX_REAL);
    Point2 rb = Point2(-MAX_REAL, -MAX_REAL);

    for (i = 0; i < 8; ++i)
    {
      if (vert2[i].x < lt.x)
        lt.x = vert2[i].x;

      if (vert2[i].y < lt.y)
        lt.y = vert2[i].y;

      if (rb.x < vert2[i].x)
        rb.x = vert2[i].x;

      if (rb.y < vert2[i].y)
        rb.y = vert2[i].y;
    }

    setOrthogonalZoom(scale * __min(w / (rb.x - lt.x), h / (rb.y - lt.y)));
  }

  Point3 new_campos = viewport->getViewMatrix().getcol(3);
  if (check_nan(new_campos.x) || check_nan(new_campos.y) || check_nan(new_campos.z) || fabsf(new_campos.x) > 1e6 ||
      fabsf(new_campos.y) > 1e6 || fabsf(new_campos.z) > 1e6)
  {
    logwarn("zoomAndCenter gives %@ -> %@, rollback", prev_campos, new_campos);
    setCameraPos(prev_campos);
  }
}


void ViewportWindow::setDragAcceptFiles(bool accept) { DragAcceptFiles((HWND)getHandle(), accept); }


bool ViewportWindow::onDropFiles(const dag::Vector<String> &files) { return false; }


void ViewportWindow::showStatSettingsDialog()
{
  if (!statSettingsDialog)
  {
    statSettingsDialog = new ViewportWindowStatSettingsDialog(*this, _pxScaled(300), _pxScaled(450));

    PropertyContainerControlBase *tab_panel = statSettingsDialog->createTabPanel();
    G_ASSERT(tab_panel);
    fillStatSettingsDialog(*tab_panel);
  }

  if (statSettingsDialog->isVisible())
    statSettingsDialog->hide();
  else
    statSettingsDialog->show();
}


void ViewportWindow::fillStatSettingsDialog(PropertyContainerControlBase &tab_panel)
{
  PropertyContainerControlBase *mainTabPage = tab_panel.createTabPage(CM_STATS_SETTINGS_MAIN_PAGE, "Main");
  mainTabPage->createCheckBox(CM_STATS_SETTINGS_MAIN_SHOW_STAT3D_STATS, "Show stat 3d stats", calcStat3d);
  mainTabPage->createCheckBox(CM_STATS_SETTINGS_MAIN_SHOW_CAMERA_STATS, "Show camera stats", showCameraStats);

  fillStat3dStatSettings(tab_panel);

  PropertyContainerControlBase *tabPage = tab_panel.createTabPage(CM_STATS_SETTINGS_CAMERA_PAGE, "Camera");
  tabPage->createCheckBox(CM_STATS_SETTINGS_CAMERA_POS, "camera pos", showCameraPos);
  tabPage->createCheckBox(CM_STATS_SETTINGS_CAMERA_DIST, "camera dist", showCameraDist);
  tabPage->createCheckBox(CM_STATS_SETTINGS_CAMERA_FOV, "camera FOV", showCameraFov);
  tabPage->createCheckBox(CM_STATS_SETTINGS_CAMERA_SPEED, "camera speed", showCameraSpeed);
  tabPage->createCheckBox(CM_STATS_SETTINGS_CAMERA_TURBO_SPEED, "camera turbo speed", showCameraTurboSpeed);
}


void ViewportWindow::handleStatSettingsDialogChange(int pcb_id)
{
  if (pcb_id == CM_STATS_SETTINGS_MAIN_SHOW_STAT3D_STATS)
    calcStat3d = !calcStat3d;
  else if (pcb_id == CM_STATS_SETTINGS_MAIN_SHOW_CAMERA_STATS)
    showCameraStats = !showCameraStats;
  else if (pcb_id == CM_STATS_SETTINGS_CAMERA_POS)
    showCameraPos = !showCameraPos;
  else if (pcb_id == CM_STATS_SETTINGS_CAMERA_DIST)
    showCameraDist = !showCameraDist;
  else if (pcb_id == CM_STATS_SETTINGS_CAMERA_FOV)
    showCameraFov = !showCameraFov;
  else if (pcb_id == CM_STATS_SETTINGS_CAMERA_SPEED)
    showCameraSpeed = !showCameraSpeed;
  else if (pcb_id == CM_STATS_SETTINGS_CAMERA_TURBO_SPEED)
    showCameraTurboSpeed = !showCameraTurboSpeed;
  else
    handleStat3dStatSettingsDialogChange(pcb_id);
}

TMatrix ViewportWindow::getViewTm() const { return viewport->getViewMatrix(); }

TMatrix4 ViewportWindow::getProjTm() const { return viewport->getProjectionMatrix(); }

void save_camera_objects(DataBlock &blk)
{
  freeCameraElem->save(*blk.addBlock("freeCamera"));
  maxCameraElem->save(*blk.addBlock("maxCamera"));
  fpsCameraElem->save(*blk.addBlock("fpsCamera"));
  tpsCameraElem->save(*blk.addBlock("tpsCamera"));
  carCameraElem->save(*blk.addBlock("carCamera"));
}


void load_camera_objects(const DataBlock &blk)
{
  freeCameraElem->load(*blk.getBlockByNameEx("freeCamera"));
  maxCameraElem->load(*blk.getBlockByNameEx("maxCamera"));
  fpsCameraElem->load(*blk.getBlockByNameEx("fpsCamera"));
  tpsCameraElem->load(*blk.getBlockByNameEx("tpsCamera"));
  carCameraElem->load(*blk.getBlockByNameEx("carCamera"));
}

void show_camera_objects_config_dialog(void *parent)
{
  CCameraElem::showConfigDlg(parent, maxCameraElem->getConfig(), freeCameraElem->getConfig(), fpsCameraElem->getConfig(),
    tpsCameraElem->getConfig());
}

CachedRenderViewports *ec_cached_viewports = NULL;
