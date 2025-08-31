// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "ec_cachedRender.h"
#include "ec_stat3d.h"

#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_ViewportWindowStatSettingsDialog.h>
#include <EditorCore/ec_GizmoSettingsDialog.h>
#include <EditorCore/ec_gizmofilter.h>
#include <EditorCore/ec_camera_dlg.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_input.h>
#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/captureCursor.h>
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_wndPublic.h>
#include "ec_viewportAxis.h"

#include <gui/dag_stdGuiRenderEx.h>

#include <libTools/renderViewports/renderViewport.h>

#include <3d/dag_render.h>
#include <3d/dag_lockTexture.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_lock.h>
#include <perfMon/dag_statDrv.h>
#include <render/dag_cur_view.h>
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_texture.h>

#include <stdio.h>
#include <stdlib.h>

#include <generic/dag_initOnDemand.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameScene.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>
#include <gui/dag_baseCursor.h>
#include <math/dag_easingFunctions.h>
#include <scene/dag_visibility.h>

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <propPanel/focusHelper.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#if _TARGET_PC_WIN
#include <windows.h>
#else
#include <X11/Xlib.h>
#undef None

namespace workcycle_internal
{
const XWindowAttributes &get_window_attrib(Window w, bool translated);
}
#endif

using hdpi::_pxActual;
using hdpi::_pxScaled;
using PropPanel::ROOT_MENU_ITEM;

static real def_viewport_zn = 0.1f, def_viewport_zf = 1000.f;
static real def_viewport_fov = 1.57f;
static eastl::unique_ptr<CamerasConfigDlg> camera_config_dialog;

static const real FALLBACK_DIST_CAM_PAN = 750.f;

bool camera_objects_config_changed = false;

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

static TMatrix createViewMatrix(const Point3 &forward, const Point3 &up)
{
  const Point3 right = normalize(up % forward);

  TMatrix tm = TMatrix::IDENT;
  tm.m[0][0] = right.x;
  tm.m[1][0] = right.y;
  tm.m[2][0] = right.z;

  tm.m[0][1] = up.x;
  tm.m[1][1] = up.y;
  tm.m[2][1] = up.z;

  tm.m[0][2] = forward.x;
  tm.m[1][2] = forward.y;
  tm.m[2][2] = forward.z;
  return tm;
}

static bool add_viewport_canvas_item(ImGuiID id, ImTextureID texture_id, const ImVec2 &image_size, float item_spacing, bool vr_mode)
{
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + image_size);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(item_spacing, item_spacing));
  ImGui::ItemSize(bb);
  ImGui::PopStyleVar();
  if (!ImGui::ItemAdd(bb, id))
    return false;

  bool held = false;
  ImGui::ButtonBehavior(bb, id, nullptr, &held,
    ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);

  if (!vr_mode)
    window->DrawList->AddImage(texture_id, bb.Min, bb.Max);

  return held;
}

static int get_mouse_buttons_for_window_messages()
{
  int wparam = 0;
  if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    wparam |= EC_MK_LBUTTON;
  if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    wparam |= EC_MK_RBUTTON;
  if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
    wparam |= EC_MK_MBUTTON;
  return wparam;
}

static int get_modifier_keys_for_window_messages()
{
  int result = 0;
  const ImGuiIO &io = ImGui::GetIO();
  if (io.KeyMods & ImGuiMod_Alt)
    result |= wingw::M_ALT;
  if (io.KeyMods & ImGuiMod_Shift)
    result |= wingw::M_SHIFT;
  if (io.KeyMods & ImGuiMod_Ctrl)
    result |= wingw::M_CTRL;
  return result;
}

Tab<ViewportParams> ViewportWindow::viewportsParams(midmem);

class ViewportWindow::ViewportClippingDlg : public PropPanel::DialogWindow
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

namespace
{
bool tryReadingDepthFromBuffer(int x, int y, BaseTexture *depth_buffer, real &depth)
{
  static_assert(sizeof(real) == 4, "Reading from D32 texture with type != 4 bytes");

  if (depth_buffer)
  {
    TextureInfo texInfo;
    depth_buffer->getinfo(texInfo);
    if (texInfo.cflg & TEXFMT_DEPTH32)
    {
      if (x >= texInfo.w || y >= texInfo.h)
      {
        return false;
      }

      if (auto tex = lock_texture<const real>(depth_buffer, 0, TEXLOCK_READ))
      {
        depth = tex.at(x, y);
        return true;
      }
    }
  }

  return false;
}
} // namespace


ViewportWindow::ViewportClippingDlg::ViewportClippingDlg(float z_near, float z_far) :
  DialogWindow(0, _pxScaled(240), _pxScaled(135), "Viewport clipping")
{
  PropPanel::ContainerPropertyControl *_panel = getPanel();
  G_ASSERT(_panel && "No panel in ViewportWindow::ViewportClippingDlg");

  _panel->createEditFloat(Z_NEAR_ID, "Near z-plane:", z_near, 3);
  _panel->createEditFloat(Z_FAR_ID, "Far z-plane:", z_far, 0);
}


real ViewportWindow::ViewportClippingDlg::get_z_near()
{
  PropPanel::ContainerPropertyControl *_panel = getPanel();
  G_ASSERT(_panel && "No panel in ViewportWindow::ViewportClippingDlg");

  return _panel->getFloat(Z_NEAR_ID);
}

