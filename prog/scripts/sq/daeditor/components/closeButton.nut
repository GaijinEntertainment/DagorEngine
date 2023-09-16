from "%darg/ui_imports.nut" import *
let textButton = require("textButton.nut")

let closeButton = @(handler) textButton("X", handler, { boxStyle = { normal = { margin = hdpx(1) }}})

return closeButton
