// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "globInput.h"
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ui/overlay.h>
#include "render/screencap.h"
#include "render/renderer.h"
#include "main/editMode.h"
#include "game/team.h"
#include "main/app.h"
#include "main/ecsUtils.h"
#include "main/appProfile.h"
#include <startup/dag_globalSettings.h>
#include "camera/sceneCam.h"
#include "ui/userUi.h"
#include <workCycle/dag_startupModules.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <startup/dag_inpDevClsDrv.h>
#include <gui/dag_imgui.h>
#include <imgui/imguiInput.h>
#include "net/netStat.h"
#include "net/userid.h"
#include <util/dag_console.h>
#include <perfMon/dag_cachesim.h>
#include <consoleKeyBindings/consoleKeyBindings.h>
#include <osApiWrappers/dag_clipboard.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_pix.h>
#include <osApiWrappers/dag_unicode.h>
#include "render/videorecorder.h"

extern bool grs_draw_wire;

static bool glob_input_process_top_level_key_down(int key_idx, unsigned key_modif)
{
#if DAGOR_DBGLEVEL > 0
  const bool ctrl = key_modif & HumanInput::KeyboardRawState::CTRL_BITS;
  const bool shift = key_modif & HumanInput::KeyboardRawState::SHIFT_BITS;
  const bool alt = key_modif & HumanInput::KeyboardRawState::ALT_BITS;

  if (const char *commandString = console_keybindings::get_command_by_key_code(key_idx, key_modif & 0xFFF))
  {
    console::command(commandString);
    return true;
  }

  if (imgui_get_state() != ImGuiState::ACTIVE)
  {
    if (imgui_handle_special_keys_down(ctrl, shift, alt, key_idx, key_modif))
      return true;
  }

  switch (key_idx)
  {
    case HumanInput::DKEY_F10: toggle_free_camera(); return true;
    case HumanInput::DKEY_F3: grs_draw_wire = !grs_draw_wire; return true;
    case HumanInput::DKEY_F5:
      if (ctrl && alt)
      {
        screencap::toggle_avi_writer();
        return true;
      }
      break;
    case HumanInput::DKEY_F7:
    {
      bool hadEnlistScene = overlay_ui::gui_scene() != nullptr;
      if (ctrl && alt && hadEnlistScene)
        overlay_ui::shutdown_ui(false);

      // USER VM
      user_ui::reload_user_ui_script(ctrl);

      if (ctrl && alt && hadEnlistScene)
        overlay_ui::init_ui();

      return true;
    }
    case HumanInput::DKEY_F9:
    {
      overlay_ui::reload_scripts(ctrl);
      return true;
    }
    default: break;
  }
#endif
  if (key_idx == HumanInput::DKEY_F11)
  {
    ::da_profiler::request_dump();
    return glob_input_gpu_profiler_handle_F11_key(key_modif & HumanInput::KeyboardRawState::LALT_BIT);
  }
  else if (key_idx == HumanInput::DKEY_F5)
  {
    videorecorder::toggle_record();
    return true;
  }
  return false;
}
static bool glob_input_process_top_level_key_up(int key_idx, unsigned key_modif)
{
  switch (key_idx)
  {
    case HumanInput::DKEY_N:
      if ((key_modif & HumanInput::KeyboardRawState::CTRL_BITS) &&
          (key_modif & HumanInput::KeyboardRawState::SHIFT_BITS)) // Ctrl+Shift+N
      {
        netstat::toggle_render();
        return true;
      }
      break;
    case HumanInput::DKEY_O:
      if (app_profile::get().replay.playFile.empty())
      {
        editmode::select_nearest_actor();
        return true;
      }
      break;
    case HumanInput::DKEY_G:
      if ((key_modif & HumanInput::KeyboardRawState::CTRL_BITS) &&
          (key_modif & HumanInput::KeyboardRawState::SHIFT_BITS)) // Ctrl+Shift+G
      {
        toggle_hide_gui();
        return true;
      }
      break;
#if DAGOR_DBGLEVEL > 0
    case HumanInput::DKEY_P:
      if ((key_modif & HumanInput::KeyboardRawState::CTRL_BITS) &&
          (key_modif & HumanInput::KeyboardRawState::SHIFT_BITS)) // Ctrl+Shift+P
      {
        if (get_timespeed() == 0.f)
          set_timespeed(1.f);
        else
          set_timespeed(0.f);
        return true;
      }
      break;
#endif
    case HumanInput::DKEY_K:
      if (key_modif & HumanInput::KeyboardRawState::CTRL_BITS)
      {
        if (key_modif & HumanInput::KeyboardRawState::SHIFT_BITS) // Ctrl+Shift+K
        {
#if DAGOR_DBGLEVEL > 0
          da_profiler::remove_mode(da_profiler::SAVE_SPIKES | da_profiler::SAMPLING); // we should not sample or save spikes during sim
          ScopedCacheSim::schedule();
#endif
        }
        else
        {
          DagorDateTime time;
          ::get_local_time(&time);

          String fname(256, "%s-%04d.%02d.%02d %02d.%02d.%02d.wpix", "GPUCapture", time.year, time.month, time.day, time.hour,
            time.minute, time.second);
          Tab<wchar_t> buf;
          PIX_GPU_CAPTURE_NEXT_FRAME(D3D11X_PIX_CAPTURE_API, convert_utf8_to_u16_buf(buf, fname.str(), fname.length()));
        }
        return true;
      }
      break;
    case HumanInput::DKEY_PAUSE: editmode::toggle_pause_current(); return true;
    case HumanInput::DKEY_E:
      if ((key_modif & HumanInput::KeyboardRawState::CTRL_BITS) &&
          (key_modif & HumanInput::KeyboardRawState::SHIFT_BITS)) // Ctrl+Shift+E
      {
        const eastl::string url = eastl::to_string(net::get_session_id());
        clipboard::set_clipboard_ansi_text(url.c_str());
        return true;
      }
      break;
  }
  return false;
}

bool glob_input_process_top_level_key(bool down, int key_idx, unsigned key_modif)
{
  return down ? glob_input_process_top_level_key_down(key_idx, key_modif) : glob_input_process_top_level_key_up(key_idx, key_modif);
}

bool glob_input_process_controller(const HumanInput::IGenJoystick *joy)
{
#if DAGOR_DBGLEVEL > 0
  if (imgui_get_state() != ImGuiState::ACTIVE)
  {
    if (imgui_handle_special_controller_combinations(joy))
      return true;
  }
#else
  G_UNUSED(joy);
#endif
  return false;
}

static ecs::EntityId glob_input_eid = ecs::INVALID_ENTITY_ID;
void init_glob_input() { glob_input_eid = create_simple_entity("glob_input"); }

void destroy_glob_input()
{
  if (glob_input_eid != ecs::INVALID_ENTITY_ID)
  {
    g_entity_mgr->destroyEntity(glob_input_eid);
    glob_input_eid = ecs::INVALID_ENTITY_ID;
  }
}

bool have_glob_input() { return true; }

static void app_play_pause_imgui_func()
{
  if (get_timespeed() == 0.f)
    set_timespeed(1.f);
  else
    set_timespeed(0.f);
}

REGISTER_IMGUI_FUNCTION_EX("App", "Play/Pause", "Ctrl+Shift+P", 100, app_play_pause_imgui_func);