real ViewportWindow::ViewportClippingDlg::get_z_far()
{
  PropPanel::ContainerPropertyControl *_panel = getPanel();
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
GridEditDialog *ViewportWindow::gridSettingsDialog = nullptr;
GizmoSettingsDialog *ViewportWindow::gizmoSettingsDialog = nullptr;


ViewportWindow::ViewportWindow()
{
  showStats = false;
  calcStat3d = true;
  opaqueStat3d = false;
  curEH = NULL;
  curMEH = nullptr;
  prevMousePositionX = 0;
  prevMousePositionY = 0;
  isMoveRotateAllowed = false;
  isXLocked = false;
  isYLocked = false;
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
  popupMenu = nullptr;
  selectionMenu = nullptr;
  allowPopupMenu = false;
  mIsCursorVisible = true;
  showCameraStats = true;
  showCameraPos = true;
  showCameraDist = false;
  showCameraFov = false;
  showCameraSpeed = false;
  showCameraTurboSpeed = false;
  statSettingsDialog = nullptr;
  wireframeOverlay = false;
  showViewportAxis = true;
  highlightedViewportAxisId = ViewportAxisId::None;
  mouseDownOnViewportAxis = ViewportAxisId::None;

  G_STATIC_ASSERT(mouseButtonDownArraySize == ImGuiMouseButton_COUNT);
  memset(mouseButtonDown, 0, sizeof(mouseButtonDown));

  viewport = new (midmem) RenderViewport;
  viewport->setPerspHFov(1.3, 0.01, 1000.0);

  if (!ec_cached_viewports)
    ec_cached_viewports = new (inimem) CachedRenderViewports;

  vpId = ec_cached_viewports->addViewport(viewport, this, false);
}


ViewportWindow::~ViewportWindow()
{
  PropPanel::remove_delayed_callback(*this);
  clear_all_ptr_items(delayedMouseEvents);

  del_it(popupMenu);
  del_it(selectionMenu);
  del_it(statSettingsDialog);
  del_it(gridSettingsDialog);
  del_it(gizmoSettingsDialog);
  if (ec_cached_viewports)
    ec_cached_viewports->removeViewport(vpId);
  del_it(viewport);
  vpId = -1;
}


void ViewportWindow::init(IGenEventHandler *eh)
{
  setEventHandler(eh);
  OnChangePosition();
}


void ViewportWindow::setEventHandler(IGenEventHandler *eh) { curEH = eh; }


void ViewportWindow::getMenuAreaSize(hdpi::Px &w, hdpi::Px &h)
{
  w = _pxScaled(MENU_AREA_SIZE_W);
  h = _pxScaled(MENU_AREA_SIZE_H);
}


void ViewportWindow::fillPopupMenu(PropPanel::IMenu &menu)
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

  menu.addSeparator(ROOT_MENU_ITEM);
  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_WIREFRAME, "Wireframe, Edged faces\tF3");
  menu.setCheckById(CM_VIEW_WIREFRAME, viewport && (viewport->wireframe || wireframeOverlay));

  {
    menu.addSubMenu(ROOT_MENU_ITEM, CM_GROUP_GRID, "Grid");

    menu.addItem(CM_GROUP_GRID, CM_VIEW_GRID_SHOW, "Show grid\tG");
    menu.addItem(CM_GROUP_GRID, CM_OPTIONS_GRID, "Grid settings...");

    menu.setCheckById(CM_VIEW_GRID_SHOW, EDITORCORE->getGrid().isVisible(vpId));
  }

  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_GIZMO, "Gizmos");

  menu.addItem(ROOT_MENU_ITEM, CM_VIEW_VIEWPORT_AXIS, "Viewport rotation gizmo");
  menu.setCheckById(CM_VIEW_VIEWPORT_AXIS, showViewportAxis);

  menu.addSeparator(ROOT_MENU_ITEM);

  {
    menu.addSubMenu(ROOT_MENU_ITEM, CM_GROUP_STATS, "Stats");

    menu.addItem(CM_GROUP_STATS, CM_VIEW_SHOW_STATS, "Show stats");
    menu.setCheckById(CM_VIEW_SHOW_STATS, showStats);

    menu.addItem(CM_GROUP_STATS, CM_OPTIONS_STAT_DISPLAY_SETTINGS, "Stats settings...");

    menu.addItem(CM_GROUP_STATS, CM_VIEW_STAT3D_OPAQUE, "Opaque stats");
    menu.setCheckById(CM_VIEW_STAT3D_OPAQUE, opaqueStat3d);
  }

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
      menu.addSeparator(ROOT_MENU_ITEM);

      menu.addItem(ROOT_MENU_ITEM, CM_SAVE_ACTIVE_VIEW, saveStr.str());
      menu.addItem(ROOT_MENU_ITEM, CM_RESTORE_ACTIVE_VIEW, restoreStr.str());
      menu.setEnabledById(CM_RESTORE_ACTIVE_VIEW, restoreFlags & (1 << vpId));
    }

    menu.addSeparator(ROOT_MENU_ITEM);

    menu.addSubMenu(ROOT_MENU_ITEM, CM_GROUP_CLIPPING, "Viewport clipping");
    menu.addItem(CM_GROUP_CLIPPING, CM_SET_VIEWPORT_CLIPPING, "Set");
    menu.addItem(CM_GROUP_CLIPPING, CM_SET_DEFAULT_VIEWPORT_CLIPPING, "Reset to default");
  }
}


void ViewportWindow::setMenuEventHandler(PropPanel::IMenuEventHandler *meh) { curMEH = meh; }


void ViewportWindow::processCameraMouseMove(CCameraElem *camera_elem, int mouse_x, int mouse_y)
{
  G_ASSERT(camera_elem && "ViewportWindow::processCameraMouseMove camera_elem is NULL!!!");

  if (!isActive())
    return;

  real deltaX, deltaY;
  processMouseMoveInfluence(deltaX, deltaY, mouse_x, mouse_y);
  camera_elem->handleMouseMove(-deltaX, -deltaY);
  ::cursor_wrap(mouse_x, mouse_y, getMainHwnd());
  prevMousePositionX = mouse_x;
  prevMousePositionY = mouse_y;
  updatePluginCamera = true;
}


void ViewportWindow::processCameraMouseWheel(CCameraElem *camera_elem, int delta)
{
  G_ASSERT(camera_elem && "ViewportWindow::processCameraMouseWheel camera_elem is NULL!!!");

  camera_elem->handleMouseWheel(delta);
}


void ViewportWindow::processMaxCameraMouseMButtonDown(int mouse_x, int mouse_y)
{
  prevMousePositionX = mouse_x;
  prevMousePositionY = mouse_y;
  isMoveRotateAllowed = true;
  isXLocked = false;
  isYLocked = false;

  if (CCameraElem::getCamera() != CCameraElem::MAX_CAMERA || !maxCameraElem)
    return;

  capture_cursor(getMainHwnd());
}


void ViewportWindow::processMaxCameraMouseMButtonUp()
{
  isMoveRotateAllowed = false;

  if (CCameraElem::getCamera() != CCameraElem::MAX_CAMERA || !maxCameraElem)
    return;

  release_cursor();
}


void ViewportWindow::processMaxCameraMouseMove(int mouse_x, int mouse_y, bool m_button_down)
{
  if (CCameraElem::getCamera() != CCameraElem::MAX_CAMERA || !maxCameraElem)
    return;

  const bool ctrlPressed = ec_is_ctrl_key_down();
  const bool altPressed = ec_is_alt_key_down();

  maxCameraElem->setMultiply(ctrlPressed);
  const real multiplier = ctrlPressed ? maxCameraElem->getMultiplier() : 1;

  if (m_button_down && isMoveRotateAllowed)
  {
    real deltaX, deltaY;

    processMouseMoveInfluence(deltaX, deltaY, mouse_x, mouse_y);
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

        const Point3 pos = getCameraPanAnchorPoint();
        real sx = sqrtf(getLinearSizeSq(pos, 1, 0));
        real sy = sqrtf(getLinearSizeSq(pos, 1, 1));

        deltaX /= ::dagor_game_act_time;
        deltaY /= ::dagor_game_act_time;

        deltaX /= sx;
        deltaY /= sy;

        maxCameraElem->strife(deltaX, deltaY, false, false);
      }
    }

    updatePluginCamera = true;
    ::cursor_wrap(mouse_x, mouse_y, getMainHwnd());
  }
  prevMousePositionX = mouse_x;
  prevMousePositionY = mouse_y;
}


