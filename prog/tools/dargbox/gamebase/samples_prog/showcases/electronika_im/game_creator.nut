from "%darg/ui_imports.nut" import *
from "%sqstd/functools.nut" import kwarg
from "dagor.time" import get_time_msec
from "dagor.random" import rnd, srnd
from "math" import min, max, clamp, fabs
from "iostream" import blob
from "gamelib.sound" import PcmSound, play_sound
from "eventbus" import eventbus_subscribe
from "datetime" import date

function makeSound(pitch, duration_s, tilt, volume) {
  const SAMPLE_RATE = 11025
  const NOISE_AMP   = 0.05

  let periods = (duration_s * pitch + 1e-9).tointeger()
  assert(periods > 0)

  let period_samps = max(4, ((SAMPLE_RATE / pitch) + 0.5).tointeger())
  let total_samps  = periods * period_samps

  let data = blob(total_samps * 4) // float32

  let half = (period_samps / 2).tointeger()

  tilt = clamp(tilt, 0.0, 0.49)
  local ramp = max(1, (tilt * half + 0.5).tointeger())
  ramp = min(ramp, max(half - 1, 1))
  // force even ramp so a sample lands exactly on the zero crossing
  if ((ramp & 1) != 0) ramp += (ramp < half - 1) ? 1 : -1

  let inv_ramp = 1.0 / ramp
  let phase0   = (ramp / 2).tointeger() // make n=0 hit the zero crossing

  for (local n = 0; n < total_samps; ++n) {
    let p = (n + phase0) % period_samps
    let h = (p < half) ? p : (p - half)
    let s = (p < half) ? -1.0 :  1.0
    let m = (h < ramp) ? h : ramp

    local v = s * (1.0 - 2.0 * m * inv_ramp)   // trapezoid in [-1, 1]
    v += NOISE_AMP * fabs(v) * srnd() // noise → 0 at zero crossings
    v = clamp(v * volume, -1.0, 1.0)

    data.writen(v, 'f')
  }

  data.seek(0)
  data.writen(0.0, 'f')
  data.seek((total_samps - 1) * 4)
  data.writen(0.0, 'f')

  return PcmSound({
    data = data
    freq = SAMPLE_RATE
    channels = 1
  })
}

function generateRandomNumber() {
  return rnd() % 16
}

enum GameMode {A, B}
enum DirFlags { Right=0x01, Bottom=0x02 }
const Sector = { LT=0, RT=DirFlags.Right, LB=DirFlags.Bottom, RB=DirFlags.Right|DirFlags.Bottom }

let NUM_TARGET_LINES = Sector.len()
const NUM_TARGET_STEPS = 5
const NUM_ESCAPE_STEPS = 4

const DEBUG_APPEARANCE = false // only to check the layout, enable to see all elements

let defaultKeys = {
  setSectorRT = "Insert"
  setSectorLT = "PageUp"
  setSectorRB = "Delete"
  setSectorLB = "PageDown"
  moveLeft = "Left"
  moveRight = "Right"
  moveDown = "Down"
  moveUp = "Up"
  gameA = "A"
  gameB = "B"
}

function mkPicCtors(width, aspect, baseImgPath, skinPath="") {
  let GAME_H = width / aspect
  let sz = @(dim) (GAME_H*dim/100).tointeger()
  local resSkinPath = "/".join([baseImgPath, skinPath].filter(@(v) v!=""))

  resSkinPath = resSkinPath!= "" && !resSkinPath.endswith("/") ? $"{resSkinPath}/" : resSkinPath
  let makePic = @(name, dim) Picture($"!{resSkinPath}{name}.svg:{sz(dim)}:{sz(dim)}:K")
  function makeFixedSprite(pic_name, pic_dim, params) {
    return params.__merge({
      rendObj = ROBJ_IMAGE
      image = makePic(pic_name, pic_dim)
      keepAspect = KEEP_ASPECT_FIT
      //size = [sz(pic_dim), sz(pic_dim)]
      //size = SIZE_TO_CONTENT
      imageAffectsLayout = true //< to infer sizes from a rasterized image
    })
  }
  let mkFont = function(name) {
    let size = type(name) == "integer" ? [6000/1296.0*2, 10000/852.0*2].map(sz) : [8, 10000/852.0*2].map(sz)
    return {
      rendObj = ROBJ_IMAGE
      keepAspect = KEEP_ASPECT_FIT
      image = name!=" " ? Picture($"!{baseImgPath}/font/{name}.svg:{size[0]}:{size[1]}:K") : null
      size
    }
  }
  let fontsSprites = [0,1,2,3,4,5,6,7,8,9, "colon", " "].totable().map(mkFont)
  return {makePic, GAME_H, sz, makeFixedSprite, fontsSprites}
}

