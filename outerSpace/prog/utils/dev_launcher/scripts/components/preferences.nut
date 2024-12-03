from "%darg/ui_imports.nut" import *

let {time, date} = require("datetime")
let {WatchedJsonFile} = require("watchedJsonFile.nut")
let {loadJson, saveJson} = require("%sqstd/json.nut")
let {makeVertScroll, mkButton, mkTextInput, dtext, hflow, cursors, vflow, textarea} = require("components.nut")
let {addModalWindow, removeModalWindow} = require("modalWindows.nut")
let fa = require("%darg/helpers/fontawesome.map.nut")

function timeToText(inttime){
  let parsedtime = date(inttime)
  let {year, month, day, hour, sec} = parsedtime
  return $"{year}-{month}-{day} {hour}:{parsedtime.min}:{sec}"
}

let presetListsWidth = sw(17)

let presetBtn = kwarg(function (sf, children, onClick, tooltip=null, override = null, onDoubleClick = null){
  let group = ElemGroup()
  return {
    clipChildren = true
    size = [flex(), SIZE_TO_CONTENT]
    children = {
      children
      flow = FLOW_VERTICAL
      size = [flex(), SIZE_TO_CONTENT]
      behavior = Behaviors.Marquee
      group
      scrollOnHover = true
    }
    behavior = Behaviors.Button
    onDoubleClick
    padding = hdpx(5)
    group
    rendObj = ROBJ_SOLID
    color = sf & S_HOVER ? Color(50,50,50) : Color(0,0,0,50)
    function onHover(on) {
      cursors.setTooltip(on ? tooltip : null)
    }
    onClick
  }.__update(override ?? {})
})

