from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let entity_editor = require_optional("entity_editor")

let {get_scene_filepath=null} = entity_editor

return @(changed) @() {
  watch = [changed]
  pos = [0, hdpx(7)]
  behavior = Behaviors.Button
  onClick = @() null
  children = txt(get_scene_filepath?())
}
