options no_aot
require ecs
require ecs.ecs_template
require ECSGlobalTags
require math.base
require DagorConsole
require DagorSystem
require ecs.templates_common
require strings
require daslib/strings_boost
require danetlibs.console_commands.main.console_common


[console_cmd(name="ecs.has_tag")]
def check_ecs_has_tag(tag_name : string)
  let msg = "ecs_has_tag(\"{tag_name}\") == {ecs_has_tag(tag_name)}"
  if ecs_has_tag("render")
    console_print(msg)
  else
    logerr(msg) // to see message from servers


def create(eid : uint; template_name : string; count = 1)
  var tm : float3x4
  var tmIsSet = false

  if eid == 0u
    find_query() <| $ [es] (camera__active : bool; transform : float3x4)
      if camera__active
        tmIsSet = true
        tm = transform
        tm[3] = tm[3] + tm |> rotate <| float3(0f, 0f, 2f)
      return camera__active

  if !tmIsSet
    query(EntityId(eid)) <| $ [es] (transform : float3x4)
      tmIsSet = true
      tm = transform
      tm[3] = tm[3] + tm[0] * 2f + tm[1]

  if !tmIsSet
    tm |> identity()

  for _ in range(count)
    let ceid = createEntity(template_name) <| $(var init : ComponentsInitializer)
      set(init, "transform", tm)
    if !!ceid
      console_print("{ceid}: creating entity <{template_name}>")


[console_processor]
def create_entity(args : array<string>;
                  collect : bool;
                  var hints : ConsoleProcessorHints)
  let cmd = "ecs.create_entity"
  if collect
    let inputs <- args[0] |> split(" ")
    add_hints([[auto[] [[CommandArgDesc name="template_name", hintType=CommandArgHintType TemplateName]];
                       [[CommandArgDesc name="eid", hintType=CommandArgHintType EntityId, defaultValue="0"]] ]],
              cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(50.0), hints)
  elif args[0] == cmd
    let argc = args |> length()
    let templateName = argc >= 2 ? args[1] : "#invalid_template_name"
    let eid = argc >= 3 ? to_uint(args[2]) : 0u
    create(eid, templateName)
    return true

  return false


[console_processor]
def create_entities(args : array<string>;
                    collect : bool;
                    var hints : ConsoleProcessorHints)
  let cmd = "ecs.create_entities"
  if collect
    let inputs <- args[0] |> split(" ")
    add_hints([[auto[] [[CommandArgDesc name="template_name", hintType=CommandArgHintType TemplateName]];
                       [[CommandArgDesc name="count", hintType=CommandArgHintType Default]];
                       [[CommandArgDesc name="eid", hintType=CommandArgHintType EntityId, defaultValue="0"]] ]],
              cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(50.0), hints)
  elif args[0] == cmd
    let argc = args |> length()
    let templateName = argc >= 2 ? args[1] : "#invalid_template_name"
    let count = argc >= 3 ? to_int(args[2]) : 1
    let eid = argc >= 4 ? to_uint(args[3]) : 0u
    create(eid, templateName, count)
    return true

  return false


def console_destroy_entity(eid : EntityId)
  if doesEntityExist(eid)
    console_print("{eid}: destroying entity <{getEntityTemplateName(eid)}>")
    destroyEntity(eid)
  else
    console_print("{eid}: non-existent entity")


[console_processor]
def destroy_entity_processor(args : array<string>;
                             collect : bool;
                             var hints : ConsoleProcessorHints)
  let cmd = "ecs.destroy_entity"
  if collect
    let inputs <- args[0] |> split(" ")
    add_hints([[auto[] [[CommandArgDesc name="eid", hintType=CommandArgHintType EntityId]];
                       [[CommandArgDesc name="last_eid", hintType=CommandArgHintType EntityId, defaultValue="0"]] ]],
              cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(50.0), hints)
    return false
  elif args[0] == cmd
    let argc = args |> length()
    let eid = argc >= 2 ? to_uint(args[1]) : 0u
    let lastEid = argc >= 3 ? to_uint(args[2]) : 0u
    if lastEid <= eid
      console_destroy_entity(EntityId(eid))
    else
      for i in urange(eid, lastEid)
        console_destroy_entity(EntityId(i))

    // Replace history command with initial input if the first arg is a template name
    // (It is more useful than a command with deleted eid in this case)
    let editTextBeforeModify = console_get_edit_text_before_modify()
    if editTextBeforeModify != ""
      let editTextBeforeModifyArgs <- editTextBeforeModify |> split(" ")
      if length(editTextBeforeModifyArgs) >= 2
        let arg0 = editTextBeforeModifyArgs[1]
        if to_uint(arg0) == 0u
          if console_top_history_command() |> starts_with(cmd)
            console_pop_front_history_command()
          console_add_history_command(editTextBeforeModify)

    return true

  return false