void ViewportWindow::processMaxCameraMouseWheel(int multiplied_delta)
{
  if (CCameraElem::getCamera() != CCameraElem::MAX_CAMERA || !maxCameraElem)
    return;

  const bool ctrlPressed = ec_is_ctrl_key_down();

  maxCameraElem->setMultiply(ctrlPressed);
  const real multiplier = ctrlPressed ? maxCameraElem->getMultiplier() : 1;

  if (orthogonalProjection)
  {
    float _zoom = (1.f + 0.1f * multiplier);
    setOrthogonalZoom((multiplied_delta < 0) && (_zoom != 0) ? orthogonalZoom / _zoom : orthogonalZoom * _zoom);
  }
  else if (multiplied_delta != 0)
    maxCameraElem->moveForward(1.0 * multiplied_delta / abs(multiplied_delta) * ::dagor_game_act_time, true, this);

  updatePluginCamera = true;
}


void ViewportWindow::processMouseLButtonPress(int mouse_x, int mouse_y)
{
  mouseButtonDown[ImGuiMouseButton_Left] = true;
  input.lmbPressed = true;

  if (highlightedViewportAxisId != ViewportAxisId::None && canStartInteractionWithViewport())
  {
    handleViewportAxisMouseLButtonDown();
    return;
  }

  if (!canRouteMessagesToExternalEventHandler())
    return;

  captureMouse();
  curEH->handleMouseLBPress(this, mouse_x, mouse_y, true, get_mouse_buttons_for_window_messages(),
    get_modifier_keys_for_window_messages());
}


void ViewportWindow::processMouseLButtonRelease(int mouse_x, int mouse_y)
{
  mouseButtonDown[ImGuiMouseButton_Left] = false;
  input.lmbPressed = false;

  if (mouseDownOnViewportAxis != ViewportAxisId::None)
  {
    handleViewportAxisMouseLButtonUp();
    return;
  }

  if (!canRouteMessagesToExternalEventHandler())
    return;

  releaseMouse();

  // Some edit functions show a modal dialog in response to the mouse button release (for example the object
  // cloning with the move gizmo), so to simplify the edit functions we always send this event delayed.
  DelayedMouseEvent *delayedMouseEvent = new DelayedMouseEvent();
  delayedMouseEvent->x = mouse_x;
  delayedMouseEvent->y = mouse_y;
  delayedMouseEvent->inside = true;
  delayedMouseEvent->buttons = get_mouse_buttons_for_window_messages();
  delayedMouseEvent->modifierKeys = get_modifier_keys_for_window_messages();
  delayedMouseEvents.push_back(delayedMouseEvent);
  PropPanel::request_delayed_callback(*this, delayedMouseEvent);
}


void ViewportWindow::processMouseLButtonDoubleClick(int mouse_x, int mouse_y)
{
  if (canRouteMessagesToExternalEventHandler())
    curEH->handleMouseDoubleClick(this, mouse_x, mouse_y, get_modifier_keys_for_window_messages());
}


void ViewportWindow::processMouseMButtonPress(int mouse_x, int mouse_y)
{
  mouseButtonDown[ImGuiMouseButton_Middle] = true;

  if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
    processMaxCameraMouseMButtonDown(mouse_x, mouse_y);
}


void ViewportWindow::processMouseMButtonRelease(int mouse_x, int mouse_y)
{
  mouseButtonDown[ImGuiMouseButton_Middle] = false;

  if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
  {
    processMaxCameraMouseMButtonUp();
    cameraPanAnchorPoint.reset();
  }
}


void ViewportWindow::processMouseRButtonPress(int mouse_x, int mouse_y)
{
  mouseButtonDown[ImGuiMouseButton_Right] = true;
  input.rmbPressed = true;
  allowPopupMenu = false;

  if (!canRouteMessagesToExternalEventHandler())
    return;

  allowPopupMenu = canStartInteractionWithViewport();
  if (!pointInMenuArea(this, mouse_x, mouse_y))
  {
    captureMouse();
    if (curEH->handleMouseRBPress(this, mouse_x, mouse_y, true, get_mouse_buttons_for_window_messages(),
          get_modifier_keys_for_window_messages()))
      allowPopupMenu = false;
  }
}


void ViewportWindow::processMouseRButtonRelease(int mouse_x, int mouse_y)
{
  // call popup menu
  mouseButtonDown[ImGuiMouseButton_Right] = false;
  input.rmbPressed = false;

  if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA && allowPopupMenu && !popupMenu && pointInMenuArea(this, mouse_x, mouse_y))
  {
    popupMenu = PropPanel::create_context_menu();
    fillPopupMenu(*popupMenu);
    allowPopupMenu = false; // Prevent opening the selection context menu.
  }

  if (!canRouteMessagesToExternalEventHandler())
    return;

  if (!selectionMenu && allowPopupMenu)
  {
    selectionMenu = PropPanel::create_context_menu();
    selectionMenu->setEventHandler(this);
  }

  releaseMouse();
  curEH->handleMouseRBRelease(this, mouse_x, mouse_y, true, get_mouse_buttons_for_window_messages(),
    get_modifier_keys_for_window_messages());
}


void ViewportWindow::processMouseMove(int mouse_x, int mouse_y)
{
  input.mouseX = mouse_x;
  input.mouseY = mouse_y;

  if (mouseDownOnViewportAxis == ViewportAxisId::RotatorCircle)
  {
    processViewportAxisCameraRotation(mouse_x, mouse_y);
    return;
  }

  switch (CCameraElem::getCamera())
  {
    case CCameraElem::FPS_CAMERA: processCameraMouseMove(fpsCameraElem, mouse_x, mouse_y); break;

    case CCameraElem::TPS_CAMERA: processCameraMouseMove(tpsCameraElem, mouse_x, mouse_y); break;

    case CCameraElem::CAR_CAMERA: processCameraMouseMove(carCameraElem, mouse_x, mouse_y); break;

    case CCameraElem::FREE_CAMERA: processCameraMouseMove(freeCameraElem, mouse_x, mouse_y); break;

    case CCameraElem::MAX_CAMERA:
      if (maxCameraElem)
        maxCameraElem->setMultiply(ec_is_ctrl_key_down());

      processMaxCameraMouseMove(mouse_x, mouse_y, ImGui::IsMouseDown(ImGuiMouseButton_Middle));
      break;
  }

  if (canRouteMessagesToExternalEventHandler())
    curEH->handleMouseMove(this, mouse_x, mouse_y, true, get_mouse_buttons_for_window_messages(),
      get_modifier_keys_for_window_messages());

  if (rectSelect.active)
  {
    if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
      processRectangularSelectionMouseMove(mouse_x, mouse_y);
    else
      rectSelect.active = false;
  }
}


