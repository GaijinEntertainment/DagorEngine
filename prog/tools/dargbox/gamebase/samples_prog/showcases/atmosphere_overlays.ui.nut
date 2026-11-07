from "%darg/ui_imports.nut" import *
from "dagor.math" import Color4
from "dagor.fs" import scan_folder
from "dagor.system" import get_arg_value_by_name
from "math" import clamp

let cursors = require("samples_prog/_cursors.nut")
let Rand = require("%sqstd/rand.nut")
let rand = Rand()

let imagesArg = get_arg_value_by_name("atmo_images")?.split("/") ?? []
let customImages = imagesArg.len() < 2 ? []
  : scan_folder({root = "/".join(imagesArg.slice(0, -1)), files_suffix = ".avif", recursive = false})
      .filter(@(f) f.split("/").top().startswith(imagesArg.top()))
customImages.sort()
let images = customImages.len() > 0 ? customImages
  : ["ui/loading.jpg", "ui/BG_1.jpg", "ui/campaign_plane.avif", "ui/campaign_tank.avif", "ui/campaign_ship.avif"]

const DESCRIPTION = "The atmo_* overlay shaders of prog/gameLibs/render/darg_fullscreen_background_shaders (its README.md lists their parameters) over a still image. A preset picks the layers, and their tint, density, speed, wind and pattern seed vary at random in a small range; Re-roll shows another variation. Start dargbox with -atmo_images:<folder>/<name prefix> to see your own .avif images, for example -atmo_images:D:/art/ui/login_"

const FX_IDS = ["fog", "smoke", "rain", "dust", "ash", "embers"]
const PANEL_FADE = [0.6, 0.95, 0.25]
const NO_FADE = [0.0, 0.0, 0.0]
const PRESETS_PER_ROW = 8

const fxDefaults = {
  embers = { tint = [1.0, 0.42, 0.1], aux = [0.012, 0.0] }
  ash = { tint = [0.45, 0.43, 0.41], aux = [0.008, 0.0] }
  dust = { tint = [1.0, 0.9, 0.74], aux = [0.45, 1.0] }
  fog = { tint = [0.74, 0.77, 0.8], aux = [0.012, 0.45] }
  smoke = { tint = [0.6, 0.57, 0.54], aux = [0.006, 0.0] }
  rain = { tint = [0.74, 0.79, 0.86], aux = [0.12, 0.0] }
}

const presets = {
  embers = [{ fx = "smoke", tint = [0.6, 0.52, 0.46], density = 0.7 }, { fx = "embers" }]
  emberStorm = [{ fx = "embers", density = 1.8, speed = 1.3, extra = 1.5, aux = [0.04, 0.0] }]
  redEmbers = [{ fx = "dust", tint = [1.0, 0.35, 0.3], extra = -1 }, { fx = "embers", tint = [1.0, 0.18, 0.12], density = 0.8 }]
  groundMist = [{ fx = "fog", aux = [0.012, 0.55] }]
  coldMist = [{ fx = "fog", tint = [0.7, 0.76, 0.84], density = 1.3, aux = [0.008, 0.35] },
    { fx = "dust", tint = [0.8, 0.86, 0.95], density = 0.6, extra = -1 }]
  warmHaze = [{ fx = "fog", tint = [0.72, 0.64, 0.56], aux = [0.01, 0.4] }, { fx = "dust", tint = [1.0, 0.85, 0.66], extra = -1 }]
  drizzle = [{ fx = "fog", density = 0.7, aux = [0.015, 0.5] }, { fx = "rain", density = 0.6, extra = 0.6 }]
  rainstorm = [{ fx = "fog", aux = [0.02, 0.45] }, { fx = "rain", density = 1.4, speed = 1.15, extra = 2.0, aux = [0.2, 0.0] }]
  sunDust = [{ fx = "dust", extra = 1.4 }]
  sunDustFromRight = [{ fx = "dust", extra = 1.4, aux = [-0.5, 1.0] }]
  darkDust = [{ fx = "dust", tint = [0.85, 0.88, 0.95], strength = 1.3, extra = -1 }]
  pollen = [{ fx = "dust", tint = [1.0, 0.95, 0.75], speed = 1.5, size = 1.4, extra = 0.6 }]
  anomalyAsh = [{ fx = "fog", density = 0.6, aux = [0.01, 0.5] }, { fx = "ash" }]
  ashfall = [{ fx = "ash", tint = [0.75, 0.73, 0.7], density = 1.4, speed = 0.8 }]
  battleHaze = [{ fx = "smoke" }, { fx = "dust", tint = [1.0, 0.82, 0.62], density = 0.8, extra = -1 }]
}