def sub_template_hints(cmd : string;
                       input : string;
                       var hints : ConsoleProcessorHints)
  let inputs <- input |> split(" ")
  add_hints([[auto[] [[CommandArgDesc name="eid", hintType=CommandArgHintType EntityId]];
                     [[CommandArgDesc name="template_name", hintType=CommandArgHintType TemplateName]] ]],
            cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(50.0), hints)


[console_processor]
def add_sub_template_processor(args : array<string>;
                               collect : bool;
                               var hints : ConsoleProcessorHints)
  let cmd = "ecs.add_sub_template"
  if collect
    sub_template_hints(cmd, args[0], hints)
    return false
  elif args[0] == cmd
    let argc = args |> length()
    let eid = argc >= 2 ? to_uint(args[1]) : 0u
    let templateName = argc >= 3 ? args[2] : "#invalid_template_name"
    addSubTemplate(EntityId(eid), templateName)
    return true

  return false


[console_processor]
def remove_sub_template_processor(args : array<string>;
                                  collect : bool;
                                  var hints : ConsoleProcessorHints)
  let cmd = "ecs.remove_sub_template"
  if collect
    sub_template_hints(cmd, args[0], hints)
    return false
  elif args[0] == cmd
    let argc = args |> length()
    let eid = argc >= 2 ? to_uint(args[1]) : 0u
    let templateName = argc >= 3 ? args[2] : "#invalid_template_name"
    removeSubTemplate(EntityId(eid), templateName)
    return true

  return false


def sub_template_all_hints(cmd : string;
                           input : string;
                           var hints : ConsoleProcessorHints)
  let inputs <- input |> split(" ")
  add_hints([[auto[] [[CommandArgDesc name="entities_template_name", hintType=CommandArgHintType TemplateName]];
                     [[CommandArgDesc name="template_name", hintType=CommandArgHintType TemplateName]] ]],
            cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(50.0), hints)


[console_processor]
def add_sub_template_all_processor(args : array<string>;
                                   collect : bool;
                                   var hints : ConsoleProcessorHints)
  let cmd = "ecs.add_sub_template_all"
  if collect
    sub_template_all_hints(cmd, args[0], hints)
    return false
  elif args[0] == cmd
    let argc = args |> length()
    let srcTemplateName = argc >= 2 ? args[1] : "#invalid_template_name"
    let templateName = argc >= 3 ? args[2] : "#invalid_template_name"
    query() <| $ [es] (eid : EntityId)
      if find(getEntityTemplateName(eid), srcTemplateName) != -1
        addSubTemplate(eid, templateName)
    return true

  return false


[console_processor]
def remove_sub_template_all_processor(args : array<string>;
                                      collect : bool;
                                      var hints : ConsoleProcessorHints)
  let cmd = "ecs.remove_sub_template_all"
  if collect
    sub_template_all_hints(cmd, args[0], hints)
    return false
  elif args[0] == cmd
    let argc = args |> length()
    let srcTemplateName = argc >= 2 ? args[1] : "#invalid_template_name"
    let templateName = argc >= 3 ? args[2] : "#invalid_template_name"
    query() <| $ [es] (eid : EntityId)
      if find(getEntityTemplateName(eid), srcTemplateName) != -1
        removeSubTemplate(eid, templateName)
    return true

  return false


[ecs_template]
struct sub_template_system_processor
  sub_template_system_processor__srcTemplateName : string
  sub_template_system_processor__templateName : string
  sub_template_system_processor__add : bool
  sub_template_system_processor__updateAt : float


