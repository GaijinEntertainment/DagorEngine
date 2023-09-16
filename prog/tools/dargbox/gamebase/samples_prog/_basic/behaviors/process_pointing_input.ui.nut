from "%darg/ui_imports.nut" import *
from "math" import sqrt, floor

let cursors = require("samples_prog/_cursors.nut")

let overlay = watchElemState(@(sf) {
  size = [pw(80), ph(80)]
  pos = [pw(20), ph(20)]
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  rendObj = ROBJ_BOX
  fillColor = (sf & S_ACTIVE) ? Color(200,120,120,180) : Color(200,120,120,220)
  borderColor = (sf & S_ACTIVE) ? Color(255,255,255) : Color(200,120,120)
  borderWidth = 2
  behavior = Behaviors.Button

  children = [
    {rendObj = ROBJ_TEXT text="Button overlay" fontSize=sh(5) padding=sh(2)}
  ]
})


let processorState = Watched({
  devId = null
  pointerId = null
  btnId = null
  x0 = null
  y0 = null
  dragDistance = 0
})

let processor = @() {
  rendObj = ROBJ_BOX
  fillColor = processorState.value.devId!=null ? Color(120,240,100) : Color(160,220,120)
  borderWidth = 2
  borderColor = processorState.value.devId!=null ? Color(255,255,255) : Color(50,240,50)
  size = [pw(80), ph(80)]
  behavior = Behaviors.ProcessPointingInput
  watch = processorState

  function onPointerPress(evt) {
    if (evt.accumRes & R_PROCESSED) {
      dlog("Click was processed above, skipping")
      return null
    }

    if (!evt.hit) {
      dlog("Clicked outside of element, skipping")
      return null
    }

    if (processorState.value.devId!=null) {
      dlog("Already pressed by another pointer/device, skipping")
      return null
    }

    processorState.mutate(function(v) {
      v.devId = evt.devId
      v.pointerId = evt.pointerId
      v.btnId = evt.btnId
      v.x0 = evt.x
      v.y0 = evt.y
    })

    dlog("Press | start processing")

    return R_PROCESSED // notify undelying elements
  }

  function onPointerRelease(evt) {
    //dlog("onPointerRelease", evt)
    let state = processorState.value
    if (evt.devId != state.devId || evt.pointerId != state.pointerId || evt.btnId!=state.btnId) {
      dlog("Not pressed by this pointer/device, skipping")
      return null
    }

    processorState.mutate(function(v) {
      v.devId = null
      v.pointerId = null
      v.btnId = null
      v.dragDistance = 0
    })
    dlog("Release | stop processing")
    return R_PROCESSED
  }

  function onPointerMove(evt) {
    //dlog("onPointerMove", evt)
    let state = processorState.value
    if (evt.devId == state.devId && evt.pointerId == state.pointerId && (evt.btnId==state.btnId || evt.devId == DEVID_MOUSE)) {
      processorState.mutate(function(v) {
        let dx = evt.x - v.x0
        let dy = evt.y - v.y0
        v.dragDistance = sqrt(dx*dx+dy*dy)
      })
    }
  }

  children = [
    {
      rendObj = ROBJ_TEXT
      text=$"Input processor | dragged distance = {floor(processorState.value.dragDistance)}"
      fontSize=sh(5)
      padding=sh(2)
    }
    overlay
  ]
}


let root = {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(60,70,80)

  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = [
    processor
  ]

  cursor = cursors.normal
}

return root
