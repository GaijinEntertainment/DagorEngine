from "%darg/ui_imports.nut" import *
//from "string" import format
from "eventbus" import eventbus_send
from "game_creator.nut" import makeGameScreen, DirFlags, NUM_TARGET_LINES, NUM_TARGET_STEPS, NUM_ESCAPE_STEPS, makeLinePoints, mkPicCtors


function mkSprites(width=sh(100), aspect=5.0/3, baseImgPath = "samples_prog/showcases/electronika_im", skinPath = "nu_pogodi_img"){

  let {sz, makeFixedSprite, makePic, fontsSprites} = mkPicCtors(width, aspect, baseImgPath, skinPath)
  let sprites = {
    subject_left = makeFixedSprite("wolf_left", 50, {pos=[sz(-12), sz(13)]})
    subject_right = makeFixedSprite("wolf_right", 50, {pos=[sz(15), sz(13)]})
    subject_lt = makeFixedSprite("basket_lt", 25, {pos=[sz(-32), sz(0)]})
    subject_lb = makeFixedSprite("basket_lb", 28, {pos=[sz(-33), sz(23)]})
    subject_rt = makeFixedSprite("basket_rt", 22, {pos=[sz(38), sz(3)]})
    subject_rb = makeFixedSprite("basket_rb", 25, {pos=[sz(35), sz(24)]})
    saver = makeFixedSprite("hare", 30, {pos=[sz(21), 0]})
  }

  let pics = {
    target = makePic("egg", 10)
    grass_a = makePic("grass_a", 20)
    grass_b = makePic("grass_b", 40)
    miss = makePic("miss", 15)
    target_fail = makePic("egg_crash", 20)
    escapes = [
      makePic("chick_run_0", 15)
      makePic("chick_run_1", 11)
      makePic("chick_run_2", 11)
      makePic("chick_run_3", 11)
    ]
  }

  let missesSprites = array(3).map(@(_, idx) {
    size = flex()
    children = {
      rendObj = ROBJ_IMAGE
      image = pics.miss
      size = [sz(11), sz(11)]
      pos = [sz(115)-(sz(2)+sz(11))*idx, sz(24)]
      margin = hdpx(2)
    }
  })

  function makeSprite(pic, params) {
    return params.__merge({
      rendObj = ROBJ_IMAGE
      image = pic
      size = params?.size ?? SIZE_TO_CONTENT
      imageAffectsLayout = (params?.size != null) //< to infer sizes from a rasterized image
    })
  }

  let overlay = {
    size = flex()
    children = [
      makeFixedSprite("bg_left_0", 30, {pos=[0, sz(0)], hplace=ALIGN_LEFT vplace=ALIGN_TOP})
      makeFixedSprite("bg_left_1", 42, {pos=[0, sz(10)], hplace=ALIGN_LEFT vplace=ALIGN_TOP})
      makeFixedSprite("bg_left_2", 42, {pos=[0, sz(38)], hplace=ALIGN_LEFT vplace=ALIGN_TOP})
      makeFixedSprite("bg_right_0", 18, {pos=[0, sz(0)], hplace=ALIGN_RIGHT vplace=ALIGN_TOP})
      makeFixedSprite("bg_right_1", 38, {pos=[0, sz(13)], hplace=ALIGN_RIGHT vplace=ALIGN_TOP})
      makeFixedSprite("bg_right_2", 41, {pos=[0, sz(37)], hplace=ALIGN_RIGHT vplace=ALIGN_TOP})
      makeSprite(pics.grass_a, {size=[sz(18), sz(5)], pos=[sz(-20), sz(38)], hplace = ALIGN_CENTER, vplace= ALIGN_CENTER})
      makeSprite(pics.grass_a, {size=[sz(18), sz(3)], pos=[sz(18), sz(38)], hplace= ALIGN_CENTER, vplace= ALIGN_CENTER, transform={scale=[-1,1]}})
      makeSprite(pics.grass_a, {size=[sz(16), sz(3)], pos=[sz(-10), sz(42)], hplace= ALIGN_CENTER, vplace= ALIGN_CENTER, transform={scale=[-1,1]}})
      makeSprite(pics.grass_a, {size=[sz(18), sz(3)], pos=[sz(10), sz(41)],hplace= ALIGN_CENTER, vplace= ALIGN_CENTER, })
      makeSprite(pics.grass_b, {size=[sz(27), sz(4)], pos=[sz(-70), sz(42)],hplace= ALIGN_CENTER, vplace= ALIGN_CENTER, })
      makeSprite(pics.grass_b, {size=[sz(27), sz(4)], pos=[sz(70), sz(42)], hplace= ALIGN_CENTER, vplace= ALIGN_CENTER, })
    ]
  }

  let targetCoords = [
    makeLinePoints([sz(-69), sz(-20)], [sz(-45), sz(-4)], NUM_TARGET_STEPS),
    makeLinePoints([sz(71), sz(-19)], [sz(49), sz(-5)], NUM_TARGET_STEPS),
    makeLinePoints([sz(-70), sz(5)], [sz(-46), sz(19)], NUM_TARGET_STEPS),
    makeLinePoints([sz(71), sz(5)], [sz(49), sz(19)], NUM_TARGET_STEPS),
  ]

  let targetSprites = array(NUM_TARGET_LINES)
    .map(function(_, line_idx){
      let rotAngle = (line_idx & DirFlags.Right) ? -40 : 40

      return array(NUM_TARGET_STEPS).map(@(_, step_idx) {
        size = flex()
//        rendObj = ROBJ_DEBUG
        halign = ALIGN_CENTER
        valign = ALIGN_CENTER
        children = {
          rendObj = ROBJ_IMAGE
          image = pics.target
          size = [sz(5), sz(7)]
          pos = targetCoords[line_idx][step_idx]
          transform = {
            rotate = (step_idx+line_idx*3)*rotAngle
            pivot = const [0.5, 0.5]
        }
      }
    })
  })
  let failsSprites = [
    {
      size = flex()
      children = {
        rendObj = ROBJ_IMAGE
        image = pics.target_fail
        size = [sz(27), sz(10)]
        pos = [sz(30), sz(87)]
        transform = { scale=[-1,1] }
      }
    },
    {
      size = flex()
      children = {
        rendObj = ROBJ_IMAGE
        image = pics.target_fail
        size = [sz(27), sz(10)]
        pos = [sz(110), sz(87)]
      }
    }
  ]
  let escapeCoords = [
    [ [sz(25), sz(72)], [sz(20), sz(80)], [sz(10), sz(80)],  [sz(0), sz(80)] ],
    [ [sz(129), sz(73)], [sz(142), sz(80)], [sz(150), sz(80)],  [sz(159), sz(80)] ],
  ]
  let escapeSprites = array(2).map(@(_, side) array(NUM_ESCAPE_STEPS).map(function(__, step){
      let pos = escapeCoords[side][step]
      return {
        rendObj = ROBJ_IMAGE
        image = pics.escapes[step]
        size = SIZE_TO_CONTENT
        imageAffectsLayout = true //< to infer sizes from a rasterized image
        pos
        transform = { scale=[side==0 ? -1 : 1, 1] }
      }
    })
  )
  return sprites.__merge({targetSprites, overlay, missesSprites, failsSprites, escapeSprites, fontsSprites})
}

