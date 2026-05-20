// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_hotkeyEditorContextMenu.h"

#include "ec_editorCommand.h"
#include "ec_singleHotkeyEditor.h"
#include "ec_ViewportWindowGridSettingsDialogController.h"

#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/ec_interface.h>

#include <propPanel/control/spinEditFloat.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/immediateFocusLossHandler.h>

#include <EASTL/unique_ptr.h>

#include <string.h>

namespace
{

enum class SnapKind
{
  Move,
  Rotate,
  Scale,
};

// Matches the precision the dialog passes to PropPanel::createEditFloat(..., 3, ...). Internally
// SpinEditControlStandalone stores precision-1 for the format string.
constexpr int SNAP_PRECISION = 3;

eastl::optional<SnapKind> trySnapKind(const char *editor_command_id)
{
  if (strcmp(editor_command_id, EditorCommandIds::VIEW_GRID_MOVE_SNAP) == 0)
  {
    return SnapKind::Move;
  }
  if (strcmp(editor_command_id, EditorCommandIds::VIEW_GRID_ANGLE_SNAP) == 0)
  {
    return SnapKind::Rotate;
  }
  if (strcmp(editor_command_id, EditorCommandIds::VIEW_GRID_SCALE_SNAP) == 0)
  {
    return SnapKind::Scale;
  }
  return eastl::nullopt;
}

const char *getSnapCaption(SnapKind kind)
{
  switch (kind)
  {
    case SnapKind::Move: return "Move snap, m";
    case SnapKind::Rotate: return "Rotate snap, deg";
    case SnapKind::Scale: return "Scale snap, %";
  }
  return "";
}

float readSnapStep(SnapKind kind)
{
  GridObject &grid = IEditorCoreEngine::get()->getGrid();
  switch (kind)
  {
    case SnapKind::Move: return grid.getStep();
    case SnapKind::Rotate: return grid.getAngleStep();
    case SnapKind::Scale: return grid.getScaleStep();
  }
  return 0.0f;
}

void writeSnapStep(SnapKind kind, float value)
{
  GridObject &grid = IEditorCoreEngine::get()->getGrid();
  switch (kind)
  {
    case SnapKind::Move: grid.setStep(value); break;
    case SnapKind::Rotate: grid.setAngleStep(value); break;
    case SnapKind::Scale: grid.setScaleStep(value); break;
  }
}

// SpinEditFloatPropertyControl normally lives inside a PropPanel container hierarchy. We want to
// drop it directly into an ImGui popup, so we have no ContainerPropertyControl parent. Override
// onWcChange to call our event handler without going through PropertyControlBase::getRootParent()
// (which asserts when there is no parent).
class StandaloneSpinEditFloat : public PropPanel::SpinEditFloatPropertyControl
{
public:
  using PropPanel::SpinEditFloatPropertyControl::SpinEditFloatPropertyControl;

  void onWcChange(WindowBase *source) override
  {
    G_UNUSED(source);
    if (PropPanel::ControlEventHandler *handler = getEventHandler())
    {
      handler->onChange(getID(), nullptr);
    }
  }
};

// Right-click context menu for toolbar buttons and main-menu items that own an editor command.
// Renders the standard hotkey-management items via raw ImGui (no PropPanel::IMenu), and, for the
// three snap-related toolbar buttons, an inline SpinEditFloatPropertyControl editing the
// corresponding GridObject step value -- the exact same control the grid settings dialog uses.
class HotkeyContextMenu : public PropPanel::ControlEventHandler
{
public:
  HotkeyContextMenu(const char *editor_command_id, eastl::optional<SnapKind> snap_kind) :
    editorCommandId(editor_command_id), snapKind(snap_kind)
  {
    if (snapKind.has_value())
    {
      // width_includes_label = false so the label gets its natural width and the spin edit
      // follows directly, matching the dialog's createEditFloat(..., 3, true, false).
      spinControl.reset(new StandaloneSpinEditFloat(this, /*parent*/ nullptr, /*id*/ 0, getSnapCaption(*snapKind), SNAP_PRECISION,
        /*width_includes_label*/ false));
      spinControl->setFloatValue(readSnapStep(*snapKind));
    }
  }

  // When the popup is dismissed out of band (clicked outside, Escape, menu-item click) while
  // the user had typed but not yet committed a step value, ImGui closes the popup in NewFrame
  // before updateImgui runs again. Trigger PropPanel's immediate-focus-loss path so the spin
  // edit pushes the in-progress value through onWcChange -> onChange -> grid before destruction.
  ~HotkeyContextMenu() override { PropPanel::send_immediate_focus_loss_notification(); }

