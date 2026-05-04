options no_aot
module mm_ui_imgui_helpers shared
require imgui
require imgui/imgui_boost
require daslib/safe_addr

def safe_slider(title : string; var value : float&; min_value, max_value : float)
  var tmpValue = value
  let result = imgui::SliderFloat(title, safe_addr(tmpValue), min_value, max_value)
  value = tmpValue
  return result

def safe_checkbox(title : string; var value : bool&)
  var tmpValue = value
  let result = imgui::Checkbox(title, safe_addr(tmpValue))
  value = tmpValue
  return result

def imgui_input_text(text : string; var value : das_string&)
  let buf <- ImGuiInputTextBuffer("{value}", 256)
  if InputText(text, buf, ImGuiInputTextFlags.AutoSelectAll)
    value := buf |> to_string()
    return true
  return false
