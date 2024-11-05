// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de3_gui.h"
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <util/dag_string.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <imgui/imguiInput.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiPointing.h>
#include <startup/dag_inpDevClsDrv.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <startup/dag_globalSettings.h>

static void de3_imgui_control_set_default(GuiControlDesc &v);
static void de3_imgui_read_control(GuiControlDesc &v, const DataBlock &b);
static void de3_imgui_save_control(const GuiControlDesc &v, DataBlock &b);
static void de3_imgui_window_add_control(GuiControlDesc &v);
static void de3_imgui_window();

static bool gui_active = false;

static Tab<GuiControlDesc *> descs;

static void clear_descs()
{
  for (auto *v : descs)
  {
    if ((v->type & 0x7FFFFFFF) == v->GCDT_IntEnum)
    {
      memfree(const_cast<char *>(v->sVal), strmem);
      v->sVal = nullptr;
    }
  }
  clear_and_shrink(descs);
}

void de3_imgui_init(const char *submenu_name, const char *window_name)
{
  clear_descs();
  REGISTER_IMGUI_WINDOW(submenu_name, window_name, de3_imgui_window);

  if (::dgs_get_settings()->getBlockByNameEx("debug")->getBool("imguiStartWithOverlay", false))
    imgui_request_state_change(ImGuiState::OVERLAY);
}
void de3_imgui_term()
{
  clear_descs();
  imgui_shutdown();
}

void de3_imgui_build(GuiControlDesc *add_desc, int add_desc_num)
{
  size_t oldSz = descs.size();
  descs.reserve(oldSz + add_desc_num);
  for (int i = 0; i < add_desc_num; i++)
    descs.push_back(add_desc + i);
  for (auto vi = descs.begin() + oldSz, ve = descs.end(); vi != ve; ++vi)
  {
    auto v = *vi;
    de3_imgui_control_set_default(*v);
    if ((v->type & 0x7FFFFFFF) == v->GCDT_IntEnum) // convert "aaa, bbb, ccc, ..." to "aaa\0bbb\0ccc\0...\0\0"
    {
      const char *src = v->sVal;
      size_t len = strlen(src) + 2;
      char *dest = (char *)memalloc(len, strmem);
      v->sVal = dest;

      memset(dest, 0, len);
      while (const char *src_e = strchr(src, ','))
      {
        memcpy(dest, src, src_e - src);
        dest += src_e - src + 1;
        src = src_e + 1;
        while (strchr(" \t\v\r\n", *src))
          src++;
      }
      size_t l = strlen(src);
      if (l)
        memcpy(dest, src, l);
      G_ASSERTF(dest + l + 2 - v->sVal <= len, "v->sVal=%p dest+l=%p len=%d", v->sVal, dest + l, len);
    }
  }
}

bool de3_imgui_act(float dt)
{
  HumanInput::KeyboardRawState &kbd = HumanInput::raw_state_kbd;

#if _TARGET_PC_MACOSX
  if (kbd.isKeyDown(HumanInput::DKEY_LWIN))
  {
    extern void macosx_app_hide();
    extern void macosx_app_miniaturize();
    if (kbd.isKeyDown(HumanInput::DKEY_M))
    {
      debug("request to minimize (Cmd+M)");
      macosx_app_miniaturize();
      return false;
    }
    else if (kbd.isKeyDown(HumanInput::DKEY_H))
    {
      debug("request to hide (Cmd+H)");
      macosx_app_hide();
      return false;
    }
    else if (kbd.isKeyDown(HumanInput::DKEY_Q))
    {
      debug("request to close (Cmd+Q)");
      quit_game(0);
      return false;
    }
  }
#endif

  bool changeImgui = kbd.isKeyDown(HumanInput::DKEY_F2);
  bool useOverlay = kbd.isKeyDown(HumanInput::DKEY_LSHIFT);
  if (changeImgui && !gui_active)
    de3_imgui_set_gui_active(true, useOverlay);

  if (!changeImgui && gui_active)
    de3_imgui_set_gui_active(false);

  imgui_update();
  return imgui_get_state() == ImGuiState::ACTIVE;
}

