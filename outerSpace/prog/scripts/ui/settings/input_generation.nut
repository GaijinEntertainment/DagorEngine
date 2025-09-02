let { eventbus_subscribe } = require("eventbus")
let {Watched} = require("frp")
let controlsGeneration = Watched({})
let controlsGenerationUpdate = @(...) controlsGeneration.set({})
const CONTROLS_SETUP_CHANGED_EVENT_ID = "controls_setup_changed"
eventbus_subscribe(CONTROLS_SETUP_CHANGED_EVENT_ID, controlsGenerationUpdate)

return { CONTROLS_SETUP_CHANGED_EVENT_ID, controlsGeneration, controlsGenerationUpdate }
