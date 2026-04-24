// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_oaHashNameMap.h>
#include <util/dag_simpleString.h>
#include <dag/dag_vector.h>

#include <EASTL/optional.h>

class DataBlock;
class String;
typedef int ImGuiKeyChord;

struct EditorCommandHotkey
{
  const char *getKeyChordAsText() const;

  bool operator==(const EditorCommandHotkey &rhs) const = default;

  ImGuiKeyChord keyChord;
};

class EditorCommand
{
public:
  dag::ConstSpan<const EditorCommandHotkey> getDefaultHotkeys() const { return defaultHotkeys; }
  dag::ConstSpan<const EditorCommandHotkey> getHotkeys() const { return hotkeys; }
  int getHotkeyCount() const { return hotkeys.size(); }
  ImGuiKeyChord getKeyChord(int index) const { return hotkeys[index].keyChord; }
  const char *getKeyChordsAsText() const;
  bool isUsingDefaultHotkeys() const;
  unsigned int getCmdId() const { return cmdId; }

private:
  friend class EditorCommands;

  dag::Vector<EditorCommandHotkey> defaultHotkeys;
  dag::Vector<EditorCommandHotkey> hotkeys;

  unsigned int cmdId = 0;
};

class EditorCommands
{
public:
  void addCommand(const char *id);
  void addCommand(const char *id, ImGuiKeyChord key_chord);
  void addCommand(const char *id, ImGuiKeyChord key_chord1, ImGuiKeyChord key_chord2);

  const uint32_t getCommandCount() const { return commands.size(); }
  const EditorCommand *getCommand(const char *id) const;
  const dag::ConstSpan<const EditorCommand> getCommands() const { return commands; }
  const char *getCommandId(const EditorCommand &command) const;
  const char *getCommandId(int index) const;

  void setCommandCmdId(const char *id, unsigned int cmd_id);

  void addHotkey(const char *id, ImGuiKeyChord key_chord);
  void changeHotkey(const char *id, int hotkey_index, ImGuiKeyChord key_chord);
  void removeHotkey(const char *id, int hotkey_index);
  void removeAllHotkeys(const char *id);
  void resetToDefaultHotkeys(const char *id);

  void loadChangedHotkeys(const DataBlock &blk);
  void saveChangedHotkeys(DataBlock &blk) const;

private:
  int getCommandIndex(const char *id) const;
  void sendChangeNotification();

  static eastl::optional<ImGuiKeyChord> getModifierKeysFromText(const char *&text);
  static eastl::optional<ImGuiKeyChord> getKeyChordFromText(const char *text);

  dag::Vector<EditorCommand> commands;
  FastNameMapEx idToIndexMap;
};

extern EditorCommands ec_editor_commands;