void de3_imgui_set_gui_active(bool active, bool overlay)
{
  if (gui_active == active)
    return;

  if (active)
    imgui_handle_special_keys_down(false, overlay, false, HumanInput::DKEY_F2, 0);
  else
    imgui_handle_special_keys_up(false, false, false, HumanInput::DKEY_F2, 0);
  gui_active = active;

  if (global_cls_drv_pnt)
    if (auto *mouse = global_cls_drv_pnt->getDevice(0))
    {
      bool cursorShouldBeRelative = imgui_get_state() != ImGuiState::ACTIVE;
      if (cursorShouldBeRelative != mouse->getRelativeMovementMode())
        mouse->setRelativeMovementMode(cursorShouldBeRelative);
    }
}

void de3_imgui_before_render()
{
  if (imgui_get_state() != ImGuiState::OFF)
    imgui_perform_registered();
}
void de3_imgui_render()
{
  if (imgui_get_state() != ImGuiState::OFF)
  {
    d3d::set_render_target();
    imgui_render();
  }
}

void de3_imgui_enable_obj(const void *v_ptr, bool enable)
{
  for (auto *v : descs)
    if (v->vPtr == v_ptr)
      v->type = enable ? (v->type & 0x7FFFFFFF) : (v->type | 0x80000000);
}
bool de3_imgui_is_obj_enabled(const void *v_ptr)
{
  for (auto *v : descs)
    if (v->vPtr == v_ptr)
      return !(v->type & 0x80000000);
  return false;
}

void de3_imgui_save_values(DataBlock &dest_blk)
{
  dest_blk.clearData();

  for (auto *v : descs)
    de3_imgui_save_control(*v, dest_blk);
}
void de3_imgui_load_values(const DataBlock &src_blk)
{
  for (auto *v : descs)
    de3_imgui_read_control(*v, src_blk);
}

