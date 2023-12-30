from "%darg/ui_imports.nut" import *

let math = require("math")

/* possible easings
Linear, InQuad, OutQuad, InOutQuad, InCubic, OutCubic, InOutCubic, InQuart, OutQuart, InOutQuart, InOutBezier, CosineFull, InStep, OutStep,
*/

let defParams = {num=30,emitter_sz=[500,200], part=null}
//let rnd_norm =@() math.rand().tofloat()/math.RAND_MAX
//let rnd_range =@(min,max) (max-min).tofloat()*math.rand()/math.RAND_MAX+min
//let rnd_sprd =@(std,spr) spr.tofloat()*math.rand()/math.RAND_MAX+std
let rnd_arange =@(range) (range[1]-range[0]).tofloat()*math.rand()/math.RAND_MAX+range[0]
let rndP = @(range) [rnd_arange(range),rnd_arange(range)]

function mkParticles(params=defParams){
  let particles=[]
  let part = {transform={pivot=[0.5,0.5]}}.__merge(params?.part ?? {})
  let emitter_sz= params?.emitter_sz ?? defParams.emitter_sz
  let rotEndSpr = params?.rotEndSpr ?? 180
  let rotStartSpr = params?.rotStartSpr ?? 80
  let duration = params?.duration ?? [0.5,1.5]
  let numParams = params?.num ?? defParams.num
  for (local i=0; i<numParams; i++) {
    let scaleTo = rndP([1.2,0.8])
    let partW = part?.size[0] ?? 10
    let partH = part?.size[1] ?? 10
    let posTo = [rnd_arange([0-partW/2,emitter_sz[0]-partW/2]), rnd_arange([0-partH/2,emitter_sz[1]-partH/2])]
    let rotateTo = rnd_arange([-rotEndSpr,rotEndSpr])
    let animations = [
      { prop=AnimProp.scale, from=rndP([0.2,0.4]), to=scaleTo, duration=rnd_arange(duration), play=true, easing=OutCubic }
      { prop=AnimProp.rotate, from=rnd_arange([-rotStartSpr,rotStartSpr])+rotateTo, to=rotateTo, duration=rnd_arange(duration), play=true, easing=OutCubic}
      { prop=AnimProp.translate, from=[emitter_sz[0]/2,emitter_sz[1]/2], to=posTo, duration=rnd_arange(duration), play=true, easing=OutCubic }
    ]

    let p = part.__merge({
      transform = {
        scale = scaleTo
        rotate = rotateTo
        translate = posTo
      }
      animations=animations
    })
    particles.append(p)
  }
  return {
    size = emitter_sz
    rendObj=ROBJ_FRAME
    children = particles
  }
}
let part = {rendObj=ROBJ_SOLID size=[120,120] transform={pivot=[0.5,0.5]} color=Color(0,0,0,20)}

function root() {
  return {
    valign = ALIGN_CENTER
    size=flex()
    rendObj=ROBJ_SOLID
    halign = ALIGN_CENTER
    color = Color(120,120,120)
    children = [
      mkParticles({num=20 part=part duration=[0.5,1.8]})
      mkParticles({num=8 rotEndSpr=30 rotStartSpr=30 duration=[0.3,0.9] part={rendObj=ROBJ_SOLID size=[300,100] transform={pivot=[0.5,0.5]} color=Color(0,0,0,20)}})
    ]
  }
}


return root