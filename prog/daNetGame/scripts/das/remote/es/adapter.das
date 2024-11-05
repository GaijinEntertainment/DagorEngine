options stack = 102400
require ecs
require app
require debugapi

require remote.server_agent


[init, export, pinvoke]
def auto_start()
  unsafe
    lock_debug_agent() <| $()
      if app::get_game_name() != "aot"
        start_agent()


[es(tag=dasDebug, no_order)]
def update_web_agent(act : UpdateStageInfoAct)
  tick_agent()
