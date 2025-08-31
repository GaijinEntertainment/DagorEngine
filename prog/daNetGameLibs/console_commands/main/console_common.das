module console_common shared
require ecs
require ecs.templates_common
require ecs.systems_common
require strings
require math.base
require DagorConsole
require DagorSystem
require daslib/strings_boost
require ECSGlobalTags


enum CommandArgHintType
  Default
  EntityId
  TemplateName
  SystemName


struct CommandArgDesc
  name : string
  hintType : CommandArgHintType
  defaultValue : string


struct FindConsoleHintEntityResult
  eid : EntityId
  camDistanceSq : float
  pos : float3


def is_probably_eid(str : string)
  if str == ""
    return false
  for c in str
    if !is_number(c)
      return false
  return true


def find_console_hint_entities(str : string;
                               threshold_cam_distance_sq : float;
                               hints_count : int) : array<FindConsoleHintEntityResult>
  var entities : array<FindConsoleHintEntityResult>

  let isEid = is_probably_eid(str)
  let isName = !isEid

  find_query() <| $ [es] (camera__active : bool;
                          transform aka camera_transform : float3x4)
    if camera__active
      let camPos = camera_transform[3]

      // Complete match eid entity
      let completeMatchEid = isEid ? EntityId(to_uint(str)) : INVALID_ENTITY_ID
      query(completeMatchEid) <| $ [es] (transform : float3x4 const?)
        let pos = transform != null ? (*transform)[3] : float3(VERY_BIG_NUMBER)
        let camDistanceSq = distance_sq(camPos, pos)
        entities |> emplace(FindConsoleHintEntityResult(eid = completeMatchEid, camDistanceSq = camDistanceSq, pos = pos))

      // Closest to camera entities
      query() <| $ [es] (eid : EntityId;
                         transform : float3x4)
        if eid == completeMatchEid
          return
        let pos = transform[3]
        let camDistanceSq = distance_sq(camPos, pos)
        if camDistanceSq <= threshold_cam_distance_sq
          if ((str == "") ||
              (isEid && find("{eid}", str) != -1) ||
              (isName && find(getEntityTemplateName(eid), str) != -1))
            entities |> emplace(FindConsoleHintEntityResult(eid = eid, camDistanceSq = camDistanceSq, pos = pos))
      entities |> sort() <| $(left : FindConsoleHintEntityResult; right : FindConsoleHintEntityResult)
        return left.camDistanceSq < right.camDistanceSq

      // Other entities
      find_query() <| $ [es] (eid : EntityId;
                              transform : float3x4 const?)
        if length(entities) >= hints_count
          return true
        if eid == completeMatchEid
          return false
        if ((str == "") ||
            (isEid && find("{eid}", str) != -1) ||
            (isName && find(getEntityTemplateName(eid), str) != -1))
          if transform != null
            let pos = (*transform)[3]
            let camDistanceSq = distance_sq(camPos, pos)
            if camDistanceSq > threshold_cam_distance_sq
              entities |> emplace(FindConsoleHintEntityResult(eid = eid, camDistanceSq = camDistanceSq, pos = pos))
          else
            entities |> emplace(FindConsoleHintEntityResult(eid = eid, pos = float3(VERY_BIG_NUMBER)))
        return false

      return true
    return false
  return <- entities


def build_entity_hint_desc(entity : FindConsoleHintEntityResult)
  let templateName = getEntityTemplateName(entity.eid)
  return "{templateName} [pos={entity.pos.x:.2f}, {entity.pos.y:.2f}, {entity.pos.z:.2f}] [dist={sqrt(entity.camDistanceSq):.2f}m]"


def build_command_up_to_argument(inputs : array<string>;
                                 arg_index : int)
  return build_string() <| $(var str)
    for i in range(min(arg_index + 1, length(inputs)))
      if i > 0
        str |> write(" ")
      str |> write(inputs[i])


def build_args_description(args_description : string;
                           arg_index : int)
  var start = -1
  for _ in range(arg_index + 1)
    start = args_description |> find(' ', start + 1)
  if start >= 0 && start < length(args_description)
    return args_description |> chop(start + 1, length(args_description) - start - 1)
  return ""


def add_hints_for_argument_default(inputs : array<string>;
                                   arg_index : int;
                                   hints_count : int;
                                   min_args : int;
                                   max_args : int;
                                   args_description : string;
                                   var hints : ConsoleProcessorHints)
  if length(inputs) != arg_index + 2
    return
  let cmdText = build_command_up_to_argument(inputs, arg_index)
  let argsText = build_args_description(args_description, arg_index - 1)
  if length(hints) < hints_count
    hints |> add_hint(cmdText,
                      max(0, min_args - arg_index - 1), max(0, max_args - arg_index - 1), argsText)