void ViewportWindow::processMouseWheel(int mouse_x, int mouse_y, int multiplied_delta)
{
  switch (CCameraElem::getCamera())
  {
    case CCameraElem::FPS_CAMERA: processCameraMouseWheel(fpsCameraElem, multiplied_delta / EC_WHEEL_DELTA); break;

    case CCameraElem::TPS_CAMERA: processCameraMouseWheel(tpsCameraElem, multiplied_delta / EC_WHEEL_DELTA); break;

    case CCameraElem::CAR_CAMERA: processCameraMouseWheel(carCameraElem, multiplied_delta / EC_WHEEL_DELTA); break;

    case CCameraElem::FREE_CAMERA: processCameraMouseWheel(freeCameraElem, multiplied_delta / EC_WHEEL_DELTA); break;

    case CCameraElem::MAX_CAMERA:
    {
      if (maxCameraElem)
        maxCameraElem->setMultiply(ec_is_ctrl_key_down());

      processMaxCameraMouseWheel(multiplied_delta);
    }
  }

  if (canRouteMessagesToExternalEventHandler())
    curEH->handleMouseWheel(this, multiplied_delta / EC_WHEEL_DELTA, mouse_x, mouse_y, get_modifier_keys_for_window_messages());
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

    case CM_VIEW_VIEWPORT_AXIS:
      showViewportAxis = !showViewportAxis;
      redrawClientRect();
      return 0;

    case CM_SET_VIEWPORT_CLIPPING:
    {
      ViewportClippingDlg dialog(projectionNearPlane, projectionFarPlane);
      if (dialog.showDialog() != PropPanel::DIALOG_ID_OK)
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

    case CM_OPTIONS_GRID: showGridSettingsDialog(); return 0;

    case CM_VIEW_GRID_SHOW:
    {
      bool is_visible = EDITORCORE->getGrid().isVisible(vpId);
      EDITORCORE->getGrid().setVisible(!is_visible, vpId);
      if (gridSettingsDialog)
        gridSettingsDialog->onGridVisibilityChanged(vpId);
    }
      return 0;

    case CM_VIEW_GIZMO: showGizmoSettingsDialog(); return 0;

    case CM_VIEW_SHOW_STATS: showStats = !showStats; return 0;

    case CM_VIEW_STAT3D_OPAQUE: opaqueStat3d = !opaqueStat3d; return 0;

    case CM_OPTIONS_STAT_DISPLAY_SETTINGS: showStatSettingsDialog(); return 0;

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
    default:
      if (curMEH)
        return curMEH->onMenuItemClick(id);
      break;
  }

  return 1;
}


int ViewportWindow::handleCommand(int p1, int p2, int p3)
{
  bool ctrlPressed = ec_is_ctrl_key_down();
  bool shiftPressed = ec_is_shift_key_down();

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

      if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
        restoreCursorAt = ec_get_cursor_pos();

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
            fpsCameraElem->setAboveSurface(true);
          if (CCameraElem::getCamera() == CCameraElem::TPS_CAMERA)
            tpsCameraElem->setAboveSurface(true);
          if (CCameraElem::getCamera() == CCameraElem::CAR_CAMERA)
          {
            carCameraElem->setAboveSurface(true);
            if (!carCameraElem->begin())
            {
              this->handleCommand(CM_CAMERAS_FREE, p2, p3);
              return 0;
            }
          }

          capture_cursor(getMainHwnd());
          if (mIsCursorVisible)
          {
            ec_show_cursor(false);
            mIsCursorVisible = false;
          }
        }
        break;

        case CCameraElem::MAX_CAMERA:
          release_cursor();
          if (!mIsCursorVisible)
          {
            ec_show_cursor(true);
            mIsCursorVisible = true;
          }
          ec_set_cursor_pos(restoreCursorAt);
          break;

        default: DAG_FATAL("Unknown camera type. Contact developers.");
      }

      break;
  }

  return 0;
}

void ViewportWindow::registerViewportAccelerators(IWndManager &wnd_manager)
{
  wnd_manager.addViewportAccelerator(CM_VIEW_GRID_SHOW, ImGuiKey_G);
  wnd_manager.addViewportAccelerator(CM_VIEW_WIREFRAME, ImGuiKey_F3);
  wnd_manager.addViewportAccelerator(CM_VIEW_PERSPECTIVE, ImGuiMod_Shift | ImGuiKey_P);
  wnd_manager.addViewportAccelerator(CM_VIEW_FRONT, ImGuiMod_Shift | ImGuiKey_F);
  wnd_manager.addViewportAccelerator(CM_VIEW_BACK, ImGuiMod_Shift | ImGuiKey_K);
  wnd_manager.addViewportAccelerator(CM_VIEW_TOP, ImGuiMod_Shift | ImGuiKey_T);
  wnd_manager.addViewportAccelerator(CM_VIEW_BOTTOM, ImGuiMod_Shift | ImGuiKey_B);
  wnd_manager.addViewportAccelerator(CM_VIEW_LEFT, ImGuiMod_Shift | ImGuiKey_L);
  wnd_manager.addViewportAccelerator(CM_VIEW_RIGHT, ImGuiMod_Shift | ImGuiKey_R);
}

bool ViewportWindow::handleViewportAcceleratorCommand(unsigned id) { return onMenuItemClick(id) == 0; }

