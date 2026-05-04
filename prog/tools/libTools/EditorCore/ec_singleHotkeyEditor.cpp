// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_singleHotkeyEditor.h"

#include "ec_editorCommand.h"

#include <gui/dag_imgui.h>
#include <propPanel/colors.h>
#include <util/dag_string.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace
{

class SingleHotkeyEditor
{
public:
  SingleHotkeyEditor(const char *editor_command_id, int hotkey_index, ImGuiKeyChord initial_key_chord) :
    editorCommandId(editor_command_id), hotkeyIndex(hotkey_index), pickedKeyChord(initial_key_chord)
  {
    updateCommandsUsingTheSameKeyLabel();
  }

  const char *getEditorCommandId() const { return editorCommandId; }
  int getHotkeyIndex() const { return hotkeyIndex; }

  eastl::optional<ImGuiKeyChord> getPickedKeyChord() const { return acceptedPick ? pickedKeyChord : eastl::optional<ImGuiKeyChord>(); }

  bool updateImgui()
  {
    const char *popupId = "###SingleHotkeyEditorPopup";
    if (!calledOpenPopup)
    {
      calledOpenPopup = true;
      ImGui::OpenPopup(popupId);
    }

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    const bool popupIsOpen = ImGui::BeginPopup(popupId);
    ImGui::PopStyleColor();

    if (!popupIsOpen)
      return false;

    ImGui::PushFont(imgui_get_bold_font(), 0.0f);
    ImGui::TextUnformatted("Press the desired shortcut.");
    ImGui::PopFont();

    ImGui::TextUnformatted("Press [Enter] to accept, [Escape] to reject, [Backspace] to clear the shortcut.");
    ImGui::NewLine();

    const bool open = updateInputControl();

    ImGui::NewLine();
    ImGui::PushStyleColor(ImGuiCol_Text, PropPanel::getOverriddenColor(PropPanel::ColorOverride::CONSOLE_LOG_WARNING_TEXT));
    ImGui::TextUnformatted(commandsUsingTheSameKeyLabel);
    ImGui::PopStyleColor();

    ImGui::EndPopup();

    return open;
  }

private:
  void updateCommandsUsingTheSameKeyLabel()
  {
    commandsUsingTheSameKeyLabel.clear();

    if (pickedKeyChord == ImGuiKey_None)
      return;

    dag::ConstSpan<const EditorCommand> commands = ec_editor_commands.getCommands();
    const EditorCommand *editedCommand = ec_editor_commands.getCommand(editorCommandId);
    G_ASSERT(editedCommand);
    String text;
    int count = 0;

    for (const EditorCommand &command : commands)
    {
      if (&command == editedCommand)
      {
        if (getKeyChordIndex(command, pickedKeyChord, hotkeyIndex) >= 0)
        {
          text += "\n";
          text += ec_editor_commands.getCommandId(command);
          text += " (this command)";
          ++count;
        }
      }
      else
      {
        if (getKeyChordIndex(command, pickedKeyChord) >= 0)
        {
          text += "\n";
          text += ec_editor_commands.getCommandId(command);
          ++count;
        }
      }
    }

    if (count > 0)
      commandsUsingTheSameKeyLabel.printf(0, "%d command(s) use this shortcut:%s", count, text.c_str());
  }

  void updateInput(const char *input_id_text, ImGuiID input_id)
  {
    int nonModifierKeyDownCount = 0;
    ImGuiKeyChord keyChord = ImGuiKey_None;

    if (ImGui::GetCurrentContext()->NavId == input_id)
    {
      // Make sure we cover all keys, and keys only.
      ImGui::SetKeyOwner(ImGuiMod_Alt, input_id);
      ImGui::SetKeyOwner(ImGuiMod_Ctrl, input_id);
      ImGui::SetKeyOwner(ImGuiMod_Shift, input_id);
      for (int key = ImGuiKey_Keyboard_BEGIN; key < ImGuiKey_Keyboard_END; ++key)
        ImGui::SetKeyOwner((ImGuiKey)key, input_id);

      keyChord = getKeyChord(input_id, nonModifierKeyDownCount);
    }

    const bool allKeysAreUp = keyChord == ImGuiKey_None && nonModifierKeyDownCount == 0;

    // Ignore reserved keys.
    if (
      keyChord == ImGuiKey_Enter || keyChord == ImGuiKey_KeypadEnter || keyChord == ImGuiKey_Escape || keyChord == ImGuiKey_Backspace)
      keyChord = ImGuiKey_None;

    if (allowPickingKeyChord)
    {
      // Reset the picked key if any key has been pressed.
      if (keyChord != ImGuiKey_None)
        pickedKeyChord = ImGuiKey_None;

      // Once a non-modifier key has been pressed then we have a pick.
      if (keyChord != ImGuiKey_None && nonModifierKeyDownCount == 1)
      {
        pickedKeyChord = keyChord;
        allowPickingKeyChord = false;
      }
    }
    else
    {
      // Once all keys have been released a new key can be picked.
      if (allKeysAreUp)
        allowPickingKeyChord = true;
    }

    if (pickedKeyChord == ImGuiKey_None)
    {
      const bool displayableChord = keyChord != ImGuiKey_None && nonModifierKeyDownCount <= 1;
      const char *name = displayableChord ? ImGui::GetKeyChordName(keyChord) : "";
      inputButtonLabel.printf(0, "%s%s", name, input_id_text);
    }
    else
    {
      const char *name = ImGui::GetKeyChordName(pickedKeyChord);
      inputButtonLabel.printf(0, "%s%s", name, input_id_text);
    }
  }

  bool updateInputControl()
  {
    const char *inputIdText = "###HotkeyInput";
    const ImGuiID inputId = ImGui::GetCurrentWindowRead()->GetID(inputIdText);
    const ImGuiKeyChord oldPickedKeyChord = pickedKeyChord;
    updateInput(inputIdText, inputId);

    ImGui::SetKeyboardFocusHere();

    const float oldAlignX = ImGui::GetStyle().ButtonTextAlign.x;
    ImGui::GetStyle().ButtonTextAlign.x = 0.0f;
    ImGui::Button(inputButtonLabel, ImVec2(-FLT_MIN, 0.0f));
    ImGui::RenderNavCursor(ImGui::GetCurrentContext()->LastItemData.Rect, inputId, ImGuiNavRenderCursorFlags_AlwaysDraw);
    ImGui::GetStyle().ButtonTextAlign.x = oldAlignX;

    if (ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_None, inputId) ||
        ImGui::Shortcut(ImGuiKey_KeypadEnter, ImGuiInputFlags_None, inputId))
    {
      acceptedPick = true;
      return false;
    }
    else if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_None, inputId))
      return false;
    else if (ImGui::Shortcut(ImGuiKey_Backspace, ImGuiInputFlags_None, inputId))
      pickedKeyChord = ImGuiKey_None;

    if (pickedKeyChord != oldPickedKeyChord)
      updateCommandsUsingTheSameKeyLabel();

    return true;
  }

  static ImGuiKeyChord getKeyChord(ImGuiID input_id, int &key_down_count)
  {
    ImGuiKeyChord keyChord = ImGuiKey_None;
    if (ImGui::IsKeyDown(ImGuiMod_Alt, input_id))
      keyChord |= ImGuiMod_Alt;
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl, input_id))
      keyChord |= ImGuiMod_Ctrl;
    if (ImGui::IsKeyDown(ImGuiMod_Shift, input_id))
      keyChord |= ImGuiMod_Shift;

    key_down_count = 0;
    for (int key = ImGuiKey_Keyboard_BEGIN; key < ImGuiKey_Keyboard_END; ++key)
    {
      if (ImGui::IsLRModKey((ImGuiKey)key))
        continue;

      if (ImGui::IsKeyDown((ImGuiKey)key, input_id))
      {
        keyChord |= key;
        ++key_down_count;
      }
    }

    return keyChord;
  }

  static int getKeyChordIndex(const EditorCommand &command, ImGuiKeyChord key_chord, int ignore_index = -1)
  {
    for (int i = 0; i < command.getHotkeyCount(); ++i)
      if (i != ignore_index && command.getKeyChord(i) == key_chord)
        return i;

    return -1;
  }

  const String editorCommandId;
  const int hotkeyIndex;
  String inputButtonLabel;
  String commandsUsingTheSameKeyLabel;
  ImGuiKeyChord pickedKeyChord = ImGuiKey_None;
  bool allowPickingKeyChord = true;
  bool acceptedPick = false;
  bool calledOpenPopup = false;
};