def add_hints_for_argument_template_name(inputs : array<string>;
                                         arg_index : int;
                                         hints_count : int;
                                         min_args : int;
                                         max_args : int;
                                         args_description : string;
                                         var hints : ConsoleProcessorHints)
  if length(inputs) != arg_index + 2
    return
  var argText = inputs[arg_index + 1]
  var cmdText = build_command_up_to_argument(inputs, arg_index) + " "
  let argsText = build_args_description(args_description, arg_index)
  let subTemplates <- argText |> split("+")
  if length(subTemplates) > 1 && subTemplates[length(subTemplates) - 1] != ""
    argText = subTemplates[length(subTemplates) - 1]
    cmdText = cmdText + chop(inputs[arg_index + 1], 0, length(inputs[arg_index + 1]) - length(argText))
  let templateNames <- (argText != "") ? find_template_names_sorted(argText) : get_first_template_names(hints_count)
  for i in range(min(length(templateNames), hints_count))
    hints |> add_hint("{cmdText}{templateNames[i]}",
                      max(0, min_args - arg_index - 1), max(0, max_args - arg_index - 1), argsText)

def add_hints_for_argument_system_name(inputs : array<string>;
                                       arg_index : int;
                                       hints_count : int;
                                       min_args : int;
                                       max_args : int;
                                       args_description : string;
                                       var hints : ConsoleProcessorHints)
  if length(inputs) != arg_index + 2
    return
  let argText = inputs[arg_index + 1]
  let cmdText = build_command_up_to_argument(inputs, arg_index)
  let argsText = build_args_description(args_description, arg_index)
  let systemNames <- (argText != "") ? find_system_names_sorted(argText) : get_first_system_names(hints_count)
  for i in range(min(length(systemNames), hints_count))
    hints |> add_hint("{cmdText} {systemNames[i]}",
                      max(0, min_args - arg_index - 1), max(0, max_args - arg_index - 1), argsText)


def add_hints_for_argument_eid(inputs : array<string>;
                               arg_index : int;
                               hints_count : int;
                               min_args : int;
                               max_args : int;
                               args_description : string;
                               threshold_cam_distance_sq : float;
                               var hints : ConsoleProcessorHints)
  if length(inputs) != arg_index + 2
    return
  let argText = inputs[arg_index + 1]
  let cmdText = build_command_up_to_argument(inputs, arg_index)
  let argsText = build_args_description(args_description, arg_index)
  let entities <- find_console_hint_entities(argText, threshold_cam_distance_sq, hints_count)
  for i in range(min(length(entities), hints_count))
    hints |> add_hint("{cmdText} {entities[i].eid}", max(0, min_args - arg_index - 1), max(0, max_args - arg_index - 1),
                      argsText, build_entity_hint_desc(entities[i]))
  if ecs_has_tag("net")
    hints |> add_hint("{cmdText} :hero:", max(0, min_args - arg_index - 1), max(0, max_args - arg_index - 1),
                        argsText)


def parse_argument_descriptions(arg_descs : auto;
                                var out_min_args : int&;
                                var out_max_args : int&;
                                var out_args_description : string&)
  out_min_args = 0
  out_max_args = 0
  out_args_description = build_string() <| $(var str)
    for i in iter_range(arg_descs)
      let argDesc = arg_descs[i]
      if i > 0
        str |> write(" ")
      if argDesc.defaultValue == ""
        str |> write("<{argDesc.name}>")
        out_min_args++
        out_max_args++
      else
        str |> write("[{argDesc.name}={argDesc.defaultValue}]")
        out_max_args++


def add_hints(arg_descs : auto;
              cmd : string;
              inputs : array<string>;
              hints_count : int;
              threshold_cam_distance_sq : float;
              var hints : ConsoleProcessorHints)
  var minArgs = 0
  var maxArgs = 0
  var argsDescription = ""
  parse_argument_descriptions(arg_descs, minArgs, maxArgs, argsDescription)

  if inputs[0] == cmd
    for i in iter_range(arg_descs)
      let argDesc = arg_descs[i]
      if argDesc.hintType == CommandArgHintType.Default
        add_hints_for_argument_default(inputs, i, hints_count, minArgs, maxArgs,
                                      argsDescription, hints)
      elif argDesc.hintType == CommandArgHintType.EntityId
        add_hints_for_argument_eid(inputs, i, hints_count, minArgs, maxArgs,
                                  argsDescription, threshold_cam_distance_sq, hints)
      elif argDesc.hintType == CommandArgHintType.TemplateName
        add_hints_for_argument_template_name(inputs, i, hints_count, minArgs, maxArgs,
                                            argsDescription, hints)
      elif argDesc.hintType == CommandArgHintType.SystemName
        add_hints_for_argument_system_name(inputs, i, hints_count, minArgs, maxArgs,
                                            argsDescription, hints)
      else
        logerr("Unsupported CommandArgHintType! hintType={argDesc.hintType}")

  if length(hints) == 0 && (inputs[0] == cmd || length(inputs) == 1)
    hints |> add_hint(cmd, minArgs, maxArgs, argsDescription)

