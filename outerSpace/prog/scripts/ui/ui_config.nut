let {gui_scene} = require("daRg")

gui_scene.setConfigProps({
  //defaultFont = sub_txt.font
  //defaultFontSize = sub_txt.fontSize
  reportNestedWatchedUpdate = false
  kbCursorControl = true
  gamepadCursorSpeed = 1.85
  //gamepadCursorDeadZone - depending on driver deadzon in controls_setup, default is setup in library

  gamepadCursorNonLin = 0.5
  gamepadCursorHoverMinMul = 0.07
  gamepadCursorHoverMaxMul = 0.8
  gamepadCursorHoverMaxTime = 1.0
  //defSceneBgColor =Color(10,10,10,160)
  //gamepadCursorControl = true

  //clickRumbleLoFreq = 0
  //clickRumbleHiFreq = 0.7
  clickRumbleLoFreq = 0
  clickRumbleHiFreq = 0.8
  clickRumbleDuration = 0.04
})