[es(no_order)]
def sub_template_system_processor(act : UpdateStageInfoAct;
                                  sub_template_system_processor__srcTemplateName : string;
                                  sub_template_system_processor__templateName : string;
                                  sub_template_system_processor__add : bool;
                                  var sub_template_system_processor__updateAt : float&)
  if act.curTime >= sub_template_system_processor__updateAt
    sub_template_system_processor__updateAt = act.curTime + 0.2f
    query() <| $ [es] (eid : EntityId)
      if find(getEntityTemplateName(eid), sub_template_system_processor__srcTemplateName) != -1
        if sub_template_system_processor__add
          addSubTemplate(eid, sub_template_system_processor__templateName)
        else
          removeSubTemplate(eid, sub_template_system_processor__templateName)


def parse_sub_template_system_console_args(args : array<string>;
                                           var out_src_template_name : string&;
                                           var out_template_name : string&)
  let argc = args |> length()
  out_src_template_name = argc >= 2 ? args[1] : "#invalid_template_name"
  out_template_name = argc >= 3 ? args[2] : "#invalid_template_name"

  let srcTemplate = getTemplateByName(out_src_template_name)
  if srcTemplate == null
    console_print("Undefined entities_template_name - {out_src_template_name}!")
    return false
  let tpl = getTemplateByName(out_template_name)
  if tpl == null
    console_print("Undefined template_name - {out_template_name}!")
    return false
  return true


def start_sub_template_system(src_template_name : string;
                              template_name : string;
                              add : bool)
  let found = find_query() <| $ [es] (sub_template_system_processor__srcTemplateName : string;
                                      sub_template_system_processor__templateName : string;
                                      var sub_template_system_processor__add : bool&)
    if (sub_template_system_processor__srcTemplateName == src_template_name &&
        sub_template_system_processor__templateName == template_name)
      sub_template_system_processor__add = add
      console_print("sub_template_system_processor updated.")
      return true
    return false

  if !found
    createEntity("sub_template_system_processor") <| $(var init : ComponentsInitializer)
      set(init, "sub_template_system_processor__srcTemplateName", src_template_name)
      set(init, "sub_template_system_processor__templateName", template_name)
      set(init, "sub_template_system_processor__add", add)
    console_print("sub_template_system_processor created.")


[console_processor]
def start_add_sub_template_system_console_processor(args : array<string>;
                                                    collect : bool;
                                                    var hints : ConsoleProcessorHints)
  let cmd = "ecs.start_add_sub_template_system"
  if collect
    sub_template_all_hints(cmd, args[0], hints)
    return false
  elif args[0] == cmd
    var srcTemplateName : string
    var templateName : string
    if parse_sub_template_system_console_args(args, srcTemplateName, templateName)
      start_sub_template_system(srcTemplateName, templateName, true)
    return true
  return false


[console_processor]
def start_remove_sub_template_system_console_processor(args : array<string>;
                                                       collect : bool;
                                                       var hints : ConsoleProcessorHints)
  let cmd = "ecs.start_remove_sub_template_system"
  if collect
    sub_template_all_hints(cmd, args[0], hints)
    return false
  elif args[0] == cmd
    var srcTemplateName : string
    var templateName : string
    if parse_sub_template_system_console_args(args, srcTemplateName, templateName)
      start_sub_template_system(srcTemplateName, templateName, false)
    return true
  return false


[console_processor]
def stop_sub_template_system_console_processor(args : array<string>;
                                               collect : bool;
                                               var hints : ConsoleProcessorHints)
  let cmd = "ecs.stop_sub_template_system"
  if collect
    sub_template_all_hints(cmd, args[0], hints)
    return false
  elif args[0] == cmd
    var srcTemplateName : string
    var templateName : string
    if parse_sub_template_system_console_args(args, srcTemplateName, templateName)
      let found = find_query() <| $ [es] (eid : EntityId;
                                          sub_template_system_processor__srcTemplateName : string;
                                          sub_template_system_processor__templateName : string)
        if (sub_template_system_processor__srcTemplateName == srcTemplateName &&
            sub_template_system_processor__templateName == templateName)
          destroyEntity(eid)
          console_print("sub_template_system_processor removed.")
          return true
        return false
      if !found
        console_print("sub_template_system_processor is not found.")
    return true
  return false


