let { Watched } = require("%sqstd/frp.nut")
let { setTimeout, clearTimer } = require("dagor.workcycle")
let { kwarg } = require("%sqstd/functools.nut")
let { get_time_msec } = require("dagor.time")
let math = require("math")

//when event have parameter ttl it will be automatically removed on time finish
//isEventsEqual = @(event1, event2) bool - used only to remove events not only by uid.
//  Previous equal event will be removed on receive new event.
let function mkEventLogState(persistId, maxActiveEvents = 10, defTtl = 0, isEventsEqual = null
) {
  let savedEvents = persist(persistId, @() { v = [] })
  let curEvents = Watched(savedEvents.v)
  curEvents.subscribe(@(v) savedEvents.v = v)
  local lastEventUid = curEvents.value?[curEvents.value.len() - 1].uid ?? 0

  let getEqualIndex = @(event) isEventsEqual == null ? null
    : curEvents.value.findindex(@(e) isEventsEqual(event, e))

  let function removeEvent(uidOrEvent) {
    let idx = type(uidOrEvent) == "integer" ? curEvents.value.findindex(@(e) e.uid == uidOrEvent)
      : getEqualIndex(uidOrEvent)
    if (idx != null)
      curEvents.mutate(@(list) list.remove(idx))
  }

  let timersCb = {}
  let function startRemoveTimer(event) {
    local { ttl = defTtl, uid, removeMsec = null } = event
    if (uid in timersCb) {
      clearTimer(timersCb[uid])
      delete timersCb[uid]
    }
    if (ttl <= 0)
      return
    if (removeMsec == null) {
      removeMsec = get_time_msec() + (1000 * ttl).tointeger()
      event.removeMsec <- removeMsec
    }
    timersCb[uid] <- function() {
      removeEvent(uid)
      if (uid in timersCb)
        delete timersCb[uid]
    }
    setTimeout(math.max(0.001 * (removeMsec - get_time_msec()), 0.01), timersCb[uid])
  }
  curEvents.value.each(startRemoveTimer)

  let function findFirstRemoveHint() {
    local time = null
    local resIdx = null
    foreach(idx, evt in curEvents.value) {
      let { removeMsec = null } = evt
      if (resIdx != null
          && (removeMsec == null || (time != null && time > removeMsec)))
        continue
      resIdx = idx
      time = removeMsec
    }
    return resIdx
  }

  let function addEvent(eventExt) {
    let uid = ++lastEventUid
    let event = eventExt.__merge({ uid })

    let idxToRemove = getEqualIndex(event)
      ?? (curEvents.value.len() >= maxActiveEvents ? findFirstRemoveHint() : null)

    curEvents.mutate(function(list) {
      if (idxToRemove != null)
        list.remove(idxToRemove)
      list.append(event)
    })
    startRemoveTimer(event)
  }

  let function modifyOrAddEvent(eventExt, isEventToModify) {
    let idx = curEvents.value.findindex(isEventToModify)
    if (idx == null) {
      addEvent(eventExt)
      return
    }

    curEvents.mutate(function(list) {
      list[idx] = eventExt.__merge({ uid = list[idx].uid })
    })
    startRemoveTimer(curEvents.value[idx])
  }

  let clearEvents = @() curEvents([])

  foreach(func in [addEvent, modifyOrAddEvent, removeEvent, clearEvents])
    curEvents.whiteListMutatorClosure(func)

  return {
    curEvents
    addEvent
    modifyOrAddEvent
    removeEvent
    clearEvents
  }
}

return kwarg(mkEventLogState)