let aspect = 5.0/3
let width = sh(100)
let sprites = mkSprites(width, aspect)
let {gameScreen, gameEvents} = makeGameScreen({game_name="nu_pogodi", sprites, width, aspect})

function mkButton(text, action, params = const {}) {
  return {
    rendObj = ROBJ_BOX
    borderWidth = hdpx(5)
    behavior = Behaviors.Button
    borderColor = Color(50,0,0)
    fillColor = Color(200,20,20)
    padding = hdpx(10)
    children = {
      rendObj = ROBJ_TEXT
      text
      fontSize = sh(2)
      color = Color(255,255,255)
      size = SIZE_TO_CONTENT
    }
    onClick = action
  }.__merge(params)
}


let newGameButtons = {
  size = [flex(), SIZE_TO_CONTENT]
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  padding = hdpx(10)
  gap = hdpx(10)
  children = [
    mkButton("Game A", @() eventbus_send(gameEvents.gameA, null))
    mkButton("Game B", @() eventbus_send(gameEvents.gameB, null))
  ]
}

let hint = {
  rendObj = ROBJ_TEXT
  color = Color(160, 160, 160, 160)
  fontSize = hdpx(20)
  text = "Use Arrow keys or Insert/Home/PageUp/PageDown to move or A/B to start a new game"
}

let root = {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  children = [
    gameScreen
    newGameButtons
    hint
  ]
}

return root