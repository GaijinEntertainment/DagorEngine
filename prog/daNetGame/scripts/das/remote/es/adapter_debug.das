options stack = 102400
options gc
options no_aot

require ecs
require app
require debugapi

require remote.server_agent


[init, export, pinvoke]
def auto_start()
  unsafe
    lock_debug_agent() <| $()
      if !is_in_aot()
        start_agent()


[es(tag=dasWebui, no_order)]
def update_web_agent(act : UpdateStageInfoAct)
  tick_agent()
