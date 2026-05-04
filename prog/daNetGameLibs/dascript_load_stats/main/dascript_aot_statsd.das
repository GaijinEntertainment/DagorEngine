require strings
require ecs
require statsd
require DagorSystem

[es(on_event=EventOnGameAppStarted)]
def send_aot_loading_statsd(evt : Event)
  if globally_enabled_aot() && get_DAGOR_THREAD_SANITIZER() == 0 && get_DAGOR_ADDRESS_SANITIZER() == 0
    let isDev = get_DAGOR_DBGLEVEL() >= 1
    if isDev && has_value(fixed_array("win32", "win64", "linux64", "macosx"), get_platform_string_id())  // On PC try measure only autotests
      if (find_index_if(get_command_line_arguments()) <| $(arg) => find(arg, "-quit_in_time") >= 0) < 0
        return
    statsd_profile("das.load_time_ms", int(das_load_time_ms()), "is_dev", "{isDev}")
    statsd_counter("das.memory_usage_kb", int(das_memory_usage() / 1024l), "is_dev", "{isDev}")
