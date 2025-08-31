// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "contextMenuInternal.h"
#include <imgui/imgui.h>

namespace PropPanel
{

void ContextMenu::onImguiDelayedCallback(void *user_data)
{
  G_ASSERT(waitingForDelayedCallback);
  waitingForDelayedCallback = false;

  Menu::onImguiDelayedCallback(user_data);
}

bool ContextMenu::updateImgui()
{
  ImGui::PushID(this);

  const char *popupId = "p"; // "p" stands for pop-up. It could be anything.

  if (!calledOpenPopup)
  {
    calledOpenPopup = true;
    ImGui::OpenPopup(popupId);
  }

  const bool open = ImGui::BeginPopup(popupId);
  if (open)
  {
    MenuItem *clickedItem = nullptr;
    rootMenu.updateImgui(clickedItem);

    if (clickedItem && eventHandler)
    {
      G_ASSERT(!waitingForDelayedCallback);
      waitingForDelayedCallback = true;

      message_queue.requestDelayedCallback(*this, (void *)((uintptr_t)clickedItem->id));
    }

    ImGui::EndPopup();
  }

  ImGui::PopID();

  return open || waitingForDelayedCallback;
}

IMenu *create_context_menu() { return new ContextMenu(); }

bool render_context_menu(IMenu &menu)
{
  G_ASSERT(menu.isContextMenu());
  return static_cast<ContextMenu &>(menu).updateImgui();
}

} // namespace PropPanel