// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace PropPanel
{
class ContainerPropertyControl;
class IMenu;
class PropertyControlBase;
} // namespace PropPanel

class DataBlock;
typedef int ImGuiKeyChord;

class IEditorCommandKeyChordChangeEventHandler
{
public:
  virtual void onEditorCommandKeyChordChanged() = 0;

  static constexpr unsigned HUID = 0x3D84B63Cu; // IEditorCommandKeyChordChangeEventHandler
};

class IEditorCommandSystem
{
public:
  virtual void addCommand(const char *id) = 0;
  virtual void addCommand(const char *id, ImGuiKeyChord key_chord) = 0;
  virtual void addCommand(const char *id, ImGuiKeyChord key_chord1, ImGuiKeyChord key_chord2) = 0;

  // Returns null if the command cannot be found.
  // The returned text is temporary. Do not store it.
  virtual const char *getCommandKeyChordsAsText(const char *id) = 0;

  virtual unsigned int getCommandCount() const = 0;
  virtual const char *getCommandId(int index) const = 0;
  virtual unsigned int getCommandCmdId(const char *id) = 0;

  virtual void loadChangedHotkeys(const DataBlock &blk) = 0;
  virtual void saveChangedHotkeys(DataBlock &blk) const = 0;

  virtual void createToolbarButton(PropPanel::ContainerPropertyControl &parent, int id, const char *editor_command_id,
    const char *tooltip) = 0;
  virtual void createToolbarRadioButton(PropPanel::ContainerPropertyControl &parent, int id, const char *editor_command_id,
    const char *tooltip) = 0;
  virtual void createToolbarToggleButton(PropPanel::ContainerPropertyControl &parent, int id, const char *editor_command_id,
    const char *tooltip) = 0;
  virtual void updateToolbarButtons(PropPanel::ContainerPropertyControl &container) = 0;

  virtual void addMenuItem(PropPanel::IMenu &menu, unsigned menu_id, unsigned item_id, const char *editor_command_id,
    const char *title) = 0;
  virtual void updateMenuItemShortcuts(PropPanel::IMenu &menu) = 0;

  virtual void enableMissingCommandLogging() = 0;

  // Renders the hotkey editor and the context menu of the toolbar buttons.
  virtual void updateImgui() = 0;

  static constexpr unsigned HUID = 0x167633F1u; // IEditorCommandSystem
};

class EditorCommandSystem : public IEditorCommandSystem
{
public:
  void addCommand(const char *id) override;
  void addCommand(const char *id, ImGuiKeyChord key_chord) override;
  void addCommand(const char *id, ImGuiKeyChord key_chord1, ImGuiKeyChord key_chord2) override;
  const char *getCommandKeyChordsAsText(const char *id) override;
  unsigned int getCommandCount() const override;
  const char *getCommandId(int index) const override;
  unsigned int getCommandCmdId(const char *id) override;
  void loadChangedHotkeys(const DataBlock &blk) override;
  void saveChangedHotkeys(DataBlock &blk) const override;
  void createToolbarButton(PropPanel::ContainerPropertyControl &parent, int id, const char *editor_command_id,
    const char *tooltip) override;
  void createToolbarRadioButton(PropPanel::ContainerPropertyControl &parent, int id, const char *editor_command_id,
    const char *tooltip) override;
  void createToolbarToggleButton(PropPanel::ContainerPropertyControl &parent, int id, const char *editor_command_id,
    const char *tooltip) override;
  void updateToolbarButtons(PropPanel::ContainerPropertyControl &container) override;
  void addMenuItem(PropPanel::IMenu &menu, unsigned menu_id, unsigned item_id, const char *editor_command_id,
    const char *title) override;
  void updateMenuItemShortcuts(PropPanel::IMenu &menu) override;
  void enableMissingCommandLogging() override;
  void updateImgui() override;
};