void ViewportWindow::OnChangePosition()
{
  EcRect clientRect;
  getClientRect(clientRect);
  if (clientRect.width() <= 0 || clientRect.height() <= 0)
    return;

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


int ViewportWindow::getW() const { return screenshotSize ? screenshotSize->x : viewportTextureSize.x; }


int ViewportWindow::getH() const { return screenshotSize ? screenshotSize->y : viewportTextureSize.y; }


void ViewportWindow::getClientRect(EcRect &clientRect) const
{
  clientRect.l = 0;
  clientRect.t = 0;
  clientRect.r = screenshotSize ? screenshotSize->x : viewportTextureSize.x;
  clientRect.b = screenshotSize ? screenshotSize->y : viewportTextureSize.y;
}


void ViewportWindow::captureMouse() { capture_cursor(getMainHwnd()); }


void ViewportWindow::releaseMouse() { release_cursor(); }


void ViewportWindow::activate()
{
  if (active)
    return;

  // Instantly mark as active (focused) and request focus from ImGui (it will be fulfilled in the next frame). Marking
  // it instantly is needed because at some places after calling activate() isActive() is used instantly.
  active = true;
  PropPanel::focus_helper.requestFocus(this);

  // Unfortunately PropPanel::focus_helper only applies the focus in the next frame. That is a problem because for
  // example when opening a dialog from the toolbar the viewport must be already the top level window before the dialog
  // opens to ensure that ImGui returns the focus back to the viewport after closing the dialog. So this immediate
  // bringing to the top is needed for activate() uses like in ObjectEditor::onClick().
  ImGuiWindow *window = ImGui::GetCurrentContext() ? ImGui::FindWindowByName("Viewport") : nullptr;
  if (window)
    ImGui::FocusWindow(window);

  OnChangeState();
}


bool ViewportWindow::isActive() { return active; }


void ViewportWindow::OnChangeState()
{
  if (isActive())
  {
    CCameraElem::setViewportWindow(this);
  }

  // Invalidate the previous mouse position so when switching to another application, moving the mouse, and then
  // switching back does not cause a camera jump in fly mode.
  prevMousePositionX = prevMousePositionY = INT32_MIN;
}


void ViewportWindow::processMouseMoveInfluence(real &deltaX, real &deltaY, int mouse_x, int mouse_y)
{
  if (prevMousePositionX == INT32_MIN && prevMousePositionY == INT32_MIN)
  {
    deltaX = deltaY = 0;
    return;
  }

  deltaX = (mouse_x - prevMousePositionX);
  deltaY = (mouse_y - prevMousePositionY);

  if (abs(deltaX) > getW() * 3 / 4)
    deltaX = 0;
  if (abs(deltaY) > getH() * 3 / 4)
    deltaY = 0;

  if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA && ec_is_shift_key_down())
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


void ViewportWindow::processRectangularSelectionMouseMove(int mouse_x, int mouse_y)
{
  if (!ec_is_key_down(ImGuiKey_MouseLeft))
    return;

  rectSelect.sel.r = mouse_x;
  rectSelect.sel.b = mouse_y;
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
  TMatrix viewMatrix = createViewMatrix(forward, up);
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


void ViewportWindow::act(real dt)
{
  resizeViewportTexture();

  if (cameraTransitioning)
  {
    // Only continue the transition if the camera has not changed (for example by the user rotating the camera).
    if (viewport->getViewMatrix() == cameraTransitionLastViewMatrix)
    {
      const float transitionDurationInSec = 0.5f;

      cameraTransitionElapsedTime += dt;
      float transition = cameraTransitionElapsedTime / transitionDurationInSec;
      if (transition >= 1.0f)
      {
        transition = 1.0f;
        cameraTransitioning = false;
      }

      const Quat quat = qinterp(cameraTransitionStartQuaternion, cameraTransitionEndQuaternion, inOutQuad(transition));
      cameraTransitionLastViewMatrix = makeTM(quat);
      cameraTransitionLastViewMatrix.setcol(3, viewport->getViewMatrix().getcol(3));
      viewport->setViewMatrix(cameraTransitionLastViewMatrix);
    }
    else
    {
      cameraTransitioning = false;
    }
  }

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

void ViewportWindow::worldToNDC(const Point3 &world, Point3 &ndc) const
{
  Point3 vertexVS(viewport->getViewMatrix() * world);
  Point4 clipSpace = Point4(vertexVS.x, vertexVS.y, vertexVS.z, 1.f) * viewport->getProjectionMatrix();
  if (clipSpace.w != 0.f)
  {
    clipSpace.w = 1.f / clipSpace.w;
    clipSpace.x = clipSpace.x * clipSpace.w;
    clipSpace.y = clipSpace.y * clipSpace.w;
    clipSpace.z = clipSpace.z * clipSpace.w;

    if (clipSpace.w < 0.f)
    {
      clipSpace.x = -clipSpace.x;
      clipSpace.y = -clipSpace.y;
      clipSpace.z = -clipSpace.z;
    }
  }
  ndc.set(clipSpace.x, clipSpace.y, clipSpace.z);
}

bool ViewportWindow::worldToClient(const Point3 &world, Point2 &screen, real *screen_z)
{
  Point3 screen3;
  worldToNDC(world, screen3);

  EcRect clientRect;
  getClientRect(clientRect);

  bool res = screen3.x >= -1.f && screen3.x <= 1.f && screen3.y >= -1.f && screen3.y <= 1.f && screen3.z >= 0.f;

  screen.x = 0.5f * (clientRect.r - clientRect.l) * (screen3.x + 1.f);
  screen.y = 0.5f * (clientRect.b - clientRect.t) * (-screen3.y + 1.f);
  if (screen_z)
    *screen_z = screen3.z;

  return res;
}


void ViewportWindow::clientToScreen(int &x, int &y)
{
#if _TARGET_PC_WIN
  POINT point = {x, y};
  ClientToScreen((HWND)getMainHwnd(), &point);

  x = point.x;
  y = point.y;
#else
  // TODO: tools Linux porting: multi viewport: clientToScreen. Use ImGui's ImGuiViewport! Change the Windows version
  // too! Also use mousePosInCanvas to have proper screen coordinates (in the Windows version too).
  G_ASSERT((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0);

  Window window = (intptr_t)win32_get_main_wnd(); // see getWindowFromPtrHandle()
  const XWindowAttributes &attributes = workcycle_internal::get_window_attrib(window, true);
  x += attributes.x;
  y += attributes.y;
#endif
}


void ViewportWindow::screenToClient(int &x, int &y)
{
#if _TARGET_PC_WIN
  POINT point = {x, y};
  ScreenToClient((HWND)getMainHwnd(), &point);

  x = point.x;
  y = point.y;
#else
  // TODO: tools Linux porting: multi viewport: screenToClient. Use ImGui's ImGuiViewport! Change the Windows version
  // too! Also use mousePosInCanvas to have proper screen coordinates (in the Windows version too).
  G_ASSERT((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0);

  Window window = (intptr_t)win32_get_main_wnd(); // see getWindowFromPtrHandle()
  const XWindowAttributes &attributes = workcycle_internal::get_window_attrib(window, true);
  x -= attributes.x;
  y -= attributes.y;
#endif
}


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


void ViewportWindow::drawText(int x, int y, const String &text)
{
  using hdpi::_px;
  using hdpi::_pxS;

  if (opaqueStat3d)
  {
    StdGuiRender::set_color(COLOR_BLACK);
    BBox2 box = StdGuiRender::get_str_bbox(text.c_str(), text.size());
    StdGuiRender::render_box(x + box[0].x, y + box[0].y - _pxS(2), x + box[1].x, y + box[1].y + _pxS(4));
  }
  else
  {
    StdGuiRender::set_color(COLOR_BLACK);
    StdGuiRender::draw_strf_to(x + 1, y, text.c_str());
    StdGuiRender::draw_strf_to(x - 1, y, text.c_str());
    StdGuiRender::draw_strf_to(x, y + 1, text.c_str());
    StdGuiRender::draw_strf_to(x, y - 1, text.c_str());
  }

  StdGuiRender::set_color(COLOR_WHITE);
  StdGuiRender::draw_strf_to(x, y, text.c_str());
}


void ViewportWindow::drawText(hdpi::Px x, hdpi::Px y, const String &text)
{
  using hdpi::_px;
  drawText(_px(x), _px(y), text);
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
  TIME_D3D_PROFILE(ViewportWindow_paint);

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

  if (showViewportAxis)
  {
    IPoint2 viewportSize;
    getViewportSize(viewportSize.x, viewportSize.y);

    const bool rotating = mouseDownOnViewportAxis == ViewportAxisId::RotatorCircle;
    const bool allowHighlight = (mouseDownOnViewportAxis != ViewportAxisId::None && !rotating) || canStartInteractionWithViewport();
    const IPoint2 mousePos(input.mouseX, input.mouseY);
    ViewportAxis viewportAxis(viewport->getViewMatrix(), viewportSize);
    highlightedViewportAxisId = viewportAxis.draw(allowHighlight ? &mousePos : nullptr, rotating);
  }
  else
    highlightedViewportAxisId = ViewportAxisId::None;

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
  showViewportAxis = blk.getBool("viewportAxis", showViewportAxis);

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
  blk.setBool("viewportAxis", showViewportAxis);

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
  if (w <= 0 || h <= 0)
    return;

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

    setOrthogonalZoom(scale * ::min(w / (rb.x - lt.x), h / (rb.y - lt.y)));
  }

  Point3 new_campos = viewport->getViewMatrix().getcol(3);
  if (check_nan(new_campos.x) || check_nan(new_campos.y) || check_nan(new_campos.z) || fabsf(new_campos.x) > 1e6 ||
      fabsf(new_campos.y) > 1e6 || fabsf(new_campos.z) > 1e6)
  {
    logwarn("zoomAndCenter gives %@ -> %@, rollback", prev_campos, new_campos);
    setCameraPos(prev_campos);
  }
}


bool ViewportWindow::onDropFiles(const dag::Vector<String> &files) { return false; }


void ViewportWindow::showStatSettingsDialog()
{
  if (!statSettingsDialog)
  {
    statSettingsDialog = new ViewportWindowStatSettingsDialog(*this, &showStats, _pxScaled(300), _pxScaled(480));
    fillStatSettingsDialog(*statSettingsDialog);
  }

  if (statSettingsDialog->isVisible())
    statSettingsDialog->hide();
  else
    statSettingsDialog->show();
}


void ViewportWindow::showGridSettingsDialog()
{
  if (!gridSettingsDialog)
    gridSettingsDialog = new GridEditDialog(nullptr, EDITORCORE->getGrid(), "Viewport grid settings");

  gridSettingsDialog->showGridEditDialog(vpId);
}

void ViewportWindow::showGizmoSettingsDialog()
{
  if (!gizmoSettingsDialog)
    gizmoSettingsDialog = new GizmoSettingsDialog();
  gizmoSettingsDialog->show();
}

void ViewportWindow::fillStatSettingsDialog(ViewportWindowStatSettingsDialog &dialog)
{
  fillStat3dStatSettings(dialog);

  PropPanel::TLeafHandle cameraGroup = dialog.addGroup(CM_STATS_SETTINGS_CAMERA_GROUP, "Camera", showCameraStats);
  dialog.addOption(cameraGroup, CM_STATS_SETTINGS_CAMERA_POS, "camera pos", showCameraPos);
  dialog.addOption(cameraGroup, CM_STATS_SETTINGS_CAMERA_DIST, "camera dist", showCameraDist);
  dialog.addOption(cameraGroup, CM_STATS_SETTINGS_CAMERA_FOV, "camera FOV", showCameraFov);
  dialog.addOption(cameraGroup, CM_STATS_SETTINGS_CAMERA_SPEED, "camera speed", showCameraSpeed);
  dialog.addOption(cameraGroup, CM_STATS_SETTINGS_CAMERA_TURBO_SPEED, "camera turbo speed", showCameraTurboSpeed);
}


void ViewportWindow::handleStatSettingsDialogChange(int pcb_id, bool value)
{
  if (pcb_id == CM_STATS_SETTINGS_STAT3D_GROUP)
    calcStat3d = value;
  else if (pcb_id == CM_STATS_SETTINGS_CAMERA_GROUP)
    showCameraStats = value;
  else if (pcb_id == CM_STATS_SETTINGS_CAMERA_POS)
    showCameraPos = value;
  else if (pcb_id == CM_STATS_SETTINGS_CAMERA_DIST)
    showCameraDist = value;
  else if (pcb_id == CM_STATS_SETTINGS_CAMERA_FOV)
    showCameraFov = value;
  else if (pcb_id == CM_STATS_SETTINGS_CAMERA_SPEED)
    showCameraSpeed = value;
  else if (pcb_id == CM_STATS_SETTINGS_CAMERA_TURBO_SPEED)
    showCameraTurboSpeed = value;
  else
    handleStat3dStatSettingsDialogChange(pcb_id, value);
}

bool ViewportWindow::canStartInteractionWithViewport()
{
  return isActive() && !rectSelect.active && !isMoveRotateAllowed && mouseDownOnViewportAxis == ViewportAxisId::None &&
         CCameraElem::getCamera() == CCameraElem::MAX_CAMERA;
}

bool ViewportWindow::canRouteMessagesToExternalEventHandler() const
{
  return curEH && !isMoveRotateAllowed && CCameraElem::getCamera() == CCameraElem::MAX_CAMERA;
}

void ViewportWindow::handleViewportAxisMouseLButtonDown()
{
  G_ASSERT(highlightedViewportAxisId != ViewportAxisId::None);

  mouseDownOnViewportAxis = highlightedViewportAxisId;
  captureMouse();

  if (mouseDownOnViewportAxis == ViewportAxisId::RotatorCircle)
  {
    restoreCursorAt = ec_get_cursor_pos();

    if (mIsCursorVisible)
    {
      ec_show_cursor(false);
      mIsCursorVisible = false;
    }
  }
}

void ViewportWindow::handleViewportAxisMouseLButtonUp()
{
  G_ASSERT(mouseDownOnViewportAxis != ViewportAxisId::None);

  if (mouseDownOnViewportAxis == ViewportAxisId::RotatorCircle)
  {
    if (!mIsCursorVisible)
    {
      ec_show_cursor(true);
      mIsCursorVisible = true;
    }

    ec_set_cursor_pos(restoreCursorAt);
  }
  else if (highlightedViewportAxisId == mouseDownOnViewportAxis)
  {
    switch (highlightedViewportAxisId)
    {
      case ViewportAxisId::X: setViewportAxisTransitionEndDirection(Point3(-1.0f, 0.0f, 0.0f), Point3(0.0f, 1.0f, 0.0f)); break;
      case ViewportAxisId::Y: setViewportAxisTransitionEndDirection(Point3(0.0f, -1.0f, 0.0f), Point3(0.0f, 0.0f, 1.0f)); break;
      case ViewportAxisId::Z: setViewportAxisTransitionEndDirection(Point3(0.0f, 0.0f, -1.0f), Point3(0.0f, 1.0f, 0.0f)); break;
      case ViewportAxisId::NegativeX: setViewportAxisTransitionEndDirection(Point3(1.0f, 0.0f, 0.0f), Point3(0.0f, 1.0f, 0.0f)); break;
      case ViewportAxisId::NegativeY: setViewportAxisTransitionEndDirection(Point3(0.0f, 1.0f, 0.0f), Point3(0.0f, 0.0f, -1.f)); break;
      case ViewportAxisId::NegativeZ: setViewportAxisTransitionEndDirection(Point3(0.0f, 0.0f, 1.0f), Point3(0.0f, 1.0f, 0.0f)); break;

      // to prevent the unhandled switch case error
      case ViewportAxisId::None:
      case ViewportAxisId::RotatorCircle: break;
    }
  }

  releaseMouse();
  mouseDownOnViewportAxis = ViewportAxisId::None;
}

void ViewportWindow::processViewportAxisCameraRotation(int mouse_x, int mouse_y)
{
  const float speedUp = 2.0f; // Compared to the Alt + Middle mouse button rotation.

  real deltaX, deltaY;
  processMouseMoveInfluence(deltaX, deltaY, mouse_x, mouse_y);
  deltaX *= ::dagor_game_act_time * speedUp;
  deltaY *= ::dagor_game_act_time * speedUp;

  G_ASSERT(maxCameraElem);
  maxCameraElem->rotate(-deltaX, -deltaY, true, true);

  if (orthogonalProjection && currentProjection != CM_VIEW_CUSTOM_CAMERA)
  {
    currentProjection = CM_VIEW_CUSTOM_CAMERA;
    setCameraViewText();
  }

  updatePluginCamera = true;

  ::cursor_wrap(mouse_x, mouse_y, getMainHwnd());
  prevMousePositionX = mouse_x;
  prevMousePositionY = mouse_y;
}

void ViewportWindow::setViewportAxisTransitionEndDirection(const Point3 &forward, const Point3 &up)
{
  cameraTransitionLastViewMatrix = viewport->getViewMatrix();
  cameraTransitionStartQuaternion = Quat(cameraTransitionLastViewMatrix);
  cameraTransitionEndQuaternion = Quat(createViewMatrix(forward, up));
  cameraTransitionElapsedTime = 0.0f;
  cameraTransitioning = true;

  currentProjection = CM_VIEW_CUSTOM_CAMERA;
  setCameraViewText();
  redrawClientRect();
}

TMatrix ViewportWindow::getViewTm() const { return viewport->getViewMatrix(); }

TMatrix4 ViewportWindow::getProjTm() const { return viewport->getProjectionMatrix(); }

void *ViewportWindow::getMainHwnd() { return EDITORCORE->getWndManager()->getMainWindow(); }

void ViewportWindow::resizeViewportTexture()
{
  // Do not do anything till the first request.
  if (requestedViewportTextureSize.x <= 0 || requestedViewportTextureSize.y <= 0)
    return;

  // Ensure a minimum resolution of 16x16 to avoid texture creation asserts (for example in ScreenSpaceReflections's
  // constructor.)
  requestedViewportTextureSize.x = max(16, requestedViewportTextureSize.x);
  requestedViewportTextureSize.y = max(16, requestedViewportTextureSize.y);

  if (viewportTextureSize == requestedViewportTextureSize)
    return;

  viewportTextureSize = requestedViewportTextureSize;

  String name;
  name.printf(32, "viewport_window_%d_rt", vpId);

  viewportTexture.close();
  viewportTexture.set(d3d::create_tex(nullptr, viewportTextureSize.x, viewportTextureSize.y, TEXCF_RTARGET | TEXFMT_DEFAULT, 1, name),
    name);

  OnChangePosition();
}

bool ViewportWindow::isViewportTextureReady() const { return viewportTexture.getTex() != nullptr; }

void ViewportWindow::copyTextureToViewportTexture(BaseTexture &source_texture, int source_width, int source_height)
{
  Texture *dstTexture = viewportTexture.getTex2D();
  G_ASSERTF(viewportTextureSize.x <= source_width && viewportTextureSize.x + 4 > source_width &&
              viewportTextureSize.y <= source_height && viewportTextureSize.y + 4 > source_height && dstTexture,
    "dstTexture=%p viewport=%dx%d source=%dx%d", //
    dstTexture, viewportTextureSize.x, viewportTextureSize.y, source_width, source_height);

  RectInt rect;
  rect.left = 0;
  rect.top = 0;
  rect.right = viewportTextureSize.x;
  rect.bottom = viewportTextureSize.y;
  d3d::stretch_rect(&source_texture, dstTexture, &rect, &rect);
}

void ViewportWindow::onImguiDelayedCallback(void *user_data)
{
  DelayedMouseEvent *event = static_cast<DelayedMouseEvent *>(user_data);

  if (curEH)
    curEH->handleMouseLBRelease(this, event->x, event->y, event->inside, event->buttons, event->modifierKeys);

  auto it = eastl::find(delayedMouseEvents.begin(), delayedMouseEvents.end(), event);
  G_ASSERT(it == delayedMouseEvents.begin());
  delayedMouseEvents.erase(it);

  delete event;
}

void ViewportWindow::updateImgui(ImGuiID canvas_id, const Point2 &size, float item_spacing, bool vr_mode)
{
  const ImVec2 imguiViewportSize = size;
  requestedViewportTextureSize = IPoint2(floorf(imguiViewportSize.x), floorf(imguiViewportSize.y));

  const TEXTUREID viewportTextureId = viewportTexture.getId();
  if (viewportTextureSize.x > 0 && viewportTextureSize.y > 0 && viewportTextureId != BAD_TEXTUREID)
  {
    PropPanel::focus_helper.setFocusToNextImGuiControlIfRequested(this);

    ImGuiWindow *currentWindow = ImGui::GetCurrentWindow();
    const ImVec2 mousePosInCanvas = ImGui::GetMousePos() - ImGui::GetCursorScreenPos();
    const bool itemMouseButtonHeld = add_viewport_canvas_item(canvas_id, (ImTextureID)((uintptr_t)((unsigned)viewportTextureId)),
      ImVec2(viewportTextureSize.x, viewportTextureSize.y), item_spacing, vr_mode);
    const bool itemHovered = ImGui::IsItemHovered();
    const bool wasActive = active;
    const IPoint2 pt((int)mousePosInCanvas.x, (int)mousePosInCanvas.y);

    // Auto focus the viewport if the cursor is hovered over it and the focus is not in an input text.
    bool autoFocused = false;
    bool itemFocused = ImGui::IsItemFocused();
    if (!itemFocused && itemHovered && !ImGui::IsAnyItemActive() && !ImGui::GetIO().WantTextInput)
    {
      itemFocused = true;
      autoFocused = true;
      ImGui::SetFocusID(canvas_id, currentWindow);
      ImGui::FocusWindow(currentWindow);
    }

    if (active != itemFocused)
    {
      // Not using activate() here because that could override the just opening dialog's requested initial focus by
      // using focus_helper.requestFocus().
      active = itemFocused;
      OnChangeState();
    }

    if (!wasActive && active && !autoFocused && itemHovered &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left, ImGuiInputFlags_None, canvas_id))
    {
      // Swallow the first left mouse button click if the viewport was manually focused. So the first click just
      // focuses the viewport. The main reason this was added to prevent selection loss in the viewport when going
      // back to it from a property panel.
    }
    else if (active)
    {
      // Prevent the hidden mouse from interacting with other controls while in fly mode.
      const bool flyModeActive = isFlyMode();
      if (flyModeActive)
      {
        ImGui::SetActiveID(canvas_id, currentWindow);
        ImGui::SetKeyOwner(ImGuiKey_MouseWheelX, canvas_id);
        ImGui::SetKeyOwner(ImGuiKey_MouseWheelY, canvas_id);

        G_STATIC_ASSERT(sizeof(canvas_id) == sizeof(unsigned));
        switch (CCameraElem::getCamera())
        {
          case CCameraElem::FREE_CAMERA: ec_camera_elem::freeCameraElem->handleKeyboardInput(canvas_id); break;
          case CCameraElem::FPS_CAMERA: ec_camera_elem::fpsCameraElem->handleKeyboardInput(canvas_id); break;
          case CCameraElem::TPS_CAMERA: ec_camera_elem::tpsCameraElem->handleKeyboardInput(canvas_id); break;
          case CCameraElem::CAR_CAMERA: ec_camera_elem::carCameraElem->handleKeyboardInput(canvas_id); break;
        }
      }

      if (itemHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        processMouseLButtonDoubleClick(pt.x, pt.y);
      else if (itemHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        processMouseLButtonPress(pt.x, pt.y);
      else if (itemHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        processMouseRButtonPress(pt.x, pt.y);
      else if (itemHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
        processMouseMButtonPress(pt.x, pt.y);
      else if (mouseButtonDown[ImGuiMouseButton_Left] && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        processMouseLButtonRelease(pt.x, pt.y);
      else if (mouseButtonDown[ImGuiMouseButton_Right] && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        processMouseRButtonRelease(pt.x, pt.y);
      else if (mouseButtonDown[ImGuiMouseButton_Middle] && ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
        processMouseMButtonRelease(pt.x, pt.y);
      else if (itemHovered && ImGui::GetIO().MouseWheel != 0.0f)
        processMouseWheel(pt.x, pt.y, ImGui::GetIO().MouseWheel * EC_WHEEL_DELTA);
      // In fly mode mouse capture is used so we have to ignore boundaries to make mouse cursor wrapping work. (We could
      // also check GetCapture instead.)
      else if ((itemHovered || itemMouseButtonHeld || flyModeActive) && (pt.x != lastMousePosition.x || pt.y != lastMousePosition.y))
      {
        lastMousePosition = IPoint2(pt.x, pt.y);
        processMouseMove(pt.x, pt.y);
      }
    }
    else if (itemHovered)
    {
      // Allow wheel events (like zooming) when the viewport is just hovered, not focused.
      if (ImGui::GetIO().MouseWheel != 0.0f)
        processMouseWheel(pt.x, pt.y, ImGui::GetIO().MouseWheel * EC_WHEEL_DELTA);
      else if (pt.x != lastMousePosition.x || pt.y != lastMousePosition.y)
      {
        lastMousePosition = IPoint2(pt.x, pt.y);
        processMouseMove(pt.x, pt.y);
      }

      if (ImGui::IsAnyItemActive() && ImGui::GetIO().WantTextInput)
        ImGui::SetMouseCursor(EDITOR_CORE_CURSOR_ADDITIONAL_CLICK);
    }
  }

  if (popupMenu)
  {
    const bool open = PropPanel::render_context_menu(*popupMenu);
    if (!open)
      del_it(popupMenu);
  }

  if (selectionMenu)
  {
    const bool open = selectionMenu->getItemCount(ROOT_MENU_ITEM) > 0 && PropPanel::render_context_menu(*selectionMenu);
    if (!open)
    {
      setMenuEventHandler(nullptr);
      del_it(selectionMenu);
    }
  }
}

Point3 ViewportWindow::getCameraPanAnchorPoint()
{
  if (!cameraPanAnchorPoint)
  {
    BBox3 box;
    bool selection = IEditorCoreEngine::get()->getSelectionBox(box);
    if (selection)
    {
      cameraPanAnchorPoint = box.center();
      lastValidCamPanAnchorPoint = box.center();
    }
    else
    {
      const TMatrix camera = inverse(viewport->getViewMatrix());
      const Point3 cameraPos = camera.getcol(3);
      const Point3 forward = camera.getcol(2);

      // try depth buffer
      {
        real depth = 0.f;
        if (tryReadingDepthFromBuffer(getW() / 2, getH() / 2, getDepthBuffer(), depth))
        {
          Point4 unprojected = Point4(0, 0, (1.f - depth), 1) * inverse44(viewport->getProjectionMatrix());
          unprojected /= unprojected.w;

          const Point3 newAnchor = camera * Point3(unprojected.x, unprojected.y, unprojected.z);

          // clamp distance
          const Point3 toAnchor = newAnchor - cameraPos;
          if (toAnchor.lengthSq() > FALLBACK_DIST_CAM_PAN * FALLBACK_DIST_CAM_PAN)
          {
            cameraPanAnchorPoint =
              lastValidCamPanAnchorPoint ? lastValidCamPanAnchorPoint : cameraPos + normalize(toAnchor) * FALLBACK_DIST_CAM_PAN;
          }
          else
          {
            cameraPanAnchorPoint = newAnchor;
            lastValidCamPanAnchorPoint = newAnchor;
          }

          return cameraPanAnchorPoint.value();
        }
      }

      // try raycast if we were not able to access depth texture
      {

        Point3 posS, dir;
        clientToWorld(Point2(getW() / 2, getH() / 2), posS, dir);

        real dist = FALLBACK_DIST_CAM_PAN;
        if (IEditorCoreEngine::get()->traceRay(posS, dir, dist))
        {
          cameraPanAnchorPoint = posS + dir * dist;
          lastValidCamPanAnchorPoint = cameraPanAnchorPoint.value();
        }
        else
        {
          cameraPanAnchorPoint = lastValidCamPanAnchorPoint ? lastValidCamPanAnchorPoint : cameraPos + forward * FALLBACK_DIST_CAM_PAN;
        }
      }
    }
  }

  G_ASSERT(cameraPanAnchorPoint.has_value());
  return cameraPanAnchorPoint.value();
}

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
  if (!camera_config_dialog)
  {
    camera_config_dialog.reset(new CamerasConfigDlg(parent, maxCameraElem->getConfig(), freeCameraElem->getConfig(),
      fpsCameraElem->getConfig(), tpsCameraElem->getConfig()));

    camera_config_dialog->setDialogButtonText(PropPanel::DIALOG_ID_OK, "Close");
    camera_config_dialog->removeDialogButton(PropPanel::DIALOG_ID_CANCEL);
    camera_config_dialog->fill();
  }

  if (camera_config_dialog->isVisible())
    camera_config_dialog->hide();
  else
    camera_config_dialog->show();
}

void close_camera_objects_config_dialog() { camera_config_dialog.reset(); }

void act_camera_objects_config_dialog()
{
  if (camera_config_dialog && camera_objects_config_changed)
  {
    camera_objects_config_changed = false;
    camera_config_dialog->fill();
  }
}

CachedRenderViewports *ec_cached_viewports = NULL;
