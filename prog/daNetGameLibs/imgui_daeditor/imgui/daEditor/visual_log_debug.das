options no_aot

require DagorImgui
require DagorConsole
require imgui/imgui_boost


[imgui_window(name="Visual Log", group="General")]
def visual_log_window()
  if IsWindowCollapsed()
    return

  imgui_set_mono_font()

  let stickToBottom = imgui::GetScrollY() == imgui::GetScrollMaxY()

  visual_log_get_history() <| $(list)
    for s in list
      imgui::TextWrapped(s)
      imgui::Separator()

  imgui_set_default_font()

  if stickToBottom
    imgui::SetScrollHereY(1.0)
