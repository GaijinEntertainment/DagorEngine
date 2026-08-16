// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ec_editorCommand.h"

#include <EditorCore/ec_rect.h>
#include <EditorCore/ec_wndPublic.h>
#include <dag/dag_vector.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

class ImguiWndManagerBase : public IWndManager
{
public:
  class WindowPositionAndSize
  {
  public:
    WindowPositionAndSize() { reset(); }

    void reset()
    {
      rectangle = EcRect{};
      maximized = true;
    }

    bool isValid() const { return rectangle.width() >= MINIMUM_SIZE && rectangle.height() >= MINIMUM_SIZE; }

    static constexpr int MINIMUM_SIZE = 32;

    EcRect rectangle;
    bool maximized;
  };

  int run(int width, int height, const char *caption, const char *icon, WindowSizeInit size) override { return 0; }

  void registerWindowHandler(IWndManagerWindowHandler *handler) override
  {
    if (!handler)
      return;

    if (eastl::find(handlers.begin(), handlers.end(), handler) != handlers.end())
      return;

    handlers.push_back(handler);
  }

  void unregisterWindowHandler(IWndManagerWindowHandler *handler) override
  {
    if (!handler)
      return;

    auto it = eastl::find(handlers.begin(), handlers.end(), handler);
    if (it == handlers.end())
      return;

    handlers.erase(it);
  }

  void reset() override
  {
    while (!windows.empty())
    {
      G_VERIFY(removeWindow(windows[0]));
    }
  }

  void show(WindowSizeInit size) override {}

  bool removeWindow(void *handle) override
  {
    auto it = eastl::find(windows.begin(), windows.end(), handle);
    if (it == windows.end())
      return false;

    windows.erase(it);

    for (int i = 0; i < handlers.size(); ++i)
      if (handlers[i]->onWmDestroyWindow(handle))
        return true;

    return false;
  }

  void setWindowType(void *handle, int type) override
  {
    G_ASSERT(!handle);

    for (int i = 0; i < handlers.size(); ++i)
    {
      handle = handlers[i]->onWmCreateWindow(type);
      if (handle)
      {
        windows.push_back(handle);
        return;
      }
    }
  }

  bool getWindowPosSize(void *handle, int &x, int &y, unsigned &width, unsigned &height) override { return false; }

  void setMenuArea(void *handle, hdpi::Px width, hdpi::Px height) override {}

  void addAccelerator(unsigned cmd_id, ImGuiKeyChord key_chord) override { addAcceleratorInternal(cmd_id, key_chord); }

  void addAccelerator(unsigned cmd_id, const char *editor_command_id) override { addAcceleratorInternal(cmd_id, editor_command_id); }

  void addViewportAccelerator(unsigned cmd_id, ImGuiKeyChord key_chord, bool allow_repeat) override
  {
    addAcceleratorInternal(cmd_id, key_chord, allow_repeat, /*viewport_accelerator = */ true);
  }

  void addViewportAccelerator(unsigned cmd_id, const char *editor_command_id, bool allow_repeat) override
  {
    addAcceleratorInternal(cmd_id, editor_command_id, allow_repeat, /*viewport_accelerator = */ true);
  }

  void clearAccelerators() override { accelerators.clear(); }

  unsigned processImguiAccelerator(bool has_active_viewport, bool &viewport_accelerator) override
  {
    for (const Accelerator &accelerator : accelerators)
    {
      if (accelerator.viewportAccelerator && !has_active_viewport)
        continue;

      if (ImGui::Shortcut(accelerator.keyChord, accelerator.inputFlags | ImGuiInputFlags_RouteGlobal))
      {
        viewport_accelerator = accelerator.viewportAccelerator;
        return accelerator.cmdId;
      }
    }

    return 0;
  }

private:
  struct Accelerator
  {
    Accelerator(unsigned cmd_id, ImGuiKeyChord key_chord, bool allow_repeat = false, bool viewport_accelerator = false) :
      cmdId(cmd_id), keyChord(key_chord), viewportAccelerator(viewport_accelerator)
    {
      G_ASSERT(cmd_id != 0);

      inputFlags = ImGuiInputFlags_None;
      if (allow_repeat)
        inputFlags |= ImGuiInputFlags_Repeat;
    }

    unsigned cmdId;
    ImGuiKeyChord keyChord;
    ImGuiInputFlags inputFlags;
    bool viewportAccelerator;
  };

  void addAcceleratorInternal(unsigned cmd_id, ImGuiKeyChord key_chord, bool allow_repeat = false, bool viewport_accelerator = false)
  {
    Accelerator accelerator(cmd_id, key_chord, allow_repeat, viewport_accelerator);
    accelerators.push_back(eastl::move(accelerator));
  }

  void addAcceleratorInternal(unsigned cmd_id, const char *editor_command_id, bool allow_repeat = false,
    bool viewport_accelerator = false)
  {
    const EditorCommand *command = ec_editor_commands.getCommand(editor_command_id);
    if (!command)
      return;

    for (int i = 0; i < command->getHotkeyCount(); ++i)
      addAcceleratorInternal(cmd_id, command->getKeyChord(i), allow_repeat, viewport_accelerator);

    ec_editor_commands.setCommandCmdId(editor_command_id, cmd_id);
  }

  dag::Vector<void *> windows;
  dag::Vector<IWndManagerWindowHandler *> handlers;
  dag::Vector<Accelerator> accelerators;
};
