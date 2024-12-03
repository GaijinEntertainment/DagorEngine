import "%dngscripts/ecs.nut" as ecs
let {get_setting_by_blk_path} = require("settings")
let {has_network, INVALID_CONNECTION_ID} = require("net")
let dedicated = require_optional("dedicated")
let dagorDebug = require("dagor.debug")
let {register_logerr_monitor, debug, clear_logerr_interceptors} = dagorDebug
let {DBGLEVEL} = require("dagor.system")
let {mkEventSqChatMessage, CmdEnableDedicatedLogger, mkCmdEnableDedicatedLogger} = require("%scripts/globs/sqevents.nut")
let {INVALID_USER_ID}= require("%scripts/globs/types.nut")

let peersThatWantToReceiveQuery = ecs.SqQuery(
  "peersThatWantToReceiveQuery",
  {
    comps_ro = [["connid",ecs.TYPE_INT], ["receive_logerr", ecs.TYPE_BOOL]],
    comps_rq=["player"]
  },
  "and(ne(connid, {0}), receive_logerr)".subst(INVALID_CONNECTION_ID)
)

let getConnidForLogReceiver = @(_eid, comp) comp.connid

function sendLogToClients(log, connids=null){
  let event = mkEventSqChatMessage(({team=-1, name="dedicated", text=log}))
  if (!has_network())
    ecs.server_msg_sink(event, null)
  else {
    connids = connids==null ? (ecs.query_map(peersThatWantToReceiveQuery, getConnidForLogReceiver) ?? []) : connids
    if (connids.len()>0)
      ecs.server_msg_sink(event, connids)
  }
}


if (dedicated == null){
  ecs.register_es("enableLoggerrMsg",
    {
      [["onInit","onChange"]] = function(_evt,eid,comp) {
        if (!comp.is_local || comp.connid==INVALID_CONNECTION_ID)
          return
        let enable = (DBGLEVEL > 0 ) ? true : get_setting_by_blk_path("debug/receiveServerLogerr")
        debug($"ask for dedicated logerr: {enable}")
        ecs.client_send_event(eid, mkCmdEnableDedicatedLogger({on = enable ?? (DBGLEVEL > 0)}))
      }
    },
    {
      comps_rq=["player"],
      comps_track = [["connid",ecs.TYPE_INT], ["is_local", ecs.TYPE_BOOL]],
    },
    {tags="gameClient"}
  )
  return
}
clear_logerr_interceptors()

function sendErrorToClient(_tag, logstring, _timestamp) {
  debug($"sending {logstring} to")
  sendLogToClients(logstring)
}

register_logerr_monitor([""], sendErrorToClient)
ecs.register_es("enable_send_logerr_msg_es", {
    [CmdEnableDedicatedLogger] = function(evt,_eid,comp) {
      let on = evt.data?.on ?? false
      debug("setting logerr sending to '{3}', for connid:{0}, userid:{1}, username:'{2}'".subst(comp["connid"], comp["userid"], comp["name"], on))
      comp["receive_logerr"] = on
    }
  },
  {
    comps_ro = [
      ["name", ecs.TYPE_STRING, ""],
      ["connid", ecs.TYPE_INT],
      ["userid", ecs.TYPE_UINT64, INVALID_USER_ID],
    ]
    comps_rq = ["player"]
    comps_rw = [["receive_logerr", ecs.TYPE_BOOL]]
  },
  {tags = "server"}
)

/*
local i=0
ecs.set_callback_timer(
  function() {
    i++
    dagorDebug.logerr($"logerrnum: {i}")
  },
10, true)
*/
