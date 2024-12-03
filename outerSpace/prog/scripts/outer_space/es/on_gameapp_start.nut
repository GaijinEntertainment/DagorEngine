import "%dngscripts/ecs.nut" as ecs
let {get_arg_value_by_name, dgs_get_settings} = require("dagor.system")
let dedicated = require_optional("dedicated")
let {EventOnGameAppStarted} = require("gameevents")
let {connect_to_session, switch_scene} = require("app")

let is_offline_mode_forced = @() dgs_get_settings()?.disableMenu
const empty = "gamedata/scenes/menu.blk"

function on_gameapp_started(_evt, _eid, _comp) {
  let settings = dgs_get_settings()
  let connect = (dedicated == null) ? get_arg_value_by_name("connect") : null
  if (connect != null)
    connect_to_session({host_urls = [connect]})
  else if (dedicated != null || settings?.disableMenu || is_offline_mode_forced())
    switch_scene(get_arg_value_by_name("scene") ?? settings?.scene ?? empty)
  else
    switch_scene("gamedata/scenes/menu.blk")
}

ecs.register_es("on_gameapp_started_es",
  {
    [EventOnGameAppStarted] = on_gameapp_started,
  })