let presetNames = presets.keys().sort()

function resolveLayer(layer, jitter, windSign = 1.0) {
  let def = fxDefaults[layer.fx]
  let j = @(lo, hi) jitter ? rand.rfloat(lo, hi) : 1.0
  let tint = (layer?.tint ?? def.tint).map(@(c) c * j(0.94, 1.06))
  let aux = layer?.aux ?? def.aux
  let mirror = layer.fx == "dust" ? 1.0 : windSign
  return {
    fx = layer.fx
    p0 = [tint[0], tint[1], tint[2], (layer?.strength ?? 1.0) * j(0.85, 1.15)]
    p1 = [(layer?.density ?? 1.0) * j(0.75, 1.3), (layer?.speed ?? 1.0) * j(0.8, 1.2), (layer?.size ?? 1.0) * j(0.9, 1.15), layer?.extra ?? 1.0]
    p3 = [aux[0] * mirror * j(0.6, 1.4), aux[1] * j(0.92, 1.08), 0.0, jitter ? rand.rfloat(0.0, 100.0) : 0.0]
  }
}

function resolvePreset(name) {
  let windSign = rand.rint(0, 1) == 0 ? -1.0 : 1.0
  return presets[name].map(@(l) resolveLayer(l, true, windSign))
}

function mkLayer(r, strength, fade) {
  return {
    rendObj = ROBJ_SHADER
    size = flex()
    shaderName = $"atmo_{r.fx}"
    shaderParams = {
      params0 = Color4(r.p0[0], r.p0[1], r.p0[2], r.p0[3] * strength)
      params1 = Color4(r.p1[0], r.p1[1], r.p1[2], r.p1[3])
      params2 = Color4(fade[0], fade[1], fade[2], 0)
      params3 = Color4(r.p3[0], r.p3[1], r.p3[2], r.p3[3])
    }
  }
}

let imageIdx = Watched(0)
let presetName = Watched("embers")
let layers = Watched(resolvePreset("embers"))
let strength = Watched(1.0)
let panelFade = Watched(true)
let showUi = Watched(true)

let nextImage = @(d) imageIdx.set((imageIdx.get() + d + images.len()) % images.len())
let changeStrength = @(d) strength.set(clamp(strength.get() + d, 0.25, 3.0))
let hasFx = @(list, id) list.findindex(@(l) l.fx == id) != null

function applyPreset(name) {
  presetName.set(name)
  layers.set(resolvePreset(name))
}

function clearLayers() {
  presetName.set(null)
  layers.set([])
}

function toggleFx(id) {
  let cur = layers.get()
  presetName.set(null)
  layers.set(hasFx(cur, id) ? cur.filter(@(l) l.fx != id) : [...cur, resolveLayer({ fx = id }, false)])
}

function button(text, onClick, isOn = null, hotkeys = null) {
  let sf = Watched(0)
  return function() {
    let on = isOn == null ? false : isOn.get()
    return {
      watch = isOn == null ? sf : [sf, isOn]
      rendObj = ROBJ_BOX
      behavior = Behaviors.Button
      onElemState = @(v) sf.set(v)
      onClick
      hotkeys
      padding = const [hdpx(4), hdpx(10)]
      borderWidth = hdpx(1)
      borderRadius = hdpx(3)
      borderColor = on ? Color(230, 180, 90) : Color(90, 90, 90)
      fillColor = (sf.get() & S_HOVER) != 0 ? Color(60, 60, 60, 220) : Color(20, 20, 20, 200)
      children = { rendObj = ROBJ_TEXT, text, color = on ? Color(255, 220, 160) : Color(210, 210, 210) }
    }
  }
}