static void de3_imgui_control_set_default(GuiControlDesc &v)
{
  switch (v.type & 0x7FFFFFFF)
  {
    case GuiControlDesc::GCDT_Float:
    case GuiControlDesc::GCDT_SliderFloat: *v.vF32 = v.vDefF32[0]; break;

    case GuiControlDesc::GCDT_Int:
    case GuiControlDesc::GCDT_SliderInt:
    case GuiControlDesc::GCDT_IntEnum: *v.vI32 = v.vDefI32[0]; break;

    case GuiControlDesc::GCDT_Bool:
    case GuiControlDesc::GCDT_Button: *v.vBool = v.vDefI32[0]; break;

    case GuiControlDesc::GCDT_Point2:
      v.vF32[0] = v.vDefF32[0];
      v.vF32[1] = v.vDefF32[1];
      break;
    case GuiControlDesc::GCDT_Point3:
      v.vF32[0] = v.vDefF32[0];
      v.vF32[1] = v.vDefF32[1];
      v.vF32[2] = v.vDefF32[2];
      break;
    case GuiControlDesc::GCDT_IPoint2:
      v.vI32[0] = v.vDefI32[0];
      v.vI32[1] = v.vDefI32[1];
      break;
    case GuiControlDesc::GCDT_IPoint3:
      v.vI32[0] = v.vDefI32[0];
      v.vI32[1] = v.vDefI32[1];
      v.vI32[2] = v.vDefI32[2];
      break;
  }
}
static void de3_imgui_read_control(GuiControlDesc &v, const DataBlock &b)
{
  const DataBlock &pb = *b.getBlockByNameEx(v.panelName);
  switch (v.type & 0x7FFFFFFF)
  {
    case GuiControlDesc::GCDT_Float:
    case GuiControlDesc::GCDT_SliderFloat: *v.vF32 = pb.getReal(v.name, v.vDefF32[0]); break;

    case GuiControlDesc::GCDT_Int:
    case GuiControlDesc::GCDT_SliderInt: *v.vI32 = pb.getInt(v.name, v.vDefI32[0]); break;
    case GuiControlDesc::GCDT_IntEnum:
      *v.vI32 = pb.getInt(v.name, v.vDefI32[0]);
      for (auto &s : v.iVals)
        if (*v.vI32 == s)
        {
          v.iSel = &s - v.iVals;
          break;
        }
      break;

    case GuiControlDesc::GCDT_Bool: *v.vBool = pb.getBool(v.name, v.vDefI32[0]); break;

    case GuiControlDesc::GCDT_Point2: *(Point2 *)(v.vF32) = pb.getPoint2(v.name, Point2(v.vDefF32[0], v.vDefF32[1])); break;
    case GuiControlDesc::GCDT_Point3:
      *(Point3 *)(v.vF32) = pb.getPoint3(v.name, Point3(v.vDefF32[0], v.vDefF32[1], v.vDefF32[2]));
      break;
    case GuiControlDesc::GCDT_IPoint2: *(IPoint2 *)(v.vI32) = pb.getIPoint2(v.name, IPoint2(v.vDefI32[0], v.vDefI32[1])); break;
    case GuiControlDesc::GCDT_IPoint3:
      *(IPoint3 *)(v.vI32) = pb.getIPoint3(v.name, IPoint3(v.vDefI32[0], v.vDefI32[1], v.vDefI32[2]));
      break;
  }
}
static void de3_imgui_save_control(const GuiControlDesc &v, DataBlock &b)
{
  DataBlock &pb = *b.addBlock(v.panelName);
  switch (v.type & 0x7FFFFFFF)
  {
    case GuiControlDesc::GCDT_Float:
    case GuiControlDesc::GCDT_SliderFloat: pb.setReal(v.name, *v.vF32); break;

    case GuiControlDesc::GCDT_Int:
    case GuiControlDesc::GCDT_SliderInt:
    case GuiControlDesc::GCDT_IntEnum: pb.setInt(v.name, *v.vI32); break;

    case GuiControlDesc::GCDT_Bool: pb.setBool(v.name, *v.vBool); break;

    case GuiControlDesc::GCDT_Point2: pb.setPoint2(v.name, *(Point2 *)(v.vF32)); break;
    case GuiControlDesc::GCDT_Point3: pb.setPoint3(v.name, *(Point3 *)(v.vF32)); break;
    case GuiControlDesc::GCDT_IPoint2: pb.setIPoint2(v.name, *(IPoint2 *)(v.vI32)); break;
    case GuiControlDesc::GCDT_IPoint3: pb.setIPoint3(v.name, *(IPoint3 *)(v.vI32)); break;
  }
}
static void de3_imgui_window_add_control(GuiControlDesc &v)
{
  switch (v.type)
  {
    case GuiControlDesc::GCDT_Float: ImGui::DragFloat(v.name, v.vF32, 1.0f, v.vMinF32, v.vMaxF32); break;
    case GuiControlDesc::GCDT_Int: ImGui::DragInt(v.name, v.vI32, 1.0f, v.vMinI32, v.vMaxI32); break;
    case GuiControlDesc::GCDT_SliderFloat: ImGui::SliderFloat(v.name, v.vF32, v.vMinF32, v.vMaxF32); break;
    case GuiControlDesc::GCDT_SliderInt: ImGui::SliderInt(v.name, v.vI32, v.vMinI32, v.vMaxI32); break;
    case GuiControlDesc::GCDT_IntEnum:
      ImGui::Combo(v.name, &v.iSel, v.sVal);
      *v.vI32 = v.iSel >= 0 ? v.iVals[v.iSel] : 0;
      break;
    case GuiControlDesc::GCDT_Bool: ImGui::Checkbox(v.name, v.vBool); break;
    case GuiControlDesc::GCDT_Button: *v.vBool = ImGui::Button(v.name); break;
    case GuiControlDesc::GCDT_Point2: ImGui::DragFloat2(v.name, v.vF32, 1.0f, v.vMinF32, v.vMaxF32); break;
    case GuiControlDesc::GCDT_Point3: ImGui::DragFloat3(v.name, v.vF32, 1.0f, v.vMinF32, v.vMaxF32); break;
    case GuiControlDesc::GCDT_IPoint2: ImGui::DragInt2(v.name, v.vI32, 1.0f, v.vMinI32, v.vMaxI32); break;
    case GuiControlDesc::GCDT_IPoint3: ImGui::DragInt3(v.name, v.vI32, 1.0f, v.vMinI32, v.vMaxI32); break;
  }
}

static void de3_imgui_window()
{
  const char *cur_panel = nullptr;
  bool cur_panel_open = false;
  for (auto *v : descs)
  {
    if (cur_panel != v->panelName)
      cur_panel_open = ImGui::CollapsingHeader(cur_panel = v->panelName, ImGuiTreeNodeFlags_CollapsingHeader);
    if (cur_panel_open)
      de3_imgui_window_add_control(*v);
  }
}
