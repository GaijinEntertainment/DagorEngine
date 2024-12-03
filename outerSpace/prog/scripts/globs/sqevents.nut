let {registerUnicastEvent, registerBroadcastEvent } = require("%dngscripts/ecs.nut")

let broadcastEvents = {}
foreach (name, payload in {})
  broadcastEvents.__update(registerBroadcastEvent(payload, name))

let unicastEvents = {}
foreach(name, payload in {
  CmdChatMessage = {mode="team", text="", qmsg=null, sound=null},
  EventSqChatMessage = {team = "", name="", sender=0, text="", qmsg=null, sound=null},
  CmdEnableDedicatedLogger = { on = true },
})
  unicastEvents.__update(registerUnicastEvent(payload, name))

return freeze(broadcastEvents.__merge(unicastEvents))