  // ControlEventHandler override -- called when the spin edit commits a new value (Enter, blur,
  // spinner click, or immediate focus loss).
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    G_UNUSED(pcb_id);
    G_UNUSED(panel);
    if (!snapKind.has_value() || !spinControl)
    {
      return;
    }
    writeSnapStep(*snapKind, spinControl->getFloatValue());
    ViewportWindowGridSettingsDialogController::onSnapStepChanged();
  }

  bool updateImgui()
  {
    const char *popupId = "###HotkeyContextMenu";
    if (!calledOpenPopup)
    {
      ImGui::OpenPopup(popupId);
      calledOpenPopup = true;
    }

    // The popup auto-sizes to its widest item. Without forcing a minimum width, the snap-step
    // spin edit collapses to whatever space is left after the label, and SpinEditFloatPropertyControl
    // clamps to that small available width. Force a sensible popup width when we host the spin edit.
    if (snapKind.has_value())
    {
      ImGui::SetNextWindowSizeConstraints(ImVec2(hdpi::_pxS(240), 0.0f), ImVec2(FLT_MAX, FLT_MAX));
    }

    if (!ImGui::BeginPopup(popupId))
    {
      return false;
    }

    renderHotkeyItems();

    if (snapKind.has_value() && spinControl)
    {
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Pick up changes from other surfaces (e.g. the grid settings dialog). setFloatValue is a
      // no-op while the user is actively editing the text input.
      spinControl->setFloatValue(readSnapStep(*snapKind));
      spinControl->updateImgui();

      ImGui::Spacing();
    }

    ImGui::EndPopup();
    return true;
  }

private:
  void renderHotkeyItems()
  {
    const EditorCommand *command = ec_editor_commands.getCommand(editorCommandId);
    if (!command)
    {
      return;
    }

    if (command->getHotkeyCount() == 1)
    {
      if (ImGui::MenuItem("Change shortcut..."))
      {
        ec_create_single_hotkey_editor(editorCommandId, 0);
      }
    }
    else
    {
      for (int i = 0; i < command->getHotkeyCount(); ++i)
      {
        const String title(0, "Change shortcut %d...", i + 1);
        if (ImGui::MenuItem(title))
        {
          ec_create_single_hotkey_editor(editorCommandId, i);
        }
      }
    }

    if (ImGui::MenuItem("Add shortcut..."))
    {
      ec_create_single_hotkey_editor(editorCommandId, -1);
    }

    if (command->getHotkeyCount() > 0)
    {
      if (ImGui::MenuItem("Remove shortcut(s)"))
      {
        ec_editor_commands.removeAllHotkeys(editorCommandId);
      }
    }

    if (!command->isUsingDefaultHotkeys())
    {
      if (ImGui::MenuItem("Reset shortcut(s)"))
      {
        ec_editor_commands.resetToDefaultHotkeys(editorCommandId);
      }
    }
  }

  const SimpleString editorCommandId;
  const eastl::optional<SnapKind> snapKind;
  bool calledOpenPopup = false;
  eastl::unique_ptr<StandaloneSpinEditFloat> spinControl;
};

eastl::unique_ptr<HotkeyContextMenu> hotkey_context_menu;

} // namespace

void *HotkeyEditorContextMenu::contextMenuMenuItem = nullptr;

void HotkeyEditorContextMenu::createContextMenu(const char *editor_command_id, void *menu_item)
{
  const EditorCommand *command = ec_editor_commands.getCommand(editor_command_id);
  if (!command)
  {
    logerr("Editor command \"%s\" cannot be found.", editor_command_id);
    return;
  }

  // The snap-step input is only relevant for the three toolbar Snap buttons. Main-menu items
  // (menu_item != nullptr) never get one.
  const eastl::optional<SnapKind> snapKind = (menu_item == nullptr) ? trySnapKind(editor_command_id) : eastl::nullopt;
  hotkey_context_menu.reset(new HotkeyContextMenu(editor_command_id, snapKind));
  contextMenuMenuItem = menu_item;
}

void HotkeyEditorContextMenu::updateImguiContextMenu()
{
  if (hotkey_context_menu && !hotkey_context_menu->updateImgui())
  {
    hotkey_context_menu.reset();
    contextMenuMenuItem = nullptr;
  }
}
