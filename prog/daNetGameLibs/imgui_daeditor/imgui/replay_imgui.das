require ecs
require DagorImgui
require DagorConsole
require imgui

[imgui_window(name="Replay control", group="Replay")]
def replay_control()
  if IsWindowCollapsed()
    return
  let size = imgui::GetWindowSize()
  if size.x <= 200f || size.y <= 200f
    imgui::SetWindowSize(float2(200))
  imgui::Text("Replay control")
  BeginGroup()
  if imgui::Button("Pause")
    console_command("app.timeSpeed 0")

  imgui::SameLine()
  if imgui::Button("x1/4")
    console_command("app.timeSpeed 0.25")

  imgui::SameLine()
  if imgui::Button("x1/2")
    console_command("app.timeSpeed 0.5")

  imgui::SameLine()
  if imgui::Button("x1")
    console_command("app.timeSpeed 1")

  imgui::SameLine()
  if imgui::Button("x2")
    console_command("app.timeSpeed 2")

  imgui::SameLine()
  if imgui::Button("x4")
    console_command("app.timeSpeed 4")
  EndGroup()
