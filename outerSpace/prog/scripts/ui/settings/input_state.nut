from "%darg/ui_imports.nut" import *
import "dagor.fs" as dagor_fs
import "dagor.system" as dagor_sys
import "dainput2" as dainput

let { loc } = require("%dngscripts/localizations.nut")
let { isPlatformRelevant, platformId } = require("%dngscripts/platform.nut")
let {startswith} = require("string")
let controlsList = {
  win32 = [
    "content/outer_space/config/outer_space.default"
  ]
}

if (controlsList?[platformId] == null)
   controlsList[platformId] <- [dainput.get_default_preset_prefix()]
let generation = Watched(0)
let nextGeneration = @() generation(generation.value + 1)
let haveChanges = mkWatched(persist, "haveChanges", false)

function mkPresetNameFromPresetPath(path){
  let split = path.split("/")
  local preset_name = split[split.len()-1]
  preset_name = preset_name.replace("outer_space.", "")
  return preset_name
}


function locPreset(name) {
  name = name?.name ?? mkPresetNameFromPresetPath(name)
  return loc($"controls/preset_{name}", loc(name))
}

let Preset = class {
  name = null
  preset = null
  constructor(preset_, name_ = null){
    this.name = name_ ?? locPreset(preset_)
    this.preset = preset_
  }
  function tostring(){
    return this.name
  }
  function value(){
    return this
  }
}

function gatherByBpath(path) {
  let use_realfs = (dagor_sys.DBGLEVEL > 0) ? true: false
  return dagor_fs.scan_folder({root=path, vromfs = true, realfs = use_realfs, recursive = false, files_suffix=".c0.preset.blk"})
    .map(@(v) startswith(v, "/") ? (v.slice(1)).replace(".c0.preset.blk", "") : v.replace(".c0.preset.blk", ""))
}
function lowerCaseSubstringInList(substring, list){
  let lowerCaseSubstring = substring.tolower()
  foreach(v in list){
    let lowerCaseValue = v.tolower()
    if (lowerCaseValue.indexof(lowerCaseSubstring)!=null)
      return true
  }
  return false
}

function gatherPresets(){
  local availablePresets = gatherByBpath("content/outer_space/config")
  availablePresets = controlsList?[platformId]?.filter(@(v) lowerCaseSubstringInList(v, availablePresets)) ?? []
  return availablePresets.map(@(v) Preset(v))
}
let availablePresets = Watched(gatherPresets())

let getActionTags = memoize(@(action_handler)
  dainput.get_group_tag_str_for_action(action_handler).split(",").map(@(v) v.replace(" ","")).filter(@(v) v!=""))

const platform_suffix = "platform="
function mkSubTagsFind(suffix){
  return memoize(function(action_handler){
    foreach (tag in getActionTags(action_handler)){
      if (startswith(tag, suffix)) {
        let tagData = tag.slice(suffix.len())
        return tagData.split("/").map(@(v) v.replace(" ","")).filter(@(v) v!="")
      }
    }
    return null
  })
}
let getSubTagsForPlatform = mkSubTagsFind(platform_suffix)
let isActionForPlatform = memoize(@(action_handler)
  isPlatformRelevant(getSubTagsForPlatform(action_handler) ?? [])
)

function getActionsList() {
  let res = []
  let total = dainput.get_actions_count()
  for (local i = 0; i < total; i++) {
    let ah = dainput.get_action_handle_by_ord(i)
    if (dainput.is_action_internal(ah) || !isActionForPlatform(ah))
      continue
    let actionType = dainput.get_action_type(ah)
    if ([dainput.TYPEGRP_DIGITAL, dainput.TYPEGRP_AXIS, dainput.TYPEGRP_STICK].indexof(actionType & dainput.TYPEGRP__MASK) != null)
      res.append(ah)
  }
  return res
}

let importantGroups = Watched([ "Movement", "Weapon", "View", "Vehicle" ])

return {
  importantGroups
  generation
  nextGeneration
  haveChanges
  availablePresets
  locPreset
  Preset
  controlsList

  getActionsList
  getActionTags

  mkSubTagsFind
}
