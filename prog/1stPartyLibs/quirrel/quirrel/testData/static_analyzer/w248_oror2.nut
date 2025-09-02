

function onDoubleClick(itemDesc, event, isLocked, isInteractive, onDoubleClickCb = null, nullNotH = null) {
    if (isLocked || !isInteractive || onDoubleClickCb == null || nullNotH != null)
      return
    onDoubleClickCb(itemDesc.__merge({ rectOrPos = event.targetRect }))
    nullNotH()
  }


onDoubleClick(1, 2, 3, 4, 5)