let label = @(text) { rendObj = ROBJ_TEXT, text, color = Color(160, 160, 160) }

let background = @() {
  watch = imageIdx
  size = flex()
  rendObj = ROBJ_IMAGE
  image = Picture(images[imageIdx.get()])
  keepAspect = KEEP_ASPECT_FILL
}

let overlays = @() {
  watch = [layers, strength, panelFade]
  size = flex()
  children = layers.get().map(@(l) mkLayer(l, strength.get(), panelFade.get() ? PANEL_FADE : NO_FADE))
}

let imageName = @() {
  watch = imageIdx
  rendObj = ROBJ_TEXT
  text = $"{imageIdx.get() + 1}/{images.len()} {images[imageIdx.get()].split("/").top()}"
  color = Color(230, 230, 230)
}

let strengthText = @() {
  watch = strength
  rendObj = ROBJ_TEXT
  text = $"strength x{strength.get()}"
  color = Color(230, 230, 230)
}

let mkRow = @(children) { flow = FLOW_HORIZONTAL, gap = hdpx(6), valign = ALIGN_CENTER, children }

let presetButtons = presetNames.map(@(name) button(name, @() applyPreset(name), Computed(@() presetName.get() == name)))
let presetRows = array((presetButtons.len() + PRESETS_PER_ROW - 1) / PRESETS_PER_ROW)
  .map(@(_, i) mkRow([label(i == 0 ? "Presets" : ""), ...presetButtons.slice(i * PRESETS_PER_ROW, (i + 1) * PRESETS_PER_ROW)]))

let controls = {
  rendObj = ROBJ_SOLID
  color = Color(0, 0, 0, 150)
  vplace = ALIGN_BOTTOM
  margin = sh(2)
  padding = hdpx(10)
  flow = FLOW_VERTICAL
  gap = hdpx(6)
  children = [
    {
      rendObj = ROBJ_TEXTAREA
      behavior = Behaviors.TextArea
      maxWidth = sh(90)
      text = DESCRIPTION
      color = Color(180, 180, 180)
    }
    mkRow([
      button("<", @() nextImage(-1), null, [["Left"]])
      button(">", @() nextImage(1), null, [["Right"]])
      imageName
    ])
    mkRow([label("Layers"), ...FX_IDS.map(@(id, i) button($"{i + 1} {id}", @() toggleFx(id),
      Computed(@() hasFx(layers.get(), id)), [[$"{i + 1}"]]))])
    ...presetRows
    mkRow([
      button("Re-roll preset (R)", @() applyPreset(presetName.get() ?? "embers"), null, [["R"]])
      button("Random preset (Space)", @() applyPreset(Rand.chooseRandom(presetNames)), null, [["Space"]])
      button("No layers", clearLayers)
    ])
    mkRow([
      button("-", @() changeStrength(-0.25))
      strengthText
      button("+", @() changeStrength(0.25))
      button("Fade under login panel", @() panelFade.set(!panelFade.get()), panelFade)
      button("Hide UI (H)", @() showUi.set(false), null, [["H"]])
    ])
  ]
}

let showUiCatcher = {
  size = flex()
  behavior = Behaviors.Button
  onClick = @() showUi.set(true)
  hotkeys = [["H"]]
}

return {
  rendObj = ROBJ_SOLID
  color = Color(0, 0, 0)
  size = flex()
  cursor = cursors.normal
  children = [
    background
    overlays
    @() {
      watch = showUi
      size = flex()
      children = showUi.get() ? controls : showUiCatcher
    }
  ]
}
