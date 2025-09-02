import "math" as math
from "dagor.workcycle" import setTimeout, clearTimer
from "%sqstd/functools.nut" import kwarg
from "dagor.time" import get_time_msec
from "%sqstd/frp.nut" import Watched

//when event have parameter ttl it will be automatically removed on time finish
//isEventsEqual = @(event1, event2) bool - used only to remove events not only by uid.
//  Previous equal event will be removed on receive new event.
function mkEventLogState(persistId, maxActiveEvents = 10, defTtl = 0, isEventsEqual = null
) {
  let savedEvents = persist(persistId, @() { v = [] })
  let curEvents = Watched(savedEvents.v)
  curEvents.subscribe(@(v) savedEvents.v = v)
  local lastEventUid = curEvents.get()?[curEvents.get().len() - 1].uid ?? 0

  let getEqualIndex = @(event) isEventsEqual == null ? null
    : curEvents.get().findindex(@(e) isEventsEqual(event, e))

  function removeEvent(uidOrEvent) {
    let idx = type(uidOrEvent) == "integer" ? curEvents.get().findindex(@(e) e.uid == uidOrEvent)
      : getEqualIndex(uidOrEvent)
    if (idx != null)
      curEvents.mutate(@(list) list.remove(idx))
  }

  let timersCb = {}
  function startRemoveTimer(event) {
    local { ttl = defTtl, uid, removeMsec = null } = event
    if (uid in timersCb) {
      clearTimer(timersCb[uid])
      timersCb?.$rawdelete(uid)
    }
    if (ttl <= 0)
      return
    if (removeMsec == null) {
      removeMsec = get_time_msec() + (1000 * ttl).tointeger()
      event.removeMsec <- removeMsec
    }
    timersCb[uid] <- function() {
      removeEvent(uid)
      timersCb?.$rawdelete(uid)
    }
    setTimeout(math.max(0.001 * (removeMsec - get_time_msec()), 0.01), timersCb[uid])
  }
  curEvents.get().each(startRemoveTimer)

  function findFirstRemoveHint() {
    local time = null
    local resIdx = null
    foreach(idx, evt in curEvents.get()) {
      let { removeMsec = null } = evt
      if (resIdx != null
          && (removeMsec == null || (time != null && time < removeMsec)))
        continue
      resIdx = idx
      time = removeMsec
    }
    return resIdx
  }

  function addEvent(eventExt) {
    let uid = ++lastEventUid
    let event = eventExt.__merge({ uid })

    let idxToRemove = getEqualIndex(event)
      ?? (curEvents.get().len() >= maxActiveEvents ? findFirstRemoveHint() : null)

    curEvents.mutate(function(list) {
      if (idxToRemove != null)
        list.remove(idxToRemove)
      list.append(event)
    })
    startRemoveTimer(event)
  }

  function modifyOrAddEvent(eventExt, isEventToModify) {
    let idx = curEvents.get().findindex(isEventToModify)
    if (idx == null) {
      addEvent(eventExt)
      return
    }

    curEvents.mutate(function(list) {
      list[idx] = eventExt.__merge({ uid = list[idx].uid })
    })
    startRemoveTimer(curEvents.get()[idx])
  }

  let clearEvents = @() curEvents.set([])

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