let makeGameScreen = kwarg(function(game_name:string, sprites:table, width:number=sh(100), aspect:number=5.0/3, keys:table|null=defaultKeys) {
  keys = keys ?? defaultKeys
  let {targetSprites, overlay, missesSprites, saver, subject_left, subject_right, subject_lt, subject_rb, subject_lb, subject_rt, failsSprites, escapeSprites, fontsSprites }= sprites
  let GAME_W = width
  let {GAME_H, sz} = mkPicCtors(width, aspect, "")


  let curSector = Watched(Sector.RT)
  let score = Watched(null)
  let isViewerShown = Watched(DEBUG_APPEARANCE ? true : false)
  let targetsState = Watched(array(NUM_TARGET_LINES).map(@(_) array(NUM_TARGET_STEPS, DEBUG_APPEARANCE ? true : false)))
  let gameMode = Watched(null)
  let gameOver = Watched(false)
  let switchedOff = Watched(false)
  let misses = Watched(DEBUG_APPEARANCE ? 3 : 0)
  let isHalfMiss = Watched(false)
  // Last fail on each side
  let fails = Watched([DEBUG_APPEARANCE ? true : false, DEBUG_APPEARANCE ? true : false]) // left, right
  // Targets spawned after a half-miss
  let escapes = Watched(array(2).map(@(_) array(NUM_ESCAPE_STEPS, DEBUG_APPEARANCE ? true : false))) // left, right

  local prevTime = null
  local gameTimeAccumulator = 0.0
  local gameUpdateInterval = 0.5
  local targetCaughtAsync = false

  let soundTargetsStep = makeSound(880, 0.025, 0.1, 0.3)
  let soundTargetCaught = makeSound(660, 0.1, 0.1, 0.3)
  let soundTargetFail = makeSound(440, 0.2, 0.1, 0.6)
  let soundGameOver = makeSound(220, 0.4, 0.1, 0.6)

  function getMaxTargets() {
    let s = score.get() ?? 0
    if (s > 160) return 12
    else if (s >= 140) return 9
    else if (s >= 110) return 7
    else if (s >= 90) return 5
    else if (s >= 50) return 4
    else if (s >= 10) return 3
    else if (s >= 5 && s <= 9) return 2
    else return 1
  }

  function getTargetInDanger() {
    let targets = targetsState.get()
    for (local dir = 0; dir < NUM_TARGET_LINES; dir++) {
      if (targets[dir][NUM_TARGET_STEPS-1]) {
        return dir  // Return which dir has target in danger
      }
    }
    return null
  }

  function updateViewerVisibility(totalTargets) {
    if (totalTargets <= 1 && generateRandomNumber() >= 15) {
      isViewerShown.set(true)
    } else if (totalTargets >= 2) {
      isViewerShown.set(false)
    }
  }

  function isDirAllowed(dir) {
    // Game A authentic logic: certain dirs disabled based on miss count
    if (gameMode.get() != GameMode.A)
      return true

    let missCount = misses.get()
    return !((dir == Sector.LT && missCount == 2) ||
             (dir == Sector.RT && missCount == 3) ||
             (dir == Sector.LB && missCount == 0) ||
             (dir == Sector.RB && missCount == 1))
  }

  function countAllTargets() {
    local count = 0
    let targets = targetsState.get()
    for (local line = 0; line < NUM_TARGET_LINES; line++) {
      for (local step = 0; step < NUM_TARGET_STEPS; step++) {
        if (targets[line][step])
          count++
      }
    }
    return count
  }

  function shiftTargets() {
    targetsState.mutate(function(targets) {
      for (local dir = 0; dir < NUM_TARGET_LINES; ++dir) {
        // Shift all targets down one position
        for (local i = NUM_TARGET_STEPS-1; i > 0; i--)
          targets[dir][i] = targets[dir][i-1]
        targets[dir][0] = false
      }
    })
  }

  function shiftEscapes() {
    escapes.mutate(function(runs) {
      for (local side = 0; side < 2; side++) {
        for (local i = NUM_ESCAPE_STEPS-1; i > 0; i--)
          runs[side][i] = runs[side][i-1]
        runs[side][0] = false
      }
    })
  }

  function checkAndCatchTarget() {
    let targetInDanger = getTargetInDanger()
    if (targetInDanger == curSector.get()) {
      // Target caught! Remove it from the bottom position
      targetsState.mutate(@(targets) targets[targetInDanger][NUM_TARGET_STEPS-1] = false)

      // Update score
      local newScore = (score.get() ?? 0) + 1
      if (newScore > 999)
        newScore = 0

      // Clear misses at 200 and 500 points
      if (newScore == 200 || newScore == 500) {
        misses.set(0)
        isHalfMiss.set(false)
      }

      score.set(newScore)

      // Update timing based on score change
      let prevScore = newScore - 1
      let prevHundreds = (prevScore / 100).tointeger()
      let newHundreds = (newScore / 100).tointeger()
      let prevTens = ((prevScore / 10).tointeger()) % 10
      let newTens = ((newScore / 10).tointeger()) % 10

      if (prevHundreds != newHundreds) {
        if (newHundreds != 9) {
          // Slow down when turning hundreds (except 899->900)
          gameUpdateInterval = gameUpdateInterval + 0.2
        }
      } else if (prevTens != newTens) {
        // Speed up every 10 points
        local newInterval = max(gameUpdateInterval - 0.02, 0.1)
        gameUpdateInterval = newInterval
      }

      return true
    }
    return false
  }


  let clearAllTargets = @() targetsState.set(array(NUM_TARGET_LINES).map(@(_) array(NUM_TARGET_STEPS, false)))

  function perFrameUpdate(_counter) {
    let t = get_time_msec()
    if (prevTime == null) {
      prevTime = t
      return
    }

    if (switchedOff.get() || gameMode.get() == null)
      return

    let dt = (t - prevTime)*0.001
    prevTime = t
    gameTimeAccumulator += dt
    if (gameTimeAccumulator < gameUpdateInterval)
      return

    gameTimeAccumulator = 0.0
    if (gameOver.get()) {
      if (gameTimeAccumulator>120) {
        gameMode.set(null)
        gameOver.set(false)
        return
      }
      else
        return
    }
    // Clear previous fails (so they display for one update cycle)
    fails.set([false, false])

    // Reset sound flags for this update
    local targetFailed = false
    local targetCaught = targetCaughtAsync
    targetCaughtAsync = false

    // Try to catch any target that reached the bottom
    if (checkAndCatchTarget()) {
      targetCaught = true
    }
    else {
      // If no target was hit, check if any failed
      let targetInDanger = getTargetInDanger()
      if (targetInDanger != null) {
        // Target failed!
        let side = (targetInDanger & DirFlags.Right) ? 1 : 0
        fails.mutate(@(v) v[side] = true)
        targetFailed = true

        // Spawn ecape if viewer is visible
        if (isViewerShown.get()) {
          escapes.mutate(@(runs) runs[side][0] = true)
        }

        clearAllTargets()

        // Handle miss counter (authentic logic)
        if (isViewerShown.get()) {
          if (isHalfMiss.get())
            isHalfMiss.set(false)
          else {
            misses.set(misses.get() + 1)
            isHalfMiss.set(true)
          }
        }
        else {
          if (misses.get() >= 3 && isHalfMiss.get())
            isHalfMiss.set(false)
          else
            misses.set(misses.get() + 1)
        }

        // Game over check
        if (misses.get() >= 3 && !isHalfMiss.get()) {
          gameMode.set(null)
          isHalfMiss.set(false)
          play_sound(soundGameOver)
          return
        }
      }
    }

    // Move animations
    shiftEscapes()

    // Move all targets down
    shiftTargets()

    // Try spawning 1 new target (max 1 per update to keep targets catchable)
    local totalTargets = countAllTargets()
    local maxTargets = getMaxTargets()

    // Pick a random dir and try to spawn there
    local dir = generateRandomNumber() % NUM_TARGET_LINES
    if (isDirAllowed(dir) &&
        !targetsState.get()[dir][0] &&
        totalTargets < maxTargets &&
        (generateRandomNumber() >= 11 || totalTargets == 0)) {
      targetsState.mutate(@(targets) targets[dir][0] = true)
    }

    // Update viewer visibility based on activity
    updateViewerVisibility(totalTargets)

    if (targetFailed)
      play_sound(soundTargetFail)
    else if (targetCaught)
      play_sound(soundTargetCaught)
    else
      play_sound(soundTargetsStep)
  }

  function newGame(mode) {
    gameOver.set(false)
    gameMode.set(mode)
    score.set(0)
    misses.set(0)
    isHalfMiss.set(false)

    clearAllTargets()
    fails.set([false, false])
    escapes.set(array(2).map(@(_) array(NUM_ESCAPE_STEPS, false)))

    gameTimeAccumulator = 0.0
    targetCaughtAsync = false
    if (mode == GameMode.A) {
      gameUpdateInterval = 0.5 // Game A starts slower
    } else {
      gameUpdateInterval = 0.4 // Game B starts faster
    }

    isViewerShown.set(false)
  }

  function moveSubject(sector) {
    if (gameMode.get() == null)
      return
    curSector.set(sector)
    targetCaughtAsync = checkAndCatchTarget()
  }

  let gameEvents = {
    new_game_evt = $"{game_name}_NEW_GAME"
    setSector = $"{game_name}_set_dir"
    sector_RT = $"{game_name}_dir_up_right"
    sector_LT = $"{game_name}_dir_up_left"
    sector_RB = $"{game_name}_dir_down_right"
    sector_LB = $"{game_name}_dir_down_left"
    moveLeft = $"{game_name}_move_left"
    moveRight = $"{game_name}_move_right"
    moveUp = $"{game_name}_move_up"
    moveDown = $"{game_name}_move_down"
    gameA = $"{game_name}_game_a"
    gameB = $"{game_name}_game_b"
    switchOn = $"{game_name}_switch_on"
    switchOff = $"{game_name}_switch_off"
  }
  let setSectorRT = @(...) moveSubject(Sector.RT)
  let setSectorLT = @(...) moveSubject(Sector.LT)
  let setSectorRB = @(...) moveSubject(Sector.RB)
  let setSectorLB = @(...) moveSubject(Sector.LB)
  let moveLeft = @(...) moveSubject(curSector.get() & ~DirFlags.Right)
  let moveRight = @(...) moveSubject(curSector.get() | DirFlags.Right)
  let moveUp = @(...) moveSubject(curSector.get() & ~DirFlags.Bottom)
  let moveDown = @(...) moveSubject(curSector.get() | DirFlags.Bottom)
  let gameA = @(...) newGame(GameMode.A)
  let gameB = @(...) newGame(GameMode.B)
  let switchOn = @(...) switchedOff.set(false)
  let switchOff = @(...) switchedOff.set(true)
  eventbus_subscribe(gameEvents.new_game_evt, @(v=null) newGame(v?.mode ?? GameMode.A))
  eventbus_subscribe(gameEvents.setSector, function(v=null) {if ("sector" in v) moveSubject(v.sector)})
  eventbus_subscribe(gameEvents.sector_RT, setSectorRT)
  eventbus_subscribe(gameEvents.sector_LT, setSectorLT)
  eventbus_subscribe(gameEvents.sector_RT, setSectorRT)
  eventbus_subscribe(gameEvents.sector_LT, setSectorLT)
  eventbus_subscribe(gameEvents.moveLeft, moveLeft)
  eventbus_subscribe(gameEvents.moveRight, moveRight)
  eventbus_subscribe(gameEvents.moveUp, moveUp)
  eventbus_subscribe(gameEvents.moveDown, moveDown)
  eventbus_subscribe(gameEvents.gameA, gameA)
  eventbus_subscribe(gameEvents.gameB, gameB)
  eventbus_subscribe(gameEvents.switchOn, switchOn)
  eventbus_subscribe(gameEvents.switchOff, switchOff)

  let keysMap = {setSectorRT,setSectorLT,setSectorRB,setSectorLB,moveLeft,moveRight,moveDown,moveUp,gameA,gameB}
  let hotkeys = []
  foreach (hotkeyName, hotkey in keys) {
    if (hotkeyName not in keysMap) {
      logerr($"unknown action {hotkeyName}")
      continue
    }
    hotkeys.append([hotkey, keysMap[hotkeyName]])
  }

  function subject() {
    let sectorVal = curSector.get()
    return {
      watch = curSector
      size = flex()
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      children = [
        (sectorVal==Sector.LT || sectorVal==Sector.LB) ? subject_left : null,
        (sectorVal==Sector.RT || sectorVal==Sector.RB) ? subject_right : null,
        (sectorVal==Sector.LT) ? subject_lt : null,
        (sectorVal==Sector.LB) ? subject_lb : null,
        (sectorVal==Sector.RT) ? subject_rt : null,
        (sectorVal==Sector.RB) ? subject_rb : null,
      ]
    }
  }

  function scoreBlock() {
    let children = []
    let gap = {size= [sz(3), 0]}
    let curScore = score.get()
    if (curScore == null){
      let dat = date()
      let hour = dat.hour > 12 ? dat.hour%12 : dat.hour
      let mins = dat.min
      if (hour>9)
        children.append(fontsSprites[hour/10], gap, fontsSprites[hour%10])
      else
        children.append(fontsSprites[hour])
      children.append(fontsSprites["colon"])
      if (mins>9)
        children.append(fontsSprites[mins/10], gap, fontsSprites[mins%10])
      else
        children.append(fontsSprites[0], gap, fontsSprites[mins])
    }
    else {
      if (curScore > 99){
        children.append(fontsSprites[curScore/100], fontsSprites[" "], fontsSprites[(curScore%100)/10], gap, fontsSprites[curScore%10])
      }
      else if (curScore > 9){
        children.append(fontsSprites[curScore/10], gap, fontsSprites[curScore%10])
      }
      else{
        children.append(fontsSprites[curScore])
      }
    }
    return {
      watch = score
      pos = [sz(140), sz(0)]
      size = 0
      halign = ALIGN_RIGHT
      children = {
        flow = FLOW_HORIZONTAL
        //rendObj = ROBJ_DEBUG
        children
      }
    }
  }


  function targetQueues() {
    let children = []
    foreach (line_idx, line in targetsState.get()){
      foreach (step_idx, step in line){
        if (!step)
          continue
        children.append(targetSprites[line_idx][step_idx])
      }
    }
    return {
      watch = targetsState
      size = flex()
      children
    }
  }

  function viewer() {
    return {
      watch = isViewerShown
      size = flex()
      children = isViewerShown.get() ? saver : null
    }
  }

  const halfMissAnim = [
    { prop=AnimProp.opacity, from=1, to=0, duration=1, easing=InOutExp, play=true, loop=true, globalTimer=true }
  ]

  function missesBlock() {
    return {
      watch = [misses, isHalfMiss]
      size = flex()
      children = array(misses.get()).map(@(_, idx) {
        size = flex()
        children = missesSprites[idx]
        key = {}
        animations = (isHalfMiss.get() && idx==misses.get()-1) ? halfMissAnim : null
      })
    }
  }

  function failsEscapingBlock() {
    let children = [
      fails.get()[0] ? failsSprites[0] : null,
      fails.get()[1] ? failsSprites[1] : null,
    ]

    for (local side=0; side<2; ++side) {
      for (local step=0; step<NUM_ESCAPE_STEPS; ++step) {
        if (!escapes.get()[side][step])
          continue
        children.append(escapeSprites[side][step])
      }
    }

    return {
      watch = [fails, escapes]
      size = flex()
      children
    }
  }

  function gameModeBlock() {
    local child = null
    if (gameMode.get() != null) {
      child = {
        rendObj = ROBJ_TEXT
        hplace = gameMode.get() == GameMode.A ? ALIGN_LEFT : ALIGN_RIGHT
        text = gameMode.get() == GameMode.A ? "Game A" : "Game B"
        fontSize = sz(5)
        color = Color(0,0,0)
        padding = hdpx(5)
      }
    }

    return {
      watch = gameMode
      size = flex()
      valign = ALIGN_BOTTOM
      children = child
    }
  }

  let gameScreen = {
    size = [GAME_W, GAME_H]
    children = [
      {
        size = flex()
        rendObj = ROBJ_SOLID
        color = Color(192, 192, 192)
        children = [
          subject
          targetQueues
          viewer
          scoreBlock
          missesBlock
          failsEscapingBlock
          gameModeBlock
          overlay
        ]
        hotkeys
        function onAttach() {
          gui_scene.updateCounter.subscribe_with_nasty_disregard_of_frp_update(perFrameUpdate)
        }
        function onDetach() {
          gui_scene.updateCounter.unsubscribe(perFrameUpdate)
        }
      }
    ]
  }
  return {gameScreen, gameEvents}
})


function interpolateCoords(start, end, t) {
  return [start[0]+(end[0]-start[0])*t, start[1]+(end[1]-start[1])*t]
}

function makeLinePoints(start, end, steps) {
  return array(steps).map(@(_, idx) interpolateCoords(start, end, idx.tofloat()/(steps-1)))
}

return {
  makeSound
  makeGameScreen
  defaultKeys
  GameMode
  DirFlags
  Sector

  NUM_TARGET_LINES
  NUM_TARGET_STEPS
  NUM_ESCAPE_STEPS

  interpolateCoords
  makeLinePoints
  mkPicCtors
}