let Preferences = kwarg(function(
    selections,
    fileName="preferences.json",
    presetsFileName = "presets.json",
    saveLastChanges=true,
    loadLastChanges=true,
    filterKeysForPrefs = null,
    filterKeysForBuiltin = null,
    launchGame = null
  ) {
  let preferences = WatchedJsonFile(fileName)
  let builtinPresets = loadJson(presetsFileName) ?? []
  let builtinPresetsGen = Watched(0)
  function saveBuiltinPresets() {
    saveJson(presetsFileName, builtinPresets)
    builtinPresetsGen.modify(@(v) v + 1)
  }

  let lastChangesName = "##last##"
  let saveSelections = @() preferences.update({[lastChangesName] = {content = selections.map(@(s) s.get()), ctime = time(), atime = time()}})
  let lastSelections = preferences.fileContent.get()?[lastChangesName].content ?? {}
  if (loadLastChanges)
      lastSelections.each(@(v, k) selections?[k].set(v))

  function loadBuiltinPreset(preset_name){
    let preset = builtinPresets?.findvalue(@(v) v.name == preset_name).content
    if (preset!=null)
      preset.each(@(v,k) selections?[k].set(v))
  }

  let filterBuiltin = @(_value, key) filterKeysForBuiltin==null || !filterKeysForBuiltin.contains(key)

  function saveBuiltinPreset(name, description=""){
    builtinPresets.append({ content = selections.map(@(s) s.get()).filter(filterBuiltin), description, name})
    saveBuiltinPresets()
  }

  function deleteBuiltinPreset(p) {
    builtinPresets.remove(builtinPresets.findindex(@(v) v?.name == p?.name))
    saveBuiltinPresets()
  }

  function builtinPresetUp(p) {
    let idx = builtinPresets.findindex(@(v) v?.name == p?.name) ?? 0
    if (idx==0)
      return
    let preset = builtinPresets[idx]
    builtinPresets.remove(idx)
    builtinPresets.insert(idx-1, preset)
    saveBuiltinPresets()
  }

  function builtinPresetDwn(p) {
    let idx = builtinPresets.findindex(@(v) v?.name == p?.name) ?? (builtinPresets.len()-1)
    if (idx==builtinPresets.len()-1)
      return
    let preset = builtinPresets[idx]
    builtinPresets.remove(idx)
    builtinPresets.insert(idx+1, preset)
    saveBuiltinPresets()
  }

  let prefContent = preferences.fileContent
  let presets = Computed(@() prefContent.get()
      ?.filter(@(_, k) k!=lastChangesName)
      .map(@(v, name) {atime = v.atime, ctime=v.ctime, name})
      .values().sort(@(a,b) b.atime<=>a.atime) ?? []
    )


  function loadPreset(preset_name){
    let sels = preferences.fileContent.get()
    if (preset_name not in sels || preset_name == lastChangesName)
      return
    let preset = sels[preset_name]
    preset.content.each(@(v,k) selections?[k].set(v))
    preferences.update({[preset_name] = preset.__update({atime = time()})})
  }

  let filterPreset = @(_value, key) filterKeysForPrefs==null || !filterKeysForPrefs.contains(key)
  let savePreset = @(preset_name) preferences.update({
    [preset_name] = {
      content = selections.map(@(s) s.get()).filter(filterPreset), ctime = time(), atime = time()
    }
  })
  let deletePreset = @(preset) preferences.deleteKey(preset?.name)

  if (saveLastChanges)
    selections.values().each(@(v) v.watch.each(@(w) w.subscribe(@(...) saveSelections())))

  function mkPresetBtn(preset, idx){
    let {name, ctime} = preset
    idx = idx+1
    if (idx==10)
      idx = 0
    let hotkey = idx < 10 ? ["L.Alt", idx] : []
    let hotkeyStr = " ".join(hotkey)
    let hotkeyHint = "+".join(hotkey)
    let launchGameHotkey = hotkey.len()>0 ? $"L.Shift L.Ctrl {idx}" : ""
    let load = @() loadPreset(name)
    let loadAndLaunch = combine(load, launchGame)
    return watchElemState(@(sf) presetBtn({
        sf,
        children = [
          dtext(name, {color = sf & S_HOVER ? Color(255,255,255) : Color(180,180,180)})
          dtext(timeToText(ctime), {color = Color(127,127,127) fontSize = hdpx(13)})
        ],
        onClick = load
        onDoubleClick = loadAndLaunch
        tooltip=$"load preset '{name}'\n{hotkeyHint}\n{launchGameHotkey}"
        override = {hotkeys = hotkey.len()>0 ? [[$"{hotkeyStr}"], [launchGameHotkey, {action = loadAndLaunch}]] : null}
      })
    )
  }
  function mkBuiltinPresetBtn(preset, idx){
    idx = idx+1
    if (idx==10)
      idx = 0
    let hotkey = idx < 10 ? ["L.Ctrl", idx] : []
    let hotkeyStr = " ".join(hotkey)
    let hotkeyHint = "+".join(hotkey)
    let launchGameHotkey = hotkey.len()>0 ? $"L.Alt L.Ctrl {idx}" : ""
    let {name="", description=""} = preset
    let load = @() loadBuiltinPreset(name)
    let loadAndLaunch = combine(load, launchGame)
    return watchElemState(@(sf) presetBtn({
        sf,
        children = [
          dtext(name, {color = sf & S_HOVER ? Color(255,255,255) : Color(180,180,180)})
        ],
        onClick = load
        onDoubleClick = loadAndLaunch
        tooltip=$"load preset '{name}'\n{description}\n{hotkeyHint}\n{launchGameHotkey}",
        override = {hotkeys = [[$"{hotkeyStr}"], [launchGameHotkey, {action = loadAndLaunch}]] }
      })
    )
  }
  let mkMkEditPresetBtn = kwarg(@(onClickPreset, text = fa["close"], fontSize = hdpx(20), size = [hdpx(20), hdpx(30)], font = Fonts?.fontawesome) @(preset) watchElemState(@(sf){
    onClick = @() onClickPreset(preset)
    function onHover(on) {
      cursors.setTooltip(on ? $"delete preset {preset?.name ?? ""}" : null)
    }
    children = dtext(text, { color = sf & S_HOVER ? Color(255,255,255) : Color(128,128,128), font, fontSize})
    behavior = Behaviors.Button
    rendObj = ROBJ_SOLID
    color = sf & S_HOVER ? Color(0,0,0) : 0
    size
    valign = ALIGN_CENTER
    hplace = ALIGN_CENTER
    vplace = ALIGN_CENTER
    halign = ALIGN_CENTER
  }))

  let mkDelBuiltinPresetBtn = mkMkEditPresetBtn({onClickPreset=deleteBuiltinPreset})
  let mkDelPresetBtn = mkMkEditPresetBtn({onClickPreset = deletePreset})
  let mkBuiltinPresetUp = mkMkEditPresetBtn({onClickPreset= builtinPresetUp, fontSize = hdpx(20), text=fa["arrow-up"], size = [hdpx(20), hdpx(15)]})
  let mkBuiltinPresetDwn = mkMkEditPresetBtn({onClickPreset= builtinPresetDwn, fontSize = hdpx(20), text=fa["arrow-down"], size = [hdpx(20), hdpx(15)]})

  let PresetOptionsKey = "PresetOptionsKey"
  let closePresetWnd = @() removeModalWindow(PresetOptionsKey)

  function mkPresetWnd() {
    let name = Watched("")
    let okClick = function() {
      savePreset(name.get())
      closePresetWnd()
    }
    let ok = mkButton({text = "OK", onClick = okClick})
    return {
      key = PresetOptionsKey
      size = flex()
      flow = FLOW_VERTICAL
      rendObj = ROBJ_SOLID
      valign = ALIGN_CENTER
      halign = ALIGN_CENTER
      gap = hdpx(10)
      color = Color(0,0,0,220)
      children = [
        mkTextInput(name,  {title = "", placeholder = "name of new preset", size = [sh(40), SIZE_TO_CONTENT], onReturn=@() name.get() != "" ? okClick() : closePresetWnd() })
        hflow(
          @() { watch = name children = name.get() != "" ? ok : null}
          mkButton({text = "Cancel", onClick = closePresetWnd, hotkeys = [["Esc"]]})
        )
        {size = [flex(), hdpx(50)]}
        dtext("Or overwrite")
        {
          flow = FLOW_VERTICAL
          gap = {size= [flex(), hdpx(1)] rendObj = ROBJ_SOLID color =Color(80,80,80) margin = hdpx(3)}
          behavior = Behaviors.WheelScroll
          rendObj = ROBJ_SOLID
          color = Color(20,20,20)
          padding = hdpx(5)
          size = [sw(30), sh(50)]
          children = presets.get().map(@(v) {
            children = hflow(dtext(v.name), dtext(timeToText(v.ctime), {fontSize= hdpx(12), color = Color(128,128,128)}))
            behavior = Behaviors.Button
            onClick = @() name(v.name)
          })
        }
      ]
    }
  }
  function mkBuiltinPresetWnd() {
    let name = Watched("")
    let description = Watched("")
    let okClick = function() {
      saveBuiltinPreset(name.get(), description.get())
      closePresetWnd()
    }
    let ok = mkButton({text = "OK", onClick = okClick})
    return {
      key = PresetOptionsKey
      size = flex()
      flow = FLOW_VERTICAL
      rendObj = ROBJ_SOLID
      valign = ALIGN_CENTER
      halign = ALIGN_CENTER
      gap = hdpx(10)
      color = Color(0,0,0,220)
      children = [
        mkTextInput(name,  {title = "", placeholder = $"name of new preset", size = [sh(40), SIZE_TO_CONTENT]
          onReturn = @() name.get()!="" ? okClick() : closePresetWnd()
        })
        mkTextInput(description,  {title = "", placeholder = $"description of new preset", size = [sh(40), SIZE_TO_CONTENT]})
        hflow(
          @() { watch = name children = name.get() != "" ? ok : null}
          mkButton({text = "Cancel", onClick = closePresetWnd, hotkeys = [["Esc"]]})
        )
        {size = [flex(), hdpx(50)]}
        dtext("Or overwrite")
        {
          flow = FLOW_VERTICAL
          gap = {size= [flex(), hdpx(1)] rendObj = ROBJ_SOLID color =Color(80,80,80) margin = hdpx(3)}
          behavior = Behaviors.WheelScroll
          rendObj = ROBJ_SOLID
          color = Color(20,20,20)
          padding = hdpx(5)
          size = [sw(30), sh(50)]
          children = builtinPresets.map(@(v) {
            children = vflow(dtext(v.name), textarea(v.description, {fontSize= hdpx(12), color = Color(128,128,128)}))
            size = [flex(), SIZE_TO_CONTENT]
            behavior = Behaviors.Button
            onClick = @() name(v.name)
          })
        }
      ]
    }
  }
  let editPresetsMode = Watched(false)
  let presetsListContent = @() {
    watch = [presets, builtinPresetsGen, editPresetsMode]
    rendObj = ROBJ_SOLID
    color = Color(0,0,0,30)
    children = [
      watchElemState(@(sf){
        text = !editPresetsMode.get() ? "Edit built-in presets" : "Stop editting built-in presets" ,
        hplace = ALIGN_RIGHT onClick = @() editPresetsMode.modify(@(v)!v), rendObj = ROBJ_TEXT behavior = Behaviors.Button
        color = sf&S_HOVER ? Color(255,255,255) : Color(120,120,120), fontSize = hdpx(17)
      })
      ]
      .extend(builtinPresets.map(@(v, idx, collection){
          size = [flex(), SIZE_TO_CONTENT]
          flow = FLOW_HORIZONTAL
          gap = hdpx(2)
          children = [
            {
              flow  = FLOW_VERTICAL
              valign = ALIGN_CENTER
              vplace = ALIGN_CENTER
              gap = hdpx(4)
              children = [
                editPresetsMode.get() && idx>0 ? mkBuiltinPresetUp(v) : null
                editPresetsMode.get() && idx < collection.len()-1 ? mkBuiltinPresetDwn(v) : null
              ]
            }
            mkBuiltinPresetBtn(v, idx)
            editPresetsMode.get() ? mkDelBuiltinPresetBtn(v) : null
          ]
        })
      )
      .append(
        editPresetsMode.get()
          ? mkButton({text = "save new built-in", onClick = @() addModalWindow(mkBuiltinPresetWnd())})
          : null,
        {size = [0, hdpx(10)]},
        presets.get().len() > 0 ? dtext("My Presets") : null
      )
      .extend(presets.get().map(@(v, idx)
          {
            size = [flex(), SIZE_TO_CONTENT]
            flow = FLOW_HORIZONTAL
            gap = hdpx(5)
            children = [
              mkPresetBtn(v, idx),
              mkDelPresetBtn(v)
            ]
          }
        )
      )
      .append({size = [0, hdpx(10)]}, mkButton({text = "Save current preset", onClick = @() addModalWindow(mkPresetWnd())
        hotkeys = [["L.Ctrl S | R.Ctrl S "]]
        tooltip = "Ctrl+S"
        override = {hplace = ALIGN_CENTER}
      }))
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_VERTICAL
    padding =hdpx(10)
    gap = hdpx(5)
  }
  let presetsList = makeVertScroll(presetsListContent, {size = [presetListsWidth, flex()]})
  return {loadPreset, savePreset, deletePreset, mkPresetBtn, mkDelPresetBtn, presets, presetsList, closePresetWnd, mkPresetWnd}
})

return {
  Preferences
}