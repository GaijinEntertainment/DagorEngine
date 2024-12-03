import "%dngscripts/ecs.nut" as ecs

let console = require("console")
let {INVALID_USER_ID}= require("%scripts/globs/types.nut")
let {find_local_player, find_human_player_by_connid} = require("%dngscripts/common_queries.nut")
let {has_network, INVALID_CONNECTION_ID} = require("net")
let {startswith} = require("string")
let {CmdChatMessage, mkEventSqChatMessage} = require("%scripts/globs/sqevents.nut")

let peersThatWantToReceiveQuery = ecs.SqQuery("peersThatWantToReceiveQuery",
  {
    comps_ro = [["connid",ecs.TYPE_INT], ["receive_logerr", ecs.TYPE_BOOL]],
    comps_rq=["player"]
  },
  "and(ne(connid, {0}), receive_logerr)".subst(INVALID_CONNECTION_ID)
)

let getConnidForLogReceiver = @(_eid, comp) comp.connid

function sendLogToClients(log, connids=null){
  let event = mkEventSqChatMessage(({name="dedicated", text=log}))
  if (!has_network())
    ecs.server_msg_sink(event, null)
  else {
    connids = connids==null ? (ecs.query_map(peersThatWantToReceiveQuery, getConnidForLogReceiver) ?? []) : connids
    if (connids.len()>0)
      ecs.server_msg_sink(event, connids)
  }
}

let connidsQuery = ecs.SqQuery("connidsQuery", {comps_ro=[["connid",ecs.TYPE_INT]], comps_rq=["player"]})

let getPlayerPossessedQuery = ecs.SqQuery("getPlayerPossessedQuery", { comps_ro=[["possessed", ecs.TYPE_EID]] })

function find_connids_to_send(){
  let connids = []
  connidsQuery.perform(function(_eid, comp){
    connids.append(comp["connid"])
  }, $"ne(connid,{INVALID_CONNECTION_ID})")
  return connids
}


const SERVERCMD_PREFIX = "/servercmd"
const AUTOREPLACE_HERO = ":hero:"
const AUTOREPLACE_PLAYER = ":player:"

function sendMessage(evtData){
  let net = has_network()
  let sender = net ? find_human_player_by_connid(evtData?.fromconnid ?? INVALID_CONNECTION_ID) : find_local_player()
  let name = ecs.obsolete_dbg_get_comp_val(sender, "name", "")
  let hero = getPlayerPossessedQuery.perform(sender, @(_, comp) comp["possessed"]) ?? ecs.INVALID_ENTITY_ID
  let senderUserId = ecs.obsolete_dbg_get_comp_val(sender, "userid", INVALID_USER_ID)
  if (startswith(evtData?.text ?? "", SERVERCMD_PREFIX)) {
    local text = evtData.text.slice(SERVERCMD_PREFIX.len())
    text = text.replace(AUTOREPLACE_HERO, $"{hero}")
    text = text.replace(AUTOREPLACE_PLAYER, $"{sender}")
    console.command($"net.set_console_connection_id {evtData?.fromconnid ?? -1}")
    sendLogToClients($"{name}: {text}")
    console.command(text)
    println($"console command '{text}' received userid:{senderUserId}")
    return
  }

  let event = mkEventSqChatMessage({ name, sender, senderUserId, text=evtData?.text ?? "" })
  let connids = find_connids_to_send()
  if (connids==null)
    return
  ecs.server_msg_sink(event, connids)
}

ecs.register_es("chat_server_es", {
    [CmdChatMessage] = @(evt,_eid,_comp) sendMessage(evt.data)
  },
  {comps_rq=["msg_sink"]}
)
