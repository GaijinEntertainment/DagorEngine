
//THIS FILE CREATED BY CODEGEN, DON'T CHANGE THIS!!! USE setup_dng_libs.py INSTEAD!!!


options no_aot = true
require danetlibs.render_debug.render_debug_init
require danetlibs.imgui_daeditor.imgui_daeditor_init
require danetlibs.console_commands.console_commands_init
require danetlibs.native_dasevents.native_dasevents_init
require danetlibs.frame_graph_renderer.frame_graph_renderer_init
require danetlibs.flash_blind.flash_blind_init
require danetlibs.light_flicker.light_flicker_init
require danetlibs.puddles_manager.puddles_manager_init
require danetlibs.local_tone_mapping.local_tone_mapping_init
require danetlibs.paint_color.paint_color_init
require danetlibs.sound.common_sounds.common_sounds_init
require danetlibs.sound.environment_sounds.environment_sounds_init
require danetlibs.sound.sound_utils.sound_utils_init
require danetlibs.sound.sound_utils_net.sound_utils_net_init

def load_dng_libs() : bool
  var ok = true
  ok = load_render_debug("%danetlibs/render_debug") && ok
  ok = load_imgui_daeditor("%danetlibs/imgui_daeditor") && ok
  ok = load_console_commands("%danetlibs/console_commands") && ok
  ok = load_native_dasevents("%danetlibs/native_dasevents") && ok
  ok = load_frame_graph_renderer("%danetlibs/frame_graph_renderer") && ok
  ok = load_flash_blind("%danetlibs/flash_blind") && ok
  ok = load_light_flicker("%danetlibs/light_flicker") && ok
  ok = load_puddles_manager("%danetlibs/puddles_manager") && ok
  ok = load_local_tone_mapping("%danetlibs/local_tone_mapping") && ok
  ok = load_paint_color("%danetlibs/paint_color") && ok
  ok = load_common_sounds("%danetlibs/sound/common_sounds") && ok
  ok = load_environment_sounds("%danetlibs/sound/environment_sounds") && ok
  ok = load_sound_utils("%danetlibs/sound/sound_utils") && ok
  ok = load_sound_utils_net("%danetlibs/sound/sound_utils_net") && ok

  return ok

[export]
def test_all
  let ok = load_dng_libs()
  assert(ok)

