module daEditor_common shared

require ecs
require strings
require EcsUtils
require DagorDataBlock
require DagorConsole
require DagorEditor

def editor_select_eid(eid : EntityId; select = true)
  entity_object_unselectAll()
  entity_object_editor_selectEntity(eid, select)

def editor_open(open : bool = true)
  console_command("daEd4.open {open ? 1 : 0}")


def editor_save_entity(eid : EntityId; var blk : DataBlock; override_template_name : string = "")
  blk |> datablock_add_new_block("entity") <| $(sub)
    var tName = empty(override_template_name) ? getEntityTemplateName(eid) : override_template_name
    tName = remove_sub_template_name(tName, "daeditor_selected")
    sub |> add("_template", tName)
    let list = get_active_scene() |> scene_findComponentsList(eid)
    if list != null
      *list |> components_to_blk(sub, "", false)
    else
      // entity wasn't added to scene
      eid |> components_to_blk(sub, "", false)