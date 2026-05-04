// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_editorCommand.h"

#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_interface.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_fastIntList.h>
#include <util/dag_string.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

EditorCommands ec_editor_commands;

const char *EditorCommandHotkey::getKeyChordAsText() const
{
  G_ASSERT(keyChord != ImGuiKey_None);
  return ImGui::GetKeyChordName(keyChord);
}

const char *EditorCommand::getKeyChordsAsText() const
{
  static String text;

  text.clear();
  for (const EditorCommandHotkey &hotkey : hotkeys)
  {
    if (!text.empty())
      text += " or ";
    text += hotkey.getKeyChordAsText();
  }

  return text.c_str();
}

bool EditorCommand::isUsingDefaultHotkeys() const { return hotkeys == defaultHotkeys; }

void EditorCommands::addCommand(const char *id) { addCommand(id, ImGuiKey_None); }

void EditorCommands::addCommand(const char *id, ImGuiKeyChord key_chord) { addCommand(id, key_chord, ImGuiKey_None); }

void EditorCommands::addCommand(const char *id, ImGuiKeyChord key_chord1, ImGuiKeyChord key_chord2)
{
  G_ASSERT(id && *id);

  const int index = idToIndexMap.addNameId(id);
  G_ASSERT(index >= 0);
  if (index < commands.size())
  {
    logerr("Editor command \"%s\" is already registered.", id);
    return;
  }

  G_ASSERT(index == commands.size());
  EditorCommand &command = commands.push_back();

  if (key_chord1 != ImGuiKey_None)
  {
    command.defaultHotkeys.push_back().keyChord = key_chord1;
    command.hotkeys.push_back().keyChord = key_chord1;
  }

  if (key_chord2 != ImGuiKey_None)
  {
    command.defaultHotkeys.push_back().keyChord = key_chord2;
    command.hotkeys.push_back().keyChord = key_chord2;
  }
}

int EditorCommands::getCommandIndex(const char *id) const { return idToIndexMap.getNameId(id); }

const EditorCommand *EditorCommands::getCommand(const char *id) const
{
  const int index = getCommandIndex(id);
  return index >= 0 ? &commands[index] : nullptr;
}

const char *EditorCommands::getCommandId(const EditorCommand &command) const
{
  if (commands.empty())
    return nullptr;

  const int index = &command - commands.data();
  G_ASSERT(index >= 0);
  G_ASSERT(commands.size() == idToIndexMap.nameCount());
  return idToIndexMap.getName(index);
}

const char *EditorCommands::getCommandId(int index) const
{
  if (commands.empty())
    return nullptr;

  G_ASSERT(index >= 0 && index < commands.size());
  G_ASSERT(commands.size() == idToIndexMap.nameCount());
  return idToIndexMap.getName(index);
}

void EditorCommands::setCommandCmdId(const char *id, unsigned int cmd_id)
{
  const int index = getCommandIndex(id);
  if (index >= 0)
    commands[index].cmdId = cmd_id;
}

void EditorCommands::sendChangeNotification()
{
  IEditorCommandKeyChordChangeEventHandler *eh = EDITORCORE->queryEditorInterface<IEditorCommandKeyChordChangeEventHandler>();
  if (eh)
    eh->onEditorCommandKeyChordChanged();
}

void EditorCommands::addHotkey(const char *id, ImGuiKeyChord key_chord)
{
  const int commandIndex = getCommandIndex(id);
  G_ASSERT(commandIndex >= 0);
  EditorCommand &command = commands[commandIndex];

  G_ASSERT(key_chord != ImGuiKey_None);
  command.hotkeys.push_back().keyChord = key_chord;
  sendChangeNotification();
}

void EditorCommands::changeHotkey(const char *id, int hotkey_index, ImGuiKeyChord key_chord)
{
  const int commandIndex = getCommandIndex(id);
  G_ASSERT(commandIndex >= 0);
  EditorCommand &command = commands[commandIndex];

  if (key_chord == command.getKeyChord(hotkey_index))
    return;

  G_ASSERT(key_chord != ImGuiKey_None); // Use removeHotkey() to remove a hotkey!
  command.hotkeys[hotkey_index].keyChord = key_chord;
  sendChangeNotification();
}

void EditorCommands::removeHotkey(const char *id, int hotkey_index)
{
  const int commandIndex = getCommandIndex(id);
  G_ASSERT(commandIndex >= 0);
  EditorCommand &command = commands[commandIndex];

  G_ASSERT(hotkey_index >= 0 && hotkey_index < command.hotkeys.size());
  command.hotkeys.erase(command.hotkeys.begin() + hotkey_index);
  sendChangeNotification();
}

