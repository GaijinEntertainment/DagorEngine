options no_aot
require ecs
require math.base
require strings
require imgui
require DagorImgui
require DagorEditor
require DagorConsole
require imgui/imgui_boost


[console_cmd(name="imgui.editor_create_window")]
def show_create_window()
  let isVisible = imgui_get_state() != ImGuiState OFF && imgui_window_is_visible("Editor", "Create entity")
  let setVisible = !isVisible
  console_command(setVisible ? "imgui.activate" : "imgui.off")
  imgui_window_set_visible("Editor", "Create entity", setVisible)


[init]
def init_create_window_shortcut()
  console_command("consoleKeybindings.bind ctrl+shift+[ imgui.editor_create_window")
