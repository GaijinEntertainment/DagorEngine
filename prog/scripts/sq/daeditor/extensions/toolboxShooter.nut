from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let cursors = require("%daeditor/components/cursors.nut")
let textButton = require("%daeditor/components/textButton.nut")

let fontawesome = freeze({ font = Fonts?.fontawesome ?? 0, fontSize = hdpxi(21) })
let faStyle = {textStyle = {normal = fontawesome}, boxStyle = {normal = {fillColor = Color(0,0,0,80)}}}
let fa = require("%darg/helpers/fontawesome.map.nut")
let faRefresh = { text = fa["refresh"], style = faStyle }

let {groupsList, updateGroupsList, mkGroupListItemName, mkGroupListItemTooltip,
     toggleGroupListItem} = require("toolboxShooterGroups.nut")

let {addToolboxOption, getToolboxState, setToolboxState,
     runToolboxCmd, toolboxStyles} = require("panelElem_buttonToolbox.nut")


let function toolboxCmd_togglePolyBattleAreas() {
  if (!getToolboxState("polyAreas"))
    runToolboxCmd("battleAreas.draw_active_poly_areas 1", null, "polyAreas", true)
  else
    runToolboxCmd("battleAreas.draw_active_poly_areas 0", null, "polyAreas", false)
}

let function toolboxCmd_toggleCapZonesPoly() {
  if (!getToolboxState("capZonesPoly"))
    runToolboxCmd("capzone.draw_active_poly_areas 1", null, "capZonesPoly", true)
  else
    runToolboxCmd("capzone.draw_active_poly_areas 0", null, "capZonesPoly", false)
}

let function toolboxCmd_toggleCapZones() {
  if (!getToolboxState("capZones"))
    runToolboxCmd("capzone.debug", null, "capZones", true)
  else
    runToolboxCmd("capzone.debug", null, "capZones", false)
}

let function toolboxCmd_toggleRespawns() {
  let mode = getToolboxState("respawns")
  if (mode == 1)
    runToolboxCmd("respbase.respbase_debug 1", "respbase.respbase_only_active_debug 0", "respawns", 2)
  else if (mode == 2)
    runToolboxCmd("respbase.respbase_debug 0", "respbase.respbase_only_active_debug 0", "respawns", 0)
  else
    runToolboxCmd("respbase.respbase_debug 0", "respbase.respbase_only_active_debug 1", "respawns", 1)
}

let function toolboxCmd_toggleGroups() {
  if (!getToolboxState("showGroups"))
    updateGroupsList()
  setToolboxState("showGroups", !getToolboxState("showGroups"))
}

let respawnsBtnText = function() {
  let mode = getToolboxState("respawns")
  return mode == 1 ? "Respawns +" : mode == 2 ? "Respawns..." : "Respawns"
}

let function groupsContent() {
  local childs = []
  childs.append(toolboxStyles.rowDiv)
  foreach (item in groupsList.value) {
    let itemParam = clone item
    childs.append({
      children = textButton(
        mkGroupListItemName(itemParam),
        function() {
          toggleGroupListItem(itemParam)

          if (getToolboxState("polyAreas")) {
            runToolboxCmd("battleAreas.draw_active_poly_areas 0")
            gui_scene.resetTimeout(0.01, @() runToolboxCmd("battleAreas.draw_active_poly_areas 1"))
          }
        },
        ((itemParam.active > 0) ? toolboxStyles.buttonStyleOn : toolboxStyles.buttonStyle).__merge({
          onHover = @(on) cursors.setTooltip(on ? mkGroupListItemTooltip(itemParam) : null)
        })
      )
    })
  }
  childs.append(toolboxStyles.rowDiv)
  return {
    flow = FLOW_VERTICAL
    pos = [hdpx(16), 0]
    gap = hdpx(5)
    watch = [groupsList]
    children = childs
  }
}

let function addToolboxOptions_Shooter() {
  addToolboxOption(@() getToolboxState("polyAreas"),    "polyAreas", false,    "PolyBattleAreas", @(_) toolboxCmd_togglePolyBattleAreas(), null, "Toggle show polygonal battle areas")
  addToolboxOption(@() false,                           null, null,            faRefresh,         @(_) runToolboxCmd("battleAreas.reinit_active_poly_areas"), null, null)
  addToolboxOption(@() getToolboxState("capZonesPoly"), "capZonesPoly", false, "CapZonesPoly",    @(_) toolboxCmd_toggleCapZonesPoly(), null, "Toggle show polygonal capture zones")
  addToolboxOption(@() false,                           null, null,            faRefresh,         @(_) runToolboxCmd("capzone.reinit_active_poly_areas"), null, null)
  addToolboxOption(@() getToolboxState("capZones"),     "capZones", false,     "CapZones",        @(_) toolboxCmd_toggleCapZones(), null, "Toggle show simple capture zones")
  addToolboxOption(@() getToolboxState("respawns") > 0, "respawns", 0,         respawnsBtnText,   @(_) toolboxCmd_toggleRespawns(), null, "Toggle show respawns")
  addToolboxOption(@() getToolboxState("showGroups"),   "showGroups", false,   "Groups Override", @(_) toolboxCmd_toggleGroups(), groupsContent, "Activate/deactivate groups for testing")
  addToolboxOption(@() false,                           null, null,            faRefresh,         @(_) updateGroupsList(), null, null)
}

return {
  addToolboxOptions_Shooter
}