[console_processor]
def does_system_process_template_cproc(args : array<string>;
                                       collect : bool;
                                       var hints : ConsoleProcessorHints)
  let cmd = "ecs.does_system_process_template"
  if collect
    let inputs <- args[0] |> split(" ")
    add_hints([[auto[] [[CommandArgDesc name="system_name", hintType=CommandArgHintType SystemName]];
                       [[CommandArgDesc name="template_name", hintType=CommandArgHintType TemplateName]] ]],
              cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(0.0), hints)
  //else actuall processing of this command happens in c++

  return false

[console_processor]
def does_system_process_entity_cproc(args : array<string>;
                                     collect : bool;
                                     var hints : ConsoleProcessorHints)
  let cmd = "ecs.does_system_process_entity"
  if collect
    let inputs <- args[0] |> split(" ")
    add_hints([[auto[] [[CommandArgDesc name="system_name", hintType=CommandArgHintType SystemName]];
                       [[CommandArgDesc name="eid", hintType=CommandArgHintType EntityId]] ]],
              cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(0.0), hints)
  //else actuall processing of this command happens in c++

  return false


[console_processor]
def does_system_process_selected_entity_cproc(args : array<string>;
                                              collect : bool;
                                              var hints : ConsoleProcessorHints)
  let cmd = "ecs.does_system_process_selected_entity"
  if collect
    let inputs <- args[0] |> split(" ")
    add_hints([[auto[] [[CommandArgDesc name="system_name", hintType=CommandArgHintType SystemName]]]],
              cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(0.0), hints)
  //else actuall processing of this command happens in c++

  return false

[console_processor]
def diff_templates_cproc(args : array<string>;
                         collect : bool;
                         var hints : ConsoleProcessorHints)
  let cmd = "ecs.diff_templates"
  if collect
    let inputs <- args[0] |> split(" ")
    add_hints([[auto[] [[CommandArgDesc name="template_name", hintType=CommandArgHintType TemplateName]];
                       [[CommandArgDesc name="template_name", hintType=CommandArgHintType TemplateName]] ]],
              cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(0.0), hints)
  //else actuall processing of this command happens in c++

  return false


[console_processor]
def diff_entities_cproc(args : array<string>;
                        collect : bool;
                        var hints : ConsoleProcessorHints)
  let cmd = "ecs.diff_entities"
  if collect
    let inputs <- args[0] |> split(" ")
    add_hints([[auto[] [[CommandArgDesc name="eid", hintType=CommandArgHintType EntityId]];
                       [[CommandArgDesc name="eid", hintType=CommandArgHintType EntityId]] ]],
              cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(0.0), hints)
  //else actuall processing of this command happens in c++

  return false


[console_processor]
def who_can_change_component_of_template_cproc(args : array<string>;
                                               collect : bool;
                                               var hints : ConsoleProcessorHints)
  let cmd = "ecs.who_can_change_component_of_template"
  if collect
    let inputs <- args[0] |> split(" ")
    add_hints([[auto[] [[CommandArgDesc name="component", hintType=CommandArgHintType Default]];
                       [[CommandArgDesc name="template", hintType=CommandArgHintType TemplateName]] ]],
              cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(0.0), hints)
  //else actuall processing of this command happens in c++

  return false

[console_processor]
def who_can_access_component_of_template_cproc(args : array<string>;
                                               collect : bool;
                                               var hints : ConsoleProcessorHints)
  let cmd = "ecs.who_can_access_component_of_template"
  if collect
    let inputs <- args[0] |> split(" ")
    add_hints([[auto[] [[CommandArgDesc name="component", hintType=CommandArgHintType Default]];
                       [[CommandArgDesc name="template", hintType=CommandArgHintType TemplateName]] ]],
              cmd, inputs, /*hints_count*/ 20, /*threshold_cam_distance_sq*/ square(0.0), hints)
  //else actuall processing of this command happens in c++

  return false


[console_cmd(name="update_from_das_templates")]
def update_from_das_templates()
  console_command("ecs.reload_templates")
  visual_log("ECS templates was reloaded. See console output to get more info")


[init]
def init_ecs_inspector_shortcut()
  console_command("consoleKeybindings.bind ctrl+shift+r update_from_das_templates")
