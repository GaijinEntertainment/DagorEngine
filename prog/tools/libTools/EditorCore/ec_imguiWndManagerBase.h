// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_wndPublic.h>
#include <dag/dag_vector.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

class ImguiWndManagerBase : public IWndManager
{
public:
  int run(int width, int height, const char *caption, const char *icon, WindowSizeInit size) override { return 0; }

  bool loadLayout(const char *filename) override { return false; }
  void saveLayout(const char *filename) override {}

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

  void addAccelerator(unsigned cmd_id, ImGuiKeyChord key_chord) override
  {
    Accelerator accelerator(cmd_id, key_chord);
    accelerators.push_back(eastl::move(accelerator));
  }

  void addAcceleratorUp(unsigned cmd_id, ImGuiKeyChord key_chord) override
  {
    G_ASSERT((key_chord & ImGuiMod_Ctrl) == 0);
    G_ASSERT((key_chord & ImGuiMod_Alt) == 0);
    G_ASSERT((key_chord & ImGuiMod_Shift) == 0);

    Accelerator accelerator(cmd_id, key_chord);
    acceleratorUps.push_back(eastl::move(accelerator));
  }

  void addViewportAccelerator(unsigned cmd_id, ImGuiKeyChord key_chord, bool allow_repeat = false) override
  {
    Accelerator accelerator(cmd_id, key_chord, allow_repeat);
    viewportAccelerators.push_back(eastl::move(accelerator));
  }

  void clearAccelerators() override
  {
    accelerators.clear();
    acceleratorUps.clear();
    viewportAccelerators.clear();
  }

  unsigned processImguiAccelerator() override
  {
    for (const Accelerator &accelerator : accelerators)
      if (ImGui::Shortcut(accelerator.keyChord, accelerator.inputFlags | ImGuiInputFlags_RouteGlobal))
        return accelerator.cmdId;

    // NOTE: ImGui porting: this should be ImGuiKeyOwner_NoOwner instead of ImGuiKeyOwner_Any but then CM_TAB_RELEASE
    // (currently the only one command that uses accelerator up) would not work.
    for (const Accelerator &accelerator : acceleratorUps)
      if (ImGui::IsKeyReleased((ImGuiKey)accelerator.keyChord, ImGuiKeyOwner_Any))
        return accelerator.cmdId;

    return 0;
  }

  unsigned processImguiViewportAccelerator(ImGuiID viewport_id) override
  {
    for (const Accelerator &accelerator : viewportAccelerators)
      if (ImGui::Shortcut(accelerator.keyChord, accelerator.inputFlags, viewport_id))
        return accelerator.cmdId;

    return 0;
  }

private:
  struct Accelerator
  {
    Accelerator(unsigned cmd_id, ImGuiKeyChord key_chord, bool allow_repeat = false) : cmdId(cmd_id), keyChord(key_chord)
    {
      G_ASSERT(cmd_id != 0);

      inputFlags = ImGuiInputFlags_None;
      if (allow_repeat)
        inputFlags |= ImGuiInputFlags_Repeat;
    }

    unsigned cmdId;
    ImGuiKeyChord keyChord;
    ImGuiInputFlags inputFlags;
  };

  dag::Vector<void *> windows;
  dag::Vector<IWndManagerWindowHandler *> handlers;
  dag::Vector<Accelerator> accelerators;
  dag::Vector<Accelerator> acceleratorUps;
  dag::Vector<Accelerator> viewportAccelerators;
};
