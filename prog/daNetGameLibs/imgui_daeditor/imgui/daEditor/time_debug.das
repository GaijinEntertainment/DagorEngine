options no_aot

require app
require DagorImgui
require imgui/imgui_boost
require daslib/strings_boost


[imgui_window(name="Time", group="General")]
def time_window()
  if IsWindowCollapsed()
    return
  let timeScales <- [[auto 0.0; 0.01; 0.1; 0.25; 0.5; 1.0; 2.0; 4.0; 10.0; 20.0]]
  let timeF = format("%.2f", get_sync_time())
  let scaleF = format("%.2f", get_timespeed())
  imgui::TextUnformatted("sync time: {timeF} (x{scaleF})")
  imgui::Text("x:")
  for scale in timeScales
    SameLine()
    let sF = format("%.2f", scale)
    if imgui::Button("{sF}##timeScale")
      set_timespeed(scale)