eastl::unique_ptr<SingleHotkeyEditor> single_hotkey_editor;

} // namespace

void ec_create_single_hotkey_editor(const char *editor_command_id, int hotkey_index)
{
  const EditorCommand *command = ec_editor_commands.getCommand(editor_command_id);
  G_ASSERT(command);

  ImGuiKeyChord initialKeyChord = ImGuiKey_None;
  if (hotkey_index >= 0)
  {
    G_ASSERT(hotkey_index < command->getHotkeyCount());
    initialKeyChord = command->getKeyChord(hotkey_index);
  }

  single_hotkey_editor.reset(new SingleHotkeyEditor(editor_command_id, hotkey_index, initialKeyChord));
}

void ec_update_imgui_single_hotkey_editor()
{
  if (single_hotkey_editor && !single_hotkey_editor->updateImgui())
  {
    eastl::optional<ImGuiKeyChord> keyChord = single_hotkey_editor->getPickedKeyChord();
    if (keyChord.has_value())
    {
      const EditorCommand *command = ec_editor_commands.getCommand(single_hotkey_editor->getEditorCommandId());
      G_ASSERT(command);
      const int hotkeyIndex = single_hotkey_editor->getHotkeyIndex();
      if (hotkeyIndex >= 0)
      {
        G_ASSERT(hotkeyIndex < command->getHotkeyCount());
        if (keyChord.value() == ImGuiKey_None)
          ec_editor_commands.removeHotkey(single_hotkey_editor->getEditorCommandId(), hotkeyIndex);
        else
          ec_editor_commands.changeHotkey(single_hotkey_editor->getEditorCommandId(), hotkeyIndex, keyChord.value());
      }
      else if (keyChord.value() != 0) // when leaving shorcut blank on adding new shortcut
      {
        ec_editor_commands.addHotkey(single_hotkey_editor->getEditorCommandId(), keyChord.value());
      }
    }

    single_hotkey_editor.reset();
  }
}