void EditorCommands::removeAllHotkeys(const char *id)
{
  const int commandIndex = getCommandIndex(id);
  G_ASSERT(commandIndex >= 0);
  EditorCommand &command = commands[commandIndex];

  if (command.getHotkeyCount() == 0)
    return;

  command.hotkeys.clear();
  sendChangeNotification();
}

void EditorCommands::resetToDefaultHotkeys(const char *id)
{
  const int commandIndex = getCommandIndex(id);
  G_ASSERT(commandIndex >= 0);
  EditorCommand &command = commands[commandIndex];

  if (command.isUsingDefaultHotkeys())
    return;

  command.hotkeys = command.defaultHotkeys;
  sendChangeNotification();
}

eastl::optional<ImGuiKeyChord> EditorCommands::getModifierKeysFromText(const char *&text)
{
  ImGuiKeyChord modifierKey = ImGuiKey_None;

  while (true)
  {
    const char *plusPos = strchr(text, '+');
    if (!plusPos)
      return modifierKey;

    if (strncmp(text, "Ctrl", 4) == 0)
    {
      modifierKey |= ImGuiMod_Ctrl;
      text += 4;
    }
    else if (strncmp(text, "Shift", 5) == 0)
    {
      modifierKey |= ImGuiMod_Shift;
      text += 5;
    }
    else if (strncmp(text, "Alt", 3) == 0)
    {
      modifierKey |= ImGuiMod_Alt;
      text += 3;
    }
    else if (strncmp(text, "Super", 5) == 0)
    {
      modifierKey |= ImGuiMod_Super;
      text += 5;
    }

    if (text != plusPos)
      return {};

    text += 1;
  }

  return modifierKey;
}

eastl::optional<ImGuiKeyChord> EditorCommands::getKeyChordFromText(const char *text)
{
  if (text == nullptr || *text == 0)
    return ImGuiKey_None;

  const char *textAfterModifierKeys = text;
  const eastl::optional<ImGuiKeyChord> modifierKey = getModifierKeysFromText(textAfterModifierKeys);
  if (!modifierKey.has_value())
    return {};

  G_ASSERT(!strchr(textAfterModifierKeys, '+'));

  for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1))
    if (strcmp(ImGui::GetKeyName(key), textAfterModifierKeys) == 0)
      return modifierKey.value() | key;

  return {};
}

void EditorCommands::loadChangedHotkeys(const DataBlock &blk)
{
  const DataBlock *blkCommands = blk.getBlockByName("commands");
  if (!blkCommands)
    return;

  const int commandId = blk.getNameId("command");
  if (commandId < 0)
    return;

  FastIntList loadedCmmandIndices;

  const int commandCount = blkCommands->blockCount();
  for (int i = 0; i < commandCount; ++i)
  {
    const DataBlock *blkCommand = blkCommands->getBlock(i);
    if (blkCommand->getNameId() != commandId)
      continue;

    const char *id = blkCommand->getStr("id");
    const int commandIndex = getCommandIndex(id);
    if (commandIndex < 0)
    {
      logwarn("Editor command \"%s\" cannot be found. Ignoring customization.", id);
      continue;
    }

    const char *key = blkCommand->getStr("key");
    const eastl::optional<ImGuiKeyChord> keyChord = getKeyChordFromText(key);
    if (!keyChord.has_value())
    {
      logerr("Ignoring unsupported key chord \"%s\" for command \"%s\".", key, id);
      continue;
    }

    // Clear the hotkeys when loading the first hotkey for the command.
    if (loadedCmmandIndices.addInt(commandIndex))
      commands[commandIndex].hotkeys.clear();

    if (keyChord.value() != ImGuiKey_None)
      commands[commandIndex].hotkeys.push_back().keyChord = keyChord.value();
  }
}

void EditorCommands::saveChangedHotkeys(DataBlock &blk) const
{
  DataBlock *blkCommands = blk.addBlock("commands");

  iterate_names_in_lexical_order(idToIndexMap, [blkCommands, this](int index, const char *id) {
    const EditorCommand &command = commands[index];
    if (command.isUsingDefaultHotkeys())
      return;

    if (command.getHotkeyCount() > 0)
    {
      // We have to save all hotkeys if not using the default hotkeys.
      for (const EditorCommandHotkey &hotkey : command.hotkeys)
      {
        DataBlock *blkCommand = blkCommands->addNewBlock("command");
        blkCommand->addStr("id", id);
        blkCommand->addStr("key", hotkey.getKeyChordAsText());
      }
    }
    else
    {
      // If the default hotkey has been removed.
      DataBlock *blkCommand = blkCommands->addNewBlock("command");
      blkCommand->addStr("id", id);
      blkCommand->addStr("key", "");
    }
  });
}
